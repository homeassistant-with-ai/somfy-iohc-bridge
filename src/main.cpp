/**
 * Phase 1/5 bring-up test — Heltec WiFi LoRa32 V3 (ESP32-S3 + SX1262)
 *
 * Goal: verify only the radio hardware. Transmits NOTHING, receives nothing
 * from real Somfy equipment. Init radio, set frequency to io-homecontrol
 * channel 2 (868.95 MHz), print SNR/RSSI/RNG, go to standby.
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <string.h>
#include "iohc_crypto.h"
#include "iohc_constants.h"
#include "iohc_frame.h"
#include "iohc_commands.h"
#include "display.h"
#include "tx_control.h"
#include "net_control.h"
#include "rx_control.h"

static const int PIN_BOOT_BUTTON = 0; // Heltec V3 onboard BOOT button

static bool eq(const uint8_t* a, const uint8_t* b, size_t n) { return memcmp(a, b, n) == 0; }

static void print_hex(const char* label, const uint8_t* b, size_t n) {
  Serial.print(label); Serial.print(F(": "));
  for (size_t i = 0; i < n; i++) { if (b[i] < 0x10) Serial.print('0'); Serial.print(b[i], HEX); }
  Serial.println();
}

/**
 * Runs the crypto core examples from iown-home docs/linklayer.md on this
 * board's REAL mbedTLS/AES implementation (not host-OpenSSL as in
 * test/test_crypto.cpp). Transmits nothing, uses no real keys.
 */
static bool run_crypto_self_test() {
  using namespace iohc;
  bool all_ok = true;

  // Vector 1: 1W key-transfer encryption
  {
    uint8_t node[3] = {0xAB, 0xCD, 0xEF};
    uint8_t key_to_send[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16};
    uint8_t iv[16], cipher[16];
    build_iv_key_transfer(node, iv);
    encrypt_key_for_transfer(TRANSFER_KEY, iv, key_to_send, cipher);
    uint8_t expected[16] = {0x7E,0x60,0x49,0x1F,0x97,0x6A,0xDF,0x65,0x3D,0xB0,0xED,0x78,0x5E,0x49,0xA2,0x01};
    bool ok = eq(cipher, expected, 16);
    print_hex("[CRYPTO] key-transfer ciphertext", cipher, 16);
    Serial.print(F("[CRYPTO] Test 1 (key-transfer AES): ")); Serial.println(ok ? F("PASS") : F("FAIL"));
    all_ok &= ok;
  }

  // Vector 2 + 3: IV checksum + full 1W-auth IV
  {
    uint8_t payload[7] = {0x00,0x01,0x43,0xD2,0x00,0x00,0x00};
    uint8_t iv[16];
    build_iv_1w_auth(payload, sizeof(payload), 0x0599, iv);
    uint8_t expected[16] = {0x00,0x01,0x43,0xD2,0x00,0x00,0x00,0x55, 0x05,0x00, 0x05,0x99, 0x55,0x55,0x55,0x55};
    bool ok = eq(iv, expected, 16);
    print_hex("[CRYPTO] 1W-auth IV", iv, 16);
    Serial.print(F("[CRYPTO] Test 2 (1W-auth IV incl. checksum): ")); Serial.println(ok ? F("PASS") : F("FAIL"));
    all_ok &= ok;
  }

  return all_ok;
}

/**
 * Builds (but does NOT transmit) a STOP button frame and a pairing frame,
 * and compares the result against the host-verified values. This proves
 * that the frame-building code produces the same result on the real ESP32
 * toolchain as it does on the host (g++/OpenSSL).
 */
static bool run_frame_self_test() {
  using namespace iohc;
  bool all_ok = true;

  uint8_t src[3]  = {0x48, 0x5B, 0x37};
  uint8_t dest[3] = {0x00, 0x00, 0x3F};
  uint8_t dummy_key[16] = {0};
  uint8_t out[MAX_TX_FRAME_SIZE];
  size_t n = build_button_frame(src, dest, BTN_STOP, 0x0000, dummy_key, out, sizeof(out));
  uint8_t expected_header[] = {0xF6,0x00, 0x00,0x00,0x3F, 0x48,0x5B,0x37, 0x00, 0x01,0x43,0xD2,0x00,0x00,0x00};
  bool ok1 = (n == 25) && eq(out, expected_header, sizeof(expected_header));
  print_hex("[FRAME] STOP frame", out, n);
  Serial.print(F("[FRAME] Test 1 (STOP header matches real capture): ")); Serial.println(ok1 ? F("PASS") : F("FAIL"));
  all_ok &= ok1;

  uint8_t pair_src[3] = {0xAB, 0xCD, 0xEF};
  uint8_t key[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16};
  size_t n2 = build_pairing_frame(pair_src, dest, key, 0x1234, out, sizeof(out));
  bool ok2 = (n2 == 31) && out[9] == 0x7E && out[10] == 0x60;
  print_hex("[FRAME] pairing frame", out, n2);
  Serial.print(F("[FRAME] Test 2 (pairing frame length+key): ")); Serial.println(ok2 ? F("PASS") : F("FAIL"));
  all_ok &= ok2;

  return all_ok;
}

// Pins for Heltec WiFi LoRa32 V3, verified against ropg/heltec_esp32_lora_v3
// and Velocet/LoRa32 (ARDUINO_HELTEC_WIFI_LORA_32_V3 block).
static const int PIN_VEXT = 36;
static const int PIN_SS   = 8;
static const int PIN_MOSI = 10;
static const int PIN_MISO = 11;
static const int PIN_SCK  = 9;
static const int PIN_DIO1 = 14;
static const int PIN_RST  = 12;
static const int PIN_BUSY = 13;

SX1262 radio = new Module(PIN_SS, PIN_DIO1, PIN_RST, PIN_BUSY);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println(F("=== Somfy io-homecontrol bridge: Phase 1 hardware bring-up ==="));
  Serial.println(F("Board: Heltec WiFi LoRa32 V3 (ESP32-S3 + SX1262)"));

  ui::display_init();
  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);

  Serial.println(F("--- Crypto self-test (mbedTLS, against docs/linklayer.md examples) ---"));
  bool crypto_ok = run_crypto_self_test();
  Serial.println(crypto_ok ? F("--- Crypto self-test: ALL OK ---") : F("--- Crypto self-test: FAILED! stopping ---"));
  if (!crypto_ok) { while (true) delay(1000); }

  Serial.println(F("--- Frame self-test (against host-verified values) ---"));
  bool frame_ok = run_frame_self_test();
  Serial.println(frame_ok ? F("--- Frame self-test: ALL OK ---") : F("--- Frame self-test: FAILED! stopping ---"));
  if (!frame_ok) { while (true) delay(1000); }

  // Enable Vext (LOW = on) - needed on some V3 boards; the radio itself
  // isn't on Vext but this costs nothing and avoids any doubt about it.
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW);
  delay(50);

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);

  // The SX1262 has a SEPARATE init for FSK vs LoRa (begin() = LoRa modem).
  // io-homecontrol is 2-FSK, so we initialize directly with beginFSK().
  // Parameters (source: iown-home docs/radio.md + src/IoHome.cpp):
  //   freq=868.95 MHz (channel 2), bitrate=38.4 kbps, freqDev=19.2 kHz,
  //   rxBw=117.3 kHz (nearest standard value above the Carson bandwidth
  //   of ~2*(19.2+38.4/2)=~77.4 kHz), power=10 dBm (provisional, no TX yet).
  Serial.print(F("[RADIO] beginFSK(868.95MHz, 38.4kbps, 19.2kHz dev, 117.3kHz rxBw)... "));
  int state = radio.beginFSK(868.95, 38.4, 19.2, 117.3, 10, 16);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("FAILED, code "));
    Serial.println(state);
    Serial.println(F("Check wiring/pins and try again."));
    ui::set_radio_state(ui::RadioState::FAILED);
    ui::display_render();
    while (true) { delay(1000); }
  }
  Serial.println(F("OK"));
  ui::set_radio_state(ui::RadioState::OK);

  Serial.print(F("[RADIO] SNR  (dB) : ")); Serial.println(radio.getSNR());
  Serial.print(F("[RADIO] RSSI (dBm): ")); Serial.println(radio.getRSSI());
  Serial.print(F("[RADIO] RNG 0-100 : ")); Serial.println(radio.random(100));

  state = radio.setEncoding(RADIOLIB_ENCODING_NRZ);
  Serial.print(F("[RADIO] setEncoding(NRZ): "));
  Serial.println(state == RADIOLIB_ERR_NONE ? F("OK") : String(state));

  state = radio.setDataShaping(RADIOLIB_SHAPING_NONE);
  Serial.print(F("[RADIO] setDataShaping(NONE): "));
  Serial.println(state == RADIOLIB_ERR_NONE ? F("OK") : String(state));

  state = radio.standby();
  Serial.print(F("[RADIO] standby(): "));
  Serial.println(state == RADIOLIB_ERR_NONE ? F("OK") : F("FAILED"));

  txctl::init(&radio);
  ui::set_link_state(txctl::is_paired() ? ui::LinkState::PAIRED : ui::LinkState::NOT_PAIRED);
  ui::set_position(ui::Position::UNKNOWN);
  ui::display_render();

  netctl::init();

  rxctl::init(&radio);
  rxctl::start_listening();

  Serial.println(F("=== Bring-up complete. ==="));
  Serial.println(F("--- Short press BOOT button: send OPEN/CLOSE/STOP (after pairing) ---"));
  Serial.println(F("--- Long press (>1.5s) BOOT button: start pairing (real transmission!) ---"));
  Serial.println(F("    Put your Somfy motor/Situo into PROG mode before the long press."));
  Serial.println(F("--- RX-sniffing (experimental) active: feel free to press your Situo ---"));
}

static const uint32_t LONG_PRESS_MS = 1500;
static bool g_button_was_down = false;
static uint32_t g_button_down_since = 0;
static int g_demo_step = 0;

static void do_pairing() {
  Serial.println(F("[ACTION] Starting pairing — REAL RF transmission to broadcast address."));
  txctl::perform_pairing();
}

static void do_send_button() {
  if (!txctl::is_paired()) {
    Serial.println(F("[ACTION] Not paired yet — hold the BOOT button >1.5s to pair."));
    return;
  }
  g_demo_step = (g_demo_step + 1) % 3;
  uint8_t code;
  const char* label;
  ui::Action act;
  ui::Position pos;
  const char* mqtt_state = nullptr; // matches net_control.cpp's on_message(): STOP publishes no new state
  switch (g_demo_step) {
    case 0: code = iohc::BTN_OPEN;  label = "OPEN";  act = ui::Action::OPENING;  pos = ui::Position::OPEN;        mqtt_state = "open";   break;
    case 1: code = iohc::BTN_CLOSE; label = "CLOSE"; act = ui::Action::CLOSING;  pos = ui::Position::CLOSED;      mqtt_state = "closed"; break;
    default: code = iohc::BTN_STOP; label = "STOP";  act = ui::Action::STOPPING; pos = ui::Position::STOPPED_MID; break;
  }
  Serial.print(F("[ACTION] Sending ")); Serial.println(label);
  bool ok = txctl::send_button(code);
  Serial.println(ok ? F("[ACTION] Transmission complete") : F("[ACTION] Transmission failed"));
  // Also publish to Home Assistant, regardless of the fact that this came
  // from the physical button - otherwise HA's shown position drifts out of
  // sync with reality.
  if (ok && mqtt_state) netctl::publish_state(mqtt_state);
  ui::set_action(act);
  ui::set_position(pos);
  ui::display_render();
}

static void handle_button() {
  bool down = (digitalRead(PIN_BOOT_BUTTON) == LOW);
  if (down && !g_button_was_down) {
    g_button_down_since = millis();
  }
  if (!down && g_button_was_down) {
    uint32_t held = millis() - g_button_down_since;
    if (held >= LONG_PRESS_MS) do_pairing();
    else do_send_button();
  }
  g_button_was_down = down;
}

void loop() {
  handle_button();
  netctl::loop();
  rxctl::poll();
  static uint32_t last_alive = 0;
  if (millis() - last_alive > 2000) {
    Serial.println(F("alive"));
    last_alive = millis();
  }
  delay(20);
}
