# Recovery Runbook: Metadata Module

**Module:** `src/metadata`  
**Version:** 2026 Q1  
**Severity:** P1 / Production Use

---

## Overview

This runbook covers recovery procedures for the following scenarios:

1. **Corrupted statistics** – `stats:<table>` key is malformed or unreadable
2. **Lost schema constraints** – `config:constraints:<table>` key is missing after restart
3. **Corrupted/missing schema versions** – `config:schema_version:*` keys are missing or inconsistent
4. **Orphaned audit entries** – `audit:schema:*` keys reference unknown tables
5. **Inconsistent schema** – live schema in SchemaManager diverges from versioned snapshots
6. **Full metadata namespace rebuild** – last resort when multiple key groups are corrupt

---

## Prerequisites

- RocksDB `ldb` tool available (included in RocksDB build)
- ThemisDB admin HTTP access
- Backup copy of the RocksDB data directory (if available)
- Access to application logs (spdlog output)

---

## 1. Corrupted Statistics

### Detect

```bash
curl -s http://localhost:8080/api/v1/metadata/stats/users | jq .
# Expected: {"status": "success", "stats": {...}}
# Problem:  {"status": "error", "message": "..."}
```

Or check the key directly:

```bash
ldb --db=/path/to/db get "stats:users"
```

### Recover

Statistics are **re-derivable** from data — no backup needed.

```bash
# Force a fresh collection
curl -s -X POST http://localhost:8080/api/v1/metadata/stats/users | jq .
```

Or via C++ API:

```cpp
stats_collector.clearStats("users");
stats_collector.collectStats("users");
```

The old `stats:users` key is overwritten automatically.

### Verify

```bash
curl -s http://localhost:8080/api/v1/metadata/stats/users | jq .stats.row_count
```

---

## 2. Lost Schema Constraints

### Detect

```bash
curl -s http://localhost:8080/api/v1/metadata/constraints/users | jq .
# If constraints array is empty but you expected NOT NULL / UNIQUE rules, they were lost.
```

Inspect raw key:

```bash
ldb --db=/path/to/db get "config:constraints:users"
```

### Recover

Constraints must be **re-registered manually** (no automatic recovery from data).  
Use your application's constraint configuration as the authoritative source.

```bash
# Re-register via the constraints API (planned endpoint) or C++ API
```

```cpp
SchemaConstraints sc;
sc.addConstraint("users", "email",
    ColumnConstraint::makeNotNull("nn_users_email"));
sc.addConstraint("users", "username",
    ColumnConstraint::makeUnique("uq_users_username"));
sc.persistTo(db);
```

Or use the batch constraint validation endpoint to verify new constraints work before going live.

### Verify

```bash
curl -s http://localhost:8080/api/v1/metadata/constraints/users | jq .constraints
```

---

## 3. Corrupted / Missing Schema Versions

### Detect

```bash
curl -s http://localhost:8080/api/v1/schema/versions/users | jq .history
# Expected: non-empty array
# Problem:  empty array or error
```

List all version keys:

```bash
ldb --db=/path/to/db scan --from="config:schema_version:users:" --to="config:schema_version:users:~"
```

### Recover – Partial History

If only some version keys are missing (e.g., intermediate version 3 of 5):

1. Note the highest intact version
2. Roll back the live schema to that version manually if needed
3. Re-snapshot the current schema as a new version:

```bash
curl -X POST http://localhost:8080/api/v1/schema/versions/users \
  -d '{"author":"recovery", "description":"recovery snapshot after version corruption"}'
```

### Recover – No History Exists

If no version keys exist but the live schema is intact:

```bash
# Snapshot the current live schema as v1
curl -X POST http://localhost:8080/api/v1/schema/versions/users \
  -d '{"author":"recovery", "description":"initial recovery snapshot"}'
```

### Recover – Live Schema Also Missing

If the `SchemaManager` has lost the table (e.g., `config:schema:users` key is gone):

1. Re-register the schema from your application source of truth:

```bash
curl -X PUT http://localhost:8080/api/v1/schema/users \
  -H 'Content-Type: application/json' \
  -d '{
    "name": "users",
    "type": "relational",
    "properties": [
      {"name": "id",    "type": "integer"},
      {"name": "email", "type": "string"}
    ]
  }'
```

2. Snapshot as v1:

```bash
curl -X POST http://localhost:8080/api/v1/schema/versions/users \
  -d '{"author":"recovery", "description":"rebuilt after data loss"}'
```

### Verify

```bash
curl -s http://localhost:8080/api/v1/schema/versions/users | jq '.history | length'
```

---

## 4. Orphaned Audit Entries

Audit entries reference a `table_name` that may no longer exist.  This is safe to leave in place — the audit log is append-only and historical.

If you need to clean up old audit entries:

```bash
# List orphaned audit keys (manual ldb scan)
ldb --db=/path/to/db scan --from="audit:schema:" --to="audit:schema:~"

# Delete specific key (use with caution)
ldb --db=/path/to/db delete "audit:schema:dropped_table:0000000000012345678"
```

> **Warning:** Do not delete audit entries for tables that still exist.

---

## 5. Inconsistent Live Schema vs. Versioned Snapshot

### Detect

Compare the live schema with the latest version snapshot:

```bash
# Get live schema
curl -s http://localhost:8080/api/v1/schema/tables/users | jq .

# Get latest version snapshot
LATEST=$(curl -s http://localhost:8080/api/v1/schema/versions/users | jq '.history | last')
echo $LATEST | jq .snapshot
```

If they differ, the live schema has been mutated without a corresponding version snapshot.

### Recover

Simply snapshot the current live schema:

```bash
curl -X POST http://localhost:8080/api/v1/schema/versions/users \
  -d '{"author":"recovery", "description":"re-sync snapshot after untracked mutation"}'
```

---

## 6. Full Metadata Namespace Rebuild

**Last resort** — use only when multiple key groups are corrupt AND a backup is not available.

### Step 1: Export what still works

```bash
# Export all intact schemas
curl -s http://localhost:8080/api/v1/schema | jq . > /tmp/schema_backup.json

# Export INFORMATION_SCHEMA
curl -s http://localhost:8080/api/v1/information_schema | jq . > /tmp/is_backup.json
```

### Step 2: Delete corrupt metadata keys (use ldb)

```bash
# Wipe statistics
for key in $(ldb --db=/path/to/db scan --from="stats:" --to="stats:~" | awk '{print $1}'); do
  ldb --db=/path/to/db delete "$key"
done

# Wipe version history
for key in $(ldb --db=/path/to/db scan --from="config:schema_version:" --to="config:schema_version:~" | awk '{print $1}'); do
  ldb --db=/path/to/db delete "$key"
done
```

### Step 3: Re-import schemas

```bash
curl -X PUT http://localhost:8080/api/v1/metadata/schema_import \
  -H 'Content-Type: application/json' \
  -d @/tmp/schema_backup.json
```

### Step 4: Re-collect statistics

```bash
# For each table:
curl -X POST http://localhost:8080/api/v1/metadata/stats/users
curl -X POST http://localhost:8080/api/v1/metadata/stats/products
# … repeat for all tables
```

### Step 5: Re-register constraints (manual)

See **Section 2** above.

### Step 6: Create initial version snapshots

```bash
for TABLE in users products orders; do
  curl -X POST http://localhost:8080/api/v1/schema/versions/$TABLE \
    -d "{\"author\":\"recovery\", \"description\":\"post-rebuild snapshot\"}"
done
```

### Step 7: Verify

```bash
curl -s http://localhost:8080/api/v1/information_schema | jq '.tables | length'
curl -s http://localhost:8080/api/v1/metadata/audit    | jq '.audit | length'
```

---

## Quick Reference: Key Namespace

```
stats:<table>                            – statistics blob
config:constraints:<table>               – constraint definitions
config:schema_version:<table>:<N>        – version snapshot (N = zero-padded)
config:schema_version:<table>:current    – current version counter
audit:schema:<table>:<ns>                – audit entry
```

Scan any namespace:

```bash
ldb --db=/path/to/db scan --from="<prefix>" --to="<prefix>~"
```

---

## See Also

- [`docs/metadata/operations_guide.md`](./operations_guide.md)
- [`docs/metadata/troubleshooting.md`](./troubleshooting.md)
- [`docs/metadata/schema_migration_runbook.md`](./schema_migration_runbook.md)
