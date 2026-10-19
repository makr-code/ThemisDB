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

# Storage Implementation Guide

## Architectural implementation map

The storage module is organized around a few concrete runtime responsibilities:

- `storage_engine.cpp` owns the high-level lifecycle and fail-closed production guards.
- `mvcc_store.cpp` implements versioned keys, latest-version lookup, and tombstones.
- `wal_storage.cpp` implements durable WAL writes, fsync, and local file semantics.
- `backup_manager.cpp` and `pitr_manager.cpp` implement backup manifest and restore safety.
- `tiered_storage.cpp` and related maintenance files implement policy-based storage movement and access tracking.
- `security_signature_manager.cpp` and `storage_audit_logger.cpp` provide integrity and audit surfaces.

## Core implementation invariants

### MVCC invariant

`MVCCStore` encodes every version with a null separator and timestamp suffix. Reads use the latest cached timestamp when available before resolving the correct historical version. This allows the module to provide a consistent snapshot model while keeping read hot paths efficient.

### WAL invariant

The WAL code treats a write as safe only after the write and fsync path complete successfully. Recovery paths are built around the assumption that a partially written WAL record is not valid and must be treated as a failure condition.

### Production safety invariant

The storage engine rejects the default no-op encryption, key provider, and index manager dependencies in production. This is an explicit guardrail against silent degradation.

### recovery invariant

Backups and restores are only trustworthy when the manifest and payloads validate. Unsupported codecs and missing crypto or transport dependencies are treated as fail-closed errors rather than silent fallback.

## Important implementation details

### `MVCCStore::putWithTimestamp()`

This method updates the logical clock, writes a versioned key to RocksDB, and refreshes the latest timestamp map. The implementation explicitly preserves ordering by only updating the cached latest timestamp when the new timestamp is not older than the current value.

### `MVCCStore::getLatest()`

The read path uses a fast lookup for the latest per-key timestamp and falls back to an iterator-based read path when there is no cached entry or the direct lookup cannot resolve the state. This is a practical compromise: fast point reads with a correct slow path.

### `wal_storage.cpp` file descriptor management

The WAL path uses a small RAII helper for file descriptors and retries around `EINTR` conditions. This is important because local file I/O is not timeouts-driven like a network I/O path, but it still needs deterministic handling of interrupt-driven system calls.

### `BackupManager` guardrails

The backup module checks for local path safety, remote manifest validity, and the availability of required codecs and cryptography. It does not quietly continue when a required backend is absent; instead it fails explicitly.

## Edge cases and deliberate handling

- empty values as tombstones,
- invalid or absent key providers in production,
- missing codec/crypto support for backup restore,
- partial WAL or replay failure,
- compaction or storage pressure under mixed workloads,
- stale snapshot states during recovery.

The project-level docs and source indicate that these are not incidental edge cases; they are the exact scenarios the module is designed to detect and classify.

## Testing and validation expectations

The storage module requires validation around:

- MVCC read/write consistency,
- WAL replay after interruption,
- failed recovery conditions,
- backup manifest validation,
- tiering and compaction correctness under load,
- production fail-closed behavior when unsafe defaults are used.

## Operational takeaway

The implementation makes a deliberate choice: correctness and explicit failure are preferred over silent recovery tricks. The module is designed to be explainable, inspectable, and recoverable under failure rather than optimized for ambiguous best-effort behavior.
