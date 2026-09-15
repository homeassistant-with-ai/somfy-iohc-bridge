/**
 * io-homecontrol frame CRC (CRC-16, poly 0x8408, init 0, LSB-first, no xorout).
 *
 * Source: iown-home docs/radio.md "CRC example" — code dumped from the
 * Semtech SX12xx Starter Kit software (decompiled), with two verified
 * test vectors (see test/test_crc.cpp). This is the one layer we adopt
 * as-is: it is pure math, no assumption about Somfy-specific command
 * values involved.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace iohc {

inline uint16_t crc_update(uint16_t crc, uint8_t data) {
  crc ^= data;
  for (int i = 0; i < 8; i++) {
    uint16_t mask = (crc & 1) ? 0x8408 : 0x0000;
    crc = (crc >> 1) ^ mask;
  }
  return crc;
}

inline uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc = 0x0000) {
  for (size_t i = 0; i < len; i++) crc = crc_update(crc, data[i]);
  return crc;
}

} // namespace iohc
