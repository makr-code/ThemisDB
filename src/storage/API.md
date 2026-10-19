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

# Storage API Contract Summary

## API scope

The storage module exposes the durable runtime contracts that underpin ThemisDB persistence. The design centers on the APIs defined in the storage headers under `include/storage/` and the implementation logic in `src/storage/`.

## API categories

### 1. Storage lifecycle and orchestration

The primary runtime entry points are built around durable lifecycle behavior: creating the storage engine, choosing dependency implementations, managing configuration, and validating fail-closed requirements in production mode.

Key design points from `storage_engine.cpp`:

- default no-op implementations are not allowed in production,
- configuration must use real encryption, key-provider, and index-manager implementations,
- storage cannot silently downgrade to insecure defaults.

### 2. MVCC and versioned reads/writes

The MVCC API model is implemented in `mvcc_store.cpp` and revolves around:

- versioned key encoding,
- timestamp-based ordering,
- latest-version fast lookup,
- tombstone-based deletion semantics.

The contract is explicit: a value is read via the newest relevant version, not by blindly overwriting the current state.

### 3. WAL durability and recovery

The WAL path exposes the semantics required for crash-safe persistence. It is not merely a buffer. It has explicit durability expectations and recovery-oriented error handling. In `wal_storage.cpp`, the module treats write and fsync success as required for acknowledged durable state.

### 4. Backup and PITR

The backup API contract is designed around manifest validation and restore safety. `backup_manager.cpp` and `pitr_manager.cpp` make the contract explicit: a restore is valid only when it can be reconstrued under the module's security and integrity rules.

### 5. Tiering, compaction, and maintenance

The storage module also includes maintenance APIs for storage pressure and compaction operations. These are operational interfaces that must remain observable and fail explicitly when their constraints are violated.

## Contract invariants

The API contract is intentionally strict:

- production mode must reject insecure no-op configuration,
- recovery must fail closed on missing codec or crypto support,
- empty values represent tombstones, not ordinary data,
- versioned keys must be strictly ordered by logical time,
- WAL durability is required before a write is acknowledged as committed.

## Error semantics

The module's error semantics are not best-effort. A failure must be diagnosable and should surface a concrete storage error or recovery fault rather than silently proceeding. This is a direct consequence of the source-level implementation in `storage_error_diagnostics.*`, `storage_recovery_fault_handler.*`, and `storage_pressure_manager.*`.

## Caller expectations

Callers should assume:

- there is no implicit fallback to insecure defaults in production,
- a partial or corrupt restore is rejected,
- a missing backend is a hard failure for the relevant operation,
- reads and writes follow version ordering guarantees,
- storage maintenance and recovery errors are observable and actionable.

## Boundary of the API

This API is constrained to storage domain concerns: persistence, durability, recovery, integrity, and maintenance. It is not the place for query planning, network transport, or application-domain logic. That separation keeps the runtime contract stable and auditable.
