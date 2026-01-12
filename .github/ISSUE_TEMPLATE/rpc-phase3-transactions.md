---
name: 🔌 RPC Phase 3 - Transaction Support Implementation
about: Implement distributed transaction support with 2PC, snapshot isolation, and ACID guarantees
title: "[RPC-P3] Implement Transaction Support (Begin/Commit/Abort with 2PC)"
labels: ["type:feature", "priority:P0", "area:networking", "area:storage", "area:sharding", "effort:large"]
assignees: []
---

## 📋 Summary

Implement distributed transaction support in the RPC service with two-phase commit (2PC), snapshot isolation, and full ACID guarantees. This is Phase 3 of the RPC service implementation plan.

**Part of**: RPC Service Full Implementation  
**Phase**: 3 of 4  
**Duration**: 5-7 days  
**LOC**: ~400 lines  
**Priority**: P0 (Critical - Enables distributed transactions)

## 🎯 Problem Statement

Distributed ThemisDB requires transaction support across multiple nodes/shards. The RPC service must implement:
- Transaction lifecycle management (begin, commit, abort)
- Two-phase commit (2PC) protocol for distributed transactions
- Snapshot isolation for consistent reads
- Deadlock detection and prevention
- Transaction recovery on failure

## 🏗️ Implementation Tasks

### Task 1: Transaction Begin (Day 1)
- [ ] Remove TODO from `handleTransactionBegin()`
- [ ] Generate unique transaction ID
- [ ] Create snapshot for snapshot isolation
- [ ] Register transaction with TransactionManager
- [ ] Set transaction timeout
- [ ] Return transaction ID and snapshot timestamp

**Implementation:**
```cpp
json handleTransactionBegin(const json& params) {
    IsolationLevel isolation = params.value("isolation", IsolationLevel::Snapshot);
    
    auto tx = tx_manager_->beginTransaction(isolation);
    
    return createSuccess({
        {"transaction_id", tx->getId()},
        {"snapshot_timestamp", tx->getSnapshotTimestamp()},
        {"isolation_level", isolationLevelToString(isolation)}
    });
}
```

### Task 2: Transaction Commit - Phase 1 (Day 2-3)
- [ ] Implement prepare phase of 2PC
- [ ] Validate all operations in transaction
- [ ] Check for conflicts with concurrent transactions
- [ ] Write prepare log entry
- [ ] Lock resources for commit
- [ ] Vote "commit" or "abort" to coordinator

**2PC Prepare Phase:**
1. Validate all transaction operations
2. Check for conflicts (read-write, write-write)
3. Write prepare record to WAL
4. Acquire commit locks
5. Return vote to coordinator

### Task 3: Transaction Commit - Phase 2 (Day 3-4)
- [ ] Remove TODO from `handleTransactionCommit()`
- [ ] Implement commit phase of 2PC
- [ ] Apply all transaction changes atomically
- [ ] Write commit log entry
- [ ] Release locks
- [ ] Update transaction status
- [ ] Clean up transaction context

**2PC Commit Phase:**
1. Receive commit decision from coordinator
2. Apply all changes via WriteBatch
3. Write commit record to WAL
4. Release all locks
5. Notify transaction completion

### Task 4: Transaction Abort (Day 4-5)
- [ ] Remove TODO from `handleTransactionAbort()`
- [ ] Rollback all transaction changes
- [ ] Write abort log entry
- [ ] Release all locks
- [ ] Clean up transaction resources
- [ ] Handle partial rollback scenarios

**Abort Scenarios:**
- Explicit abort by client
- Conflict detection (deadlock, serialization failure)
- Timeout exceeded
- Coordinator-initiated abort
- Node failure during transaction

### Task 5: Distributed Coordination (Day 5)
- [ ] Implement coordinator role for distributed TX
- [ ] Implement participant role for distributed TX
- [ ] Handle cross-shard transaction coordination
- [ ] Implement transaction recovery protocol
- [ ] Add heartbeat mechanism for liveness detection

**Coordinator Responsibilities:**
- Orchestrate 2PC across participants
- Collect votes from all participants
- Make commit/abort decision
- Handle participant failures

**Participant Responsibilities:**
- Execute local transaction operations
- Participate in 2PC protocol
- Maintain transaction state
- Recover from failures

### Task 6: Deadlock Detection (Day 6)
- [ ] Implement wait-for graph construction
- [ ] Add cycle detection algorithm
- [ ] Implement deadlock resolution (victim selection)
- [ ] Add timeout-based deadlock detection
- [ ] Log deadlock events for monitoring

### Task 7: Transaction Recovery (Day 6-7)
- [ ] Implement crash recovery for in-flight transactions
- [ ] Reconstruct transaction state from WAL
- [ ] Complete or abort interrupted transactions
- [ ] Clean up orphaned transaction resources
- [ ] Test recovery scenarios

### Task 8: Testing & Metrics (Day 7)
- [ ] Unit tests for transaction lifecycle
- [ ] Integration tests for distributed TX
- [ ] Test conflict scenarios (deadlock, timeout)
- [ ] Test recovery from failures
- [ ] Add transaction metrics

## 📝 Implementation Notes

### Transaction State Machine
```
BEGIN → ACTIVE → PREPARING → PREPARED → COMMITTING → COMMITTED
                      ↓                        ↓
                   ABORTING ← ← ← ← ← ← ← ABORTED
```

### 2PC Protocol Flow
```
Coordinator                Participant1          Participant2
     |                          |                     |
     |---PREPARE--------------->|                     |
     |---PREPARE-------------------------------->|
     |                          |                     |
     |<--VOTE-YES---------------|                     |
     |<--VOTE-YES--------------------------------|
     |                          |                     |
     |---COMMIT---------------->|                     |
     |---COMMIT----------------------------------->|
     |                          |                     |
     |<--ACK--------------------|                     |
     |<--ACK---------------------------------------|
```

### Snapshot Isolation
- Each transaction reads from a consistent snapshot
- Snapshot timestamp determined at BEGIN
- Writes are visible only after COMMIT
- Read-write conflicts detected at commit time

## 🧪 Testing

### Unit Tests
- Test transaction begin/commit/abort
- Test 2PC protocol (mock participants)
- Test conflict detection
- Test deadlock detection
- Target: > 85% code coverage

### Integration Tests
- Test distributed transactions across shards
- Test concurrent transactions
- Test conflict scenarios (lost update, write skew)
- Test deadlock scenarios
- Test recovery from node failures

### Stress Tests
- Test with 1000+ concurrent transactions
- Test with high contention workloads
- Test with network partitions
- Test with node crashes during commit

## ✅ Acceptance Criteria

- [ ] All 3 transaction methods implemented (begin, commit, abort)
- [ ] All TODO comments removed from transaction methods
- [ ] 2PC protocol working across multiple nodes
- [ ] Snapshot isolation enforced
- [ ] Deadlock detection functional
- [ ] Transaction recovery working
- [ ] Unit tests passing (> 85% coverage)
- [ ] Integration tests passing
- [ ] Performance benchmarks met:
  - Transaction begin < 5ms
  - Transaction commit < 50ms
  - Deadlock detection < 100ms
- [ ] Documentation updated

## 📚 Resources

### Key Files
- `include/transaction/transaction_manager.h` - Transaction interface
- `include/storage/rocksdb_wrapper.h` - Atomic writes
- `src/transaction/transaction_manager.cpp` - TX implementation

### Examples
- `tests/test_transaction_manager.cpp` - Transaction usage
- `tests/test_distributed_transactions.cpp` - Distributed TX examples
- `tests/test_mvcc.cpp` - Snapshot isolation

### Documentation
- [Implementation Plan](../../docs/planning/rpc-implementation-plan.md)
- [Transaction Manager Documentation](../../include/transaction/transaction_manager.h)
- [Distributed Transactions](../../docs/de/development/GAPS_STUBS_SUMMARY.md)

## 🔗 Related Issues

- **Depends On**: Phase 1 (CRUD), Phase 2 (Queries)
- **Blocks**: Distributed Sharding, Multi-node Replication
- **Related**: Issue #5 (Distributed Transaction Completion)

## 📅 Timeline

| Day | Tasks | Deliverable |
|-----|-------|-------------|
| 1 | Transaction begin | TX lifecycle starts |
| 2-3 | 2PC prepare phase | Prepare working |
| 3-4 | 2PC commit phase | Commit working |
| 4-5 | Transaction abort | Abort/rollback working |
| 5 | Distributed coordination | Cross-shard TX |
| 6 | Deadlock detection | Deadlock resolution |
| 6-7 | Recovery & testing | Phase 3 complete |

**Total**: 5-7 days

---

**Created**: 2026-01-12  
**Phase**: 3/4  
**Depends**: Phase 1, Phase 2
