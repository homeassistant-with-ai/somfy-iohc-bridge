/**
 * io-homecontrol physical layer: UART-like bit framing + preamble/sync word,
 * packed into a raw byte buffer that gets sent over the air UNCHANGED (no
 * CRC/whitening/address filtering from the radio chip itself).
 *
 * Source: docs/radio.md ("Raw Data Sending") + verified against samr037/
 * iohc-flipper's frame_build.c (BitPacker/bp_push_uart_byte), without their
 * CC1101-specific 0x99 residue byte (which compensates for THEIR chip's own
 * hardware sync-word detection already consuming part of the bit stream —
 * for us (SX1262/RadioLib) we instead send the FULL preamble+sync word+
 * frame ourselves as one raw buffer, so that trick isn't needed/applicable
 * here).
 *
 * Every logical byte is UART-wrapped: start bit(0), 8 data bits LSB-first,
 * stop bit(1) — so 10 raw bits per logical byte. This 10-bits-per-byte
 * stream is packed MSB-first into output bytes (trailing bits padded with
 * 1, the "idle line" state).
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace iohc {

constexpr uint8_t PHY_SYNC_BYTE_1 = 0xFF;
constexpr uint8_t PHY_SYNC_BYTE_2 = 0x33;

/**
 * Builds the complete on-air buffer: `preamble_bytes` times 0x55 (raw
 * alternating bits, NO UART wrapping, because this already IS the
 * alternating 0/1 preamble), followed by UART-wrapped FF, UART-wrapped 33,
 * followed by UART-wrapped versions of every byte in `frame`.
 *
 * out must be at least phy_encoded_size(frame_len, preamble_bytes) bytes.
 * Returns the number of bytes written.
 */
size_t phy_encode(const uint8_t* frame, size_t frame_len, uint8_t preamble_bytes,
                   uint8_t* out, size_t out_size);

/** Computes how many bytes phy_encode() needs for the given input. */
size_t phy_encoded_size(size_t frame_len, uint8_t preamble_bytes);

/**
 * Receive side: searches a RAW bit buffer (1 byte per bit, value 0 or 1 -
 * this is the format a GPIO sample capture produces) for the UART-wrapped
 * FF 33 sync pattern, and then decodes the following UART-wrapped bytes
 * back into the original frame (the inverse of phy_encode's sync+frame
 * portion; the raw 0x55 preamble itself doesn't need to be recognized,
 * only the sync word).
 *
 * bits/bit_count: the raw, unsorted bit stream to search.
 * frame_out/frame_out_size: output buffer for the decoded frame bytes.
 *
 * Returns the number of decoded frame bytes (this can be an incomplete/
 * broken frame if the bit stream stops halfway or a start/stop bit doesn't
 * check out — validate the CRC (iohc_crc.h) before trusting a frame).
 * Returns 0 if the sync pattern does not occur anywhere in the buffer.
 */
size_t phy_decode(const uint8_t* bits, size_t bit_count,
                   uint8_t* frame_out, size_t frame_out_size);

/**
 * Fills `out` (20 bytes, 1 per bit) with the UART-wrapped FF+33 sync bits.
 * Made public so RX code (which sets the radio's HARDWARE sync word to
 * part of this, see rx_control.cpp) can reuse the exact same, already-
 * tested bit sequence instead of retyping it separately.
 */
void phy_sync_bits(uint8_t out[20]);

} // namespace iohc
