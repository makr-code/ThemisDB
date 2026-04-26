> **Build:** `cmake --preset release && cmake --build build/release`

# Replication Module Headers

Public interfaces and declarations for the ThemisDB replication module.

## Table of Contents

1. [Overview](#overview)
2. [Header Files](#header-files)
3. [Data Structures](#data-structures)
4. [API Reference](#api-reference)
5. [Integration Guide](#integration-guide)
6. [Thread Safety](#thread-safety)

## Overview

This directory contains the public API headers for ThemisDB's replication system. These headers define the interfaces for leader-follower replication with Raft-like consensus, multi-master replication with eventual consistency, and all related infrastructure for high availability and data durability.

### Key Components

- **ReplicationManager**: Leader-follower replication with automatic failover
- **MultiMasterReplicationManager**: Multi-master replication with conflict resolution
- **WALManager**: Write-Ahead Log management for durable replication
- **LeaderElection**: Raft-like leader election for consensus
- **VectorClock & HLC**: Causality tracking for multi-master deployments

## Header Files

### replication_manager.h

**Purpose:** Leader-follower replication with Raft-like consensus and automatic failover

**Key Classes:**
- `ReplicationManager`: Main orchestrator for replication
- `WALManager`: Write-Ahead Log management
- `LeaderElection`: Leader election using Raft-like protocol
- `ReplicationStream`: Streaming WAL entries to followers

**Key Enums:**
```cpp
enum class ReplicationRole {
    LEADER,         // Primary node accepting writes
    FOLLOWER,       // Read replica receiving updates
    CANDIDATE,      // Participating in leader election
    OBSERVER,       // Non-voting member (async replica)
    WITNESS         // Vote-only member: participates in quorum but stores no data
};

enum class ReplicationMode {
    SYNC,           // Synchronous: wait for all replicas
    SEMI_SYNC,      // Semi-synchronous: wait for quorum
    ASYNC           // Asynchronous: don't wait
};

enum class ConflictResolution {
    LAST_WRITE_WINS,    // Timestamp-based
    FIRST_WRITE_WINS,   // First value preserved
    VECTOR_CLOCK,       // Causal ordering
    CUSTOM              // User-defined resolver
};

enum class HealthStatus {
    HEALTHY,        // Replica is responding and up-to-date
    DEGRADED,       // Replica is lagging but responsive
    FAILED,         // Replica is not responding
    UNKNOWN         // Health status not yet determined
};

enum class ReadPreference {
    PRIMARY,                // Read from primary only
    SECONDARY,              // Read from secondary replicas only
    PRIMARY_PREFERRED,      // Prefer primary, fallback to secondary
    SECONDARY_PREFERRED,    // Prefer secondary, fallback to primary
    NEAREST                 // Read from replica with lowest latency
};
```

**Key Structures:**
```cpp
struct WALEntry {
    uint64_t sequence_number;       // Monotonic sequence
    uint64_t term;                  // Leader term (Raft-like)
    std::chrono::system_clock::time_point timestamp;
    std::string operation;          // INSERT, UPDATE, DELETE
    std::string collection;
    std::string document_id;
    std::string data;               // JSON payload
    std::string checksum;           // SHA-256 integrity check

    std::vector<uint8_t> serialize() const;
    static std::optional<WALEntry> deserialize(const std::vector<uint8_t>& data);
};

struct ReplicaInfo {
    std::string node_id;
    std::string endpoint;           // hostname:port
    ReplicationRole role;
    uint64_t last_applied_sequence;
    uint64_t last_applied_term;
    std::chrono::system_clock::time_point last_heartbeat;
    bool is_voting_member;
    std::string datacenter;
    int32_t priority;               // For leader election preference
    HealthStatus health_status;
    uint32_t consecutive_failures;

    bool isHealthy() const;
    bool isHealthyWithTimeout(uint32_t timeout_ms) const;
    int64_t replicationLagMs() const;
};

struct ReplicationConfig {
    bool enabled = false;
    ReplicationMode mode = ReplicationMode::ASYNC;
    ConflictResolution conflict_strategy = ConflictResolution::LAST_WRITE_WINS;

    // Timing
    uint32_t heartbeat_interval_ms = 1000;
    uint32_t election_timeout_min_ms = 3000;
    uint32_t election_timeout_max_ms = 5000;
    uint32_t replication_timeout_ms = 10000;

    // Batching
    uint32_t batch_size = 100;
    uint32_t batch_timeout_ms = 50;

    // WAL settings
    std::string wal_directory = "/var/lib/themisdb/wal";
    uint64_t wal_segment_size_bytes = 64 * 1024 * 1024;
    uint32_t wal_retention_segments = 100;
    bool wal_sync_on_commit = true;

    // Quorum settings
    uint32_t min_sync_replicas = 1;
    bool allow_stale_reads = false;
    uint32_t max_replication_lag_ms = 10000;

    // HA settings
    bool enable_auto_failover = true;
    uint32_t failure_detection_timeout_ms = 5000;
    uint32_t min_quorum_for_failover = 2;
    uint32_t max_consecutive_failures = 3;
    uint32_t degraded_lag_threshold_ms = 5000;
    ReadPreference default_read_preference = ReadPreference::PRIMARY_PREFERRED;

    // TLS/Security
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
    bool require_mtls = true;

    // Initial cluster members
    std::vector<std::string> seed_nodes;
};

struct ReplicationStats {
    std::atomic<uint64_t> entries_replicated{0};
    std::atomic<uint64_t> bytes_replicated{0};
    std::atomic<uint64_t> replication_errors{0};
    std::atomic<uint64_t> leader_elections{0};
    std::atomic<uint64_t> conflicts_resolved{0};
    std::atomic<int64_t> max_replication_lag_ms{0};
    std::atomic<int64_t> avg_replication_lag_ms{0};
    std::atomic<uint64_t> automatic_failovers{0};
    std::atomic<uint64_t> manual_failovers{0};
    std::atomic<uint64_t> replica_failures_detected{0};
    std::atomic<uint64_t> network_partitions_detected{0};

    std::string toPrometheusFormat() const;
};
```

**Example:**
```cpp
#include "replication/replication_manager.h"

using namespace themisdb::replication;

// Configure replication
ReplicationConfig config;
config.enabled = true;
config.mode = ReplicationMode::SEMI_SYNC;
config.min_sync_replicas = 2;
config.seed_nodes = {"node1:7000", "node2:7000", "node3:7000"};

// Create manager
ReplicationManager repl_mgr(config);
repl_mgr.initialize();

// Replicate a write
WALEntry entry;
entry.operation = "INSERT";
entry.collection = "users";
entry.document_id = "user123";
entry.data = R"({"name": "Alice"})";

if (repl_mgr.replicate(entry)) {
    // Wait for quorum acknowledgment
    repl_mgr.waitForReplication(entry.sequence_number, 5000);
}

// Check replication status
for (const auto& replica : repl_mgr.getReplicas()) {
    std::cout << replica.node_id << " lag: "
              << replica.replicationLagMs() << "ms" << std::endl;
}
```

**Thread Safety:** Yes (all public methods are thread-safe)

---

### multi_master_replication.h

**Purpose:** Multi-master replication with conflict detection and resolution for geo-distributed deployments

**Key Classes:**
- `MultiMasterReplicationManager`: Main multi-master coordinator
- `VectorClock`: Causality tracking with vector clocks
- `HybridLogicalClock`: Hybrid logical clock for consistent timestamps
- `ConflictResolver`: Base class for conflict resolution strategies
- `LastWriteWinsResolver`: LWW conflict resolution
- `CRDTMergeResolver`: CRDT-based automatic merging
- `CustomResolver`: Application-defined conflict resolution

**Key Enums:**
```cpp
enum class MMNodeState {
    ACTIVE,             // Normal operation
    SYNCING,            // Catching up with peers
    PARTITIONED,        // Network partition detected
    RECOVERING,         // Recovery after failure
    OFFLINE             // Node is offline
};

enum class ConflictType {
    CONCURRENT_UPDATE,      // Same document updated on different nodes
    DELETE_UPDATE,          // One node deleted, another updated
    SCHEMA_CONFLICT,        // Incompatible schema changes
    CONSTRAINT_VIOLATION,   // Unique constraint violated
    CAUSALITY_VIOLATION     // Causal order violated
};
```

**Key Structures:**
```cpp
struct MMWriteEntry {
    std::string write_id;           // Unique write identifier
    std::string origin_node;        // Node that originated the write
    std::string collection;
    std::string document_id;
    std::string operation;          // INSERT, UPDATE, DELETE, MERGE
    std::string data;               // JSON payload
    VectorClock vector_clock;       // Causality tracking
    HybridLogicalClock::Timestamp hlc;  // Hybrid logical timestamp
    std::string checksum;           // Content checksum
    std::vector<std::string> dependencies;  // Causal dependencies

    std::vector<uint8_t> serialize() const;
    static std::optional<MMWriteEntry> deserialize(const std::vector<uint8_t>& data);
};

struct ConflictRecord {
    std::string conflict_id;
    ConflictType type;
    std::string document_id;
    std::string collection;
    std::vector<MMWriteEntry> conflicting_writes;
    std::chrono::system_clock::time_point detected_at;
    bool resolved;
    std::string resolution_strategy;
    std::string winning_write_id;
};

struct MMPeerInfo {
    std::string node_id;
    std::string endpoint;
    std::string datacenter;
    std::string region;
    MMNodeState state;
    VectorClock last_known_clock;
    HybridLogicalClock::Timestamp last_heartbeat_hlc;
    uint64_t replication_lag_ms;
    uint32_t priority;
    bool is_local_datacenter;
};

struct MMReplicationConfig {
    std::string node_id;
    std::vector<std::string> seed_peers;
    std::string datacenter;
    std::string region;

    // Replication settings
    uint32_t replication_factor = 3;
    uint32_t write_quorum = 2;
    uint32_t read_quorum = 1;

    // Timing
    uint32_t heartbeat_interval_ms = 1000;
    uint32_t sync_interval_ms = 100;
    uint32_t timeout_ms = 5000;

    // Conflict resolution
    std::string default_resolution_strategy = "LAST_WRITE_WINS";
    std::map<std::string, std::string> collection_strategies;

    // Performance
    uint32_t max_batch_size = 1000;
    uint32_t max_pending_writes = 10000;
    bool async_apply = true;

    // Network
    bool use_mtls = true;
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
};
```

**Vector Clock API:**
```cpp
class VectorClock {
public:
    VectorClock() = default;
    explicit VectorClock(const std::string& node_id);

    // Increment this node's clock
    void increment(const std::string& node_id);

    // Merge with another vector clock
    void merge(const VectorClock& other);

    // Get timestamp for a node
    uint64_t get(const std::string& node_id) const;

    // Compare clocks
    int compare(const VectorClock& other) const;
    bool happensBefore(const VectorClock& other) const;
    bool isConcurrent(const VectorClock& other) const;

    // Serialization
    std::string toJson() const;
    static VectorClock fromJson(const std::string& json);
};
```

**Hybrid Logical Clock API:**
```cpp
class HybridLogicalClock {
public:
    struct Timestamp {
        uint64_t physical;  // Physical time (milliseconds since epoch)
        uint32_t logical;   // Logical counter
        std::string node_id;

        bool operator<(const Timestamp& other) const;
        bool operator==(const Timestamp& other) const;
        std::string toString() const;
    };

    explicit HybridLogicalClock(const std::string& node_id);

    // Generate timestamp for local event
    Timestamp now();

    // Update clock based on received timestamp
    Timestamp receive(const Timestamp& received);

    // Get current timestamp without incrementing
    Timestamp current() const;
};
```

**Example:**
```cpp
#include "replication/multi_master_replication.h"

using namespace themisdb::replication;

// Configure multi-master
MMReplicationConfig config;
config.node_id = "us-west-1";
config.datacenter = "us-west";
config.region = "us";
config.seed_peers = {"us-east-1:7000", "eu-west-1:7000"};
config.replication_factor = 3;
config.write_quorum = 2;
config.read_quorum = 1;

// Create manager
MultiMasterReplicationManager mm_mgr(config);
mm_mgr.start();

// Write with callback
std::string write_id = mm_mgr.write(
    "products",
    "product789",
    "UPDATE",
    R"({"price": 29.99})",
    [](const MMWriteEntry& entry, bool success) {
        std::cout << "Write " << (success ? "OK" : "FAILED") << std::endl;
    }
);

// Synchronous write with quorum
bool ok = mm_mgr.writeSync(
    "orders",
    "order456",
    "INSERT",
    R"({"total": 99.99})",
    std::chrono::seconds(5)
);

// Read with quorum
auto result = mm_mgr.read("products", "product789", 2);
if (result.success) {
    std::cout << "Data: " << result.data << std::endl;
    std::cout << "Version: " << result.version.toJson() << std::endl;
}

// Handle conflicts
mm_mgr.registerConflictCallback([](const ConflictRecord& conflict) {
    std::cerr << "Conflict on " << conflict.document_id << std::endl;
});

// Custom conflict resolver
auto resolver = std::make_shared<CustomResolver>(
    [](const std::string& doc_id, const std::vector<MMWriteEntry>& writes) {
        // Custom resolution logic
        return writes[0];  // Example: pick first write
    }
);
mm_mgr.setConflictResolver("critical_collection", resolver);
```

**Thread Safety:** Yes (all public methods are thread-safe)

---

## Data Structures

### WAL Entry Format

Binary format for WALEntry serialization:

```
┌─────────────────────────────────────────────────────────┐
│ Sequence Number (8 bytes)                               │
├─────────────────────────────────────────────────────────┤
│ Term (8 bytes)                                          │
├─────────────────────────────────────────────────────────┤
│ Timestamp (8 bytes, milliseconds since epoch)           │
├─────────────────────────────────────────────────────────┤
│ Operation Length (4 bytes) + Operation String           │
├─────────────────────────────────────────────────────────┤
│ Collection Length (4 bytes) + Collection String         │
├─────────────────────────────────────────────────────────┤
│ Document ID Length (4 bytes) + Document ID String       │
├─────────────────────────────────────────────────────────┤
│ Data Length (4 bytes) + Data String                     │
├─────────────────────────────────────────────────────────┤
│ Checksum Length (4 bytes) + Checksum String (64 chars)  │
└─────────────────────────────────────────────────────────┘
```

### Vector Clock Format

JSON representation:
```json
{
  "node1": 42,
  "node2": 37,
  "node3": 51
}
```

Comparison rules:
- `A < B` if all A[i] ≤ B[i] and exists i: A[i] < B[i]
- `A > B` if all A[i] ≥ B[i] and exists i: A[i] > B[i]
- `A || B` (concurrent) if neither A < B nor A > B

### Hybrid Logical Clock Format

String representation: `physical.logical@node_id`

Example: `1704067200000.42@us-west-1`

Comparison rules:
1. Compare physical timestamps
2. If equal, compare logical counters
3. If equal, compare node IDs (lexicographic)

## API Reference

### ReplicationManager

#### Initialization
```cpp
explicit ReplicationManager(const ReplicationConfig& config);
~ReplicationManager();

bool initialize();
void shutdown();
```

#### Write Operations
```cpp
bool replicate(const WALEntry& entry);
bool waitForReplication(uint64_t sequence, uint32_t timeout_ms = 0);
```

#### Query Operations
```cpp
ReplicationRole getRole() const;
std::string getLeaderEndpoint() const;
std::vector<ReplicaInfo> getReplicas() const;
const ReplicationStats& getStats() const;
```

#### Replica Management
```cpp
void addReplica(const ReplicaInfo& replica);
void removeReplica(const std::string& node_id);
int64_t getReplicationLag(const std::string& replica_id) const;
std::vector<std::pair<std::string, HealthStatus>> getReplicaHealthStatus() const;
```

#### Failover & Promotion
```cpp
bool triggerFailover(const std::string& target_node_id);
bool promoteToLeader();
bool demoteToFollower();
bool promoteReplica(const std::string& replica_id);
```

#### Multi-Region & Cascading
```cpp
bool enableMultiRegion(const std::string& region_id,
                      const std::vector<std::string>& peer_regions);
bool setupCascadingReplication(const std::string& source_replica,
                               const std::vector<std::string>& target_replicas);
```

#### Health & Monitoring
```cpp
std::map<std::string, bool> getClusterHealth() const;
bool hasQuorum() const;
void performHealthCheck();
bool detectNetworkPartition() const;
std::string exportPrometheusMetrics() const;
```

#### Read Preferences
```cpp
ReadPreference getReadPreference() const;
void setReadPreference(ReadPreference preference);
```

#### Event Listeners
```cpp
void setConflictResolver(std::shared_ptr<IConflictResolver> resolver);
void addListener(std::shared_ptr<IReplicationListener> listener);
```

### MultiMasterReplicationManager

#### Lifecycle
```cpp
explicit MultiMasterReplicationManager(const MMReplicationConfig& config);
~MultiMasterReplicationManager();

bool start();
void stop();
bool isRunning() const;
```

#### Write Operations
```cpp
using WriteCallback = std::function<void(const MMWriteEntry&, bool success)>;

std::string write(
    const std::string& collection,
    const std::string& document_id,
    const std::string& operation,
    const std::string& data,
    WriteCallback callback = nullptr
);

bool writeSync(
    const std::string& collection,
    const std::string& document_id,
    const std::string& operation,
    const std::string& data,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)
);
```

#### Read Operations
```cpp
struct ReadResult {
    bool success;
    std::string data;
    VectorClock version;
    std::string source_node;
};

ReadResult read(
    const std::string& collection,
    const std::string& document_id,
    uint32_t read_quorum = 0
);
```

#### Peer Management
```cpp
void addPeer(const MMPeerInfo& peer);
void removePeer(const std::string& node_id);
std::vector<MMPeerInfo> getPeers() const;
MMPeerInfo getLocalInfo() const;
```

#### Conflict Management
```cpp
using ConflictCallback = std::function<void(const ConflictRecord&)>;

void registerConflictCallback(ConflictCallback callback);
void setConflictResolver(
    const std::string& collection,
    std::shared_ptr<ConflictResolver> resolver
);
std::vector<ConflictRecord> getUnresolvedConflicts() const;
bool resolveConflict(const std::string& conflict_id,
                    const std::string& winning_write_id);
```

#### Synchronization
```cpp
void triggerSync();
uint64_t getReplicationLag() const;
```

#### Statistics
```cpp
struct Stats {
    uint64_t writes_total;
    uint64_t writes_replicated;
    uint64_t writes_pending;
    uint64_t conflicts_detected;
    uint64_t conflicts_resolved;
    uint64_t sync_rounds;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    std::chrono::milliseconds avg_replication_latency;
};

Stats getStats() const;
std::string exportPrometheusMetrics() const;
```

## Integration Guide

### Basic Leader-Follower Setup

```cpp
#include "replication/replication_manager.h"

// 1. Configure
ReplicationConfig config;
config.enabled = true;
config.mode = ReplicationMode::SEMI_SYNC;
config.seed_nodes = {"node1:7000", "node2:7000", "node3:7000"};

// 2. Initialize
ReplicationManager repl(config);
repl.initialize();

// 3. Replicate writes
WALEntry entry;
entry.operation = "INSERT";
entry.collection = "users";
entry.document_id = "user123";
entry.data = R"({"name": "Alice"})";

repl.replicate(entry);
```

### Multi-Master Setup

```cpp
#include "replication/multi_master_replication.h"

// 1. Configure
MMReplicationConfig config;
config.node_id = "node1";
config.seed_peers = {"node2:7000", "node3:7000"};
config.write_quorum = 2;

// 2. Start
MultiMasterReplicationManager mm(config);
mm.start();

// 3. Write with quorum
mm.writeSync("users", "user123", "INSERT", R"({"name": "Alice"})");
```

### Event Listener Integration

```cpp
class MyReplicationListener : public IReplicationListener {
public:
    void onRoleChange(ReplicationRole old_role, ReplicationRole new_role) override {
        std::cout << "Role changed from " << (int)old_role
                  << " to " << (int)new_role << std::endl;
    }

    void onLeaderElected(const std::string& leader_id) override {
        std::cout << "New leader: " << leader_id << std::endl;
    }

    void onReplicationLagWarning(int64_t lag_ms) override {
        std::cerr << "WARNING: Replication lag " << lag_ms << "ms" << std::endl;
    }

    void onFailoverStarted(const std::string& failed_leader_id,
                          const std::string& new_leader_id) override {
        std::cout << "Failover: " << failed_leader_id
                  << " -> " << new_leader_id << std::endl;
    }

    // ... implement other methods
};

// Register listener
auto listener = std::make_shared<MyReplicationListener>();
repl_mgr.addListener(listener);
```

## Thread Safety

### Thread-Safe Components

**ReplicationManager:**
- ✅ All public methods are thread-safe
- ✅ Internal locking for concurrent access
- ✅ Can be called from multiple threads

**MultiMasterReplicationManager:**
- ✅ All public methods are thread-safe
- ✅ Concurrent writes from multiple threads supported
- ✅ Internal synchronization for peer state

**WALManager:**
- ✅ Thread-safe append operations
- ✅ Concurrent reads supported
- ⚠️  Serialized writes (internal mutex)

**VectorClock:**
- ✅ Thread-safe (internal shared_mutex)
- ✅ Concurrent reads, exclusive writes

**HybridLogicalClock:**
- ✅ Thread-safe (atomic operations + mutex)
- ✅ Lock-free for read operations

### Best Practices

1. **Don't share WALEntry objects** across threads during modification
2. **ReplicationManager can be shared** across threads safely
3. **Use separate VectorClock instances** per thread if high contention
4. **Callbacks (WriteCallback, ConflictCallback)** must be thread-safe
5. **IReplicationListener methods** may be called from any thread

### Performance Considerations

- **Lock Contention**: High write rates may cause contention on WAL append lock
- **Callback Overhead**: Keep callbacks fast to avoid blocking replication
- **Vector Clock Merging**: Can be expensive with many nodes (100+)
- **HLC Updates**: Lock-free reads, but writes require mutex

## Dependencies

### Required
- OpenSSL (libssl, libcrypto) for checksumming and TLS
- C++17 standard library (threads, atomics, chrono, filesystem)

### Optional
- Boost.ASIO for asynchronous networking
- Protobuf for efficient serialization

## See Also

- [Replication Module Implementation](../../src/replication/README.md)
- [Future Enhancements](./FUTURE_ENHANCEMENTS.md)
- [Storage Module Headers](../storage/README.md)
- [Transaction Module Headers](../transaction/README.md)

*Last Updated: April 2026*
*API Version: v1.5.0*
*Next Review: v1.6.0 Release*

## Additional Header Files

The following headers are present in `include/replication/` and supplement the components documented above.

### conflict_resolution.h
Defines `IConflictResolver` base interface and built-in strategies (`LastWriteWinsResolver`, `FirstWriteWinsResolver`) used by both `ReplicationManager` and `MultiMasterReplicationManager`. <!-- TODO: verify -->

### crdt_types.h
CRDT (Conflict-free Replicated Data Type) primitives: G-Counter, PN-Counter, OR-Set, LWW-Register. Used by `CRDTMergeResolver`. <!-- TODO: verify -->

### event_stream.h
Change-event streaming interface for emitting replication events to external consumers (Kafka, WebSocket, CDC). <!-- TODO: verify -->

### kafka_change_stream.h
Kafka-backed change-data-capture (CDC) stream; publishes WAL entries as Kafka messages for downstream consumers. <!-- TODO: verify -->

### logical_replication.h
Logical (row-level) replication layer that decodes WAL entries into structured change events independent of physical storage format. <!-- TODO: verify -->

### multi_tier_replication.h
Multi-tier (hierarchical) replication topology: data flows from primary → regional secondaries → edge replicas. <!-- TODO: verify -->

### observability.h
Replication-specific observability hooks: OpenTelemetry spans, Prometheus counters, and health-check endpoints. <!-- TODO: verify -->

### policy.h
Declarative replication policies (retention windows, quorum overrides, geo-routing rules) applied per collection. <!-- TODO: verify -->

### raft_v2.h
Raft v2 consensus implementation: leader election, log replication, snapshot installation, and membership changes. <!-- TODO: verify -->

### replication_slot.h
Persistent replication slots that track consumer progress through the WAL (similar to PostgreSQL replication slots). <!-- TODO: verify -->

### schema_cdc.h
Schema change-data-capture: captures DDL events (collection create/drop, index add/remove) as replication entries. <!-- TODO: verify -->

## Installation
