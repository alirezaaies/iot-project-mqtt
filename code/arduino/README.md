# ESP8266 firmware

The firmware is the PlatformIO project in `mqtt/`. It connects a NodeMCU
ESP8266 to Wi-Fi and publishes normalized sensor readings to a local MQTT
broker. Large sensor sets are automatically split into numbered MQTT batches.
It also subscribes to a device-specific control topic, applies validated
actions through the actuator module, and publishes a correlated response.

Before building, copy `mqtt/include/secrets.example.h` to
`mqtt/include/secrets.h` and enter the local Wi-Fi credentials. The copied file
is ignored by Git.

Run these commands from `code/arduino/mqtt`:

```bash
pio run
pio run --target upload
pio device monitor
```

Broker setup and the complete Windows, Linux, and macOS test procedure are in
chapters 2 and 3 of the Persian report.

Sensor hardware is isolated in `mqtt/src/sensors.cpp`. Add or replace readings
there without changing Wi-Fi, MQTT, or JSON packaging code. The initial values
simulate one temperature and one humidity sensor.

Output hardware is isolated in `mqtt/src/actuators.cpp`. The built-in LED is
enabled for the first test. Heater, cooler, and relay outputs are disabled by
default and must only be enabled after their pins, driver circuits, active
levels, and independent safety protections have been checked.
