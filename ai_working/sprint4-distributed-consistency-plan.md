# Sprint 4: Distributed Consistency & Error Handling Quick-Wins

## Implementation Strategy

### Phase 1: Cross-Shard Transaction Consistency (QW-5a)
**File**: src/sharding/cross_shard_transaction.cpp

**Patterns identified**:
- Fail-closed validation (lines 635-656)
- Timeout-based lock acquisition (line 659-663)
- State machine validation (lines 672-675)
- Duplicate detection (line 678-681)

**Changes needed**:
1. Add state consistency guards in prepare() method after txn state check
2. Ensure participant set consistency matches prepared set invariant
3. Add Doxygen documentation for cross-shard consistency invariants
4. Validate no participants added after PREPARING state

**Acceptance criteria**:
- prepare() rejects participant additions in PREPARING/PREPARED states
- prepare() documents consistency invariants
- No silent state transitions allowed
- All validations use fail-closed pattern

### Phase 2: WAL Recovery Path Hardening (QW-5b)
**File**: src/sharding/transaction_wal.cpp

**Patterns identified**:
- Static_assert guards (lines 31-46)
- Directory creation with error handling (lines 62-70)
- WAL manager initialization (line 79)

**Changes needed**:
1. Add WAL entry sequence validation in readEntries()
2. Handle incomplete PREPARE/PREPARED cycles
3. Add validation for corrupted WAL segments
4. Document recovery state machine

**Acceptance criteria**:
- Recovery validates sequence integrity
- Incomplete cycles handled gracefully
- Corrupted segments detected and logged
- Recovery state machine documented

### Phase 3: Replication Manager Error Propagation (QW-5c)
**File**: src/replication/replication_manager.cpp

**Patterns identified**:
- Comprehensive replication model (lines 68-101)
- Error handling structure

**Changes needed**:
1. Add error collection from replica write failures
2. Propagate replica failures to coordinating transaction
3. Add retry logic with exponential backoff
4. Document error propagation semantics

**Acceptance criteria**:
- Replica failures collected and propagated
- Backoff strategy implemented
- Error semantics documented
- Existing tests pass

### Phase 4: Orphan Transaction Detector Robustness (QW-6a)
**File**: src/sharding/orphan_detector.cpp

**Patterns identified**:
- State-based orphan detection (lines 36-45)
- Threshold-based age check (lines 87-94)
- Timeout configuration (lines 75-76)

**Changes needed**:
1. Add state-specific timeout configurations
2. Add detection for transactions blocked on specific state
3. Add detailed logging for orphan lifecycle
4. Handle cleanup of detected orphans with safe rollback

**Acceptance criteria**:
- State-specific timeouts configurable
- Blocked state detection working
- Orphan lifecycle logged
- Safe rollback implemented

### Phase 5: Raft Membership Transition Safety (QW-6b)
**File**: src/replication/raft_v2.cpp

**Patterns identified**:
- Joint consensus state management (lines 36-60)
- Exception-based error handling (lines 39-40, 49-50)

**Changes needed**:
1. Add WAL durability gate before JOINT→COMMIT transition
2. Validate quorum requirement during membership change
3. Add rollback if persistence fails
4. Document membership change safety contracts

**Acceptance criteria**:
- JOINT entries persisted before transitions
- Quorum validation working
- Rollback on persistence failure
- Safety contracts documented

### Phase 6: Distributed Consensus Timeout Handling (QW-6c)
**File**: src/sharding/two_phase_commit_coordinator.cpp

**Patterns identified**:
- State machine coordination (lines 53-79)
- Configuration-based timeouts
- Participant management (lines 86-97)

**Changes needed**:
1. Add adaptive timeout calculation based on participant response times
2. Implement stale lock detection and cleanup
3. Add in-doubt transaction monitoring
4. Document timeout recovery semantics

**Acceptance criteria**:
- Adaptive timeouts functional
- Stale lock detection working
- In-doubt monitoring implemented
- Recovery semantics documented

## Commit Strategy

**Commit 1**: QW-5a + QW-5b + QW-5c
- Focus: Cross-shard consistency, WAL recovery, replication error handling
- Files: cross_shard_transaction.cpp, transaction_wal.cpp, replication_manager.cpp
- Expected insertions: ~120-130 lines

**Commit 2**: QW-6a + QW-6b + QW-6c
- Focus: Orphan detection, Raft membership, timeout handling
- Files: orphan_detector.cpp, raft_v2.cpp, two_phase_commit_coordinator.cpp
- Expected insertions: ~100-120 lines

## Quality Gates

1. All changes use fail-closed error handling
2. RAII patterns and modern C++ constructs only
3. Doxygen documentation for all public API changes
4. No breaking changes
5. Existing tests must pass
6. No stubs or TODOs as final output
