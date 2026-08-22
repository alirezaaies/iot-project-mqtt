# Local MQTT collector

This folder contains two small modules:

- `database.py` owns the SQLite schema and database operations;
- `mqtt_collector.py` validates MQTT telemetry and calls the database module.
- `command_sender.py` sends one or more control actions and waits for the
  matching device response.

Python 3.10 or newer is required.

Create a virtual environment, install `requirements.txt`, initialize the
database, and start the collector. The complete Windows, Linux, and macOS
instructions and expected outputs are in chapters 4 and 5 of the Persian guide.

Local defaults are `localhost:1883`, topic `iot/devices/+/telemetry`, and
database `runtime/iot.db`. Override them with `IOT_MQTT_HOST`,
`IOT_MQTT_PORT`, `IOT_MQTT_TOPIC`, or `IOT_DATABASE_PATH` environment
variables when needed.
