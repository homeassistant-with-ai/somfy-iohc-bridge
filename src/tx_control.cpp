#include "tx_control.h"
#include "iohc_constants.h"
#include "iohc_commands.h"
#include "iohc_phy.h"
#include "display.h"
#include "nvs_store.h"
#include "rx_control.h"
#include <string.h>

namespace txctl {

using namespace iohc;

static const uint8_t PHY_PREAMBLE_BYTES = 8; // 64-bit own 0x55 preamble in the payload

static SX1262* g_radio = nullptr;
static bool g_paired = false;
static uint8_t g_src[3] = {0};
static uint8_t g_install_key[16] = {0};
static uint16_t g_seq = 0;

static void fill_random(uint8_t* buf, size_t len) {
  for (size_t i = 0; i < len; i++) buf[i] = (uint8_t)g_radio->random(256);
}

static bool tx_raw_frame(const uint8_t* frame, size_t frame_len) {
  uint8_t packed[MAX_TX_FRAME_SIZE * 2];
  size_t needed = phy_encoded_size(frame_len, PHY_PREAMBLE_BYTES);
  if (needed > sizeof(packed)) {
    Serial.println(F("[TX] ERROR: buffer too small"));
    return false;
  }
  size_t n = phy_encode(frame, frame_len, PHY_PREAMBLE_BYTES, packed, sizeof(packed));
  if (n == 0) {
    Serial.println(F("[TX] ERROR: phy_encode failed"));
    return false;
  }
  // Radio alternates between RX (rx_control.cpp) and TX (here) - so the
  // sync word/length must be set again EVERY time, regardless of what the
  // RX side last set.
  uint8_t tx_sync[1] = {0x12};
  g_radio->setSyncWord(tx_sync, 1);
  g_radio->fixedPacketLengthMode((uint8_t)n);
  int state = g_radio->transmit(packed, n);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[TX] transmit() ERROR, code ")); Serial.println(state);
    return false;
  }
  return true;
}

static void check(const char* label, int state) {
  Serial.print(F("[TXCTL] ")); Serial.print(label); Serial.print(F(": "));
  Serial.println(state == RADIOLIB_ERR_NONE ? F("OK") : String(state));
}

void init(SX1262* radio) {
  g_radio = radio;

  // Raw bit-accurate transmission: disable everything the radio chip itself
  // would add/change to our bytes (CRC, whitening) - we already handle our
  // own CRC/preamble/sync word in the payload (iohc_phy.h).
  check("setCRC(0)", g_radio->setCRC(0));
  check("setWhitening(false)", g_radio->setWhitening(false));
  check("setPreambleLength(16)", g_radio->setPreambleLength(16)); // hardware preamble
                                   // kept minimal; our OWN 0x55 preamble in the payload does the job
  uint8_t sync[1] = {0x12};       // arbitrary, non-protocol-bound value:
  check("setSyncWord", g_radio->setSyncWord(sync, 1)); // only used to kick off THIS radio's TX

  // Restore a previously paired identity from NVS flash, so a reboot does
  // not require a fresh PROG action (phase 5, step 9).
  store::Identity id = store::load();
  if (id.paired) {
    memcpy(g_src, id.src, 3);
    memcpy(g_install_key, id.install_key, 16);
    g_seq = id.seq;
    g_paired = true;
    Serial.println(F("[TXCTL] Previously paired identity restored from NVS flash"));
  } else {
    Serial.println(F("[TXCTL] No stored identity found - pairing needed"));
  }
}

bool is_paired() { return g_paired; }

void get_src_address(uint8_t out[3]) { memcpy(out, g_src, 3); }

bool perform_pairing() {
  if (!g_radio) return false;

  fill_random(g_src, 3);
  // Avoid the broadcast address and the "all zero" address as our own address.
  if ((g_src[0] == 0 && g_src[1] == 0) || (g_src[0] == 0xFF && g_src[1] == 0xFF && g_src[2] == 0xFF)) {
    g_src[0] |= 0x01;
  }
  fill_random(g_install_key, 16);
  g_seq = (uint16_t)g_radio->random(65536);

  Serial.print(F("[PAIR] New own address: "));
  for (int i = 0; i < 3; i++) { if (g_src[i] < 0x10) Serial.print('0'); Serial.print(g_src[i], HEX); }
  Serial.println();

  ui::set_link_state(ui::LinkState::PAIRING);
  ui::display_render();

  uint8_t frame[MAX_TX_FRAME_SIZE];
  bool all_ok = true;

  // Step 1: 0x39 "Remove 1W Controller" announce - 1x LPM + 3x plain, 14ms apart
  // (docs/linklayer.md 1W Discovery: first request exclusion, then send the key)
  Serial.println(F("[PAIR] Sending 0x39 (announce)..."));
  size_t n = build_remove_controller_frame(g_src, BROADCAST_ADDR, g_seq, g_install_key,
                                            frame, sizeof(frame), /*lpm=*/true);
  all_ok &= tx_raw_frame(frame, n);
  for (int rep = 0; rep < 3; rep++) {
    delay(14);
    n = build_remove_controller_frame(g_src, BROADCAST_ADDR, g_seq, g_install_key,
                                       frame, sizeof(frame), /*lpm=*/false);
    all_ok &= tx_raw_frame(frame, n);
  }

  delay(40);

  // Step 2: 0x30 key transfer to broadcast - 1x LPM + 3x plain
  Serial.println(F("[PAIR] Sending 0x30 (key transfer)..."));
  n = build_pairing_frame(g_src, BROADCAST_ADDR, g_install_key, g_seq, frame, sizeof(frame), /*lpm=*/true);
  all_ok &= tx_raw_frame(frame, n);
  for (int rep = 0; rep < 3; rep++) {
    delay(14);
    n = build_pairing_frame(g_src, BROADCAST_ADDR, g_install_key, g_seq, frame, sizeof(frame), /*lpm=*/false);
    all_ok &= tx_raw_frame(frame, n);
  }

  g_seq++;
  // 1W gives no confirmation back - "paired" here only means we
  // successfully TRANSMITTED, not that the motor actually accepted it.
  g_paired = all_ok;
  ui::set_link_state(g_paired ? ui::LinkState::PAIRED : ui::LinkState::NOT_PAIRED);
  ui::display_render();

  if (g_paired) {
    store::Identity id{};
    id.paired = true;
    memcpy(id.src, g_src, 3);
    memcpy(id.install_key, g_install_key, 16);
    id.seq = g_seq;
    store::save(id);
    Serial.println(F("[TXCTL] Identity saved to NVS flash"));
  }

  Serial.println(all_ok ? F("[PAIR] Transmission complete (no confirmation possible in 1W mode)")
                         : F("[PAIR] Something went wrong while transmitting"));
  rxctl::start_listening();
  return all_ok;
}

bool send_button(uint8_t button_code) {
  if (!g_radio || !g_paired) return false;

  uint8_t frame[MAX_TX_FRAME_SIZE];
  bool all_ok = true;

  size_t n = build_button_frame(g_src, BROADCAST_ADDR, button_code, g_seq, g_install_key,
                                 frame, sizeof(frame), /*lpm=*/true);
  all_ok &= tx_raw_frame(frame, n);
  for (int rep = 0; rep < 5; rep++) {
    delay(14);
    n = build_button_frame(g_src, BROADCAST_ADDR, button_code, g_seq, g_install_key,
                            frame, sizeof(frame), /*lpm=*/false);
    all_ok &= tx_raw_frame(frame, n);
  }
  g_seq++;
  store::save_seq(g_seq); // prevents a sequence number from being reused after reboot/crash
  rxctl::start_listening();
  return all_ok;
}

} // namespace txctl
