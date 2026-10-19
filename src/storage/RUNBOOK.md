---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

AI-assisted drafting for this document was performed with MAI-Code-1.1-Flash in the current task session.

# Storage Runbook

## Purpose

This runbook is for operators and maintainers responding to storage failures in ThemisDB. It focuses on the concrete failure paths present in the source: WAL write or fsync failure, MVCC inconsistency, recovery regression, backup corruption, tiering failure, and production-config misdeployment.

## Quick diagnosis checklist

1. Confirm whether the issue is at the write, recovery, or maintenance boundary.
2. Check storage log output for explicit `StorageErrorContext` or recovery diagnostics.
3. Determine whether the problem is local persistence, remote backup transport, or maintenance logic.
4. If production startup fails, verify that no insecure default backend is being used.
5. If a backup or PITR operation fails, verify the manifest, codec, and crypto prerequisites.

## Scenario 1: WAL write or fsync failure

### Symptoms

- writes are acknowledged but recovery detects incomplete durable records,
- logs show storage write or fsync errors,
- the database becomes unavailable after a crash or restart.

### Diagnosis

Check the storage log output near the write path and confirm whether the failure occurred before or after the WAL flush completed.

### Recovery

1. stop write traffic,
2. restore the last known-good checkpoint or backup,
3. replay the WAL only after verifying that the log integrity is intact,
4. restart writes after successful recovery validation.

### Prevention

- keep WAL write and fsync paths validated under crash simulations,
- ensure the storage filesystem path remains writable and consistent,
- avoid silent fallback to an unreliable local backend.

## Scenario 2: MVCC read inconsistency

### Symptoms

- reads return version churn or stale data,
- historical snapshots cannot be resolved as expected,
- the latest-version cache appears to regress.

### Diagnosis

Examine the versioned key encoding and timestamp ordering in `mvcc_store.cpp`, especially the latest timestamp update path.

### Recovery

1. pause writes,
2. rebuild or rehydrate the latest-version cache from the versioned key history,
3. verify reads against a known-good snapshot,
4. resume writes only after consistency checks pass.

## Scenario 3: Recovery or PITR failure

### Symptoms

- backup restore is rejected,
- PITR targets cannot be reconstructed,
- the module fails startup or recovery after a crash.

### Diagnosis

Check whether the failure is caused by:

- missing codec support,
- missing crypto backend support,
- invalid manifest structure,
- corrupted or partial payload objects.

### Recovery

1. verify the manifest and payloads are complete,
2. confirm the restore target is consistent with available storage snapshots,
3. reject partial restore attempts rather than continuing silently,
4. recover from a known-good backup if the latest snapshot cannot be validated.

## Scenario 4: Storage pressure or compaction stall

### Symptoms

- writes slow dramatically,
- compaction or maintenance stalls,
- storage audit logs show stress or capacity warnings.

### Diagnosis

Review the storage pressure and compaction diagnostics and confirm whether the runtime is exhausting the configured disk or tier thresholds.

### Recovery

1. inspect storage pressure logs and free space,
2. verify tiering thresholds and migration policies,
3. reduce write load or repair the failing tier backend,
4. resume normal operation only after compaction and pressure metrics return to expected ranges.

## Scenario 5: Production misconfiguration

### Symptoms

- storage service starts with default or insecure implementations,
- the service uses no-op encryption or missing key providers,
- the startup logs explicitly warn or throw about insecure defaults.

### Diagnosis

Review the dependency wiring for the storage engine. Production mode must not use default insecure helpers.

### Recovery

1. replace the default dependency with the correct implementation,
2. re-run startup validation,
3. verify the dependency is active and configured before resuming the workload.

## Escalation guidance

Escalate to the storage maintainer or release owner when:

- WAL, recovery, or PITR remains inconsistent after a clean restore attempt,
- integrity checks fail repeatedly,
- storage pressure or compaction issues reoccur across more than one recovery cycle,
- a change to storage config alters the durability contract.

## Rollback policy

Rollback is appropriate only when the issue is configuration-driven or caused by a recent storage change. Before rolling back, verify that the previous configuration will not silently reintroduce unsafe fallback behavior. Recovery, manifest integrity, and data durability checks must remain green before the system is returned to steady state.
