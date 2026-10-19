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

# Storage Domain Modeling

## Domain model

The storage module models data as a durable, versioned key-value substrate with a few explicit overlays:

- key-value records with MVCC timestamps,
- WAL commit records for crash recovery,
- blob objects with redundancy or tier placement,
- backup manifests plus PITR restore metadata,
- storage integrity and audit state.

This model is visible in the source tree: `mvcc_store.cpp` maintains versioned keys and tombstones, `wal_storage.cpp` serializes writes into a durable log, and `backup_manager.cpp` treats backup creation and recovery as manifest-driven operations.

## Core entities

### 1. Versioned record

A record is the logical pair of:

- a base key, such as a logical document or row identifier,
- a timestamped revision, encoded as `base_key + '\0' + HLCTimestamp`.

This is the central MVCC representation used in `MVCCStore::encodeVersionedKey()` and `decodeTimestamp()`. The code explicitly tracks the latest timestamp for each key to optimize point reads while preserving a full version history for older snapshots.

### 2. Tombstone

Deletion is modeled as a versioned key with an empty value. This is important because it preserves a deletion event in the timeline instead of silently removing history. That design lets reads remain deterministic and allows replay to see that a key was deleted at a specific logical time.

### 3. WAL entry

The WAL path in `wal_storage.cpp` writes a sequence of durable records and ensures the data is flushed to disk before the write is considered complete. The implementation treats `fsync()` and file-descriptor error paths as part of the contract, not as optional quality enhancements.

### 4. Backup manifest

`backup_manager.cpp` builds backup workflows around a manifest plus payload objects. The manifest defines the backup's structure, and restore logic validates it before reconstructing the stored state. This keeps remote and local backup paths consistent and prevents partial backups from being treated as valid.

## Invariants

The module depends on a few invariants that must remain true:

1. Every durable write must either be fully present in the WAL or explicitly fail.
2. A key version must be ordered by the logical clock; older versions remain readable until compaction or expiry removes them.
3. Empty values indicate tombstones, never ordinary data.
4. Recovery must fail closed when integrity, codec, or backend availability cannot be verified.
5. Production mode must reject unsafe defaults rather than quietly downgrade to insecure or no-op behavior.

## Failure-domain model

The storage module explicitly separates the following concerns:

- persistence and lifecycle (`storage_engine.cpp`, `rocksdb_wrapper.cpp`),
- versioning and snapshot semantics (`mvcc_store.cpp`),
- durable replay (`wal_storage.cpp`),
- recovery and backup (`backup_manager.cpp`, `pitr_manager.cpp`),
- tiering and maintenance (`tiered_storage.cpp`, `compaction_manager.cpp`),
- integrity, audit, and verification (`security_signature_manager.cpp`, `storage_audit_logger.cpp`).

These are not overloaded responsibilities; they are distinct layers with different failure modes.

## Modeling decisions that matter

### Fail-closed dependency model

Production safety was strengthened by rejecting default no-op implementations for encryption, key providers, and index managers. The design assumes that misconfiguration is a higher-risk failure than a temporary degraded mode.

### Explicit serialization around the durability path

The WAL and write-ahead lifecycle enforce that the write is not acknowledged before the log succeeds. This is crucial for any system that needs to preserve correctness after restart.

### Recovery as a first-class contract

`BackupManager` and related recovery logic are modeled as recovery routines, not as optional maintenance. That means restore is validated, manifests are checked, and errors surface as explicit storage failure states rather than silent rollback.

## Data flow summary

1. application write arrives at storage engine,
2. versioned key is generated alongside logical timestamp,
3. WAL is updated for durability,
4. in-memory latest-version cache records the newest state,
5. compaction, recovery, and tiering operate over the versioned data set,
6. backup/PITR read the manifest and the stored payloads to reconstruct a consistent point-in-time view.

## Operational implications

This model matters because storage behavior is directly observable by higher layers. If the underlying storage path is unsafe, every downstream layer inherits the risk. The module therefore keeps the model simple and explicit: versioned state, durable commit log, guarded recovery, and fail-closed configuration enforcement.
