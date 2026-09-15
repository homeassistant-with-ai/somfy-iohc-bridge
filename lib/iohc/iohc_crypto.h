/**
 * io-homecontrol AES-128/MAC/pairing crypto.
 *
 * Every step here was numerically verified against the worked examples in
 * iown-home docs/linklayer.md (Python/host-side, see test/test_crypto.cpp)
 * before any of it went anywhere near the radio. Uses mbedTLS (bundled with
 * Arduino-ESP32) for AES-128-ECB single-block encryption.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace iohc {

/**
 * The "checksum" used in IV construction (docs/linklayer.md). NOT the same
 * as the frame CRC. Operates on the ORIGINAL (unpadded) data — numerically
 * verified against the example in the docs.
 */
void compute_iv_checksum(const uint8_t* data, size_t len, uint8_t& chksum1, uint8_t& chksum2);

/**
 * Builds the 16-byte IV for 1W authentication (MAC generation).
 * payload: the data to sign (Command ID + parameters), payload_len <= 8
 * seq: 2-byte sequence number of this frame
 * iv_out: 16 bytes
 */
void build_iv_1w_auth(const uint8_t* payload, size_t payload_len, uint16_t seq, uint8_t iv_out[16]);

/**
 * Builds the 16-byte IV for 1W key transfer (pairing): the 3-byte NodeID
 * repeated to fill 16 bytes.
 */
void build_iv_key_transfer(const uint8_t node_id[3], uint8_t iv_out[16]);

/** AES-128-ECB, single block (16 bytes in, 16 bytes out). */
void aes128_encrypt_block(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);

/**
 * Encrypts a 16-byte key for transmission during pairing:
 * ciphertext = AES(transfer_or_stack_key, iv) XOR key_to_transmit
 */
void encrypt_key_for_transfer(const uint8_t crypt_key[16], const uint8_t iv[16],
                               const uint8_t key_to_transmit[16], uint8_t ciphertext_out[16]);

/**
 * Generates the 6-byte MAC for 1W authentication:
 * MAC = first 6 bytes of AES(stack_key, iv)
 */
void compute_1w_mac(const uint8_t stack_key[16], const uint8_t iv[16], uint8_t mac_out[6]);

} // namespace iohc
