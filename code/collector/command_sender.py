"""Send one or more control actions to an ESP8266 and wait for its response."""

from __future__ import annotations

import argparse
import json
import os
import re
import threading
import uuid
from typing import Any

import paho.mqtt.client as mqtt


MQTT_HOST = os.getenv("IOT_MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("IOT_MQTT_PORT", "1883"))
MAX_ACTIONS = 8
TARGET_PATTERN = re.compile(r"^[a-z][a-z0-9_]{0,63}$")
# Device IDs are also used as one MQTT topic level. A hyphen is safe there and
# is already used by the firmware's default ID: esp8266-01.
DEVICE_ID_PATTERN = re.compile(r"^[a-z][a-z0-9_-]{0,63}$")


class CommandError(ValueError):
    """Raised for invalid actions, connection failures, or response timeouts."""


def validate_device_id(device_id: str) -> None:
    """Accept simple topic-safe device IDs, including hyphens."""
    if not DEVICE_ID_PATTERN.fullmatch(device_id):
        raise CommandError("device_id contains unsupported characters")


def parse_action(text: str) -> dict[str, Any]:
    """Convert a CLI value such as led=on or temperature_setpoint=24.5."""
    if "=" not in text:
        raise CommandError("action must use target=value")
    target, raw_value = (part.strip() for part in text.split("=", 1))
    if not TARGET_PATTERN.fullmatch(target):
        raise CommandError(f"invalid target: {target!r}")

    lowered = raw_value.lower()
    if lowered in {"on", "true"}:
        value: bool | float = True
    elif lowered in {"off", "false"}:
        value = False
    else:
        try:
            value = float(raw_value)
        except ValueError as error:
            raise CommandError(
                f"value for {target!r} must be on, off, true, false, or a number"
            ) from error

    return {"target": target, "value": value}


def build_command(actions: list[dict[str, Any]], command_id: str | None = None) -> dict:
    """Build the versioned JSON object published to the device."""
    if not 1 <= len(actions) <= MAX_ACTIONS:
        raise CommandError(f"a command must contain 1 to {MAX_ACTIONS} actions")
    return {
        "schema_version": 1,
        "command_id": command_id or uuid.uuid4().hex,
        "actions": actions,
    }


class CommandSender:
    """Coordinates subscribe-before-publish and response correlation."""

    def __init__(self, host: str = MQTT_HOST, port: int = MQTT_PORT) -> None:
        self.host = host
        self.port = port
        self.connected = threading.Event()
        self.subscribed = threading.Event()
        self.response_received = threading.Event()
        self.connection_error: str | None = None
        self.expected_command_id = ""
        self.response: dict[str, Any] | None = None

        self.client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=f"iot-command-sender-{uuid.uuid4().hex[:8]}",
            protocol=mqtt.MQTTv311,
        )
        self.client.on_connect = self._on_connect
        self.client.on_subscribe = self._on_subscribe
        self.client.on_message = self._on_message

    def _on_connect(
        self,
        client: mqtt.Client,
        _userdata: object,
        _flags: mqtt.ConnectFlags,
        reason_code: mqtt.ReasonCode,
        _properties: mqtt.Properties | None,
    ) -> None:
        if reason_code.is_failure:
            self.connection_error = str(reason_code)
        self.connected.set()

    def _on_subscribe(
        self,
        _client: mqtt.Client,
        _userdata: object,
        _mid: int,
        reason_codes: list[mqtt.ReasonCode],
        _properties: mqtt.Properties | None,
    ) -> None:
        if any(code.is_failure for code in reason_codes):
            self.connection_error = "broker rejected the response subscription"
        self.subscribed.set()

    def _on_message(
        self, _client: mqtt.Client, _userdata: object, message: mqtt.MQTTMessage
    ) -> None:
        try:
            response = json.loads(message.payload.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return
        if response.get("command_id") == self.expected_command_id:
            self.response = response
            self.response_received.set()

    def send(
        self, device_id: str, command: dict[str, Any], timeout: float = 10.0
    ) -> dict[str, Any]:
        """Publish a command and return the matching device response."""
        validate_device_id(device_id)

        # Reset per-command state so one instance can safely be reused by a
        # later web or automation module.
        self.connected.clear()
        self.subscribed.clear()
        self.response_received.clear()
        self.connection_error = None
        self.response = None
        self.expected_command_id = str(command["command_id"])
        response_topic = f"iot/devices/{device_id}/response"
        control_topic = f"iot/devices/{device_id}/control"

        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()
        try:
            if not self.connected.wait(timeout):
                raise CommandError("timed out while connecting to MQTT")
            if self.connection_error:
                raise CommandError(f"MQTT connection failed: {self.connection_error}")

            self.client.subscribe(response_topic, qos=1)
            if not self.subscribed.wait(timeout):
                raise CommandError("timed out while subscribing to the response topic")
            if self.connection_error:
                raise CommandError(self.connection_error)

            publication = self.client.publish(
                control_topic,
                json.dumps(command, separators=(",", ":")),
                qos=1,
            )
            publication.wait_for_publish(timeout=timeout)
            if publication.rc != mqtt.MQTT_ERR_SUCCESS:
                raise CommandError(f"MQTT publish failed with code {publication.rc}")

            if not self.response_received.wait(timeout):
                raise CommandError(
                    "device response timed out; check device power, topic, and serial output"
                )
            if self.response is None:
                raise CommandError("device returned no usable response")
            return self.response
        finally:
            self.client.disconnect()
            self.client.loop_stop()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Send one or more actions to an IoT device"
    )
    parser.add_argument("device_id", help="device ID, for example esp8266-01")
    parser.add_argument(
        "--set",
        dest="actions",
        action="append",
        required=True,
        metavar="TARGET=VALUE",
        help="repeat for multiple actions; example: --set led=on",
    )
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()

    try:
        actions = [parse_action(item) for item in args.actions]
        command = build_command(actions)
        print("Sending command:")
        print(json.dumps(command, indent=2))
        response = CommandSender().send(args.device_id, command, args.timeout)
        print("Device response:")
        print(json.dumps(response, indent=2))
        if not response.get("success", False):
            raise SystemExit(2)
    except CommandError as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
