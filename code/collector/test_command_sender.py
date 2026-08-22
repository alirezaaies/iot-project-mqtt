"""Independent tests for command parsing and message construction."""

import unittest

from command_sender import (
    CommandError,
    build_command,
    parse_action,
    validate_device_id,
)


class CommandSenderTests(unittest.TestCase):
    def test_boolean_actions_are_parsed(self) -> None:
        self.assertEqual(parse_action("led=on"), {"target": "led", "value": True})
        self.assertEqual(
            parse_action("heater=off"), {"target": "heater", "value": False}
        )

    def test_numeric_setpoint_is_parsed(self) -> None:
        self.assertEqual(
            parse_action("temperature_setpoint=24.5"),
            {"target": "temperature_setpoint", "value": 24.5},
        )

    def test_firmware_device_id_with_hyphen_is_valid(self) -> None:
        # This is the default DEVICE_ID in the ESP8266 firmware.
        validate_device_id("esp8266-01")

    def test_multiple_actions_share_one_command_id(self) -> None:
        command = build_command(
            [parse_action("led=on"), parse_action("humidity_setpoint=60")],
            command_id="test-command",
        )
        self.assertEqual(command["command_id"], "test-command")
        self.assertEqual(len(command["actions"]), 2)

    def test_invalid_text_value_is_rejected(self) -> None:
        with self.assertRaises(CommandError):
            parse_action("led=maybe")

    def test_more_than_eight_actions_is_rejected(self) -> None:
        with self.assertRaises(CommandError):
            build_command([{"target": "led", "value": True}] * 9)


if __name__ == "__main__":
    unittest.main()
