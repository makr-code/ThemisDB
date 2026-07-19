# ThemisDB Storage Module

<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The storage module provides persistence, versioned data handling, blob and tiered storage behavior, WAL/durability paths, backup-recovery behavior, and storage-level observability for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| include/storage/storage_module.h | Doxygen module entry point for storage-wide API grouping |
| rocksdb_wrapper.cpp | RocksDB integration and durable storage wrapper behavior |
| storage_engine.cpp | storage orchestration and lifecycle behavior |
| mvcc_store.cpp | MVCC version/snapshot behavior |
| wal_storage.cpp | WAL path behavior |
| backup_manager.cpp | backup creation and lifecycle behavior |
| pitr_manager.cpp | point-in-time recovery behavior |
| tiered_storage.cpp | tiered data placement behavior |
| blob_redundancy_manager.cpp | blob redundancy behavior |
| encrypted_blob_backend.cpp | encrypted blob backend behavior |
| erasure_coding_backend.cpp | erasure-coded backend behavior |
| compaction_manager.cpp | compaction control behavior |
| adaptive_compaction.cpp | adaptive compaction behavior |
| index_maintenance.cpp | storage-side index maintenance behavior |
| storage_audit_logger.cpp | storage audit behavior |
| security_signature_manager.cpp | storage signature/integrity behavior |

## Scope

In scope:
- durable key-value and blob/tiered storage behavior
- MVCC/WAL/backup/PITR/compaction and lifecycle behavior
- storage integrity, audit, and maintenance surfaces

Out of scope:
- query planning and non-storage API orchestration
- external business-domain logic outside storage boundaries

## Runtime Behavior and Limits

- durable write and replay behavior is bounded by storage configuration.
- MVCC/version lifecycle behavior exposes explicit validity transitions.
- backup and PITR behavior exposes deterministic success/failure outcomes.
- tiered/blob paths are configuration-sensitive and must remain observable.

## Sourcecode Verification (Module: storage/readme)

- Verified files:
  - src/storage/rocksdb_wrapper.cpp
  - src/storage/storage_engine.cpp
  - src/storage/mvcc_store.cpp
  - src/storage/wal_storage.cpp
  - src/storage/backup_manager.cpp
  - src/storage/pitr_manager.cpp
  - src/storage/tiered_storage.cpp
  - src/storage/blob_redundancy_manager.cpp
  - src/storage/encrypted_blob_backend.cpp
  - src/storage/erasure_coding_backend.cpp
  - src/storage/compaction_manager.cpp
  - src/storage/adaptive_compaction.cpp
  - src/storage/index_maintenance.cpp
  - src/storage/storage_audit_logger.cpp
  - src/storage/security_signature_manager.cpp
- Verified behavior surfaces:
  - persistence/MVCC/WAL/backup/recovery/tiering/security and maintenance paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md