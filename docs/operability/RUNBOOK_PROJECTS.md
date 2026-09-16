# Runbook: Projects — Wave D Operability

<!-- Runbook: projects | Wave D | validated: 2026-09-16 -->
<!-- Links: src/projects/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `projects`
module. Use it to diagnose and remediate project mutation failures, snapshot
corruption, collaboration conflicts, and lifecycle errors.

---

## Scenario 1 — Mutation Failed

**Log pattern:** `[PROJECTS:MutationFailed]`

### Symptoms
- Project field mutation operations fail or are dropped.
- `[PROJECTS:MutationFailed]` emitted with `project_id`, `mutation_type`, and `error_code` fields.
- Version graph may be inconsistent; diff operations may return stale data.

### Diagnosis
1. Confirm mutation failures:
   ```
   grep '\[PROJECTS:MutationFailed\]' /var/log/themisdb/projects.log
   ```
2. Identify `mutation_type` (create, update, delete, merge) and `error_code`.
3. Check version graph health:
   ```
   themisdb-admin projects check-version-graph --project-id <id>
   ```
4. Review `projects_mutation_failure_total` metric.

### Remediation
1. For transient failures: retry mutation with exponential backoff.
2. For version graph inconsistency: run `themisdb-admin projects repair-version-graph`.
3. Confirm `[PROJECTS:MutationFailed]` stops and mutation throughput returns to baseline.

### Escalation
Escalate to the data team if mutation failure rate exceeds 0.01% or if version graph
repair fails.

---

## Scenario 2 — Snapshot Corruption

**Log pattern:** `[PROJECTS:SnapshotCorruption]`

### Symptoms
- Snapshot restore operations fail or return inconsistent state.
- `[PROJECTS:SnapshotCorruption]` emitted with `snapshot_id`, `version`, and `checksum_mismatch` fields.
- Restore operations may return stale or partially applied state.

### Diagnosis
1. Confirm snapshot corruption:
   ```
   grep '\[PROJECTS:SnapshotCorruption\]' /var/log/themisdb/projects.log
   ```
2. Identify `snapshot_id` and `checksum_mismatch` details.
3. Verify snapshot checksum against stored reference:
   ```
   themisdb-admin projects verify-snapshot --snapshot-id <id>
   ```
4. Review `projects_snapshot_corruption_total` metric.

### Remediation
1. Mark corrupted snapshot as invalid: `themisdb-admin projects invalidate-snapshot --id <id>`.
2. Identify and use the last known good snapshot for restore.
3. Investigate storage backend for write errors (disk health, NFS stability).
4. Confirm subsequent snapshots pass checksum validation.

### Escalation
Escalate to the storage team if multiple snapshots are corrupt or if the storage backend
shows write errors.

---

## Scenario 3 — Collaboration Conflict

**Log pattern:** `[PROJECTS:CollaborationConflict]`

### Symptoms
- Multi-actor collaboration operations result in unresolved conflicts.
- `[PROJECTS:CollaborationConflict]` emitted with `project_id`, `actor_count`, and `conflict_type` fields.
- Collaboration lock contention may increase; some actors may be blocked.

### Diagnosis
1. Confirm collaboration conflicts:
   ```
   grep '\[PROJECTS:CollaborationConflict\]' /var/log/themisdb/projects.log
   ```
2. Identify `conflict_type` (merge, lock_contention, permission_denied).
3. Check current lock holders:
   ```
   themisdb-admin projects list-locks --project-id <id>
   ```
4. Review `projects_collaboration_conflict_total` metric.

### Remediation
1. For lock contention: identify stale lock holders and release with
   `themisdb-admin projects release-lock --lock-id <id>`.
2. For merge conflicts: invoke merge strategy resolver with
   `themisdb-admin projects resolve-conflict --project-id <id> --strategy last-write-wins`.
3. Confirm conflict rate returns to zero after remediation.

### Escalation
Escalate to the platform team if lock contention persists for more than 5 minutes or if
merge conflict resolution fails.

---

## Scenario 4 — Lifecycle Error

**Log pattern:** `[PROJECTS:LifecycleError]`

### Symptoms
- Project lifecycle transitions fail (create, archive, restore, delete).
- `[PROJECTS:LifecycleError]` emitted with `project_id`, `from_state`, `to_state`, and `error_code` fields.
- Projects may be stuck in an invalid intermediate state.

### Diagnosis
1. Confirm lifecycle errors:
   ```
   grep '\[PROJECTS:LifecycleError\]' /var/log/themisdb/projects.log
   ```
2. Identify `from_state`, `to_state`, and `error_code`.
3. Check project state machine consistency:
   ```
   themisdb-admin projects inspect --project-id <id>
   ```
4. Review `projects_lifecycle_error_total` metric.

### Remediation
1. For invalid transition errors: reset to last known valid state via
   `themisdb-admin projects reset-state --project-id <id> --to-state <state>`.
2. For stuck projects: force state reconciliation with
   `themisdb-admin projects reconcile --project-id <id>`.
3. Confirm project transitions normally after remediation.

### Escalation
Escalate to the data team if multiple projects are in invalid states or if reconciliation
fails.

---

## Scenario 5 — Snapshot Restore Under High Mutation Load

**Log pattern:** `[PROJECTS:SnapshotCorruption]` with `mutation_pressure=high`

### Symptoms
- Snapshot restore operations fail during high-throughput mutation windows.
- Race between active mutations and in-flight snapshot operations.
- `projects_snapshot_concurrent_mutations` metric elevated.

### Diagnosis
1. Confirm concurrent mutation pressure:
   ```
   grep 'mutation_pressure=high' /var/log/themisdb/projects.log
   ```
2. Review active mutation queue depth at time of failure.
3. Check snapshot locking behavior (`projects.snapshot.exclusive_lock_timeout_ms`).

### Remediation
1. Enable write quiesce before snapshot: `projects.snapshot.quiesce_writes=true`.
2. Increase snapshot lock timeout if quiesce is not feasible.
3. Schedule snapshots during low-mutation windows (e.g., maintenance windows).
4. Confirm subsequent snapshots complete without corruption.

### Escalation
Escalate to the platform team if snapshot success rate falls below 99% during business hours.
