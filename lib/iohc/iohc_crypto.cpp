#include "iohc_crypto.h"
#include <string.h>

#if defined(ARDUINO) || defined(ESP_PLATFORM)
  #include <mbedtls/aes.h>
  #define IOHC_USE_MBEDTLS 1
#else
  // Host-side (native g++ test build): use OpenSSL EVP so the test suite
  // can run without the ESP32 toolchain.
  #include <openssl/evp.h>
  #define IOHC_USE_MBEDTLS 0
#endif

namespace iohc {

void compute_iv_checksum(const uint8_t* data, size_t len, uint8_t& chksum1, uint8_t& chksum2) {
  uint8_t c1 = 0, c2 = 0;
  for (size_t i = 0; i < len; i++) {
    uint8_t frame_byte = data[i];
    uint8_t tmpchksum = frame_byte ^ c2;
    uint8_t new_c2 = (uint8_t)(((c1 & 0x7f) << 1) & 0xff);
    if ((c1 & 0x80) == 0) {
      if (tmpchksum >= 128) new_c2 |= 1;
      c1 = new_c2;
      c2 = (uint8_t)((tmpchksum << 1) & 0xff);
    } else {
      if (tmpchksum >= 128) new_c2 |= 1;
      c1 = new_c2 ^ 0x55;
      c2 = (uint8_t)(((tmpchksum << 1) ^ 0x5b) & 0xff);
    }
  }
  chksum1 = c1;
  chksum2 = c2;
}

void build_iv_1w_auth(const uint8_t* payload, size_t payload_len, uint16_t seq, uint8_t iv_out[16]) {
  // bytes 0-7: first 8 bytes of payload, 0x55-padded if shorter
  uint8_t padded[8];
  for (int i = 0; i < 8; i++) {
    padded[i] = (i < (int)payload_len) ? payload[i] : 0x55;
  }
  memcpy(&iv_out[0], padded, 8);

  // bytes 8-9: checksum over the ORIGINAL (unpadded) payload
  uint8_t c1, c2;
  compute_iv_checksum(payload, payload_len, c1, c2);
  iv_out[8] = c1;
  iv_out[9] = c2;

  // bytes 10-11: sequence number, MSB first (matches example "0599" -> 05,99)
  iv_out[10] = (uint8_t)(seq >> 8);
  iv_out[11] = (uint8_t)(seq & 0xFF);

  // bytes 12-15: padding
  iv_out[12] = iv_out[13] = iv_out[14] = iv_out[15] = 0x55;
}

void build_iv_key_transfer(const uint8_t node_id[3], uint8_t iv_out[16]) {
  for (int i = 0; i < 16; i++) iv_out[i] = node_id[i % 3];
}

void aes128_encrypt_block(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
#if IOHC_USE_MBEDTLS
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, key, 128);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
  mbedtls_aes_free(&ctx);
#else
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, key, nullptr);
  EVP_CIPHER_CTX_set_padding(ctx, 0);
  int outlen = 0;
  EVP_EncryptUpdate(ctx, out, &outlen, in, 16);
  int finlen = 0;
  EVP_EncryptFinal_ex(ctx, out + outlen, &finlen);
  EVP_CIPHER_CTX_free(ctx);
#endif
}

void encrypt_key_for_transfer(const uint8_t crypt_key[16], const uint8_t iv[16],
                               const uint8_t key_to_transmit[16], uint8_t ciphertext_out[16]) {
  uint8_t e[16];
  aes128_encrypt_block(crypt_key, iv, e);
  for (int i = 0; i < 16; i++) ciphertext_out[i] = e[i] ^ key_to_transmit[i];
}

void compute_1w_mac(const uint8_t stack_key[16], const uint8_t iv[16], uint8_t mac_out[6]) {
  uint8_t e[16];
  aes128_encrypt_block(stack_key, iv, e);
  memcpy(mac_out, e, 6);
}

} // namespace iohc
