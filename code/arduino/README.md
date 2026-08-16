# Arduino and ESP8266

Put the first ESP8266 sketch and its hardware-related files in this directory.

When implementation starts, the main sketch can be:

```text
code/arduino/arduino.ino
```

This filename matches the current sketch directory and works naturally with the Arduino IDE. Supporting `.h` and `.cpp` files can remain beside it until the project becomes large.

This directory will contain sensor reading, Wi-Fi connection, MQTT communication, and device command handling. Do not store passwords in the committed sketch.
