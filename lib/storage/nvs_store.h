/**
 * Persistent storage (ESP32 NVS via the Arduino Preferences library) of the
 * pairing identity: our own NodeID, install key, and sequence counter.
 *
 * Without this we'd lose the link with the motor on every reboot (the
 * identity used to live only in RAM) - this fixes that (phase 5, step 9:
 * "correct counter persistence so a reboot never causes desynchronization").
 */
#pragma once
#include <stdint.h>

namespace store {

struct Identity {
  bool paired;
  uint8_t src[3];
  uint8_t install_key[16];
  uint16_t seq;
};

void init();

/** Loads the stored identity. identity.paired == false if nothing is stored yet. */
Identity load();

/** Saves the full identity (after a pairing attempt). */
void save(const Identity& identity);

/**
 * Saves ONLY the sequence counter. Called after every transmission so a
 * reboot never reuses a sequence number that's already been used (and thus
 * stale/invalid to the motor).
 */
void save_seq(uint16_t seq);

/** Clears the stored identity (e.g. to pair again). */
void clear();

} // namespace store
