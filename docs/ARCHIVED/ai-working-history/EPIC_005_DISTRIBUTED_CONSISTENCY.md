# EPIC-005: Distributed Consistency Hardening (2PC/WAL/Replication/Raft)

**Status:** 🔵 Planned  
**Target Release:** v2.4 Stable (2026-11-30)  
**Timeline:** 10 Weeks (W40-W49, 2026-10-06 to 2026-12-07)  
**Owner:** Distributed Systems Team  
**Scope:** Unify commit-orchestration semantics, consolidate WAL recovery, verify cross-module consistency

## Epic Summary

Harden and unify distributed consistency infrastructure across sharding + replication modules. Consolidate duplicated 2PC/3PC logic, establish unified WAL recovery contract, extend FK validation with performance guarantees, and verify Raft membership safety at scale.

**Key Issues Addressed:**
- Issue #5373: Commit/Prepare/Abort/WAL/Recovery Inventory (2PC/3PC duplication)
- Issue #5390: Cross-Shard FK Validation (already implemented, needs hardening)
- Raft Membership Safety Audit (JOINT→COMMIT consensus verification)

## Success Criteria

- ✅ **2PC/3PC Protocol Unified:** Single coordinator implementation, ~400 lines duplicate code eliminated
- ✅ **WAL Recovery Contract Unified:** Single recoverFromWAL interface, 4 paths consolidated
- ✅ **FK Validation at Scale:** Batch validation API, < 50ms for 1K checks @ 10 shards
- ✅ **Raft Membership Safety:** Property-based tests verify no split-brain
- ✅ **Zero Regression:** TPC-H distributed workloads unchanged
- ✅ **Production Ready:** v2.4 stable certified for hyperscale deployments

## Key Deliverables

| Deliverable | Scope | Verification |
|-------------|-------|--------------|
| Unified 2PC coordinator | Merge CrossShardTransactionCoordinator + TwoPhaseCommitCoordinator | 20+ protocol tests PASS |
| Unified WAL recovery interface | recoverFromWAL() standardized | 4 recovery paths consolidated |
| FK batch validation API | CrossShardForeignKeyValidator extension | Performance: < 50ms/1K @ 10 shards |
| Raft membership audit | JOINT→COMMIT state machine review | 15+ property-based tests |
| Integration tests | Full TPC-H workload | Zero regressions, latency < 2% |

## Dependencies

- **Prerequisite:** EPIC-002 Sharding module hardening (2PC timeout safety)
- **Parallel:** EPIC-002 (weeks 40-42 overlap), EPIC-003 Phase 6 Scanner
- **Supports:** v2.4 stable release, hyperscale production deployment

## Sub-Issues

### [005-A] 2PC/3PC Protocol Unification & Documentation
- **Title:** `Distributed: 2PC/3PC Protocol Unification (Issue #5373 Continuation)`
- **Type:** Enhancement / Refactoring
- **Labels:** `distributed-consistency`, `2pc`, `refactoring`, `issue-5373`
- **Estimated Effort:** 40 hours
- **Scope:** Merge CrossShardTransactionCoordinator + TwoPhaseCommitCoordinator
- **Key Tasks:**
  - Analyze both implementations (currently ~400 lines duplicate)
  - Define unified protocol state machine (prepare→precommit→commit/abort)
  - Merge implementations into single UnifiedTwoPhaseCommitCoordinator
  - Update protocol documentation
  - Migrate 3PC callback handling
- **Acceptance Criteria:**
  - Single coordinator implementation active (zero duplicates)
  - Protocol specification document complete
  - 20+ protocol state machine tests PASS
  - Zero regression on 2PC + 3PC workloads
  - Code review + architecture board approval

### [005-B] WAL Recovery Contract Consolidation
- **Title:** `Distributed: WAL Recovery Contract Consolidation`
- **Type:** Enhancement / Architecture
- **Labels:** `distributed-consistency`, `wal-recovery`, `durability`
- **Estimated Effort:** 35 hours
- **Scope:** Consolidate 4 WAL recovery paths:
  1. Sharding coordinator recovery (CrossShardTransactionCoordinator::recoverFromWAL)
  2. Sharding participant recovery (TwoPhaseCommitParticipant::recoverFromWAL)
  3. Replication logical slot recovery (LogicalReplication::loadPersistedSlots)
  4. Raft membership recovery (RaftV2::applyEntry for membership WAL)
- **Key Tasks:**
  - Define canonical WAL recovery interface (recoverFromWAL)
  - Create unified recovery orchestrator
  - Consolidate recovery semantics (atomicity, ordering, durability)
  - Add in-doubt transaction detection
- **Acceptance Criteria:**
  - Unified WAL recovery interface specified + tested
  - 4 recovery implementations consolidated
  - In-doubt transaction recovery verified
  - Zero data loss under coordinator failure + recovery scenarios
  - 15+ recovery path tests added

### [005-C] Cross-Shard FK Validation at Scale
- **Title:** `Distributed: Cross-Shard FK Validation Performance & Correctness`
- **Type:** Enhancement / Performance
- **Labels:** `distributed-consistency`, `fk-validation`, `performance`, `issue-5390`
- **Estimated Effort:** 30 hours
- **Scope:** Extend CrossShardForeignKeyValidator (already exists, Issue #5390 implementation)
- **Key Tasks:**
  - Add batch validation API (validate 1000s of FKs in single call)
  - Implement performance monitoring + metrics
  - Add timeout enforcement (< 50ms target for 1K checks @ 10 shards)
  - Test with synthetic TPC-H dataset (1M+ records, 10+ shards)
  - Parallel validation optimization
- **Acceptance Criteria:**
  - Batch validation API designed + tested
  - Performance: < 50ms for 1K FK checks @ 10 shards (99th percentile)
  - No FK constraint violations under concurrent writes
  - Metrics exported (validation latency, throughput, failures)
  - Load testing with realistic workloads

### [005-D] Raft Membership Safety Audit
- **Title:** `Distributed: Raft Membership Consensus Safety Audit`
- **Type:** Verification / Testing
- **Labels:** `distributed-consistency`, `raft`, `consensus`
- **Estimated Effort:** 25 hours
- **Scope:** Review + audit JOINT→COMMIT consensus path (Issue #5373)
- **Key Tasks:**
  - Document Raft membership state machine
  - Verify JOINT consensus entry persistence to WAL
  - Verify COMMIT transition entry correctness
  - Verify quorum safety cannot be violated during transitions
  - Add property-based membership tests
  - Simulate network partitions, slow replicas
- **Acceptance Criteria:**
  - Membership state machine documented
  - 15+ property-based tests added (jepsen-style)
  - Zero split-brain vulnerabilities identified
  - Network partition resilience verified
  - Performance under membership changes measured

## Metrics & KPIs

| Metric | Baseline | Target | Success |
|--------|----------|--------|---------|
| 2PC Code Duplication | ~400 lines | 0 lines | 100% elimination |
| WAL Recovery Paths | 4 separate | 1 unified | Consolidated |
| FK Validation Latency | TBD | < 50ms | Performance target |
| FK Validation Throughput | TBD | > 20K/sec @ 10 shards | Acceptable |
| Raft Membership Tests | TBD | 15+ | Coverage target |
| TPC-H Performance Regression | TBD | < 2% | Within bounds |
| Consensus Safety | TBD | 100% verified | All properties hold |

## References & Evidence

- **Source:** ROADMAP.md §Issue #5373 Inventory
- **Current Implementation:** 
  - include/sharding/cross_shard_transaction.h (coordinator)
  - include/sharding/two_phase_commit_coordinator.h (alternative)
  - include/sharding/cross_shard_fk_validator.h (Issue #5390)
  - include/replication/raft_v2.h (membership)
- **Related Issues:** #5373 (inventory), #5390 (FK validation)

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Refactoring introduces bugs | Medium | High | Extensive testing before refactor, canary deployments |
| WAL format breakage | Low | Critical | Versioned WAL format, migration path |
| Raft safety violation | Low | Critical | Formal verification, jepsen-style testing |
| Performance regression | Medium | High | Baseline benchmarking, load testing |
| Backward compatibility | Low | High | Feature flag for unified coordinator |

## Timeline & Milestones

```
W40-W43: [005-A] 2PC/3PC Protocol unification
W41-W43: [005-B] WAL recovery consolidation (parallel start)
W44-W45: [005-C] FK validation hardening + performance tuning
W45-W49: [005-D] Raft membership audit + integration testing
```

## Success Stories & Examples

### 2PC Unification Example
```cpp
// Before: Two separate implementations
CrossShardTransactionCoordinator coordinator1;
TwoPhaseCommitCoordinator coordinator2;

// After: Single unified implementation
UnifiedTwoPhaseCommitCoordinator coordinator;
coordinator.execute2PC(...);  // 2PC path
coordinator.execute3PC(...);  // 3PC path
```

### WAL Recovery Consolidation Example
```cpp
// Before: 4 separate recovery interfaces
CrossShardTransactionCoordinator::recoverFromWAL();
TwoPhaseCommitParticipant::recoverFromWAL();
LogicalReplication::loadPersistedSlots();
RaftV2::applyEntry();  // membership recovery

// After: Single unified interface
WALRecoveryOrchestrator orchestrator;
orchestrator.recoverFromWAL(recovery_point);  // All paths unified
```

---

**Last Updated:** 2026-07-05  
**Created By:** AI Deep-Dive Implementation Team  
**Related Epics:** EPIC-002 (prerequisite), EPIC-001 (depends on v1.5.0)
