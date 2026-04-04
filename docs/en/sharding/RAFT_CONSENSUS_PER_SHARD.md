# Raft Consensus Per Shard - Usage Guide

## Overview

ThemisDB now supports Raft consensus per shard, enabling leader-based writes, automatic failover, and strong consistency guarantees across distributed shards.

## Key Components

### 1. RaftShardManager
Manages one Raft consensus instance per shard. Each shard forms its own Raft group with configurable replication factor.

**Features:**
- Per-shard leader election
- Automatic failover on leader failure
- Quorum-based write replication
- Partition detection and split-brain prevention
- Comprehensive health monitoring

### 2. Raft-Enhanced ShardTopology
Tracks Raft state for each shard including:
- Current role (LEADER/FOLLOWER/CANDIDATE)
- Term number
- Commit index and log position
- Known leader ID
- Quorum status

### 3. Integrated Write Path
RedundancyStrategy automatically enforces leader-based writes when Raft is enabled:
- Only shard leaders accept writes
- Writes are replicated via Raft log
- Quorum acknowledgment required for commit
- Automatic redirect hints on leader changes

## Configuration

### Enable Raft Consensus

```cpp
// 1. Configure RaftShardManager
RaftShardManager::Config raft_config;
raft_config.replication_factor = 3;  // Number of replicas per shard
raft_config.enable_auto_start = true;
raft_config.raft_config.raft_config.election_timeout_min = std::chrono::milliseconds(150);
raft_config.raft_config.raft_config.election_timeout_max = std::chrono::milliseconds(300);
raft_config.raft_config.raft_config.heartbeat_interval = std::chrono::milliseconds(50);

// Create RaftShardManager
auto raft_manager = std::make_shared<RaftShardManager>(raft_config);

// 2. Initialize Raft for each shard
std::string shard_id = "shard_001";
std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
raft_manager->initializeShard(shard_id, replicas);

// 3. Enable Raft in RedundancyStrategy
RedundancyConfig redundancy_config;
redundancy_config.enable_raft_consensus = true;  // Enable Raft
redundancy_config.mode = RedundancyMode::MIRROR;
redundancy_config.replication_factor = 3;

auto strategy = std::make_shared<RedundancyStrategy>(redundancy_config);
strategy->setRaftShardManager(raft_manager);
```

### Configuration Options

**RaftShardManager::Config:**
- `replication_factor`: Number of replicas per shard (default: 3)
- `enable_auto_start`: Auto-start Raft on initialization (default: true)
- `leader_check_interval`: How often to check leader status (default: 1000ms)

**RaftConsensus::Config (nested in RaftShardManager::Config):**
- `election_timeout_min`: Minimum election timeout (default: 150ms)
- `election_timeout_max`: Maximum election timeout (default: 300ms)
- `heartbeat_interval`: Leader heartbeat interval (default: 50ms)
- `enable_partition_detection`: Enable network partition detection (default: true)
- `enable_split_brain_prevention`: Prevent split-brain scenarios (default: true)
- `read_only_on_partition`: Become read-only in minority partition (default: true)

**RedundancyConfig:**
- `enable_raft_consensus`: Enable Raft for writes (default: false)
- `write_concern`: Write concern level (ONE, MAJORITY, ALL, QUORUM)
- `replication_timeout`: Timeout for Raft commit (default: 5000ms)

## API Usage

### Check Leader Status

```cpp
// Check if local node is leader for a shard
bool is_leader = raft_manager->isShardLeader("shard_001");

// Get current leader ID
std::string leader = raft_manager->getShardLeader("shard_001");

// Get detailed Raft info
auto raft_info = raft_manager->getShardRaftInfo("shard_001");
if (raft_info) {
    std::cout << "Shard: " << raft_info->shard_id << "\n";
    std::cout << "Role: " << raftNodeStateToString(raft_info->role) << "\n";
    std::cout << "Term: " << raft_info->term << "\n";
    std::cout << "Commit Index: " << raft_info->commit_index << "\n";
    std::cout << "Leader: " << raft_info->leader_id << "\n";
    std::cout << "Has Quorum: " << raft_info->has_quorum << "\n";
}
```

### Write Data (Leader-Enforced)

```cpp
// Writes automatically go through Raft when enabled
WriteResult result = strategy->write(
    "doc_123",                  // document ID
    document_data,              // data to write
    "my_collection",            // collection name
    hash_ring,                  // consistent hash ring
    topology,                   // shard topology
    write_handler               // write handler callback
);

if (result.success) {
    std::cout << "Write committed via Raft\n";
    std::cout << "Written to shards: ";
    for (const auto& shard : result.written_shards) {
        std::cout << shard << " ";
    }
} else {
    std::cout << "Write failed: " << result.error_message << "\n";
    // May contain redirect hint if not leader
}
```

### Monitor Health

```cpp
// Check if shard has quorum
bool has_quorum = raft_manager->hasQuorum("shard_001");

// Get all shard Raft info
auto all_info = raft_manager->getAllShardRaftInfo();
for (const auto& [shard_id, info] : all_info) {
    if (!info.is_healthy) {
        std::cout << "Shard " << shard_id << " is unhealthy!\n";
    }
}

// Get Raft leaders from topology
auto leaders = topology.getRaftLeaders();
std::cout << "Current leaders: ";
for (const auto& leader : leaders) {
    std::cout << leader << " ";
}
```

## Prometheus Metrics

When Raft is enabled, the following metrics are exposed at `/metrics`:

### State Metrics
- `themis_raft_role{shard_id}`: Current role (0=FOLLOWER, 1=CANDIDATE, 2=LEADER)
- `themis_raft_term{shard_id}`: Current term number
- `themis_raft_commit_index{shard_id}`: Last committed log index
- `themis_raft_last_applied{shard_id}`: Last applied log index
- `themis_raft_log_size{shard_id}`: Total log entries

### Leadership Metrics
- `themis_raft_leader_elections_total{shard_id}`: Number of elections
- `themis_raft_leader_election_duration_seconds{shard_id}`: Election duration histogram
- `themis_raft_leader_changes_total{shard_id}`: Number of leader changes
- `themis_raft_heartbeats_total{shard_id,result}`: Heartbeat count (success/failure)
- `themis_raft_heartbeat_latency_seconds{shard_id}`: Heartbeat latency

### Replication Metrics
- `themis_raft_log_appends_total{shard_id,result}`: Log append count
- `themis_raft_log_append_latency_seconds{shard_id}`: Append latency
- `themis_raft_replication_lag_entries{shard_id,follower_id}`: Replication lag
- `themis_raft_has_quorum{shard_id}`: Quorum status (0=no, 1=yes)

### Partition Detection Metrics
- `themis_raft_partitions_detected_total{shard_id}`: Partitions detected
- `themis_raft_partitions_healed_total{shard_id}`: Partitions healed
- `themis_raft_read_only_mode{shard_id}`: Read-only status (0=RW, 1=RO)

## Best Practices

### 1. Replication Factor
- **Minimum: 3** - Tolerates 1 failure
- **Recommended: 5** - Tolerates 2 failures
- **Production: Odd numbers** - Ensures clear quorum (3, 5, 7)

### 2. Timeouts
- **Election timeout**: 150-300ms (default) - balance between quick failover and stability
- **Heartbeat interval**: 50ms (default) - must be << election timeout
- **Replication timeout**: 5000ms (default) - adjust based on network latency

### 3. Write Concern
- **MAJORITY**: Recommended for most use cases (data safe with quorum)
- **QUORUM**: Use with custom quorum size
- **ALL**: Maximum durability, but slower and less available

### 4. Monitoring
- Monitor `themis_raft_has_quorum` - alert if any shard loses quorum
- Monitor `themis_raft_leader_elections_total` - frequent elections indicate issues
- Monitor `themis_raft_replication_lag_entries` - high lag indicates slow followers

### 5. Failure Handling
- Raft automatically handles single node failures
- Split-brain prevention protects against network partitions
- Minority partitions become read-only (configurable)
- Leader elections complete in < 1 second typically

## Troubleshooting

### No Known Leader
**Symptom:** `getShardLeader()` returns empty string

**Causes:**
- Cluster is still electing leader (wait for election timeout)
- No quorum available (majority of nodes down)
- Network partition preventing communication

**Solution:**
- Check node health: `hasQuorum()`
- Verify network connectivity between nodes
- Check `themis_raft_has_quorum` metric

### Write Rejected (Not Leader)
**Symptom:** `proposeRaftWrite()` fails with "Not the leader" message

**Causes:**
- Node is not the current leader
- Recent leader change
- Node is in CANDIDATE or FOLLOWER state

**Solution:**
- Redirect writes to current leader (see error log for leader ID)
- Wait for election to complete if in CANDIDATE state
- Use load balancer that tracks leader status

### High Replication Lag
**Symptom:** `themis_raft_replication_lag_entries` metric is high

**Causes:**
- Slow follower node (CPU/disk bottleneck)
- Network latency between leader and follower
- High write rate overwhelming follower

**Solution:**
- Scale up follower resources
- Check network latency/bandwidth
- Reduce write rate or add read replicas

### Frequent Leader Elections
**Symptom:** `themis_raft_leader_elections_total` increasing rapidly

**Causes:**
- Network instability
- Node resource constraints
- Timeout values too aggressive

**Solution:**
- Increase election timeout values
- Check network stability between nodes
- Verify node has sufficient CPU/memory

## Migration from Non-Raft

To migrate existing shards to Raft consensus:

1. **Plan Maintenance Window**: Brief downtime required during migration

2. **Enable Raft Configuration**:
```cpp
redundancy_config.enable_raft_consensus = true;
```

3. **Initialize Raft for Shards**:
```cpp
for (const auto& shard : all_shards) {
    raft_manager->initializeShard(shard.id, shard.replicas);
}
```

4. **Wait for Leader Election**: Monitor until leaders are elected

5. **Enable Write Enforcement**: Start routing writes through Raft

6. **Validate**: Confirm writes are being committed via Raft log

## Performance Impact

**Write Latency:**
- +2-5ms for Raft log append and quorum replication
- Depends on network latency between replicas
- Negligible for most applications

**Read Latency:**
- No impact (reads don't require Raft)
- Can read from followers if eventual consistency is acceptable

**Throughput:**
- Slight reduction in write throughput due to Raft overhead
- Read throughput unaffected
- Overall improvement in consistency guarantees

**Resource Usage:**
- Additional CPU for Raft state machine (~5% overhead)
- Additional memory for Raft log (~100MB per 1M entries)
- Additional network traffic for heartbeats (~1KB/s per shard)

## See Also

- [RAID_SHARD_REFERENCING_ARCHITECTURE.md](../docs/en/sharding/RAID_SHARD_REFERENCING_ARCHITECTURE.md)
- [Raft Paper](https://raft.github.io/raft.pdf)
- [CockroachDB Raft Implementation](https://www.cockroachlabs.com/docs/stable/architecture/replication-layer.html)
