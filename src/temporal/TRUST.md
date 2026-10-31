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

# Temporal trust boundaries

This page describes trust and data-protection boundaries for the temporal
module. It complements [`SECURITY.md`](SECURITY.md) with concrete API and
operator responsibilities; it is not an authentication or authorization
specification.

## Data and execution boundaries

- **Caller to temporal API:** Keys, timestamps, ranges, JSON documents,
  retention policies, and table names are caller-provided. Validate them at the
  service boundary and use the runtime's millisecond, half-open range
  convention. The temporal library does not establish caller identity or
  authorize a table operation.
- **CDC to subscribers:** `ChangeEvent` includes table/entity identifiers,
  before/after JSON, timestamps, and an optional user identifier. Subscriber
  callbacks receive these payloads synchronously. Restrict subscriptions and
  avoid placing secrets or unnecessary personal data in event payloads.
- **In-memory history:** snapshots, the default CDC replay log, conflict
  history, manual conflict queues, and retention archive are process memory.
  They are not durable backups and may expose historical payloads to
  application code with access to those objects.
- **Persistent CDC WAL:** `CDCPersistentLog` writes serialized event payloads to
  filesystem segments. CRC-32 detects invalid records; it does not encrypt data
  or authenticate the writer. Deployment owners must protect directory
  permissions, backups, retention, and deletion.
- **Extension callbacks:** custom merge resolvers and CDC callbacks execute
  application-supplied code. Validate their inputs, constrain their privileges,
  and review concurrency and exception behavior before registering them.

## Controls and incident handling

Keep authorization at the owning service boundary; do not disable those checks
to recover temporal data. Use bounded retention and snapshots appropriate to
the required audit horizon. Protect WAL and backup files with deployment
controls because temporal events may contain full record values. Treat a
nonzero CDC overflow count as an audit/replay completeness warning.

If retention deletes data unexpectedly, stop further scheduling, preserve
available backups/logs, record the active policy and table identity, and
escalate before attempting recovery. If payloads were exposed through a
subscription or WAL, follow the service's data-incident process and assess
subscriber access, file access, and retention/deletion obligations. See
[`RUNBOOK.md`](RUNBOOK.md) and
[`docs/operability/RUNBOOK_TEMPORAL_STORE.md`](../../docs/operability/RUNBOOK_TEMPORAL_STORE.md).

Exact model backend not verifiable from final document text alone.
