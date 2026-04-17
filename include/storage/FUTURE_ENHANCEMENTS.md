# Storage Module - Future Enhancements

## Scope

- API-level enhancements to `include/storage/` headers
- Tiered storage API (`TieredStorageManager`, transparent hot/warm/cold migration)
- WAL compaction interface (`WALCompactor`, log truncation and archival API)
- Encryption-at-rest API (`StorageEncryptionConfig`, configured at storage initialization)
- Column family management interface (`ColumnFamilyManager`, thread-safe operations)
- Backup/restore API (`BackupManager`, async initiation with HMAC-verified restore)

## Design Constraints

- [ ] Tiered storage API is transparent to callers — reads/writes use the same interface regardless of tier
- [ ] Encryption-at-rest is configured at storage initialization; cannot be toggled at runtime
- [ ] Column family API is fully thread-safe — no external locking required by callers
- [ ] Backup API is async — `initiateBackup()` returns immediately; progress via callback/future
- [ ] Storage API header types never embed raw encryption keys — only opaque key identifiers allowed
- [ ] All public API operations return `Result<T>` for error propagation (no exceptions)

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `TieredStorageManager` | Database engine, archival pipelines | Transparent to callers; policy-driven migration |
| `StorageEncryptionConfig` | Storage initialization, admin tooling | Mandatory in `THEMIS_PRODUCTION_MODE` |
| `ColumnFamilyManager` | Schema evolution, multi-tenant storage | Thread-safe; supports online add/drop |
| `BackupManager` | Ops tooling, disaster recovery | Async; restore verified via HMAC before apply |
| `WALCompactor` | Compaction subsystem, log archival | Non-blocking log truncation |

## Planned Features

### Distributed Transactions with Raft
**Priority:** High  
**Target Version:** v1.7.0

Replace single-node RocksDB transactions with Raft-based distributed consensus.

**Features:**
- Multi-shard atomic transactions
- Cross-datacenter coordination
- ACID guarantees across distributed nodes
- Two-phase commit with Raft log replication

**Implementation:**
```cpp
// Future API
DistributedTransactionManager dtx(raft_cluster);
auto tx = dtx.beginDistributedTransaction();
tx->write("shard1:users:123", user_data);
tx->write("shard2:orders:456", order_data);
tx->commit();  // Atomically commits across shards via Raft
```

**Benefits:**
- Horizontal scalability without sacrificing ACID
- Automatic failover and recovery
- Strong consistency across shards

---

### Tiered Storage (Hot/Warm/Cold)
**Priority:** High  
**Target Version:** v1.7.0

Automatic data migration between storage tiers based on access patterns.

**Storage Tiers:**
- **Hot**: NVMe SSD (L0-L2 SSTables) - <1ms latency
- **Warm**: SATA SSD (L3-L4 SSTables) - ~5ms latency
- **Cold**: Object storage (S3/Azure) - ~50ms latency
- **Archive**: Glacier/Archive tier - minutes to retrieve

**Policy-Based Migration:**
```cpp
TieredStorageConfig config;
config.hot_tier_max_age = std::chrono::hours(24);      // Move to warm after 1 day
config.warm_tier_max_age = std::chrono::days(30);      // Move to cold after 30 days
config.cold_tier_max_age = std::chrono::days(365);     // Archive after 1 year
config.hot_tier_access_count_threshold = 100;          // Keep hot if accessed frequently

TieredStorageManager tiered(config);
tiered.applyPolicies();  // Automatically migrates data between tiers
```

**Expected Savings:** 70-90% storage cost reduction for time-series and archival data

---

### GPU-Accelerated Compression
**Priority:** Medium  
**Target Version:** v1.8.0

Hardware-accelerated compression/decompression using NVIDIA GPUs.

**Supported Algorithms:**
- nvCOMP (NVIDIA) - LZ4, Snappy, Zstd on GPU
- Deflate/Gzip with GPU acceleration
- Custom neural compression models

**Performance Target:**
- 10-50x faster compression vs CPU
- 5-20x faster decompression
- Especially effective for columnar data

**Use Cases:**
- Real-time data ingestion compression
- Batch compression of historical data
- Columnar format compression for analytics

---

### Erasure Coding for Blob Storage
**Priority:** High  
**Target Version:** v1.7.0

Replace RAID-1 mirroring with Reed-Solomon erasure coding for space efficiency.

**Encoding Schemes:**
- **6+3**: 6 data chunks + 3 parity chunks (50% overhead vs 200% for mirroring)
- **10+4**: 10 data + 4 parity (40% overhead)
- **14+2**: 14 data + 2 parity (14% overhead, less fault tolerance)

**Implementation:**
```cpp
ErasureCodingConfig config;
config.data_shards = 6;
config.parity_shards = 3;
config.chunk_size_mb = 64;

BlobErasureCoding encoder(config);
auto chunks = encoder.encode(large_blob);
// Store chunks across multiple S3 buckets/Azure containers
for (auto& chunk : chunks) {
    blob_backend->put(chunk.id, chunk.data);
}

// Retrieve with fault tolerance
auto reconstructed = encoder.decode(available_chunks);
```

**Benefits:**
- 50-70% space savings vs mirroring
- Tolerates multiple backend failures
- Configurable fault tolerance

---

### Zero-Copy Large Value Access
**Priority:** Medium  
**Target Version:** v1.8.0

Memory-mapped access to BlobDB without copying into memory.

**Current:** Read large value → Copy to buffer → Return to caller  
**Target:** Memory-map .blob file → Return pointer to mapped region

**API:**
```cpp
// Zero-copy read
BlobMemoryMap map = blobdb->mmap("blob-123");
const uint8_t* data = map.data();  // Direct pointer to file
size_t size = map.size();
// No copy, no heap allocation
```

**Expected Improvement:** 80% reduction in memory bandwidth for large values

---

### Automatic Index Recommendation
**Priority:** Medium  
**Target Version:** v1.8.0

ML-based index recommendation based on query patterns.

**Features:**
- Analyze query logs to identify slow queries
- Recommend missing indexes
- Estimate index overhead (space, write amp)
- Auto-create indexes with administrator approval

**Workflow:**
```cpp
IndexRecommendationEngine engine;
engine.analyzeQueryLog(query_log, std::chrono::days(7));
auto recommendations = engine.generateRecommendations();

for (auto& rec : recommendations) {
    logger->info("Recommend index on {}.{} - estimated speedup: {}x",
                 rec.table, rec.column, rec.speedup_factor);
    if (rec.confidence > 0.8 && auto_create_enabled) {
        index_manager->createIndex(rec.table, rec.column, rec.index_type);
    }
}
```

---

### Online Schema Evolution
**Priority:** High  
**Target Version:** v1.7.0

Add/remove columns without blocking reads/writes.

**Operations:**
- Add column (backward compatible)
- Remove column (forward compatible with reader versioning)
- Change column type (with data migration)
- Add/remove indexes (non-blocking)

**Implementation:**
```cpp
SchemaEvolution evolution(storage);

// Add column without downtime
evolution.addColumn("users", "phone_number", ColumnType::STRING, 
                    /*default_value=*/"", /*nullable=*/true);
// Reads continue with old schema, writes use new schema
// Background backfill process adds default values

// Remove column safely
evolution.deprecateColumn("users", "legacy_field");
// Marked for deletion, invisible to new queries
// Background garbage collection removes after retention period
```

---

### Write-Optimized Columnar Format
**Priority:** Medium  
**Target Version:** v1.8.0

Hybrid row-column storage for analytics workloads.

**Strategy:**
- Recent data (<1 day): Row format in RocksDB
- Historical data (>1 day): Columnar format in Parquet
- Automatic conversion via background compaction

**Query Rewrite:**
```sql
-- Automatically queries both row and columnar storage
SELECT user_id, SUM(amount) FROM orders
WHERE order_date > '2024-01-01'
GROUP BY user_id;

-- Execution plan:
-- 1. Recent orders from RocksDB (row format)
-- 2. Historical orders from Parquet (columnar)
-- 3. Merge results
```

**Expected Performance:** 5-50x faster for OLAP queries on historical data

---

## Performance Optimizations

### Adaptive Compaction Scheduling
**Priority:** High  
**Target Version:** v1.7.0

Machine learning model to predict optimal compaction timing.

**Current:** Fixed compaction triggers based on level sizes  
**Target:** Adaptive triggers based on workload characteristics

**Approach:**
- Monitor query patterns (OLTP vs OLAP)
- Predict future write/read rates
- Schedule compaction during low-traffic periods
- Balance write amplification vs read performance

**Expected Improvement:** 20-40% reduction in compaction overhead

---

### NUMA-Aware Memory Allocation
**Priority:** Medium  
**Target Version:** v1.7.0

Optimize memory allocation for multi-socket NUMA systems.

**Features:**
- Pin memtables to NUMA nodes
- Thread affinity for compaction threads
- Local memory allocation for block cache
- Cross-socket communication minimization

**Expected Improvement:** 15-30% throughput increase on NUMA systems

---

### Intel QuickAssist Integration
**Priority:** Low  
**Target Version:** v1.9.0

Hardware-accelerated compression and encryption via Intel QAT.

**Benefits:**
- Offload compression to dedicated hardware
- AES-NI acceleration for field encryption
- Reduce CPU overhead for crypto operations
- 5-10x compression throughput increase

---

### Bloom Filter Optimization
**Priority:** Medium  
**Target Version:** v1.7.0

Adaptive Bloom filter sizing based on working set.

**Current:** Fixed false positive rate (1%)  
**Target:** Dynamic FP rate based on cache hit ratio

**Logic:**
- High cache hit rate → Smaller Bloom filters (less memory)
- Low cache hit rate → Larger Bloom filters (avoid disk I/O)
- Per-SSTable FP rate tuning

**Expected Improvement:** 10-20% reduction in memory usage or I/O

---

### Transparent Huge Pages Support
**Priority:** Medium  
**Target Version:** v1.7.0

Leverage 2MB/1GB huge pages for block cache.

**Benefits:**
- Reduced TLB misses
- Lower memory management overhead
- Better performance for large caches (>10GB)

**Configuration:**
```cpp
RocksDBWrapper::Config config;
config.block_cache_use_huge_pages = true;
config.memtable_use_huge_pages = true;
```

**Expected Improvement:** 5-15% throughput increase with large caches

---

## Refactoring Opportunities

### Separate BlobDB into Standalone Library
**Priority:** Medium  
**Target Version:** v1.8.0

Extract blob storage into independent library for reuse.

**Motivation:**
- Reuse blob storage in other projects
- Independent versioning and releases
- Simpler testing and maintenance
- Pluggable blob backends

**New Structure:**
```
libthemis-blob/
├── blob_manager.{cpp,h}
├── backends/
│   ├── s3_backend.{cpp,h}
│   ├── azure_backend.{cpp,h}
│   ├── filesystem_backend.{cpp,h}
│   └── webdav_backend.{cpp,h}
└── erasure_coding.{cpp,h}
```

---

### Unified Transaction Interface
**Priority:** High  
**Target Version:** v1.7.0

Abstract transaction API supporting both local and distributed TX.

**Current:** Separate APIs for RocksDB transactions and distributed TX  
**Target:** Unified ITransaction interface

**Benefits:**
- Seamless migration from single-node to distributed
- Testability (mock transactions)
- Support multiple transaction backends

---

### Pluggable Storage Backend
**Priority:** Low  
**Target Version:** v1.9.0

Abstract storage interface to support backends beyond RocksDB.

**Potential Backends:**
- **RocksDB**: Current default (LSM-tree)
- **LevelDB**: Lighter alternative
- **PebbleDB**: Go-based LSM-tree
- **BoltDB**: Pure key-value store
- **LMDB**: Memory-mapped B-tree

**Interface:**
```cpp
class IStorageBackend {
public:
    virtual Result<void> put(const std::string& key, const std::string& value) = 0;
    virtual Result<std::string> get(const std::string& key) = 0;
    virtual std::unique_ptr<IIterator> createIterator() = 0;
    virtual std::unique_ptr<ITransaction> beginTransaction() = 0;
};
```

---

### Decouple Encryption from Storage
**Priority:** Medium  
**Target Version:** v1.8.0

Move encryption logic from StorageEngine to dedicated layer.

**Benefits:**
- Cleaner separation of concerns
- Pluggable encryption strategies
- Easier testing without storage dependency
- Support for multiple encryption algorithms

---

## Known Issues

### Issue #1: Write Amplification on Small Updates
**Severity:** Medium  
**Reported:** v1.5.0

Point updates to large documents cause full document rewrite.

**Example:**
```cpp
// Update single field in 1MB document
// Current: Read 1MB → Modify → Write 1MB → Compaction writes 1MB × 10
// Amplification: 10MB+ written for 10-byte update
```

**Workaround:** Use separate keys for frequently updated fields  
**Fix:** Delta encoding for document updates (v1.7.0)

**Planned Fix:** v1.7.0 - Implement delta compression for document updates

---

### Issue #2: Backup Verification Performance
**Severity:** Low  
**Reported:** v1.5.1

Backup verification (`verifyBackup()`) is slow for large databases.

**Current:** O(N) checksum validation of all SSTables  
**Impact:** Hours for multi-TB databases

**Workaround:** Skip verification or verify in background  
**Fix:** Incremental verification with sampling

**Planned Fix:** v1.7.0

---

### Issue #3: BlobDB Compaction Stalls
**Severity:** High  
**Reported:** v1.5.2

RocksDB compaction can stall waiting for BlobDB garbage collection.

**Symptoms:**
- Write stalls during heavy BlobDB usage
- "Stall due to compaction" messages in logs

**Workaround:** Increase `max_background_jobs` and `blob_gc_force_threshold`  
**Fix:** Decouple BlobDB GC from SSTable compaction

**Planned Fix:** v1.6.1 (hotfix planned)

---

### Issue #4: Key Schema Migration Complexity
**Severity:** Medium  
**Reported:** v1.5.0

Migrating between key schema versions requires full database rewrite.

**Example:** Changing from `doc:collection:id` to `doc:v2:collection:id`

**Workaround:** Use key rewriting during compaction  
**Fix:** Lazy migration with transparent key translation

**Planned Fix:** v1.7.0

---

### Issue #5: Async I/O on Windows
**Severity:** Low  
**Reported:** v1.4.2

Async I/O optimizations not available on Windows platform.

**Impact:** 30-50% slower scans on Windows vs Linux

**Workaround:** Use Linux/WSL2 for production  
**Fix:** Implement Windows IOCP-based async I/O

**Planned Fix:** v1.8.0

---

## Research Areas

### Learned Index Structures
**Focus:** Replace B-trees with ML models

**Concept:**
- Train neural network to predict key location
- O(1) lookup instead of O(log N)
- Smaller memory footprint than traditional indexes

**Research Questions:**
- How to handle insertions/updates efficiently?
- What's the training overhead for dynamic workloads?
- Can we achieve <10μs lookup latency?

**References:**
- "The Case for Learned Index Structures" (Kraska et al., 2018)
- "ALEX: Adaptive Learned Index" (Ding et al., 2020)

---

### Persistent Memory (PMEM) Integration
**Focus:** Intel Optane / byte-addressable NVM

**Opportunities:**
- Use PMEM for WAL (persistent, low-latency)
- Bypass page cache with direct PMEM access
- Hybrid DRAM+PMEM for memtables

**Research Questions:**
- How to optimize for PMEM's asymmetric read/write latency?
- What data structures work best on PMEM?
- Cost-benefit vs NVMe SSDs?

---

### Differential Compression
**Focus:** Delta encoding for similar documents

**Approach:**
- Detect similar documents (e.g., versions of same entity)
- Store diff instead of full document
- Reconstruct on read by applying diff chain

**Use Cases:**
- Version control (snapshots, branches)
- Time-series with slowly changing values
- Document versioning

**Research Questions:**
- How to bound diff chain length?
- What diff algorithm (xdelta, bsdiff, custom)?
- Performance impact on reads?

---

### Write-Behind Caching
**Focus:** Battery-backed write caching

**Concept:**
- Write to NVRAM cache immediately (ack client)
- Flush to RocksDB asynchronously in background
- Crash recovery from NVRAM

**Benefits:**
- Sub-microsecond write latency
- Higher write throughput
- Lower write amplification (better batching)

**Research Questions:**
- How to guarantee durability?
- What's the optimal cache size?
- Integration with RocksDB WAL?

---

### Automatic Data Partitioning
**Focus:** Intelligent data placement

**Approach:**
- Analyze access patterns
- Co-locate frequently accessed keys
- Partition based on temperature (hot/cold)
- Minimize cross-partition queries

**Research Questions:**
- How to detect co-access patterns?
- What's the overhead of dynamic repartitioning?
- Can we predict future access patterns?

---

## Migration Paths

### v1.5.x → v1.6.x: BlobDB Improvements
**Breaking Changes:** BlobRef serialization format

**Old Format (v1.5.x):**
```cpp
struct BlobRef {
    std::string blob_id;
    BlobStorageType type;
    std::string uri;
};
```

**New Format (v1.6.x):**
```cpp
struct BlobRef {
    std::string blob_id;
    BlobStorageType type;
    std::string uri;
    size_t size_bytes;           // NEW
    std::string sha256_hash;     // NEW
    std::string compression;     // NEW
};
```

**Migration Steps:**
1. Upgrade to v1.5.5 (includes forward-compatible BlobRef parser)
2. Run `themis-admin migrate-blobref --dry-run` to test migration
3. Upgrade to v1.6.0
4. Run `themis-admin migrate-blobref --execute` to upgrade BlobRefs
5. Verify: `themis-admin verify-blobref-migration`

**Timeline:** v1.5.5 available 2 months before v1.6.0

---

### v1.6.x → v1.7.x: Key Schema Evolution
**Breaking Changes:** Key prefix format changes

**Old Key Schema:**
```
rel:table:pk
doc:collection:pk
node:pk
edge:pk
```

**New Key Schema (v1.7.0):**
```
v2:rel:table:pk
v2:doc:collection:pk
v2:node:pk
v2:edge:pk
```

**Why:** Version prefix enables future schema changes without full rewrites

**Migration Steps:**
1. Upgrade to v1.6.5 (includes dual-schema reader)
2. Enable dual-write mode: `THEMIS_SCHEMA_DUAL_WRITE=1`
3. Wait for all data to be dual-written (monitor `themis_dual_write_percentage` metric)
4. Switch to v2 reads: `THEMIS_SCHEMA_VERSION=v2`
5. Disable v1 writes: `THEMIS_SCHEMA_DUAL_WRITE=0`
6. Garbage collect old keys: `themis-admin gc-old-schema --version=v1`

**Timeline:** 6-month migration window (v1.6.5 to v1.7.6)

---

### v1.7.x → v1.8.x: Distributed Transactions
**Breaking Changes:** Transaction API signatures

**Old API:**
```cpp
auto tx = db->beginTransaction();
tx->put(key, value);
tx->commit();
```

**New API:**
```cpp
// Local transaction (unchanged)
auto tx = db->beginLocalTransaction();
tx->put(key, value);
tx->commit();

// Distributed transaction (new)
auto dtx = cluster->beginDistributedTransaction();
dtx->put("shard1:key", value1);
dtx->put("shard2:key", value2);
dtx->commit();  // Two-phase commit via Raft
```

**Migration Steps:**
1. Update code to use `beginLocalTransaction()` for existing transactions
2. Run provided codemod: `scripts/migrate_transaction_api_v18.sh`
3. Rebuild and test
4. Optionally adopt distributed transactions for multi-shard operations

**Timeline:** 12 months deprecation period (v1.7.x still supports old API)

---

## Community Contributions Welcome

### High-Impact, Beginner-Friendly
- [ ] Additional blob storage backends (GCS, DigitalOcean Spaces)
- [ ] Improved backup verification with sampling
- [ ] Compaction statistics dashboard
- [ ] Storage metrics exporter (Grafana dashboards)

### Medium Complexity
- [ ] Erasure coding implementation
- [ ] Tiered storage policy engine
- [ ] Zero-copy blob access via mmap
- [ ] Online schema evolution framework

### Advanced Topics
- [ ] Distributed transactions with Raft
- [ ] GPU-accelerated compression
- [ ] Learned index structures
- [ ] Persistent memory integration

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for storage module improvements? Open an issue or discussion:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

---

*Last Updated: April 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.7.0 Release*

## Test Strategy

- Unit tests for `TieredStorageManager` policy engine with synthetic access-pattern timestamps
- Thread-safety tests for `ColumnFamilyManager` under concurrent add/drop operations (≥ 20 threads)
- Encryption-at-rest tests verifying ciphertext on disk and successful plaintext roundtrip
- Backup/restore integration tests with injected HMAC corruption (verify backup is rejected)
- WAL compaction tests confirming no data loss across truncation boundary
- Tiered migration performance tests measuring migration throughput against the 5 s/GB target

## Performance Targets

- Write batch throughput ≥ 100,000 ops/s on NVMe with encryption-at-rest enabled
- Tiered data migration ≤ 5 s/GB (hot → warm tier)
- Backup initiation ≤ 100 ms (async; must not block concurrent writes)
- Column family creation ≤ 50 ms online with no write stall
- WAL compaction cycle ≤ 200 ms for logs up to 1 GB

## Security / Reliability

- Encryption-at-rest is mandatory when `THEMIS_PRODUCTION_MODE` is set; startup fails otherwise
- Backup files verified via HMAC before restore is applied — corrupted backups are rejected
- Storage API header types never contain raw encryption keys; only opaque key identifiers
- Column family operations are idempotent — duplicate add/drop calls are safe
- Tiered storage migration is atomic at the SSTable level; partial migrations roll back on failure

---

## Paper 2 — Layer 6: SchemaDeadWeightDetector & Layer 10: StorageLayoutAdvisor

> Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 6 & 10
> Issues: `docs/issues/optimization_layers/IMPL-B6-schema-deadweight.md` · `docs/issues/optimization_layers/IMPL-B10-layout-advisor.md`

### SchemaDeadWeightDetector (IMPL-B6)
- 180-day rolling access window; fields with 0 reads AND 0 writes → archival candidate
- GDPR-tagged fields: always `recommendation = RETAIN`, regardless of access count
- Seasonal protection: if prior 90-day window had any access, `seasonal` flag prevents auto-archival
- Cross-shard aggregation: `setShardAccessSource(IShardAccessStats*)` merges access data from peer shards (Layer 11C)
- Audit: every archival recommendation written to `AIDecisionAuditor`

### StorageLayoutAdvisor (IMPL-B10)
- Columnar recommendation: temporal time-series with ≥ 10 k rows/day write rate
- Compression estimate: ≥ 50 % space improvement for columnar vs. row-store for analytics workloads
- `LayoutType` enum: `ROW_STORE`, `COLUMNAR`, `HYBRID`, `COMPRESSED_COLUMNAR`, `TIERED`
- Integrates with `distributed_knowledge` Layer 11C `FederatedRAGMerger` for cross-shard layout hints

### Performance Targets
- `SchemaDeadWeightDetector::analyzeCollection()` ≤ 50 ms for 1 000-field collection
- `StorageLayoutAdvisor::adviseLayout()` ≤ 10 ms per collection profile
