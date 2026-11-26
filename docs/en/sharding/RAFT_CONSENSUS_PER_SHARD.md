---
Author: "ThemisDB Contributors"
Created: "2026-02-17"
Last Updated: "2026-11-26"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Raft Consensus Per Shard - Usage Guide

## Overview

> **Scope:** This guide describes per-shard Raft interfaces and intended behavior. It
> does not establish that cross-shard transaction decisions or large-object bytes are
> durably committed end-to-end. See the [proposed block replication and transaction
> target](../../architecture/cross_shard_block_replication_target.md) and the
> [replication roadmap](../../../src/replication/ROADMAP.md) for open contracts and
> integration evidence.

ThemisDB provides per-shard Raft consensus primitives. Production database
apply, crash-consistent snapshot activation and Raft-backed RAID writes are
not yet integrated end-to-end.

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

### 3. Database Write Path

`RedundancyStrategy` does not route writes through Raft. Setting
`RedundancyConfig::enable_raft_consensus=true` is rejected by the C++ strategy
and collection manager until a durable DB state machine is wired. When RAID is
enabled, the server also rejects true or non-Boolean values in `raid.default`
and `raid.collections`. A Raft log commit is not evidence that database state
was applied.

## Configuration

### Configure a Raft Group

```cpp
RaftShardManager::Config raft_config;
raft_config.replication_factor = 3;
raft_config.enable_auto_start = true;
raft_config.raft_config.raft_config.node_id = "node_001";

auto raft_manager = std::make_shared<RaftShardManager>(raft_config);
std::string shard_id = "shard_001";
std::vector<std::string> replicas = {"node_001", "node_002", "node_003"};
raft_manager->initializeShard(shard_id, replicas);
```

For multi-node operation, configure durable journals and a trusted per-peer
mTLS resolver. This example does not configure transport and cannot establish
multi-node quorum or database write support.

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
- `enable_raft_consensus`: Must remain false; true is rejected (default: false)
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
- A durable database applied index is currently unavailable.
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

Migration to Raft-backed database writes is not supported until the durable
apply path, snapshot activation and restart/failover gates are implemented and
validated. Do not treat log replication alone as a migration.

## Performance Impact

No end-to-end performance claim is available for the missing Raft-backed
database write path. Benchmark latency, throughput and resource use after the
complete deployment and failure-recovery path is implemented.

## See Also

- [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md)
- [Raft Paper](https://raft.github.io/raft.pdf)
- [CockroachDB Raft Implementation](https://www.cockroachlabs.com/docs/stable/architecture/replication-layer.html)
