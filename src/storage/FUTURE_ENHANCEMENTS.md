# Storage Module - Future Enhancements

## Planned Features

### Distributed Transactions
**Priority:** High  
**Target Version:** v1.7.0

Raft-based distributed transactions across multiple nodes.

**Features:**
- Two-phase commit (2PC) protocol
- Raft consensus for transaction coordination
- Cross-shard atomic operations
- Automatic deadlock detection

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

Space-efficient redundancy using erasure codes (e.g., Reed-Solomon).

**Encoding Schemes:**
- **RS(10,4)**: 10 data + 4 parity blocks (40% overhead vs 200% for mirroring)
- **RS(6,3)**: 6 data + 3 parity blocks (50% overhead)
- **RS(4,2)**: 4 data + 2 parity blocks (50% overhead, faster)

**Benefits:**
- 50-70% storage savings vs mirroring
- Survives multiple node failures
- Configurable fault tolerance

**Example:**
```cpp
ErasureCodingConfig config;
config.data_blocks = 10;
config.parity_blocks = 4;

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

Alternative to LSM-tree for write-heavy workloads.

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

Use CUDA/ROCm for parallel compression/decompression.

**Target Algorithms:**
- Zstd (NVIDIA nvCOMP library)
- Snappy (GPU-accelerated variant)
- LZ4 (parallel decompress)

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
**Severity:** High  
**Reported:** v1.5.0

Columnar storage has unresolved correctness issues.

**Workaround:** Do not use in production

**Fix:** Complete testing and validation

**Planned Fix:** v1.7.0

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
- [ ] Additional compression algorithms (Brotli, LZMA)
- [ ] Blob backend for Google Cloud Storage
- [ ] Improved error messages and logging
- [ ] Performance benchmarks for different workloads

### Medium Complexity
- [ ] Automatic failover for blob backends
- [ ] PITR snapshot cleanup automation
- [ ] Jittered exponential backoff for transaction retries
- [ ] Additional merge operators (sets, counters)

### Advanced Topics
- [ ] Distributed transactions (Raft-based)
- [ ] Tiered storage implementation
- [ ] Erasure coding for blob storage
- [ ] GPU-accelerated compression
- [ ] NVMe optimizations (io_uring, ZNS)

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for storage improvements? We'd love to hear from you:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 📊 Performance results: Share benchmarks in discussions

---

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
