#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "actuators.h"
#include "secrets.h"
#include "sensors.h"

namespace config {
constexpr char DEVICE_ID[] = "esp8266-01";
constexpr char FIRMWARE_VERSION[] = "0.3.1";
constexpr char MQTT_HOST[] = "192.168.1.15";
constexpr uint16_t MQTT_PORT = 1883;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 5000;
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;

// A fixed upper bound avoids unpredictable heap use on the ESP8266. Raising it
// reserves more global memory but does not make MQTT packets larger.
constexpr size_t MAX_SENSOR_COUNT = 128;

// Large sensor sets are split into small packets. Six readings keep each JSON
// payload comfortably below the configured MQTT packet buffer.
constexpr size_t READINGS_PER_BATCH = 6;
constexpr size_t JSON_CAPACITY = 768;
constexpr size_t MAX_COMMAND_ACTIONS = 8;
constexpr size_t COMMAND_JSON_CAPACITY = 1024;
constexpr size_t MQTT_BUFFER_SIZE = 1280;
}  // namespace config

WiFiClient networkClient;
PubSubClient mqttClient(networkClient);
SensorReading sensorReadings[config::MAX_SENSOR_COUNT];
String statusTopic;
String telemetryTopic;
String controlTopic;
String responseTopic;
StaticJsonDocument<config::COMMAND_JSON_CAPACITY> incomingCommand;
StaticJsonDocument<config::COMMAND_JSON_CAPACITY> outgoingResponse;
char commandResponsePayload[config::COMMAND_JSON_CAPACITY];

unsigned long lastTelemetryAt = 0;
unsigned long lastWiFiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;
uint32_t messageSequence = 0;
bool wifiWasConnected = false;

// Telemetry is sent cooperatively: one batch per loop iteration. This keeps
// mqttClient.loop() responsive even when a project has many sensors.
bool telemetryCycleActive = false;
size_t telemetrySensorCount = 0;
size_t telemetryBatchCount = 0;
size_t nextTelemetryBatch = 0;
uint32_t telemetryMessageId = 0;
unsigned long telemetrySampledAtMs = 0;

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
  StaticJsonDocument<160> document;
  document["device_id"] = config::DEVICE_ID;
  document["status"] = status;
  document["firmware"] = config::FIRMWARE_VERSION;

  char payload[160];
  serializeJson(document, payload, sizeof(payload));
  mqttClient.publish(statusTopic.c_str(), payload, true);
}

void publishCommandResponse() {
  const size_t length = serializeJson(outgoingResponse, commandResponsePayload,
                                      sizeof(commandResponsePayload));
  if (length == 0 || length >= sizeof(commandResponsePayload) - 1) {
    Serial.println("Command response serialization failed.");
    return;
  }

  if (!mqttClient.publish(responseTopic.c_str(),
                          reinterpret_cast<const uint8_t*>(commandResponsePayload), length,
                          false)) {
    Serial.println("Command response publish failed.");
    return;
  }
  Serial.print("Command response published: ");
  Serial.println(commandResponsePayload);
}

void mqttMessageReceived(char* topic, byte* rawPayload, unsigned int length) {
  if (controlTopic != topic) {
    return;
  }

  incomingCommand.clear();
  const DeserializationError error =
      deserializeJson(incomingCommand, rawPayload, length);
  if (error) {
    Serial.print("Invalid command JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char* commandId = incomingCommand["command_id"];
  const JsonArrayConst actions = incomingCommand["actions"].as<JsonArrayConst>();
  if (incomingCommand["schema_version"] != 1 || commandId == nullptr ||
      strlen(commandId) == 0 || strlen(commandId) > 64 || actions.isNull() ||
      actions.size() == 0 || actions.size() > config::MAX_COMMAND_ACTIONS) {
    Serial.println("Command rejected: invalid envelope.");
    return;
  }

  // Each action receives an independent result. This makes partial success
  // explicit when a multi-action command contains a disabled or invalid target.
  outgoingResponse.clear();
  outgoingResponse["schema_version"] = 1;
  outgoingResponse["device_id"] = config::DEVICE_ID;
  outgoingResponse["command_id"] = commandId;
  JsonArray results = outgoingResponse.createNestedArray("results");
  bool allSucceeded = true;

  for (JsonObjectConst action : actions) {
    const char* target = action["target"];
    const JsonVariantConst jsonValue = action["value"];
    ControlResult result{false, "value must be boolean or numeric"};

    if (target == nullptr) {
      result = {false, "target is missing"};
    } else if (jsonValue.is<bool>()) {
      const ControlValue value{ControlValueKind::Boolean,
                               jsonValue.as<bool>(), 0.0F};
      result = applyControlAction(target, value);
    } else if (jsonValue.is<float>()) {
      const ControlValue value{ControlValueKind::Number, false,
                               jsonValue.as<float>()};
      result = applyControlAction(target, value);
    }

    JsonObject actionResult = results.createNestedObject();
    actionResult["target"] = target == nullptr ? "unknown" : target;
    actionResult["success"] = result.success;
    actionResult["message"] = result.message;
    allSucceeded = allSucceeded && result.success;
  }

  outgoingResponse["success"] = allSucceeded;
  publishCommandResponse();
}

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }
  if (millis() - lastMqttAttemptAt < config::RECONNECT_INTERVAL_MS) {
    return;
  }
  lastMqttAttemptAt = millis();

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
  if (!mqttClient.subscribe(controlTopic.c_str(), 1)) {
    Serial.println("Could not subscribe to the control topic.");
    mqttClient.disconnect();
    return;
  }
  Serial.print("Subscribed to control topic: ");
  Serial.println(controlTopic);
  publishStatus("online");
}

bool publishTelemetryBatch(const SensorReading* readings, size_t readingCount,
                           uint32_t messageId, size_t batchIndex,
                           size_t batchCount, unsigned long sampledAtMs) {
  StaticJsonDocument<config::JSON_CAPACITY> document;
  document["schema_version"] = 1;
  document["device_id"] = config::DEVICE_ID;
  document["message_id"] = messageId;
  document["sampled_at_ms"] = sampledAtMs;
  document["batch_index"] = batchIndex + 1;  // Human-readable, starts at one.
  document["batch_count"] = batchCount;

  JsonArray values = document.createNestedArray("readings");
  for (size_t index = 0; index < readingCount; ++index) {
    JsonObject reading = values.createNestedObject();
    reading["sensor_id"] = readings[index].sensorId;
    reading["value"] = readings[index].value;
    reading["unit"] = readings[index].unit;
  }

  char payload[config::JSON_CAPACITY];
  const size_t payloadLength = serializeJson(document, payload, sizeof(payload));
  if (payloadLength == 0 || payloadLength >= sizeof(payload) - 1) {
    Serial.println("Telemetry serialization failed: payload buffer is too small.");
    return false;
  }

  if (!mqttClient.publish(telemetryTopic.c_str(),
                          reinterpret_cast<const uint8_t*>(payload),
                          payloadLength, false)) {
    Serial.println("Telemetry publish failed.");
    return false;
  }

  Serial.print("Telemetry batch published: ");
  Serial.println(payload);
  return true;
}

void startTelemetryCycle() {
  telemetrySensorCount = readSensors(sensorReadings, config::MAX_SENSOR_COUNT);
  if (telemetrySensorCount == 0) {
    Serial.println("No sensor readings available.");
    return;
  }

  telemetryMessageId = ++messageSequence;
  telemetrySampledAtMs = millis();
  telemetryBatchCount =
      (telemetrySensorCount + config::READINGS_PER_BATCH - 1) /
      config::READINGS_PER_BATCH;
  nextTelemetryBatch = 0;
  telemetryCycleActive = true;
}

void publishNextTelemetryBatch() {
  if (!telemetryCycleActive) return;

  const size_t firstReading = nextTelemetryBatch * config::READINGS_PER_BATCH;
  const size_t remaining = telemetrySensorCount - firstReading;
  const size_t readingsInBatch =
      remaining < config::READINGS_PER_BATCH ? remaining
                                             : config::READINGS_PER_BATCH;

  if (!publishTelemetryBatch(&sensorReadings[firstReading], readingsInBatch,
                             telemetryMessageId, nextTelemetryBatch,
                             telemetryBatchCount, telemetrySampledAtMs)) {
    Serial.println("Telemetry cycle stopped after a failed batch.");
    telemetryCycleActive = false;
    return;
  }

  ++nextTelemetryBatch;
  telemetryCycleActive = nextTelemetryBatch < telemetryBatchCount;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP8266 MQTT telemetry and control - firmware 0.3.1");

  randomSeed(micros());
  statusTopic = String("iot/devices/") + config::DEVICE_ID + "/status";
  telemetryTopic = String("iot/devices/") + config::DEVICE_ID + "/telemetry";
  controlTopic = String("iot/devices/") + config::DEVICE_ID + "/control";
  responseTopic = String("iot/devices/") + config::DEVICE_ID + "/response";
  initializeActuators();
  mqttClient.setServer(config::MQTT_HOST, config::MQTT_PORT);
  mqttClient.setCallback(mqttMessageReceived);
  mqttClient.setBufferSize(config::MQTT_BUFFER_SIZE);
  startWiFiConnection();
}

void loop() {
  maintainWiFi();
  connectMqtt();

  if (mqttClient.connected()) {
    // Process incoming commands first. Telemetry never controls this schedule.
    mqttClient.loop();

    if (!telemetryCycleActive &&
        millis() - lastTelemetryAt >= config::TELEMETRY_INTERVAL_MS) {
      lastTelemetryAt = millis();
      startTelemetryCycle();
    }

    // At most one telemetry packet is produced in each pass through loop().
    publishNextTelemetryBatch();

    // Service packets that arrived while the telemetry batch was serialized.
    mqttClient.loop();
  }

  // Let the ESP8266 background network tasks run without a blocking delay.
  yield();
}
