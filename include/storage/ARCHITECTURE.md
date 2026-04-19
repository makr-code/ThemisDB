<!-- Status: current | validated: 2026-04-06 -->

# include/storage/ — Architecture

> Public header interfaces for the ThemisDB Storage module.
> Implementation details live in [`../../src/storage/`](../../src/storage/).

---

## Overview

The `include/storage/` directory exposes the complete public API surface of
ThemisDB's persistent data layer.  Consumers depend only on these headers;
concrete implementations reside exclusively in `../../src/storage/`.

The storage module provides:
- **LSM-tree key-value engine** via RocksDB wrapper with MVCC support
- **Multi-model key schema** (relational, document, graph, vector, timeseries)
- **Blob storage** with pluggable backends (filesystem, GCS, erasure-coded)
- **Tiered storage** (NVMe → SSD → HDD → object store) with adaptive compaction
- **Backup and Point-in-Time Recovery (PITR)**
- **Compression** (pluggable strategy + GPU-accelerated path)
- **Security signatures** and field-level encryption interfaces
- **MVCC / Raft bridge** for distributed transaction integration
- **Online schema migration** and index maintenance
- **Audit logging** for compliance workloads

---

## Design Principles

1. **Layered Abstraction** — `storage_engine.h` is the single entry point;
   callers should not depend on RocksDB internals or blob backend specifics.
2. **Pluggable Backends** — blob backends (`blob_storage_backend.h`,
   `blob_backend_filesystem.h`, `blob_backend_gcs.h`) share a common interface
   enabling transparent backend substitution.
3. **MVCC-First Transactions** — `mvcc_store.h` and `raft_mvcc_bridge.h`
   provide snapshot-isolated reads and linearisable writes without exposing
   RocksDB column-family details.
4. **Security by Default** — `security_signature.h` and
   `security_signature_manager.h` enforce content integrity; `storage_audit_logger.h`
   provides immutable access logs for compliance.
5. **Observability & Operability** — `disk_space_monitor.h`,
   `storage_audit_logger.h`, and compaction progress events enable
   production monitoring without internal coupling.

---

## Interface Inventory

| Header | Primary Classes / Interfaces | Purpose |
|--------|------------------------------|---------|
| `adaptive_compaction.h` | `AdaptiveCompaction` | Workload-adaptive LSM compaction policy |
| `backup_manager.h` | `BackupManager` | Full and incremental backup coordination |
| `base_entity.h` | `BaseEntity` | Base class for all storable entities |
| `batch_write_optimizer.h` | `BatchWriteOptimizer` | Coalescing and reordering for batch writes |
| `blob_backend_filesystem.h` | `BlobBackendFilesystem` | Local filesystem blob backend |
| `blob_backend_gcs.h` | `BlobBackendGcs` | Google Cloud Storage blob backend |
| `blob_redundancy_manager.h` | `BlobRedundancyManager` | Cross-backend blob redundancy |
| `blob_storage_backend.h` | `IBlobStorageBackend` | Abstract blob backend interface |
| `blob_storage_manager.h` | `BlobStorageManager` | High-level blob lifecycle manager |
| `columnar_format.h` | `ColumnarFormat` | Columnar data layout for analytics reads |
| `compaction_manager.h` | `CompactionManager` | LSM compaction scheduling and execution |
| `compressed_storage.h` | `CompressedStorage` | Transparent compressed storage layer |
| `compression_strategy.h` | `ICompressionStrategy` | Pluggable compression algorithm interface |
| `database_connection_manager.h` | `DatabaseConnectionManager` | Connection pool for storage engine |
| `disk_space_monitor.h` | `DiskSpaceMonitor` | Real-time disk usage monitoring |
| `distributed_transaction_manager.h` | `DistributedTransactionManager` | Distributed ACID transaction coordinator |
| `erasure_coding_backend.h` | `ErasureCodingBackend` | Erasure-coded blob storage backend |
| `gpu_compression.h` | `GpuCompression` | GPU-accelerated compression interface |
| `history_manager.h` | `HistoryManager` | Row-version history for time-travel queries |
| `hlc.h` | `HybridLogicalClock` | Hybrid logical clock for causality tracking |
| `index_maintenance.h` | `IndexMaintenance` | Asynchronous secondary index upkeep |
| `key_schema.h` | `KeySchema` | Multi-model key encoding schemas |
| `merge_operators.h` | `IMergeOperator` | RocksDB merge operator interface |
| `mvcc_store.h` | `MvccStore` | MVCC snapshot-isolated key-value store |
| `nlp_metadata_extractor.h` | `NlpMetadataExtractor` | NLP-based metadata extraction from blobs |
| `nvme_manager.h` | `NvmeManager` | NVMe device lifecycle and io_uring integration |
| `online_schema_migration.h` | `OnlineSchemaMigration` | Zero-downtime schema migration |
| `pitr_manager.h` | `PitrManager` | Point-in-time recovery management |
| `raft_mvcc_bridge.h` | `RaftMvccBridge` | Bridge between Raft log and MVCC store |
| `rocksdb_wrapper.h` | `RocksDbWrapper` | Thin RocksDB abstraction (opaque internals) |
| `security_signature.h` | `SecuritySignature` | Content integrity signature |
| `security_signature_manager.h` | `SecuritySignatureManager` | Signature lifecycle management |
| `storage_audit_logger.h` | `StorageAuditLogger` | Immutable audit log for storage operations |
| `storage_engine.h` | `IStorageEngine` | Primary storage engine abstraction |
| `tiered_storage.h` | `TieredStorage` | Multi-tier data placement policy |
| `transaction_retry_manager.h` | `TransactionRetryManager` | Retry logic for conflicting transactions |
| `wal_storage.h` | `WalStorage` | WAL-backed durable write interface |
| `wom_tree.h` | `WomTree` | Write-optimised merge tree structure |
| `zero_copy_blob_transfer.h` | `ZeroCopyBlobTransfer` | Zero-copy blob send/receive via sendfile/splice |
| *(planned)* `schema_dead_weight_detector.h` | `SchemaDeadWeightDetector`, `DeadWeightReport` | Layer 6: 180-day rolling access analysis; identifies archivable fields/collections (IMPL-B6) |
| *(planned)* `storage_layout_advisor.h` | `StorageLayoutAdvisor`, `LayoutHint` | Layer 10: row/columnar/tiered layout recommendation per collection profile (IMPL-B10) |

> **Paper 2 additions (IMPL-B6, IMPL-B10):**
> Both advisors write `DecisionRecord` to `AIDecisionAuditor`. GDPR-tagged fields are always retained by `SchemaDeadWeightDetector`. `StorageLayoutAdvisor` feeds into the `distributed_knowledge` Layer 11C merge path.

---

## Module Relationships

```
storage_engine.h (IStorageEngine)
  ├── rocksdb_wrapper.h → mvcc_store.h → raft_mvcc_bridge.h
  ├── blob_storage_manager.h
  │     ├── blob_storage_backend.h (interface)
  │     │     ├── blob_backend_filesystem.h
  │     │     ├── blob_backend_gcs.h
  │     │     └── erasure_coding_backend.h
  │     └── blob_redundancy_manager.h
  ├── compaction_manager.h → adaptive_compaction.h
  ├── tiered_storage.h → nvme_manager.h
  ├── backup_manager.h → pitr_manager.h
  ├── security_signature_manager.h → security_signature.h
  └── storage_audit_logger.h
```

---

## Implementation Reference

See [`../../src/storage/`](../../src/storage/) for all `.cpp` implementations.
See [`../../docs/src/storage/`](../../docs/src/storage/) for detailed design documentation.
