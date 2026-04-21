> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Storage Module

**Last Audit:** 2026-04-21 | **Auditor:** Copilot | **Status:** 🔴 Critical — 1 S0 use-after-free in `close()`

> **Note:** Previous audit claimed "Security Issues: None critical". Source code analysis found
> a TOCTOU race in `RocksDBWrapper::close()` that causes use-after-free under concurrent load,
> a blob atomicity gap (partial blob visible to snapshot readers), and a non-durable write
> default. Header quality scores do not reflect actual correctness.

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified (`cmake/CMakeLists.txt`, `cmake/StorageEnhancements.cmake`, `cmake/BlobStorage.cmake`) |
| Source Files | 51 (`.cpp` in `src/storage/`) |
| Test Coverage | ✅ 21 focused standalone test targets |
| S0 Critical / Safety Violations | 🔴 1 (use-after-free in `close()`) |
| S1 High | ⚠️ 1 (partial blob visibility) |
| S2 Medium | ⚠️ 5 |
| S3 Low | ℹ️ 2 |
| Durability-by-default | 🔴 `write_options_->sync = false` — power-loss loses acknowledged writes |

## Source Files Audited

- `adaptive_compaction.cpp` — Adaptive RocksDB compaction strategy
- `backup_manager.cpp` — Backup creation and scheduling
- `base_entity.cpp` — Base entity model with common storage traits
- `batch_write_optimizer.cpp` — Batched write optimization for RocksDB
- `blob_backend_azure.cpp` — Azure Blob Storage backend
- `blob_backend_filesystem.cpp` — Local filesystem blob backend
- `blob_backend_gcs.cpp` — Google Cloud Storage backend
- `blob_backend_s3.cpp` — AWS S3 blob backend with multipart upload
- `blob_backend_webdav.cpp` — WebDAV blob backend
- `blob_redundancy_manager.cpp` — RAID-1 mirror redundancy for blob storage
- `columnar_cache.cpp` — Columnar data cache for analytical workloads
- `columnar_format.cpp` — Columnar storage format encoding and decoding
- `compaction_manager.cpp` — RocksDB compaction lifecycle management
- `compressed_storage.cpp` — Transparent compression layer for storage engine
- `compression_strategy.cpp` — Pluggable compression strategy (LZ4, Zstd, Snappy)
- `concurrent_write_controller.cpp` — Concurrency control for parallel write paths
- `database_connection_manager.cpp` — Database connection pool and lifecycle manager
- `disk_space_monitor.cpp` — Disk space monitoring with threshold alerts
- `distributed_transaction_manager.cpp` — Distributed 2PC transaction coordinator
- `encrypted_blob_backend.cpp` — AES-256-GCM encrypted blob backend wrapper
- `erasure_coder_factory.cpp` — Factory for erasure coding codec selection
- `erasure_coding_backend.cpp` — Reed-Solomon erasure coding backend
- `gpu_compression.cpp` — GPU-accelerated compression for large blobs
- `history_manager.cpp` — Document version history management
- `hlc.cpp` — Hybrid Logical Clock for distributed timestamp ordering
- `index_maintenance.cpp` — Background index maintenance and compaction
- `key_schema.cpp` — Unified multi-model key encoding
- `merge_operators.cpp` — RocksDB merge operators for CRDT-style updates
- `mvcc_chain_pruner.cpp` — MVCC version chain pruning for old snapshot cleanup
- `mvcc_store.cpp` — Multi-version concurrency control
- `nlp_metadata_extractor.cpp` — NLP-based metadata extraction from stored documents
- `nvme_manager.cpp` — NVMe device management and direct I/O
- `online_schema_migration.cpp` — Zero-downtime online schema migration
- `pitr_manager.cpp` — Point-in-time recovery management
- `raft_mvcc_bridge.cpp` — Bridge between Raft consensus and MVCC storage
- `rocksdb_wrapper.cpp` — Primary RocksDB wrapper: MVCC, WAL, BlobDB
- `schema_dead_weight_detector.cpp` — Detects unused schema fields and stale indexes
- `security_signature.cpp` — Encryption and tamper detection for stored data
- `security_signature_manager.cpp` — Key management for storage security signatures
- `simd_filter.cpp` — SIMD-accelerated predicate filtering over columnar data
- `storage_audit_logger.cpp` — Structured audit trail for all storage operations
- `storage_engine.cpp` — High-level storage abstraction with dependency injection
- `storage_layout_advisor.cpp` — Recommends optimal storage layout based on access patterns
- `storage_parquet_exporter.cpp` — Apache Parquet export for analytics offload
- `streaming_ingest_manager.cpp` — Manages streaming write pipelines into storage
- `tiered_storage.cpp` — Hot/warm/cold tiered storage migration
- `transaction_retry_manager.cpp` — Exponential backoff retry for optimistic transactions
- `vector_index_backend.cpp` — Vector index storage backend (HNSW, IVF, Flat)
- `wal_storage.cpp` — Write-ahead log management
- `wom_tree.cpp` — Write-optimized merge tree implementation
- `zero_copy_blob_transfer.cpp` — Zero-copy blob transfer between storage tiers

## Test Coverage

21 focused standalone test targets including: `StorageEngineDI`, `StorageEngineProd`, `StorageAuditLogger`, `BlobStorage`, `TieredStorage`, `WalStorage`, `WalManager`, `MvccStore`, `MvccHistory`, `MvccWalIntegration`, and more.

## Findings

### S0 — Critical

#### R-1 · `rocksdb_wrapper.cpp` · `close()` — TOCTOU race: use-after-free under concurrent operations

`close()` acquires and releases `db_lifecycle_mutex_` in a scoped block to set a shutdown flag,
then busy-waits on `active_operations_`. But after the scoped lock is released, new
`OperationGuard`s can still increment `active_operations_`. On a constantly-loaded DB the
busy-wait never converges. More critically, `db_.reset()` (called unconditionally after the
loop) races with operations that started after the mutex was released. Any iterator,
transaction, or `OperationGuard` holding a reference to the now-deleted DB object causes
**use-after-free / undefined behavior**.

```cpp
{
    std::lock_guard<std::mutex> lock(db_lifecycle_mutex_);
    // ... sets shutdown flag
}
// ← lock released here; new OperationGuards can still start
while (active_operations_.load(std::memory_order_acquire) > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // may never exit
}
db_.reset();   // races with operations started after lock release
```

**Fix required:** Keep `db_lifecycle_mutex_` held (or use a shared/exclusive upgrade pattern)
for the entire duration of close; set a `closing_` atomic flag under the lock before releasing
so that `OperationGuard` constructors observe it and fail fast.

---

### S1 — High

#### R-2 · `rocksdb_wrapper.cpp` · `putBlob()` — Partial blob visible to concurrent snapshot readers

`putBlob()` builds a `WriteBatch` containing all chunk entries plus a manifest key, then
commits via `commitBatch()` which calls `db_->Write()` directly on the `TransactionDB`.
The `WriteBatch` becomes visible atomically at the LSM level **after the write**, but
concurrent snapshot-isolated reads (`GetForUpdate`, `GetWithSnapshot`) taken between chunk
writes and manifest commit can read chunks that belong to an incomplete multi-chunk blob.
A concurrent `getBlob()` assembling from those partial chunks constructs a corrupt object.

**Fix required:** Wrap the multi-chunk blob write in an explicit `rocksdb::Transaction`
(`db_->BeginTransaction(write_options_)`) and commit it as a single transaction, so MVCC
prevents mid-write snapshots from observing partial state.

---

### S2 — Medium

| ID | Function | Description |
|----|----------|-------------|
| R-3 | `open()` | `THEMIS_ENABLE_SHARDING` environment variable silently drops all non-default column families with no audit log and no authorization check |
| R-4 | `TransactionWrapper::~TransactionWrapper()` | Intentional `txn_.release()` (explicit leak) when DB closes while transaction is active; accumulates under rapid restart cycles |
| R-5 | `configureOptions()` | `write_options_->sync = false` default — power failure between `db_->Write()` returning OK and the next OS `fsync` loses acknowledged writes; default is silent |
| W-1 | `crc32_update()` in `wal_storage.cpp` | Static CRC table uses non-atomic `if (!initialized)` pattern — data race under concurrent `WALStorage` opens (UB per C++ memory model); fix: `std::call_once` or `constexpr` table |
| W-2 | `appendBatch()` in `wal_storage.cpp` | Segment rotation failure mid-batch leaves earlier segment entries durable; recovery of partial batch requires idempotent replay logic not verified to exist |

### S3 — Low

| ID | Function | Description |
|----|----------|-------------|
| W-3 | `checkpoint()` in `wal_storage.cpp` | Double mutex acquisition: `appendEntry()` and then `checkpoint()` re-acquire `mutex_`; short window allows other writers to append after the checkpoint marker |
| R-6 | `putBlob()` manifest | Manifest uses host-endian `memcpy` without explicit little-endian enforcement — non-portable across mixed-endian architectures |

---

### Resolved (from 2026-04-19 audit)
- Build system: all `src/storage/*.cpp` files verified registered (March 2026)
- 21 focused test targets added in `tests/CMakeLists.txt` (March 2026)
- Jitter value UB fixed in `TransactionRetryManager` (February 2026)
- `const_cast` UB removed from audit code (February 2026)
- `THEMIS_PRODUCTION_MODE` safety flag implemented (February 2026)
- BackupManager scheduling implemented (March 2026)
- Erasure coding (`RedundancyMode::PARITY`) in `BlobRedundancyManager` implemented via `ErasureCodingBackend` (RS(k,m)) (v1.7.0)
- Distributed 2PC transactions across shards implemented via `DistributedTransactionManager` (v1.7.0)

### Open (carried forward)
- Chaos/fault-injection tests for blob backend failover not yet complete (Target: v2.0.0)
- `InMemoryVectorIndex` is flat-scan only; persistent HNSW/IVF backend not yet implemented

## Compliance

- GDPR: Field-level encryption supports data minimisation; `StorageAuditLogger` provides audit trail for Article 30 records
- HIPAA: AES-256-GCM encryption at rest; audit logging for PHI access
- SOC 2: Complete audit trail; encrypted backups; PITR for data recovery
