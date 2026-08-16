# Database

Put everything related to persistent data here:

- SQLite table definitions;
- database initialization;
- schema changes or migrations;
- Python functions that insert, update, and read data;
- database-specific tests.

The first database file itself should be created under `runtime/`, not committed to Git.

Backend and web code should not contain scattered SQL queries. They should use the functions provided by this directory. If SQLite is replaced later, most changes should remain here.
