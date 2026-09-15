/**
 * Experimental RX-sniffing: makes the SX1262 passively listen on
 * 868.95 MHz to detect frames from the ORIGINAL Situo remote,
 * so we can correct our assumed position when someone operates the
 * awning outside of our bridge.
 *
 * UNCERTAIN (experimental phase - see project discussion): the SX1262 has
 * no raw/direct receive mode (unlike the SX127x/T-Beam), so we use a
 * trick: set the radio's HARDWARE sync word to the first 16 bits of our
 * UART-wrapped FF/33 sync pattern (0x7F 0xD9). After a match, the radio
 * delivers raw bytes which, together with the 16 known sync bits in front
 * of them, we run through our existing/tested iohc::phy_decode() again.
 * This HAD to be confirmed empirically with a real reception from the
 * Situo - hence the extensive diagnostic logging.
 */
#pragma once
#include <RadioLib.h>
#include <stdint.h>

namespace rxctl {

void init(SX1262* radio);

/** Start/resume listening mode. Call after every transmission (TX and RX can't run at the same time). */
void start_listening();

/** Call from loop(): processes any packet that was received. */
void poll();

} // namespace rxctl
