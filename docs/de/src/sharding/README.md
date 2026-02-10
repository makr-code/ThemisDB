# Sharding Module

Horizontal scaling and sharding implementation for ThemisDB v1.4+.

## Components

### Core Infrastructure
- Shard manager and topology
- Data distribution strategies
- Consistent hashing
- Shard rebalancing
- TrueTime integration for global consistency

### NEW in v1.4+ - Pluggable Consensus Architecture
- **Consensus Module Interface** - Abstract interface for pluggable consensus
- **Raft Consensus Adapter** - Adapter for existing Raft implementation
- **Gossip Consensus Adapter** - Adapter for Gossip protocol
- **Paxos Consensus** - New Multi-Paxos implementation
- **Consensus Factory** - Runtime consensus selection

### NEW in v1.4+ - Enhanced Transaction Support
- **Cross-Shard Transaction Coordinator** - Pluggable transaction protocols
  - Two-Phase Commit (2PC)
  - Three-Phase Commit (3PC)
  - SAGA (compensating transactions)
  - Percolator (optimistic concurrency)
- **Distributed Deadlock Detection**
- **Snapshot Isolation** across shards

### NEW in v1.4+ - Metadata Sharding
- **Metadata Shard** - Horizontally partitioned metadata
- **Metadata Shard Router** - Consistent hashing for metadata routing
- Partitioned by type: SCHEMA, INDEX, SHARD_MAP, TRANSACTION_LOG, etc.

## Features

### Scalability
- Horizontal data partitioning
- Consistent hashing for shard assignment
- Dynamic shard rebalancing
- Metadata sharding prevents bottlenecks

### Consistency
- **Pluggable consensus algorithms** (Raft, Gossip, Paxos)
- **Multiple transaction protocols** (2PC, 3PC, SAGA, Percolator)
- **ACID guarantees across multiple shards**
- **TrueTime-based external consistency**
- **Snapshot isolation** for distributed reads

### Availability
- Automatic failover with hot spares
- Partition detection and split-brain prevention
- Deadlock detection and resolution
- Multi-datacenter support

## Implementation Status

### ✅ Completed (v1.4)
- Pluggable consensus module architecture
- Raft, Gossip, and Paxos consensus implementations
- Cross-shard transaction coordinator
- Transaction protocol abstraction (2PC, 3PC, SAGA, Percolator)
- Deadlock detection framework
- Metadata sharding design
- Comprehensive documentation

### 🚧 Partial (requires integration)
- Full RPC integration for cross-shard operations
- Persistent state management for Paxos
- Complete metadata shard implementation
- Advanced query optimization

## Documentation

For comprehensive sharding documentation, see:
- **[Distributed Sharding Architecture](../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)** - NEW v1.4!
- **[Consensus Module Architecture](../../docs/de/sharding/CONSENSUS_MODULE.md)** - NEW v1.4!
- **[Data Migration Guide](../../docs/de/migration/DATA_MIGRATION_COMPATIBILITY.md)** - NEW v1.4!
- [Distributed Transactions with 2PC](../../docs/DISTRIBUTED_TRANSACTIONS.md)
- [Sharding Implementation Summary](../../docs/SHARDING_IMPLEMENTATION_SUMMARY.md)
- [Sharding Phase 1 Report](../../docs/SHARDING_PHASE1_REPORT.md)
- [Sharding Phases 1-3 Summary](../../docs/SHARDING_PHASES_1-3_SUMMARY.md)
- [Horizontal Scaling Strategy](../../docs/horizontal_scaling_implementation_strategy.md)

## Quick Start

See usage examples in the architecture documentation above.
