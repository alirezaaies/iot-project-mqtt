# ESP8266 firmware

The first firmware module is the PlatformIO project in `mqtt/`. It connects a
NodeMCU ESP8266 to Wi-Fi and publishes test telemetry to a local MQTT broker.

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
chapter 5 of the Persian report.
