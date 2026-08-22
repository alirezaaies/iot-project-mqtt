#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "secrets.h"

// The first firmware module only publishes test telemetry through MQTT.
// Sensors and control commands are deliberately left for later modules.
namespace config {
constexpr char DEVICE_ID[] = "esp8266-01";
constexpr char FIRMWARE_VERSION[] = "0.1.0";
constexpr char MQTT_HOST[] = "192.168.1.15";
constexpr uint16_t MQTT_PORT = 1883;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 5000;
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;
}  // namespace config

WiFiClient networkClient;
PubSubClient mqttClient(networkClient);

unsigned long lastTelemetryAt = 0;
unsigned long lastWiFiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;
bool wifiWasConnected = false;

String topicFor(const char* suffix) {
  return String("iot/devices/") + config::DEVICE_ID + "/" + suffix;
}

void startWiFiConnection() {
  Serial.print("Connecting to Wi-Fi SSID: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWiFiAttemptAt = millis();
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      Serial.print("Wi-Fi connected. Device IP: ");
      Serial.println(WiFi.localIP());
    }
    return;
  }

  if (wifiWasConnected) {
    wifiWasConnected = false;
    Serial.println("Wi-Fi disconnected.");
  }

  if (millis() - lastWiFiAttemptAt >= config::RECONNECT_INTERVAL_MS) {
    startWiFiConnection();
  }
}

void publishStatus(const char* status) {
  StaticJsonDocument<128> document;
  document["device_id"] = config::DEVICE_ID;
  document["status"] = status;
  document["firmware"] = config::FIRMWARE_VERSION;

  char payload[128];
  serializeJson(document, payload, sizeof(payload));
  mqttClient.publish(topicFor("status").c_str(), payload, true);
}

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }
  if (millis() - lastMqttAttemptAt < config::RECONNECT_INTERVAL_MS) {
    return;
  }
  lastMqttAttemptAt = millis();

  const String statusTopic = topicFor("status");
  const String clientId = String(config::DEVICE_ID) + "-" +
                          String(ESP.getChipId(), HEX);
  const String offlinePayload =
      String("{\"device_id\":\"") + config::DEVICE_ID +
      "\",\"status\":\"offline\",\"firmware\":\"" +
      config::FIRMWARE_VERSION + "\"}";

  Serial.print("Connecting to MQTT broker: ");
  Serial.println(config::MQTT_HOST);
  if (!mqttClient.connect(clientId.c_str(), statusTopic.c_str(), 1, true,
                          offlinePayload.c_str())) {
    Serial.print("MQTT connection failed; state = ");
    Serial.println(mqttClient.state());
    return;
  }

  Serial.println("MQTT connected.");
  publishStatus("online");
}

void publishTelemetry() {
  StaticJsonDocument<192> document;
  document["device_id"] = config::DEVICE_ID;
  document["uptime_ms"] = millis();
  document["sample_value"] = random(0, 1024);  // Replaced by a sensor later.
  document["firmware"] = config::FIRMWARE_VERSION;

  char payload[192];
  serializeJson(document, payload, sizeof(payload));
  if (mqttClient.publish(topicFor("telemetry").c_str(), payload)) {
    Serial.print("Telemetry published: ");
    Serial.println(payload);
  } else {
    Serial.println("Telemetry publish failed.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP8266 MQTT telemetry - firmware 0.1.0");

  randomSeed(micros());
  mqttClient.setServer(config::MQTT_HOST, config::MQTT_PORT);
  mqttClient.setBufferSize(256);
  startWiFiConnection();
}

void loop() {
  maintainWiFi();
  connectMqtt();

  if (mqttClient.connected()) {
    mqttClient.loop();
    if (millis() - lastTelemetryAt >= config::TELEMETRY_INTERVAL_MS) {
      lastTelemetryAt = millis();
      publishTelemetry();
    }
  }

  delay(10);
}
