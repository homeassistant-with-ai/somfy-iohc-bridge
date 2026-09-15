/**
 * io-homecontrol protocol constants.
 *
 * Source: iown-home docs/radio.md + docs/linklayer.md + docs/commands.md
 * (https://github.com/rspaargaren/iown-home), cross-checked against real
 * sniffed frames in that documentation. NOT copied blindly from iown-home's
 * src/protocol/iohome_constants.h — that file contains at least two
 * demonstrable errors versus its own docs:
 *   1) CMD_OPEN/CMD_CLOSE/CMD_STOP = 0x60-0x63, marked by the authors
 *      themselves as "TBD: verify from docs" — and 0x60-0x65 does NOT
 *      appear anywhere in their own docs/commands.md "## Command IDs"
 *      section (which runs 0x00-0x57, then 0xE0-0xFF). Movement commands
 *      go through Command ID 0x00 ("Activate/Execute Function"), not a
 *      separate open/close/stop ID.
 *   2) SYNC_WORD_LEN = 3, while docs/linklayer.md explicitly describes the
 *      sync word as 2 bytes (FF 33).
 * Anything not 100% certain is flagged explicitly in the comment.
 */
#pragma once
#include <stdint.h>

namespace iohc {

// ---- Physical layer (verified on real SX1262 hardware, see main.cpp) ----
constexpr float FREQ_CHANNEL_1 = 868.25f;  // MHz, 2W only
constexpr float FREQ_CHANNEL_2 = 868.95f;  // MHz, 1W + 2W (primary)
constexpr float FREQ_CHANNEL_3 = 869.85f;  // MHz, 2W only

constexpr float BITRATE_KBPS   = 38.4f;
constexpr float FREQ_DEV_KHZ   = 19.2f;
constexpr float RX_BW_KHZ      = 117.3f;   // nearest standard value > Carson bandwidth

// ---- Sync word (docs/linklayer.md: "Sync Word | 2 Byte", value FF 33) ----
constexpr uint8_t SYNC_WORD[2] = {0xFF, 0x33};

// ---- Frame layout (docs/linklayer.md byte table, cross-validated against
//      the SMOOVE Origin IO / 101010b captures in docs/radio.md) ----
constexpr uint8_t NODE_ID_SIZE   = 3;
constexpr uint8_t CRC_SIZE       = 2;
constexpr uint8_t SEQNUM_SIZE    = 2;  // 1W authenticated frames only
constexpr uint8_t MAC_SIZE       = 6;  // authenticated frames only

// Control Byte 0: Order[7:6] | ProtocolMode[5] | Size[4:0]
constexpr uint8_t CTRL0_ORDER_MASK    = 0xC0;
constexpr uint8_t CTRL0_ONEWAY_MASK   = 0x20; // bit5: 1=1W, 0=2W (docs/linklayer.md "isOneWay")
constexpr uint8_t CTRL0_SIZE_MASK     = 0x1F;

// Control Byte 1: UseBeacon[7] | Routed[6] | LowPowerMode[5] | Ack[4] | ProtoVersion[3:0]
constexpr uint8_t CTRL1_USE_BEACON    = 0x80;
constexpr uint8_t CTRL1_ROUTED        = 0x40;
constexpr uint8_t CTRL1_LOW_POWER     = 0x20;
constexpr uint8_t CTRL1_ACK           = 0x10;
constexpr uint8_t CTRL1_PROTO_VER_MASK= 0x0F;

// ---- Addresses ----
constexpr uint8_t BROADCAST_ADDR[NODE_ID_SIZE] = {0x00, 0x00, 0x3F};

// ---- Command IDs (docs/commands.md "## Command IDs", 0x00-0x57 + 0xE0-0xFF) ----
constexpr uint8_t CMD_ACTIVATE_FUNCTION = 0x00; // "Activate/Execute Function": OPEN/CLOSE/STOP go through this
constexpr uint8_t CMD_ACTIVATE_MODE     = 0x01;
constexpr uint8_t CMD_DISCOVER_ACTUATOR = 0x28;
constexpr uint8_t CMD_DISCOVER_ANSWER   = 0x29;
constexpr uint8_t CMD_DISCOVER_CONFIRM  = 0x2C;
constexpr uint8_t CMD_DISCOVER_CONFIRM_ACK = 0x2D;
constexpr uint8_t CMD_SEND_1W_KEY       = 0x30; // pairing: controller -> actuator
constexpr uint8_t CMD_ASK_CHALLENGE     = 0x31;
constexpr uint8_t CMD_KEY_TRANSFER      = 0x32;
constexpr uint8_t CMD_CHALLENGE_REQUEST = 0x3C;
constexpr uint8_t CMD_CHALLENGE_RESPONSE= 0x3D;
constexpr uint8_t CMD_REMOVE_1W_CONTROLLER = 0x39;

// Command Originator (docs/commands.md "Command Originator")
constexpr uint8_t ORIGINATOR_LOCAL_USER  = 0x00;
constexpr uint8_t ORIGINATOR_USER_REMOTE = 0x01; // used by a handheld remote control
constexpr uint8_t ORIGINATOR_EMERGENCY   = 0xFF;

// ---- Button-frame layout for cmd 0x00 (Activate/Execute Function) ----
// VERIFIED (no longer "probable"): cross-validated between three
// independent sources that match exactly:
//   1) docs/radio.md's REAL sniffed SMOOVE Origin IO frame: data=01 43 D2 00 00 00
//   2) docs/commands.md "Standard Values": 0xD200 = "Current" (= stay put = STOP)
//   3) samr037/iohc-flipper (hardware-validated, working FAP for Flipper Zero),
//      state/tx_state.h: IOHC_BTN_UP=0x00, IOHC_BTN_DOWN=0xC8, IOHC_BTN_STOP=0xD2,
//      IOHC_VENDOR_SOMFY=0x43 — originating from rspaargaren/iown-homecontrol-esp32sx1276
//      (Apache-2.0), see iohc-flipper's NOTICE file for provenance.
//
// Frame "data" field for a button frame (6 bytes after cmd=0x00):
//   [0]=ORIGINATOR_USER_REMOTE(0x01), [1]=VENDOR_SOMFY(0x43),
//   [2]=button code, [3]=0x00, [4]=0x00, [5]=0x00
// (this is "Originator + ACEI + MainParam(2 bytes, only the high byte used) + FP1 + FP2"
//  from docs/commands.md "00: Activate/Execute Function", where ACEI here carries
//  the vendor marker instead of the generic ACEI bit flags — real Somfy remote
//  controls use this field the same way, per the captures.)
constexpr uint8_t VENDOR_SOMFY = 0x43;

constexpr uint8_t BTN_OPEN  = 0x00; // "Up" — verified
constexpr uint8_t BTN_CLOSE = 0xC8; // "Down" — verified
constexpr uint8_t BTN_STOP  = 0xD2; // "Current" (= stop) — verified against a real capture
// NOTE — NOT yet independently confirmed: the PROG/"My" button code. docs/commands.md
// suggests a standalone 0x0003 for "1W Button Prog", but that is a DIFFERENT scheme
// (small integer codes) than the one verified above. Not needed for pairing anyway
// (pairing goes through cmd 0x30, see below) — only relevant if you later want to
// implement a separate "My" button.
constexpr uint8_t BTN_PROG_UNVERIFIED = 0x03;

// ---- Manufacturer ID for the pairing command 0x30 ----
// Verified against both docs/commands.md ("Manufacturer = 0x02 (Somfy)")
// and iohc-flipper's device_book.c (case 0x43 vendor -> returns 0x02 man_id).
constexpr uint8_t MANUFACTURER_ID_SOMFY = 0x02;

// ---- Pairing: publicly known "transfer key" (docs/linklayer.md, NOT secret,
//      only used to obfuscate the key during pairing) ----
constexpr uint8_t TRANSFER_KEY[16] = {
  0x34, 0xC3, 0x46, 0x6E, 0xD8, 0x8F, 0x4E, 0x8E,
  0x16, 0xAA, 0x47, 0x39, 0x49, 0x88, 0x43, 0x73
};

constexpr uint16_t CRC_INIT = 0x0000;

} // namespace iohc
