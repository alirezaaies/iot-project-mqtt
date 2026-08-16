# Code Directory

The code is split into a few folders based on practical tasks.

| Directory | Purpose |
|---|---|
| `arduino/` | ESP8266 sketch, sensor reading, Wi-Fi, and device-side MQTT code |
| `backend/` | Python logic, MQTT receiver/sender, validation, and coordination |
| `database/` | SQLite schema and all database access code |
| `webapp/` | Flask routes, templates, static files, and HTTP API |
| `contracts/` | Topic names and message examples shared by device and Python code |
| `tests/` | Tests covering connections between these folders |

## Simple dependency rule

```text
Arduino <-> MQTT <-> Backend <-> Database
                         |
                       Webapp
```

The web application should call backend functions. SQL should stay in the database folder. Arduino and Python should agree through files in the contracts folder.

Do not create extra layers or subdirectories until the number of files makes them necessary.
