// Host-native test (no ESP32/hardware needed):
//   g++ -std=c++17 -I ../lib/iohc -o /tmp/test_frame test_frame.cpp ../lib/iohc/iohc_frame.cpp && /tmp/test_frame
#include "../lib/iohc/iohc_frame.h"
#include <cstdio>
#include <cstring>

using namespace iohc;

static void hexprint(const char* label, const uint8_t* b, size_t n) {
  printf("%s: ", label);
  for (size_t i = 0; i < n; i++) printf("%02X ", b[i]);
  printf("\n");
}

int main() {
  int failures = 0;

  // Real captured frame from docs/radio.md ("SMOOVE Origin IO packet 1"),
  // preamble (FF 33) already removed:
  const uint8_t captured[] = {
    0xF6,0x00, 0x00,0x00,0x3F, 0x48,0x5B,0x37, 0x00,
    0x01,0x43,0xD2,0x00,0x00,0x00,
    0x03,0xD6,0xB6,0x3C,0xB3,0xCD,0xCD,0x2B,
    0x8A,0x2E
  };

  // Test 1: CRC validation over the full, real captured frame
  bool crc_ok = validate_crc(captured, sizeof(captured));
  printf("Test 1 - CRC over real captured frame valid: %s\n", crc_ok ? "PASS" : "FAIL");
  if (!crc_ok) failures++;

  // Test 2: our frame builder reproduces the header 1:1
  // (ctrl0..data of the same captured frame; this is exactly the example
  // "00 01 43 D200 00 00" from docs/commands.md §"00: Activate/Execute Function")
  FrameHeader f;
  f.ctrl0 = 0xF6;
  f.ctrl1 = 0x00;
  const uint8_t dest[3] = {0x00,0x00,0x3F};
  const uint8_t src[3]  = {0x48,0x5B,0x37};
  set_dest(f, dest);
  set_src(f, src);
  f.cmd = CMD_ACTIVATE_FUNCTION;
  const uint8_t data[] = {0x01, 0x43, 0xD2, 0x00, 0x00, 0x00}; // Originator=User, ACEI=0x43, MainParam=0xD200(Current), FP1=0, FP2=0
  memcpy(f.data, data, sizeof(data));
  f.data_len = sizeof(data);

  uint8_t out[FRAME_HEADER_MAX_SIZE];
  size_t n = serialize_header(f, out, sizeof(out));

  const uint8_t expected_header[] = {0xF6,0x00, 0x00,0x00,0x3F, 0x48,0x5B,0x37, 0x00, 0x01,0x43,0xD2,0x00,0x00,0x00};
  bool header_ok = (n == sizeof(expected_header)) && memcmp(out, expected_header, n) == 0;
  hexprint("built    ", out, n);
  hexprint("expected ", expected_header, sizeof(expected_header));
  printf("Test 2 - Frame builder reproduces real header: %s\n", header_ok ? "PASS" : "FAIL");
  if (!header_ok) failures++;

  return failures == 0 ? 0 : 1;
}
