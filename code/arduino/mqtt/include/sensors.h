#pragma once

#include <Arduino.h>

// One normalized numeric reading. Stable IDs are used by MQTT consumers and
// should not be renamed after data collection has started.
struct SensorReading {
  const char* sensorId;
  float value;
  const char* unit;
};

// Writes at most `capacity` readings into `readings` and returns the number
// written. This keeps hardware-specific code out of MQTT packaging code.
size_t readSensors(SensorReading* readings, size_t capacity);
