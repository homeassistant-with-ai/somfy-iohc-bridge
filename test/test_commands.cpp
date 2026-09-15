// Host-native test:
//   g++ -std=c++17 -I ../lib/iohc -o /tmp/test_commands test_commands.cpp \
//       ../lib/iohc/iohc_commands.cpp ../lib/iohc/iohc_frame.cpp ../lib/iohc/iohc_crypto.cpp -lcrypto
//   /tmp/test_commands
#include "../lib/iohc/iohc_commands.h"
#include "../lib/iohc/iohc_constants.h"
#include <cstdio>
#include <cstring>

using namespace iohc;

static void hex(const char* label, const uint8_t* b, size_t n) {
  printf("%-20s: ", label);
  for (size_t i = 0; i < n; i++) printf("%02X ", b[i]);
  printf("\n");
}

int main() {
  int failures = 0;

  // Test 1: the ctrl0..data portion of a STOP button frame must exactly
  // match the REAL captured SMOOVE Origin IO frame from
  // docs/radio.md (F6 00 00 00 3F 48 5B 37 00 01 43 D2 00 00 00).
  // We don't know the real install_key/seq, so we only compare the
  // portion before seq/mac/crc.
  {
    uint8_t src[3]  = {0x48, 0x5B, 0x37};
    uint8_t dest[3] = {0x00, 0x00, 0x3F};
    uint8_t dummy_key[16] = {0}; // unknown for this test, only header/data matters
    uint8_t out[MAX_TX_FRAME_SIZE];
    size_t n = build_button_frame(src, dest, BTN_STOP, 0x0000, dummy_key, out, sizeof(out));

    uint8_t expected_header[] = {0xF6,0x00, 0x00,0x00,0x3F, 0x48,0x5B,0x37, 0x00, 0x01,0x43,0xD2,0x00,0x00,0x00};
    hex("built (first 15)", out, sizeof(expected_header));
    hex("expected (capture)", expected_header, sizeof(expected_header));
    bool ok = (n > sizeof(expected_header)) && memcmp(out, expected_header, sizeof(expected_header)) == 0;
    printf("Test 1 - STOP frame header matches real capture: %s\n", ok ? "PASS" : "FAIL");
    printf("  total frame length: %zu bytes\n\n", n);
    if (!ok) failures++;
  }

  // Test 2: pairing frame length must be exactly 31 bytes (msg_len=28 from
  // iohc-flipper's iohc_frame_build_pair_0x30, +ctrl0 +crc(2) = 31)
  {
    uint8_t src[3]  = {0xAB, 0xCD, 0xEF};
    uint8_t dest[3] = {0x00, 0x00, 0x3F};
    uint8_t key[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16};
    uint8_t out[MAX_TX_FRAME_SIZE];
    size_t n = build_pairing_frame(src, dest, key, 0x1234, out, sizeof(out));
    hex("pairing frame", out, n);
    bool ok = (n == 31) && (out[9] == 0x7E) && (out[10] == 0x60); // enc_key starts with the verified value
    printf("Test 2 - pairing frame length (31) and encrypted key correct: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;
  }

  return failures == 0 ? 0 : 1;
}
