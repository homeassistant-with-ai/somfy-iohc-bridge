/**
 * OLED status screen for the Heltec WiFi LoRa32 V3 (SSD1306 128x64, I2C).
 *
 * Pins verified against ropg/heltec_esp32_lora_v3 (heltec_unofficial.h):
 * SDA=17, SCL=18, RST=21, I2C address 0x3C.
 *
 * Shows: radio status, pairing/connection status, last executed action
 * (OPEN/CLOSE/STOP), and the position TRACKED by the controller.
 *
 * NOTE: io-homecontrol 1W gives no feedback from the motor ("In 1-way mode,
 * the controller does not get any answer" - iown-home docs/linklayer.md).
 * The "position" here is therefore always an ASSUMPTION based on the last
 * command sent, never an actual status reading from the awning itself.
 */
#pragma once
#include <stdint.h>

namespace ui {

enum class RadioState : uint8_t { INIT, OK, FAILED };
enum class LinkState  : uint8_t { NOT_PAIRED, PAIRING, PAIRED };
enum class Action     : uint8_t { NONE, OPENING, CLOSING, STOPPING };
enum class Position   : uint8_t { UNKNOWN, OPEN, CLOSED, STOPPED_MID };

void display_init();

void set_radio_state(RadioState s);
void set_link_state(LinkState s);
void set_action(Action a);       // briefly show which action is being executed
void set_position(Position p);   // tracked (assumed) position

/** Redraws the screen with the current status. Call regularly from loop(). */
void display_render();

} // namespace ui
