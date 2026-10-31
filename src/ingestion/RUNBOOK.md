---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Ingestion Runbook

## Purpose and safety

This runbook gives source-verifiable triage steps for an operator or service
owner using the in-process ingestion API. The module does not define a
deployment-wide CLI, alert thresholds, or an HTTP admin endpoint. Apply
deployment-specific authorization and change controls before invoking admin
methods. See [`OPERATIONS.md`](./OPERATIONS.md) for signals and
[`TRUST.md`](./TRUST.md) for data-handling constraints.

## First response: inspect before changing state

1. Query `IngestionAdminApi::healthJson()` and `listSources()` for module status,
   enabled/available sources, and last-run data.
2. Inspect the corresponding `IngestionStats` or `IngestionReport`: source ID,
   processed/failed counts, error codes/messages/details, retry counts,
   correlation ID, and the quarantine entries.
3. If lineage is enabled, query records by source or run correlation ID. Lineage
   is held in memory and can be empty after restart; do not use absence as proof
   that a run never occurred.
4. Use `sanitizeOptions()` before logging source options. Do not include
   credentials or raw payloads in an incident ticket.

## Symptom-to-action

| Symptom | Diagnose | Safe first action |
|---|---|---|
| Source unavailable or connector init failure | Check `SourceStatus::available`, source type/location, connector error details, and external endpoint/storage health. | Correct the source config with `reconfigureSource()` or pause that source while investigating. |
| HTTP 401 / 403 | Confirm credential source, token expiry/scope, and whether the connector supports configured OAuth refresh. | Rotate/fix credentials through the secret store; never paste tokens into logs. |
| Repeated timeout, 429, or 5xx | Check retry count, timeout, rate limit, and source-side availability. | Reduce source rate or pause it; resume only after the external service recovers. |
| Schema or parsing failures | Inspect the structured error and schema violation summary; determine whether the source changed format. | Keep invalid data rejected when correctness is uncertain; fix the upstream contract or schema before replay. |
| Quality or workflow failure | Inspect the active profile, failing step, backend availability, warnings, and quality scores. | Do not interpret an unevaluated fail-open score as a quality pass; hold the affected workflow through controlled configuration. |
| Quarantine growth | Inspect each entry's path, source, error code, retry count, and `permanently_failed` state. | Correct the cause first, then retry selected recoverable entries. Avoid bulk dismissal or clearing until records are retained elsewhere. |
| Checkpoint resume is wrong | Verify source ordering/offset semantics and the configured checkpoint directory. | Pause the source; preserve checkpoint data before changing/removing it; validate resume against a fixture before replay. |

## Recovery procedures

### Isolate a failing source

Call `IngestionAdminApi::pauseSource(source_id)` (or the manager equivalent).
This disables the source for future `ingestAll()` runs; it does not cancel an
already executing connector run. Check its boolean result, update configuration
only with `reconfigureSource()` for a registered source, then verify availability
and run a bounded test before `resumeSource()`.

### Retry quarantined items

Use `listQuarantine()` to select entries. `retryQuarantineItem(item_path)`
retries a recoverable entry with bounded exponential delay; failed entries can
be marked permanently failed once `max_quarantine_retries` is reached. Confirm
that the error cause is corrected before retrying. `dismissQuarantineItem()` is
destructive and removes the entry; it does not repair or persist the source
payload elsewhere.

### Resume incremental ingestion

Configure an existing checkpoint directory with `setCheckpointDir()` and enable
incremental mode. Preserve the checkpoint store when diagnosing a resume issue.
Do not manually advance offsets or delete checkpoints unless the source's
ordering semantics and replay consequence are understood. The default
in-process coordinator stores are not durable across process restart or shared
across hosts.

## Escalation and closure

Escalate when the source's integrity, authorization boundary, or checkpoint
position cannot be established; when a backend error is persistent; or when
replay could duplicate or omit records. Include the run correlation ID, source
ID/type, timestamps, structured error codes, counts, and sanitized options.
Exclude secrets and document content.

Close the incident only after a bounded verification run has expected processed
and failure counts, no unexplained quarantine growth, correct checkpoint
progress, and healthy downstream writes where the configured execution path
includes a sink. For the filesystem workflow manager path, verify persistence
separately because successful processing is not currently handed to a sink.
Preserve the original report and the recovery action in the deployment's
incident system.

## References

- [`include/ingestion/ingestion_manager.h`](../../include/ingestion/ingestion_manager.h) — admin, quarantine, source, and checkpoint APIs.
- [`include/ingestion/ingestion_diagnostic_emitter.h`](../../include/ingestion/ingestion_diagnostic_emitter.h) — structured incident categories and listener hooks.
- [`docs/operability/RUNBOOK_INGESTION_PIPELINE.md`](../../docs/operability/RUNBOOK_INGESTION_PIPELINE.md) — cross-module runbook; validate its deployment-specific commands and metrics before using them.

> Exact model backend not verifiable from final document text alone.
