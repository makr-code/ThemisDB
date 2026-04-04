# Troubleshooting Guide: Metadata Module

**Module:** `src/metadata`  
**Version:** 2026 Q1

---

## Statistics Collector

### `TABLE_NOT_FOUND` on `collectStats()`

**Symptom:** `StatsResult::error == StatsErrorCode::TABLE_NOT_FOUND`

**Causes:**
1. Table name is empty.
2. Table has no rows — the prefix scan finds nothing.
3. Table prefix does not match the key format `<table>:<pk>`.

**Fix:**
- Confirm the table has been registered with `SchemaManager` and has at least one stored entity.
- Verify the key format used by `RocksDBWrapper::put()` matches `<table>:<pk>`.

---

### `ITERATOR_ERROR` on `collectStats()`

**Symptom:** `StatsResult::error == StatsErrorCode::ITERATOR_ERROR`

**Cause:** `RocksDBWrapper::newIterator()` returned `nullopt`, typically because the database is not open.

**Fix:**
- Verify `db.isOpen()` returns `true` before calling `collectStats()`.
- Check RocksDB logs for compaction or corruption errors.

---

### Statistics are stale

**Symptom:** Query planner uses wrong cardinality estimates; `last_updated` timestamp is old.

**Fix:**
- Call `collectStats()` or `PUT /api/v1/metadata/stats/<table>` to force a refresh.
- Check if the automatic refresh schedule is running (v2.0 feature).
- Ensure sufficient disk space for RocksDB to flush.

---

### Statistics differ between restarts

**Symptom:** `getStats()` returns different results after server restart.

**Cause:** The in-memory cache is cold on startup; `getStats()` reads from the `stats:<table>` RocksDB key.  If the key is missing (e.g., first run), it returns `TABLE_NOT_FOUND`.

**Fix:** Call `collectStats()` at least once to populate the persistent key.

---

## Schema Constraints

### Constraint violations not reported

**Symptom:** Rows with NULL values pass `enforce()` without a violation.

**Cause:** No `NOT_NULL` constraint has been registered for that column.

**Fix:**
```cpp
sc.addConstraint("users", "email", ColumnConstraint::makeNotNull("nn_users_email"));
sc.persistTo(db);
```

---

### Constraints lost after restart

**Symptom:** Constraints are enforced during the current session but disappear on restart.

**Cause:** `persistTo()` was not called, or `loadFrom()` failed during startup.

**Fix:**
- Always call `persistTo(db)` after modifying constraints.
- Check `SchemaConstraints::loadFrom()` return value and log output.
- Inspect key `config:constraints:<table>` in RocksDB with `ldb`:
  ```bash
  ldb --db=/path/to/db scan --from="config:constraints:" --to="config:constraints:~"
  ```

---

### `UNIQUE_VIOLATION` on bulk import

**Symptom:** Batch constraint validation returns violations for unique columns even though values look distinct.

**Cause:** The `UNIQUE` constraint in `SchemaConstraints` uses an in-memory set for cross-row checking.  If the same batch request is sent twice, or if existing rows already have conflicting values, violations are expected.

**Fix:** Query existing values before import, or use `applyDefaults()` followed by `enforce()` per row.

---

## Schema Versioning

### `VERSION_NOT_FOUND` on `rollbackToVersion()`

**Symptom:** Rollback fails with `VersionErrorCode::VERSION_NOT_FOUND`.

**Cause:** The requested version number has never been created for this table.

**Fix:**
```bash
curl http://localhost:8080/api/v1/schema/versions/<table> | jq '.history[].version'
```
Select a version that appears in the history.

---

### `TABLE_NOT_FOUND` on `createSchemaVersion()`

**Symptom:** `VersionResult::error == VersionErrorCode::TABLE_NOT_FOUND`.

**Cause:** The table has not been registered in `SchemaManager` yet.

**Fix:** Call `PUT /api/v1/schema/<table>` to register the schema first, then snapshot.

---

### Version history is empty after restart

**Symptom:** `getChangeHistory()` returns an empty list even though versions were created.

**Cause:** The `config:schema_version:<table>:current` or version keys were not written to disk (RocksDB write failure).

**Fix:**
```bash
ldb --db=/path/to/db scan --from="config:schema_version:" --to="config:schema_version:~"
```
If no keys exist, the history was never persisted.  Check the `spdlog` output for `STORAGE_ERROR` entries.

---

## Audit Log

### Audit entries missing

**Symptom:** `GET /api/v1/metadata/audit/<table>` returns an empty array.

**Causes:**
1. `SchemaAuditLog` was not attached to `SchemaVersionManager` via `setAuditLog()`.
2. `record()` returned `false` due to a RocksDB write error.
3. The `audit:schema:` prefix scan is failing.

**Fix:**
- Confirm `setAuditLog()` is called during initialization (should be automatic in `HttpServer::init()`).
- Check `spdlog` for `SchemaAuditLog: Failed to persist entry` warnings.
- Scan the audit namespace:
  ```bash
  ldb --db=/path/to/db scan --from="audit:schema:" --to="audit:schema:~"
  ```

---

## Schema Import

### Partial import (HTTP 207)

**Symptom:** `PUT /api/v1/metadata/schema_import` returns HTTP 207 with some tables in `errors`.

**Cause:** One or more schemas in the `tables` array is malformed (missing `name`, bad JSON, etc.).

**Fix:** Check the `errors` array in the response body:
```json
{
  "errors": [
    {"error": "Table schema missing 'name' field"},
    {"table": "bad_table", "error": "Failed to register schema"}
  ]
}
```
Fix the offending schema definitions and retry.

---

## INFORMATION_SCHEMA

### `columns` view returns empty

**Symptom:** `GET /api/v1/information_schema/columns/<table>` returns `[]`.

**Cause:** The table exists but has no `properties` defined in its `TableSchema`.

**Fix:** Re-register the schema with at least one property:
```bash
curl -X PUT http://localhost:8080/api/v1/schema/users \
  -d '{"name":"users","type":"relational","properties":[{"name":"id","type":"integer"}]}'
```

---

## General RocksDB Issues

### Keys missing or corrupted

Run the built-in consistency check:

```bash
ldb --db=/path/to/db check_consistency
```

If corruption is detected, follow the **Recovery Runbook** at [`docs/metadata/recovery_runbook.md`](./recovery_runbook.md).

---

## Common Error Codes

| Code | Enum | Meaning |
|------|------|---------|
| `0`  | `OK` | Success |
| `1`  | `TABLE_NOT_FOUND` | Table not registered or has no data |
| `2`  | `COLLECTION_FAILED` | Stats scan failed partway through |
| `3`  | `STORAGE_ERROR` | RocksDB write or read error |
| `4`  | `ITERATOR_ERROR` | Failed to create RocksDB iterator |
| `5`  | `SERIALIZATION_ERROR` | JSON parse/serialize failure |

For constraint errors, see `ConstraintErrorCode` in `include/metadata/schema_constraints.h`.  
For version errors, see `VersionErrorCode` in `include/metadata/schema_version_manager.h`.

---

## See Also

- [`docs/metadata/operations_guide.md`](./operations_guide.md)
- [`docs/metadata/schema_migration_runbook.md`](./schema_migration_runbook.md)
- [`docs/metadata/recovery_runbook.md`](./recovery_runbook.md)
