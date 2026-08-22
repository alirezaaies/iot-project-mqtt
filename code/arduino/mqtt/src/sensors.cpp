#include "sensors.h"

namespace {
constexpr size_t CURRENT_SENSOR_COUNT = 2;
}

size_t readSensors(SensorReading* readings, size_t capacity) {
  if (readings == nullptr || capacity < CURRENT_SENSOR_COUNT) {
    return 0;
  }

  // Simulated values keep this module testable without physical sensors.
  // Replace only these value expressions when real sensor hardware is added.
  readings[0] = {"temperature", random(200, 351) / 10.0F, "celsius"};
  readings[1] = {"humidity", random(400, 901) / 10.0F, "percent"};

  // Example extension:
  // readings[2] = {"room_2_temperature", sensor.readTemperature(), "celsius"};
  // Remember to update CURRENT_SENSOR_COUNT when adding a reading.
  return CURRENT_SENSOR_COUNT;
}
