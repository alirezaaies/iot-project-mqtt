#pragma once

#include <Arduino.h>

enum class ControlValueKind {
  Boolean,
  Number,
};

struct ControlValue {
  ControlValueKind kind;
  bool booleanValue;
  float numberValue;
};

struct ControlResult {
  bool success;
  const char* message;
};

// Configures only outputs explicitly enabled in actuators.cpp. All enabled
// outputs start in their safe OFF state.
void initializeActuators();

// Validates and applies one action. Hardware safety rules remain in this
// module instead of being duplicated in MQTT callback code.
ControlResult applyControlAction(const char* target, const ControlValue& value);

// Future automatic-control logic can read setpoints without depending on MQTT
// or accessing this module's private state directly.
float getTemperatureSetpoint();
float getHumiditySetpoint();
