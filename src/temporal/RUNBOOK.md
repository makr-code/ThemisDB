---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Temporal incident runbook

This runbook is for maintainers diagnosing failures at the temporal C++ API
boundary. It uses observable API state rather than assuming that module code
provides a particular deployment CLI or metric exporter. For deployment-specific
log patterns and escalation contacts, consult
[`docs/operability/RUNBOOK_TEMPORAL_STORE.md`](../../docs/operability/RUNBOOK_TEMPORAL_STORE.md)
and validate its commands against the deployed administrative tooling.

## Triage sequence

1. Identify the affected table, operation, timestamps, configured policy, and
   whether the symptom began after retention enforcement or process restart.
2. Preserve relevant logs and CDC/WAL files before changing policy or deleting
   records. Do not retry destructive operations until the data horizon and
   backup/recovery path are known.
3. Check the owning runtime surface:

| Symptom | Inspect | Interpretation / safe next action |
|---|---|---|
| Snapshot query returns no rows | `isAlive`, `getSnapshotMetadata`, included table names | A released/unknown handle or absent table returns an empty result. Re-capture only when a current table view is acceptable; snapshots are in-memory copies, not restart recovery points. |
| Retention did not run or returned errors | `getPolicy`, `RetentionStats.errors`, `getCumulativeStats`, `schedulerRunning` | Missing policy is reported in `errors`; verify table identity and policy before enforcement. Keep the table alive while it is scheduled. |
| Historical rows/events are missing | Retention policy and counts; CDC `logSize`/`overflowCount` or persistent-log state | Retention physically removes eligible history. CDC ring overflow makes in-memory replay incomplete; do not treat it as a complete audit trail. |
| Conflict remains unresolved | `getUnresolvedConflicts` / `getQueuedConflicts`, policy, resolver statistics | Manual policy intentionally needs a resolution decision. Preserve both versions and obtain the owning application's merge policy before resolving. |
| Query result differs at a boundary | Timestamp unit, `TimeRange` start/end, namespace/header used | Runtime `TimeRange` is millisecond, half-open. Check conversion and endpoint equality before changing data. |

## Recovery and rollback safety

- For retention errors, stop the scheduler before table teardown or policy
  changes; inspect the exact policy and affected historical versions before
  restarting it. A purge is not reversed by restoring a policy.
- For lost in-memory CDC history, replay only the retained range and report the
  gap. If a `CDCPersistentLog` was configured, preserve its segment directory
  and validate/replay through that API; do not assume the in-memory log can
  reconstruct evicted events.
- A new `TemporalSnapshotManager` snapshot captures current scans. It does not
  restore an earlier process state. Do not describe it as point-in-time recovery.
- For unresolved conflicts, retain both input snapshots and the conflict
  record. Custom merge logic must be tested for determinism and the documented
  commutativity/idempotency properties.
- Escalate suspected data loss, unexpected purge, corrupted WAL, or disagreement
  between deployed CLI output and the C++ API to the temporal/storage owners.
  Avoid manual file edits or unverified repair commands.

The project-level operational runbook lists five deployment scenarios and
operator examples, but the module headers do not declare those CLI commands or
metric names. Treat them as deployment procedures only after local validation.

Exact model backend not verifiable from final document text alone.
