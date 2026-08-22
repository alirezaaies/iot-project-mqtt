"""SQLite storage module for normalized MQTT sensor readings."""

from __future__ import annotations

import argparse
import json
import sqlite3
from pathlib import Path
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DATABASE_PATH = PROJECT_ROOT / "runtime" / "iot.db"

SCHEMA = """
CREATE TABLE IF NOT EXISTS telemetry_batches (
    id INTEGER PRIMARY KEY,
    received_at_utc TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    topic TEXT NOT NULL,
    device_id TEXT NOT NULL,
    message_id INTEGER NOT NULL,
    sampled_at_ms INTEGER NOT NULL,
    batch_index INTEGER NOT NULL,
    batch_count INTEGER NOT NULL,
    payload_json TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS sensor_readings (
    id INTEGER PRIMARY KEY,
    batch_id INTEGER NOT NULL REFERENCES telemetry_batches(id) ON DELETE CASCADE,
    sensor_id TEXT NOT NULL,
    value REAL NOT NULL,
    unit TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_batches_device_received
ON telemetry_batches(device_id, received_at_utc DESC);

CREATE INDEX IF NOT EXISTS idx_readings_sensor
ON sensor_readings(sensor_id, id DESC);
"""


class SensorDatabase:
    """Owns the SQLite connection and all SQL used by the collector."""

    def __init__(self, path: Path | str = DEFAULT_DATABASE_PATH) -> None:
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.connection = sqlite3.connect(self.path)
        self.connection.row_factory = sqlite3.Row
        self.connection.execute("PRAGMA foreign_keys = ON")
        self.connection.execute("PRAGMA journal_mode = WAL")
        self.create_tables()

    def create_tables(self) -> None:
        """Create missing tables and indexes without deleting existing data."""
        self.connection.executescript(SCHEMA)

    def save_telemetry_batch(self, topic: str, payload: dict[str, Any]) -> int:
        """Save one MQTT batch and all its readings in one transaction."""
        readings = payload["readings"]
        with self.connection:
            cursor = self.connection.execute(
                """
                INSERT INTO telemetry_batches (
                    topic, device_id, message_id, sampled_at_ms,
                    batch_index, batch_count, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    topic,
                    payload["device_id"],
                    payload["message_id"],
                    payload["sampled_at_ms"],
                    payload["batch_index"],
                    payload["batch_count"],
                    json.dumps(payload, separators=(",", ":")),
                ),
            )
            batch_id = int(cursor.lastrowid)
            self.connection.executemany(
                """
                INSERT INTO sensor_readings (batch_id, sensor_id, value, unit)
                VALUES (?, ?, ?, ?)
                """,
                [
                    (batch_id, item["sensor_id"], item["value"], item["unit"])
                    for item in readings
                ],
            )
        return batch_id

    def recent_readings(self, limit: int = 20) -> list[sqlite3.Row]:
        """Return the newest normalized readings for inspection or reporting."""
        return self.connection.execute(
            """
            SELECT
                b.received_at_utc,
                b.device_id,
                b.message_id,
                r.sensor_id,
                r.value,
                r.unit
            FROM sensor_readings AS r
            JOIN telemetry_batches AS b ON b.id = r.batch_id
            ORDER BY r.id DESC
            LIMIT ?
            """,
            (limit,),
        ).fetchall()

    def close(self) -> None:
        """Close the SQLite file cleanly."""
        self.connection.close()

    def __enter__(self) -> "SensorDatabase":
        return self

    def __exit__(self, *_: object) -> None:
        self.close()


def main() -> None:
    parser = argparse.ArgumentParser(description="Initialize or inspect IoT data")
    parser.add_argument("--database", type=Path, default=DEFAULT_DATABASE_PATH)
    parser.add_argument("--recent", type=int, metavar="COUNT")
    args = parser.parse_args()

    with SensorDatabase(args.database) as database:
        print(f"Database ready: {database.path}")
        if args.recent is not None:
            for row in database.recent_readings(args.recent):
                print(
                    f"{row['received_at_utc']} {row['device_id']} "
                    f"{row['sensor_id']}={row['value']} {row['unit']}"
                )


if __name__ == "__main__":
    main()
