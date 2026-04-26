> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Replication Module

## Module Purpose

The Replication module provides ThemisDB's high-availability and data durability infrastructure through comprehensive replication strategies. It implements both leader-follower (master-slave) replication with Raft-like consensus for strong consistency and multi-master replication for geo-distributed deployments with eventual consistency. The module ensures data redundancy, automatic failover, and horizontal read scalability across distributed database clusters.

### Core Capabilities

- **Leader-Follower Replication**: Primary-replica architecture with automatic failover and strong consistency guarantees
- **Multi-Master Replication**: Write-anywhere architecture with conflict detection and resolution for geo-distributed deployments
- **Raft Consensus**: Leader election and log replication based on Raft protocol for strong consistency
- **WAL Shipping**: Write-Ahead Log based replication for guaranteed durability and point-in-time recovery
- **Change Data Capture (CDC)**: Capture and stream database changes for event-driven architectures and ETL pipelines
- **Logical Replication**: Schema-aware logical slots with include/exclude filters, row predicates, DDL streaming, and cross-version data transformation
- **Conflict Resolution**: Multiple strategies including Last-Write-Wins, CRDT-based merging, and custom resolvers
- **Automatic Failover**: Health monitoring with automatic leader promotion on failure detection
- **Replication Lag Monitoring**: Real-time tracking and alerting for replication lag thresholds
- **Selective Replication**: Filter-based replication for specific collections, tenants, or data patterns

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `raft_node.cpp` | Raft consensus node: leader election and log replication |
| `replication_log.cpp` | WAL-based replication log management |
| `snapshot_manager.cpp` | Snapshot creation and restoration for PITR |
| `leader_election.cpp` | Raft leader election protocol implementation |

## Scope

**In Scope:**
- Asynchronous, semi-synchronous, and synchronous replication modes
- Leader-follower replication with Raft-like consensus
- Multi-master replication with causality tracking (vector clocks, HLC)
- Write-Ahead Log (WAL) management and shipping
- Automatic leader election and failover
- Conflict detection using vector clocks and hybrid logical clocks
- Conflict resolution (LWW, CRDT, custom strategies)
- Replication lag monitoring and health checking
- Read replica routing with configurable read preferences
- Cross-datacenter and cross-region replication
- Point-in-time recovery (PITR) via WAL replay
- Cascading replication for hierarchical topologies
- Prometheus metrics export for monitoring

**Out of Scope:**
- Storage engine implementation (handled by storage module)
- Network protocol and transport layer (handled by network module)
- Authentication and authorization (handled by auth module)
- Query execution and routing (handled by query module)
- Data encryption at rest (handled by storage module)
- Client-side load balancing (handled by client libraries)

## Key Components

### ReplicationManager
**Location:** `replication_manager.cpp`, `../include/replication/replication_manager.h`

Main orchestrator for leader-follower replication with Raft-like consensus.

**Features:**
- **Replication Modes**:
  - `SYNC`: Wait for all replicas to acknowledge (strong consistency)
  - `SEMI_SYNC`: Wait for quorum (configurable min_sync_replicas)
  - `ASYNC`: Don't wait for replicas (eventual consistency, best performance)
- **Leader Election**: Raft-like election with terms and voting
- **WAL Shipping**: Stream write-ahead log entries to followers
- **Automatic Failover**: Detect leader failure and elect new leader
- **Health Monitoring**: Continuous replica health checks with heartbeat
- **Read Preferences**: Route reads to primary, secondary, or nearest replica
- **Cascading Replication**: Replicas can replicate to other replicas

**Thread Safety:**
- All public methods are thread-safe
- Internal locking for concurrent access to replica state
- WAL operations are serialized

**Usage Example:**
```cpp
#include "replication/replication_manager.h"

using namespace themisdb::replication;

// Configure replication
ReplicationConfig config;
config.enabled = true;
config.mode = ReplicationMode::SEMI_SYNC;
config.min_sync_replicas = 2;
config.heartbeat_interval_ms = 1000;
config.enable_auto_failover = true;
config.wal_directory = "/var/lib/themisdb/wal";
config.seed_nodes = {
    "node1.example.com:7000",
    "node2.example.com:7000",
    "node3.example.com:7000"
};

// Initialize replication manager
ReplicationManager repl_mgr(config);
if (!repl_mgr.initialize()) {
    std::cerr << "Failed to initialize replication" << std::endl;
    return;
}

// Replicate a write operation
WALEntry entry;
entry.operation = "INSERT";
entry.collection = "users";
entry.document_id = "user123";
entry.data = R"({"name": "Alice", "email": "alice@example.com"})";

if (!repl_mgr.replicate(entry)) {
    std::cerr << "Replication failed" << std::endl;
}

// Wait for replication to complete (semi-sync mode)
uint64_t seq = entry.sequence_number;
if (!repl_mgr.waitForReplication(seq, 5000)) {
    std::cerr << "Replication timeout" << std::endl;
}

// Check replication status
auto replicas = repl_mgr.getReplicas();
for (const auto& replica : replicas) {
    std::cout << "Replica " << replica.node_id
              << " at sequence " << replica.last_applied_sequence
              << " (lag: " << replica.replicationLagMs() << "ms)"
              << std::endl;
}
```

**Configuration Options:**
```cpp
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
    uint64_t wal_segment_size_bytes = 64 * 1024 * 1024;  // 64MB
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
```

### LogicalReplicationManager
**Location:** `logical_replication.cpp`, `../include/replication/logical_replication.h`

Provides schema-aware logical replication independent of physical WAL shipping. LogicalReplicationManager listens to WAL events via `IReplicationListener`, evaluates per-slot filters (include/exclude collections, row predicates), streams DDL changes, and supports cross-version replication with data transformations and conflict-free initial sync snapshots.

**Usage Example:**
```cpp
auto wal = std::make_shared<WALManager>(config);
LogicalReplicationManager::Config lcfg;
lcfg.wal_directory = config.wal_directory;
lcfg.target_version = "v1.6";
lcfg.transform = [](LogicalChange& change) {
    if (change.new_data.is_object()) {
        change.new_data["tenant"] = "acme";
    }
};
auto logical = std::make_shared<LogicalReplicationManager>(wal, lcfg);
replication_manager.addListener(logical);

LogicalReplicationManager::ReplicationFilter filter;
filter.include_collections = {"orders", "customers"};
filter.row_filter_expression = "tenant == 'acme'";
auto slot = logical->createSlot("acme_replica", "json_output", filter);

// Consume logical changes
auto changes = logical->readChanges("acme_replica", 100);
for (const auto& change : changes) {
    applyToSubscriber(change);
}
logical->advanceSlot("acme_replica", changes.back().lsn);
```

> Note: set `Config::wal_directory` to the WALManager directory to persist slot state; leaving it empty disables persistence. Row filter expressions are validated up front—unsupported predicates are rejected and fail closed to avoid unintended replication.
> Set `Config::parallel_decoding=false` to force sequential slot dispatch; default is `true`, dispatching slots in parallel when 2 or more slots are registered. When `std::thread::hardware_concurrency()` is unavailable, parallel decoding automatically falls back to sequential processing.

**Replication Roles:**
```cpp
enum class ReplicationRole {
    LEADER,         // Primary node accepting writes
    FOLLOWER,       // Read replica receiving updates
    CANDIDATE,      // Participating in leader election
    OBSERVER,       // Non-voting member (async replica)
    WITNESS         // Vote-only member: participates in quorum but stores no data
};
```

**Read Preferences:**
```cpp
enum class ReadPreference {
    PRIMARY,                // Read from primary only
    SECONDARY,              // Read from secondary replicas only
    PRIMARY_PREFERRED,      // Prefer primary, fallback to secondary
    SECONDARY_PREFERRED,    // Prefer secondary, fallback to primary
    NEAREST                 // Read from replica with lowest latency
};
```

### WALManager
**Location:** `replication_manager.cpp`, `../include/replication/replication_manager.h`

Write-Ahead Log manager for durable replication and point-in-time recovery.

**Features:**
- **Segmented WAL**: 64MB segments for efficient rotation and cleanup
- **Checksumming**: SHA-256 integrity verification for each entry
- **Crash Recovery**: Load WAL from disk on startup
- **Compaction**: Automatic cleanup of old segments based on retention policy
- **Point-in-Time Recovery**: Replay WAL entries to any sequence number

**WAL Entry Structure:**
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
```

**Usage Example:**
```cpp
#include "replication/replication_manager.h"

// Create WAL manager
ReplicationConfig config;
config.wal_directory = "/var/lib/themisdb/wal";
config.wal_segment_size_bytes = 64 * 1024 * 1024;
config.wal_retention_segments = 100;
config.wal_sync_on_commit = true;

WALManager wal(config);

// Append entry
WALEntry entry;
entry.operation = "INSERT";
entry.collection = "orders";
entry.document_id = "order456";
entry.data = R"({"total": 99.99, "items": [...])";

uint64_t seq = wal.append(entry);
std::cout << "Written at sequence " << seq << std::endl;

// Read from specific sequence
auto entries = wal.readFrom(seq - 100, 100);
for (const auto& e : entries) {
    std::cout << e.sequence_number << ": " << e.operation
              << " on " << e.collection << std::endl;
}

// Truncate old entries
wal.truncateBefore(seq - 10000);
```

**WAL File Format:**
```
Segment File: wal_0000000001.log
  [4 bytes: entry length]
  [N bytes: serialized WALEntry]
  [4 bytes: entry length]
  [N bytes: serialized WALEntry]
  ...
```

### LeaderElection
**Location:** `replication_manager.cpp`, `../include/replication/replication_manager.h`

Raft-like leader election for high availability and automatic failover.

**Features:**
- **Term-based Election**: Monotonically increasing term numbers prevent split-brain
- **Randomized Timeout**: Election timeout randomized (3-5s default) to avoid collisions
- **Majority Voting**: Requires quorum (N/2 + 1) to elect leader
- **Heartbeat Protocol**: Leader sends periodic heartbeats to maintain leadership
- **Priority-based Election**: Configurable node priorities for deterministic leadership

**Election Algorithm:**
1. Follower detects missing heartbeat (timeout)
2. Follower increments term and becomes CANDIDATE
3. Candidate votes for itself and requests votes from peers
4. Peer votes for candidate if:
   - Candidate's term >= peer's term
   - Peer hasn't voted in this term
   - Candidate's log is at least as up-to-date
5. Candidate receives majority votes → becomes LEADER
6. Leader sends heartbeats to maintain authority

**Usage Example:**
```cpp
#include "replication/replication_manager.h"

// Leader election is managed internally by ReplicationManager
// But you can query election state:

ReplicationManager repl_mgr(config);
repl_mgr.initialize();

// Check current role
ReplicationRole role = repl_mgr.getRole();
if (role == ReplicationRole::LEADER) {
    std::cout << "This node is the leader" << std::endl;
} else {
    std::cout << "Leader is at: " << repl_mgr.getLeaderEndpoint() << std::endl;
}

// Manual failover (for maintenance)
if (repl_mgr.triggerFailover("node2.example.com")) {
    std::cout << "Failover to node2 initiated" << std::endl;
}

// Promote follower to leader (force)
if (repl_mgr.promoteToLeader()) {
    std::cout << "This node promoted to leader" << std::endl;
}

// Demote leader to follower (planned maintenance)
if (repl_mgr.demoteToFollower()) {
    std::cout << "Stepped down as leader" << std::endl;
}
```

### ReplicationStream
**Location:** `replication_manager.cpp`, `../include/replication/replication_manager.h`

Streaming WAL entries from leader to followers.

**Features:**
- **Batch Streaming**: Configurable batch size (default: 100 entries)
- **Flow Control**: Backpressure handling when follower is slow
- **Retry Logic**: Automatic retry on transient network failures
- **Compression**: Optional compression for cross-datacenter replication
- **TLS Encryption**: mTLS for secure replication channels

**Stream Protocol:**
1. Leader tracks each follower's last acknowledged sequence
2. Leader reads WAL entries from (last_acked + 1)
3. Leader sends batch to follower
4. Follower applies entries and acknowledges
5. Leader updates follower's last_acked_sequence
6. Repeat

**Performance Characteristics:**
- **Throughput**: 10K-50K entries/sec per stream
- **Latency**: 1-10ms (local network), 50-500ms (cross-region)
- **Batch Efficiency**: Batching reduces network overhead by 80-90%

### MultiMasterReplicationManager
**Location:** `../include/replication/multi_master_replication.h`

Multi-master replication for geo-distributed deployments with eventual consistency.

**Features:**
- **Write Anywhere**: Any node can accept writes
- **Vector Clocks**: Causal ordering and conflict detection
- **Hybrid Logical Clocks (HLC)**: Combines physical time with logical counters for consistent snapshots
- **Conflict Detection**: Automatic detection of concurrent updates
- **Conflict Resolution**: Pluggable strategies (LWW, CRDT, custom)
- **Anti-Entropy**: Background synchronization to repair divergence
- **Quorum Writes**: Configurable write quorum for durability
- **Causal Consistency**: Maintains causal dependencies across writes

**Usage Example:**
```cpp
#include "replication/multi_master_replication.h"

using namespace themisdb::replication;

// Configure multi-master replication
MMReplicationConfig mm_config;
mm_config.node_id = "node1";
mm_config.datacenter = "us-west";
mm_config.region = "us";
mm_config.seed_peers = {
    "node2.example.com:7000",
    "node3.example.com:7000"
};
mm_config.replication_factor = 3;
mm_config.write_quorum = 2;
mm_config.read_quorum = 1;
mm_config.default_resolution_strategy = "LAST_WRITE_WINS";

// Create multi-master manager
MultiMasterReplicationManager mm_mgr(mm_config);
mm_mgr.start();

// Write to any node
std::string write_id = mm_mgr.write(
    "products",          // collection
    "product789",        // document_id
    "UPDATE",            // operation
    R"({"price": 29.99, "stock": 100})",  // data
    [](const MMWriteEntry& entry, bool success) {
        std::cout << "Write " << (success ? "succeeded" : "failed") << std::endl;
    }
);

// Synchronous write with quorum
bool success = mm_mgr.writeSync(
    "orders",
    "order999",
    "INSERT",
    R"({"customer": "Alice", "total": 199.99})",
    std::chrono::seconds(5)
);

// Read with quorum
auto result = mm_mgr.read("products", "product789", 2);
if (result.success) {
    std::cout << "Data: " << result.data << std::endl;
    std::cout << "Version: " << result.version.toJson() << std::endl;
    std::cout << "Source: " << result.source_node << std::endl;
}

// Register conflict callback
mm_mgr.registerConflictCallback([](const ConflictRecord& conflict) {
    std::cout << "Conflict detected on " << conflict.document_id
              << " between " << conflict.conflicting_writes.size()
              << " writes" << std::endl;
});

// Get statistics
auto stats = mm_mgr.getStats();
std::cout << "Total writes: " << stats.writes_total << std::endl;
std::cout << "Conflicts detected: " << stats.conflicts_detected << std::endl;
std::cout << "Conflicts resolved: " << stats.conflicts_resolved << std::endl;
```

**Vector Clocks:**
```cpp
// Vector clock for causality tracking
VectorClock vc("node1");
vc.increment("node1");  // {node1: 1}
vc.merge(other_vc);     // Merge with another clock

// Compare clocks
if (vc.happensBefore(other_vc)) {
    std::cout << "vc happened before other_vc" << std::endl;
} else if (vc.isConcurrent(other_vc)) {
    std::cout << "vc and other_vc are concurrent (conflict!)" << std::endl;
}
```

**Hybrid Logical Clocks:**
```cpp
// HLC for globally consistent timestamps
HybridLogicalClock hlc("node1");

// Generate timestamp for local event
auto ts1 = hlc.now();
std::cout << "Timestamp: " << ts1.toString() << std::endl;

// Update clock when receiving remote event
auto ts2 = hlc.receive(remote_timestamp);
```

### Conflict Resolution Strategies

#### Last-Write-Wins (LWW)
```cpp
// Uses HLC timestamps to determine winner
LastWriteWinsResolver lww;
auto winner = lww.resolve(document_id, conflicting_writes);
```

#### CRDT-Based Merge
```cpp
// Automatic merge using CRDT semantics
CRDTMergeResolver crdt(CRDTMergeResolver::CRDTType::LWW_MAP);
auto merged = crdt.resolve(document_id, conflicting_writes);

// Supported CRDT types:
// - LWW_REGISTER: Last-Write-Wins Register
// - MV_REGISTER: Multi-Value Register (returns all concurrent values)
// - G_COUNTER: Grow-only Counter (sum all increments)
// - PN_COUNTER: Positive-Negative Counter (sum adds, subtract deletes)
// - G_SET: Grow-only Set (union all adds)
// - OR_SET: Observed-Remove Set (tracks adds/removes with causality)
// - LWW_MAP: Last-Write-Wins Map (per-field LWW)
// - TWO_P_SET: Two-Phase Set (add-only then remove; removed elements cannot be re-added)
// - RGA: Replicated Growable Array (ordered sequence with stable unique element ids)
// - FLAG_EW: Enable-Wins Flag (concurrent enable+disable → enabled)
// - FLAG_DW: Disable-Wins Flag (concurrent enable+disable → disabled)
```

#### Custom Resolver
```cpp
// Application-specific conflict resolution
auto custom = std::make_shared<CustomResolver>(
    [](const std::string& doc_id, const std::vector<MMWriteEntry>& writes) {
        // Custom logic: prioritize writes from specific datacenter
        for (const auto& write : writes) {
            if (write.origin_node.find("us-west") != std::string::npos) {
                return write;
            }
        }
        return writes[0];  // Fallback
    }
);

mm_mgr.setConflictResolver("critical_collection", custom);
```

## Architecture

### Leader-Follower Replication Flow

```
Leader Node
    ↓
[Application Write]
    ↓
[Write to Storage]
    ↓
[Append to WAL] ─────────────┐
    ↓                         │
[Replicate to Followers] ←────┘
    ↓
┌────────────────┬────────────────┐
│                │                │
▼                ▼                ▼
Follower 1     Follower 2     Follower 3
[Receive Entry] [Receive Entry] [Receive Entry]
    ↓                ↓                ↓
[Apply to Storage]  [Apply to Storage]  [Apply to Storage]
    ↓                ↓                ↓
[Send ACK]       [Send ACK]       [Send ACK]
    │                │                │
    └────────────────┴────────────────┘
                     ↓
              [Leader Waits for Quorum]
                     ↓
              [Return Success to App]
```

### Multi-Master Replication Flow

```
Node 1 (US-West)                Node 2 (EU-West)                Node 3 (AP-East)
[Write: doc123]                 [Write: doc123]                 [Write: doc456]
    ↓                               ↓                               ↓
[Increment Vector Clock]        [Increment Vector Clock]        [Increment Vector Clock]
{n1:1, n2:0, n3:0}             {n1:0, n2:1, n3:0}             {n1:0, n2:0, n3:1}
    ↓                               ↓                               ↓
[Replicate to Peers] ──────────────┼───────────────┬─────────────► [Receive]
    ↓                               ↓               │               ↓
[Receive] ◄────────────────────[Replicate to Peers]│            [Replicate to Peers]
    ↓                               ↓               │               ↓
[Detect Conflict] ◄─────────────────────────────────┘           [Apply]
    ↓
[Resolve with Strategy]
    ↓
[Merged Result: doc123]
{n1:2, n2:1, n3:0}
    ↓
[Replicate Merged Result] ───────────────────────────────────────►
```

### Failover Process

```
Normal Operation:
Leader → Follower1, Follower2, Follower3

Leader Failure Detected:
Follower1 (no heartbeat for 5s)
    ↓
[Increment Term: 5 → 6]
    ↓
[Become CANDIDATE]
    ↓
[Vote for Self]
    ↓
[Request Votes] ──────────┬──────────► Follower2
                          │              ↓
                          │         [Check Term: 6 > 5]
                          │              ↓
                          │         [Vote for Follower1]
                          │              ↓
                          └──────────► Follower3
                                         ↓
                                    [Check Term: 6 > 5]
                                         ↓
                                    [Vote for Follower1]
                                         ↓
Follower1                          ◄────┴─────
[Received 3/3 votes (quorum)]
    ↓
[Become LEADER]
    ↓
[Send Heartbeat to All]
    ↓
New Leader
```

### WAL Segment Management

```
/var/lib/themisdb/wal/
├── wal_0000000000.log  (sequences 0-9999)      [ACTIVE]
├── wal_0000000001.log  (sequences 10000-19999) [RETAINED]
├── wal_0000000002.log  (sequences 20000-29999) [RETAINED]
├── ...
└── wal_0000000100.log  (sequences 1000000-1009999) [COMPACTED]

Retention Policy:
- Keep last 100 segments (configurable)
- Retain segments referenced by any follower
- Truncate segments older than min(follower_sequences)
```

## Integration Points

### Storage Module Integration

```cpp
#include "replication/replication_manager.h"
#include "storage/storage_engine.h"

// Storage engine notifies replication on writes
class ReplicatedStorageEngine : public StorageEngine {
public:
    ReplicatedStorageEngine(
        std::shared_ptr<ReplicationManager> repl_mgr
    ) : repl_mgr_(repl_mgr) {}

    bool put(const std::string& collection,
             const std::string& key,
             const std::string& value) override {
        // Write to local storage
        if (!StorageEngine::put(collection, key, value)) {
            return false;
        }

        // Replicate to followers
        WALEntry entry;
        entry.operation = "INSERT";
        entry.collection = collection;
        entry.document_id = key;
        entry.data = value;

        return repl_mgr_->replicate(entry);
    }

private:
    std::shared_ptr<ReplicationManager> repl_mgr_;
};
```

### Transaction Module Integration

```cpp
#include "replication/replication_manager.h"
#include "transaction/transaction_manager.h"

// Transactions coordinate with replication
class ReplicatedTransaction {
public:
    bool commit() {
        // 1. Prepare phase
        auto wal_entries = transaction_.getWALEntries();

        // 2. Replicate to followers
        for (const auto& entry : wal_entries) {
            if (!repl_mgr_->replicate(entry)) {
                // Rollback on replication failure
                transaction_.rollback();
                return false;
            }
        }

        // 3. Wait for quorum (semi-sync mode)
        if (!repl_mgr_->waitForReplication(
                wal_entries.back().sequence_number, 5000)) {
            transaction_.rollback();
            return false;
        }

        // 4. Commit locally
        return transaction_.commit();
    }

private:
    Transaction transaction_;
    std::shared_ptr<ReplicationManager> repl_mgr_;
};
```

### Query Module Integration

```cpp
#include "replication/replication_manager.h"
#include "query/query_engine.h"

// Route reads based on read preference
class ReplicationAwareQueryEngine {
public:
    Result<std::vector<Document>> executeQuery(const Query& query) {
        ReadPreference pref = repl_mgr_->getReadPreference();

        if (pref == ReadPreference::PRIMARY) {
            // Execute on primary only
            return executePrimary(query);
        } else if (pref == ReadPreference::SECONDARY) {
            // Execute on secondary
            auto replica = repl_mgr_->selectHealthyReplica();
            return executeOnReplica(query, replica);
        } else if (pref == ReadPreference::PRIMARY_PREFERRED) {
            // Try primary first
            if (repl_mgr_->getRole() == ReplicationRole::LEADER) {
                return executePrimary(query);
            }
            // Fallback to secondary
            auto replica = repl_mgr_->selectHealthyReplica();
            return executeOnReplica(query, replica);
        }
        // ... other preferences
    }
};
```

### CDC Integration

```cpp
// Change Data Capture stream
class CDCStream : public IReplicationListener {
public:
    void onWALEntryApplied(const WALEntry& entry) override {
        // Stream changes to external systems
        CDCEvent event;
        event.operation = entry.operation;
        event.collection = entry.collection;
        event.document_id = entry.document_id;
        event.data = entry.data;
        event.timestamp = entry.timestamp;

        // Send to Kafka, Kinesis, etc.
        kafka_producer_.send(event);
    }
};

// Register CDC stream
repl_mgr.addListener(std::make_shared<CDCStream>());
```

## API Reference

### Basic Replication Setup

```cpp
#include "replication/replication_manager.h"

// 1. Configure replication
ReplicationConfig config;
config.enabled = true;
config.mode = ReplicationMode::SEMI_SYNC;
config.min_sync_replicas = 2;
config.seed_nodes = {"node1:7000", "node2:7000", "node3:7000"};

// 2. Create replication manager
ReplicationManager repl_mgr(config);

// 3. Initialize
if (!repl_mgr.initialize()) {
    std::cerr << "Initialization failed" << std::endl;
}

// 4. Replicate writes
WALEntry entry;
entry.operation = "INSERT";
entry.collection = "users";
entry.document_id = "user123";
entry.data = R"({"name": "Alice"})";

repl_mgr.replicate(entry);

// 5. Shutdown gracefully
repl_mgr.shutdown();
```

### Multi-Region Setup

```cpp
// Node 1 (US-West)
ReplicationConfig us_config;
us_config.seed_nodes = {
    "us-west-1:7000",
    "us-west-2:7000",
    "eu-west-1:7000",  // Cross-region
    "ap-east-1:7000"   // Cross-region
};

ReplicationManager us_repl(us_config);
us_repl.enableMultiRegion(
    "us-west",
    {"eu-west", "ap-east"}
);

// Cascading replication within region
us_repl.setupCascadingReplication(
    "us-west-1",  // Source
    {"us-west-2", "us-west-3"}  // Targets
);
```

### Monitoring and Health Checks

```cpp
// Check cluster health
auto health = repl_mgr.getClusterHealth();
for (const auto& [node_id, is_healthy] : health) {
    std::cout << node_id << ": "
              << (is_healthy ? "HEALTHY" : "UNHEALTHY")
              << std::endl;
}

// Check replication lag
for (const auto& replica : repl_mgr.getReplicas()) {
    int64_t lag_ms = replica.replicationLagMs();
    if (lag_ms > 5000) {
        std::cerr << "WARNING: Replica " << replica.node_id
                  << " is lagging by " << lag_ms << "ms" << std::endl;
    }
}

// Export Prometheus metrics
std::string metrics = repl_mgr.exportPrometheusMetrics();
// Send to Prometheus push gateway or expose via HTTP endpoint
```

### Point-in-Time Recovery

```cpp
#include "replication/replication_manager.h"

// Recover to specific point in time
class PITRRecovery {
public:
    bool recoverToTimestamp(
        std::chrono::system_clock::time_point target_time
    ) {
        // 1. Find WAL sequence at target time
        uint64_t target_seq = findSequenceAtTime(target_time);

        // 2. Read WAL entries from beginning
        auto entries = wal_->readFrom(0, UINT32_MAX);

        // 3. Replay entries up to target sequence
        for (const auto& entry : entries) {
            if (entry.sequence_number > target_seq) {
                break;
            }
            storage_->apply(entry);
        }

        return true;
    }
};
```

## Dependencies

### Internal Dependencies

- **storage**: Storage engine for applying replicated writes
- **transaction**: Transaction coordination for atomic replication
- **network**: Network transport for replication streams
- **core**: Base types, configuration, metrics

### External Dependencies

**Required:**
- **OpenSSL**: SHA-256 checksumming, TLS encryption (via libssl, libcrypto)
- **C++17**: Standard library (threads, atomics, chrono, filesystem)

**Optional:**
- **Boost.ASIO**: Asynchronous networking (alternative to native sockets)
- **Protobuf**: Efficient serialization for cross-datacenter replication

### Build Configuration

```cmake
# CMakeLists.txt
add_library(themisdb_replication
    src/replication/replication_manager.cpp
)

target_link_libraries(themisdb_replication
    PUBLIC themisdb_storage
    PUBLIC themisdb_core
    PRIVATE OpenSSL::SSL
    PRIVATE OpenSSL::Crypto
)

# Optional dependencies
if(BOOST_ASIO_FOUND)
    target_link_libraries(themisdb_replication PRIVATE Boost::asio)
endif()
```

## Performance Characteristics

### Replication Throughput

- **Async Mode**: 50K-100K writes/sec (no wait for replicas)
- **Semi-Sync Mode (quorum=2)**: 10K-30K writes/sec
- **Sync Mode (all replicas)**: 5K-15K writes/sec
- **Multi-Master (quorum=2)**: 8K-20K writes/sec per node

### Replication Latency

- **Local Network** (same datacenter):
  - Async: <1ms
  - Semi-Sync: 1-5ms
  - Sync: 2-10ms
- **Cross-Region** (e.g., US-West to EU-West):
  - Async: 100-200ms (background)
  - Semi-Sync: 150-300ms
  - Sync: 200-500ms

### Failover Time

- **Leader Election**: 3-8 seconds (randomized timeout)
- **Automatic Failover**: 5-15 seconds (detection + election + promotion)
- **Manual Failover**: 1-3 seconds (immediate promotion)

### WAL Performance

- **Append Rate**: 100K-200K entries/sec (with fsync)
- **Append Rate (no fsync)**: 500K-1M entries/sec
- **WAL Read**: 200K-500K entries/sec
- **Segment Rotation**: <10ms per 64MB segment
- **Compaction**: ~1 second per 1000 segments

### Memory Usage

- **ReplicationManager**: 10-50MB (base overhead)
- **WAL Buffer**: 1-10MB per active stream
- **Multi-Master Vector Clocks**: 1KB per document version
- **Conflict Records**: 5-10KB per unresolved conflict

### Network Bandwidth

- **Leader-Follower**: 1-10 MB/sec per replica (depends on write rate)
- **Multi-Master**: 5-50 MB/sec per peer (with anti-entropy)
- **Cross-Region**: 100-500 KB/sec (compressed)

### Tuning Recommendations

1. **High-Throughput Workloads**:
   - Use ASYNC mode for maximum write throughput
   - Increase batch_size to 500-1000
   - Disable wal_sync_on_commit for even higher performance (risk: loss of last ~100ms writes on crash)

2. **Strong Consistency Requirements**:
   - Use SEMI_SYNC or SYNC mode
   - Set min_sync_replicas to (N/2 + 1) for quorum
   - Enable wal_sync_on_commit

3. **Cross-Region Deployments**:
   - Use ASYNC mode for cross-region replication
   - Enable compression for bandwidth efficiency
   - Increase batch_size and batch_timeout_ms
   - Use cascading replication within regions

4. **Large Clusters (10+ nodes)**:
   - Use observer nodes (non-voting) to reduce election complexity
   - Enable cascading replication to reduce leader load
   - Partition writes by shard/tenant to distribute load

5. **Monitoring**:
   - Alert on replication lag > 5 seconds
   - Alert on consecutive_failures >= 3
   - Monitor WAL size to prevent disk exhaustion
   - Track conflict rate in multi-master setups

## Known Limitations

1. **Leader Election**:
   - Election timeout is randomized (3-5s), can delay failover
   - No deterministic leadership (use priority for preference)
   - Split-brain possible if network partition isn't detected properly

2. **Multi-Master Conflicts**:
   - Conflict resolution adds latency (1-10ms per conflict)
   - CRDT merge can produce unexpected results for application logic
   - No support for cross-document transactions in multi-master mode

3. **Replication Lag**:
   - Cross-region lag can reach 200-500ms
   - Slow replicas can delay synchronous writes
   - No automatic read-your-writes guarantee with SECONDARY read preference

4. **WAL Management**:
   - WAL segments are not compressed (disk usage can be high)
   - No automatic WAL archival to object storage (S3, GCS)
   - WAL replay for PITR can take hours for large databases

5. **Scalability**:
   - Leader becomes bottleneck in leader-follower mode (>50 replicas)
   - Multi-master anti-entropy overhead grows as O(N²)
   - Vector clock size grows with number of nodes

6. **Network Partitions**:
   - Minority partition will refuse writes (no quorum)
   - Multi-master: both partitions accept writes → conflicts on merge
   - No automatic partition healing (manual intervention may be required)

7. **Security**:
   - mTLS is required for secure replication (no plaintext option)
   - No support for replication ACLs (all-or-nothing replication)
   - WAL files are not encrypted at rest

8. **Cascading Replication**:
   - Limited to 2-level hierarchy (leader → relay → replicas)
   - No automatic failover for relay nodes
   - Increases end-to-end replication latency

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- Leader-follower replication with Raft-like consensus
- WAL-based replication and PITR
- Automatic leader election and failover
- Replication lag monitoring
- Read preferences and replica routing
- Prometheus metrics export

⚠️ **Beta Features:**
- Multi-master replication (v1.5.0+)
- CRDT-based conflict resolution (v1.5.0+)
- Cascading replication (v1.5.0+)
- Cross-region replication (v1.5.0+)

🔬 **Experimental:**
- Selective replication (filtered) (v1.5.0+)
- Compressed replication streams (v1.5.0+)

## Related Documentation

- [Storage Module](../storage/README.md) - Storage engine integration
- [Transaction Module](../transaction/README.md) - Transaction coordination
- [Network Module](../network/README.md) - Network transport layer
- [Deployment Guide](../../docs/deployment/replication.md) - Replication setup and best practices
- [Operations Guide](../../docs/operations/failover.md) - Failover procedures and recovery

*Last Updated: April 2026*
*Module Version: v1.5.0*
*Next Review: v1.6.0 Release*

## Scientific References

1. Ongaro, D., & Ousterhout, J. (2014). **In Search of an Understandable Consensus Algorithm**. *Proceedings of the 2014 USENIX Annual Technical Conference (ATC)*, 305–320. https://www.usenix.org/system/files/conference/atc14/atc14-paper-ongaro.pdf

2. Lamport, L. (1998). **The Part-Time Parliament**. *ACM Transactions on Computer Systems*, 16(2), 133–169. https://doi.org/10.1145/279227.279229

3. Gilbert, S., & Lynch, N. (2002). **Brewer's Conjecture and the Feasibility of Consistent, Available, Partition-Tolerant Web Services**. *ACM SIGACT News*, 33(2), 51–59. https://doi.org/10.1145/564585.564601

4. DeCandia, G., Hastorun, D., Jampani, M., Kakulapati, G., Lakshman, A., Pilchin, A., … Vogels, W. (2007). **Dynamo: Amazon's Highly Available Key-Value Store**. *Proceedings of the 21st ACM SIGOPS Symposium on Operating Systems Principles (SOSP)*, 205–220. https://doi.org/10.1145/1294261.1294281

5. Vogels, W. (2009). **Eventually Consistent**. *Communications of the ACM*, 52(1), 40–44. https://doi.org/10.1145/1435417.1435432

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
