#include "iohc_commands.h"
#include "iohc_constants.h"
#include "iohc_crypto.h"
#include "iohc_crc.h"
#include "iohc_frame.h"
#include <string.h>

namespace iohc {

// Builds a generic 1W frame: ctrl0 ctrl1 dest src cmd data [seq] [mac] crc.
// mac_key == nullptr  -> no sequence/mac suffix (used by build_pairing_frame,
//                        which handles its OWN seq+no-mac layout separately)
// mac_key != nullptr  -> appends seq(2, MSB first) + 6-byte MAC, MAC computed
//                        over [cmd + data] with build_iv_1w_auth (verified).
static size_t build_authenticated_1w_frame(
    uint8_t cmd, const uint8_t* data, size_t data_len,
    const uint8_t src[3], const uint8_t dest[3],
    uint16_t seq, const uint8_t mac_key[16],
    uint8_t* out, size_t out_size, bool low_power_mode) {

  if (data_len > FRAME_HEADER_MAX_DATA) return 0;

  // Size field (ctrl0 bits 4-0): ctrl1+dest+src+cmd+data+seq+mac,
  // verified against the real SMOOVE capture (docs/radio.md) and
  // iohc-flipper's frame_build.c.
  size_t size_field = 1 + NODE_ID_SIZE + NODE_ID_SIZE + 1 + data_len + SEQNUM_SIZE + MAC_SIZE;
  if (size_field > 0x1F) return 0;

  FrameHeader f;
  // Order bits 11 (0xC0): verified against the real captured SMOOVE frame
  // (ctrl0=0xF6) AND iohc-flipper (end_frame=start_frame=true for every
  // standalone command frame) — so NOT "0,0" as the docs table might
  // suggest at first glance.
  f.ctrl0 = build_ctrl0(/*one_way=*/true, (uint8_t)size_field, /*order_bits=*/0b11);
  f.ctrl1 = build_ctrl1(/*proto_version=*/0, /*ack=*/false);
  if (low_power_mode) f.ctrl1 |= CTRL1_LOW_POWER;
  set_dest(f, dest);
  set_src(f, src);
  f.cmd = cmd;
  memcpy(f.data, data, data_len);
  f.data_len = (uint8_t)data_len;

  size_t n = serialize_header(f, out, out_size);
  if (n == 0) return 0;

  if (out_size < n + SEQNUM_SIZE + MAC_SIZE + CRC_SIZE) return 0;

  out[n++] = (uint8_t)(seq >> 8);
  out[n++] = (uint8_t)(seq & 0xFF);

  // HMAC payload = cmd + data (verified: iohc-flipper frame_build.c
  // lines 91-96, "HMAC covers cmd + every payload byte before seq").
  uint8_t hmac_payload[1 + FRAME_HEADER_MAX_DATA];
  hmac_payload[0] = cmd;
  memcpy(&hmac_payload[1], data, data_len);

  uint8_t iv[16];
  build_iv_1w_auth(hmac_payload, 1 + data_len, seq, iv);
  uint8_t mac[MAC_SIZE];
  compute_1w_mac(mac_key, iv, mac);
  memcpy(&out[n], mac, MAC_SIZE);
  n += MAC_SIZE;

  n = append_crc(out, n);
  return n;
}

size_t build_button_frame(const uint8_t src[3], const uint8_t dest[3],
                           uint8_t button_code, uint16_t seq,
                           const uint8_t install_key[16],
                           uint8_t* out, size_t out_size,
                           bool low_power_mode) {
  uint8_t data[6] = {
    ORIGINATOR_USER_REMOTE, VENDOR_SOMFY, button_code, 0x00, 0x00, 0x00
  };
  return build_authenticated_1w_frame(CMD_ACTIVATE_FUNCTION, data, sizeof(data),
                                       src, dest, seq, install_key, out, out_size,
                                       low_power_mode);
}

size_t build_remove_controller_frame(const uint8_t src[3], const uint8_t dest[3],
                                      uint16_t seq, const uint8_t install_key[16],
                                      uint8_t* out, size_t out_size,
                                      bool low_power_mode) {
  uint8_t data[1] = {0x00};
  return build_authenticated_1w_frame(CMD_REMOVE_1W_CONTROLLER, data, sizeof(data),
                                       src, dest, seq, install_key, out, out_size,
                                       low_power_mode);
}

size_t build_pairing_frame(const uint8_t src[3], const uint8_t dest[3],
                            const uint8_t install_key[16], uint16_t seq,
                            uint8_t* out, size_t out_size,
                            bool low_power_mode) {
  // Layout (verified against iohc-flipper frame_build.c
  // iohc_frame_build_pair_0x30, NO mac):
  //   ctrl0 ctrl1 dest src cmd=0x30 enc_key[16] man_id data(=0x01) seq[2] crc[2]
  constexpr size_t payload_len = 16 + 1 + 1; // enc_key + man_id + data byte
  size_t size_field = 1 + NODE_ID_SIZE + NODE_ID_SIZE + 1 + payload_len + SEQNUM_SIZE;
  if (size_field > 0x1F) return 0;

  uint8_t enc_key[16];
  uint8_t iv[16];
  build_iv_key_transfer(src, iv);
  encrypt_key_for_transfer(TRANSFER_KEY, iv, install_key, enc_key);

  FrameHeader f;
  f.ctrl0 = build_ctrl0(/*one_way=*/true, (uint8_t)size_field, /*order_bits=*/0b11);
  f.ctrl1 = build_ctrl1();
  if (low_power_mode) f.ctrl1 |= CTRL1_LOW_POWER;
  set_dest(f, dest);
  set_src(f, src);
  f.cmd = CMD_SEND_1W_KEY;
  memcpy(f.data, enc_key, 16);
  f.data[16] = MANUFACTURER_ID_SOMFY;
  f.data[17] = 0x01; // "data" byte, value 0x01 verified in iohc-flipper
  f.data_len = (uint8_t)payload_len;

  size_t n = serialize_header(f, out, out_size);
  if (n == 0) return 0;
  if (out_size < n + SEQNUM_SIZE + CRC_SIZE) return 0;

  out[n++] = (uint8_t)(seq >> 8);
  out[n++] = (uint8_t)(seq & 0xFF);

  n = append_crc(out, n);
  return n;
}

} // namespace iohc
