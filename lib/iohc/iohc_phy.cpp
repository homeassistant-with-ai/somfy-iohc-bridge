#include "iohc_phy.h"
#include <string.h>

namespace iohc {

namespace {

struct BitPacker {
  uint8_t* buf;
  size_t bit_count;
  size_t bit_capacity;
};

void bp_push_bit(BitPacker& p, uint8_t bit) {
  size_t byte_idx = p.bit_count >> 3;
  uint8_t bit_idx = 7 - (uint8_t)(p.bit_count & 7);
  if (byte_idx >= (p.bit_capacity >> 3)) return;
  if (bit & 1) p.buf[byte_idx] |= (uint8_t)(1u << bit_idx);
  else         p.buf[byte_idx] &= (uint8_t)~(1u << bit_idx);
  p.bit_count++;
}

// UART-wraps 1 byte: start(0), 8 data bits LSB-first, stop(1).
void bp_push_uart_byte(BitPacker& p, uint8_t b) {
  bp_push_bit(p, 0);
  for (int i = 0; i < 8; i++) bp_push_bit(p, (uint8_t)((b >> i) & 1));
  bp_push_bit(p, 1);
}

// Raw alternating bits (the "0x55 repeated" preamble pattern): no
// start/stop bit, just the 8 bits of 0x55 itself, MSB-first as they go
// over the air (0x55 = 01010101, so this already IS the alternating pulse).
void bp_push_raw_byte(BitPacker& p, uint8_t b) {
  for (int i = 7; i >= 0; i--) bp_push_bit(p, (uint8_t)((b >> i) & 1));
}

} // namespace

size_t phy_encoded_size(size_t frame_len, uint8_t preamble_bytes) {
  size_t total_bits = (size_t)preamble_bytes * 8 + (2 + frame_len) * 10;
  return (total_bits + 7) / 8;
}

size_t phy_encode(const uint8_t* frame, size_t frame_len, uint8_t preamble_bytes,
                   uint8_t* out, size_t out_size) {
  size_t needed = phy_encoded_size(frame_len, preamble_bytes);
  if (out_size < needed) return 0;

  memset(out, 0, needed);
  BitPacker p{out, 0, needed * 8};

  for (uint8_t i = 0; i < preamble_bytes; i++) bp_push_raw_byte(p, 0x55);
  bp_push_uart_byte(p, PHY_SYNC_BYTE_1);
  bp_push_uart_byte(p, PHY_SYNC_BYTE_2);
  for (size_t i = 0; i < frame_len; i++) bp_push_uart_byte(p, frame[i]);

  // Pad remaining bits with 1 (idle line state), as iohc-flipper does.
  while (p.bit_count & 7) bp_push_bit(p, 1);

  return (p.bit_count + 7) / 8;
}

void phy_sync_bits(uint8_t out[20]) {
  size_t i = 0;
  auto push_uart = [&](uint8_t b) {
    out[i++] = 0; // start
    for (int k = 0; k < 8; k++) out[i++] = (uint8_t)((b >> k) & 1);
    out[i++] = 1; // stop
  };
  push_uart(PHY_SYNC_BYTE_1);
  push_uart(PHY_SYNC_BYTE_2);
}

size_t phy_decode(const uint8_t* bits, size_t bit_count,
                   uint8_t* frame_out, size_t frame_out_size) {
  uint8_t sync[20];
  phy_sync_bits(sync);

  if (bit_count < 20) return 0;

  // Find the first position where the 20 sync bits match exactly.
  size_t sync_pos = (size_t)-1;
  for (size_t i = 0; i + 20 <= bit_count; i++) {
    bool match = true;
    for (size_t k = 0; k < 20; k++) {
      if ((bits[i + k] & 1) != sync[k]) { match = false; break; }
    }
    if (match) { sync_pos = i; break; }
  }
  if (sync_pos == (size_t)-1) return 0;

  // From here on: decode consecutive 10-bit UART groups (start,8xdata,stop)
  // into bytes, until the bits run out, the buffer is full, or a
  // start/stop bit doesn't check out (= sync lost / end of transmission).
  size_t pos = sync_pos + 20;
  size_t out_len = 0;
  while (pos + 10 <= bit_count && out_len < frame_out_size) {
    if ((bits[pos] & 1) != 0) break;       // start bit must be 0
    if ((bits[pos + 9] & 1) != 1) break;   // stop bit must be 1
    uint8_t byte_val = 0;
    for (int k = 0; k < 8; k++) {
      if (bits[pos + 1 + k] & 1) byte_val |= (uint8_t)(1u << k); // LSB first
    }
    frame_out[out_len++] = byte_val;
    pos += 10;
  }
  return out_len;
}

} // namespace iohc
