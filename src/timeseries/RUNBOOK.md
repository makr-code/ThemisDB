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

# Timeseries Incident Runbook

This runbook gives source-backed triage steps for maintainers and operators
with access to the application that owns the timeseries objects. It describes
module APIs, not a standalone `themis-admin` command-line interface. Use the
deployment's authenticated administrative surface to invoke these operations.

Exact model backend not verifiable from final document text alone.

## First response

1. Record the affected metric/entity, UTC time range, caller, recent config or
   key changes, and whether writes or reads are affected.
2. Preserve application logs, server traces, `TimeSeriesMetrics`, and
   `TsOperatorDiagnostics` output before restarting a worker or clearing
   in-memory diagnostic state.
3. Avoid destructive retention, key deletion, or chunk replacement until
   storage backup and recovery options have been confirmed.
4. Classify the returned error using the caller's actual API result. The
   canonical error helper functions are documented in
   `include/timeseries/timeseries_api_contract.h`; not all module interfaces
   return those codes.

## Buffered writes stalled or delayed

**Signals:** `TSAutoBufferStats` shows buffered points/memory growing, flush
counts not advancing, or overdue flush/backpressure counters increasing.

1. Read `TSAutoBuffer::getStats()`, `isRunning()`, and the active
   `TSAutoBufferConfig`. Check the configured size/time thresholds and any
   attached `TimeSeriesMetrics`.
2. Check the underlying `TSStore` result path and storage health for write
   failures. `TimeSeriesMetrics::recordOverdueFlush()` is intended for data
   that remains buffered after prior store errors.
3. If the owner has confirmed the store is healthy and a manual drain is safe,
   call `flush()` or `flushFor(metric, entity)` and record the returned count.
   `stop()` also joins the background thread and flushes remaining points; it
   is a lifecycle operation, not a safe substitute for diagnosing storage
   failures.
4. If points remain buffered or flush errors recur, preserve the buffer stats
   and escalate to the timeseries/storage owner. Do not destroy `TSStore`
   while its buffer is active.

## Retention is not progressing or data was deleted unexpectedly

1. Inspect `RetentionManager::getPolicy()`, `isAsyncRunning()`, and
   `getStats()`. The background loop only runs after `startAsync()` and
   `apply()` iterates the configured per-metric policies.
2. Review `getAuditLog()` and any configured audit callback. Entries are
   in-memory and bounded; export them to the deployment's durable audit
   system if retention evidence must survive process restart.
3. Before any manual `apply()`, validate the metric durations and the
   calculated cutoff. `apply()` deletes expired data; the module does not
   expose an undelete operation. For unexpected deletion, stop further
   cleanup with `stopAsync()` and recover from the deployment's backup or
   replica process.
4. Escalate storage deletion failures with the metric, policy, audit entries,
   and storage error. Do not repeatedly force cleanup against an unhealthy
   backend.

## Gorilla decode or encrypted chunk failure

1. Identify the affected metric/entity and chunk time bounds from the storage
   record/log context; retain the original chunk and its metadata for
   investigation.
2. Check decoder error state and decoded count. A partial decode is not a
   complete successful read.
3. For encrypted chunks, verify that the stored key identifier can be resolved
   by the configured historical-key callback. Keep old keys available until
   all stored chunks referencing them have been migrated or expired.
4. Do not disable authentication checks, replace the original ciphertext, or
   rotate keys again as an unverified repair. Validate recovery on a copied
   store/backup, then involve the storage and key-management owners.

## Prometheus remote-write parse or ingest errors

1. Confirm the request uses the expected Snappy-compressed protobuf input when
   using `PromWriteRequest::decodeSnappy()`. The lower-level `decode()` accepts
   raw protobuf bytes.
2. For malformed or truncated input, capture a request identifier and parser
   error without logging sensitive payload content. Such a format failure is
   not repaired by retrying the same bytes.
3. For endpoint/network failures, inspect the server-side transport and
   downstream availability. The module parser does not own HTTP authentication,
   listener health, or a retry queue.
4. Escalate sustained failures to the server/integration owner with sanitized
   protocol metadata and incident diagnostics.

## Structured incident handling

`TsOperatorDiagnostics` stores a bounded ring of 512 incidents, each with
severity, timestamp, optional timeseries error code, and remediation text.
Use its query/summary/export methods where wired by the application. Preserve
the exported evidence externally; the registry is not a durable incident
database.

The linked program-level incident packet contains `themis-admin` commands and
metric names not declared by the timeseries module sources inspected for this
guide. Verify those commands and signals against the deployed server before
executing them; this module exposes the C++ APIs above, not that CLI contract.

## References

- [Operations lifecycle](OPERATIONS.md)
- [Operator guide](OPERATOR_GUIDE.md)
- [Production requirements](PRODUCTION_REQUIREMENTS.md)
- [Metrics interface](../../include/timeseries/timeseries_metrics.h)
- [Diagnostics interface](../../include/timeseries/ts_operator_diagnostics.h)
- [Buffer interface](../../include/timeseries/ts_auto_buffer.h)
- [Retention interface](../../include/timeseries/retention.h)
- [Encrypted chunk interface](../../include/timeseries/encrypted_chunk_store.h)
- [Existing incident runbook](../../docs/operability/RUNBOOK_TIMESERIES_STORE.md)
