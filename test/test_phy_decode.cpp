// Host-native test:
//   g++ -std=c++17 -I ../lib/iohc -o /tmp/test_phy_decode test_phy_decode.cpp \
//       ../lib/iohc/iohc_phy.cpp ../lib/iohc/iohc_commands.cpp ../lib/iohc/iohc_frame.cpp \
//       ../lib/iohc/iohc_crypto.cpp -lcrypto
//   /tmp/test_phy_decode
#include "../lib/iohc/iohc_phy.h"
#include "../lib/iohc/iohc_commands.h"
#include "../lib/iohc/iohc_constants.h"
#include "../lib/iohc/iohc_frame.h"
#include "../lib/iohc/iohc_crc.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace iohc;

// Converts a byte buffer into individual bits (MSB first), the way a raw
// GPIO-sample capture of the over-the-air bit stream would look.
static std::vector<uint8_t> bytes_to_bits(const uint8_t* buf, size_t len) {
  std::vector<uint8_t> bits;
  for (size_t i = 0; i < len; i++) {
    for (int b = 7; b >= 0; b--) bits.push_back((buf[i] >> b) & 1);
  }
  return bits;
}

int main() {
  int failures = 0;

  // Test: encode a real STOP button frame (same as in test_commands.cpp),
  // unpack it into individual bits, and decode it back again. Also prepend
  // some "noise" bits to simulate the capture not starting exactly at the
  // preamble (realistic for a free-running receive buffer).
  {
    uint8_t src[3]  = {0x48, 0x5B, 0x37};
    uint8_t dest[3] = {0x00, 0x00, 0x3F};
    uint8_t key[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16};
    uint8_t frame[MAX_TX_FRAME_SIZE];
    size_t frame_len = build_button_frame(src, dest, BTN_STOP, 0x0000, key, frame, sizeof(frame));

    uint8_t encoded[128];
    size_t encoded_len = phy_encode(frame, frame_len, /*preamble_bytes=*/8, encoded, sizeof(encoded));
    printf("frame_len=%zu encoded_len=%zu\n", frame_len, encoded_len);

    std::vector<uint8_t> bits;
    for (int i = 0; i < 37; i++) bits.push_back(i % 2); // arbitrary noise up front
    auto encoded_bits = bytes_to_bits(encoded, encoded_len);
    bits.insert(bits.end(), encoded_bits.begin(), encoded_bits.end());

    uint8_t decoded[64];
    size_t decoded_len = phy_decode(bits.data(), bits.size(), decoded, sizeof(decoded));

    printf("decoded_len=%zu (expected >= %zu)\n", decoded_len, frame_len);
    bool ok = (decoded_len >= frame_len) && memcmp(decoded, frame, frame_len) == 0;
    printf("Test 1 - round-trip encode/decode matches original frame: %s\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;

    // Extra: CRC over the decoded frame must be correct (proves the
    // decoder is truly correct bit-for-bit, not just accidentally right).
    bool crc_ok = validate_crc(decoded, decoded_len);
    printf("Test 2 - CRC over decoded frame valid: %s\n", crc_ok ? "PASS" : "FAIL");
    if (!crc_ok) failures++;
  }

  // Test 3: no sync pattern present -> must return 0, not crash.
  {
    std::vector<uint8_t> noise(200, 0);
    for (size_t i = 0; i < noise.size(); i++) noise[i] = (uint8_t)(i * 7 % 2);
    uint8_t decoded[64];
    size_t n = phy_decode(noise.data(), noise.size(), decoded, sizeof(decoded));
    printf("Test 3 - no sync in noise -> 0: %s (n=%zu)\n", n == 0 ? "PASS" : "FAIL", n);
    if (n != 0) failures++;
  }

  return failures == 0 ? 0 : 1;
}
