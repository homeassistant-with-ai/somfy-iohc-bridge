#include "display.h"
#include <Arduino.h>
#include <SSD1306Wire.h>

namespace ui {

static const int PIN_SDA_OLED = 17;
static const int PIN_SCL_OLED = 18;
static const int PIN_RST_OLED = 21;

static SSD1306Wire g_display(0x3C, PIN_SDA_OLED, PIN_SCL_OLED);

static RadioState g_radio = RadioState::INIT;
static LinkState  g_link  = LinkState::NOT_PAIRED;
static Action     g_action = Action::NONE;
static Position   g_position = Position::UNKNOWN;

void display_init() {
  // The OLED on the Heltec V3 has its own reset line that must be pulsed first.
  pinMode(PIN_RST_OLED, OUTPUT);
  digitalWrite(PIN_RST_OLED, LOW);
  delay(20);
  digitalWrite(PIN_RST_OLED, HIGH);
  delay(20);

  g_display.init();
  g_display.flipScreenVertically();
  g_display.setFont(ArialMT_Plain_10);
  g_display.clear();
  g_display.drawString(0, 0, "Somfy Bridge");
  g_display.drawString(0, 14, "Starting...");
  g_display.display();
}

void set_radio_state(RadioState s)   { g_radio = s; }
void set_link_state(LinkState s)     { g_link = s; }
void set_action(Action a)            { g_action = a; }
void set_position(Position p)        { g_position = p; }

static const char* radio_text() {
  switch (g_radio) {
    case RadioState::OK:     return "Radio: OK";
    case RadioState::FAILED: return "Radio: FAILED";
    default:                 return "Radio: init...";
  }
}

static const char* link_text() {
  switch (g_link) {
    case LinkState::PAIRED:     return "Status: Connected";
    case LinkState::PAIRING:    return "Status: Pairing...";
    default:                    return "Status: Not paired";
  }
}

static const char* action_text() {
  switch (g_action) {
    case Action::OPENING:  return ">> OPEN";
    case Action::CLOSING:  return ">> CLOSE";
    case Action::STOPPING: return ">> STOP";
    default:                return "";
  }
}

static const char* position_text() {
  switch (g_position) {
    case Position::OPEN:        return "Position: Open";
    case Position::CLOSED:      return "Position: Closed";
    case Position::STOPPED_MID: return "Position: Mid-stop";
    default:                    return "Position: unknown";
  }
}

void display_render() {
  g_display.clear();
  g_display.setFont(ArialMT_Plain_10);
  g_display.drawString(0, 0,  "Somfy Bridge");
  g_display.drawHorizontalLine(0, 12, 128);
  g_display.drawString(0, 16, radio_text());
  g_display.drawString(0, 28, link_text());
  g_display.drawString(0, 40, position_text());

  const char* act = action_text();
  if (act[0] != '\0') {
    g_display.setFont(ArialMT_Plain_16);
    g_display.drawString(0, 46, act);
  }

  g_display.display();
}

} // namespace ui
