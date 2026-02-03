# Distributed Sharding Issue Templates

This directory contains specialized issue templates for ThemisDB's distributed sharding infrastructure.

## Available Templates

### 1. **Sharding Bug Report** (`sharding_bug.md`)
For reporting bugs in distributed sharding, consensus modules, or transaction coordination.

**Use this template when:**
- Consensus algorithm (Raft/Paxos/Gossip) behaves incorrectly
- Cross-shard transactions fail or produce inconsistent results
- Shard router has routing issues
- Metadata sharding problems
- Deadlock detection failures
- Split-brain scenarios or data inconsistencies

**Auto-applied labels:** `type:bug`, `area:sharding`, `priority:P1`

### 2. **Sharding Feature Request** (`sharding_feature.md`)
For suggesting new features in distributed systems, consensus, or transactions.

**Use this template when:**
- Proposing a new consensus algorithm or enhancement
- Requesting a new transaction protocol (e.g., Calvin, Spanner-style)
- Suggesting improvements to shard routing
- Proposing metadata sharding enhancements
- Requesting new distributed systems capabilities

**Auto-applied labels:** `type:feature`, `area:sharding`, `priority:P2`

### 3. **Sharding Performance Issue** (`sharding_performance.md`)
For reporting performance problems in distributed operations.

**Use this template when:**
- Consensus latency is higher than expected
- Transaction throughput is below target
- Excessive network overhead in distributed operations
- Deadlock detection performance issues
- Metadata operation bottlenecks

**Auto-applied labels:** `type:performance`, `area:sharding`, `priority:P2`

### 4. **Consensus Implementation Task** (`consensus_implementation.md`)
For tracking completion of consensus module stub implementations.

**Use this template when:**
- Completing a TODO in Raft/Gossip/Paxos adapters
- Implementing persistence for consensus state
- Adding missing consensus functionality
- Integrating consensus with other subsystems

**Auto-applied labels:** `type:refactoring`, `area:sharding`, `priority:P1`, `status:ready`

### 5. **Transaction Protocol Implementation Task** (`transaction_implementation.md`)
For tracking completion of transaction protocol stub implementations.

**Use this template when:**
- Implementing RPC communication for 2PC/3PC/SAGA/Percolator
- Adding transaction state persistence
- Completing deadlock detection
- Implementing recovery logic
- Adding cross-shard transaction features

**Auto-applied labels:** `type:refactoring`, `area:sharding`, `priority:P1`, `status:ready`

## Label Guidelines for Sharding Issues

### Priority Labels
Based on the INTEGRATION_CHECKLIST.md priorities:

- **P0 (Critical)**: Data loss, split-brain, security vulnerabilities
- **P1 (High)**: RPC integration, Paxos persistence, transaction failures
- **P2 (Medium)**: Raft adapter completion, SAGA execution, performance optimization
- **P3 (Low)**: Documentation improvements, optional enhancements

### Area Labels
Common combinations for sharding issues:

- `area:sharding` - All distributed sharding issues
- `area:replication` - WAL, replication consistency
- `area:networking` - RPC, gRPC, network protocols
- `area:storage` - RocksDB integration, persistence
- `area:monitoring` - Metrics, observability for distributed systems

### Status Labels
Track implementation progress:

- `status:ready` - Ready to be worked on
- `status:in-progress` - Currently being implemented
- `status:blocked` - Blocked by another issue (e.g., RPC infrastructure)
- `status:needs-review` - Implementation complete, needs review
- `status:on-hold` - Waiting for design decision

### Effort Labels
Based on INTEGRATION_CHECKLIST.md estimates:

- `effort:small` - < 1 day (e.g., single TODO completion)
- `effort:medium` - 1-3 days (e.g., RPC integration, Paxos persistence)
- `effort:large` - 1-2 weeks (e.g., full protocol implementation)
- `effort:x-large` - > 2 weeks (e.g., new consensus algorithm)

## Creating Issues from Stub TODOs

The distributed sharding implementation has 10 documented TODO markers. Use the implementation task templates to track their completion:

### Priority 1 (High Risk - Required Before Production)
1. **RPC Integration** (2-3 days)
   - File: `src/sharding/cross_shard_transaction.cpp`
   - Use: `transaction_implementation.md` template
   - Labels: `type:refactoring`, `area:sharding`, `priority:P1`, `effort:medium`

2. **Paxos Persistence** (1 day)
   - File: `src/sharding/paxos_consensus.cpp`
   - Use: `consensus_implementation.md` template
   - Labels: `type:refactoring`, `area:sharding`, `priority:P1`, `effort:small`

### Priority 2 (Medium Risk - Post-Launch)
3. **Raft Adapter Completion** (1-2 days)
   - File: `src/sharding/raft_consensus_adapter.cpp`
   - Use: `consensus_implementation.md` template
   - Labels: `type:refactoring`, `area:sharding`, `priority:P2`, `effort:medium`

4. **SAGA Execution** (1-2 days)
   - File: `src/sharding/cross_shard_transaction.cpp`
   - Use: `transaction_implementation.md` template
   - Labels: `type:refactoring`, `area:sharding`, `priority:P2`, `effort:medium`

### Priority 3 (Low Risk - Optional)
5. **Metadata Shard Implementation** (2-3 days)
   - File: `include/sharding/metadata_shard.h`
   - Use: `sharding_feature.md` template
   - Labels: `type:feature`, `area:sharding`, `priority:P3`, `effort:medium`

## Quick Reference

| Template | Type | Priority | When to Use |
|----------|------|----------|-------------|
| `sharding_bug.md` | Bug | P1 | Incorrect behavior in distributed system |
| `sharding_feature.md` | Feature | P2 | New distributed systems capability |
| `sharding_performance.md` | Performance | P2 | Slow consensus/transactions |
| `consensus_implementation.md` | Refactoring | P1 | Complete consensus TODO |
| `transaction_implementation.md` | Refactoring | P1 | Complete transaction TODO |

## Additional Resources

- **Architecture Documentation**: `docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md`
- **Consensus Module Guide**: `docs/de/sharding/CONSENSUS_MODULE.md`
- **Quick Start Guide**: `docs/de/sharding/QUICK_START_GUIDE.md`
- **Integration Checklist**: `INTEGRATION_CHECKLIST.md`
- **Implementation Summary**: `DISTRIBUTED_SHARDING_IMPLEMENTATION_SUMMARY.md`
- **Label Guide**: `.github/LABELS_GUIDE.md`

## Contributing

When creating an issue using these templates:

1. **Choose the right template** based on the type of issue
2. **Fill out all required sections** - don't skip the checklist
3. **Apply appropriate labels** - the template provides defaults, but add more as needed
4. **Link related issues** - reference related bugs, features, or implementation tasks
5. **Be specific** - include configuration, logs, metrics, and reproduction steps

For questions about distributed sharding issues, refer to the documentation or ask in GitHub Discussions.
