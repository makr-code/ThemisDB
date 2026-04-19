<!-- Status: current | validated: 2026-04-19 -->

# include/storage/ — Audit Report

| Field | Value |
|-------|-------|
| **Last Audit Date** | 2026-04-19 |
| **Auditor** | ThemisDB Core Team |
| **Audit Type** | Public Header Surface Review |
| **Status** | ✅ Pass |
| **Total Headers** | 49 |
| **Issues Found** | 0 critical · 0 high · 2 informational |

---

## Summary

| Category | Count |
|----------|-------|
| Core storage engine headers | 4 |
| Blob storage headers | 6 |
| Compaction & tiering headers | 4 |
| MVCC & transaction headers | 6 |
| Compression headers | 3 |
| Backup & recovery headers | 3 |
| Security & audit headers | 3 |
| Index & schema headers | 4 |
| Monitoring & utility headers | 6 |

---

## Header Files Audited

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `adaptive_compaction.h` | `AdaptiveCompaction` | ✅ Clean API |
| `backup_manager.h` | `BackupManager` | ✅ Clean API |
| `base_entity.h` | `BaseEntity` | ✅ Clean API |
| `batch_write_optimizer.h` | `BatchWriteOptimizer` | ✅ Clean API |
| `blob_backend_filesystem.h` | `BlobBackendFilesystem` | ✅ Clean API |
| `blob_backend_gcs.h` | `BlobBackendGcs` | ℹ️ Requires GCS SDK headers at build time |
| `blob_redundancy_manager.h` | `BlobRedundancyManager` | ✅ Clean API |
| `blob_storage_backend.h` | `IBlobStorageBackend` | ✅ Pure-virtual contract verified |
| `blob_storage_manager.h` | `BlobStorageManager` | ✅ Clean API |
| `columnar_format.h` | `ColumnarFormat` | ✅ Clean API |
| `compaction_manager.h` | `CompactionManager` | ✅ Clean API |
| `compressed_storage.h` | `CompressedStorage` | ✅ Clean API |
| `compression_strategy.h` | `ICompressionStrategy` | ✅ Pure-virtual contract verified |
| `database_connection_manager.h` | `DatabaseConnectionManager` | ✅ Clean API |
| `disk_space_monitor.h` | `DiskSpaceMonitor` | ✅ Clean API |
| `distributed_transaction_manager.h` | `DistributedTransactionManager` | ✅ Clean API |
| `erasure_coding_backend.h` | `ErasureCodingBackend` | ✅ Clean API |
| `gpu_compression.h` | `GpuCompression` | ℹ️ CUDA path conditionally compiled |
| `history_manager.h` | `HistoryManager` | ✅ Clean API |
| `hlc.h` | `HybridLogicalClock` | ✅ Clean API |
| `index_maintenance.h` | `IndexMaintenance` | ✅ Clean API |
| `key_schema.h` | `KeySchema` | ✅ Clean API |
| `merge_operators.h` | `IMergeOperator` | ✅ Pure-virtual contract verified |
| `mvcc_store.h` | `MvccStore` | ✅ Clean API |
| `nlp_metadata_extractor.h` | `NlpMetadataExtractor` | ✅ Clean API |
| `nvme_manager.h` | `NvmeManager` | ✅ Clean API |
| `online_schema_migration.h` | `OnlineSchemaMigration` | ✅ Clean API |
| `pitr_manager.h` | `PitrManager` | ✅ Clean API |
| `raft_mvcc_bridge.h` | `RaftMvccBridge` | ✅ Clean API |
| `rocksdb_wrapper.h` | `RocksDbWrapper` | ✅ RocksDB types not leaked into public API |
| `security_signature.h` | `SecuritySignature` | ✅ Clean API |
| `security_signature_manager.h` | `SecuritySignatureManager` | ✅ Clean API |
| `storage_audit_logger.h` | `StorageAuditLogger` | ✅ Clean API |
| `storage_engine.h` | `IStorageEngine` | ✅ Pure-virtual contract verified |
| `tiered_storage.h` | `TieredStorage` | ✅ Clean API |
| `transaction_retry_manager.h` | `TransactionRetryManager` | ✅ Clean API |
| `wal_storage.h` | `WalStorage` | ✅ Clean API |
| `wom_tree.h` | `WomTree` | ✅ Clean API |
| `zero_copy_blob_transfer.h` | `ZeroCopyBlobTransfer` | ✅ Clean API |
| `columnar_cache.h` | `ColumnarCache` | ✅ Clean API |
| `concurrent_write_controller.h` | `ConcurrentWriteController` | ✅ Clean API |
| `encrypted_blob_backend.h` | `EncryptedBlobBackend` | ✅ Clean API |
| `mvcc_chain_pruner.h` | `MvccChainPruner` | ✅ Clean API |
| `schema_dead_weight_detector.h` | `SchemaDeadWeightDetector` | ✅ Clean API |
| `simd_filter.h` | `SimdFilter` | ✅ Clean API |
| `storage_layout_advisor.h` | `StorageLayoutAdvisor` | ✅ Clean API |
| `storage_parquet_exporter.h` | `StorageParquetExporter` | ✅ Clean API |
| `streaming_ingest_manager.h` | `StreamingIngestManager` | ✅ Clean API |
| `vector_index_backend.h` | `VectorIndexBackend` | ✅ Clean API |

---

## Findings

### ℹ️ INFO-001 — GCS SDK Dependency in `blob_backend_gcs.h`

`blob_backend_gcs.h` transitively requires the Google Cloud Storage C++ SDK
headers at compile time.  Consumers not using GCS blobs should guard inclusion
with `#ifdef THEMIS_ENABLE_GCS`.  This is documented in the header and causes
no issue for standard builds.  No action required.

### ℹ️ INFO-002 — Conditional CUDA Compilation in `gpu_compression.h`

`gpu_compression.h` uses `#ifdef THEMIS_CUDA_ENABLED` guards.  Non-GPU builds
fall back to the CPU `ICompressionStrategy` path transparently.  No action required.

---

## Next Audit

Scheduled: **2026-09-22**
