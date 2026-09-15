/**
 * Orchestration of real RF transmission: radio configuration for raw
 * bit-accurate payloads, plus the pairing and button transmission flows.
 * Keeps the (randomly generated for this session) identity in RAM — NVS
 * persistence is layered on in a later step (phase 3, step 9).
 */
#pragma once
#include <RadioLib.h>
#include <stdint.h>

namespace txctl {

void init(SX1262* radio);

bool is_paired();
void get_src_address(uint8_t out[3]);

/** Generates a new random identity and attempts to pair (0x39 + 0x30 bursts). */
bool perform_pairing();

/** Sends a button command (OPEN/CLOSE/STOP) with the current identity. */
bool send_button(uint8_t button_code);

} // namespace txctl
