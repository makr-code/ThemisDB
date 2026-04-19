> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/storage/README.md -->

# Storage Module — Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-04-06
**Module Path:** `src/storage/`

---

## 1. Overview

The Storage module is ThemisDB's persistent data layer, built on RocksDB TransactionDB for
high-performance LSM-tree storage with MVCC. It provides a unified key schema spanning
all five data models (relational, document, graph, vector, timeseries), large object
storage (BlobDB + cloud backends), backup/PITR, compression strategies, columnar formats,
field-level encryption, and blob redundancy management.

---

## 2. Design Principles

- **Single RocksDB Instance** – all data models share one RocksDB instance with
  model-prefixed keys, enabling atomic cross-model writes via WriteBatch.
- **MVCC** – optimistic concurrency via RocksDB TransactionDB; readers never block writers.
- **BlobDB** – large values (> threshold) are offloaded to BlobDB automatically to keep
  the LSM-tree compact.
- **Key Schema** – a well-defined prefix hierarchy separates models, collections, and
  secondary indexes within the same keyspace.
- **Dependency Injection** – `storage_engine.h` defines an abstract interface; the
  RocksDB wrapper is one implementation, enabling testing with in-memory stubs.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `rocksdb_wrapper.cpp` | Primary RocksDB wrapper: MVCC, WAL, BlobDB, async I/O |
| `mvcc_store.cpp` | MVCC snapshot management and version chain |
| `key_schema.cpp` | Unified key encoding for all data models |
| `wal_storage.cpp` | Write-Ahead Log management and replay |
| `backup_manager.cpp` | RocksDB backup API and PITR |
| `pitr_manager.cpp` | Point-in-time recovery via WAL replay and snapshot restore |
| `storage_engine.cpp` | High-level storage abstraction with dependency injection |
| `base_entity.cpp` | Base document serialization/deserialization |
| `batch_write_optimizer.cpp` | WriteBatch coalescing for throughput |
| `compaction_manager.cpp` | Manual compaction triggering and tuning |
| `index_maintenance.cpp` | Secondary index consistency on writes |
| `history_manager.cpp` | Version history for document snapshots |
| `hlc.cpp` | Hybrid Logical Clock for distributed timestamps |
| `merge_operators.cpp` | RocksDB merge operators for counter/set semantics |
| `compressed_storage.cpp` | Compressed column families |
| `compression_strategy.cpp` | Adaptive compression (zstd, lz4, snappy) |
| `columnar_format.cpp` | Columnar storage layout for analytics workloads |
| `blob_backend_s3.cpp` | S3 blob backend |
| `blob_backend_azure.cpp` | Azure Blob Storage backend |
| `blob_backend_filesystem.cpp` | Local filesystem blob backend |
| `blob_backend_webdav.cpp` | WebDAV blob backend |
| `blob_backend_gcs.cpp` | Google Cloud Storage backend (requires `THEMIS_ENABLE_GCS`) |
| `blob_redundancy_manager.cpp` | RAID-1 mirror redundancy across multiple backends |
| `database_connection_manager.cpp` | Connection lifecycle management |
| `disk_space_monitor.cpp` | Disk space monitoring and write throttling |
| `nlp_metadata_extractor.cpp` | NLP metadata extraction on ingest |
| `security_signature.cpp` | Field-level AES-256-GCM encryption primitives |
| `security_signature_manager.cpp` | HMAC-SHA256 tamper detection and signature management |
| `storage_audit_logger.cpp` | Structured audit trail for all storage operations |
| `tiered_storage.cpp` | Hot/warm/cold tiered storage with automatic migration |
| `transaction_retry_manager.cpp` | Exponential backoff retry for failed transactions |
| `raft_mvcc_bridge.cpp` | Raft consensus log to MVCC storage integration |
| `adaptive_compaction.cpp` | Load-aware compaction scheduling via `AdaptiveCompactionScheduler` |
| `columnar_cache.cpp` | LRU in-memory columnar segment cache (`ColumnarCache`, `namespace themis::storage`) |
| `concurrent_write_controller.cpp` | Bounded FIFO write-concurrency semaphore (`ConcurrentWriteController`, `namespace themis::storage`) |
| `distributed_transaction_manager.cpp` | 2PC coordinator: `DistributedTransactionManager`, `IDistributedShardParticipant`, `ManagerSharedState` |
| `encrypted_blob_backend.cpp` | AES-256-GCM blob backend wrapper (`EncryptedBlobBackend`, `IEncryptionKeyProvider`) |
| `erasure_coder_factory.cpp` | Factory for Reed-Solomon erasure codec selection |
| `erasure_coding_backend.cpp` | Reed-Solomon erasure coding backend (`ErasureCodingBackend`, `namespace themisdb::storage`) |
| `gpu_compression.cpp` | GPU-accelerated compression via nvCOMP/ROCm with CPU fallback (`GpuCompressionManager`) |
| `mvcc_chain_pruner.cpp` | Background MVCC version-chain pruning (`MVCCChainPruner`) |
| `nvme_manager.cpp` | NVMe device management: io_uring, multi-queue, ZNS, Direct I/O (`NVMeManager`) |
| `online_schema_migration.cpp` | Zero-downtime DDL via `SchemaMigrator` (add/drop/rename column, index, partition) |
| `schema_dead_weight_detector.cpp` | Unused field and stale index detection; `GdprFieldRegistry` for PII tracking |
| `simd_filter.cpp` | AVX-512/AVX2/NEON/scalar SIMD predicate filter over columnar data (`SIMDColumnFilter`) |
| `storage_layout_advisor.cpp` | Layout recommendations (row/columnar/tiered/vector) from `CollectionAccessStats` |
| `storage_parquet_exporter.cpp` | Native Apache Parquet v2 export from columnar format (`StorageParquetExporter`) |
| `streaming_ingest_manager.cpp` | Ring-buffer streaming ingest with flush thread (`StreamingIngestManager`) |
| `vector_index_backend.cpp` | `IVectorIndexBackend` interface + `InMemoryVectorIndex` implementation |
| `wom_tree.cpp` | Write-Optimized Merge (Bε) Tree for write-heavy workloads (`WomTree`) |
| `zero_copy_blob_transfer.cpp` | Zero-copy blob transfer via mmap and sendfile (`ZeroCopyBlobTransfer`) |

### 3.2 Key Schema

```
Relational:    rel:{table}:{pk}
Document:      doc:{collection}:{pk}
Graph Node:    node:{pk}
Graph Edge:    edge:{from_id}:{type}:{to_id}
Vector:        vec:{index_name}:{pk}
Timeseries:    ts:{series}:{timestamp}:{pk}

Secondary idx: idx:{table}:{field}:{value}:{pk}
Graph idx:     gidx:{from_id}:{type}:{to_id}
```

### 3.3 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│     Higher-Layer Callers (query, index, graph, storage)         │
│   db.get("doc:users:123") / db.put(key, value) / db.scan(range) │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   RocksDBWrapper                                 │
│                                                                  │
│  MVCC (TransactionDB) │ WAL │ BlobDB │ Async I/O │ Compression  │
│  Block Cache │ Bloom Filter │ SSTable multi-path │ Level-tuning  │
└──────────────────────────┬──────────────────────────────────────┘
                           │
         ┌─────────────────┴───────────────────────────┐
         │                                             │
  ┌──────▼──────────────┐                  ┌──────────▼────────────┐
  │  RocksDB (LSM tree)  │                  │  BlobDB / Cloud Blob  │
  │  small values (≤4KB) │                  │  large values (>4KB)  │
  │  SSTable files       │                  │  S3 / Azure / FS     │
  └─────────────────────┘                  └───────────────────────┘
```

---

## 4. Data Flow

### 4.1 Document Write

```
Write: collection="users", key="user:123", value={name:"Alice"}
    │
    ├─ key_schema.encode("doc", "users", "user:123") → full RocksDB key
    ├─ field_encryption (if enabled): encrypt sensitive fields
    ├─ batch_write_optimizer: add to current WriteBatch
    ├─ WAL: append WAL entry
    ├─ value.size > threshold? → BlobDB → write to blob file
    └─ RocksDB: memtable → L0 SSTable → compaction
```

### 4.2 Range Scan with MVCC

```
Scan: prefix="doc:orders:", snapshot_id=42
    │
    ├─ mvcc_store: acquire read snapshot 42
    ├─ RocksDB iterator with snapshot: skip versions > 42
    ├─ async_io: prefetch next block (readahead)
    └─ yield documents in key order
```

### 4.3 Backup

```
backup_manager.createBackup(destination="/backup/2026-02-24")
    │
    ├─ RocksDB BackupEngine: hard-link SSTable files (no copy)
    ├─ Copy WAL files
    └─ Write backup manifest (can restore to this PITR timestamp)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Used by** | `src/query/` | Document scans and point reads |
| **Used by** | `src/index/` | Index updates via WriteBatch |
| **Used by** | `src/transaction/` | MVCC transaction management |
| **Used by** | `src/replication/` | WAL shipping |
| **Used by** | `src/cdc/` | Post-commit write hooks |
| **Used by** | `src/security/` | Field encryption |

---

## 6. Threading & Concurrency Model

- Concurrent reads are lock-free (MVCC snapshots).
- WriteBatch operations use RocksDB's internal write mutex.
- `batch_write_optimizer` coalesces writes from multiple threads into single batches.
- `compaction_manager` runs background compaction threads (configurable count).
- `disk_space_monitor` runs a background polling thread.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Block cache | LRU block cache (configurable size, default 1 GB) |
| Bloom filters | Per-SSTable bloom filters reduce false-positive I/O |
| BlobDB | Large values separated from LSM-tree (reduces compaction I/O) |
| Async I/O | Readahead prefetch: 2-5× scan throughput improvement |
| CPU prefetch | Software prefetch hints for random access patterns |
| Multi-path SSTable | Distribute SSTable files across multiple NVMe drives |

| Metric | Range |
|---|---|
| Point reads | 10–50 μs (cache hit), 100–500 μs (cache miss) |
| Sequential scans | 100K–500K keys/sec |
| Writes | 50K–200K ops/sec (with WAL) |
| Write amplification | 10–30× (LSM characteristic) |

---

## 8. Security Considerations

- Field-level encryption is applied before writing to RocksDB (plaintext never on disk).
- WAL files are written to a separate directory; they should be on an encrypted volume.
- Backup files inherit the encryption of the source data files.
- `disk_space_monitor` triggers write throttling before disk full to prevent data loss.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `storage.rocksdb.path` | "./data" | RocksDB data directory |
| `storage.rocksdb.wal_dir` | "./data/wal" | WAL directory |
| `storage.rocksdb.memtable_size_mb` | 512 | Memtable buffer size |
| `storage.rocksdb.block_cache_mb` | 1024 | Block cache size |
| `storage.rocksdb.blobdb.enabled` | true | Enable BlobDB |
| `storage.rocksdb.blobdb.threshold_bytes` | 4096 | BlobDB offload threshold |
| `storage.rocksdb.compaction_threads` | 4 | Background compaction threads |
| `storage.rocksdb.async_io` | true | Enable async I/O prefetch |
| `storage.backup.path` | "" | Backup destination |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| RocksDB write error | Return error to caller; WAL ensures no silent data loss |
| Disk full | `disk_space_monitor` throttles writes; return 507 before full |
| Compaction failure | Log error; continue (data safe in WAL); alert operator |
| Corruption detected | RocksDB checksum check → halt and alert (do not serve corrupt data) |
| Blob backend unreachable | Fail write; return error (fail closed) |

---

## 11. Known Limitations & Future Work

- Column family per data model (for independent compaction tuning) is not yet implemented.
- Columnar format is production-ready for analytics but not yet used as the primary store for all workloads.
- `WomTree` is beta; it is not a drop-in replacement for RocksDB in all access patterns.
- `MVCCChainPruner` and `SchemaDeadWeightDetector` are experimental (not yet wired into the default storage engine pipeline).
- `InMemoryVectorIndex` is a flat-scan fallback; an HNSW/IVF persistent backend is planned.

---

## 12. References

- `src/storage/README.md` — module overview
- `docs/storage/` — storage documentation
- `docs/ROCKSDB_CONFIGURATION.md` — RocksDB tuning guide
- `ARCHITECTURE.md` (root) — full system architecture
