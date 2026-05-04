> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/storage/ -->

# Storage Module - Future Enhancements

## Scope

- RocksDB-based persistent storage with MVCC, WAL, BlobDB, multi-path SSTables, and async I/O
- Multi-model key encoding (relational, document, graph, vector, time-series) via `KeySchema`
- Backup/PITR, blob backends (Filesystem, S3, Azure Blob, WebDAV, GCS), and RAID-1 redundancy
- Field-level AES-256-GCM encryption, HMAC-SHA256 tamper detection, and structured audit logging
- Columnar format for analytics, adaptive write batching, and pluggable per-table compression
- Raft-MVCC integration (`RaftMVCCBridge`), hybrid logical clocks (`HLC`), and transaction retry management
- Tiered storage (hot NVMe → warm SATA → cold object storage) implemented in v1.6.0

## Design Constraints

- [ ] All blob backends must implement `IBlobBackend`; no direct SDK calls outside backend implementation files
- [ ] `THEMIS_PRODUCTION_MODE` must reject no-op (plaintext) encryption at startup — fail-closed is mandatory
- [ ] Data migration (tier moves, schema changes) must use copy-then-delete; partial migration must never leave data inconsistent
- [ ] All new RocksDB column families must be registered in `KeySchema` before use; ad-hoc CF creation is forbidden
- [ ] `StorageAuditLogger` must record every write, delete, and compaction with caller identity and HLC timestamp
- [ ] Public `StorageEngine` API is frozen for v1.x; all new capabilities must be additive (no removed or renamed methods)
- [ ] Transaction retries must use exponential backoff with jitter; max retry count must be configurable via `TransactionRetryConfig`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IBlobBackend` | `BlobStorageManager`, `BlobRedundancyManager` | put/get/delete/exists; implemented by Filesystem, S3, Azure, GCS |
| `IEncryptionProvider` | `StorageEngine`, `SecuritySignatureManager` | AES-256-GCM key provision and rotation |
| `IKeyProvider` | `StorageEngine` | Key derivation per tenant and column family |
| `IIndexManager` | `StorageEngine`, `IndexMaintenance` | Rebuild, optimize, consistency check |
| `ICompressionStrategy` | `CompressedStorage`, `ColumnarFormat` | Snappy, Zstd, LZ4, Brotli, None; selected per column family |
| `IRaftMVCCBridge` | `MVCCStore`, Raft consensus layer | Append Raft log entry → apply to MVCC snapshot atomically |
| `IAuditLogger` | All storage write/delete paths | Structured JSON audit trail with HLC timestamps and caller ID |

## Planned Features

### `IndexAnalyzer`: Per-Index Analyze with Hot/Warm/Cold Tier Awareness, Cron Scheduling, and AI/ML Hook

**Priority:** High
**Target Version:** v1.9.0
**Status:** 🟡 In Progress

#### Scope
- Tier-aware fragmentation analysis per index (hot/warm/cold thresholds independently configurable)
- Produces `IndexAnalysisReport` with `IndexRecommendation` (NONE / UPDATE_STATS / REORGANIZE / PARTIAL_REBUILD / FULL_REBUILD)
- Cron-based automatic scheduling via the existing `CronExpression` parser (standard 5-field POSIX cron + @-shortcuts)
- AI/ML intervention hook (`IIndexAnalysisAdvisor`) that can override the rule-based recommendation
- Full YAML configuration via `config/index_analyze.yaml` (or inline in `config.yaml` under `index_analyze:`)

#### Design Constraints
- `IIndexAnalysisAdvisor::advise()` must be thread-safe; the scheduler calls it from a background thread
- Thresholds for warm and cold tiers MUST be ≥ hot-tier thresholds (looser = cheaper maintenance)
- AI advisor is disabled by default (`ai_advisor.enabled: false`); opt-in to avoid unexpected interventions in production
- Cron scheduler uses copy-then-read config snapshot to avoid holding the mutex during RocksDB I/O
- No new dependency introduced; yaml-cpp and CronExpression are already in the build

#### Required Interfaces

```cpp
// AI/ML intervention hook
class IIndexAnalysisAdvisor {
public:
    virtual std::optional<std::pair<IndexRecommendation, std::string>>
    advise(const IndexAnalysisReport& report) = 0;
};

// Per-index analysis
Result<IndexAnalysisReport> IndexAnalyzer::analyze(
    const std::string& index_name,
    storage::StorageTierLevel tier,
    std::optional<TierThresholds> overrides = std::nullopt);

// Batch analysis of all configured indices
std::vector<IndexAnalysisReport> IndexAnalyzer::analyzeAll();

// YAML config load
static Result<IndexAnalyzeConfig> IndexAnalyzeConfig::fromYamlFile(const std::string& path);

// Cron scheduler lifecycle
Result<void> startScheduled();   // validates cron expression, launches background thread
void         stopScheduled();    // graceful shutdown
```

#### Implementation Notes
- Fragmentation estimate: `(total_sst - live_sst) / total_sst * 100 + l0_files * 2.0` using RocksDB properties `rocksdb.total-sst-files-size`, `rocksdb.live-sst-files-size`, `rocksdb.num-files-at-level0`
- Statistics staleness: tracked via `stats_age_hours`; initially approximated at 2 h until a metadata CF for stats-update-time is introduced
- AI advisor receives the fully populated preliminary `IndexAnalysisReport`; returning `std::nullopt` leaves the rule-based recommendation unchanged
- Scheduler loop: compute `CronExpression::getNextExecution(now)`, `cv_.wait_until(*next)`, run `analyzeAll()`, repeat

#### Test Strategy
- IA-01…IA-15 in `tests/test_index_analyzer.cpp` (`IndexAnalyzerFocusedTests`)
- IA-01..IA-04: threshold defaults and tier dispatch (pure logic, no DB)
- IA-05..IA-06: YAML load – error path + valid config
- IA-07: ctor guard for null db_wrapper
- IA-08..IA-09: setConfig / setAdvisor thread safety
- IA-10..IA-14: classify() function covering all five recommendations
- IA-15: lastReports() empty before first run

#### Performance Targets
- `analyzeAll()` for 100 indices: ≤ 50 ms wall-clock (RocksDB property reads are non-blocking)
- Scheduler thread wake-up overhead: ≤ 1 ms (uses `condition_variable::wait_until`)
- AI advisor timeout: caller-controlled; no internal timeout imposed by IndexAnalyzer

#### Security / Reliability
- Cron expression validated before thread launch; malformed expression → error returned, no thread started
- AI advisor exceptions are caught and logged; the rule-based recommendation is preserved
- `stopScheduled()` is called from the destructor; no thread left running after object destruction

---

### `RocksDBWrapper`: Implement Proper Size Calculation
**Priority:** Medium
**Target Version:** v1.8.0

`rocksdb_wrapper.cpp` line 1445: "TODO: Implement proper size calculation". The `RocksDBWrapper::getApproximateSize()` or equivalent method returns 0 or a placeholder, making disk-space monitoring, compaction triggers, and admin API storage metrics unreliable.

**Implementation Notes:**
- `[x]` Use `rocksdb::DB::GetApproximateSizes()` API to compute the on-disk SST file sizes for a key range.
- `[x]` Alternatively, use `rocksdb::DB::GetIntProperty(rocksdb::DB::Properties::kTotalSstFilesSize)` for total CF size.
- `[x]` Wire the result into `DiskSpaceMonitor` and the `/v1/admin/storage/stats` endpoint.

---

### `SecuritySignatureManager`: Implement RocksDB Iteration
**Priority:** Medium
**Target Version:** v1.8.0

`security_signature_manager.cpp` line 110: "TODO: Implement proper RocksDB iteration when `RocksDBWrapper` supports it". Without iteration, the signature manager cannot verify integrity across all stored records.

**Implementation Notes:**
- `[x]` Add `RocksDBWrapper::iterateRange(start_key, end_key, callback)` that uses a `rocksdb::Iterator` under the hood.
- `[x]` Wire into `SecuritySignatureManager::verifyAll()` to scan all document keys and verify their signatures in sequence.

---

### `BlobRedundancyManager`: Implement RocksDB Event Listener

**Priority:** Low
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`BlobRedundancyManager::createRocksDBListener()` returns a working `RocksDBBlobListener`
subclassing `rocksdb::EventListener`. Overrides `OnTableFileDeleted` to mark affected blob
locations unhealthy when backing SST files are deleted during compaction. Register the
listener via `rocksdb::Options::listeners` at database open time.

**Implementation Notes:**
- `[x]` `BlobRedundancyEventListener` subclasses `rocksdb::EventListener`; `OnTableFileDeleted` triggers re-replication of blobs whose backing SST was deleted.
- `[x]` Registered via `rocksdb::Options::listeners` at database open time.

---


**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented

Raft-based distributed transactions across multiple nodes.

**Features:**
- Two-phase commit (2PC) protocol
- Cross-shard atomic operations
- Abort on any participant vote NO
- Thread-safe coordinator with per-transaction handles

**Implementation:**
- `include/storage/distributed_transaction_manager.h` — `DistributedTransactionManager`, `IDistributedShardParticipant`, `DistributedTransaction`, `DistributedOperation`
- `src/storage/distributed_transaction_manager.cpp` — full 2PC implementation

**API:**
```cpp
DistributedTransactionManager dtx_manager(nodes);
auto tx = dtx_manager.beginDistributedTransaction();

// Write to multiple shards
tx->put("shard1:key1", "value1");
tx->put("shard2:key2", "value2");

// Commit atomically across shards
tx->commit();  // 2PC protocol
```

**Use Cases:**
- Multi-tenant data isolation
- Geographic data distribution
- Horizontal scaling

---

### Tiered Storage (Hot/Warm/Cold)
**Priority:** High
**Target Version:** v1.6.0

Automatic data migration based on access patterns.

**Tiers:**
- **Hot**: NVMe SSDs (frequent access)
- **Warm**: SATA SSDs (moderate access)
- **Cold**: Object storage (rare access, archival)

**Policies:**
- Age-based: Move data older than N days to warm/cold
- Access-based: Move rarely accessed data to cold tier
- Size-based: Move large blobs to object storage

**Configuration:**
```cpp
TieredStorageConfig config;
config.hot_tier_path = "/nvme/data";
config.warm_tier_path = "/sata/data";
config.cold_tier_backend = "s3://archive-bucket";
config.hot_to_warm_days = 30;
config.warm_to_cold_days = 90;

TieredStorageManager tiered(config);
```

---

### Erasure Coding for Blob Storage
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Implemented

Space-efficient redundancy using erasure codes (e.g., Reed-Solomon).

**Encoding Schemes:**
- **RS(10,4)**: 10 data + 4 parity blocks (40% overhead vs 200% for mirroring)
- **RS(6,3)**: 6 data + 3 parity blocks (50% overhead)
- **RS(4,2)**: 4 data + 2 parity blocks (50% overhead, faster)

**Benefits:**
- 50-70% storage savings vs mirroring
- Survives multiple node failures
- Configurable fault tolerance

**Implementation:** `src/storage/erasure_coding_backend.cpp` / `include/storage/erasure_coding_backend.h`

`BlobRedundancyManager` activates erasure coding automatically when `BlobRedundancyConfig::mode == RedundancyMode::PARITY`.

**Example:**
```cpp
ErasureCodingConfig config;
config.data_shards   = 10;   // RS(10,4): 40% overhead vs 200% for mirroring
config.parity_shards = 4;

ErasureCodingBackend backend(config);
backend.put("blob-123", data);  // Automatically encodes and distributes

// Survives loss of up to 4 blocks
auto result = backend.get("blob-123");  // Reconstructs from available blocks
```

---

### Online Schema Migration
**Priority:** Medium
**Target Version:** v1.7.0

Zero-downtime schema changes for relational and document models.

**Supported Operations:**
- Add/drop columns
- Rename columns
- Change column types
- Add/drop indexes
- Partition tables

**Migration Framework:**
```cpp
SchemaMigrator migrator(storage);

// Define migration
migrator.addColumn("users", "phone_number", "VARCHAR(20)");
migrator.renameColumn("users", "email", "email_address");
migrator.addIndex("users", "email_address");

// Apply migration online (no downtime)
migrator.migrate();  // Background process, versioned migrations
```

---

### Write-Optimized Merge (WOM) Tree
**Priority:** Low
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0)

Alternative to LSM-tree for write-heavy workloads.

**Implementation:** `src/storage/wom_tree.cpp` / `include/storage/wom_tree.h`

**Advantages:**
- Lower write amplification (2-5x vs 10-30x for LSM)
- Better for update-heavy workloads
- Reduced compaction overhead

**Trade-offs:**
- Higher space amplification
- Slower point reads

---

### Blockchain-Style Immutable Log
**Priority:** Low
**Target Version:** v1.9.0

Immutable append-only log with cryptographic hashing for audit trails.

**Features:**
- Merkle tree structure
- Hash chaining (each block references previous)
- Tamper detection
- Replay protection

**Use Cases:**
- Audit logs
- Financial transactions
- Compliance requirements

---

## Performance Optimizations

### GPU-Accelerated Compression
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented

Use CUDA/ROCm for parallel compression/decompression.

**Target Algorithms:**
- ✅ Zstd (NVIDIA nvCOMP library) — `GpuCompressionAlgorithm::ZSTD` / `CompressionMethod::GPU_ZSTD`
- ✅ Snappy (GPU-accelerated variant) — `GpuCompressionAlgorithm::SNAPPY` / `CompressionMethod::GPU_SNAPPY`
- ✅ LZ4 (parallel decompress) — `GpuCompressionAlgorithm::LZ4` / `CompressionMethod::GPU_LZ4`

**Implementation:** `src/storage/gpu_compression.cpp` / `include/storage/gpu_compression.h`

GPU paths are enabled via `THEMIS_ENABLE_CUDA` (nvCOMP) or `THEMIS_ENABLE_HIP` (ROCm)
compile-time flags.  All algorithms transparently fall back to CPU implementations
(zstd_codec, libsnappy, liblz4) when no GPU is present, ensuring zero-dependency
operation in environments without CUDA/HIP toolchains.

**Expected Improvement:** 5-10x compression throughput

---

### NVMe Optimizations
**Priority:** High
**Target Version:** v1.6.0

Leverage NVMe-specific features for better performance.

**Optimizations:**
- **io_uring**: Linux async I/O framework
- **Multi-queue**: Parallel I/O submission
- **Zone namespaces (ZNS)**: Direct control over flash management
- **Direct I/O**: Bypass page cache for predictable latency

**Expected Improvement:** 30-50% lower latency, 2x throughput

---

### Adaptive Compaction
**Priority:** Medium
**Target Version:** v1.7.0

Machine learning-based compaction scheduling.

**Approach:**
- Monitor read/write patterns
- Predict compaction impact
- Schedule compactions during low-load periods
- Adjust compaction triggers dynamically

**Expected Improvement:** 20-30% less compaction CPU overhead

---

### Zero-Copy Blob Transfers
**Priority:** Medium
**Target Version:** v1.7.0

Eliminate memory copies when transferring blobs between backends.

**Techniques:**
- `sendfile()` for local filesystem
- S3 multipart upload with streaming
- Memory-mapped files for large blobs

**Expected Improvement:** 40-60% faster blob transfers

---

### Bloom Filter Optimization
**Priority:** Low
**Target Version:** v1.8.0

Replace standard Bloom filters with more efficient alternatives.

**Options:**
- **Cuckoo filters**: 20% less space for same false positive rate
- **Blocked Bloom filters**: Better cache locality
- **Xor filters**: Fastest queries, immutable

**Expected Improvement:** 15-20% faster point lookups

---

## Refactoring Opportunities

### Separate RocksDB Wrapper into Multiple Classes
**Priority:** Medium
**Target Version:** v1.7.0

Split monolithic RocksDBWrapper into specialized classes.

**Proposed Structure:**
```
RocksDBWrapper (main interface)
├─ RocksDBReader (read operations)
├─ RocksDBWriter (write operations)
├─ RocksDBTransaction (transaction mgmt)
├─ RocksDBIterator (iteration)
├─ RocksDBConfig (configuration)
└─ RocksDBMonitor (statistics/metrics)
```

**Benefits:**
- Smaller, more focused classes
- Easier testing
- Better separation of concerns

---

### Extract Blob Backend Interface
**Priority:** Low
**Target Version:** v1.8.0

Create more granular interfaces for blob backends.

**Current:**
```cpp
class IBlobStorageBackend {
    // All methods required
};
```

**Proposed:**
```cpp
class IBlobReader {
    virtual Result<std::vector<uint8_t>> get(const std::string& id) = 0;
};

class IBlobWriter {
    virtual Result<BlobRef> put(const std::string& id, const std::vector<uint8_t>& data) = 0;
};

class IBlobDeleter {
    virtual Result<void> del(const std::string& id) = 0;
};

// Compose interfaces
class IBlobStorageBackend : public IBlobReader, public IBlobWriter, public IBlobDeleter {};
```

**Benefits:**
- Read-only backends (S3 Glacier)
- Write-only backends (append-only logs)
- Fine-grained permissions

---

### Unified Backup/PITR API
**Priority:** Medium
**Target Version:** v1.7.0

Merge BackupManager and PITRManager into single cohesive API.

**Proposed:**
```cpp
class RecoveryManager {
public:
    // Backup operations
    Result<BackupId> createBackup(BackupType type = INCREMENTAL);
    Result<void> restoreBackup(BackupId id);

    // PITR operations
    Result<SnapshotId> createSnapshot();
    Result<void> restoreToTimestamp(Timestamp ts);

    // Combined operations
    Result<void> restoreToBackupOrSnapshot(Timestamp ts);  // Auto-select best method
};
```

---

### Key Schema as Plugin
**Priority:** Low
**Target Version:** v1.8.0

Allow custom key encoding schemes via plugin API.

**Benefits:**
- Domain-specific key formats
- Custom sorting orders
- Tenant-specific schemas

---

## Known Issues

### Issue #1: RocksDB Compaction Stalls Under Heavy Write Load
**Severity:** Medium
**Reported:** v1.5.0

Write stalls occur when L0 files accumulate faster than compaction.

**Workaround:**
```cpp
config.level0_slowdown_writes_trigger = 20;
config.level0_stop_writes_trigger = 36;
config.max_background_jobs = 8;  // Increase compaction threads
```

**Fix:** Implement adaptive compaction scheduling

**Planned Fix:** v1.6.0

---

### Issue #2: Blob Storage Backend Failover Not Automatic
**Severity:** Medium
**Reported:** v1.5.0

BlobStorageManager doesn't automatically retry on backend failure.

**Workaround:** Manually retry or use BlobRedundancyManager

**Fix:** Add automatic failover with circuit breaker pattern

**Planned Fix:** v1.6.0

---

### Issue #3: PITR Snapshot Cleanup Not Automatic
**Severity:** Low
**Reported:** v1.5.1

Old PITR snapshots accumulate, consuming disk space.

**Workaround:** Manually call `cleanupOldSnapshots()`

**Fix:** Add background cleanup job with retention policy

**Planned Fix:** v1.6.1

---

### Issue #4: Transaction Retry Manager Exponential Backoff Too Aggressive
**Severity:** Low
**Reported:** v1.5.2

Default backoff can lead to long delays for contended keys.

**Workaround:** Configure custom backoff strategy

**Fix:** Implement jittered exponential backoff

**Planned Fix:** v1.6.0

---

### Issue #5: Columnar Format Not Production-Ready
**Severity:** Resolved
**Reported:** v1.5.0
**Fixed:** v2.0.0

Columnar storage correctness issues resolved; `ColumnarFormat` is production-ready for analytical workloads. `SIMDColumnFilter` provides vectorized predicate evaluation; `StorageParquetExporter` provides native Parquet v2 export.

---

## Research Areas

### CXL (Compute Express Link) Integration
**Focus:** Next-generation memory expansion

Explore CXL for:
- Disaggregated memory pools
- Shared RocksDB block cache across nodes
- Near-memory computation

**Research Questions:**
- Can we share memtables via CXL?
- What's the latency impact?
- How to handle coherency?

---

### Learned Index Structures
**Focus:** ML-based indexes for sorted data

Replace traditional B-trees with learned models:
- Predict key position from model
- Reduce memory footprint (models vs index)
- Faster lookups for skewed distributions

**Research Questions:**
- What ML models work best?
- How to handle updates efficiently?
- Can we learn RocksDB SSTable positions?

---

### Persistent Memory (PMem) Integration
**Focus:** Intel Optane or future PMem technologies

Use PMem for:
- Persistent memtables (no WAL needed)
- Write-ahead log (faster than disk)
- Block cache (survive restarts)

**Research Questions:**
- How to handle PMem errors?
- What's the durability guarantee?
- Can we bypass filesystem?

---

### Multi-Version B-Trees (MVBT)
**Focus:** Alternative to LSM-tree for MVCC

Explore MVBT for:
- Lower write amplification
- Better point update performance
- Simpler compaction

**Research Questions:**
- How to integrate with RocksDB?
- What's the space overhead?
- Can we match LSM scan performance?

---

### Quantum-Resistant Encryption
**Focus:** Post-quantum cryptography for field encryption

Prepare for quantum computers:
- NIST PQC algorithms (CRYSTALS-Kyber, CRYSTALS-Dilithium)
- Hybrid classical/quantum schemes
- Key rotation strategies

**Research Questions:**
- What's the performance impact?
- How to migrate existing data?
- Which algorithms to standardize on?

---

## Migration Paths

### v1.5.x → v1.6.x: Tiered Storage
**Breaking Changes:** None (additive)

**New APIs:**
```cpp
TieredStorageManager tiered(config);
storage->setTieredStorage(tiered);
```

**Migration Steps:**
1. Update to v1.6.0
2. Configure tiered storage
3. Enable automatic data migration
4. Monitor tier distribution

**Timeline:** 3 months gradual rollout

---

### v1.6.x → v1.7.x: Distributed Transactions
**Breaking Changes:** Transaction API extends

**Old API:**
```cpp
auto tx = db->beginTransaction();
tx->commit();
```

**New API (backward compatible):**
```cpp
auto tx = db->beginTransaction();  // Local transaction
auto dtx = dtx_manager.beginDistributedTransaction();  // Distributed transaction
```

**Migration Steps:**
1. Update to v1.7.0
2. Test existing transactions (no changes needed)
3. Optionally adopt distributed transactions

**Timeline:** 6 months parallel support

---

### v1.7.x → v1.8.x: WOM Tree Option
**Breaking Changes:** Storage engine selection at creation

**Configuration Change:**
```cpp
// Old: Always use RocksDB (LSM-tree)
RocksDBWrapper::Config config;

// New: Choose storage engine
StorageEngineConfig config;
config.engine_type = EngineType::LSM;  // or EngineType::WOM
```

**Migration Steps:**
1. Update to v1.8.0
2. Default is still LSM-tree (no changes needed)
3. Test WOM tree on non-critical workloads
4. Migrate write-heavy workloads to WOM

**Timeline:** 12 months evaluation period

---

### v1.8.x → v2.0.x: Unified Storage Abstraction
**Breaking Changes:** Major API redesign

**Refactored API:**
```cpp
// Old: RocksDBWrapper directly
RocksDBWrapper db(config);

// New: Generic storage interface
auto storage = StorageFactory::create(config);
// Automatically selects best engine (LSM, WOM, etc.)
```

**Migration Steps:**
1. Update to v2.0.0
2. Replace RocksDBWrapper with StorageFactory
3. Update configuration format
4. Rebuild and test

**Automated Migration Tool:** `scripts/migrate_storage_v2.sh`

**Timeline:** 24 months deprecation period (v1.x maintained in parallel)

---

## Community Contributions Welcome

We welcome contributions in the following areas:

### High-Impact, Beginner-Friendly
- [x] Additional compression algorithms (Brotli, LZMA) — Brotli implemented in `CompressionStrategy`
- [x] Blob backend for Google Cloud Storage — `blob_backend_gcs.cpp` (requires `THEMIS_ENABLE_GCS`)
- [ ] Improved error messages and logging — ongoing
- [ ] Performance benchmarks for different workloads — ongoing

### Medium Complexity
- [ ] Automatic failover for blob backends
- [ ] PITR snapshot cleanup automation
- [x] Jittered exponential backoff for transaction retries — implemented in `TransactionRetryManager`
- [x] Additional merge operators (sets, counters) — `SetMergeOperator`, `CounterMergeOperator`, `AppendMergeOperator`, `MaxMergeOperator` in `merge_operators.cpp`

### Advanced Topics
- [x] Distributed transactions (Raft-based) — `DistributedTransactionManager` (v1.7.0)
- [x] Tiered storage implementation — `TieredStorageManager` (v1.6.0)
- [x] Erasure coding for blob storage — `ErasureCodingBackend` RS(k,m) (v1.7.0)
- [x] GPU-accelerated compression — `GpuCompressionManager` CUDA/ROCm with CPU fallback
- [x] NVMe optimizations (io_uring, ZNS) — `NVMeManager` (v1.6.0)

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for storage improvements? We'd love to hear from you:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 📊 Performance results: Share benchmarks in discussions

---

*Last Updated: April 2026*
*Module Version: v2.0.0*
*Next Review: v2.1.0 Release*

---

## Test Strategy

- Unit test coverage ≥ 80% for all storage classes: `MVCCStore`, `WALStorage`, `BackupManager`, `PITRManager`, and `BatchWriteOptimizer`
- Integration tests for read-after-write, PITR restore to a specific HLC timestamp, and cross-backend blob round-trips (Filesystem, S3-emulator, Azure-emulator)
- Fault-injection tests: kill process during WAL replay, corrupt SSTable block checksums, verify self-healing recovery without data loss
- Tiered storage integration test: write a key on hot tier, trigger migration to warm, verify read returns original value within 1 s
- Erasure coding round-trip test: encode with RS(4, 2), drop any 2 shards, verify full decode correctness
- Encryption regression test: confirm process fails at startup with `THEMIS_PRODUCTION_MODE` when no AES-256-GCM key is configured

## Performance Targets

- Sustained write throughput ≥ 100,000 ops/s on NVMe with batch size of 256 writes at 4 KB average value size
- p99 point-read latency ≤ 1 ms for hot-tier key lookups with bloom filter enabled
- Incremental backup throughput ≥ 500 MB/s on NVMe using parallel SSTable file copy
- Tiered-storage migration background I/O overhead ≤ 5% of sustained foreground write throughput
- Columnar scan throughput ≥ 4× scalar baseline on integer equality predicates with AVX2 SIMD (v2.0.0 target)
- Write amplification factor ≤ 10× under sustained uniform-random-write workload with level-based compaction

## Security / Reliability

- `THEMIS_PRODUCTION_MODE` startup check must reject any configuration without AES-256-GCM encryption; no silent degradation to plaintext storage
- All blob backend credentials (S3 access keys, Azure SAS, GCS ADC) must be loaded from environment variables or a secret store; plaintext credentials in config files are rejected
- HMAC-SHA256 tamper detection is verified on every read via `SecuritySignatureManager`; a mismatch returns `StorageError::TAMPERED` and raises an audit log event
- WAL replay must be idempotent: re-applying the same WAL sequence number must produce the same storage state without duplicate side effects
- `DiskSpaceMonitor` triggers write rejection at 95% disk capacity to prevent WAL and SSTable corruption from space exhaustion
- All backup bundles include a SHA-256 checksum manifest; restore operation aborts if any file checksum does not match the manifest

---

## 📚 Scientific Foundations

All planned features in this document are grounded in the following peer-reviewed research and industry specifications (IEEE format):

1. P. O'Neil, E. Cheng, D. Gawlick, and E. O'Neil, "The log-structured merge-tree (LSM-tree)," *Acta Informatica*, vol. 33, no. 4, pp. 351–385, 1996, doi: 10.1007/s002360050048.
   — Foundational design of RocksDB (`rocksdb_wrapper.cpp`); informs compaction strategy, write amplification trade-offs, and the `BatchWriteOptimizer`.

2. S. Dong, M. Callaghan, L. Galanis, D. Borthakur, T. Savor, and M. Strum, "Optimizing space amplification in RocksDB," in *Proc. 8th Biennial Conf. Innovative Data Systems Research (CIDR)*, 2017. [Online]. Available: https://www.cidrdb.org/cidr2017/papers/p82-dong-cidr17.pdf [Accessed: 2026-03-10]
   — Informs `CompactionManager` tuning, level-based compaction, and the `BlobDB` value separation for write amplification reduction.

3. D. Ongaro and J. Ousterhout, "In search of an understandable consensus algorithm," in *Proc. USENIX Annual Technical Conf. (ATC)*, 2014, pp. 305–319. [Online]. Available: https://raft.github.io/raft.pdf [Accessed: 2026-03-10]
   — Informs `RaftMVCCBridge` design and the planned two-phase-commit (2PC) distributed transactions (`DistributedTransactionManager`, ROADMAP v1.7.0).

4. J. N. Gray and A. Reuter, *Transaction Processing: Concepts and Techniques*. San Mateo, CA: Morgan Kaufmann, 1992.
   — Informs 2PC coordinator/participant protocol design for cross-shard atomicity and the `TransactionRetryManager` exponential backoff strategy.

5. I. S. Reed and G. Solomon, "Polynomial codes over certain finite fields," *J. SIAM*, vol. 8, no. 2, pp. 300–304, 1960, doi: 10.1137/0108018.
   — Theoretical foundation for the planned Reed-Solomon erasure coding in `BlobRedundancyManager` (ROADMAP v1.7.0, `ErasureCodingConfig`).

6. A. G. Dimakis, P. B. Godfrey, Y. Wu, M. J. Wainwright, and K. Ramchandran, "Network coding for distributed storage systems," *IEEE Trans. Inf. Theory*, vol. 56, no. 9, pp. 4539–4551, Sep. 2010, doi: 10.1109/TIT.2010.2054295.
   — Informs minimum storage regenerating (MSR) codes for the erasure coding redundancy mode; motivates RS(4,2) default parameter choice.

7. A. Verbitski et al., "Amazon Aurora: Design considerations for high throughput cloud-native relational databases," in *Proc. ACM SIGMOD Int. Conf. Management of Data*, 2017, pp. 1041–1052, doi: 10.1145/3035918.3056101.
   — Informs `TieredStorageManager` design (hot/warm/cold with background migration) and the PITR restore architecture.

8. T. Kraska, A. Beutel, E. H. Chi, J. Dean, and N. Polyzotis, "The case for learned index structures," in *Proc. ACM SIGMOD Int. Conf. Management of Data*, 2018, pp. 489–504, doi: 10.1145/3183713.3196909.
   — Research basis for the `Learned Index Structures` exploration area; motivates replacing B-trees with ML models for RocksDB SSTable position prediction.

9. V. Balmau, D. Didona, R. Guerraoui, W. Zwaenepoel, H. Yuan, A. Arora, K. Rao, and P. Tsuru, "TRIAD: Creating synergies between memory, disk and log in log structured key-value stores," in *Proc. USENIX Annual Technical Conf. (ATC)*, 2017, pp. 363–375.
   — Informs the `Write-Optimized Merge (WOM) Tree` research area; motivates alternative compaction strategies for update-heavy workloads.

10. A. Kemper and T. Neumann, "HyPer: A hybrid OLTP&OLAP main memory database system based on virtual memory snapshots," in *Proc. IEEE 27th Int. Conf. Data Engineering (ICDE)*, 2011, pp. 195–206, doi: 10.1109/ICDE.2011.5767867.
    — Informs MVCC snapshot isolation design in `MVCCStore` and motivates the planned `Multi-Version B-Trees (MVBT)` research area.

11. P. Boncz, M. Zukowski, and N. Nes, "MonetDB/X100: Hyper-pipelining query execution," in *Proc. 2nd Biennial Conf. Innovative Data Systems Research (CIDR)*, 2005, pp. 225–237.
    — Informs the planned vectorized execution (AVX2 SIMD) in `ColumnarFormat` (ROADMAP v2.0.0, `simd_filter.cpp`).

12. J. Willhalm, N. Popovici, Y. Boshmaf, H. Plattner, A. Zeier, and J. Schaffner, "SIMD-scan: Ultra fast in-memory table scan using on-chip vector processing units," *Proc. VLDB Endow.*, vol. 2, no. 1, pp. 385–394, 2009, doi: 10.14778/1687627.1687671.
    — Provides algorithmic foundations for SIMD-accelerated integer equality and range predicates targeting the `ColumnarFormat` scan throughput goal (≥4× scalar baseline).

13. Apache Software Foundation, "Apache Parquet format specification," Apache Parquet, 2023. [Online]. Available: https://parquet.apache.org/docs/file-format/ [Accessed: 2026-03-10]
    — Specification basis for the planned native Parquet export from `ColumnarFormat` (ROADMAP v2.0.0, `parquet_exporter.cpp`).

14. A. Coviello et al. (NIST), "Module-lattice-based key-encapsulation mechanism standard (FIPS 203)," National Institute of Standards and Technology, 2024, doi: 10.6028/NIST.FIPS.203.
    — Defines CRYSTALS-Kyber (ML-KEM) standard referenced in the `Quantum-Resistant Encryption` research area.

15. D. Bernhard, T. Jager, A. Lehmann, and M. Püschel (NIST), "Module-lattice-based digital signature standard (FIPS 204)," National Institute of Standards and Technology, 2024, doi: 10.6028/NIST.FIPS.204.
    — Defines CRYSTALS-Dilithium (ML-DSA) for post-quantum signatures; relevant to the `Quantum-Resistant Encryption` research area in `security_signature.cpp`.

16. J. Axboe, "Efficient I/O with io_uring," 2019. [Online]. Available: https://kernel.dk/io_uring.pdf [Accessed: 2026-03-10]
    — Basis for the `NVMe Optimizations` (`io_uring` kernel bypass) and Zero-Copy Blob Transfers performance area.

17. P. Mishra, U. Roesler, J. Luo, and R. Zhao, "CXL: Enabling innovations in memory through an open industry-standard interconnect," *IEEE Micro*, vol. 41, no. 3, pp. 8–17, May–Jun. 2021, doi: 10.1109/MM.2021.3059102.
    — Reference for the `CXL (Compute Express Link) Integration` research area; motivates disaggregated shared block-cache across nodes.

## See Also

- [`src/storage/README.md`](README.md) — Module overview and API entry points
- [`src/storage/ARCHITECTURE.md`](ARCHITECTURE.md) — Component diagram and data flow
- [`src/storage/ROADMAP.md`](ROADMAP.md) — Implementation phases and planned milestones
- [`docs/de/storage/README.md`](../../../docs/de/storage/README.md) — German-language secondary documentation
- [`docs/de/storage/MISSING_IMPLEMENTATIONS.md`](../../../docs/de/storage/MISSING_IMPLEMENTATIONS.md) — Reality-check findings

---

## Security Hardening Backlog (Q3 2026)

> GAP-015 – identified via static analysis (2026-04-21).
> Reference: `docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md`.

### GAP-015 – Replace `system(tar)` with libarchive in BackupManager

**Scope:** `src/storage/backup_manager.cpp:940,979`

### Design Constraints
- libarchive (BSD licence) is already indirectly available in the dependency tree;
  it must be added as an explicit CMake target dependency
- Backup/restore semantics (gzip-compressed tarball) must be preserved
- The "directory" parameter from `POST /admin/backup` must be sandboxed to a
  configurable backup root before being passed to `BackupManager`

### Required Interfaces
```cpp
// New helper replacing system(tar):
Result<std::string> archiveDirectory(const std::filesystem::path& src_dir,
                                      const std::filesystem::path& dest_archive);
Result<std::string> extractArchive(const std::filesystem::path& src_archive,
                                    const std::filesystem::path& dest_dir);
```
- Uses `archive_write_open_filename` + `archive_write_add_filter_gzip` +
  `archive_write_set_format_pax_restricted` from `<archive.h>`

### Implementation Notes
- **Sandbox check**: before calling `BackupManager`, `admin_api_handler.cpp` must
  validate that `body["directory"]` is within `config_.backup_root_dir`:
  ```cpp
  if (!std::filesystem::weakly_canonical(dir).string().starts_with(backup_root)) {
      return makeErrorResponse(400, "backup path out of sandbox");
  }
  ```
- `system()` calls are unsafe even with double-quoted arguments because a newline
  in `backup_dir` terminates the command and starts a new shell command

### Test Strategy
- Unit test: create a temp directory, archive it via `archiveDirectory`, extract it
  via `extractArchive`, compare file trees
- Unit test: path `"../../etc"` as backup dir → 400 from handler, no archive created
- Fuzz test: random path strings as `backup_dir` → no shell command executed

### Performance Targets
- Throughput: ≥ gzip(1) baseline (libarchive uses the same zlib backend)
- No measurable regression in CI backup integration tests

### Security / Reliability
- No shell process spawned; OS signal delivery to child process cannot escape the parent
- On libarchive error: return structured error, no partial archive left on disk
