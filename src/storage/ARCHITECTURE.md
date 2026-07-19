# Architecture - Storage Module

<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The storage module composes durable key-value persistence, MVCC/WAL lifecycle behavior, blob/tiering and redundancy paths, backup-recovery behavior, and storage integrity/audit surfaces into a bounded subsystem.

## Main Execution Planes

1. Persistence and lifecycle plane
- RocksDB wrapper, storage engine, key schema, and WAL/MVCC behavior

2. Recovery and maintenance plane
- backup/PITR/compaction/pruning/index-maintenance behavior

3. Blob, tiering, and integrity plane
- blob/tiered/redundancy/encryption behavior and storage audit surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| persistence contract | durable read/write behavior with explicit lifecycle control |
| versioning contract | deterministic MVCC and replay semantics |
| recovery contract | explicit backup/PITR success/failure behavior |
| integrity contract | storage-side signature/audit and redundancy observability |

## Failure Semantics

- storage lifecycle and open/write/replay failures are explicit.
- backup/PITR faults remain diagnosable and non-silent.
- blob/tier/redundancy path failures surface deterministic outcomes.
- maintenance job failures remain observable through module diagnostics.

## Sourcecode Verification (Module: storage/architecture)

- Verified files:
  - src/storage/rocksdb_wrapper.cpp
  - src/storage/storage_engine.cpp
  - src/storage/mvcc_store.cpp
  - src/storage/wal_storage.cpp
  - src/storage/backup_manager.cpp
  - src/storage/pitr_manager.cpp
  - src/storage/compaction_manager.cpp
  - src/storage/tiered_storage.cpp
  - src/storage/blob_redundancy_manager.cpp
  - src/storage/security_signature_manager.cpp
  - src/storage/storage_audit_logger.cpp
- Verified architecture claims:
  - persistence/lifecycle + recovery/maintenance + integrity/tiering plane split
  - explicit failure boundaries for storage, recovery, and maintenance faults
  - module-local ownership of storage-domain behavior