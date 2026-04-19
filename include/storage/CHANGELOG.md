<!-- Status: current | validated: 2026-04-06 -->

# Changelog — include/storage/

All notable changes to the **public storage headers** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

> Implementation changelog: [`../../src/storage/CHANGELOG.md`](../../src/storage/CHANGELOG.md)

---

## [Unreleased]

### Planned
- `vector_index_backend.h` — ANN / HNSW index backend interface
- `encrypted_blob_backend.h` — transparent client-side encryption backend

---

## [1.6.0] — 2026-03-12

### Added
- `wom_tree.h` — Write-Optimised Merge Tree interface for high-ingest workloads
- `zero_copy_blob_transfer.h` — zero-copy blob send/receive (sendfile / splice)
- `nlp_metadata_extractor.h` — NLP-based blob metadata extraction interface
- `online_schema_migration.h` — zero-downtime online schema migration
- `batch_write_optimizer.h` — write coalescing and reordering for hot paths

### Changed
- `storage_engine.h` — added `BeginBulkLoad()` / `EndBulkLoad()` for ETL paths
- `mvcc_store.h` — added `CompactRange()` to force compaction on specific key ranges
- `tiered_storage.h` — added `PromotionPolicy` enum for manual tier promotion
- `pitr_manager.h` — added `EstimateRestoreTime()` for operational planning
- `backup_manager.h` — added incremental backup chaining (`ChainedBackup`)

### Fixed
- `raft_mvcc_bridge.h` — corrected `ApplyLogEntry()` return semantics on
  no-op entries (was incorrectly returning `NOT_FOUND`)

---

## [1.5.0] — 2025-12-10

### Added
- `gpu_compression.h` — GPU-accelerated compression interface (CUDA-optional)
- `erasure_coding_backend.h` — erasure-coded blob storage backend
- `blob_redundancy_manager.h` — cross-backend redundancy management
- `nvme_manager.h` — NVMe device lifecycle and io_uring integration
- `distributed_transaction_manager.h` — distributed ACID transaction coordinator

### Changed
- `compression_strategy.h` — added `CompressionLevel` enum; deprecated
  plain `bool fast` parameter
- `compaction_manager.h` — added `PauseCompaction()` / `ResumeCompaction()`
- `disk_space_monitor.h` — added threshold-based callback registration

---

## [1.4.0] — 2025-09-15

### Added
- `adaptive_compaction.h` — workload-adaptive compaction policy
- `columnar_format.h` — columnar data layout for analytics reads
- `hlc.h` — Hybrid Logical Clock interface
- `history_manager.h` — row-version history for time-travel queries
- `transaction_retry_manager.h` — configurable retry logic for write conflicts

### Changed
- `rocksdb_wrapper.h` — added `OpenReadOnly()` for secondary read replicas
- `key_schema.h` — added vector and timeseries key encoding schemas
- `index_maintenance.h` — added background index rebuild API

---

## [1.3.0] — 2025-06-20

### Added
- `blob_backend_gcs.h` — Google Cloud Storage backend
- `security_signature.h`, `security_signature_manager.h` — content integrity signing
- `storage_audit_logger.h` — immutable storage audit log
- `pitr_manager.h` — Point-in-Time Recovery management

### Changed
- `blob_storage_backend.h` — added `MultipartUpload` API
- `backup_manager.h` — added `VerifyBackup()` integrity check
- `mvcc_store.h` — exposed `GetSequenceNumber()` for external MVCC coordination

---

## [1.2.0] — 2025-03-10

### Added
- `raft_mvcc_bridge.h` — bridge between Raft log and MVCC store
- `wal_storage.h` — WAL-backed durable write interface
- `merge_operators.h` — pluggable RocksDB merge operator interface
- `compressed_storage.h` — transparent compressed storage layer

### Changed
- `storage_engine.h` — refactored to pure-virtual `IStorageEngine` interface
- `database_connection_manager.h` — added connection health-check API

---

## [1.1.0] — 2024-12-10

### Added
- `blob_backend_filesystem.h` — local filesystem blob backend
- `tiered_storage.h` — multi-tier data placement policy
- `online_schema_migration.h` (initial stub — promoted to full API in v1.6.0)

### Changed
- `rocksdb_wrapper.h` — added column-family management API

---

## [1.0.0] — 2024-09-01

### Added
- Initial public header set: `storage_engine.h`, `mvcc_store.h`,
  `rocksdb_wrapper.h`, `key_schema.h`, `backup_manager.h`,
  `blob_storage_manager.h`, `blob_storage_backend.h`,
  `compaction_manager.h`, `disk_space_monitor.h`,
  `index_maintenance.h`, `base_entity.h`.
