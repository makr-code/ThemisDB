# Distributed Sharding Architecture Overview

## Executive Summary

ThemisDB v1.4+ introduces a production-ready distributed sharding architecture that enables horizontal scalability, high availability, and multi-datacenter operation. This document provides a comprehensive overview of the sharding architecture and its components.

## Architecture Components

### 1. Shard Router with Pluggable Consensus

The **Shard Router** (`ShardRouter`) is responsible for routing queries to the appropriate shards. It integrates with pluggable consensus modules for distributed coordination.

```
┌─────────────────────────────────────────────────────┐
│                   Shard Router                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────┐ │
│  │ URN Resolver │  │ Query Router │  │ Load     │ │
│  │              │  │              │  │ Balancer │ │
│  └──────────────┘  └──────────────┘  └──────────┘ │
│  ┌──────────────────────────────────────────────┐  │
│  │        Consensus Module (Raft/Gossip/Paxos) │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
         │              │              │
         ▼              ▼              ▼
    ┌────────┐     ┌────────┐     ┌────────┐
    │ Shard1 │     │ Shard2 │     │ Shard3 │
    └────────┘     └────────┘     └────────┘
```

**Supported Consensus Algorithms:**
- **Raft**: Leader-based, strongly consistent
- **Gossip**: Leaderless, eventually consistent  
- **Paxos/Multi-Paxos**: Quorum-based, strongly consistent

### 2. Cross-Shard Transaction Coordinator

The **CrossShardTransactionCoordinator** manages distributed transactions across multiple shards using pluggable transaction protocols.

**Supported Transaction Protocols:**
- **2PC (Two-Phase Commit)**: Blocking, strongly consistent
- **3PC (Three-Phase Commit)**: Non-blocking variant
- **SAGA**: Compensating transactions for long-running workflows
- **Percolator**: Optimistic concurrency for distributed writes

```
Transaction Flow (2PC):
┌────────────┐
│ Client     │
│ Request    │
└──────┬─────┘
       │
       ▼
┌─────────────────────────────────┐
│ Transaction Coordinator         │
│  Phase 1: Prepare               │
│    ├──> Shard 1 (PREPARED)      │
│    ├──> Shard 2 (PREPARED)      │
│    └──> Shard 3 (PREPARED)      │
│  Phase 2: Commit                │
│    ├──> Shard 1 (COMMITTED)     │
│    ├──> Shard 2 (COMMITTED)     │
│    └──> Shard 3 (COMMITTED)     │
└─────────────────────────────────┘
```

### 3. Metadata Sharding

Metadata is horizontally partitioned across multiple metadata shards for scalability.

**Metadata Partitions:**
- **SCHEMA**: Schema definitions
- **INDEX**: Index metadata
- **SHARD_MAP**: Shard mapping information
- **TRANSACTION_LOG**: Transaction log entries
- **STATISTICS**: Statistics and metrics
- **CONFIGURATION**: Configuration data

```
┌─────────────────────────────────────┐
│      Metadata Shard Router          │
└────────┬────────────────────────┬───┘
         │                        │
    ┌────▼────┐              ┌────▼────┐
    │ Meta    │              │ Meta    │
    │ Shard 1 │              │ Shard 2 │
    │ - SCHEMA│              │ - INDEX │
    │ - CONFIG│              │ - STATS │
    └─────────┘              └─────────┘
```

### 4. Distributed Deadlock Detection

Automatic deadlock detection and resolution for cross-shard transactions.

**Features:**
- Wait-for graph construction
- Cycle detection algorithm
- Automatic victim selection (youngest transaction)
- Configurable detection interval

### 5. Snapshot Isolation Across Shards

Provides MVCC-based snapshot isolation for consistent reads across multiple shards.

**Features:**
- TrueTime integration for globally consistent timestamps
- Distributed snapshot management
- Cross-shard consistent reads
- Configurable isolation levels

## Deployment Modes

### Single Datacenter

```
┌─────────────────────────────────────────┐
│         Datacenter (US-East)            │
│  ┌────────┐  ┌────────┐  ┌────────┐    │
│  │ Shard1 │  │ Shard2 │  │ Shard3 │    │
│  │ Raft   │  │ Raft   │  │ Raft   │    │
│  │ Leader │  │ Leader │  │ Leader │    │
│  └────┬───┘  └────┬───┘  └────┬───┘    │
│       │           │           │         │
│  ┌────▼───┐  ┌────▼───┐  ┌────▼───┐    │
│  │Replica1│  │Replica2│  │Replica3│    │
│  └────────┘  └────────┘  └────────┘    │
└─────────────────────────────────────────┘
```

### Multi-Datacenter (Geo-Distributed)

```
┌──────────────────────┐      ┌──────────────────────┐
│ DC1 (US-East)        │      │ DC2 (EU-West)        │
│  ┌────────┐          │      │  ┌────────┐          │
│  │ Shard1 │──────────┼──────┼─>│ Replica│          │
│  │ Paxos  │  WAN     │      │  │        │          │
│  │ Leader │          │      │  └────────┘          │
│  └────────┘          │      │                      │
└──────────────────────┘      └──────────────────────┘
         ▲                            │
         └────────Consensus───────────┘
```

## Key Features

### High Availability

- **Automatic Failover**: Hot spare manager for instant failover
- **Replication**: Configurable replication factor (default: 3)
- **Partition Detection**: Split-brain prevention
- **Health Monitoring**: Continuous health checks with predictive failure detection

### Scalability

- **Horizontal Scaling**: Add shards dynamically
- **Auto-Rebalancing**: Automatic data rebalancing on topology changes
- **Load Detection**: Intelligent load-based routing
- **Metadata Sharding**: Distributed metadata prevents bottlenecks

### Consistency

- **Strong Consistency**: Via Raft or Paxos consensus
- **Snapshot Isolation**: MVCC across shards
- **Linearizability**: TrueTime-based ordering
- **Configurable Isolation**: Multiple isolation levels

### Performance

- **Locality-Aware Routing**: Minimize cross-datacenter traffic
- **Query Pushdown**: Push predicates to shards
- **Parallel Execution**: Scatter-gather queries
- **Result Caching**: Distributed cache layer

## Configuration Example

```yaml
# Sharding Configuration
sharding:
  enabled: true
  num_shards: 3
  replication_factor: 3
  
  # Consensus Configuration
  consensus:
    type: raft  # or gossip, paxos
    heartbeat_interval_ms: 500
    election_timeout_min_ms: 1000
    election_timeout_max_ms: 2000
    enable_partition_detection: true
  
  # Transaction Configuration
  transactions:
    protocol: two_phase_commit  # or saga, percolator
    isolation_level: serializable
    prepare_timeout_ms: 5000
    commit_timeout_ms: 5000
    enable_deadlock_detection: true
  
  # Metadata Configuration
  metadata:
    num_shards: 3
    enable_cache: true
    cache_size: 10000
    replication_factor: 3
  
  # Routing Configuration
  router:
    scatter_timeout_ms: 30000
    max_concurrent_shards: 10
    enable_query_pushdown: true
```

`serializable` should remain the normal default for distributed transactions that must prevent
write-skew and phantom-read anomalies. Switch to `snapshot_isolation` only when those trade-offs
are explicitly acceptable for the workload.

## Migration from Previous Versions

See [Data Migration and Compatibility Matrix](../migration/DATA_MIGRATION_COMPATIBILITY.md) for detailed migration procedures.

## Performance Characteristics

### Write Scalability

- **Single Shard**: 45,000 ops/s
- **3 Shards**: 135,000 ops/s (3x)
- **10 Shards**: 450,000 ops/s (10x)

*Note: Linear scaling with number of shards for shard-local operations*

### Latency

- **Single Shard Write**: ~2ms (local)
- **Cross-Shard Transaction**: ~10ms (2PC overhead)
- **Cross-DC Write**: ~150ms (WAN latency)

### Consensus Overhead

- **Raft**: 1-2 RTT for commit
- **Paxos**: 2-3 RTT for commit
- **Gossip**: Variable (eventual consistency)

## Monitoring and Operations

### Key Metrics

- **Shard Distribution**: Data distribution across shards
- **Replication Lag**: Replication delay
- **Consensus Health**: Quorum status, leader elections
- **Transaction Stats**: Commit rate, abort rate, deadlocks
- **Query Performance**: Latency distribution, throughput

### Operational Commands

```bash
# Check shard status
themis-cli sharding status

# View consensus state
themis-cli consensus status

# Monitor transactions
themis-cli transaction monitor

# Rebalance shards
themis-cli sharding rebalance

# Check metadata consistency
themis-cli metadata validate
```

## Security

- **mTLS**: Certificate-based shard authentication
- **PKI Integration**: Shard certificate management
- **Signed Requests**: Cryptographic request signing
- **Encryption**: At-rest and in-transit encryption

## Future Roadmap

- [ ] Dynamic shard splitting/merging
- [ ] Cross-region active-active replication
- [ ] Byzantine fault tolerance (BFT)
- [ ] CRDTs for conflict-free replication
- [ ] ML-based query routing optimization

## Related Documentation

- [Consensus Module Architecture](CONSENSUS_MODULE.md)
- [Cross-Shard Transactions](CROSS_SHARD_TRANSACTIONS.md)
- [Metadata Sharding](METADATA_SHARDING.md)
- [Migration Guide](../migration/DATA_MIGRATION_COMPATIBILITY.md)

## References

### Academic Papers

1. **Raft Consensus**: Ongaro & Ousterhout (2014). "In Search of an Understandable Consensus Algorithm"
2. **Paxos**: Lamport (2001). "Paxos Made Simple"
3. **Spanner/TrueTime**: Corbett et al. (2012). "Spanner: Google's Globally-Distributed Database"
4. **Percolator**: Peng & Dabek (2010). "Large-scale Incremental Processing Using Distributed Transactions and Notifications"

### Industry Examples

- **Google Spanner**: Globally distributed SQL database
- **CockroachDB**: Distributed SQL with Raft
- **YugabyteDB**: Multi-model distributed database
- **Apache Cassandra**: Wide-column store with gossip protocol

## Support

- **Documentation**: https://makr-code.github.io/ThemisDB/
- **GitHub**: https://github.com/makr-code/ThemisDB
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Enterprise Support**: enterprise@themisdb.com
