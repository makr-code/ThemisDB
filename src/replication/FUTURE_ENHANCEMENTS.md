> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Replication Module - Future Enhancements

- Raft-like leader election and follower management with automatic failover and heartbeat-based health monitoring
- SYNC, SEMI_SYNC, and ASYNC replication modes with configurable `min_sync_replicas` for quorum writes
- WAL shipping to followers with Zstd compression and point-in-time recovery (PITR) via WAL replay
- Conflict detection using vector clocks and Hybrid Logical Clocks (HLC) in multi-master topologies
- CRDT-based conflict resolution (G-Counter, LWW-Register, OR-Set) plus custom resolver hooks
- Change Data Capture (CDC) with per-operation type filtering (INSERT/UPDATE/DELETE) for ETL pipelines
- Cross-datacenter and cross-region replication with Raft leader lease reads for linearizable read scale-out
- Selective replication filtered by collection, tenant pattern, or CDC event type

## Design Constraints

- [ ] Replication lag at p99 must not exceed 50 ms under 10,000 write/s in SEMI_SYNC mode on a 3-node LAN cluster (requires integration environment; WAL append throughput prerequisite validated by `WALAppendThroughputPerfTest`)
- [ ] WAL shipping must sustain ≥ 500 MB/s compressed throughput per follower connection (requires network environment; in-process Zstd throughput validated by `CompressedStreamThroughputPerfTest`)
- [ ] Leader failover must complete (new leader elected + followers re-pointed) within 10 s under default heartbeat settings (requires distributed cluster; configured `heartbeat_interval_ms=1000`, `election_timeout_min_ms=3000` in `ReplicationConfig`)
- [x] Vector clock comparison and HLC conflict detection must add < 5 µs per write operation (validated by `VectorClockPerfTest::IncrementAndCompareSingleOpUnder5us` and `HLCPerfTest::NowCallUnder5us` — set `THEMIS_RUN_PERF_TESTS=1`)
- [x] CRDT merge operations must be idempotent and commutative; incorrect usage must produce a compile-time error where possible
- [x] CDC event emission must not block the write path; use a dedicated async queue with configurable max depth
- [x] Selective replication filters must be evaluated in O(1) per write using a pre-compiled pattern set
- [x] Full Raft v2 (joint consensus) must not break existing `ReplicationConfig`; new fields are additive only

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ReplicationManager::start_election()` | HA watchdog, operator | Initiates Raft-like vote; resolves in < 10 s on healthy cluster |
| `ReplicationManager::set_mode(mode)` | Admin API, operator CRD | SYNC / SEMI_SYNC / ASYNC; applied without restart |
| `WALShipper::ship(segment, follower)` | Replication background thread | Zstd-compressed; retries on transient network failure |
| `ConflictResolver::resolve(local, remote)` | Multi-master write path | Selects LWW / CRDT / custom strategy from `ReplicationConfig` |
| `CDCStream::subscribe(filter, cb)` | ETL consumers, Kafka bridge | Async queue; filter by op-type and collection pattern |
| `PITRManager::restore(timestamp)` | Admin API, disaster recovery | Replays WAL segments up to `timestamp`; read-only replay mode |
| `ReplicationSlot::pause() / resume()` | Admin API | Slot management without data loss; slot state persisted to WAL |
| `TopologyVisualizer::export_json()` | Web UI, monitoring | Returns cluster graph as JSON; updated on each topology change |

## Planned Features

### Logical Replication
**Priority:** High
**Target Version:** v1.7.0

#### Scope
- Schema-aware logical replication managed by `LogicalReplicationManager`, with slot state persisted under `<wal_directory>/logical_slots`.
- Slot-level include/exclude collections plus row predicates for multi-tenant filtering and bandwidth reduction.
- Cross-version streaming metadata (`source_version` v1.5 → `target_version` v1.6) with optional transformation hooks for schema evolution.
- Conflict-free initial sync using snapshot buffers and per-slot dedup keyed by collection + document ID.
- DDL streaming routed to subscribers alongside DML.

#### Features (Acceptance)
- [x] Schema-aware replication (replicate DDL changes)
- [x] Selective table/collection replication with filters
- [x] Cross-version replication (v1.5 → v1.6)
- [x] Data transformation during replication
- [x] Conflict-free initial sync for new replicas
- [x] Replicate only relevant data (reduce bandwidth and storage)
- [x] Enable multi-tenant replication (separate replica per tenant)
- [x] Easier upgrades (replicate from old version to new version)
- [x] Integrate with external systems (Kafka, Elasticsearch, Snowflake)
- [x] Use output plugins for different formats (JSON, Protobuf, Avro)
- [x] Maintain replication slots persistently
- [x] Support parallel decoding for high throughput

#### Design Constraints
- Row-filter evaluation must remain O(1) per change using lightweight predicate parsing.
- Slot persistence must survive restart without blocking WAL writers; JSON state is fsync-safe under WAL directory.
- Parallel decoding remains future work; current implementation decodes inline and cannot exceed WAL apply latency > 5 ms at p99 for 1k changes once parallelization is added.

#### Required Interfaces
- `LogicalReplicationManager::createSlot(name, plugin, filter, initial_sync, snapshot)` for lifecycle.
- `LogicalReplicationManager::readChanges(slot, max)` to stream logical changes.
- `LogicalReplicationManager::advanceSlot(slot, lsn)` for ack / restart-lsn progression.
- `LogicalReplicationManager::recordDDLChange(stmt, schema_version, lsn)` for schema-aware DDL propagation.
- `LogicalReplicationManager::getStats()` for observability.

#### Implementation Notes
- Filters combine include/exclude lists with optional row predicate (`tenant == 'acme'` style) evaluated against JSON payloads.
- DDL bypasses collection filters but honors `replicate_ddl` flags for each slot.
- Initial sync snapshots enqueue `SNAPSHOT` changes and dedupe conflicting WAL entries (collection + document id) when `lsn <= restart_lsn`.
- Transform hook (`Config::transform`) enables per-change rewrites for target-version compatibility.

#### Test Strategy
- `LogicalReplicationManagerTest`: filters + predicates, DDL propagation, cross-version transform hook, conflict-free initial sync.
- Regression coverage integrated into `LogicalReplicationTests` CTest label `replication;logical;schema;filters;unit`.

#### Performance Targets
- In-memory slot buffer push/pop amortized O(1); shared_mutex protects slot map, per-slot mutex guards buffers.
- Parallel decoding flag ensures consumer threads can drain without blocking producer enqueue on other slots.
- Row filter parsing supports equality/inequality checks without allocations beyond JSON parse.

#### Security / Reliability
- Snapshot dedupe avoids duplicate application during bootstrap, guaranteeing conflict-free initial sync.
- Slot JSON state persisted under WAL directory to maintain restart/confirmed LSNs after restart.
- Transform hook executes in-process; consumers should ensure idempotent transformations to prevent divergent replicas.

---

### Bidirectional Replication ✅ **IMPLEMENTED — v1.7.0**
**Priority:** High
**Target Version:** v1.7.0

> **Status:** Fully implemented in `include/replication/replication_manager.h` +
> `src/replication/replication_manager.cpp` as `BidirectionalReplicationManager`.
> 22 unit tests in `tests/test_replication_ha.cpp` (BidirectionalReplicationTest).
> CI: `.github/workflows/bidirectional-replication-ci.yml`.

Enable true bidirectional replication between two nodes with automatic conflict resolution, useful for active-active deployments.

**Features:**
- Symmetric replication (both nodes are primary)
- Conflict detection using timestamps and sequence numbers
- Configurable conflict resolution per table
- Origin tracking to prevent replication loops
- DDL replication with conflict detection

**Architecture:**
```cpp
class BidirectionalReplicationManager {
public:
    struct BidiConfig {
        std::string local_node_id;
        std::string remote_node_id;
        std::string remote_endpoint;

        // Conflict resolution
        ConflictResolution default_strategy = ConflictResolution::LAST_WRITE_WINS;
        std::map<std::string, ConflictResolution> collection_strategies;

        // Origin tracking
        bool track_origin = true;
        bool replicate_foreign_changes = false;  // Prevent loops

        // Sync
        uint32_t sync_interval_ms = 1000;
        bool bidirectional_sync = true;
    };

    explicit BidirectionalReplicationManager(const BidiConfig& config);

    // Start bidirectional replication
    bool start();
    void stop();

    // Check synchronization status
    struct SyncStatus {
        uint64_t local_sequence;
        uint64_t remote_sequence;
        int64_t lag_ms;
        uint64_t conflicts_last_hour;
        bool is_synchronized;
    };
    SyncStatus getSyncStatus() const;

    // Manual conflict resolution
    void resolveConflict(
        const std::string& document_id,
        const std::string& winner_node
    );

private:
    // Origin tracking
    struct OriginInfo {
        std::string origin_node;
        uint64_t origin_sequence;
        std::chrono::system_clock::time_point origin_timestamp;
    };

    OriginInfo getOrigin(const std::string& document_id);
    bool isLocalOrigin(const OriginInfo& origin);
};

// Example: Active-active setup
BidiConfig config;
config.local_node_id = "us-west-1";
config.remote_node_id = "us-east-1";
config.remote_endpoint = "us-east-1.example.com:7000";
config.default_strategy = ConflictResolution::LAST_WRITE_WINS;
config.collection_strategies["critical_data"] = ConflictResolution::CUSTOM;

BidirectionalReplicationManager bidi(config);
bidi.start();

// Monitor synchronization
auto status = bidi.getSyncStatus();
if (!status.is_synchronized) {
    std::cerr << "WARNING: Nodes not synchronized, lag="
              << status.lag_ms << "ms" << std::endl;
}
```

**Use Cases:**
- Active-active database pairs for high availability
- Cross-datacenter writes with low latency
- Disaster recovery without data loss

---

### Parallel Replication
**Priority:** High
**Target Version:** v1.6.0

Enable parallel application of replication changes on followers to reduce replication lag and improve throughput.

**Features:**
- Multi-threaded WAL application on followers
- Dependency tracking to maintain consistency
- Configurable parallelism (2-64 threads)
- Transaction grouping for batch application
- Conflict-free parallel writes (different keys)

**Architecture:**
```cpp
class ParallelReplicationWorker {
public:
    struct ParallelConfig {
        uint32_t worker_threads = 4;
        uint32_t queue_size = 10000;
        bool use_dependency_tracking = true;
        bool group_transactions = true;
    };

    explicit ParallelReplicationWorker(const ParallelConfig& config);

    // Submit WAL entry for parallel application
    void submit(const WALEntry& entry);

    // Wait for all pending entries to be applied
    void sync();

    // Get statistics
    struct Stats {
        uint64_t entries_applied;
        uint64_t dependencies_detected;
        uint64_t average_latency_us;
        double parallelism_factor;  // Effective parallelism
    };
    Stats getStats() const;

private:
    // Dependency graph
    struct Dependency {
        std::string document_id;
        uint64_t sequence_number;
        std::vector<uint64_t> depends_on;
    };

    // Worker thread pool
    std::vector<std::thread> workers_;
    std::queue<WALEntry> work_queue_;
    std::map<std::string, std::vector<uint64_t>> dependency_graph_;

    void workerLoop(int worker_id);
    bool hasDependencies(const WALEntry& entry);
};

// Example: Enable parallel replication
ParallelConfig pconfig;
pconfig.worker_threads = 8;
pconfig.use_dependency_tracking = true;

ParallelReplicationWorker parallel(pconfig);

// Follower applies entries in parallel
for (const auto& entry : wal_batch) {
    parallel.submit(entry);
}
parallel.sync();

auto stats = parallel.getStats();
std::cout << "Parallelism: " << stats.parallelism_factor << "x" << std::endl;
```

**Performance Targets:**
- 3-10x throughput improvement for independent writes
- Reduce replication lag from seconds to milliseconds
- Support 100K+ writes/sec on followers

**Implementation Notes:**
- Use document_id as dependency key
- Group transactions to apply atomically
- Handle DDL operations serially (no parallelism)

---

### Compressed Replication ✅ Implemented (v1.6.0)
**Priority:** Medium
**Target Version:** v1.6.0
**Status:** Production-ready — `CompressedReplicationStream` in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`

Compress replication streams to reduce bandwidth usage, especially for cross-region replication.

**Features:**
- Multiple compression algorithms (LZ4, Zstd, Snappy)
- Adaptive compression based on data characteristics
- Configurable compression level
- Compression statistics and monitoring

**Architecture:**
```cpp
class CompressedReplicationStream {
public:
    enum CompressionAlgorithm {
        NONE,
        LZ4,        // Fast, moderate compression
        ZSTD,       // Best compression ratio
        SNAPPY,     // Very fast, low compression
        AUTO        // Automatically select based on data
    };

    struct CompressionConfig {
        CompressionAlgorithm algorithm = AUTO;
        int compression_level = 3;  // 1-9
        bool adaptive = true;
        uint32_t min_batch_size = 1024;  // Only compress batches >= 1KB
    };

    CompressedReplicationStream(
        const std::string& endpoint,
        const CompressionConfig& config
    );

    // Send compressed batch
    bool sendBatch(const std::vector<WALEntry>& entries);

    // Get compression statistics
    struct CompressionStats {
        uint64_t bytes_uncompressed;
        uint64_t bytes_compressed;
        double compression_ratio;
        std::string algorithm_used;
    };
    CompressionStats getStats() const;
};

// Example: Cross-region replication with compression
CompressionConfig comp_config;
comp_config.algorithm = CompressionAlgorithm::ZSTD;
comp_config.compression_level = 6;

CompressedReplicationStream stream("eu-west-1:7000", comp_config);
stream.sendBatch(wal_entries);

auto stats = stream.getStats();
std::cout << "Saved " << (stats.bytes_uncompressed - stats.bytes_compressed)
          << " bytes (" << stats.compression_ratio << "x)" << std::endl;
```

**Compression Ratios (typical):**
- JSON documents: 5-10x with Zstd
- Binary data: 1.5-3x
- Already compressed data: ~1x (minimal benefit)

**Network Bandwidth Savings:**
- Cross-region: 80-90% reduction
- Local network: 60-80% reduction

---

### Geo-Replication with Consistency Levels ✅ **IMPLEMENTED — v1.7.0**
**Priority:** Medium
**Target Version:** v1.7.0

**Status:** ✅ Implemented (`include/replication/replication_manager.h`, `src/replication/replication_manager.cpp`)
- `GeoReplicationManager` class with `GeoConfig` (local_region, regions, replication_factor, local_replicas, global_replicas, default_consistency, max_staleness_ms, session_token_ttl_ms)
- All four consistency levels implemented: STRONG (linearizable), BOUNDED_STALENESS (lag-bound), SESSION (read-your-writes), EVENTUAL (always-local)
- Automatic routing via `selectReadRegion()` for each level
- Session tokens with TTL encoding sequence number for read-your-writes guarantee
- `updateRegionStaleness()` for replication layer integration
- Prometheus metrics: per-level read counters, write counter, rejection counter, per-region lag gauges
- 34 unit tests in `tests/test_geo_replication_consistency.cpp` → `GeoReplicationConsistencyFocusedTests`
- CI: `.github/workflows/geo-replication-consistency-ci.yml`


Support multiple consistency levels for geo-distributed deployments, allowing applications to choose between consistency and availability.

**Features:**
- Per-request consistency level
- Consistency levels: STRONG, BOUNDED_STALENESS, SESSION, EVENTUAL
- Automatic routing based on consistency requirements
- Session tokens for read-your-writes guarantee

**Architecture:**
```cpp
class GeoReplicationManager {
public:
    enum ConsistencyLevel {
        STRONG,             // Linearizable (sync replication to all regions)
        BOUNDED_STALENESS,  // Stale reads within time bound (e.g., 5s)
        SESSION,            // Read-your-writes within session
        EVENTUAL            // No guarantee (fastest)
    };

    struct GeoConfig {
        std::vector<std::string> regions;
        uint32_t replication_factor = 3;
        uint32_t local_replicas = 2;
        uint32_t global_replicas = 1;
        ConsistencyLevel default_consistency = SESSION;
    };

    // Write with specific consistency level
    bool write(
        const std::string& key,
        const std::string& value,
        ConsistencyLevel consistency = SESSION
    );

    // Read with specific consistency level
    std::optional<std::string> read(
        const std::string& key,
        ConsistencyLevel consistency = SESSION,
        const std::string& session_token = ""
    );

    // Get session token (for SESSION consistency)
    std::string getSessionToken() const;

    // Check staleness of local replica
    std::chrono::milliseconds getStaleness(const std::string& region) const;
};

// Example: Geo-distributed writes
GeoReplicationManager geo_repl(geo_config);

// Strong consistency (slow but guaranteed)
geo_repl.write("critical_data", value, ConsistencyLevel::STRONG);

// Session consistency (read-your-writes)
auto token = geo_repl.getSessionToken();
geo_repl.write("user_profile", profile, ConsistencyLevel::SESSION);
auto profile_read = geo_repl.read("user_profile", ConsistencyLevel::SESSION, token);

// Bounded staleness (fast reads, slightly stale)
auto data = geo_repl.read("analytics_data", ConsistencyLevel::BOUNDED_STALENESS);

// Eventual consistency (fastest, possibly stale)
auto cached = geo_repl.read("product_catalog", ConsistencyLevel::EVENTUAL);
```

**Consistency Guarantees:**
- STRONG: Linearizable, up-to-date reads
- BOUNDED_STALENESS: Stale by at most N seconds/versions
- SESSION: Read-your-writes within session
- EVENTUAL: No guarantee, best performance

---

### WAL Archival to Object Storage ✅ IMPLEMENTED — v1.6.0
**Priority:** Medium
**Target Version:** v1.6.0

Archive old WAL segments to cloud object storage (S3, GCS, Azure Blob) for long-term retention and cost optimization.

**Features:**
- Automatic archival of old segments
- Configurable retention policy
- On-demand retrieval for PITR
- Encryption at rest in object storage
- Lifecycle management (transition to glacier/cold storage)

**Architecture:**
```cpp
class WALArchivalManager {
public:
    struct ArchivalConfig {
        std::string storage_type;  // "s3", "gcs", "azure"
        std::string bucket_name;
        std::string prefix;

        // Archival policy
        uint32_t archive_after_segments = 100;
        uint32_t local_retention_segments = 10;
        bool compress_before_upload = true;
        bool encrypt_at_rest = true;

        // Lifecycle
        uint32_t transition_to_cold_after_days = 90;
        uint32_t delete_after_days = 365;
    };

    explicit WALArchivalManager(const ArchivalConfig& config);

    // Archive old segments
    void archiveSegments(const std::vector<std::string>& segment_paths);

    // Retrieve archived segment
    std::optional<std::vector<uint8_t>> retrieveSegment(uint64_t segment_id);

    // List archived segments
    struct ArchivedSegment {
        uint64_t segment_id;
        uint64_t start_sequence;
        uint64_t end_sequence;
        uint64_t size_bytes;
        std::chrono::system_clock::time_point archived_at;
        std::string storage_tier;  // "standard", "cold", "glacier"
    };
    std::vector<ArchivedSegment> listArchived() const;
};

// Example: Archive to S3
ArchivalConfig s3_config;
s3_config.storage_type = "s3";
s3_config.bucket_name = "themisdb-wal-archive";
s3_config.prefix = "prod-cluster/";
s3_config.archive_after_segments = 50;
s3_config.compress_before_upload = true;
s3_config.transition_to_cold_after_days = 30;

WALArchivalManager archival(s3_config);

// Background archival process
while (true) {
    auto old_segments = wal_manager.getArchivedSegments();
    archival.archiveSegments(old_segments);
    std::this_thread::sleep_for(std::chrono::hours(1));
}

// Point-in-time recovery from archive
auto segment = archival.retrieveSegment(12345);
if (segment) {
    wal_manager.restoreSegment(12345, *segment);
}
```

**Cost Savings:**
- 90% reduction in local storage costs
- S3 Standard: $0.023/GB/month
- S3 Glacier: $0.004/GB/month
- S3 Deep Archive: $0.00099/GB/month

---

### Multi-Tier Replication
**Priority:** Low
**Target Version:** v1.8.0

Hierarchical replication with different consistency and durability tiers.

**Features:**
- Tier 1: Strong consistency, high durability (3+ replicas)
- Tier 2: Eventual consistency, moderate durability (2 replicas)
- Tier 3: Best-effort, low durability (1 replica, async)
- Per-collection tier assignment
- Automatic tier promotion/demotion based on access patterns

**Architecture:**
```cpp
class MultiTierReplicationManager {
public:
    enum ReplicationTier {
        TIER_1_CRITICAL,    // 3+ replicas, sync, <10ms
        TIER_2_STANDARD,    // 2 replicas, semi-sync, <50ms
        TIER_3_ARCHIVAL     // 1 replica, async, no guarantee
    };

    struct TierConfig {
        ReplicationTier tier;
        uint32_t replica_count;
        ReplicationMode mode;
        uint32_t max_latency_ms;
        uint32_t min_availability_percent;
    };

    // Assign collection to tier
    void assignTier(const std::string& collection, ReplicationTier tier);

    // Automatic tier adjustment based on access patterns
    void enableAutoTiering(bool enabled);

    // Get current tier for collection
    ReplicationTier getTier(const std::string& collection) const;
};

// Example: Multi-tier replication
MultiTierReplicationManager multi_tier;

// Tier assignments
multi_tier.assignTier("financial_transactions", ReplicationTier::TIER_1_CRITICAL);
multi_tier.assignTier("user_profiles", ReplicationTier::TIER_2_STANDARD);
multi_tier.assignTier("audit_logs", ReplicationTier::TIER_3_ARCHIVAL);

// Auto-tiering based on access
multi_tier.enableAutoTiering(true);  // Hot data → Tier 1, Cold data → Tier 3
```

---

### Replication Analytics and Insights
**Priority:** Low
**Target Version:** v1.7.0

Built-in analytics for replication performance, bottleneck detection, and optimization recommendations.

**Features:**
- Replication lag heatmaps
- Bottleneck detection (network, disk, CPU)
- Historical trend analysis
- Automatic optimization recommendations
- Integration with monitoring systems (Prometheus, Grafana)

**Architecture:**
```cpp
class ReplicationAnalytics {
public:
    struct Insight {
        std::string type;  // "LAG_SPIKE", "SLOW_REPLICA", "NETWORK_ISSUE"
        std::string description;
        std::string recommendation;
        std::chrono::system_clock::time_point detected_at;
        std::map<std::string, std::string> metadata;
    };

    // Get current insights
    std::vector<Insight> getInsights() const;

    // Historical lag analysis
    struct LagHistory {
        std::vector<std::pair<std::chrono::system_clock::time_point, int64_t>> data_points;
        int64_t avg_lag_ms;
        int64_t p95_lag_ms;
        int64_t p99_lag_ms;
        int64_t max_lag_ms;
    };
    LagHistory getLagHistory(
        const std::string& replica_id,
        std::chrono::hours duration
    ) const;

    // Bottleneck detection
    struct Bottleneck {
        std::string replica_id;
        std::string bottleneck_type;  // "NETWORK", "DISK_IO", "CPU"
        double severity;  // 0.0 - 1.0
        std::string details;
    };
    std::vector<Bottleneck> detectBottlenecks() const;
};

// Example: Analytics and recommendations
ReplicationAnalytics analytics;

auto insights = analytics.getInsights();
for (const auto& insight : insights) {
    std::cout << insight.type << ": " << insight.description << std::endl;
    std::cout << "Recommendation: " << insight.recommendation << std::endl;
}

// Example insight:
// Type: LAG_SPIKE
// Description: Replica node2 experienced 10s lag spike at 2026-02-10T14:32:00Z
// Recommendation: Check network connectivity to node2, consider increasing batch_size

auto lag_history = analytics.getLagHistory("node2", std::chrono::hours(24));
std::cout << "Average lag: " << lag_history.avg_lag_ms << "ms" << std::endl;
std::cout << "P99 lag: " << lag_history.p99_lag_ms << "ms" << std::endl;
```

---

### Quorum-Based Reads
**Priority:** Medium
**Target Version:** v1.6.0
**Status:** ✅ Implemented (`include/replication/replication_manager.h`, `src/replication/replication_manager.cpp`)

Enable quorum reads for strong consistency guarantees even when reading from replicas.

**Features:**
- Read from multiple replicas and reconcile ✅
- Configurable read quorum (e.g., 2 out of 3) ✅
- Automatic conflict resolution on divergence ✅
- Session consistency with read quorum ✅

**Implemented API:**
```cpp
class QuorumReadManager {
public:
    struct QuorumReadConfig {
        uint32_t read_quorum          = 2;
        uint32_t read_timeout_ms      = 1000;
        bool     repair_on_read       = true;    // Log stale replicas for repair
        uint32_t session_token_ttl_ms = 30000;   // TTL for session tokens (ms)
    };

    struct QuorumReadResult {
        bool        success;
        std::string data;
        uint64_t    version;
        bool        had_conflicts;
        std::vector<std::string> sources;   // Replica endpoints that responded
        std::string session_token;          // Opaque token for session consistency
    };

    QuorumReadResult read(
        const std::string& collection,
        const std::string& document_id,
        uint32_t quorum = 0,                    // 0 = use config default
        const std::string& session_token = ""   // Session token for monotonic reads
    );

    void setReplicas(const std::vector<ReplicaInfo>& replicas);
};

// Example: Strong consistency reads with session token chaining
QuorumReadManager::QuorumReadConfig cfg;
cfg.read_quorum = 2;
cfg.repair_on_read = true;
QuorumReadManager qrm(cfg, replicas);

auto result = qrm.read("users", "user123");
if (result.success) {
    std::cout << "Data: " << result.data << std::endl;
    std::cout << "Version: " << result.version << std::endl;
    if (result.had_conflicts) {
        std::cout << "WARNING: Divergence detected and repair triggered" << std::endl;
    }
    // Use session token for monotonic reads
    auto next = qrm.read("users", "user123", 0, result.session_token);
}
```

**Tests:** 13 tests in `tests/test_replication_ha.cpp` — `QuorumReadManagerTest.*`

---

## Performance Improvements

### Zero-Copy WAL Streaming
**Target Version:** v1.6.0

Use zero-copy I/O (sendfile, splice) to stream WAL entries without copying data through user space.

**Expected Benefits:**
- 30-50% reduction in CPU usage
- 20-40% increase in throughput
- Lower memory bandwidth usage

### Batched Acknowledgments
**Target Version:** v1.6.0

Batch acknowledgments from followers to reduce network round-trips.

**Expected Benefits:**
- 50-70% reduction in ACK packets
- Lower leader CPU usage
- Improved throughput under high load

### Persistent Replication State
**Target Version:** v1.6.0

Persist replication state (last_applied_sequence) to avoid full WAL replay on restart.

**Expected Benefits:**
- Faster follower startup (seconds instead of minutes)
- Reduced leader load during follower recovery

---

## Testing & Validation

### Chaos Engineering Tests
**Target Version:** v1.6.0

Automated chaos tests for replication resilience.

**Test Scenarios:**
- Random node failures
- Network partitions
- Slow replicas (artificial latency injection)
- Byzantine failures (corrupted WAL entries)
- Clock skew and time jumps

### Performance Benchmarks
**Target Version:** v1.6.0

Standardized benchmarks for replication performance.

**Metrics:**
- Replication throughput (writes/sec)
- Replication latency (p50, p95, p99)
- Failover time
- Recovery time
- Resource usage (CPU, memory, network)

---

## Documentation & Tooling

### Replication Dashboard
**Target Version:** v1.7.0

Web-based dashboard for replication monitoring and management.

**Features:**
- Real-time replication topology visualization
- Lag monitoring with graphs
- Automatic failover controls
- Conflict resolution UI
- Health status and alerts

### Replication Doctor Tool
**Target Version:** v1.6.0

CLI tool for diagnosing and fixing replication issues.

**Features:**
```bash
# Check replication health
themisdb-repl-doctor check --cluster prod-cluster

# Detect and fix divergence
themisdb-repl-doctor repair --replica node2

# Analyze replication lag
themisdb-repl-doctor analyze-lag --duration 24h

# Validate WAL integrity
themisdb-repl-doctor validate-wal --segment 12345
```

*Last Updated: April 2026*
*Next Review: v1.6.0 Planning*

---

## Test Strategy

- Unit test coverage ≥ 80% for `ReplicationManager`, `WALShipper`, `ConflictResolver`, `CDCStream`, and `PITRManager`
- Leader election tests: simulate follower crashes and network partitions; assert new leader is elected in < 10 s and no split-brain occurs
- WAL shipping correctness: replay 1 M write operations through WAL on a follower; assert bit-for-bit data equality with the primary
- Conflict resolution tests: generate concurrent writes to the same key on two masters with known HLC timestamps; assert LWW picks the later write and CRDT merge is commutative and idempotent
- PITR restoration test: corrupt 100 random WAL entries, run `PITRManager::restore(t)`, and assert data matches the known state at time `t`
- CDC filtering tests: emit 10,000 INSERT/UPDATE/DELETE events; assert filter by collection pattern and op-type delivers exactly the matching subset
- Replication lag monitoring tests: inject artificial follower delay; assert threshold alert fires within 2 heartbeat intervals
- Kubernetes operator smoke test: deploy a 3-node cluster via operator CRD, kill the leader pod, and assert failover completes within 30 s

## Performance Targets

- Replication lag at p99: ≤ 50 ms under 10,000 write/s in SEMI_SYNC mode on a 3-node LAN cluster
- WAL shipping throughput: ≥ 500 MB/s compressed (Zstd level 3) per follower connection on 10 GbE
- Leader failover time: new leader elected and followers re-pointed in ≤ 10 s under default heartbeat interval (1 s)
- Vector clock / HLC conflict detection overhead: < 5 µs per write operation on the primary
- CRDT merge latency: ≤ 1 µs per merge operation for G-Counter and LWW-Register types
- Point-in-time recovery replay: ≥ 200 MB/s WAL replay throughput; full recovery of a 100 GB dataset in ≤ 10 min
- CDC event emission latency: ≤ 1 ms from commit to CDC queue enqueue at p99
- Cross-datacenter replication lag: ≤ 200 ms at p99 for ASYNC mode over a 50 ms RTT WAN link

## Security / Reliability

- WAL encryption in transit: all WAL segments shipped between nodes must be encrypted with TLS 1.3; plaintext WAL shipping must be rejected unless explicitly disabled in development mode
- CDC stream authentication: CDC consumers must authenticate via mTLS or a signed JWT token; unauthenticated subscriptions must be rejected with HTTP 401
- Split-brain prevention: `min_sync_replicas` must be enforced so that no write is acknowledged without a quorum; a misconfigured `min_sync_replicas = 0` must emit a startup warning and be blocked in SYNC mode
- Raft vote integrity: leader election votes must be signed with the node's private key; unsigned or replayed votes are rejected
- CRDT conflict resolution must never silently discard writes; all conflicts must be logged with their HLC timestamps for audit purposes
- Selective replication filter patterns must be validated at configuration load time; invalid patterns must reject the configuration with a descriptive error
- Cascading replication chains must enforce a maximum depth of 5 hops; exceeding this limit triggers an error to prevent unbounded lag accumulation
