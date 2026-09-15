// Host-native test (uses OpenSSL instead of mbedTLS, see iohc_crypto.cpp):
//   g++ -std=c++17 -I ../lib/iohc -o /tmp/test_crypto test_crypto.cpp ../lib/iohc/iohc_crypto.cpp -lcrypto
//   /tmp/test_crypto
#include "../lib/iohc/iohc_crypto.h"
#include "../lib/iohc/iohc_constants.h"
#include <cstdio>
#include <cstring>

using namespace iohc;

static void hex(const char* label, const uint8_t* b, size_t n) {
  printf("%-28s: ", label);
  for (size_t i = 0; i < n; i++) printf("%02X", b[i]);
  printf("\n");
}

static bool eq(const uint8_t* a, const uint8_t* b, size_t n) { return memcmp(a, b, n) == 0; }

int main() {
  int failures = 0;

  // ---- Test 1: 1W key-transfer encryption (docs/linklayer.md "1W Key Exchange") ----
  // Node ABCDEF, key to transmit 01020304050607080910111213141516,
  // expected ciphertext 7E60491F976ADF653DB0ED785E49A201
  {
    uint8_t node[3] = {0xAB, 0xCD, 0xEF};
    uint8_t key_to_send[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16};
    uint8_t iv[16];
    build_iv_key_transfer(node, iv);
    hex("IV (key transfer)", iv, 16);

    uint8_t cipher[16];
    encrypt_key_for_transfer(TRANSFER_KEY, iv, key_to_send, cipher);
    hex("ciphertext", cipher, 16);

    uint8_t expected[16] = {0x7E,0x60,0x49,0x1F,0x97,0x6A,0xDF,0x65,0x3D,0xB0,0xED,0x78,0x5E,0x49,0xA2,0x01};
    bool ok = eq(cipher, expected, 16);
    printf("Test 1 - 1W key-transfer encryption: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;
  }

  // ---- Test 2: IV checksum for 1W authentication (docs/linklayer.md example) ----
  // Payload 000143D2000000 (7 bytes, unpadded) -> checksum must be 05 00
  {
    uint8_t payload[7] = {0x00,0x01,0x43,0xD2,0x00,0x00,0x00};
    uint8_t c1, c2;
    compute_iv_checksum(payload, sizeof(payload), c1, c2);
    printf("checksum (expected 05 00)  : %02X %02X\n", c1, c2);
    bool ok = (c1 == 0x05 && c2 == 0x00);
    printf("Test 2 - IV checksum: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;
  }

  // ---- Test 3: full 1W-auth IV (docs/linklayer.md example) ----
  // Payload 000143D2000000, seq 0x0599 -> IV must be 000143D2000000550500059955555555
  {
    uint8_t payload[7] = {0x00,0x01,0x43,0xD2,0x00,0x00,0x00};
    uint8_t iv[16];
    build_iv_1w_auth(payload, sizeof(payload), 0x0599, iv);
    hex("IV (1W auth)", iv, 16);
    uint8_t expected[16] = {0x00,0x01,0x43,0xD2,0x00,0x00,0x00,0x55, 0x05,0x00, 0x05,0x99, 0x55,0x55,0x55,0x55};
    bool ok = eq(iv, expected, 16);
    printf("Test 3 - 1W auth IV construction: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) failures++;
  }

  return failures == 0 ? 0 : 1;
}
