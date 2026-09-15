/**
 * io-homecontrol frame header (Layer 2) — build and parse.
 *
 * Covers only ctrl0/ctrl1/dest/src/cmd/data/CRC. The 1W authentication
 * suffix (sequence number + 6-byte MAC, see docs/linklayer.md) is handled
 * in a later step (pairing/crypto), together with the AES IV construction.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "iohc_constants.h"
#include "iohc_crc.h"

namespace iohc {

constexpr size_t FRAME_HEADER_MAX_DATA = 21;
// ctrl0(1) + ctrl1(1) + dest(3) + src(3) + cmd(1) + data(<=21)
constexpr size_t FRAME_HEADER_MAX_SIZE = 1 + 1 + NODE_ID_SIZE + NODE_ID_SIZE + 1 + FRAME_HEADER_MAX_DATA;

struct FrameHeader {
  uint8_t ctrl0 = 0;
  uint8_t ctrl1 = 0;
  uint8_t dest[NODE_ID_SIZE] = {0, 0, 0};
  uint8_t src[NODE_ID_SIZE]  = {0, 0, 0};
  uint8_t cmd = 0;
  uint8_t data[FRAME_HEADER_MAX_DATA] = {0};
  uint8_t data_len = 0;
};

inline void set_dest(FrameHeader& f, const uint8_t addr[NODE_ID_SIZE]) {
  for (int i = 0; i < NODE_ID_SIZE; i++) f.dest[i] = addr[i];
}

inline void set_src(FrameHeader& f, const uint8_t addr[NODE_ID_SIZE]) {
  for (int i = 0; i < NODE_ID_SIZE; i++) f.src[i] = addr[i];
}

/**
 * Builds ctrl0 from the order relation, 1W/2W mode and the total frame
 * size (excl. ctrl0 and CRC — see docs/linklayer.md "Size").
 */
inline uint8_t build_ctrl0(bool one_way, uint8_t total_size_excl_ctrl0_and_crc, uint8_t order_bits = 0) {
  uint8_t c = 0;
  c |= (order_bits << 6) & CTRL0_ORDER_MASK;
  if (one_way) c |= CTRL0_ONEWAY_MASK;
  c |= (total_size_excl_ctrl0_and_crc & CTRL0_SIZE_MASK);
  return c;
}

inline uint8_t build_ctrl1(uint8_t proto_version = 0, bool ack = false) {
  uint8_t c = proto_version & CTRL1_PROTO_VER_MASK;
  if (ack) c |= CTRL1_ACK;
  return c;
}

/**
 * Serializes ctrl0..data (WITHOUT CRC, without SEQ/MAC suffix) into a buffer.
 * Returns the number of bytes written, or 0 on error.
 */
size_t serialize_header(const FrameHeader& f, uint8_t* out, size_t out_size);

/**
 * Computes and appends the 2-byte CRC (LSB first) after an already
 * serialized frame (header [+ suffix]).
 * `buf` must have 2 free bytes remaining at the end.
 */
size_t append_crc(uint8_t* buf, size_t len);

/**
 * Validates that the last 2 bytes of `buf` are a correct CRC over the
 * preceding bytes.
 */
bool validate_crc(const uint8_t* buf, size_t len);

} // namespace iohc
