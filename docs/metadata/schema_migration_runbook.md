---
Author: "ThemisDB Team"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Schema Migration Runbook

**Module:** `src/metadata` — `SchemaVersionManager`  
**Maintained by:** ThemisDB Contributors

> This source-backed revision replaces an older runbook; its original creation
> date was not recorded. Exact model backend not verifiable from final document text alone.

---

## Overview

This runbook describes the implemented schema HTTP routes and
`SchemaVersionManager` API. Snapshots are recorded only when the version API is
called; `PUT` and `PATCH` schema requests do not automatically create version
records. The API supports rollback through the C++ manager, but not through an
HTTP route.

---

## Prerequisites

- Running ThemisDB instance with storage initialized and schema routes enabled
- `curl` or equivalent HTTP client
- Operator understands the target schema change
- If a metadata security provider is configured, an authenticated identity with
  the required metadata operation/resource grant

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

The response contains the version manager's diff under `diff`; inspect the
returned JSON rather than assuming a fixed field layout.

### Run integration tests (if available)

Run the registered metadata test target from the configured build:

```bash
ctest --test-dir build --output-on-failure -R SchemaVersionManagerFocusedTests
```

---

## 6. Rollback Procedure

If the migration causes problems, roll back to the baseline version:

> **HTTP limitation:** `POST /api/v1/schema/versions/{table}` only creates a
> version snapshot. No HTTP rollback route is registered; do not send a
> `PATCH` request to that path.

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

## 8. Generate Migration Script

`SchemaVersionManager::generateMigrationScript()` converts the diff between two versions into executable DDL statements.

```cpp
#include "metadata/schema_version_manager.h"

SchemaVersionManager svm(db, schema_mgr);

// Generate the script that upgrades "users" from version 1 to version 3
auto result = svm.generateMigrationScript("users", /*from=*/1, /*to=*/3);
if (!result.ok) {
    spdlog::error("Script generation failed: {}", result.error_message);
} else {
    // Review, then apply to the target database
    std::cout << result.value;
}
```

Example output:
```sql
-- Migration: users from version 1 to version 3
ALTER TABLE users ADD COLUMN phone VARCHAR;
ALTER TABLE users DROP COLUMN legacy_id;
ALTER TABLE users ALTER COLUMN amount TYPE DOUBLE PRECISION;
ALTER TABLE users ALTER COLUMN email SET NOT NULL;
```

The method returns generated SQL text; it does not execute the script against
ThemisDB. Review and apply it through the separately supported migration path.

---

## 9. Dry-Run Validation

Use `validateMigration()` to check a proposed schema against the current version without persisting anything:

```cpp
SchemaVersionManager svm(db, schema_mgr);

SchemaManager::TableSchema new_schema = /* ... */;
auto dry_run = svm.validateMigration("users", new_schema);
if (!dry_run.ok) {
    spdlog::error("Validation failed: {}", dry_run.error_message);
    // e.g. "Migration validation failed: duplicate column 'id'"
}
```

Checks performed:
- Schema name is non-empty
- At least one column is defined
- No duplicate column names
- New schema differs from the currently versioned schema

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
- [`docs/metadata/operations_guide.md`](./operations_guide.md)
