> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Storage Module

All notable changes to the Storage module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Documentation governance sync: ROADMAP/FUTURE moved to future-only planning, and module docs aligned with source-verified wording (`README`, `ARCHITECTURE`, `SECURITY`, `PERFORMANCE_EXPECTATIONS`, `AUDIT`).

## [2.0.0] — 2026-04-12
### Added
- `ConcurrentWriteController` – bounded FIFO write-concurrency semaphore; `WriteGuard` RAII; EWMA stats and P99 sliding window (PERF-D6)
- `StreamingIngestManager` – ring-buffer + flush-thread → single `WriteBatch`; `OverflowPolicy::BLOCK/DROP`; 10 ms flush interval; 1M event max buffer (Issue #4571)
- `ColumnarCache` – LRU in-memory columnar segment cache; `PinGuard` RAII; `SegmentDType` typed; hit/miss counters; `on_evict` callback (Issue #4572)
- `SIMDColumnFilter` – AVX-512/AVX2/NEON/scalar dispatch; runtime CPU feature detection; `SIMDColumnFilter::scan()` with zone-map early-out; 6 ops × 4 numeric types
- `StorageParquetExporter` – native Apache Parquet v2 file writer; PAR1 magic; Thrift binary FileMetaData; Arrow/Parquet C++ delegation when `ARROW_ENABLED`
- `RocksDBWrapper::putBlob()` / `getBlob()` / `delBlob()` – streaming write path for blobs ≥ 64 KB; parallel chunk encoding via `std::async`; atomic `WriteBatch` commit (PERF-D5)
- `AdaptiveCompactionScheduler` – IO-sample-based load-aware compaction scheduling
- `MVCCChainPruner` – background MVCC version-chain pruning
- `VectorIndexBackend` – `IVectorIndexBackend` + `InMemoryVectorIndex` flat-scan implementation
- `ZeroCopyBlobTransfer` – `MmapBlobView` + `sendfile` zero-copy transfer
- `EncryptedBlobBackend` – AES-256-GCM blob backend wrapper; `IEncryptionKeyProvider` + `StaticKeyProvider`
- `OnlineSchemaMigration` – zero-downtime DDL via `SchemaMigrator`
- `SchemaDeadWeightDetector` – unused field and stale index detection; `GdprFieldRegistry`
- `StorageLayoutAdvisor` – layout recommendations (row/columnar/tiered/vector) from access stats

## [1.7.0] — 2026-02-01
### Added
- `DistributedTransactionManager` – storage-layer 2PC coordinator; `IDistributedShardParticipant`; `DistributedTransaction`; `ManagerSharedState` shared ownership for safe concurrent shard lifecycle
- `WomTree` – Write-Optimized Merge (WOM/Bε) Tree: write amplification 2–5× vs 10–30× for LSM; lazy buffer propagation; put/get/remove/scan/compact API; thread-safe
- `NVMeManager` – io_uring async I/O (Linux ≥ 5.1); multi-queue NVMe; ZNS zone management; Direct I/O flag recommendation; `RocksDBWrapper` NVMe integration
- `BlobRedundancyManager` PARITY mode – Reed-Solomon erasure coding via `ErasureCodingBackend`; RS(k,m) configurable; copy-then-delete shard migration
- `ErasureCodingBackend` – Reed-Solomon RS(k,m) encode/decode; `erasure_coder_factory.cpp` for codec selection
- `BlobRedundancyManager::createRocksDBListener()` – `RocksDBBlobListener` reacts to SST file deletions

## [1.6.0] — 2026-03-12
### Added
- Tiered storage (hot/warm/cold) with age- and access-based migration policies (`tiered_storage.cpp`)
- GCS (Google Cloud Storage) blob backend (`blob_backend_gcs.cpp`, requires `THEMIS_ENABLE_GCS`)
- BackupManager in-memory scheduling: `scheduleBackup`, `cancelScheduledBackup`, `listScheduledBackups`
- Cloud backup routing: `uploadBackupToCloud`, `restoreFromCloud` gated on `THEMIS_ENABLE_S3/AZURE/GCS`
- Production-mode safety flag (`THEMIS_PRODUCTION_MODE`) preventing insecure encryption defaults
- Build system audit: all `src/storage/*.cpp` registered; 21 focused standalone test targets added

### Changed
- `StorageEngine::createDefault()` deprecated in favour of explicit DI constructor

### Fixed
- Jitter value validation in `TransactionRetryManager` (prevents non-finite delay calculation)
- `const_cast` UB removed in storage audit code

## [1.5.0] — 2025-09-01
### Added
- `TransactionRetryManager` with exponential/linear/fixed backoff and circuit breaker
- `BlobRedundancyManager` RAID-1 mirror across multiple blob backends
- `RaftMVCCBridge` integrating Raft consensus log with MVCC storage
- `HLC` Hybrid Logical Clock for causally-consistent distributed timestamps
- `HistoryManager` version history and change tracking per key
- `SecuritySignature` + `SecuritySignatureManager` AES-GCM encryption and HMAC-SHA256

## [1.0.0] — 2024-01-01
### Added
- `RocksDBWrapper` with MVCC, WAL, BlobDB, multi-path SSTables, async I/O
- `MVCCStore` snapshot isolation
- `WALStorage` write-ahead log management and replay
- `KeySchema` unified multi-model key encoding (relational, document, graph, vector, timeseries)
- `StorageEngine` high-level abstraction with dependency injection
- `BackupManager` incremental and full backups
- `PITRManager` point-in-time recovery
- `CompressionStrategy` pluggable per-table compression (Snappy, Zstd, LZ4, Brotli)
- `CompressedStorage` transparent compression/decompression layer
- `ColumnarFormat` columnar storage for analytical workloads
- `BatchWriteOptimizer` adaptive write batching
- `CompactionManager` manual and scheduled RocksDB compaction
- `IndexMaintenance` background index rebuild, optimize, and consistency checks
- `MergeOperators` custom RocksDB merge operators (counters, list appends, sets, max)
- `StorageAuditLogger` structured audit trail for all storage operations
- `DiskSpaceMonitor` real-time disk quota monitoring and alerting
- `DatabaseConnectionManager` connection pooling and lifecycle management
- `BaseEntity` common base type for all storage-layer entities
- `BlobStorageManager` with INLINE, RocksDB BlobDB, Filesystem, S3, Azure Blob, WebDAV backends
- `BlobRedundancyManager` RAID-1 mirror redundancy
- `FilesystemBlobBackend`, S3, Azure, WebDAV, GCS blob backend implementations
