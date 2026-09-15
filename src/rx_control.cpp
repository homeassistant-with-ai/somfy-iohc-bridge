#include "rx_control.h"
#include "iohc_phy.h"
#include "iohc_frame.h"
#include "iohc_constants.h"
#include "display.h"
#include "net_control.h"
#include "secrets.h"
#include <string.h>

namespace rxctl {

// First 16 of the 20 sync bits (UART-wrapped FF followed by the first half
// of UART-wrapped 33) as the hardware sync word. Verified via test_phy.cpp:
// these bits pack exactly into the bytes 0x7F 0xD9.
static uint8_t g_sync_word[2] = {0x7F, 0xD9};
static const uint8_t RX_CAPTURE_LEN = 40; // ample for the largest frame (pairing, 31 bytes)

// Address of YOUR OWN original Somfy remote - lives in secrets.h
// (not in Git), so this code stays generic/reusable for anyone who
// forks this project. Filtering on this specific address prevents a
// neighbor's Somfy remote from throwing off our assumed position.
// See secrets.h.example for how to find this address yourself.
static const uint8_t KNOWN_REMOTE_SRC[3] = SOMFY_REMOTE_SRC;

static SX1262* g_radio = nullptr;
static volatile bool g_packet_flag = false;

#if defined(ESP32)
IRAM_ATTR
#endif
static void on_packet() {
  g_packet_flag = true;
}

static void log_hex(const char* label, const uint8_t* b, size_t n) {
  Serial.print(label); Serial.print(F(": "));
  for (size_t i = 0; i < n; i++) { if (b[i] < 0x10) Serial.print('0'); Serial.print(b[i], HEX); Serial.print(' '); }
  Serial.println();
}

void init(SX1262* radio) {
  g_radio = radio;
  radio->setPacketReceivedAction(on_packet);
}

// The radio alternates between TX (tx_control.cpp) and RX (here) - both
// sides therefore reset their own sync word/frame length EVERY time,
// instead of relying on a setting that only happens once.
void start_listening() {
  if (!g_radio) return;
  g_radio->setSyncWord(g_sync_word, sizeof(g_sync_word));
  g_radio->fixedPacketLengthMode(RX_CAPTURE_LEN);
  int state = g_radio->startReceive();
  Serial.print(F("[RX] startReceive(): "));
  Serial.println(state == RADIOLIB_ERR_NONE ? F("OK, listening on 868.95MHz...") : String(state));
}

void poll() {
  if (!g_packet_flag) return;
  g_packet_flag = false;

  uint8_t raw[RX_CAPTURE_LEN];
  int state = g_radio->readData(raw, RX_CAPTURE_LEN);
  float rssi = g_radio->getRSSI();

  Serial.println(F("[RX] === Sync word match! (hardware detected something on 868.95MHz) ==="));
  Serial.print(F("[RX] readData() status: ")); Serial.println(state);
  Serial.print(F("[RX] RSSI: ")); Serial.print(rssi); Serial.println(F(" dBm"));
  log_hex("[RX] Raw bytes after sync match", raw, RX_CAPTURE_LEN);

  // Reconstruct the full bit stream: the 16 known sync bits (already
  // "eaten" by the hardware), followed by the received raw bytes as
  // individual bits - then let our ALREADY TESTED phy_decode() do the rest.
  uint8_t sync20[20];
  iohc::phy_sync_bits(sync20);

  uint8_t bits[16 + RX_CAPTURE_LEN * 8];
  memcpy(bits, sync20, 16);
  size_t bi = 16;
  for (size_t i = 0; i < RX_CAPTURE_LEN; i++) {
    for (int b = 7; b >= 0; b--) bits[bi++] = (uint8_t)((raw[i] >> b) & 1);
  }

  uint8_t decoded[48];
  size_t n = iohc::phy_decode(bits, sizeof(bits), decoded, sizeof(decoded));

  if (n == 0) {
    Serial.println(F("[RX] phy_decode: nothing decoded (unexpected after a hardware sync match)"));
  } else {
    log_hex("[RX] Decoded frame", decoded, n);
    bool crc_ok = iohc::validate_crc(decoded, n);
    Serial.print(F("[RX] CRC valid: "));
    Serial.println(crc_ok ? F("YES - this is a real io-homecontrol frame!") : F("no (could be noise, or a decode error)"));
    if (crc_ok && n >= 8) {
      log_hex("[RX]   dest", &decoded[2], 3);
      log_hex("[RX]   src ", &decoded[5], 3);
      Serial.print(F("[RX]   cmd  : 0x")); Serial.println(decoded[8], HEX);
      if (n > 9) log_hex("[RX]   data", &decoded[9], n - 9 > 6 ? 6 : n - 9);

      // Only react to broadcast button frames (cmd 0x00) from the KNOWN
      // Situo, matching our own transmission scheme - this is exactly the
      // frame type we verified live (01 43 {00|C8|D2} 00 00 00).
      bool is_broadcast = memcmp(&decoded[2], iohc::BROADCAST_ADDR, 3) == 0;
      bool is_known_src = memcmp(&decoded[5], KNOWN_REMOTE_SRC, 3) == 0;
      bool is_activate  = decoded[8] == iohc::CMD_ACTIVATE_FUNCTION;
      // data layout (after cmd at decoded[8]): [9]=Originator, [10]=ACEI/vendor, [11]=MainParam byte0 (=button_code)
      if (is_broadcast && is_known_src && is_activate && n >= 12) {
        {
          uint8_t button_code = decoded[11];
          const char* label = nullptr;
          ui::Position pos = ui::Position::UNKNOWN;
          const char* mqtt_state = nullptr;
          if (button_code == iohc::BTN_OPEN)       { label = "OPEN";  pos = ui::Position::OPEN;        mqtt_state = "open"; }
          else if (button_code == iohc::BTN_CLOSE) { label = "CLOSE"; pos = ui::Position::CLOSED;      mqtt_state = "closed"; }
          else if (button_code == iohc::BTN_STOP)  { label = "STOP";  pos = ui::Position::STOPPED_MID; }
          if (label) {
            Serial.print(F("[RX] *** External control detected (original Situo): "));
            Serial.print(label);
            Serial.println(F(" - position tracking updated ***"));
            ui::set_action(ui::Action::NONE); // no action animation, we didn't send this ourselves
            ui::set_position(pos);
            ui::display_render();
            if (mqtt_state) netctl::publish_state(mqtt_state);
          }
        }
      }
    }
  }

  start_listening(); // re-arm for the next packet
}

} // namespace rxctl
