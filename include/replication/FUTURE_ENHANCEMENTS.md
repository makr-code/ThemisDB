# Replication Module Headers - Future Enhancements

## Scope

- API-level enhancements to `include/replication/` headers — public C++ interfaces for replication management
- Replication mode configuration interface: `ReplicationPolicy` with atomic mode change and audit logging
- Conflict resolution hook API: `ConflictResolver` / `AdvancedConflictResolver` stateless hook called per conflict
- Vector clock interface: `VectorClock` with thread-safe `increment()`, `merge()`, and `compare()` methods
- Replication topology API: `ReplicationObserver::getTopology()` returning structured `TopologyNode` graph
- Replication admin API: `ReplicationManager` admin methods gated by role check at the API boundary

## Design Constraints

- [ ] Replication mode changes are atomic and appended to the audit log before taking effect
- [x] Conflict resolver is stateless; `resolve()` receives all context by value and returns a single winner
- [x] Vector clock API is thread-safe; `increment()` and `merge()` use `std::atomic` or `std::mutex` internally
- [x] Replication policy assignment is validated against current topology before acceptance
- [ ] All replication admin API methods require an explicit `AdminCredential` parameter; no ambient authority
- [x] Event stream subscriptions must be unregisterable; RAII subscription handle auto-unsubscribes on destruction

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ReplicationPolicy` | Admin tooling, cluster manager | Atomic mode changes; `validatePolicy()` pre-checks feasibility |
| `ConflictResolver` / `AdvancedConflictResolver` | Multi-master write path | Stateless; receives `ResolutionContext` by value |
| `VectorClock` | WAL entries, replication events | Thread-safe; `compare()` returns `PartialOrder` enum |
| `ReplicationObserver` | Monitoring, topology visualization | `getTopology()` returns `vector<TopologyNode>` |
| `ReplicationEventStream` | CDC, auditing, analytics export | RAII subscription handle; auto-unsubscribes on destruction |

## Planned API Extensions

### Replication Observability API
**Priority:** High
**Target Version:** v1.7.0

Enhanced API for replication monitoring, diagnostics, and troubleshooting.

**New Headers:**
```cpp
// replication/observability.h
namespace themisdb::replication {

class ReplicationObserver {
public:
    // Real-time lag monitoring
    struct LagSnapshot {
        std::string replica_id;
        int64_t lag_ms;
        uint64_t lag_sequences;
        std::chrono::system_clock::time_point captured_at;
        bool is_critical;  // Exceeds threshold
    };

    std::vector<LagSnapshot> getLagSnapshots(
        std::chrono::seconds window = std::chrono::seconds(60)
    ) const;

    // Topology visualization
    struct TopologyNode {
        std::string node_id;
        ReplicationRole role;
        std::vector<std::string> downstream_replicas;
        std::string upstream_source;  // Empty if leader
        bool is_healthy;
    };

    std::vector<TopologyNode> getTopology() const;

    // Replication bottleneck detection
    struct Bottleneck {
        enum Type { NETWORK, DISK_IO, CPU, MEMORY };
        Type type;
        std::string affected_replica;
        double severity;  // 0.0-1.0
        std::string description;
        std::vector<std::string> recommendations;
    };

    std::vector<Bottleneck> detectBottlenecks() const;

    // Replication health score (0-100)
    struct HealthScore {
        int overall_score;
        int lag_score;
        int throughput_score;
        int availability_score;
        std::vector<std::string> issues;
    };

    HealthScore calculateHealthScore() const;
};

} // namespace themisdb::replication
```

**Example:**
```cpp
#include "replication/observability.h"

ReplicationObserver observer(repl_mgr);

// Monitor lag
auto lags = observer.getLagSnapshots(std::chrono::minutes(5));
for (const auto& lag : lags) {
    if (lag.is_critical) {
        alert("Critical lag on " + lag.replica_id);
    }
}

// Visualize topology
auto topology = observer.getTopology();
renderTopologyGraph(topology);

// Detect bottlenecks
auto bottlenecks = observer.detectBottlenecks();
for (const auto& b : bottlenecks) {
    std::cout << "Bottleneck: " << b.description << std::endl;
    for (const auto& rec : b.recommendations) {
        std::cout << "  - " << rec << std::endl;
    }
}

// Health score
auto health = observer.calculateHealthScore();
std::cout << "Health: " << health.overall_score << "/100" << std::endl;
```

---

### Advanced Conflict Resolution API
**Priority:** High
**Target Version:** v1.7.0

Extensible conflict resolution framework with machine learning support.

**New Headers:**
```cpp
// replication/conflict_resolution.h
namespace themisdb::replication {

// Application-defined conflict resolver with context
class AdvancedConflictResolver : public ConflictResolver {
public:
    struct ResolutionContext {
        std::string collection;
        std::string document_id;
        std::map<std::string, std::string> metadata;
        std::vector<std::string> user_roles;
        std::string client_ip;
        std::chrono::system_clock::time_point request_time;
    };

    virtual MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes,
        const ResolutionContext& context
    ) = 0;
};

// ML-based conflict resolver
class MLConflictResolver : public AdvancedConflictResolver {
public:
    struct MLConfig {
        std::string model_path;
        std::vector<std::string> features;
        double confidence_threshold = 0.85;
    };

    explicit MLConflictResolver(const MLConfig& config);

    MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes,
        const ResolutionContext& context
    ) override;

    // Training interface
    void trainOnHistoricalConflicts(
        const std::vector<ConflictRecord>& conflicts,
        const std::vector<std::string>& ground_truth_winners
    );

    double getConfidence() const;
};

// Three-way merge resolver (like git)
class ThreeWayMergeResolver : public AdvancedConflictResolver {
public:
    MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes,
        const ResolutionContext& context
    ) override;

private:
    // Find common ancestor
    std::optional<MMWriteEntry> findCommonAncestor(
        const std::vector<MMWriteEntry>& writes
    );

    // Perform three-way merge
    std::string mergeThreeWay(
        const std::string& base,
        const std::string& left,
        const std::string& right
    );
};

// Field-level merge resolver
class FieldLevelMergeResolver : public AdvancedConflictResolver {
public:
    enum MergeStrategy {
        UNION,       // Take all non-conflicting fields
        INTERSECT,   // Take only common fields
        LEFT_BIAS,   // Prefer left on conflict
        RIGHT_BIAS   // Prefer right on conflict
    };

    explicit FieldLevelMergeResolver(MergeStrategy strategy);

    MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes,
        const ResolutionContext& context
    ) override;
};

} // namespace themisdb::replication
```

**Example:**
```cpp
#include "replication/conflict_resolution.h"

// ML-based resolver
MLConflictResolver::MLConfig ml_config;
ml_config.model_path = "/models/conflict_resolver.pb";
ml_config.features = {"field_count", "value_length", "write_frequency"};

auto ml_resolver = std::make_shared<MLConflictResolver>(ml_config);

// Train on historical data
ml_resolver->trainOnHistoricalConflicts(past_conflicts, ground_truth);

mm_mgr.setConflictResolver("critical_data", ml_resolver);

// Three-way merge (like git)
auto three_way = std::make_shared<ThreeWayMergeResolver>();
mm_mgr.setConflictResolver("documents", three_way);

// Field-level merge
auto field_merge = std::make_shared<FieldLevelMergeResolver>(
    FieldLevelMergeResolver::UNION
);
mm_mgr.setConflictResolver("user_profiles", field_merge);
```

---

### Replication Transaction Coordinator API
**Priority:** Medium
**Target Version:** v1.8.0

Coordinate distributed transactions across replicated nodes.

**New Headers:**
```cpp
// replication/transaction_coordinator.h
namespace themisdb::replication {

class ReplicationTransactionCoordinator {
public:
    struct DistributedTransaction {
        std::string txn_id;
        std::vector<std::string> participating_nodes;
        std::chrono::system_clock::time_point start_time;
        enum State { PREPARING, PREPARED, COMMITTING, COMMITTED, ABORTING, ABORTED };
        State state;
    };

    // Begin distributed transaction
    std::string beginTransaction(
        const std::vector<std::string>& nodes,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)
    );

    // Add operation to transaction
    void addOperation(
        const std::string& txn_id,
        const std::string& node,
        const WALEntry& entry
    );

    // Two-phase commit
    bool prepareTransaction(const std::string& txn_id);
    bool commitTransaction(const std::string& txn_id);
    void abortTransaction(const std::string& txn_id);

    // Query transaction state
    DistributedTransaction getTransaction(const std::string& txn_id) const;
    std::vector<DistributedTransaction> getActiveTransactions() const;
};

} // namespace themisdb::replication
```

**Example:**
```cpp
#include "replication/transaction_coordinator.h"

ReplicationTransactionCoordinator coord(repl_mgr);

// Begin distributed transaction
std::string txn_id = coord.beginTransaction({"node1", "node2", "node3"});

// Add operations
WALEntry entry1;
entry1.collection = "accounts";
entry1.operation = "UPDATE";
entry1.data = R"({"balance": 1000})";
coord.addOperation(txn_id, "node1", entry1);

WALEntry entry2;
entry2.collection = "accounts";
entry2.operation = "UPDATE";
entry2.data = R"({"balance": 500})";
coord.addOperation(txn_id, "node2", entry2);

// Two-phase commit
if (coord.prepareTransaction(txn_id)) {
    if (!coord.commitTransaction(txn_id)) {
        coord.abortTransaction(txn_id);
    }
}
```

---

### Replication Policy API
**Priority:** Medium
**Target Version:** v1.7.0

Flexible replication policies for different data classes.

**New Headers:**
```cpp
// replication/policy.h
namespace themisdb::replication {

class ReplicationPolicy {
public:
    struct Policy {
        std::string name;

        // Replication factor
        uint32_t min_replicas = 2;
        uint32_t desired_replicas = 3;
        uint32_t max_replicas = 5;

        // Geographic distribution
        std::vector<std::string> required_datacenters;
        std::vector<std::string> preferred_datacenters;
        uint32_t min_datacenters = 1;

        // Consistency
        ReplicationMode mode = ReplicationMode::SEMI_SYNC;
        uint32_t write_quorum = 2;
        uint32_t read_quorum = 1;

        // Performance
        uint32_t max_replication_lag_ms = 10000;
        bool enable_compression = false;
        bool enable_encryption = true;

        // Retention
        std::chrono::hours wal_retention = std::chrono::hours(168);  // 7 days
        bool enable_pitr = true;
    };

    // Define policy
    void definePolicy(const std::string& policy_name, const Policy& policy);

    // Assign policy to collection
    void assignPolicy(const std::string& collection, const std::string& policy_name);

    // Query policy
    Policy getPolicy(const std::string& collection) const;

    // Validate policy (check if achievable with current topology)
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> violations;
        std::vector<std::string> recommendations;
    };
    ValidationResult validatePolicy(const Policy& policy) const;
};

} // namespace themisdb::replication
```

**Example:**
```cpp
#include "replication/policy.h"

ReplicationPolicy policy_mgr(repl_mgr);

// Define policies
ReplicationPolicy::Policy critical;
critical.name = "critical";
critical.desired_replicas = 5;
critical.min_datacenters = 3;
critical.mode = ReplicationMode::SYNC;
critical.write_quorum = 3;
critical.enable_pitr = true;

policy_mgr.definePolicy("critical", critical);

ReplicationPolicy::Policy standard;
standard.name = "standard";
standard.desired_replicas = 3;
standard.mode = ReplicationMode::SEMI_SYNC;

policy_mgr.definePolicy("standard", standard);

// Assign policies
policy_mgr.assignPolicy("financial_transactions", "critical");
policy_mgr.assignPolicy("user_profiles", "standard");

// Validate
auto validation = policy_mgr.validatePolicy(critical);
if (!validation.is_valid) {
    for (const auto& violation : validation.violations) {
        std::cerr << "Violation: " << violation << std::endl;
    }
}
```

---

### Replication Event Stream API
**Priority:** Medium
**Target Version:** v1.7.0

Stream replication events for external consumption (CDC, auditing, analytics).

**New Headers:**
```cpp
// replication/event_stream.h
namespace themisdb::replication {

class ReplicationEventStream {
public:
    enum EventType {
        WRITE_REPLICATED,
        ROLE_CHANGED,
        LEADER_ELECTED,
        REPLICA_ADDED,
        REPLICA_REMOVED,
        FAILOVER_STARTED,
        FAILOVER_COMPLETED,
        CONFLICT_DETECTED,
        CONFLICT_RESOLVED,
        LAG_WARNING,
        NETWORK_PARTITION
    };

    struct Event {
        EventType type;
        std::chrono::system_clock::time_point timestamp;
        std::string node_id;
        nlohmann::json data;
    };

    using EventCallback = std::function<void(const Event&)>;

    // Subscribe to events
    uint64_t subscribe(EventType type, EventCallback callback);
    uint64_t subscribeAll(EventCallback callback);
    void unsubscribe(uint64_t subscription_id);

    // Query historical events
    std::vector<Event> getEvents(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end,
        std::optional<EventType> filter = std::nullopt
    ) const;

    // Export to external systems
    void exportToKafka(const std::string& topic);
    void exportToKinesis(const std::string& stream);
    void exportToWebhook(const std::string& url);
};

} // namespace themisdb::replication
```

**Example:**
```cpp
#include "replication/event_stream.h"

ReplicationEventStream event_stream(repl_mgr);

// Subscribe to specific events
event_stream.subscribe(
    ReplicationEventStream::CONFLICT_DETECTED,
    [](const ReplicationEventStream::Event& event) {
        std::cout << "Conflict: " << event.data.dump() << std::endl;
        // Alert ops team
    }
);

// Subscribe to all events
event_stream.subscribeAll([](const ReplicationEventStream::Event& event) {
    log_to_audit_system(event);
});

// Export to Kafka for analytics
event_stream.exportToKafka("themisdb.replication.events");

// Query historical events
auto start = std::chrono::system_clock::now() - std::chrono::hours(24);
auto end = std::chrono::system_clock::now();
auto events = event_stream.getEvents(start, end,
    ReplicationEventStream::FAILOVER_COMPLETED);
```

---

### Replication Testing API
**Priority:** Low
**Target Version:** v1.8.0

API for testing replication resilience and correctness.

**New Headers:**
```cpp
// replication/testing.h
namespace themisdb::replication {

class ReplicationTestHarness {
public:
    // Simulate failures
    void injectNodeFailure(const std::string& node_id,
                          std::chrono::milliseconds duration);
    void injectNetworkPartition(const std::vector<std::string>& partition1,
                               const std::vector<std::string>& partition2,
                               std::chrono::milliseconds duration);
    void injectLatency(const std::string& node_id,
                      std::chrono::milliseconds latency);
    void injectPacketLoss(const std::string& node_id, double loss_rate);

    // Chaos testing
    void startChaosMonkey(const ChaosConfig& config);
    void stopChaosMonkey();

    // Correctness validation
    struct ValidationResult {
        bool is_consistent;
        std::vector<std::string> inconsistencies;
        uint64_t checked_documents;
    };
    ValidationResult validateConsistency() const;

    // Performance testing
    struct LoadTestResult {
        double throughput_writes_per_sec;
        double avg_latency_ms;
        double p95_latency_ms;
        double p99_latency_ms;
        uint64_t total_writes;
        uint64_t failed_writes;
    };
    LoadTestResult runLoadTest(uint32_t duration_sec,
                              uint32_t concurrent_writers);
};

struct ChaosConfig {
    double failure_rate = 0.01;  // 1% of nodes fail per hour
    double partition_rate = 0.005;  // 0.5% chance of partition per hour
    std::chrono::milliseconds max_failure_duration = std::chrono::minutes(5);
    bool enable_latency_injection = true;
    bool enable_packet_loss = false;
};

} // namespace themisdb::replication
```

**Example:**
```cpp
#include "replication/testing.h"

ReplicationTestHarness harness(repl_mgr);

// Inject node failure
harness.injectNodeFailure("node2", std::chrono::seconds(30));

// Validate consistency after recovery
auto validation = harness.validateConsistency();
if (!validation.is_consistent) {
    std::cerr << "Inconsistencies found:" << std::endl;
    for (const auto& issue : validation.inconsistencies) {
        std::cerr << "  - " << issue << std::endl;
    }
}

// Chaos testing
ChaosConfig chaos;
chaos.failure_rate = 0.05;  // 5% per hour
chaos.enable_latency_injection = true;

harness.startChaosMonkey(chaos);
std::this_thread::sleep_for(std::chrono::hours(1));
harness.stopChaosMonkey();

// Load testing
auto load_result = harness.runLoadTest(300, 100);  // 5 min, 100 writers
std::cout << "Throughput: " << load_result.throughput_writes_per_sec
          << " writes/sec" << std::endl;
std::cout << "P99 latency: " << load_result.p99_latency_ms << "ms" << std::endl;
```

---

## API Improvements

### Enhanced Type Safety
**Target Version:** v1.6.0

Replace raw strings with strong types for better compile-time safety.

**Changes:**
```cpp
// Before
std::string node_id = "node1";
repl_mgr.addReplica(node_id);

// After (v1.6.0)
struct NodeId {
    std::string value;
    explicit NodeId(std::string v) : value(std::move(v)) {}
    bool operator==(const NodeId& other) const { return value == other.value; }
};

struct Endpoint {
    std::string host;
    uint16_t port;
    std::string toString() const { return host + ":" + std::to_string(port); }
};

NodeId node_id{"node1"};
Endpoint endpoint{"10.0.0.1", 7000};
repl_mgr.addReplica(node_id, endpoint);
```

### Result Types Instead of Exceptions
**Target Version:** v1.6.0

Use `Result<T>` or `std::expected<T, E>` for error handling.

**Changes:**
```cpp
// Before
try {
    repl_mgr.replicate(entry);
} catch (const ReplicationException& e) {
    std::cerr << e.what() << std::endl;
}

// After (v1.6.0)
auto result = repl_mgr.replicate(entry);
if (!result) {
    std::cerr << result.error().message << std::endl;
}
```

### Async/Await API
**Target Version:** v1.7.0

Coroutine-based API for asynchronous operations.

**Changes:**
```cpp
// Before
repl_mgr.replicate(entry);
repl_mgr.waitForReplication(seq, 5000);

// After (v1.7.0) - C++20 coroutines
co_await repl_mgr.replicateAsync(entry);

// Parallel replication
auto [result1, result2] = co_await std::tuple{
    repl_mgr.replicateAsync(entry1),
    repl_mgr.replicateAsync(entry2)
};
```

### Builder Pattern for Configuration
**Target Version:** v1.6.0

Fluent API for building configurations.

**Changes:**
```cpp
// Before
ReplicationConfig config;
config.mode = ReplicationMode::SEMI_SYNC;
config.min_sync_replicas = 2;
config.seed_nodes = {"node1:7000", "node2:7000"};

// After (v1.6.0)
auto config = ReplicationConfig::builder()
    .mode(ReplicationMode::SEMI_SYNC)
    .minSyncReplicas(2)
    .addSeedNode("node1:7000")
    .addSeedNode("node2:7000")
    .build();
```

---

## Breaking Changes

### v1.6.0
- `ReplicationConfig::seed_nodes` will be deprecated in favor of `addSeedNode()`
- `WALEntry::serialize()` will return `Result<std::vector<uint8_t>>` instead of throwing
- `ReplicationManager::replicate()` will return `Result<uint64_t>` instead of `bool`

### v1.7.0
- `ConflictResolver::resolve()` will require `ResolutionContext` parameter
- `IReplicationListener` will add new method `onNetworkPartitionHealed()`
- `VectorClock::toJson()` format will change to include node metadata

### v2.0.0 (Future)
- Complete rewrite with async/await API
- Remove all exception-based error handling
- Migrate to strong types (NodeId, Endpoint, etc.)

---

## Test Strategy

- Unit tests for `VectorClock`: concurrent `increment()` from 4 threads; assert no data races under TSan
- Unit tests for `ConflictResolver`: provide 3 conflicting writes with `ResolutionContext`; assert deterministic winner selection
- Unit tests for `ReplicationPolicy`: attempt invalid mode change (SYNC on single-node); assert `ValidationResult::is_valid == false`
- Unit tests for `ReplicationObserver::getTopology()`: mock 5-node cluster; assert all nodes present with correct roles
- Integration tests: inject node failure via `ReplicationTestHarness`; assert `validateConsistency()` passes after recovery
- Security tests: call admin API without `AdminCredential`; assert `ERR_UNAUTHORIZED` is returned

## Performance Targets

- Replication lag query (`getLagSnapshots()`) ≤ 1 ms per call for up to 10 replicas
- Conflict resolution hook ≤ 10 ms per conflict including `ResolutionContext` construction
- `VectorClock::merge()` ≤ 500 ns for clocks with up to 64 node entries
- `ReplicationPolicy::validatePolicy()` ≤ 5 ms for topologies with up to 20 nodes
- `ReplicationEventStream` callback delivery latency ≤ 2 ms from event commit to callback invocation
- Topology snapshot (`getTopology()`) ≤ 10 ms for clusters with up to 50 nodes

## Security / Reliability

- Replication channel requires mTLS; connections without a valid client certificate are rejected at the transport layer
- Replication admin API (`definePolicy`, `assignPolicy`, `addReplica`) requires `AdminCredential`; privilege checked at API entry
- WAL entries are checksummed (CRC-32C); corrupt entries are rejected and trigger a replication pause with alert
- `ReplicationEventStream` webhook export validates target URL against an allowlist to prevent SSRF
- Vector clock overflow (counter wrap-around at `uint64_t` max) is detected and raises `ERR_CLOCK_OVERFLOW`

---

## See Also

- [Replication Module Headers](./README.md)
- [Replication Implementation](../../src/replication/README.md)
- [Future Implementation Enhancements](../../src/replication/FUTURE_ENHANCEMENTS.md)

*Last Updated: April 2026*
*Next Review: v1.6.0 Planning*
