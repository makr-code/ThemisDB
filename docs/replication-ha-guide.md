## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# High Availability Replication - Implementation Guide

> Alignment note (2026-05-31): This guide is a secondary operational document.
> Authoritative current workload and target behavior are defined in:
> - `src/replication/FUTURE_ENHANCEMENTS.md`
> - `src/replication/MODULE_GAPS.md`
> - `src/replication/ROADMAP.md`
> If this guide conflicts with newer planning docs, the planning docs take precedence.

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
- ✅ **Witness Nodes** - Vote-only members for quorum in 2-node data clusters (no WAL overhead)

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
> - [REPLICATION_IMPLEMENTATION_STATUS.md](reports/REPLICATION_IMPLEMENTATION_STATUS.md) - Detailed status of WAL components
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

### 4. 2-Node + Witness (Minimal HA)

**Configuration**: 1 Primary + 1 data Follower + 1 lightweight Witness node

A *witness node* participates in leader-election voting (and therefore
contributes to quorum) but **does not receive WAL data** and never holds any
database records.  This lets a 2-node data cluster maintain a majority quorum
(2 out of 3 voting members) without provisioning a third full data replica —
ideal for cost-sensitive or resource-constrained environments.

```yaml
replication:
  mode: "semi_sync"
  min_sync_replicas: 1          # Wait for the data follower only
  enable_auto_failover: true
  min_quorum_for_failover: 2    # Leader + witness = 2 is sufficient
  seed_nodes:
    - "node-01.cluster.local:8765"
    - "node-02.cluster.local:8765"
    # Witness-01 can optionally appear in seed_nodes for cluster-membership
    # discovery (heartbeat / election RPCs).  ThemisDB will never create a
    # WAL replication stream to a WITNESS node regardless of seed_nodes — the
    # distinction is made by the role set in addWitnessNode(), not by
    # seed_nodes membership.
    # - "witness-01.cluster.local:8765"
```

**API usage** (adding the witness at runtime):
```cpp
// Registers a vote-only node; no WAL stream is created for it.
replication_manager.addWitnessNode("witness-01", "10.0.0.3:8765");
```

**Topology**:
```
┌─────────┐
│ Node-01 │ ◄─── Primary (LEADER, data)
└─────────┘
     │
     └──────► ┌─────────┐
              │ Node-02 │  Standby (FOLLOWER, data + voting)
              └─────────┘

              ┌────────────┐
              │ Witness-01 │  Vote-only (WITNESS, no data, voting)
              └────────────┘
                   │
                   └── Contributes to quorum; never stores data
```

**Behaviour summary**:

| Property | FOLLOWER | WITNESS |
|---|---|---|
| Receives WAL / stores data | ✅ | ❌ |
| Votes in leader election | ✅ | ✅ |
| Counts toward `hasQuorum()` | ✅ | ✅ |
| Eligible for read routing | ✅ | ❌ |
| Can be promoted to leader | ✅ | ❌ (priority = 0) |

**Quorum arithmetic**:  
3 voting members (leader + follower + witness). Quorum = ⌊3/2⌋ + 1 = **2**.  
The cluster remains available as long as **any 2 of the 3 voting members are
reachable** — including when the witness is the surviving non-primary node.

**Use Case**: Cost-efficient 2-node HA (witness runs on a small VM or edge device)  
**RPO**: 0 (semi-sync with `min_sync_replicas: 1`)  
**RTO**: < 30 seconds (same as 3-Node Active-Passive)

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

#### Cross-Cluster Pub/Sub Metrics
```
themisdb_cross_cluster_publication_published_total{publication}  # Entries published
themisdb_cross_cluster_publication_subscribers{publication}      # Active subscribers
themisdb_cross_cluster_subscription_applied_total{subscription}  # Entries applied
themisdb_cross_cluster_subscription_errors_total{subscription}   # Apply errors
themisdb_cross_cluster_subscription_last_applied_sequence{subscription}  # Last seq
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

## Cross-Cluster Publish/Subscribe Replication

Cross-cluster logical replication allows one ThemisDB cluster (the *publisher*)
to stream filtered WAL changes to one or more remote clusters (the *subscribers*)
using an asynchronous publish/subscribe model.

### How It Works

```
Publisher Cluster                   Subscriber Cluster
─────────────────                   ──────────────────
WALManager                          CrossClusterSubscription
     │ onWALEntryApplied()               │ applyEntry(WALEntry)
     ▼                                   │
CrossClusterPublication ──callback──────▶│
  (filter: collection/op)            local storage
```

1. A `CrossClusterPublication` is attached to the publisher's `WALManager` via
   `addListener()`.  The publication applies an optional `PublicationFilter`
   and fans out matching entries to all registered subscriber callbacks.
2. A `CrossClusterSubscription` on the target cluster registers its delivery
   callback with the publication and applies each received entry locally.

### Publisher Configuration

```cpp
// Create a named publication
auto pub = std::make_shared<CrossClusterPublication>("orders_pub");

// Optional: filter by collection and/or operation type
PublicationFilter filter;
filter.include_collections = {"orders", "customers"};  // empty = all collections
filter.include_operations  = {"INSERT", "UPDATE"};      // empty = all operations
pub->setFilter(filter);

// Attach to the WAL pipeline so every committed entry flows in automatically
replication_manager.addListener(pub);
```

### Subscriber Configuration

```cpp
// Create a subscription that applies incoming entries to a local WAL/store
CrossClusterSubscription sub(
    "orders_sub",          // unique name
    pub,                   // shared pointer to the publication
    [&](const WALEntry& e) {
        local_storage.apply(e);  // user-supplied apply function
    }
);
sub.enable();

// Monitor progress
uint64_t last_seq = sub.lastAppliedSequence();  // last applied WAL seq
uint64_t errors   = sub.errorCount();           // apply errors (non-fatal)
```

### Prometheus Metrics

Cross-cluster publications and subscriptions expose Prometheus metrics via
`exportPrometheusMetrics()`:

```
# Publication
themisdb_cross_cluster_publication_published_total{publication="orders_pub"}
themisdb_cross_cluster_publication_subscribers{publication="orders_pub"}

# Subscription
themisdb_cross_cluster_subscription_applied_total{subscription="orders_sub"}
themisdb_cross_cluster_subscription_errors_total{subscription="orders_sub"}
themisdb_cross_cluster_subscription_last_applied_sequence{subscription="orders_sub"}
```

### Constraints & Notes

- **Asynchronous delivery**: Cross-cluster pub/sub is inherently asynchronous.
  There is no back-pressure or flow-control between publisher and subscriber.
- **Apply errors are non-fatal**: A failed `applyEntry` call is counted in
  `error_count` and logged, but the subscription continues processing subsequent
  entries.
- **Multiple subscribers**: A single publication supports any number of
  independent subscriber callbacks.
- **CDC stream authentication**: Authentication of the CDC stream is the
  responsibility of downstream consumers (see [SECURITY.md](./SECURITY.md)).

## Multi-Region Active-Active with Bounded Staleness

Multi-region active-active deployments allow every region to accept writes
simultaneously.  `MultiRegionActiveActiveManager` coordinates consistency
guarantees across regions by tracking per-region replication lag and
enforcing the requested `ConsistencyLevel` on each read.

### Consistency Levels

| Level | Guarantee | Rejected when |
|---|---|---|
| `STRONG` | Linearizable – local replica is fully up-to-date | `staleness_ms > 0` |
| `BOUNDED_STALENESS` | Stale reads permitted up to `max_staleness_ms` | `staleness_ms > max_staleness_ms` |
| `SESSION` | Read-your-writes within a session token | local sequence < token sequence |
| `EVENTUAL` | No guarantee – best availability | Never rejected |

### Configuration

```cpp
MultiRegionActiveActiveConfig cfg;
cfg.local_region_id     = "us-east-1";
cfg.peer_region_ids     = {"eu-west-1", "ap-south-1"};
cfg.default_consistency = ConsistencyLevel::BOUNDED_STALENESS;
cfg.max_staleness_ms    = 5000;   // 5-second staleness bound
cfg.session_token_ttl_ms = 30000; // session tokens valid for 30 s
cfg.conflict_strategy   = ConflictResolution::LAST_WRITE_WINS;

MultiRegionActiveActiveManager mgr(cfg);
```

### Writing with a Session Token

```cpp
// Write returns a session token embedding the write sequence.
auto w = mgr.write("orders", "ord-1", "INSERT", payload,
                   ConsistencyLevel::SESSION);

// Pass the session token back on the next read to guarantee read-your-writes.
auto r = mgr.read("orders", "ord-1", ConsistencyLevel::SESSION, w.session_token);
assert(r.success);  // guaranteed because local seq >= w.sequence_number
```

### Bounded-Staleness Read

```cpp
// The read succeeds only when local lag <= 5 s.
auto r = mgr.read("analytics", "summary", ConsistencyLevel::BOUNDED_STALENESS);
if (!r.success) {
    // staleness exceeded; retry on a fresher replica or degrade to EVENTUAL
}
```

### Feeding Staleness Updates from the Replication Layer

The replication layer must call `updateRegionStaleness()` whenever a
heartbeat or WAL acknowledgement arrives from a remote region so that
the manager has up-to-date lag information:

```cpp
// Called by ReplicationManager on each heartbeat ACK from eu-west-1
mgr.updateRegionStaleness(
    "eu-west-1",
    peer_lag_ms,              // measured replication lag in ms
    peer_last_applied_seq     // last WAL sequence applied by that region
);
```

### Prometheus Metrics

```
# Total writes accepted by the local region
themisdb_mraaa_writes_total{region="us-east-1"}

# Total read attempts (all consistency levels combined)
themisdb_mraaa_reads_total{region="us-east-1"}

# Reads rejected because staleness exceeded the configured bound
themisdb_mraaa_staleness_rejections_total{region="us-east-1"}

# Reads per consistency level
themisdb_mraaa_strong_reads_total{region="us-east-1"}
themisdb_mraaa_bounded_staleness_reads_total{region="us-east-1"}
themisdb_mraaa_session_reads_total{region="us-east-1"}
themisdb_mraaa_eventual_reads_total{region="us-east-1"}

# STRONG writes rejected because the local region is not the designated leader
themisdb_mraaa_leader_write_rejections_total{region="us-east-1"}

# Current replication lag per region (gauge)
themisdb_mraaa_region_staleness_ms{region="eu-west-1"}
```

### Constraints & Notes

- **STRONG consistency is region-local only**: Cross-region writes are always
  eventual unless all writes are routed exclusively through the designated
  `leader_region_id`.  Mixing STRONG writes across regions **will not** provide
  linearisability and may result in lost updates or split-brain divergence.
- **Vector-clock conflict resolution is non-deterministic for concurrent writes**:
  LAST_WRITE_WINS and MERGE strategies may produce different outcomes when two
  regions accept writes to the same document at the same time.  For SERIALIZABLE
  workloads, all writes must go to a single region.
- **Staleness feeds are caller-managed**: the manager does not measure lag
  itself; the replication layer must call `updateRegionStaleness()` on every
  WAL ACK or heartbeat.
- **No cross-region RPC**: `write()` and `read()` operate on local state
  only.  Actual WAL shipping to peer regions is handled by the underlying
  `ReplicationManager`.
- **STRONG reads require 0 lag**: `staleness_ms` must be exactly 0 for a
  STRONG read to succeed.  A local `write()` resets the local region's
  staleness to 0 immediately.
- **Session tokens are stateless**: tokens embed the required sequence and
  an expiry timestamp and are validated without server-side storage.

### Per-Collection Consistency Overrides

Individual collections (tables) can enforce a different consistency level than
the global default.  Use `collection_consistency_overrides` in the configuration
to require STRONG consistency for critical data while keeping the default at
BOUNDED_STALENESS for bulk workloads.

```cpp
MultiRegionActiveActiveConfig cfg;
cfg.local_region_id     = "us-east-1";
cfg.peer_region_ids     = {"eu-west-1"};
cfg.default_consistency = ConsistencyLevel::BOUNDED_STALENESS;
cfg.max_staleness_ms    = 5000;

// Critical financial data must always be strongly consistent
cfg.collection_consistency_overrides["financial_ledger"] = ConsistencyLevel::STRONG;
// Analytics events can tolerate any lag
cfg.collection_consistency_overrides["page_views"]       = ConsistencyLevel::EVENTUAL;

MultiRegionActiveActiveManager mgr(cfg);

// The override takes precedence over the caller-supplied level:
auto r = mgr.read("financial_ledger", "acct-42",
                  ConsistencyLevel::BOUNDED_STALENESS); // → enforces STRONG
```

> **Warning**: Configuring `STRONG` on a collection that is also written from
> non-leader regions will cause all writes to that collection to be rejected
> unless a `leader_region_id` is set and the write arrives at that leader.

### Leader Region for Critical Cross-Region Writes

To prevent split-brain lost-update scenarios for critical workloads, designate
one region as the exclusive acceptor of STRONG writes.  Non-leader regions
reject STRONG writes and must route them to the leader.

```cpp
MultiRegionActiveActiveConfig cfg;
cfg.local_region_id  = "eu-west-1";
cfg.peer_region_ids  = {"us-east-1", "ap-south-1"};
cfg.leader_region_id = "eu-west-1";   // This region is the STRONG-write leader

MultiRegionActiveActiveManager mgr(cfg);

// Accepted – local region IS the leader
auto w = mgr.write("orders", "ord-1", "INSERT", payload,
                   ConsistencyLevel::STRONG);
assert(w.success && w.is_leader_region);

// Non-leader regions should check w.success == false and retry on the leader.
```

Non-leader regions should forward STRONG write requests to the leader via their
application routing layer; ThemisDB does not perform cross-region RPC itself.

> **Trade-off**: Designating a single leader for STRONG writes reduces
> availability during leader-region outages and increases write latency from
> remote regions (cross-datacenter round-trip penalty).  For workloads that
> can tolerate bounded staleness, use BOUNDED_STALENESS or SESSION instead.

### Split-Brain Detection

Enable heuristic split-brain detection to surface potential network partitions:

```cpp
cfg.split_brain_detection_enabled = true;

MultiRegionActiveActiveManager mgr(cfg);

// Called periodically (e.g. before a critical STRONG write)
if (mgr.isSplitBrain()) {
    // All configured peer regions have exceeded the staleness bound.
    // Consider rejecting writes or alerting operations.
    LOG_WARN("Possible network partition – refusing critical write");
}
```

`isSplitBrain()` returns `true` when every peer region reports unhealthy
staleness (> `max_staleness_ms * 2`).  It is a heuristic: a false negative
is possible if staleness feeds are delayed.  Combine with infrastructure-level
health checks for production use.



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
- **[REPLICATION_IMPLEMENTATION_STATUS.md](reports/REPLICATION_IMPLEMENTATION_STATUS.md)** - Historical implementation snapshot with component breakdown
- **[replication_raid_plan.md](./replication_raid_plan.md)** - RAID 1/10 readiness plan and implementation roadmap
- **[docs/replication/](./replication/)** - Additional replication documentation and examples

### Related Documentation
- [ARCHITECTURE.md](../ARCHITECTURE.md) - System architecture overview
- [SECURITY.md](../SECURITY.md) - Security configuration for replication
- [MONITORING.md](./production/MONITORING.md) - Monitoring and metrics setup
- [Disaster Recovery](./en/guides/disaster_recovery.md) - DR procedures and best practices
- [Distributed Sharding Architecture](./de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md) - Sharding module documentation
