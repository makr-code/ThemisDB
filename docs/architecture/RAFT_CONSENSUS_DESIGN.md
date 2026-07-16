# Raft Consensus & Network Partition Handling Design

## Overview

This document describes the implementation of Raft-based consensus, quorum-enforced operations, and network partition handling in ThemisDB's sharding system. These features provide Byzantine Fault Tolerant (BFT) consistency for distributed LoRA adapter storage.

## Architecture

### Core Components

1. **RaftConsensus** - Enhanced Raft implementation with partition detection
2. **QuorumManager** - Quorum enforcement for read/write operations
3. **PartitionDetector** - Network partition monitoring and detection
4. **ReplicaConsistencyManager** - Vector clock-based conflict resolution

### Design Principles

- **Split-Brain Prevention**: Automatically detect and prevent split-brain scenarios
- **Quorum Enforcement**: Write operations require majority acknowledgment
- **Partition Healing**: Automatic reconciliation when partitions heal
- **Backward Compatibility**: Quorum enforcement OFF by default (RC1)

## Raft Consensus Protocol

### States

```
FOLLOWER → CANDIDATE → LEADER
    ↓         ↑
    └─────────┘
   (election timeout)
```

- **FOLLOWER**: Receives heartbeats, participates in elections
- **CANDIDATE**: Campaigns for leadership, requests votes
- **LEADER**: Accepts writes, broadcasts to followers

### Leader Election

1. Follower times out waiting for heartbeat (150-300ms)
2. Becomes CANDIDATE, increments term
3. Votes for self, requests votes from peers
4. If receives majority votes → becomes LEADER
5. If election timeout → starts new election
6. If discovers higher term → reverts to FOLLOWER

### Log Replication

```cpp
// Leader proposes entry
LogEntry entry{term, index, command};
log.append(entry);

// Replicate to followers
for (node : followers) {
    replicate(node, entry);
}

// Wait for quorum acknowledgments
if (acks >= quorum_size) {
    log.setCommitIndex(index);
    return SUCCESS;
}
```

### Heartbeats

- Leader sends heartbeats every 50ms (configurable)
- Followers reset election timeout on heartbeat
- Heartbeat includes:
  - Leader ID and term
  - Commit index
  - Reachable nodes (for partition detection)

## Quorum Operations

### Write Quorum

```cpp
// Execute write across replicas
QuorumResult result = quorum_manager.executeWrite(
    [&](const std::string& node_id) -> bool {
        return writeToNode(node_id, data);
    },
    target_nodes
);

if (result.success) {
    // Quorum achieved
    // result.acks_received >= result.acks_required
}
```

### Quorum Sizes

- **MAJORITY**: `N/2 + 1` (recommended for strong consistency)
- **ALL**: `N` (strongest consistency, lowest availability)
- **ONE**: `1` (highest availability, eventual consistency)
- **CUSTOM**: User-defined quorum size

### Read Quorum

- Default: `ONE` (read from any replica)
- Can be set to `MAJORITY` for linearizable reads
- Read preference: `PRIMARY`, `NEAREST`, `ROUND_ROBIN`, `RANDOM`

## Network Partition Detection

### Detection Algorithm

1. Monitor heartbeats from all nodes
2. Track consecutive failures per node
3. Calculate reachable node ratio
4. If ratio < 0.7 and > 0.3 → **SPLIT-BRAIN detected**
5. If ratio < 0.3 → **PARTITIONED**

### Health States

```
HEALTHY → DEGRADED → PARTITIONED
   ↓         ↓            ↓
   └─────────┴────────────┘
        (healing)
```

- **HEALTHY**: All nodes reachable, low latency
- **DEGRADED**: Some nodes slow or packet loss > 30%
- **PARTITIONED**: Cannot reach quorum of nodes

### Split-Brain Prevention

When partition detected and node is in **minority partition**:

1. Enter **READ-ONLY mode**
2. Reject all write operations
3. Continue serving reads
4. Wait for partition healing

Majority partition continues operating normally.

### Partition Healing

1. Detect all nodes reachable again
2. Exchange vector clocks
3. Merge divergent logs using causality
4. Resolve conflicts using configured strategy
5. Resume normal operation

## Vector Clocks & Conflict Resolution

### Vector Clock Structure

```cpp
VectorClock {
    map<node_id, timestamp>
    
    node1: 5
    node2: 3
    node3: 7
}
```

### Causality Tracking

```cpp
// Event A happens before Event B
A.version.happensBefore(B.version)

// Events are concurrent (conflict)
A.version.isConcurrent(B.version)
```

### Conflict Resolution Strategies

1. **LAST_WRITE_WINS**: Use wall-clock timestamp
2. **VECTOR_CLOCK_ORDERING**: Use causality (recommended)
3. **HIGHEST_NODE_ID**: Deterministic by node ID
4. **MANUAL**: Callback for application-defined resolution

### Example: Conflict Resolution

```
Partition A writes: {key: "value1", clock: {A:1, B:0, C:0}}
Partition B writes: {key: "value2", clock: {A:0, B:1, C:0}}

After healing:
- Both versions concurrent → CONFLICT
- Strategy: LAST_WRITE_WINS
- Winner: version2 (later timestamp)
```

## Integration with LoRA Storage

### Configuration

```cpp
LoRAStorageService::Config config;
config.enable_quorum_writes = true;  // Enable quorum enforcement
config.enable_partition_detection = true;
config.write_quorum_size = 2;  // Require 2/3 replicas
config.read_quorum_size = 1;   // Read from any replica
```

### Write Flow

```
Client → LoRAStorageService
    ↓
QuorumManager.executeWrite()
    ↓
RaftConsensus.propose()
    ↓
Replicate to N replicas
    ↓
Wait for quorum ACKs
    ↓
Commit to log
    ↓
Return success
```

### Partition Handling

```
Normal operation → Partition detected
    ↓
Minority partition → READ-ONLY
Majority partition → Continue writes
    ↓
Partition heals
    ↓
Merge logs with vector clocks
    ↓
Resolve conflicts
    ↓
Resume normal operation
```

## Performance Characteristics

### Latency

- **Write latency**: ~50ms overhead for quorum
  - Network RTT to replicas
  - Log replication time
  - Quorum acknowledgment wait
  
- **Read latency**: Minimal (read from nearest replica)

### Throughput

- Limited by leader write capacity
- Parallel reads from any replica
- Batch writes for better throughput

### Availability

With replication factor `N` and quorum size `Q`:

- **Fault tolerance**: `N - Q` failures
- Example: `N=5, Q=3` → tolerates 2 failures
- Minority partition becomes read-only

## Testing Strategy

### Unit Tests

- Leader election correctness
- Log replication accuracy
- Quorum calculation
- Partition detection logic
- Vector clock operations
- Conflict resolution strategies

### Integration Tests

- Multi-node cluster simulation
- Network partition injection
- Partition healing scenarios
- Split-brain prevention
- Concurrent writes from partitions

### Chaos Tests

- Random node failures
- Network delays and packet loss
- Byzantine failures
- Time skew scenarios

## Configuration

### Recommended Settings

#### Development
```cpp
config.enable_quorum_writes = false;  // Faster iteration
config.enable_partition_detection = false;
```

#### Production (3-node cluster)
```cpp
config.enable_quorum_writes = true;
config.enable_partition_detection = true;
config.write_quorum_size = 2;  // Majority
config.read_quorum_size = 1;
config.heartbeat_timeout = 500ms;
config.election_timeout_min = 150ms;
config.election_timeout_max = 300ms;
```

#### Production (5-node cluster)
```cpp
config.replication_factor = 5;
config.write_quorum_size = 3;  // Majority
config.read_quorum_size = 2;   // Strong consistency
```

## Monitoring

### Key Metrics

- `raft_current_term` - Current Raft term
- `raft_is_leader` - Boolean, is this node leader
- `quorum_write_latency_ms` - P50/P95/P99 write latency
- `quorum_write_success_rate` - Percentage of successful writes
- `partition_detected_count` - Number of partitions detected
- `partition_healed_count` - Number of healed partitions
- `split_brain_detected` - Boolean, split-brain active
- `replica_health{node=X}` - Health status per replica

### Alerts

- Write quorum failures > 5% → Critical
- Partition detected → Warning
- Split-brain detected → Critical
- Leader elections > 5/min → Warning

## Troubleshooting

### Writes Failing

1. Check quorum configuration
2. Verify replica health (`/metrics`)
3. Check network connectivity
4. Examine Raft logs for conflicts

### Split-Brain Detected

1. Identify network partition cause
2. Check minority partition in read-only mode
3. Wait for partition healing
4. Verify conflict resolution

### High Latency

1. Check network RTT between nodes
2. Reduce quorum size (trade-off: consistency)
3. Increase `operation_timeout`
4. Use `NEAREST` read preference

## Future Enhancements

1. **Pre-vote Protocol**: Reduce disruptive elections
2. **Learner Nodes**: Read-only replicas
3. **Dynamic Membership**: Add/remove nodes online
4. **Snapshot Compaction**: Reduce log size
5. **Parallel Raft**: Multiple Raft groups per node

## References

- [Raft Paper](https://raft.github.io/raft.pdf) - Original Raft consensus paper
- [etcd Raft](https://github.com/etcd-io/etcd/tree/main/raft) - Production Raft implementation
- [Vector Clocks](https://en.wikipedia.org/wiki/Vector_clock) - Causality tracking
- [Quorum Systems](https://www.cs.cornell.edu/courses/cs614/2003sp/papers/GL02.pdf) - Quorum theory

## License

Copyright 2025 ThemisDB
Licensed under MIT License
