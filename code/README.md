# Code Directory

The code is split into a few folders based on practical tasks.

| Directory | Purpose |
|---|---|
| `arduino/` | ESP8266 sketch, sensor reading, actuators, Wi-Fi, and device-side MQTT code |
| `collector/` | Python MQTT receiving, command sending, validation, and SQLite storage |
| `webapp/` | Flask routes, templates, static files, and HTTP API |
| `contracts/` | Topic names and message examples shared by device and Python code |
| `tests/` | Tests covering connections between these folders |

## Simple dependency rule

```text
Sensors -> ESP8266 -> MQTT -> Collector -> SQLite
                         ^
                         |
                  Command sender
```

SQL stays in `collector/database.py`; MQTT callbacks call its methods instead
of containing SQL. Arduino and Python use the documented versioned JSON
contract. Device output logic stays in `arduino/mqtt/src/actuators.cpp`, not
inside the MQTT callback.

Do not create extra layers or subdirectories until the number of files makes them necessary.
