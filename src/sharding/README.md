# Sharding Module

Horizontal scaling and sharding implementation for ThemisDB.

## Components

- Shard manager
- Data distribution strategies
- Consistent hashing
- Shard rebalancing
- **Distributed Transaction Coordinator** - 2PC protocol for cross-shard transactions
- TrueTime integration for global consistency

## Features

- Horizontal data partitioning
- Consistent hashing for shard assignment
- Dynamic shard rebalancing
- Cross-shard query execution
- **Distributed transactions with Two-Phase Commit (2PC)**
- **ACID guarantees across multiple shards**
- **TrueTime-based external consistency**
- **Wait-free read-only transactions**

## Documentation

For sharding and distributed transaction documentation, see:
- **[Distributed Transactions with 2PC](../../docs/DISTRIBUTED_TRANSACTIONS.md)** - NEW!
- [Sharding Implementation Summary](../../docs/SHARDING_IMPLEMENTATION_SUMMARY.md)
- [Sharding Phase 1 Report](../../docs/SHARDING_PHASE1_REPORT.md)
- [Sharding Phases 1-3 Summary](../../docs/SHARDING_PHASES_1-3_SUMMARY.md)
- [Horizontal Scaling Strategy](../../docs/horizontal_scaling_implementation_strategy.md)
