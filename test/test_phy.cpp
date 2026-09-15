// Host-native test:
//   g++ -std=c++17 -I ../lib/iohc -o /tmp/test_phy test_phy.cpp ../lib/iohc/iohc_phy.cpp
//   /tmp/test_phy
#include "../lib/iohc/iohc_phy.h"
#include <cstdio>

using namespace iohc;

int main() {
  int failures = 0;

  // Test 1: UART wrapping of just FF+33 (no preamble), derived manually
  // from docs/radio.md: "0 11111111 1 0 11001100 1"
  //   FF -> bits: 0,1,1,1,1,1,1,1,1,1        (10 bits)
  //   33 -> bits: 0,1,1,0,0,1,1,0,0,1        (10 bits)
  //   together 20 bits, packed MSB-first:
  //   byte0 = 01111111 = 0x7F
  //   byte1 = 11011001 = 0xD9
  //   byte2 = 1001 + padding 1111 = 10011111 = 0x9F
  {
    uint8_t frame[0]; // no extra frame bytes, just testing the sync
    uint8_t out[8];
    size_t n = phy_encode(frame, 0, /*preamble_bytes=*/0, out, sizeof(out));
    printf("n=%zu bytes: ", n);
    for (size_t i = 0; i < n; i++) printf("%02X ", out[i]);
    printf("\n");
    bool ok = (n == 3) && out[0] == 0x7F && out[1] == 0xD9 && out[2] == 0x9F;
    printf("Test 1 - UART-wrapped FF 33 sync matches manual derivation: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;
  }

  // Test 2: size calculation matches the actual output
  {
    uint8_t frame[5] = {0x00, 0x01, 0x43, 0xD2, 0x00};
    size_t expected = phy_encoded_size(sizeof(frame), 4);
    uint8_t out[64];
    size_t n = phy_encode(frame, sizeof(frame), 4, out, sizeof(out));
    printf("phy_encoded_size=%zu, phy_encode returns=%zu\n", expected, n);
    bool ok = (n == expected) && (n > 0);
    printf("Test 2 - size function consistent with encode: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;
  }

  return failures == 0 ? 0 : 1;
}
