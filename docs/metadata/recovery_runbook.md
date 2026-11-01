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

> Exact model backend not verifiable from final document text alone.

# Metadata Recovery: Implemented Operations and Limits

## Scope

This page documents recovery operations visible in the current implementation.
It is not a procedure for manually editing RocksDB keys. Back up the database
before any operator-approved change, and preserve returned results and logs.

The [metadata developer runbook](../../src/metadata/RUNBOOK.md) is the
source-backed triage reference. The
[metadata store operations page](../operability/RUNBOOK_METADATA_STORE.md)
records former `themis-ctl` commands and synthetic metrics as unsupported.

## Statistics

`GET /api/v1/metadata/stats/{table}` reads collected statistics and can return
not-found when no stored result exists. `POST /api/v1/metadata/stats/{table}`
calls `StatisticsCollector::collectStats()` and returns the collection result.
Verify that result and read the statistics again. There is no automatic
repair/rebuild API. `StatisticsCollector::clearStats()` deletes cached and
persisted values and should not be used as a refresh operation.

## Constraints

`GET /api/v1/metadata/constraints/{table}` is read-only. There is no HTTP
constraint restoration route. In-process callers can recreate definitions
using `SchemaConstraints::addConstraint()` and `persistTo()`, after validating
the authoritative application configuration. Confirm the persisted result
through `getTableConstraints()` or the read-only HTTP route.

## Schema versions

The HTTP API supports:

- `GET /api/v1/schema/versions/{table}` to read history.
- `POST /api/v1/schema/versions/{table}` to record a new snapshot of the
  current schema.
- `GET /api/v1/schema/diff/{table}?from=V&to=V` to compare recorded versions.

There is no HTTP rollback route. In-process callers may use
`SchemaVersionManager::rollbackToVersion()` for an existing version. Serialize
calls because the manager is not thread-safe. After a successful rollback,
verify the live schema, current version, and history; the rollback can report
success even if recording its new history entry failed.

## Consistency findings

`SchemaConsistencyChecker::runCheck()` reports findings; it does not repair
records, statistics, or constraints. The key scan is limited to 5,000 keys and
optional statistics/constraint checks are only performed when those
collaborators are passed. A clean result is therefore not proof of a complete
store scan or automatic recovery.

## Unsupported operations

No source-backed procedure is available here for raw `ldb` edits, a full
metadata namespace rebuild, orphan audit-entry deletion, partition restore,
or exporter-queue replay/flush. Do not execute undocumented commands or delete
keys based on a log message. Escalate with sanitized logs, exact typed API
results, table/version identifiers, and the focused metadata test result.

For schema migration test coverage, run:

```bash
ctest --test-dir build --output-on-failure -R SchemaVersionManagerFocusedTests
```
