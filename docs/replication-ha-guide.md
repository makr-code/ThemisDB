# High Availability Replication - Implementation Guide

## Overview

ThemisDB's High Availability (HA) Replication provides enterprise-grade reliability with automatic failure detection, failover, and recovery mechanisms. This ensures continuous database availability even in the face of node failures, network partitions, or datacenter outages.

## Table of Contents

1. [Features](#features)
2. [Architecture](#architecture)
3. [Configuration](#configuration)
4. [Deployment Topologies](#deployment-topologies)
5. [Failure Detection](#failure-detection)
6. [Automatic Failover](#automatic-failover)
7. [Monitoring](#monitoring)
8. [Operational Procedures](#operational-procedures)
9. [Performance Tuning](#performance-tuning)
10. [Troubleshooting](#troubleshooting)

## Features

### Core HA Capabilities

- ✅ **Automatic Failure Detection** - Heartbeat-based health monitoring with configurable timeouts
- ✅ **Automatic Failover** - Quorum-based leader election on primary failure
- ✅ **Network Partition Handling** - Detection and graceful degradation during network splits
- ✅ **Health Monitoring** - Continuous tracking with HEALTHY/DEGRADED/FAILED states
- ✅ **Read Preferences** - Flexible query routing (primary/secondary/nearest)
- ✅ **Metrics & Alerting** - Comprehensive Prometheus metrics for monitoring
- ✅ **Zero Data Loss** - RPO=0 with synchronous/semi-synchronous replication
- ✅ **Fast Recovery** - RTO<30s with automatic failover

### Replication Modes

1. **Synchronous (SYNC)** - Wait for all replicas before acknowledging writes
   - Guarantees: Zero data loss (RPO=0)
   - Use Case: Mission-critical transactional systems
   - Latency: Highest (waits for slowest replica)

2. **Semi-Synchronous (SEMI_SYNC)** - Wait for quorum of replicas
   - Guarantees: Zero data loss with quorum
   - Use Case: Production systems balancing durability and performance
   - Latency: Moderate (waits for N replicas)

3. **Asynchronous (ASYNC)** - Don't wait for replicas
   - Guarantees: Eventual consistency
   - Use Case: High-throughput, read-heavy workloads
   - Latency: Lowest (immediate acknowledgment)

## Architecture

### Module Organization

ThemisDB's replication system is split across two main module directories:

**`replication/` Module** - High-level replication orchestration:
- **Location**: `include/replication/`, `src/replication/`
- **Components**:
  - `ReplicationManager` - Orchestrates replication lifecycle and configuration
  - `MultiMasterReplicationManager` - Multi-master replication coordination
- **Responsibility**: High-level replication strategies, multi-master coordination, and configuration management

**`sharding/` Module** - Low-level replication infrastructure and distributed coordination:
- **Location**: `include/sharding/`, `src/sharding/`
- **Components**:
  - `WALManager` - Write-Ahead Log persistence with LSN tracking
  - `WALShipper` - Batch-based WAL shipping to replicas
  - `WALApplier` - Idempotent WAL application on replicas
  - `ReplicationCoordinator` - Write concern enforcement (ONE/MAJORITY/ALL)
  - `ReplicaTopology` - Shard-to-replica mapping with RAID support
  - **Consensus modules**: Raft, Gossip, Paxos for leader election
  - `HealthMonitor` - Replica health tracking
  - Various distributed system components (topology, metrics, etc.)
- **Responsibility**: WAL-based replication mechanics, distributed consensus, topology management, and low-level coordination

**Design Rationale**: The split allows `replication/` to focus on business logic and orchestration while `sharding/` handles the complex distributed systems infrastructure needed for both replication and horizontal scaling.

> **See Also**: 
> - [REPLICATION_IMPLEMENTATION_STATUS.md](./REPLICATION_IMPLEMENTATION_STATUS.md) - Detailed status of WAL components
> - [replication_raid_plan.md](./replication_raid_plan.md) - RAID 1/10 implementation roadmap

### High-Level Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Replication Manager                       │
│                   (replication/ module)                      │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │    WAL       │  │   Leader     │  │   Health     │     │
│  │  Manager     │  │  Election    │  │  Monitor     │     │
│  │ (sharding/)  │  │ (sharding/)  │  │ (sharding/)  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Replication  │  │  Conflict    │  │  Metrics &   │     │
│  │   Streams    │  │  Resolution  │  │  Events      │     │
│  │ (sharding/)  │  │              │  │ (sharding/)  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### Health States

```
UNKNOWN ──┐
          │
          ├──> HEALTHY ──> DEGRADED ──> FAILED
          │        ▲           │            │
          └────────┴───────────┴────────────┘
                 (Recovery paths)
```

**HEALTHY**: Replica responding within heartbeat timeout, lag < threshold  
**DEGRADED**: Replica responding but lagging beyond threshold  
**FAILED**: Replica not responding for timeout duration  
**UNKNOWN**: Initial state, health not yet determined

## Configuration

### Minimal HA Configuration

```yaml
replication:
  enabled: true
  mode: "semi_sync"
  
  # HA Settings
  enable_auto_failover: true
  failure_detection_timeout_ms: 5000
  min_quorum_for_failover: 2
  
  seed_nodes:
    - "node-01:8765"
    - "node-02:8765"
    - "node-03:8765"
```

### Full HA Configuration

See [config/replication-ha.example.yaml](../config/replication-ha.example.yaml) for comprehensive configuration with all options.

### Key HA Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `enable_auto_failover` | `true` | Enable automatic failover on leader failure |
| `failure_detection_timeout_ms` | `5000` | Time before marking replica as FAILED |
| `degraded_lag_threshold_ms` | `5000` | Lag threshold for DEGRADED status |
| `max_consecutive_failures` | `3` | Failures before permanent FAILED state |
| `min_quorum_for_failover` | `2` | Minimum healthy replicas for failover |
| `default_read_preference` | `primary_preferred` | Default read routing strategy |

## Deployment Topologies

### 1. Active-Passive (3-Node)

**Configuration**: 1 Primary + 2 Standby replicas

```yaml
replication:
  mode: "semi_sync"
  min_sync_replicas: 1
  enable_auto_failover: true
```

**Topology**:
```
┌─────────┐
│ Node-01 │ ◄─── Primary (LEADER)
└─────────┘
     │
     ├──────► ┌─────────┐
     │        │ Node-02 │  Standby (FOLLOWER, voting)
     │        └─────────┘
     │
     └──────► ┌─────────┐
              │ Node-03 │  Read Replica (FOLLOWER, voting)
              └─────────┘
```

**Use Case**: Standard production setup with automatic failover  
**RPO**: 0 (zero data loss)  
**RTO**: < 30 seconds

### 2. Active-Active (5-Node)

**Configuration**: 5 writable nodes with distributed consensus

```yaml
replication:
  mode: "sync"
  min_sync_replicas: 3  # Quorum of 5
  enable_auto_failover: true
  conflict_strategy: "vector_clock"
```

**Topology**:
```
     ┌─────────┐
     │ Node-01 │ ◄───┐
     └─────────┘     │
           │         │
    ┌──────┴──────┐  │
    ▼             ▼  │
┌─────────┐   ┌─────────┐
│ Node-02 │   │ Node-03 │  All nodes
└─────────┘   └─────────┘  can accept
    │             │        writes
    └──────┬──────┘
           ▼
     ┌─────────┐
     │ Node-04 │
     └─────────┘
           │
           ▼
     ┌─────────┐
     │ Node-05 │
     └─────────┘
```

**Use Case**: High-throughput systems requiring write scalability  
**RPO**: 0  
**RTO**: < 15 seconds (faster due to more candidates)

### 3. Multi-Datacenter (6-Node)

**Configuration**: 2 datacenters with 3 nodes each

```yaml
replication:
  mode: "semi_sync"
  min_sync_replicas: 1  # Per datacenter
  degraded_lag_threshold_ms: 10000  # Higher for cross-DC
  enable_auto_failover: true
```

**Topology**:
```
DC-East (Primary)          DC-West (DR)
┌───────────────────┐      ┌───────────────────┐
│ ┌─────────┐       │      │ ┌─────────┐       │
│ │ Node-01 │◄──────┼──────┼─┤ Node-04 │       │
│ └─────────┘       │      │ └─────────┘       │
│  LEADER           │      │  FOLLOWER         │
│                   │      │                   │
│ ┌─────────┐       │      │ ┌─────────┐       │
│ │ Node-02 │◄──────┼──────┼─┤ Node-05 │       │
│ └─────────┘       │      │ └─────────┘       │
│  FOLLOWER         │      │  FOLLOWER         │
│                   │      │                   │
│ ┌─────────┐       │      │ ┌─────────┐       │
│ │ Node-03 │◄──────┼──────┼─┤ Node-06 │       │
│ └─────────┘       │      │ └─────────┘       │
│  OBSERVER         │      │  OBSERVER         │
└───────────────────┘      └───────────────────┘
```

**Use Case**: Disaster recovery across geographic regions  
**RPO**: 0 (with sync to at least 1 remote replica)  
**RTO**: < 60 seconds (cross-DC failover)

## Failure Detection

### Heartbeat Mechanism

Each node sends periodic heartbeats to replicas:

```cpp
// Configure heartbeat interval
replication:
  heartbeat_interval_ms: 1000  // 1 second
```

### Health Transitions

1. **HEALTHY → DEGRADED**: Lag exceeds `degraded_lag_threshold_ms`
2. **DEGRADED → FAILED**: No heartbeat for `failure_detection_timeout_ms`
3. **FAILED → HEALTHY**: Heartbeat resumes and lag catches up
4. **Any → FAILED**: `max_consecutive_failures` reached

### Network Partition Detection

A network partition is detected when:
- More than 50% of replicas are in FAILED state
- Quorum cannot be established

Actions taken:
- Emit `network_partitions_detected_total` metric
- Trigger `onNetworkPartitionDetected` event
- Disable automatic failover (prevent split-brain)

## Automatic Failover

### Failover Process

1. **Failure Detection**: Health monitor detects leader failure
2. **Quorum Check**: Verify sufficient healthy replicas (`min_quorum_for_failover`)
3. **Leader Election**: Raft-based consensus elects new leader
4. **Candidate Selection**: Priority-based with log completeness check
5. **Promotion**: New leader promoted and notified
6. **Stream Reconnection**: Followers reconnect to new leader

### Failover Timing

```
┌─────────────────────────────────────────────────────────────┐
│ Failure Detection: ~5s (failure_detection_timeout_ms)       │
├─────────────────────────────────────────────────────────────┤
│ Leader Election:   ~3-5s (election_timeout)                 │
├─────────────────────────────────────────────────────────────┤
│ Stream Setup:      ~2-5s (reconnection time)                │
├─────────────────────────────────────────────────────────────┤
│ TOTAL RTO:         ~10-15 seconds (automatic recovery)      │
└─────────────────────────────────────────────────────────────┘
```

### Manual Failover

For planned maintenance or testing:

```bash
# Trigger failover to specific node
curl -X POST http://localhost:8765/api/v1/replication/failover \
  -H "Content-Type: application/json" \
  -d '{"target_node_id": "node-02"}'
```

## Monitoring

### Prometheus Metrics

#### Replication Metrics
```
themisdb_replication_entries_total          # Total entries replicated
themisdb_replication_bytes_total            # Total bytes replicated
themisdb_replication_errors_total           # Replication errors
themisdb_replication_lag_max_ms             # Maximum lag across replicas
themisdb_replication_lag_avg_ms             # Average lag
```

#### HA Metrics
```
themisdb_automatic_failovers_total          # Automatic failovers
themisdb_manual_failovers_total             # Manual failovers
themisdb_replica_failures_detected_total    # Replica failures
themisdb_network_partitions_detected_total  # Network partitions
themisdb_leader_elections_total             # Leader elections
```

### Topology Visualizer (Web UI)

The built-in topology visualizer provides a live, auto-refreshing view of the
replication cluster directly in the browser.

**Endpoints:**

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/ui/replication/topology` | Interactive SVG topology page (auto-refreshes every 5 s) |
| `GET` | `/api/v1/replication/topology` | JSON snapshot: nodes, roles, health, WAL lag |
| `GET` | `/api/v1/replication/health` | Aggregated health summary (quorum, lag, ship stats) |

**Access the visualizer:**

```bash
# Open in browser
open http://localhost:8765/ui/replication/topology

# JSON topology snapshot
curl http://localhost:8765/api/v1/replication/topology | jq .

# JSON health summary
curl http://localhost:8765/api/v1/replication/health | jq .
```

**Topology response example:**

```json
{
  "primary_node_id": "primary-node-1",
  "primary_lsn": 1042,
  "nodes": [
    { "node_id": "primary-node-1", "role": "PRIMARY", "is_primary": true,
      "health_status": "HEALTHY", "replication_lag_ms": 0 },
    { "node_id": "replica-us-east", "role": "REPLICA", "is_primary": false,
      "health_status": "HEALTHY", "replication_lag_ms": 42 }
  ],
  "edges": [
    { "from": "primary-node-1", "to": "replica-us-east", "type": "WAL_STREAM" }
  ],
  "total_nodes": 2,
  "replica_count": 1
}
```

**Health response example:**

```json
{
  "primary_node_id": "primary-node-1",
  "has_quorum": true,
  "total_nodes": 3,
  "healthy_replicas": 2,
  "failed_replicas": 0,
  "max_replication_lag_ms": 42,
  "overall_status": "HEALTHY"
}
```

The primary node ID can be set via the `THEMIS_WAL_PRIMARY_ID` environment variable;
if unset it defaults to an empty string.

### Grafana Dashboard

Import the provided dashboard: [grafana/replication-ha.json](../grafana/replication-ha.json)

Includes panels for:
- Cluster topology visualization
- Replication lag trends
- Failover history
- Health status matrix
- Network partition alerts

### Alerting Rules

```yaml
# High Replication Lag
- alert: HighReplicationLag
  expr: themisdb_replication_lag_max_ms > 30000
  for: 5m
  severity: warning

# Replica Failed
- alert: ReplicaFailed
  expr: increase(themisdb_replica_failures_detected_total[5m]) > 0
  severity: critical

# Network Partition
- alert: NetworkPartition
  expr: increase(themisdb_network_partitions_detected_total[5m]) > 0
  severity: critical

# Frequent Failovers
- alert: FrequentFailovers
  expr: increase(themisdb_automatic_failovers_total[1h]) > 3
  severity: warning
```

## Operational Procedures

### Check Cluster Health

```bash
curl http://localhost:8765/api/v1/replication/health
```

Response:
```json
{
  "role": "LEADER",
  "replicas": [
    {
      "node_id": "node-02",
      "health_status": "HEALTHY",
      "lag_ms": 125
    },
    {
      "node_id": "node-03",
      "health_status": "DEGRADED",
      "lag_ms": 6234
    }
  ],
  "has_quorum": true
}
```

### Promote Node to Leader

```bash
curl -X POST http://localhost:8765/api/v1/replication/promote
```

### Demote Leader (Graceful Stepdown)

```bash
curl -X POST http://localhost:8765/api/v1/replication/demote
```

### Add New Replica

```bash
curl -X POST http://localhost:8765/api/v1/replication/replicas \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": "node-04",
    "endpoint": "node-04:8765",
    "is_voting_member": true,
    "priority": 1
  }'
```

### Remove Replica

```bash
curl -X DELETE http://localhost:8765/api/v1/replication/replicas/node-04
```

## Performance Tuning

### High-Throughput Workloads

```yaml
replication:
  mode: "async"              # Lowest latency
  batch_size: 1000           # Larger batches
  batch_timeout_ms: 200      # Longer batching window
  wal_segment_size_bytes: 268435456  # 256MB segments
```

**Trade-offs**: Higher throughput, higher lag, potential data loss

### Low-Latency Requirements

```yaml
replication:
  mode: "sync"               # Zero data loss
  batch_size: 50             # Smaller batches
  batch_timeout_ms: 10       # Immediate shipping
  wal_sync_on_commit: true   # Force fsync
```

**Trade-offs**: Lower latency for writes, lower throughput

### Multi-Datacenter

```yaml
replication:
  mode: "semi_sync"
  min_sync_replicas: 1       # One per DC
  degraded_lag_threshold_ms: 50000  # 50-100ms for cross-DC
  heartbeat_interval_ms: 2000       # Less frequent heartbeats
```

## Troubleshooting

### Problem: Frequent Failovers

**Symptoms**: `themisdb_automatic_failovers_total` increasing rapidly

**Causes**:
- Network instability
- Insufficient resources (CPU/memory)
- Timeout too aggressive

**Solutions**:
```yaml
# Increase timeouts
failure_detection_timeout_ms: 10000  # 10 seconds
degraded_lag_threshold_ms: 10000

# Reduce failover sensitivity
max_consecutive_failures: 5
min_quorum_for_failover: 3
```

### Problem: High Replication Lag

**Symptoms**: `themisdb_replication_lag_max_ms` > threshold

**Causes**:
- Slow replica hardware
- Network bandwidth limitation
- High write load

**Solutions**:
```yaml
# Increase batch size and timeout
batch_size: 500
batch_timeout_ms: 100

# Scale replicas horizontally
# Add read replicas to distribute load
```

### Problem: Split-Brain Scenario

**Symptoms**: Multiple leaders elected after network partition

**Prevention**:
```yaml
# Ensure odd number of voting members
# Configure proper quorum
min_quorum_for_failover: 2  # For 3-node cluster

# Use datacenter-aware deployment
# Ensure proper network segmentation
```

### Problem: Replica Always DEGRADED

**Symptoms**: Replica stuck in DEGRADED state

**Causes**:
- Persistent lag due to slow disk
- Under-provisioned replica
- Network latency

**Solutions**:
1. Check replica hardware: `iostat -x 1`
2. Increase degraded threshold: `degraded_lag_threshold_ms: 10000`
3. Upgrade replica hardware or add caching
4. Check network latency: `ping -c 100 replica-host`

## API Reference

See [API.md](./API.md) for complete API documentation.

## See Also

### Replication Documentation
- **[REPLICATION_IMPLEMENTATION_STATUS.md](./REPLICATION_IMPLEMENTATION_STATUS.md)** - Detailed implementation status (~85% complete) with component breakdown
- **[replication_raid_plan.md](./replication_raid_plan.md)** - RAID 1/10 readiness plan and implementation roadmap
- **[docs/replication/](./replication/)** - Additional replication documentation and examples

### Related Documentation
- [ARCHITECTURE.md](../ARCHITECTURE.md) - System architecture overview
- [SECURITY.md](../SECURITY.md) - Security configuration for replication
- [MONITORING.md](./production/MONITORING.md) - Monitoring and metrics setup
- [Disaster Recovery](./en/guides/disaster_recovery.md) - DR procedures and best practices
- [Distributed Sharding Architecture](./de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md) - Sharding module documentation
