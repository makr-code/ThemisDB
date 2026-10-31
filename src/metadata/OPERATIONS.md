# Metadata Module Operations

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
**Canonical source references:** [statistics lifecycle](../../include/metadata/statistics_collector.h), [schema cache API](../../include/metadata/schema_manager.h), [module roadmap](./ROADMAP.md)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Runtime lifecycle

### Schema discovery and cache

- `SchemaManager` uses discovered RocksDB key prefixes and sampled entity
  properties to populate schema views.
- Call `refreshCache()` after structural changes when the caller requires an
  immediate rescan; otherwise cache expiration follows the configured fixed TTL
  or adaptive TTL mode.
- Adaptive TTL is driven by mutation records and its configured window,
  sensitivity, and min/max limits. Read the active configuration rather than
  assuming the defaults fit a deployment.
- The schema manager's `Changefeed*` is non-owning. Keep it alive while
  registered and clear it before destruction if lifetimes differ.

### Statistics

- `StatisticsCollector::collectStats()` samples stored entities (default
  sample size 1,000 when passed 0), updates its in-memory cache, and persists
  statistics in RocksDB.
- `getStats()` may load persisted data when not cached; it is not a forced
  rescan. Use `updateStats()` for a fresh collection.
- `setRefreshInterval()` starts or updates background refresh for tables
  already sampled; interval zero stops the worker. Destruction also stops it.
- Attach an `IMetricsHook` only when its non-owning target outlives the
  collector. Hook events distinguish collection result, cache hits/misses, and
  internal error codes.

### Consistency checks

- `SchemaConsistencyChecker::runCheck()` performs a synchronous check and
  updates the last result snapshot.
- A positive `startBackgroundCheck()` interval starts periodic checks; zero
  disables them. `stopBackgroundCheck()` waits for the thread to exit and is
  called by destruction.
- Start/stop calls must be serialized by the owner thread; do not call either
  concurrently with the other.
- Review [RUNBOOK.md](./RUNBOOK.md) before treating a check result as a full
  repair or complete scan.

## Persistence and availability boundaries

- Statistics and schema audit/version records use the RocksDB wrapper as their
  documented persistence layer.
- `ColumnLineageTracker` is in-memory; callers needing durability must persist
  its JSON export themselves.
- `InMemoryMetadataSnapshotStore` is explicitly non-durable across process
  exit. Do not use it as a production backup store.
- `SchemaConstraints` stores constraints in memory; the persistence test/API
  path is separate from the base class behavior.
- `DistributedMetadataCatalog` depends on the injected shard router and returns
  explicit success/absence results; it does not make an unavailable router
  available.

## Operational guardrails

- Configure external catalog timeouts and credentials at the deployment
  boundary. Never commit secrets or place them in examples/log attachments.
- Treat exporter success/failure outcomes and consistency findings as
  observable states; do not assume retries, repair, or queueing unless an owning
  integration implements them.
- Verify the actual build/test targets and runtime integrations for the
  deployment. This module page does not define a CLI, default alert rules, or
  recovery commands.

For current roadmap status, see [`ROADMAP.md`](./ROADMAP.md); for deployment
triage guidance, see [`docs/operability/RUNBOOK_METADATA_STORE.md`](../../docs/operability/RUNBOOK_METADATA_STORE.md).
