# Consensus Module Architecture

## Overview

ThemisDB v1.4+ introduces a pluggable consensus module architecture that enables horizontal scaling through distributed sharding. The consensus layer provides strong consistency guarantees across multiple shards while supporting different consensus algorithms based on deployment requirements.

## Architecture

### Consensus Module Interface

The `ConsensusModule` interface provides a unified API for different consensus algorithms:

```cpp
namespace themisdb::sharding {
    class ConsensusModule {
        virtual ConsensusType getType() const = 0;
        virtual bool initialize(...) = 0;
        virtual bool start() = 0;
        virtual void stop() = 0;
        virtual std::optional<uint64_t> propose(...) = 0;
        virtual bool waitForCommit(...) = 0;
        // ... more methods
    };
}
```

### Supported Consensus Algorithms

#### 1. Raft Consensus
- **Type**: `ConsensusType::RAFT`
- **Characteristics**: Leader-based, strongly consistent
- **Best for**: Primary-backup replication, consistent reads
- **Implementation**: `RaftConsensusAdapter` (adapts existing `RaftConsensus`)
- **Features**:
  - Partition detection
  - Split-brain prevention
  - Automatic leader election
  - Log replication with MVCC

#### 2. Gossip Protocol
- **Type**: `ConsensusType::GOSSIP`
- **Characteristics**: Leaderless, eventually consistent
- **Best for**: Cluster membership, failure detection, configuration propagation
- **Implementation**: `GossipConsensusAdapter` (adapts existing `GossipProtocol`)
- **Features**:
  - SWIM-based peer discovery
  - Anti-entropy
  - Datacenter-aware topology
  - Low overhead

#### 3. Paxos/Multi-Paxos
- **Type**: `ConsensusType::PAXOS` / `ConsensusType::MULTI_PAXOS`
- **Characteristics**: Quorum-based, strongly consistent
- **Best for**: High-availability scenarios, geo-distributed deployments
- **Implementation**: `PaxosConsensus`
- **Features**:
  - Multi-Paxos optimization (stable leader)
  - Fast path optimization
  - Quorum-based agreement
  - Persistent state management

## Configuration

### Consensus Configuration

```cpp
ConsensusConfig config;
config.type = ConsensusType::RAFT;  // or GOSSIP, PAXOS
config.node_id = "node1";
config.cluster_nodes = {"node1", "node2", "node3"};
config.heartbeat_interval = std::chrono::milliseconds(500);
config.election_timeout_min = std::chrono::milliseconds(1000);
config.election_timeout_max = std::chrono::milliseconds(2000);
```

### Algorithm-Specific Settings

#### Raft Settings
```cpp
config.raft_log_max_entries = 10000;
config.raft_enable_pipelining = true;
```

#### Gossip Settings
```cpp
config.gossip_fanout = 3;
config.gossip_interval = std::chrono::milliseconds(1000);
```

#### Paxos Settings
```cpp
config.paxos_quorum_size = 0;  // 0 = auto-calculate (n/2 + 1)
config.paxos_enable_fast_path = true;
config.paxos_prepare_timeout = std::chrono::milliseconds(1000);
config.paxos_accept_timeout = std::chrono::milliseconds(500);
```

## Usage

### Creating a Consensus Module

```cpp
#include "sharding/consensus_factory.h"

// Using factory
ConsensusConfig config;
config.type = ConsensusType::RAFT;
config.node_id = "node1";
config.cluster_nodes = {"node1", "node2", "node3"};

auto consensus = ConsensusFactory::create(config);
```

### Initializing and Starting

```cpp
// Initialize
if (!consensus->initialize(config.node_id, config.cluster_nodes)) {
    // Handle error
}

// Start consensus protocol
if (!consensus->start()) {
    // Handle error
}
```

### Proposing Operations

```cpp
// Propose a write operation
nlohmann::json data = {
    {"table", "users"},
    {"key", "user:123"},
    {"value", {{"name", "Alice"}, {"age", 30}}}
};

auto log_index = consensus->propose("PUT", data);
if (!log_index.has_value()) {
    // Handle error
}

// Wait for commit
bool committed = consensus->waitForCommit(
    *log_index,
    std::chrono::seconds(5)
);
```

### Reading Committed Log

```cpp
// Read committed entries
auto entries = consensus->readLog(1, std::nullopt);  // All entries from index 1
for (const auto& entry : entries) {
    std::cout << "Index: " << entry.index
              << ", Operation: " << entry.operation
              << ", Data: " << entry.data.dump()
              << std::endl;
}
```

### Monitoring and Callbacks

```cpp
// Register commit callback
consensus->onCommit([](const ConsensusLogEntry& entry) {
    std::cout << "Committed: " << entry.operation << std::endl;
});

// Register state change callback
consensus->onStateChange([](ConsensusState old_state, ConsensusState new_state) {
    std::cout << "State changed from " << (int)old_state
              << " to " << (int)new_state << std::endl;
});

// Register leader change callback
consensus->onLeaderChange([](const std::string& old_leader, const std::string& new_leader) {
    std::cout << "Leader changed from " << old_leader
              << " to " << new_leader << std::endl;
});
```

### Getting Statistics

```cpp
auto stats = consensus->getStats();
std::cout << "Current term: " << stats.current_term << std::endl;
std::cout << "Commit index: " << stats.commit_index << std::endl;
std::cout << "State: " << (int)stats.state << std::endl;
std::cout << "Leader: " << stats.current_leader << std::endl;
std::cout << "Cluster size: " << stats.cluster_size << std::endl;
std::cout << "Reachable nodes: " << stats.reachable_nodes << std::endl;
```

### Getting Status JSON

```cpp
auto status = consensus->getStatus();
std::cout << status.dump(2) << std::endl;
```

## Choosing the Right Consensus Algorithm

### Use Raft when:
- ✅ You need strong consistency
- ✅ You have a clear leader-follower model
- ✅ You want simple reasoning about state
- ✅ Your deployment is within a single datacenter
- ⚠️ Downside: Single leader can be a bottleneck

### Use Gossip when:
- ✅ You need high availability over consistency
- ✅ You want decentralized coordination
- ✅ You need efficient failure detection
- ✅ You can tolerate eventual consistency
- ⚠️ Downside: No strong consistency guarantees

### Use Paxos when:
- ✅ You need strong consistency with high availability
- ✅ You deploy across multiple datacenters
- ✅ You want to avoid single points of failure
- ✅ You need flexible quorum configurations
- ⚠️ Downside: More complex implementation

## Integration with Sharding

The consensus module integrates with ThemisDB's sharding infrastructure:

```cpp
// In ShardRouter
class ShardRouter {
private:
    std::shared_ptr<ConsensusModule> consensus_;
    
public:
    void setConsensusModule(std::shared_ptr<ConsensusModule> consensus) {
        consensus_ = consensus;
    }
    
    // Use consensus for shard metadata updates
    bool updateShardMetadata(const std::string& shard_id, const nlohmann::json& metadata) {
        auto log_index = consensus_->propose("UPDATE_SHARD_METADATA", {
            {"shard_id", shard_id},
            {"metadata", metadata}
        });
        
        if (!log_index.has_value()) return false;
        return consensus_->waitForCommit(*log_index, std::chrono::seconds(5));
    }
};
```

## Performance Considerations

### Raft
- **Write latency**: 1-2 RTT (append entries + commit)
- **Read latency**: Local read (leader) or linearizable read (requires heartbeat)
- **Throughput**: Limited by leader capacity
- **Network overhead**: Low (leader to followers)

### Gossip
- **Write latency**: Eventually consistent (variable)
- **Read latency**: Local read (may be stale)
- **Throughput**: High (all nodes can accept writes)
- **Network overhead**: Medium (periodic gossip rounds)

### Paxos
- **Write latency**: 2-3 RTT (prepare + accept)
- **Read latency**: Requires quorum read or lease
- **Throughput**: Medium (distributed across proposers)
- **Network overhead**: Higher (quorum communication)

## Testing

Run consensus module tests:

```bash
./build/tests/test_consensus_module
```

## References

### Raft
- Ongaro, D., & Ousterhout, J. (2014). "In Search of an Understandable Consensus Algorithm"
- https://raft.github.io/

### Gossip Protocol
- van Renesse, R., Birman, K. P., & Vogels, W. (2003). "Astrolabe: A robust and scalable technology"
- Apache Cassandra Gossip: https://cassandra.apache.org/doc/latest/architecture/gossip.html

### Paxos
- Lamport, L. (1998). "The Part-Time Parliament"
- Lamport, L. (2001). "Paxos Made Simple"
- van Renesse, R. & Altinbuken, D. (2015). "Paxos Made Moderately Complex"

## Future Enhancements

- [ ] Dynamic quorum reconfiguration
- [ ] Multi-Raft for parallel consensus instances
- [ ] CRDTs for conflict-free replicated data types
- [ ] Snapshot compression and incremental snapshots
- [ ] Byzantine fault tolerance (BFT) option
- [ ] Witness nodes for tie-breaking in even-sized clusters
