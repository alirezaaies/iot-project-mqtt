"""Small independent tests for validation and SQLite storage."""

import tempfile
import unittest
from pathlib import Path

from database import SensorDatabase
from mqtt_collector import InvalidTelemetry, validate_telemetry


def sample_payload() -> dict:
    return {
        "schema_version": 1,
        "device_id": "esp8266-01",
        "message_id": 7,
        "sampled_at_ms": 12000,
        "batch_index": 1,
        "batch_count": 1,
        "readings": [
            {"sensor_id": "temperature", "value": 24.6, "unit": "celsius"},
            {"sensor_id": "humidity", "value": 58.2, "unit": "percent"},
        ],
    }


class CollectorTests(unittest.TestCase):
    def test_valid_payload_is_saved_as_two_rows(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.db"
            with SensorDatabase(path) as database:
                payload = validate_telemetry(sample_payload())
                batch_id = database.save_telemetry_batch(
                    "iot/devices/esp8266-01/telemetry", payload
                )
                rows = database.recent_readings()

            self.assertEqual(batch_id, 1)
            self.assertEqual(len(rows), 2)
            self.assertEqual({row["sensor_id"] for row in rows}, {"temperature", "humidity"})

    def test_missing_readings_is_rejected(self) -> None:
        payload = sample_payload()
        del payload["readings"]
        with self.assertRaises(InvalidTelemetry):
            validate_telemetry(payload)

    def test_boolean_sensor_value_is_rejected(self) -> None:
        payload = sample_payload()
        payload["readings"][0]["value"] = True
        with self.assertRaises(InvalidTelemetry):
            validate_telemetry(payload)


if __name__ == "__main__":
    unittest.main()
