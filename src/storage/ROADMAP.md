> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-04-06 | Branch: develop | Reality-check: see docs/de/storage/missing-implementations.md -->

# Storage Module Roadmap

## Current Status
v1.8.0 – Production-grade persistent storage layer built on RocksDB with MVCC, WAL, BlobDB, multi-model key schema, backup/PITR, compression, field-level encryption, columnar format support, NVMe optimizations, Reed-Solomon erasure coding, distributed 2PC transactions, and Write-Optimized Merge (WOM) Tree.

## Completed ✅
- [x] `RocksDBWrapper` – MVCC transactions, WAL, BlobDB, multi-path SSTables, async I/O
- [x] `MVCCStore` – multi-version concurrency control with snapshot isolation
- [x] `WALStorage` – write-ahead log management and replay (`wal_storage.cpp`)
- [x] `KeySchema` – unified multi-model key encoding (relational, document, graph, vector, timeseries)
- [x] `StorageEngine` – high-level abstraction with dependency injection (evaluator, encryption, key provider, index manager)
- [x] `BackupManager` – incremental and full backup with checksum verification
- [x] `BackupManager` – in-memory backup scheduling (`scheduleBackup`, `cancelScheduledBackup`, `listScheduledBackups`); cloud upload/restore routing via `THEMIS_ENABLE_S3/AZURE/GCS` compile-time flags (`uploadBackupToCloud`, `restoreFromCloud`)
- [x] `PITRManager` – point-in-time recovery via WAL replay and snapshot restore
- [x] Blob storage backends: INLINE, RocksDB BlobDB, Filesystem, S3, Azure Blob, WebDAV
- [x] `BlobRedundancyManager` – RAID-1 mirror across multiple backends
- [x] `BlobRedundancyManager` – Reed-Solomon erasure coding (`PARITY` mode via `ErasureCodingBackend`); configurable RS(k,m) with copy-then-delete shard migration
- [x] `BlobRedundancyManager` – `RocksDBBlobListener` (`createRocksDBListener()`) reacts to SST file deletions and marks affected locations unhealthy
- [x] `CompressionStrategy` – pluggable per-table compression (Snappy, Zstd, LZ4, Brotli, None)
- [x] `ColumnarFormat` – columnar storage for analytical workloads
- [x] `BatchWriteOptimizer` – adaptive batching to reduce write amplification
- [x] `SecuritySignature` + `SecuritySignatureManager` – field-level AES-GCM encryption and HMAC-SHA256 tamper detection; full RocksDB key-range iteration via `iterateRange` for `listAllSignatures()`
- [x] `StorageAuditLogger` – structured audit trail for all storage operations
- [x] `IndexMaintenance` – background index rebuild, optimize, and consistency checks
- [x] `CompactionManager` – manual and scheduled RocksDB compaction control
- [x] `DiskSpaceMonitor` – real-time disk quota monitoring and alerting
- [x] `DatabaseConnectionManager` – connection pooling and lifecycle management
- [x] `TransactionRetryManager` – exponential backoff retry for failed transactions
- [x] `MergeOperators` – custom RocksDB merge operators (counters, list appends)
- [x] `RaftMVCCBridge` – integration between Raft consensus log and MVCC storage
- [x] `HLC` (Hybrid Logical Clock) – causally consistent timestamps across nodes
- [x] `HistoryManager` – version history and change tracking per key
- [x] `NLPMetadataExtractor` – automatic metadata extraction for ingested documents
- [x] `CompressedStorage` – transparent compression/decompression layer
- [x] `BaseEntity` – common base type for all storage-layer entities
- [x] `RocksDBWrapper` – `getApproximateSize()` returns real on-disk SST file size via `rocksdb.total-sst-files-size` property with fallback to `GetApproximateSizes`
- [x] `DistributedTransactionManager` – storage-layer Two-Phase Commit (2PC) coordinator with `IDistributedShardParticipant`; `ManagerSharedState` shared ownership for safe concurrent shard lifecycle
- [x] `NVMeManager` – io_uring async I/O (Linux ≥ 5.1), multi-queue NVMe, ZNS zone management, Direct I/O flag recommendation; RocksDBWrapper NVMe integration (`enable_nvme_optimizations` config)
- [x] `WomTree` – Write-Optimized Merge (WOM) Tree: Bε-tree alternative to LSM; write amplification 2–5× vs 10–30× for LSM; lazy buffer propagation; full put/get/remove/scan/compact API
- [x] Production-mode safety flag (`THEMIS_PRODUCTION_MODE`) preventing no-op encryption defaults
- [x] `StreamingIngestManager` — ring-buffer + flush-thread → single `WriteBatch` (Issue: #4571) (2026-04-12)
  - `include/storage/streaming_ingest_manager.h` + `src/storage/streaming_ingest_manager.cpp`
  - `flush_interval`=10 ms, `max_buffer`=1 M events, `max_batch`=65 536; `OverflowPolicy::BLOCK/DROP`
  - API: `ingest(key, value)` / `ingestBatch()` / `flush()` / `stats()`
  - 10 focused tests (SM-01…SM-10) in `tests/test_streaming_ingest_manager.cpp`
- [x] `ColumnarCache` — LRU in-memory columnar cache for analytics acceleration (Issue: #4572) (2026-04-12)
  - `include/storage/columnar_cache.h` + `src/storage/columnar_cache.cpp`
  - LRU eviction + `PinGuard` RAII; `SegmentDType` (Int64/Double/String/Bool); `byteSize()` accounting
  - `on_evict` callback; hit/miss counters; default ctor delegates to `Config{}`
  - 12 focused tests (CC-01…CC-12) in `tests/test_columnar_cache.cpp`

- [x] `AdaptiveCompactionScheduler` – load-aware compaction scheduling with IO-sample history and impact prediction (`adaptive_compaction.h/.cpp`, `namespace themis`)
- [x] `MVCCChainPruner` – background MVCC version-chain pruning for old snapshot cleanup (`mvcc_chain_pruner.h/.cpp`)
- [x] `VectorIndexBackend` – `IVectorIndexBackend` interface + `InMemoryVectorIndex` flat-scan implementation (`vector_index_backend.h/.cpp`, `namespace themis::storage`)
- [x] `ZeroCopyBlobTransfer` – mmap + sendfile zero-copy blob transfer; `MmapBlobView` RAII view (`zero_copy_blob_transfer.h/.cpp`, `namespace themis::storage`)
- [x] `EncryptedBlobBackend` – AES-256-GCM encryption wrapper for any `IBlobStorageBackend`; `IEncryptionKeyProvider` interface + `StaticKeyProvider` (`encrypted_blob_backend.h/.cpp`, `namespace themis::storage`)
- [x] `OnlineSchemaMigration` – zero-downtime DDL via `SchemaMigrator`; supports add/drop columns, rename, type change, add/drop indexes, partition (`online_schema_migration.h/.cpp`, `namespace themis::storage`)
- [x] `SchemaDeadWeightDetector` – detects unused schema fields and stale indexes; `GdprFieldRegistry` for PII field tracking (`schema_dead_weight_detector.h/.cpp`, `namespace themis::storage`)
- [x] `StorageLayoutAdvisor` – recommends layout type (row vs columnar vs tiered vs vector) based on `CollectionAccessStats` and `SchemaInfo` (`storage_layout_advisor.h/.cpp`, `namespace themis::storage`)

## In Progress 🚧

*(No items currently in progress — all v1.x and v1.8.0 roadmap items are complete.)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Tiered storage (hot/warm/cold) with automatic data migration based on access patterns (Target: v1.6.0)
  - Inputs: RocksDB key-space with per-key last-access timestamps tracked by `AccessTracker`
  - Outputs: transparent reads/writes regardless of tier; background `TierMigrationWorker` moves data between NVMe (hot), SATA (warm), and object storage (cold)
  - Affected files: new `tiered_storage.cpp`, extend `StorageEngine`, `DiskSpaceMonitor`
  - Migration policy: age-based (hot→warm after 30 days, warm→cold after 90 days) and access-based (zero-read in window → demote); configurable via `TieredStorageConfig`
  - Errors: migration failure rolls back and retries with exponential backoff; partial migration never leaves data inconsistent (copy-then-delete)
  - Tests: unit tests for policy evaluation; integration test verifying read-after-migrate returns original data
  - Perf: migration background overhead < 5% of sustained write throughput; cold-tier read latency documented in config (object storage SLA dependent)
- [x] GCS (Google Cloud Storage) blob backend (`blob_backend_gcs.cpp`) (Target: v1.6.0)
  - Inputs/outputs: same `IBlobBackend` interface as existing S3/Azure backends
  - Affected files: new `blob_backend_gcs.cpp`, `../include/storage/blob_backend_gcs.h`; register in `BlobStorageManager`
  - Auth: ADC (Application Default Credentials) via `GOOGLE_APPLICATION_CREDENTIALS`; fail-closed if credentials absent
  - Errors: GCS API errors mapped to `StorageError`; retry on transient 5xx/429 with jitter backoff
  - Tests: integration test against GCS emulator (`fake-gcs-server`)
- [x] NVMe Optimizations via `NVMeManager` (Target: v1.6.0) ✅
  - Inputs: `NVMeConfig` with device_path, queue_depth, feature flags
  - Outputs: `NVMeCapabilities` report; adjusted RocksDB Direct I/O flags; background-thread count recommendation; io_uring async I/O submit/poll; ZNS zone reset/finish/wp-query
  - Affected files: `src/storage/nvme_manager.cpp`, `include/storage/nvme_manager.h`; `RocksDBWrapper::Config` extended with `enable_nvme_optimizations`, `nvme_*` fields; `configureOptions()` applies NVMe flags
  - Constraints: all Linux-specific code (#ifdef __linux__); io_uring requires THEMIS_ENABLE_IO_URING compile flag; graceful fallback to pread/pwrite when unavailable
  - Errors: io_uring setup failure → disable io_uring and log WARN; ZNS unavailable → log WARN and skip; direct I/O unsupported filesystem → fall back to buffered I/O
  - Tests: `tests/test_nvme_manager.cpp` — 20+ focused unit tests (lifecycle, capabilities, Direct I/O flags, background threads, async I/O fallback, ZNS no-ops, RocksDB integration)
  - CI: `.github/workflows/02-feature-modules_storage_nvme-manager-ci.yml`
  - Perf target: 30–50% lower p99 latency vs. buffered I/O on NVMe with io_uring + Direct I/O enabled
- [x] Erasure coding redundancy mode in `BlobRedundancyManager` (space-efficient alternative to RAID-1) (Target: v1.7.0) ✅
  - Implemented as `RedundancyMode::PARITY` using `ErasureCodingBackend` (Reed-Solomon)
  - Algorithm: RS(k,m) with configurable data/parity shards; RS(4,2) default (1.5× overhead vs 3× for RAID-1)
  - Affected files: `blob_redundancy_manager.cpp` (PARITY path); `erasure_coding_backend.cpp` for RS encode/decode
  - Errors: reconstruction failure if fewer than k shards available
  - Tests: `tests/test_erasure_coding_backend.cpp` — RS(10,4)/RS(6,3)/RS(4,2) encode/decode, multi-shard fault tolerance, BlobRedundancyManager PARITY mode integration
  - CI: `.github/workflows/02-feature-modules_storage_erasure-coding-blob-storage-ci.yml`

### Long-term (6-12 months)
- [x] Distributed transactions across shards via two-phase commit (2PC) with Raft coordination (Target: v1.7.0) ✅
  - **Implemented (v1.7.0):** `DistributedTransactionManager` + `IDistributedShardParticipant` + `DistributedTransaction`
    - Storage-layer 2PC coordinator (Phase 1 PREPARE / Phase 2 COMMIT|ABORT)
    - `ManagerSharedState` shared ownership: transactions safely outlive their manager
    - `shared_ptr` participant references: no use-after-free on concurrent `unregisterShard()`
  - Inputs: multi-shard operations spanning separate RocksDB instances
  - Outputs: atomic commit or rollback across all participant shards
  - Affected files: `distributed_transaction_manager.h/.cpp`; coordinates via existing `RaftMVCCBridge`
  - Tests: 27 unit tests in `tests/test_distributed_transactions.cpp`
  - Perf: 2PC round-trip adds ≤ 2× single-shard latency on same-datacenter nodes
- [x] Vectorized execution support in `ColumnarFormat` (SIMD batch processing) (Target: v2.0.0) ✅
  - Inputs: columnar scan batches of up to 8,192 rows
  - Outputs: filtered/projected result batches processed via AVX2 SIMD instructions
  - Affected files: new `include/storage/simd_filter.h`, `src/storage/simd_filter.cpp`; no ABI change to `ColumnarFormat` public API
  - Dispatch: AVX-512 → AVX2 → NEON → scalar; runtime CPU feature detection (memoised)
  - `SIMDColumnFilter::scan()` integrates zone-map early-out on `ColumnSegment`
  - Tests: 25 tests in `tests/test_simd_columnar_filter.cpp` (SIMDColumnarFilterFocusedTests); scalar-reference parity verified for all 6 ops + all 4 numeric types
  - Perf: throughput SLO gated by `THEMIS_RUN_PERF_TESTS=1` (SF-25)
- [x] Native Parquet export from columnar format (Target: v2.0.0) ✅
  - Inputs: `ColumnarFormat` table scan result (`vector<vector<ColumnSegment>>`)
  - Outputs: Apache Parquet v2 file compatible with Spark, DuckDB, Pandas; PAR1 magic, Thrift binary FileMetaData
  - Affected files: new `include/storage/storage_parquet_exporter.h`, `src/storage/storage_parquet_exporter.cpp`
  - Supports INT32, INT64, FLOAT32, FLOAT64, BOOL, STRING column types with native Parquet type mapping
  - With `ARROW_ENABLED`: delegates to Apache Arrow / Parquet C++ library; without it: portable Thrift binary Parquet v2 writer
  - Errors: `ERR_EXPORT_FORMAT_INVALID` for unknown types; `ERR_EXPORT_IO_ERROR` for file write failures; `ERR_EXPORT_CONFIG_INVALID` for mismatched schema
  - Tests: 15 tests in `tests/test_storage_parquet_exporter.cpp` (StorageParquetExporterFocusedTests)

## Implementation Phases

### Phase 1: Core RocksDB Layer (Status: Completed ✅)
- [x] `RocksDBWrapper` with MVCC, WAL, BlobDB, multi-path SSTables, async I/O
- [x] `MVCCStore` snapshot isolation
- [x] `WALStorage` management and replay
- [x] `KeySchema` multi-model encoding
- [x] `StorageEngine` with dependency injection
- [x] `MergeOperators` for counter and list operations
- [x] `HLC` hybrid logical clocks

### Phase 2: Backup, Recovery & Blob Storage (Status: Completed ✅)
- [x] `BackupManager` incremental and full backups
- [x] `PITRManager` point-in-time recovery
- [x] Blob backends: Filesystem, RocksDB BlobDB, S3, Azure Blob, WebDAV
- [x] `BlobRedundancyManager` RAID-1 mirror

### Phase 3: Compression, Optimization & Security (Status: Completed ✅)
- [x] `CompressionStrategy` pluggable per-table compression
- [x] `ColumnarFormat` analytical storage
- [x] `BatchWriteOptimizer` adaptive write batching
- [x] `CompactionManager` manual and scheduled compaction
- [x] `SecuritySignature` field-level AES-GCM encryption
- [x] `SecuritySignatureManager` HMAC-SHA256 tamper detection
- [x] `StorageAuditLogger` structured audit trail
- [x] Production-mode safety flag

### Phase 4: Operational & Advanced Features (Status: Completed ✅)
- [x] `DiskSpaceMonitor` quota monitoring
- [x] `IndexMaintenance` background optimization
- [x] `TransactionRetryManager` exponential backoff
- [x] `DatabaseConnectionManager` connection pooling
- [x] `RaftMVCCBridge` Raft-to-MVCC integration
- [x] `HistoryManager` version tracking
- [x] `NLPMetadataExtractor` automatic metadata

### Phase 5: Tiered Storage & Distributed Transactions (Status: Completed ✅ — v1.7.0)
- [x] Tiered storage (hot/warm/cold) with age- and access-based migration policies
- [x] GCS blob backend
- [x] NVMe Optimizations: `NVMeManager` with io_uring, multi-queue, ZNS, Direct I/O (CI: nvme-manager-ci.yml)
- [x] Erasure coding in `BlobRedundancyManager` (`PARITY` mode via `ErasureCodingBackend`, RS(k,m)) (CI: erasure-coding-blob-storage-ci.yml)
- [x] 2PC distributed transactions (`DistributedTransactionManager`, v1.7.0)

### Phase 6: Write-Optimized Storage (Status: Completed ✅ — v1.8.0)
- [x] `WomTree` – Write-Optimized Merge (WOM) Tree: Bε-tree alternative to LSM for write-heavy workloads
  - Write amplification 2–5× (vs. 10–30× for LSM)
  - Lazy buffer propagation — reduced compaction overhead
  - Configurable fanout, leaf capacity, buffer size
  - Full put/get/remove/scan/compact API with thread safety
  - Write-amplification and read-hit-ratio metrics in Stats

### Phase 6b: PERF-D6 – Concurrent Write Controller (Status: Completed ✅ — v2.0.0)
- [x] `ConcurrentWriteController` – bounded FIFO write-concurrency semaphore (Target: v2.0.0) (Issue: PERF-D6)
  — implemented in `include/storage/concurrent_write_controller.h`, `src/storage/concurrent_write_controller.cpp`
- [x] `ConcurrentWriteControllerConfig` – `max_concurrent_writes` (0 = hw_concurrency/2), `max_queue_depth` (0 = unlimited), `acquire_timeout` (0 = blocking)
- [x] `WriteGuard` – RAII slot holder; move-only; destructor wakes the next FIFO waiter
- [x] `acquire()` – blocking acquire; FIFO wakeup via `std::promise`/`std::future` chain; raises `std::runtime_error` on queue-full, timeout, or shutdown
- [x] `tryAcquire()` – non-blocking; returns `std::nullopt` if at capacity
- [x] `shutdown()` – unblocks all waiters atomically (no deadlock on destruction)
- [x] EWMA statistics: `avg_wait_us` updated on every acquire (α ≈ 0.1, integer-scaled)
- [x] P99 latency: sliding window of last 128 wait times; sorted on read via `getStats()`
- [x] Lifetime max and total acquired / rejected counters (lock-free atomics)
- [x] Throughput: ≥ 50k acquires/s (measured: ~616k ops/s, 8 threads) ✅
- [x] CV regression guard: 10-thread stress test (AC-D6-21) verifies CV < 20 % — eliminates the thundering-herd variance that caused D-6
- [x] Tests: 25 tests in `tests/test_concurrent_write_controller.cpp` (`ConcurrentWriteControllerFocusedTests`)
  - AC-D6-1–20: unit tests (config, guard lifecycle, FIFO, stats, timeout, shutdown)
  - AC-D6-21: 10-thread stress CV < 20 % regression guard
  - AC-D6-22–24: move semantics, idempotent release, unlimited queue
  - AC-D6-25: throughput SLO (gated by `THEMIS_RUN_PERF_TESTS=1`)

### Phase 5.5: Build System Audit & Stub Fixes (Status: Completed ✅ — March 2026)
- [x] All `src/storage/*.cpp` files verified registered in cmake build system (main `CMakeLists.txt` + `StorageEnhancements.cmake` + `BlobStorage.cmake`)
- [x] 21+ focused standalone test targets added in `tests/CMakeLists.txt`: StorageEngineDI, StorageEngineProd, StorageAuditLogger, StorageFuzz, StorageLatencyBench, BlobStorage, BlobTransferCheckpoint, CompressionStrategy, TieredStorage, WalStorage, WalManager, WalArchiving, WalBackupManager, WalChaos, WalManifestCorruption, WalReplication, WalReplicationIntegration, WalGrpcApply, MvccStore, MvccHistory, MvccWalIntegration, NVMeFocusedTests, ErasureCodingFocusedTests, RocksDBSizeCalculationFocusedTests, BlobRedundancyEventListenerFocusedTests
- [x] `RocksDBWrapper::getApproximateSize()` returns real on-disk SST size (`rocksdb.total-sst-files-size`) — was returning 0 (CI: rocksdb-size-calculation-ci.yml)
- [x] `SecuritySignatureManager::listAllSignatures()` iterates full RocksDB key-range via `iterateRange` — was stub (CI: security-signature-rocksdb-iteration-ci.yml)
- [x] `BlobRedundancyManager::createRocksDBListener()` returns working `RocksDBBlobListener` — was returning error stub (CI: blob-redundancy-event-listener-ci.yml)

### Phase 7: Streaming Blob Write Path (Status: Completed ✅ — v2.0.0, PERF-D5)
- [x] `RocksDBWrapper::putBlob()` — streaming write path for large blobs (Issue PERF-D5)
  - Blobs ≥ `blob_streaming_threshold_bytes` (default 64 KB) are split into `blob_chunk_size_bytes` (default 128 KB) chunks
  - Parallel chunk encoding via `std::async` thread pool (`blob_streaming_threads`, default 4)
  - All chunks + manifest committed atomically via single `WriteBatch::Write()` — bypasses per-write transaction overhead
  - Internal key scheme: manifest `__tmbs_m__:<key>`, chunks `__tmbs_c__:<key>:<6-digit-index>`
  - Backward compatible: blobs below threshold fall back to regular `put()`
- [x] `RocksDBWrapper::getBlob()` — transparent reassembly from manifest + chunk keys via `MultiGet`
- [x] `RocksDBWrapper::delBlob()` — atomic manifest + chunk deletion via `WriteBatch`
- [x] New Config fields: `enable_blob_streaming`, `blob_streaming_threshold_bytes`, `blob_chunk_size_bytes`, `blob_streaming_threads`
- [x] `tests/test_blob_streaming.cpp` — 16 focused tests (BlobStreamingFocusedTests): round-trip, boundary, parallel, integrity, delete, overwrite, fallback, custom chunk size, single thread, zero-length, non-aligned
- [x] `benchmarks/bench_comprehensive.cpp` `StoreLargeBlobs_1MB` updated to use `putBlob()` with 8-thread encoding

## Production Readiness Checklist
- [x] Unit test coverage for core storage paths
- [x] Integration tests with real RocksDB instance
- [x] Backup and PITR restore validation
- [x] Encryption enabled in all production deployments (`THEMIS_PRODUCTION_MODE`)
- [x] Audit logging for all write operations
- [x] Performance benchmarks for tiered storage migration
- [x] NVMe optimizations with graceful fallback on non-NVMe hosts
- [x] Erasure coding (Reed-Solomon) for space-efficient blob redundancy
- [x] Distributed 2PC transactions for cross-shard atomicity
- [x] Write-Optimized Merge Tree for write-heavy workloads
- [x] Streaming blob write path (`putBlob` / `getBlob` / `delBlob`) for high-throughput 1 MB+ blob storage (PERF-D5, v2.0.0)
- [ ] Chaos/fault-injection tests for blob backend failover (Target: v2.0.0)

## Known Issues & Limitations
- `NLPMetadataExtractor` depends on an external NLP model; slow startup if model is not pre-warmed
- Tiered storage uses flat filesystem files per key; for large datasets a more efficient store (e.g. RocksDB column-family per tier) is recommended

## Breaking Changes
- `StorageEngine::createDefault()` factory is deprecated; use the DI constructor with explicit `IExpressionEvaluatorPtr`, `IFieldEncryptionPtr`, `IKeyProviderPtr`, and `IIndexManagerPtr` to avoid insecure defaults in production
- `KeySchema` key format v1.5.0+ (prefix-based) is not backward-compatible with keys created before v1.5.0; migration required for existing data
