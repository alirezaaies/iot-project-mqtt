#include "actuators.h"

#include <cmath>
#include <cstring>

namespace {
// The built-in LED is safe for the first control test and is active LOW.
constexpr bool ENABLE_LED_OUTPUT = true;
constexpr uint8_t LED_PIN = LED_BUILTIN;
constexpr uint8_t LED_ON_LEVEL = LOW;

// These external outputs remain disabled until their wiring, relay module,
// voltage, current rating, fuse, and fail-safe behavior have been verified.
// Change a flag to true only after assigning the correct board pin.
constexpr bool ENABLE_HEATER_OUTPUT = false;
constexpr uint8_t HEATER_PIN = D1;
constexpr uint8_t HEATER_ON_LEVEL = HIGH;

constexpr bool ENABLE_COOLER_OUTPUT = false;
constexpr uint8_t COOLER_PIN = D2;
constexpr uint8_t COOLER_ON_LEVEL = HIGH;

constexpr bool ENABLE_RELAY_OUTPUT = false;
constexpr uint8_t RELAY_PIN = D5;
constexpr uint8_t RELAY_ON_LEVEL = HIGH;

bool ledOn = false;
bool heaterOn = false;
bool coolerOn = false;
bool relayOn = false;
float temperatureSetpoint = 25.0F;
float humiditySetpoint = 60.0F;

void writeOutput(uint8_t pin, uint8_t onLevel, bool enabled) {
  digitalWrite(pin, enabled ? onLevel : !onLevel);
}

ControlResult requireBoolean(const ControlValue& value) {
  if (value.kind != ControlValueKind::Boolean) {
    return {false, "value must be boolean"};
  }
  return {true, "valid"};
}

ControlResult requireNumberInRange(const ControlValue& value, float minimum,
                                   float maximum) {
  if (value.kind != ControlValueKind::Number || !std::isfinite(value.numberValue)) {
    return {false, "value must be a finite number"};
  }
  if (value.numberValue < minimum || value.numberValue > maximum) {
    return {false, "value is outside the allowed range"};
  }
  return {true, "valid"};
}
}  // namespace

void initializeActuators() {
  if (ENABLE_LED_OUTPUT) {
    pinMode(LED_PIN, OUTPUT);
    writeOutput(LED_PIN, LED_ON_LEVEL, false);
  }
  if (ENABLE_HEATER_OUTPUT) {
    pinMode(HEATER_PIN, OUTPUT);
    writeOutput(HEATER_PIN, HEATER_ON_LEVEL, false);
  }
  if (ENABLE_COOLER_OUTPUT) {
    pinMode(COOLER_PIN, OUTPUT);
    writeOutput(COOLER_PIN, COOLER_ON_LEVEL, false);
  }
  if (ENABLE_RELAY_OUTPUT) {
    pinMode(RELAY_PIN, OUTPUT);
    writeOutput(RELAY_PIN, RELAY_ON_LEVEL, false);
  }
}

ControlResult applyControlAction(const char* target, const ControlValue& value) {
  if (target == nullptr) {
    return {false, "target is missing"};
  }

  if (std::strcmp(target, "led") == 0) {
    const ControlResult validation = requireBoolean(value);
    if (!validation.success) return validation;
    if (!ENABLE_LED_OUTPUT) return {false, "LED output is disabled"};
    ledOn = value.booleanValue;
    writeOutput(LED_PIN, LED_ON_LEVEL, ledOn);
    return {true, ledOn ? "LED turned on" : "LED turned off"};
  }

  if (std::strcmp(target, "heater") == 0) {
    const ControlResult validation = requireBoolean(value);
    if (!validation.success) return validation;
    if (!ENABLE_HEATER_OUTPUT) return {false, "heater output is disabled"};
    if (value.booleanValue && coolerOn) {
      return {false, "heater blocked because cooler is on"};
    }
    heaterOn = value.booleanValue;
    writeOutput(HEATER_PIN, HEATER_ON_LEVEL, heaterOn);
    return {true, heaterOn ? "heater turned on" : "heater turned off"};
  }

  if (std::strcmp(target, "cooler") == 0) {
    const ControlResult validation = requireBoolean(value);
    if (!validation.success) return validation;
    if (!ENABLE_COOLER_OUTPUT) return {false, "cooler output is disabled"};
    if (value.booleanValue && heaterOn) {
      return {false, "cooler blocked because heater is on"};
    }
    coolerOn = value.booleanValue;
    writeOutput(COOLER_PIN, COOLER_ON_LEVEL, coolerOn);
    return {true, coolerOn ? "cooler turned on" : "cooler turned off"};
  }

  if (std::strcmp(target, "relay") == 0) {
    const ControlResult validation = requireBoolean(value);
    if (!validation.success) return validation;
    if (!ENABLE_RELAY_OUTPUT) return {false, "relay output is disabled"};
    relayOn = value.booleanValue;
    writeOutput(RELAY_PIN, RELAY_ON_LEVEL, relayOn);
    return {true, relayOn ? "relay turned on" : "relay turned off"};
  }

  if (std::strcmp(target, "temperature_setpoint") == 0) {
    const ControlResult validation = requireNumberInRange(value, -40.0F, 125.0F);
    if (!validation.success) return validation;
    temperatureSetpoint = value.numberValue;
    return {true, "temperature setpoint updated"};
  }

  if (std::strcmp(target, "humidity_setpoint") == 0) {
    const ControlResult validation = requireNumberInRange(value, 0.0F, 100.0F);
    if (!validation.success) return validation;
    humiditySetpoint = value.numberValue;
    return {true, "humidity setpoint updated"};
  }

  return {false, "unknown target"};
}

float getTemperatureSetpoint() { return temperatureSetpoint; }

float getHumiditySetpoint() { return humiditySetpoint; }
