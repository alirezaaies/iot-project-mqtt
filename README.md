# IoT Monitoring and Control Project

A simple IoT project for receiving sensor data, storing it on disk, displaying it in a web application, and sending control commands to devices.

The first version will use ESP8266, Arduino code, MQTT, Python, SQLite, and Flask. The folders are separated so each part can be developed and tested on its own without introducing a complicated architecture.

## Current status

The broker, device telemetry, local storage, and command-control steps are implemented. The local MQTT broker is tested independently, then the
ESP8266 publishes status and normalized temperature and humidity telemetry.
The same format supports larger sensor sets by splitting them into numbered
MQTT batches. A Python collector now validates those batches and stores
normalized readings in local SQLite tables. A separate Python command sender
can send one or several actions to the ESP8266 and waits for the matching
device response. The web module remains intentionally unimplemented.

## Project structure

```text
.
├── code/
│   ├── arduino/       # ESP8266 sketch and hardware-related files
│   ├── collector/     # MQTT receiving, validation, and SQLite storage
│   ├── contracts/     # MQTT topics and JSON message formats
│   ├── webapp/        # Flask application, pages, templates, and static files
│   └── tests/         # Tests that connect multiple parts of the project
├── documentation/     # Persian LaTeX implementation report
├── figures/           # Report images and diagrams
├── references/        # Bibliography
├── runtime/           # Local database, logs, and generated runtime data
└── tables/            # Reusable report tables
```

## What belongs where?

| Work | Directory |
|---|---|
| Arduino or ESP8266 code | `code/arduino/` |
| Receiving MQTT messages in Python | `code/collector/mqtt_collector.py` |
| Sending device commands in Python | `code/collector/command_sender.py` |
| Message validation | `code/collector/mqtt_collector.py` |
| SQLite tables and database operations | `code/collector/database.py` |
| Flask routes, HTML templates, CSS, and JavaScript | `code/webapp/` |
| MQTT topic names and JSON examples | `code/contracts/` |
| Local database and logs created while running | `runtime/` |
| Persian implementation notes and test results | `documentation/` |

## Keeping future changes easy

- To replace Flask with Django, replace the implementation inside `code/webapp/`. The Arduino, MQTT, and database folders should not need to change.
- To replace SQLite with PostgreSQL, replace `code/collector/database.py`. The MQTT collector already calls its methods instead of writing SQL directly.
- To add another board, create another clearly named folder next to `code/arduino/` when it is actually needed.
- Shared message formats belong in `code/contracts/`, so Arduino and Python use the same definitions.

This is a simple separation rule, not a requirement to build every possible replacement now.

## Recommended implementation order

1. Test the local MQTT broker without the ESP8266.
2. Publish status and test telemetry from the ESP8266.
3. Store received data in a local SQLite database. (Implemented)
4. Read and display stored data with a small Python module.
5. Add safe single-action and multi-action control commands. (Implemented)
6. Add a simple web application.
7. Add integration tests and local deployment instructions.

Each step should work independently before it is connected to the next step.

## Documentation

The main README and folder READMEs are written in English. The implementation report is written in Persian and built with XeLaTeX. See [documentation/README.md](documentation/README.md).

## License

This project is licensed under the [MIT License](LICENSE).
