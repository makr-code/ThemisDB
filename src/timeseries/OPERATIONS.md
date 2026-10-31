---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

# Timeseries Operational Lifecycle

This page is for deployment engineers and on-call maintainers. It describes
the lifecycle obligations exposed by module interfaces; deployment-specific
configuration names, service commands, and alert thresholds must come from the
actual server and platform configuration.

Exact model backend not verifiable from final document text alone.

## Startup and dependency ownership

- Construct `TSStore` with a live RocksDB `TransactionDB`; the database and
  optional column-family handle are non-owning dependencies.
- If buffering is enabled, attach the `TSAutoBuffer` to the store, configure
  its limits, then start it. The buffer points to `TSStore` without owning it.
- If retention is required, provide explicit per-metric durations and call
  `RetentionManager::startAsync()` with the chosen interval. A policy alone
  does not start the background worker.
- If encrypted chunks are enabled, configure current-key and historical-key
  callbacks before writes. Keep historical keys until no retained chunk
  requires them. Supply the intended audit logger and accessor identity.
- Register a `TimeSeriesMetrics` instance and diagnostics/incident callbacks
  explicitly where the application needs those signals.

The module-level production requirements call for configured retention,
explicit resource bounds, active authorization controls, and audit logging in
production. Verify that the deployment and server actually provide these
controls; module objects do not independently establish a secure deployment.

## Capacity and observability

Size buffer limits, flush intervals, batch sizes, retention windows, and
storage capacity for the workload. The public buffer configuration includes
point, memory, and flush thresholds. Its statistics expose buffered/flushed
points, flush counts, rejected or deduplicated points, backpressure, EWMA
latency, and current adaptive batch size.

`TimeSeriesMetrics` supports recording writes, batch writes, compression,
queries, aggregation, backpressure, overdue flushes, storage statistics,
retention, and errors. The class defines metric collection methods, but this
module page does not prescribe exported metric names or alert values; obtain
those from the configured exporter and production SLO. Avoid enabling
per-metric labels without evaluating cardinality cost.

`TsOperatorDiagnostics` has a fixed 512-entry in-memory incident ring. Export
incidents to durable logging/alerting if they are needed for audit or recovery.
Retention's audit log is also in-memory and bounded.

## Shutdown order

1. Stop or quiesce upstream producers.
2. Stop the auto-buffer and verify its remaining points were flushed or
   persisted for replay. If a WAL snapshot is restored after restart, call
   `flush()` or start the buffer to replay the points, then remove the snapshot
   only after successful completion.
3. Stop retention and key-rotation background workers and wait for them to
   join.
4. Only then destroy `TSStore`, its RocksDB database, and column-family
   handles.

Do not destroy a non-owned dependency while a module worker can still use it.

## Backup, restore, and rollback

The module's retention operation permanently deletes data and its key-rotation
worker replaces encrypted chunk values. The code does not define a
module-level undelete or rollback command. Use the deployment's tested
RocksDB backup/replication procedure and preserve required historical keys.
For recovery, first validate the backup and keys on an isolated copy; do not
point a live worker at a partially restored store.

When rolling back a binary that reads persisted timeseries data, verify
compatibility with the current individual-point key form, Gorilla chunk
framing, encryption envelope, and aggregate watermark keys. A binary rollback
does not undo retention deletions or key rotations.

## Escalation and change controls

- Flush/storage errors: timeseries and storage owners.
- Missing historical key, authentication failure, or key rotation issue:
  security/key-management and storage owners.
- Remote-write protocol, HTTP authorization, or endpoint outage: server and
  integration owners.
- Unexpected deletes or retention policy violations: data owner plus
  compliance/audit owner where applicable.

For symptom-based steps, use [RUNBOOK.md](RUNBOOK.md). Production constraints
are recorded in [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md).
