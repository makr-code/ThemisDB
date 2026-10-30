---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Storage Operations Guide

## Operational scope

This document covers the operator-visible actions required to keep the storage module healthy in production. It focuses on the lifecycle and operational behavior enforced by `storage_engine.cpp`, `mvcc_store.cpp`, `wal_storage.cpp`, `backup_manager.cpp`, `pitr_manager.cpp`, and the maintenance and diagnostics surfaces in the storage module.

## Core metrics and signals

Operators should pay attention to the following categories:

- WAL write and fsync health,
- MVCC snapshot and latest-version cache correctness,
- compaction and tier migration status,
- storage pressure and filesystem capacity,
- backup manifest and restore validation,
- integrity and audit signals from the security and audit layers.

### Concrete diagnostics and metrics

The storage error path emits diagnostics through the `storage.audit` logger with `code`, `incident`, `severity`, `op`, `resource`, `msg`, and `recovery` fields. In particular, `WAL_WRITE_FAILED`, `WAL_CORRUPTED`, `RECOVERY_INCOMPLETE`, `CHECKPOINT_FAILED`, `BACKUP_CORRUPTED`, `RESTORE_FAILED`, and `STORAGE_EXHAUSTED` require operator review. Recovery-fault events identify conditions such as `torn_wal_entry`, `checkpoint_invalid`, `recovery_timeout`, `wal_corruption`, and `recovery_retry_exhausted`.

The implemented Prometheus family for blob redundancy is:

- `themis_blob_redundancy_total_blobs`, `themis_blob_redundancy_healthy_blobs`,
  `themis_blob_redundancy_degraded_blobs`, and `themis_blob_redundancy_critical_blobs`
- `themis_blob_redundancy_logical_bytes`, `themis_blob_redundancy_physical_bytes`,
  `themis_blob_redundancy_storage_efficiency`, and
  `themis_blob_redundancy_repairs_total`

Alert on increasing critical/degraded blob counts or failed durability/recovery codes; correlate them with affected IDs and repair results. The source does not identify a single module-wide metric series for every WAL, MVCC, backup, and compaction path, so use the named error/audit diagnostics for those paths rather than assuming a metric exists.

## Daily operations

### Verify storage health

Check for explicit errors from the storage engine and recovery handlers. A healthy storage layer should not produce silent fallback warnings in production mode.

### Confirm WAL and write durability

Ensure the WAL path remains available and continues to flush successfully. Any failure in the write or fsync path is a direct durability issue and should be treated as production critical.

### Review backup and PITR validation

A valid backup is only a valid backup when its manifest and payloads validate cleanly. Scheduled backups should therefore be checked not only for completion but also for structural and integrity correctness.

### Review maintenance actions

Compaction and tiering are maintenance surfaces that can affect latency and recovery time. Operators should review whether maintenance is completing within expected time windows and whether any fail-closed protections are triggering.

## Incident response flow

1. identify the failing layer: WAL, MVCC, recovery, tiering, or integrity,
2. confirm the explicit error and affected subsystem,
3. isolate write traffic if the issue impacts durability or recovery,
4. restore from a validated backup or snapshot if recovery cannot complete,
5. implement corrective action before re-enabling normal traffic.

## Readiness checks

Before release or after a storage change, verify:

- production fail-closed guards remain active,
- no default insecure storage implementations are deployed,
- backup and PITR logic remains manifest-validated,
- WAL durability tests continue to pass,
- compaction and tiering remain observable under load.

## Escalation triggers

Escalate to the storage owner or release governance when:

- a backup or restore path has failed validation twice,
- a WAL failure prevents a safe restart,
- MVCC or latest-version correctness is in question,
- a config change affects the durable storage contract,
- compaction or tiering issues are coupled with data integrity risk.

## Operating principle

The storage module favors explicit, diagnosable failures over silent degradation. The operator's task is therefore not to guess whether the system is okay; it is to confirm that each critical subsystem remains validateable and recoverable.

## References

- [RUNBOOK.md](RUNBOOK.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/storage/storage_api_contract.h`
- `src/storage/storage_error_diagnostics.cpp`
- `src/storage/storage_recovery_fault_handler.cpp`
- `src/storage/blob_redundancy_manager.cpp`
