#include "net_control.h"
#include "tx_control.h"
#include "iohc_constants.h"
#include "display.h"
#include "secrets.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

namespace netctl {

static const char* TOPIC_SET          = "somfy/awning/set";
static const char* TOPIC_STATE        = "somfy/awning/state";
static const char* TOPIC_AVAILABILITY = "somfy/awning/availability";
static const char* DISCOVERY_TOPIC    = "homeassistant/cover/somfy_bridge_awning/config";

static WiFiClient g_wifi_client;
static PubSubClient g_mqtt(g_wifi_client);
static String g_client_id;
static String g_unique_id;
static uint32_t g_last_wifi_attempt = 0;
static uint32_t g_last_mqtt_attempt = 0;

static void publish_discovery() {
  JsonDocument doc;
  doc["name"] = nullptr; // use the device name as the entity name (avoids "Somfy Bridge Somfy Bridge")
  doc["unique_id"] = g_unique_id;
  doc["command_topic"] = TOPIC_SET;
  doc["state_topic"] = TOPIC_STATE;
  doc["availability_topic"] = TOPIC_AVAILABILITY;
  doc["payload_open"] = "OPEN";
  doc["payload_close"] = "CLOSE";
  doc["payload_stop"] = "STOP";
  doc["state_open"] = "open";
  doc["state_closed"] = "closed";
  doc["optimistic"] = false; // we publish the (assumed) state ourselves
  doc["device_class"] = "curtain";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray ids = device["identifiers"].to<JsonArray>();
  ids.add(g_unique_id);
  device["name"] = "Somfy Bridge";
  device["manufacturer"] = "DIY (io-homecontrol, rspaargaren/iohc-flipper protocol)";
  device["model"] = "Heltec WiFi LoRa32 V3";

  char buf[768];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  Serial.print(F("[MQTT] Discovery payload (")); Serial.print(n); Serial.println(F(" bytes):"));
  Serial.println(buf);
  bool ok = g_mqtt.publish(DISCOVERY_TOPIC, (const uint8_t*)buf, n, /*retained=*/true);
  Serial.println(ok ? F("[MQTT] Discovery config published") : F("[MQTT] ERROR: discovery publish failed (too large for MQTT_MAX_PACKET_SIZE?)"));
}

static void on_message(char* topic, uint8_t* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.print(F("[MQTT] Message on ")); Serial.print(topic); Serial.print(F(": ")); Serial.println(msg);

  if (String(topic) != TOPIC_SET) return;

  if (!txctl::is_paired()) {
    Serial.println(F("[MQTT] Ignored: not yet paired (hold BOOT button >1.5s during PROG mode)"));
    return;
  }

  uint8_t code;
  ui::Action act;
  ui::Position pos;
  const char* new_state = nullptr;

  if (msg == "OPEN")       { code = iohc::BTN_OPEN;  act = ui::Action::OPENING;  pos = ui::Position::OPEN;        new_state = "open"; }
  else if (msg == "CLOSE") { code = iohc::BTN_CLOSE; act = ui::Action::CLOSING;  pos = ui::Position::CLOSED;      new_state = "closed"; }
  else if (msg == "STOP")  { code = iohc::BTN_STOP;  act = ui::Action::STOPPING; pos = ui::Position::STOPPED_MID; }
  else { Serial.println(F("[MQTT] Unknown command, ignored")); return; }

  bool ok = txctl::send_button(code);
  Serial.println(ok ? F("[MQTT] Command sent") : F("[MQTT] Send failed"));

  ui::set_action(act);
  ui::set_position(pos);
  ui::display_render();
  if (new_state) publish_state(new_state);
}

static void ensure_wifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - g_last_wifi_attempt < 5000) return;
  g_last_wifi_attempt = millis();
  Serial.println(F("[WIFI] Connecting..."));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

static void ensure_mqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (g_mqtt.connected()) return;
  if (millis() - g_last_mqtt_attempt < 5000) return;
  g_last_mqtt_attempt = millis();

  Serial.println(F("[MQTT] Connecting..."));
  bool ok = g_mqtt.connect(g_client_id.c_str(), MQTT_USER, MQTT_PASSWORD,
                            TOPIC_AVAILABILITY, /*willQos=*/0, /*willRetain=*/true, "offline");
  if (ok) {
    Serial.println(F("[MQTT] Connected"));
    g_mqtt.publish(TOPIC_AVAILABILITY, "online", /*retained=*/true);
    publish_discovery();
    g_mqtt.subscribe(TOPIC_SET);
  } else {
    Serial.print(F("[MQTT] Connection failed, rc=")); Serial.println(g_mqtt.state());
  }
}

void init() {
  uint64_t mac = ESP.getEfuseMac();
  char id[24];
  snprintf(id, sizeof(id), "somfy-bridge-%04X", (uint16_t)(mac & 0xFFFF));
  g_client_id = id;
  g_unique_id = g_client_id;

  g_mqtt.setServer(MQTT_HOST, MQTT_PORT);
  g_mqtt.setCallback(on_message);
  g_mqtt.setBufferSize(1024); // discovery JSON is larger than PubSubClient's default of 256 bytes

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("[WIFI] Connecting to ")); Serial.println(WIFI_SSID);
}

void loop() {
  ensure_wifi();
  ensure_mqtt();
  if (g_mqtt.connected()) g_mqtt.loop();
}

bool is_connected() { return g_mqtt.connected(); }

void publish_state(const char* state) {
  if (!g_mqtt.connected()) {
    Serial.println(F("[MQTT] Cannot publish state: not connected"));
    return;
  }
  g_mqtt.publish(TOPIC_STATE, state, /*retained=*/true);
}

} // namespace netctl
