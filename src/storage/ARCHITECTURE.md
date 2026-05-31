> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Storage Module - Architecture Guide

**Version:** 1.2
**Last Updated:** 2026-05-31
**Module Path:** `src/storage/`

## 1. Overview

The storage module provides persistent data management on top of RocksDBWrapper, including MVCC/WAL surfaces, backup-recovery flows, blob backends, storage security helpers, and higher-level storage orchestration APIs.

## 2. Architecture Surfaces

| Surface | Source files |
|---|---|
| Core wrapper and engine orchestration | `rocksdb_wrapper.cpp`, `storage_engine.cpp`, `key_schema.cpp`, `base_entity.cpp` |
| Transaction/MVCC/WAL lifecycle | `mvcc_store.cpp`, `wal_storage.cpp`, `history_manager.cpp`, `merge_operators.cpp` |
| Backup and recovery | `backup_manager.cpp`, `pitr_manager.cpp` |
| Blob/tier/redundancy storage | `blob_backend_filesystem.cpp`, `blob_backend_s3.cpp`, `blob_backend_azure.cpp`, `blob_backend_webdav.cpp`, `blob_backend_gcs.cpp`, `blob_redundancy_manager.cpp`, `encrypted_blob_backend.cpp`, `erasure_coding_backend.cpp` |
| Performance and maintenance helpers | `batch_write_optimizer.cpp`, `compaction_manager.cpp`, `adaptive_compaction.cpp`, `columnar_cache.cpp`, `simd_filter.cpp`, `zero_copy_blob_transfer.cpp`, `streaming_ingest_manager.cpp` |
| Distributed/advanced coordination | `distributed_transaction_manager.cpp`, `raft_mvcc_bridge.cpp`, `online_schema_migration.cpp`, `storage_layout_advisor.cpp` |
| Security and audit in storage layer | `security_signature.cpp`, `security_signature_manager.cpp`, `storage_audit_logger.cpp` |

## 3. Runtime Control Flow

1. Open wrapper/engine and validate storage configuration.
2. Process writes and reads through wrapper/engine with key schema conventions.
3. Track transactional/versioned behavior via MVCC and WAL-related paths.
4. Route larger payloads through blob/redundancy/encryption abstractions.
5. Run maintenance and recovery flows (compaction, backup, PITR, pruning) as configured.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Used by | query/index/transaction and higher-level runtime components |
| Uses | RocksDB and storage backend abstractions |
| Exposes | storage wrapper/engine, backup-recovery, and storage-layer audit/security hooks |

## 5. Concurrency Model

- Wrapper and engine operations rely on explicit lifecycle/operation guards.
- Transaction/MVCC flows coordinate snapshot and write behavior through dedicated storage components.
- Background components (compaction, monitoring, ingest, retry) execute independently and feed into core storage paths.

## 6. Known Limits (Source-Visible)

- Capability breadth is high; operational behavior depends on runtime configuration and selected backends.
- Several advanced paths are present as dedicated components but are not universally default-enabled in all deployments.
- Performance expectations should remain benchmark-backed and not rely on unmeasured assumptions.

## 7. Sourcecode Verification (Module: storage/architecture)

- Verified files:
  - `src/storage/rocksdb_wrapper.cpp`
  - `src/storage/storage_engine.cpp`
  - `src/storage/mvcc_store.cpp`
  - `src/storage/wal_storage.cpp`
  - `src/storage/backup_manager.cpp`
  - `src/storage/pitr_manager.cpp`
  - `src/storage/key_schema.cpp`
  - `src/storage/blob_redundancy_manager.cpp`
  - `src/storage/security_signature.cpp`
  - `src/storage/security_signature_manager.cpp`
  - `src/storage/storage_audit_logger.cpp`
  - `src/storage/distributed_transaction_manager.cpp`
  - `src/storage/concurrent_write_controller.cpp`
  - `src/storage/online_schema_migration.cpp`
  - `src/storage/simd_filter.cpp`
- Verified interfaces/behaviors:
  - wrapper/engine orchestration and lifecycle gating
  - MVCC/WAL/backup/PITR behavior surfaces
  - blob redundancy and storage security/audit surfaces
  - maintenance and advanced coordination paths
