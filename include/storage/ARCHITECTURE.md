> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/storage/ARCHITECTURE.md -->

# Storage Module — Public Header Architecture

**Module Path:** `include/storage/`
**Implementation:** `../../src/storage/`
**Canonical architecture doc:** [`../../src/storage/ARCHITECTURE.md`](../../src/storage/ARCHITECTURE.md)

---

## 1. Overview

`include/storage/` contains the **public C++ contract** for ThemisDB's storage layer. Headers define engine interfaces, backend abstractions, MVCC primitives, blob/columnar stores, compaction policies, and WAL management that are consumed by query, transaction, replication, and tensor subsystems.

For full architectural details — RocksDB integration, WAL structure, MVCC chain layout, tensor compaction pipeline — see:
→ [`../../src/storage/ARCHITECTURE.md`](../../src/storage/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Engine

| Header | Public Type | Purpose |
|--------|------------|---------|
| `storage_engine.h` | `IStorageEngine` | Primary read/write/scan engine interface |
| `mvcc_store.h` | `MVCCStore`, `MVCCVersion` | MVCC key-value primitives |
| `mvcc_chain_pruner.h` | `MVCCChainPruner` | Background version garbage collection |
| `wal_storage.h` | `WALStorage` | Write-ahead log interface |
| `rocksdb_wrapper.h` | `RocksDBWrapper` | RocksDB engine adapter |
| `database_connection_manager.h` | `DatabaseConnectionManager` | Connection pool and lifecycle |

### 2.2 Compaction and Maintenance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `compaction_manager.h` | `CompactionManager` | RocksDB compaction coordination |
| `adaptive_compaction.h` | `AdaptiveCompactionPolicy` | Workload-adaptive compaction tuning |
| `tensor_compaction_filter.h` | `TensorCompactionFilter` | Compaction filter for tensor-encoded values |
| `index_maintenance.h` | `IndexMaintenanceScheduler` | Background index maintenance |
| `index_analyzer.h` | `IndexAnalyzer` | Index health and utilization analysis |

### 2.3 Blob and Columnar Storage

| Header | Public Type | Purpose |
|--------|------------|---------|
| `blob_storage_backend.h` | `IBlobStorageBackend` | Abstract blob backend interface |
| `blob_storage_manager.h` | `BlobStorageManager` | Blob lifecycle and routing |
| `blob_backend_filesystem.h` | `FilesystemBlobBackend` | Local filesystem blob backend |
| `blob_backend_gcs.h` | `GCSBlobBackend` | Google Cloud Storage backend |
| `blob_redundancy_manager.h` | `BlobRedundancyManager` | Redundancy and erasure coding coordination |
| `encrypted_blob_backend.h` | `EncryptedBlobBackend` | AES-GCM encrypted blob wrapper |
| `zero_copy_blob_transfer.h` | `ZeroCopyBlobTransfer` | DMA/sendfile-based zero-copy path |
| `erasure_coding_backend.h` | `ErasureCodingBackend` | Reed-Solomon erasure coding backend |
| `columnar_format.h` | `ColumnarPage`, `ColumnarBlock` | Columnar page layout for analytics scans |
| `columnar_cache.h` | `ColumnarCache` | Columnar page LRU cache |
| `storage_parquet_exporter.h` | `StorageParquetExporter` | Parquet-format export from columnar store |

### 2.4 Compression and Encoding

| Header | Public Type | Purpose |
|--------|------------|---------|
| `compression_strategy.h` | `ICompressionStrategy` | Pluggable compression backend |
| `compressed_storage.h` | `CompressedStorage` | Transparent compression layer |
| `codec_tags.h` | `CodecTag` | Enum of supported codecs (Snappy, Zstd, LZ4, Brotli) |
| `gpu_compression.h` | `GPUCompressionEngine` | CUDA-accelerated compression path |

### 2.5 Tensor and GGML Integration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gguf_metadata.h` | `GGUFMetadata` | GGUF model-file metadata parsing |
| `ggml_tensor_bridge.h` | `GGMLTensorBridge` | GGML tensor ↔ storage bridge |
| `tensor_network_storage_engine.h` | `TensorNetworkStorageEngine` | Tensor-network-aware storage engine |
| `tensor_compaction_filter.h` | `TensorCompactionFilter` | Compaction filter for tensor payloads |
| `tensor_router.h` | `TensorRouter` | Routes tensor reads to hot/cold tiers |
| `tensor_train_decomposer.h` | `TensorTrainDecomposer` | TT-decomposition for compressed tensor storage |
| `hierarchical_tucker_decomposer.h` | `HierarchicalTuckerDecomposer` | HT-decomposition interface |
| `tt_quantizer.h` | `TTQuantizer` | TT-format quantization utilities |

### 2.6 Schema, Migration, and Lifecycle

| Header | Public Type | Purpose |
|--------|------------|---------|
| `online_schema_migration.h` | `OnlineSchemaMigration` | Zero-downtime schema migration |
| `schema_dead_weight_detector.h` | `SchemaDeadWeightDetector` | Detects obsolete columns and index dead weight |
| `pitr_manager.h` | `PITRManager` | Point-in-time recovery manager |
| `backup_manager.h` | `BackupManager` | Incremental backup and restore |
| `batch_write_optimizer.h` | `BatchWriteOptimizer` | Batched write path optimization |
| `concurrent_write_controller.h` | `ConcurrentWriteController` | Write concurrency limiter |
| `streaming_ingest_manager.h` | `StreamingIngestManager` | High-throughput streaming ingestion |

### 2.7 Metadata and Key Layout

| Header | Public Type | Purpose |
|--------|------------|---------|
| `key_schema.h` | `KeySchema`, `KeyEncoder` | Composite key encoding layout |
| `merge_operators.h` | `IMergeOperator` | RocksDB merge operator interfaces |
| `base_entity.h` | `BaseEntity` | Base type for all stored entities |
| `history_manager.h` | `HistoryManager` | Temporal history chain management |
| `hlc.h` | `HybridLogicalClock` | HLC for distributed timestamp ordering |
| `nlp_metadata_extractor.h` | `NLPMetadataExtractor` | NLP-derived metadata tagging at ingest |

### 2.8 Auxiliary Interfaces

| Header | Public Type | Purpose |
|--------|------------|---------|
| `distributed_transaction_manager.h` | `IDistributedTransactionManager` | Cross-shard transaction interface (storage view) |
| `raft_mvcc_bridge.h` | `RaftMVCCBridge` | Raft log ↔ MVCC commit bridge |
| `vector_index_backend.h` | `IVectorIndexBackend` | Pluggable ANN index backend |
| `disk_space_monitor.h` | `DiskSpaceMonitor` | Disk quota and pressure alerts |
| `nvme_manager.h` | `NVMeManager` | NVMe device lifecycle management |
| `tiered_storage.h` | `TieredStorageManager` | Hot/warm/cold tier policy enforcement |
| `storage_layout_advisor.h` | `StorageLayoutAdvisor` | Layout recommendations for workload patterns |
| `storage_audit_logger.h` | `StorageAuditLogger` | Per-operation audit trail |
| `security_signature.h` | `StorageSecuritySignature` | HMAC signing for stored blocks |
| `security_signature_manager.h` | `SecuritySignatureManager` | Key rotation and verification |
| `simd_filter.h` | `SIMDFilter` | AVX2/NEON-accelerated predicate filtering |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::storage` | All core engine and backend types |
| `themis::storage::blob` | Blob backend hierarchy |
| `themis::storage::tensor` | Tensor-native storage engine types |
| `themis::storage::compression` | Compression strategy hierarchy |

---

## 4. Relationship to Strategic Architecture

- **Graph Truth Layer**: `mvcc_store.h`, `rocksdb_wrapper.h`, `raft_mvcc_bridge.h` back the graph truth store
- **Tensor Mid-Layer**: `tensor_network_storage_engine.h`, `tensor_router.h`, `ggml_tensor_bridge.h` implement the tensor storage tier
- **ANN Frontdoor**: `vector_index_backend.h` provides the pluggable ANN backend used by `include/index/`
- **LLM/LoRA Final Layer**: `gguf_metadata.h`, `ggml_tensor_bridge.h` support model loading from the storage layer
