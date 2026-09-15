#include "iohc_frame.h"
#include <string.h>

namespace iohc {

size_t serialize_header(const FrameHeader& f, uint8_t* out, size_t out_size) {
  size_t needed = 1 + 1 + NODE_ID_SIZE + NODE_ID_SIZE + 1 + f.data_len;
  if (out_size < needed || f.data_len > FRAME_HEADER_MAX_DATA) return 0;

  size_t i = 0;
  out[i++] = f.ctrl0;
  out[i++] = f.ctrl1;
  memcpy(&out[i], f.dest, NODE_ID_SIZE); i += NODE_ID_SIZE;
  memcpy(&out[i], f.src,  NODE_ID_SIZE); i += NODE_ID_SIZE;
  out[i++] = f.cmd;
  memcpy(&out[i], f.data, f.data_len); i += f.data_len;
  return i;
}

size_t append_crc(uint8_t* buf, size_t len) {
  uint16_t crc = crc16(buf, len, CRC_INIT);
  buf[len]     = (uint8_t)(crc & 0xFF);       // LSB first (docs/radio.md)
  buf[len + 1] = (uint8_t)((crc >> 8) & 0xFF);
  return len + 2;
}

bool validate_crc(const uint8_t* buf, size_t len) {
  if (len < 2) return false;
  // CRC over [buf, len-2) followed by the 2 CRC bytes themselves must give
  // 0x0000 (docs/radio.md CRC example: residual CRC == 0 means valid).
  return crc16(buf, len, CRC_INIT) == 0x0000;
}

} // namespace iohc
