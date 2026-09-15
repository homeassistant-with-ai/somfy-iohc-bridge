/**
 * WiFi + MQTT + Home Assistant MQTT Discovery for the Somfy Bridge.
 *
 * Topics:
 *   somfy/awning/set          <- commands: OPEN / CLOSE / STOP
 *   somfy/awning/state        -> open / closed (optimistic, see below)
 *   somfy/awning/availability -> online / offline (Last Will and Testament)
 *
 * NOTE: io-homecontrol 1W gives no feedback from the motor. The
 * published state is therefore always an ASSUMPTION based on the last
 * command sent, never an actual position reading.
 */
#pragma once

namespace netctl {

void init();

/** Must be called every loop() cycle: maintains the WiFi/MQTT connection. */
void loop();

bool is_connected();

/**
 * Publishes the (assumed) position to somfy/awning/state, retained.
 * Must be called after EVERY successful transmission, regardless of
 * whether it was triggered via MQTT/Home Assistant or via the physical
 * BOOT button - otherwise HA and the actual position drift apart as soon
 * as you use the button on the board itself.
 */
void publish_state(const char* state);

} // namespace netctl
