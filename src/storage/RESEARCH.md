---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Storage Research Notes

## Scope

This research note covers the design assumptions behind the ThemisDB storage module as implemented in `src/storage` and aligned to `src/storage/README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, and the durability-oriented runtime code in `mvcc_store.cpp`, `wal_storage.cpp`, `backup_manager.cpp`, and `storage_engine.cpp`.

## Source-based problem statement

The storage module is not a generic key-value wrapper. It owns the durability boundary for ThemisDB: a write must be persisted, replayable, and recoverable even after power loss, partial writes, or maintenance replay. The module codifies that in three major design decisions:

1. versioned writes using MVCC and tombstones,
2. WAL-first durability before write completion,
3. explicit fail-closed behavior for missing production dependencies and unsafe restore paths.

The source code is explicit about this. `MVCCStore::putWithTimestamp()` advances a logical clock and writes versioned keys with a tombstone model, while `wal_storage.cpp` treats local WAL fsync and write operations as explicit durability checkpoints. `StorageEngine` also rejects default no-op dependency implementations in production, which prevents silent data-loss or misconfiguration.

## Research questions

### 1. What is the correctness boundary?

The fundamental requirement is deterministic read consistency after crash or replay. The code treats the versioned key as the atomic unit: writes are encoded as `base_key + '\0' + timestamp`, and reads prefer the most recent version from the latest timestamp cache before falling back to iterator-based recovery. This establishes a single source of truth for snapshot reads without requiring a broad in-memory state.

### 2. How much durability can be trusted?

The WAL path is the central durability gate. In `wal_storage.cpp`, writes and fsyncs are retried around EINTR and fail explicitly when the underlying filesystem or local storage layer cannot complete the write. That is a stronger strategy than silent data loss. It is operationally equivalent to the principle that commit acknowledgment only happens after durable persistence succeeds.

### 3. What failure modes are accepted?

The storage module deliberately fails closed, not softly. Examples in source include:

- `StorageEngine` rejects default no-op encryption, key provider, and index manager implementations in production mode.
- `BackupManager` rejects unsupported restore operations when the required codec or crypto backend is not available instead of silently copying raw data.
- recovery and storage fault logic in `storage_error_diagnostics.*`, `storage_recovery_fault_handler.*`, and `storage_pressure_manager.*` makes error states diagnosable rather than hidden.

## Assumptions and constraints

- Recovery is deterministic only if WAL ordering is preserved and replay is idempotent.
- Snapshot reads require a stable version ordering; the `MVCCStore` design assumes a monotonic logical clock for version ordering.
- Tiered storage and redundancy paths need bounded placement logic and explicit errors when backends are unavailable.
- Backup and PITR require both integrity validation and manifest-based restore safety.

## Methods used in the module

The implementation is evidence-driven rather than speculative:

- `mvcc_store.cpp`: version encoding, latest-version cache, tombstone semantics, and read-before-write access patterns.
- `wal_storage.cpp`: durable write semantics, fsync/retry behavior, and file descriptor lifecycle.
- `backup_manager.cpp`: local and remote backup manifests, guarded restore, checksum and codec validation.
- `storage_engine.cpp`: production fail-closed dependency checks and runtime guardrails.
- `tiered_storage.cpp`: access-driven promotion and demotion decisions.

## Evaluation criteria

The storage module is evaluated by the following outcomes:

- write durability on local/remote persistence layers,
- replay correctness after unclean shutdown,
- snapshot isolation under active writes,
- backup integrity and PITR determinism,
- bounded behavior under storage pressure and compaction,
- no silent degradation in production configuration.

## Trade-offs

### MVCC correctness vs. write amplification

MVCC introduces additional version keys and cache management overhead, but it gives consistent read snapshots and a clean recovery story. The module intentionally trades some write amplification for stronger transaction semantics.

### WAL durability vs. throughput

The synchronous WAL path prioritizes safety over maximum throughput. It chooses explicit sync semantics over cheap but unsafe buffered writes, which matches the production requirement for trustworthy durability.

### Recovery safety vs. convenience

Fail-closed logic reduces service convenience in misconfigured environments, but it prevents silent corruption. The source code repeatedly chooses explicit errors over hidden fallback behavior.

## Conclusion

The storage module is best understood as a correctness-first durability substrate. It is not designed to optimize for abstract throughput alone; its core value is that state remains recoverable, versioned, and traceable even under failure. That design matches the project-level requirement that storage remain a trustworthy foundation for higher-level data services.
