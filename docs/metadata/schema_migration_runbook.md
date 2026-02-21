# Schema Migration Runbook

**Module:** `src/metadata` — `SchemaVersionManager`  
**Maintained by:** ThemisDB Contributors  
**Last Updated:** February 2026

---

## Overview

This runbook describes how to safely perform schema changes on a live ThemisDB instance using the `SchemaVersionManager` API.  All schema mutations are recorded as immutable version snapshots, enabling safe rollback if a migration causes problems.

---

## Prerequisites

- Running ThemisDB instance (v1.8.0+)
- Admin HTTP access (`/api/v1/schema/*` endpoints)
- `curl` or equivalent HTTP client
- Operator understands the target schema change

---

## 1. Take a Baseline Snapshot

Always snapshot the current schema before making any change.

```bash
# Snapshot table "users" at the current state
curl -s -X POST http://localhost:8080/api/v1/schema/versions/users \
  -H 'Content-Type: application/json' \
  -d '{"author": "ops-engineer", "description": "pre-migration baseline"}' | jq .
```

Expected response:
```json
{
  "status": "success",
  "table_name": "users",
  "version": 1
}
```

Record the returned `version` number — you will need it if you need to roll back.

---

## 2. Review the Current Schema

```bash
curl -s http://localhost:8080/api/v1/schema/tables/users | jq .
```

Or via INFORMATION_SCHEMA:

```bash
curl -s 'http://localhost:8080/api/v1/information_schema/columns/users' | jq .
```

---

## 3. Apply the Schema Change

Use the standard PUT or PATCH endpoint to apply the change.

### Full replacement (PUT)

```bash
curl -s -X PUT http://localhost:8080/api/v1/schema/users \
  -H 'Content-Type: application/json' \
  -d '{
    "name": "users",
    "type": "relational",
    "properties": [
      {"name": "id",    "type": "integer", "indexed": true},
      {"name": "email", "type": "string",  "indexed": true},
      {"name": "role",  "type": "string",  "nullable": true}
    ]
  }' | jq .
```

### Partial update (PATCH)

```bash
curl -s -X PATCH http://localhost:8080/api/v1/schema/users \
  -H 'Content-Type: application/json' \
  -d '{
    "properties": [
      {"name": "phone", "type": "string", "nullable": true}
    ]
  }' | jq .
```

---

## 4. Snapshot the Post-Migration Schema

```bash
curl -s -X POST http://localhost:8080/api/v1/schema/versions/users \
  -H 'Content-Type: application/json' \
  -d '{"author": "ops-engineer", "description": "added phone column"}' | jq .
```

---

## 5. Validate the Change

### Check the schema looks correct

```bash
curl -s http://localhost:8080/api/v1/schema/tables/users | jq .tables[0].properties
```

### Check the diff between versions

```bash
curl -s 'http://localhost:8080/api/v1/schema/diff/users?from=1&to=2' | jq .
```

Expected diff format:
```json
{
  "status": "success",
  "diff": {
    "table_name": "users",
    "version_a": 1,
    "version_b": 2,
    "added":    [{"name": "phone", "type": "string", ...}],
    "removed":  [],
    "modified": []
  }
}
```

### Run integration tests (if available)

```bash
# Adjust path to your test runner
cd tests && ./run_schema_tests.sh users
```

---

## 6. Rollback Procedure

If the migration causes problems, roll back to the baseline version:

```bash
BASELINE_VERSION=1
TABLE_NAME=users

curl -s -X PATCH "http://localhost:8080/api/v1/schema/versions/${TABLE_NAME}" \
  -H 'Content-Type: application/json' \
  -d "{\"rollback_to\": ${BASELINE_VERSION}, \"author\": \"ops-engineer\"}"
```

> **Note:** The rollback endpoint is not yet exposed via HTTP; use the C++ API directly via `SchemaVersionManager::rollbackToVersion()` until the endpoint is wired.

Alternatively call the API in code:

```cpp
#include "metadata/schema_version_manager.h"

SchemaVersionManager svm(db, schema_mgr);
auto result = svm.rollbackToVersion("users", /*version=*/1, "ops-engineer");
if (!result.ok) {
    spdlog::error("Rollback failed: {}", result.error_message);
}
```

---

## 7. Review Full Version History

```bash
curl -s http://localhost:8080/api/v1/schema/versions/users | jq .history
```

---

## 8. Dry-Run Validation (planned)

> **TODO (v1.8.0):** A dry-run flag will be added to the migration endpoint.  Until then, test schema changes in a staging environment before applying to production.

---

## Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `TABLE_NOT_FOUND` | Table not registered with SchemaManager | Register schema with PUT first |
| `VERSION_NOT_FOUND` | Requested rollback version doesn't exist | Check history with `GET /api/v1/schema/versions/:table` |
| `STORAGE_ERROR` | RocksDB write failed | Check disk space and RocksDB health |
| `INVALID_VERSION` | Version 0 requested | Versions start at 1 |

---

## See Also

- [`include/metadata/schema_version_manager.h`](../../include/metadata/schema_version_manager.h)
- [`docs/metadata/metadata_roadmap.md`](./metadata_roadmap.md)
- [`docs/metadata/operations_guide.md`](./operations_guide.md) *(planned)*
