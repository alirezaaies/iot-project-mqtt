"""Receive ESP8266 telemetry from MQTT and store it through database.py."""

from __future__ import annotations

import json
import logging
import math
import os
from pathlib import Path
from typing import Any

import paho.mqtt.client as mqtt

from database import DEFAULT_DATABASE_PATH, SensorDatabase


MQTT_HOST = os.getenv("IOT_MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("IOT_MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("IOT_MQTT_TOPIC", "iot/devices/+/telemetry")
DATABASE_PATH = Path(os.getenv("IOT_DATABASE_PATH", str(DEFAULT_DATABASE_PATH)))

LOGGER = logging.getLogger("iot-collector")


class InvalidTelemetry(ValueError):
    """Raised when an MQTT payload does not follow schema version 1."""


def validate_telemetry(payload: Any) -> dict[str, Any]:
    """Validate and return a schema-version-1 telemetry dictionary."""
    if not isinstance(payload, dict):
        raise InvalidTelemetry("payload must be a JSON object")

    required = {
        "schema_version",
        "device_id",
        "message_id",
        "sampled_at_ms",
        "batch_index",
        "batch_count",
        "readings",
    }
    missing = required - payload.keys()
    if missing:
        raise InvalidTelemetry(f"missing fields: {', '.join(sorted(missing))}")

    if payload["schema_version"] != 1:
        raise InvalidTelemetry("unsupported schema_version")
    if not isinstance(payload["device_id"], str) or not payload["device_id"]:
        raise InvalidTelemetry("device_id must be a non-empty string")

    for field in ("message_id", "sampled_at_ms", "batch_index", "batch_count"):
        if isinstance(payload[field], bool) or not isinstance(payload[field], int):
            raise InvalidTelemetry(f"{field} must be an integer")

    if payload["message_id"] < 0 or payload["sampled_at_ms"] < 0:
        raise InvalidTelemetry("message_id and sampled_at_ms cannot be negative")
    if not 1 <= payload["batch_index"] <= payload["batch_count"]:
        raise InvalidTelemetry("batch_index must be between 1 and batch_count")

    readings = payload["readings"]
    if not isinstance(readings, list) or not readings:
        raise InvalidTelemetry("readings must be a non-empty list")

    for index, reading in enumerate(readings):
        if not isinstance(reading, dict):
            raise InvalidTelemetry(f"readings[{index}] must be an object")
        if set(reading) != {"sensor_id", "value", "unit"}:
            raise InvalidTelemetry(
                f"readings[{index}] must contain sensor_id, value, and unit"
            )
        if not isinstance(reading["sensor_id"], str) or not reading["sensor_id"]:
            raise InvalidTelemetry(f"readings[{index}].sensor_id is invalid")
        if len(reading["sensor_id"]) > 64:
            raise InvalidTelemetry(f"readings[{index}].sensor_id is too long")
        if not isinstance(reading["unit"], str) or len(reading["unit"]) > 32:
            raise InvalidTelemetry(f"readings[{index}].unit is invalid")
        value = reading["value"]
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise InvalidTelemetry(f"readings[{index}].value must be numeric")
        if not math.isfinite(value):
            raise InvalidTelemetry(f"readings[{index}].value must be finite")

    return payload


def decode_message(raw_payload: bytes) -> dict[str, Any]:
    """Decode UTF-8 JSON and validate its telemetry structure."""
    try:
        decoded = raw_payload.decode("utf-8")
        payload = json.loads(decoded)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise InvalidTelemetry(f"invalid UTF-8 JSON: {error}") from error
    return validate_telemetry(payload)


def on_connect(
    client: mqtt.Client,
    _userdata: SensorDatabase,
    _flags: mqtt.ConnectFlags,
    reason_code: mqtt.ReasonCode,
    _properties: mqtt.Properties | None,
) -> None:
    """Subscribe after each successful connection or reconnection."""
    if reason_code.is_failure:
        LOGGER.error("MQTT connection rejected: %s", reason_code)
        return
    client.subscribe(MQTT_TOPIC, qos=0)
    LOGGER.info("Connected to MQTT; subscribed to %s", MQTT_TOPIC)


def on_message(
    _client: mqtt.Client, database: SensorDatabase, message: mqtt.MQTTMessage
) -> None:
    """Validate one MQTT message and store it without stopping the loop."""
    try:
        payload = decode_message(message.payload)
        batch_id = database.save_telemetry_batch(message.topic, payload)
        LOGGER.info(
            "Saved batch %s: device=%s message=%s batch=%s/%s readings=%s",
            batch_id,
            payload["device_id"],
            payload["message_id"],
            payload["batch_index"],
            payload["batch_count"],
            len(payload["readings"]),
        )
    except InvalidTelemetry as error:
        LOGGER.warning("Ignored invalid message on %s: %s", message.topic, error)
    except Exception:
        LOGGER.exception("Could not store message from %s", message.topic)


def run() -> None:
    """Create the database and run the MQTT network loop until interrupted."""
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    database = SensorDatabase(DATABASE_PATH)
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="iot-local-collector",
        protocol=mqtt.MQTTv311,
        userdata=database,
    )
    client.on_connect = on_connect
    client.on_message = on_message

    LOGGER.info("Database: %s", DATABASE_PATH)
    LOGGER.info("Connecting to MQTT broker at %s:%s", MQTT_HOST, MQTT_PORT)
    try:
        client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
        client.loop_forever(retry_first_connection=True)
    except KeyboardInterrupt:
        LOGGER.info("Collector stopped by user")
    finally:
        client.disconnect()
        database.close()


if __name__ == "__main__":
    run()
