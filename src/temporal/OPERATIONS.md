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

# Temporal operations

This page defines operational checks for teams embedding temporal components.
It covers lifecycle and data-safety constraints visible in the module APIs;
deployment configuration, metrics export, and administrative commands belong
to the consuming service and its operations documentation.

## Before enabling lifecycle work

- Set and review a retention policy for each table before calling
  `enforceRetention` or starting the scheduler. `RetentionPolicy` has defaults,
  including a one-year time period and zero meaning unlimited for the
  storage-based limit and incremental batch size; do not assume those defaults
  match a deployment's legal or recovery horizon.
- Decide whether archived records are required. The `RetentionManager` archive
  is process-memory state and is cleared by `clearArchive`; it is not durable
  cold storage.
- Ensure every `SystemVersionedTable` passed to `scheduleTable` outlives its
  registration. Stop the scheduler before destroying or replacing scheduled
  tables.
- Set a bound for snapshots. `TemporalSnapshotManager` retains deep copies until
  explicit release or age/count garbage collection; inspect snapshot count and
  statistics when sizing memory.
- Size the `TemporalCDC` ring for the replay window. Default capacity is
  65,536 events; its overflow counter indicates that replay may be incomplete.
  If durable replay is required, configure `CDCPersistentLog` and protect its
  directory as persistent event data.

## Routine checks

Inspect `RetentionStats` (`versions_examined`, `versions_deleted`,
`versions_archived`, execution time, and errors) after enforcement and review
`getCumulativeStats()` for totals. Check snapshot liveness and metadata before
using a handle; release no-longer-needed snapshots. Monitor CDC log size and
overflow count, and account for callback execution on the publishing path.
Inspect unresolved conflicts rather than treating a manual-resolution policy
as an automatic merge.

Use [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for release
benchmark thresholds and its required benchmark-manifest completeness. This
module does not define a standalone deployment metric exporter or operational
CLI in its public headers.

## Change, rollback, and escalation

Changing a retention policy affects future cleanup only; versions already
purged are not recreated by reverting the policy. Before rollout, preserve
backups or other approved recovery sources and validate the policy on the
expected history shape. Roll back by stopping the scheduler and restoring the
previous reviewed policy, not by manually editing table or WAL data.

Escalate policy-design questions to the temporal data owner, durable storage
and WAL issues to storage operations, and deployment CLI/metrics questions to
the service operator. Use [`RUNBOOK.md`](RUNBOOK.md) for incident triage.

Exact model backend not verifiable from final document text alone.
