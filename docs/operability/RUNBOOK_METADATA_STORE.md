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

# Metadata Store Operations: Evidence and Limits

## Status

This page is **not an actionable production recovery runbook**. The previously
published `themis-ctl` commands, `[METADATA:…]` log patterns, and
`metadata_*` exporter/partition metrics below were not supported by the
metadata/server implementation inspected for this update. Do not execute or
monitor for them as if they were shipped features.

Specifically, no implementation was found for `themis-ctl metadata
index-verify/index-rebuild`, `schema-history/schema-apply/schema-verify`,
partition verification/restore, exporter queue operations, the listed synthetic
log tags, or the listed `metadata_*` metrics. The consistency checker is
diagnostic; it does not repair keys, partitions, or catalog state.

## Verified HTTP surfaces

The routes below are registered in
[`http_server.cpp`](../../src/server/http_server.cpp) and handled by
[`schema_api_handler.cpp`](../../src/server/schema_api_handler.cpp). Availability
still depends on server initialization and deployment configuration.

| Method and path | Implemented behavior |
|---|---|
| `GET /api/v1/schema` | Returns the current schema; may return 503 when the schema manager is unavailable. |
| `GET /api/v1/schema/tables` and `/api/v1/schema/tables/{table}` | Lists tables or returns a table schema. |
| `GET /api/v1/information_schema[/...]` | Returns information-schema views. |
| `GET /api/v1/metadata/stats/{table}` | Reads persisted statistics; not-found is returned when no stats are available. |
| `POST /api/v1/metadata/stats/{table}` | Calls the statistics collector and returns its result; this is not a generic repair command. |
| `GET /api/v1/metadata/constraints/{table}` | Reads constraints; no HTTP route to create or restore constraints is registered. |
| `GET /api/v1/schema/versions/{table}` / `POST /api/v1/schema/versions/{table}` | Reads version history / records the current schema as a new version. |
| `GET /api/v1/schema/diff/{table}?from=V&to=V` | Compares two recorded versions. |
| `GET /api/v1/metadata/audit[/{table}]` | Reads schema audit history when the audit handler is initialized. |
| `GET /api/v1/metadata/lineage/{table}[/{column}]` | Reads lineage from the in-memory tracker. |
| `POST /api/v1/metadata/lineage` | Records a lineage derivation in the in-memory tracker. |

These are API routes, not evidence of `themis-ctl`, deployment metrics,
automatic repair, partition recovery, or durable lineage storage.

## Supported diagnostic and recovery boundaries

- For source-backed developer triage, statistics refresh, consistency-check
  limits, version rollback, and escalation guidance, use the
  [metadata developer runbook](../../src/metadata/RUNBOOK.md).
- `SchemaConsistencyChecker::runCheck()` reports findings; it does not perform
  repairs. Its scan is capped at 5,000 keys and optional statistics/constraint
  checks only run when those collaborators are supplied.
- Statistics can be recollected through `POST /api/v1/metadata/stats/{table}`
  or `StatisticsCollector::collectStats()` / `updateStats()`. Verify the
  returned result and reread the stats. `clearStats()` removes cached and
  persisted values; it is not a refresh operation.
- Constraints can be re-created only through the C++ `SchemaConstraints`
  API (`addConstraint()` and `persistTo()`); the HTTP route listed above is
  read-only.
- Schema rollback is available through
  `SchemaVersionManager::rollbackToVersion()` in C++; there is no HTTP rollback
  route. Verify both the live schema and resulting version history.
- No source-backed full namespace rebuild, partition restore, or exporter
  queue flush procedure is available here. Stop and escalate rather than
  inventing commands or deleting RocksDB keys.

## Evidence required before operational promotion

Promote this page to an actionable operator runbook only after each procedure
has an implemented command/API, tested failure behavior, an owner, and
deployment-specific prerequisites. Do not use the unsupported claims from the
superseded version as evidence.
