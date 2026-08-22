# Runtime Data

This directory is reserved for local databases, logs, broker state, uploaded files, and other generated runtime data. Its contents are ignored by Git except for this file.

The collector currently creates:

```text
runtime/
`-- iot.db       # Local SQLite dataset; ignored by Git
```

Do not store source code, documentation, or secrets here.
