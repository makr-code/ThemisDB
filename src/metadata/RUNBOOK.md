# Metadata Developer Runbook

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** module-behavior
**Canonical source references:** [consistency checker API](../../include/metadata/schema_consistency_checker.h), [statistics API](../../include/metadata/statistics_collector.h), [schema version API](../../include/metadata/schema_version_manager.h), [consistency implementation](./schema_consistency_checker.cpp), [focused test registration](../../tests/metadata/CMakeLists.txt)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Scope

This page is a source-backed developer triage procedure for metadata code and
tests. It complements, but does not replace, the deployment-oriented
[`RUNBOOK_METADATA_STORE.md`](../../docs/operability/RUNBOOK_METADATA_STORE.md).
Command examples from that deployment runbook are not repeated here because
their implementation is not established by the metadata module sources; verify
them with their owning executable before use. The module roadmap also conflicts
about whether that runbook is complete; see
[`RESEARCH.md`](./RESEARCH.md).

## Verified schema HTTP routes

The route dispatch is in
[`http_server.cpp`](../../src/server/http_server.cpp), and handler behavior is
in [`schema_api_handler.cpp`](../../src/server/schema_api_handler.cpp), with
the declarations in [`schema_api_handler.h`](../../include/server/schema_api_handler.h).
These are source routes, not a claim that every deployment exposes them:

| Method and path | Handler behavior / useful triage |
|---|---|
| `GET /api/v1/schema` | Full schema via `SchemaManager::toJSON()`; returns 503 when the manager is unavailable and 500 on caught handler exceptions. |
| `GET /api/v1/schema/tables` | Lists discovered table summaries; same unavailable-manager and exception outcomes. |
| `GET /api/v1/schema/tables/{name}` | Reads a table schema; malformed/missing path input is rejected by the handler. |
| `PUT /api/v1/schema/{table}` | Parses and validates a schema body before storing it; malformed JSON or validation failures return 400. |
| `PATCH /api/v1/schema/{table}` | Applies a schema patch; malformed JSON is rejected with 400. |
| `GET /api/v1/schema/versions/{table}` / `POST /api/v1/schema/versions/{table}` | Reads history / creates a version through the configured version manager. |
| `GET /api/v1/schema/diff/{table}?from=V&to=V` | Compares versions; missing or invalid version query values return 400. |
| `GET /api/v1/information_schema[/...]` | Routed to the information-schema handler. |

Use the handler's actual response and server logs to diagnose a route failure;
do not infer support for additional verbs, repair actions, authentication
behavior, or deployment-specific host/port settings from this route list.

## Triage procedure

1. **Name the failing surface.** Identify schema discovery/cache,
   statistics, consistency, versioning, lineage, export, or distributed
   catalog. Record table name, operation, and whether the failure reproduces
   with one caller or concurrent callers.
2. **Capture the API result.** Preserve `StatsResult`, `VersionResult`,
   `PublishResult`, boolean, optional, or exception details as returned. Do not
   turn empty results into proof of healthy state.
3. **Run an in-process consistency check when available.** Construct
   `SchemaConsistencyChecker` with the relevant `RocksDBWrapper`,
   `SchemaManager`, and optional statistics/constraints collaborators; call
   `runCheck()` and inspect `ConsistencyIssue::issue_type`, table, and detail.
   The checker reports orphan key prefixes, missing/stale statistics when a
   collector is supplied, and missing constraints when a constraint object is
   supplied.
4. **Inspect existing history.** For schema changes, use
   `SchemaVersionManager::getChangeHistory()` / `getVersion()` and
   `SchemaAuditLog::getHistory()`. These are separate records; one does not
   replace the other.
5. **Reproduce with the narrow test.** Select the corresponding target/test
   from [`tests/metadata/CMakeLists.txt`](../../tests/metadata/CMakeLists.txt).
   For external catalog failures, use the injected HTTP callback in
   [`test_catalog_exporter.cpp`](../../tests/metadata/test_catalog_exporter.cpp)
   and exercise success and non-success responses.
6. **Check concurrency assumptions.** Review the class header for thread-safety.
   `SchemaVersionManager` and `SchemaConstraints` require external
   synchronization; do not race `SchemaConsistencyChecker` start/stop calls.
7. **Escalate with evidence.** Attach minimal sanitized logs, typed API outcome,
   test name/result, affected source path, and whether the outcome was reproduced.
   Never attach credentials, raw sensitive metadata, or unredacted payloads.

## Consistency-check limitations

- The orphan-key scan is capped at 5,000 scanned keys in
  [`schema_consistency_checker.cpp`](./schema_consistency_checker.cpp); a clean
  result is not proof that larger stores were fully scanned.
- The check skips configured system key prefixes and identifies table prefixes
  from keys of the form `table_name:row_id`.
- Stale-statistics and missing-constraint checks are conditional on passing
  those collaborators.
- The checker reports findings; it does not repair orphan records, recollect
  statistics, or add constraints automatically.

## Source-backed recovery choices

### Stale or missing statistics

1. Inspect `StatisticsCollector::getStats(table)` and preserve its typed result.
2. For a fresh persisted collection, call `updateStats(table)`. It delegates to
   `collectStats()` and uses the default sample size when none is specified.
   If a specific sample size is appropriate, call
   `collectStats(table, sample_size)` directly.
3. Check the returned result, then read `getStats(table)` again and verify its
   `last_updated`, `sample_size`, and relevant estimates.
4. Do not use `clearStats()` as a refresh operation: it removes both cached and
   persisted table statistics. Use it only when intentionally resetting that
   state and after considering downstream query-planning impact.

The public behavior is declared in
[`statistics_collector.h`](../../include/metadata/statistics_collector.h) and
covered by [`test_statistics_collector.cpp`](../../tests/metadata/test_statistics_collector.cpp).

### Schema rollback

1. Confirm a `SchemaVersionManager` is available and inspect
   `getChangeHistory(table)`, `getVersion(table, target)`, and, when useful,
   `diffVersions(table, current, target)` before changing the live schema.
2. Serialize access: `SchemaVersionManager` is not thread-safe.
3. Call `rollbackToVersion(table, target, author)` only for an existing version;
   handle a failed `VersionResult` without assuming the schema changed.
4. After success, verify the live schema with `SchemaManager::getTable()` and
   inspect `getCurrentVersion()` / `getChangeHistory()`. The implementation
   applies the saved snapshot and attempts to record the rollback as a new
   version. If that new record fails, it logs a warning while the rollback
   result can still report success, so verify history explicitly.
5. A rollback audit entry is emitted only when an audit log has been attached.
   If live schema and version history disagree, stop further changes and
   escalate with the sanitized result and history evidence.

See [`schema_version_manager.h`](../../include/metadata/schema_version_manager.h),
[`schema_version_manager.cpp`](./schema_version_manager.cpp), and
[`test_schema_version_manager.cpp`](../../tests/metadata/test_schema_version_manager.cpp).

### Repair and escalation boundary

The consistency checker is diagnostic, not a repair engine. It does not expose
automatic actions to delete orphan keys, create constraints, or repair catalog
state. Do not invent or run a repair command based on a finding. Escalate when
the finding persists after the relevant supported API operation, when a
rollback cannot be verified, or when storage/API results indicate a failure
outside these recovery choices. Preserve only sanitized logs, typed results,
history, and test evidence.

For ongoing refresh and lifecycle details, see [OPERATIONS.md](./OPERATIONS.md).
