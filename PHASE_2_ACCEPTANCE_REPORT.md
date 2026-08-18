# Phase 2 Routing/Coordination Internals Hardening — Acceptance Report

**Status**: 🔄 In Progress  
**Date**: 2026-08-17  
**Target Scope**: Production-readiness hardening for routing, coordination, quorum, and load detection internals.

## Executive Summary

This report documents the Phase 2 hardening effort for ThemisDB's sharding module routing and coordination subsystems. The objective is to ensure production-ready behavior under concurrent load, with comprehensive thread-safety, timeout handling, exception safety, and deterministic testability guarantees.

### Scope

Four key implementation files:
- `src/sharding/shard_router.cpp` — Routing logic and fallback behavior
- `src/sharding/distributed_coordinator.cpp` — Coordination state machine and election
- `src/sharding/quorum_manager.cpp` — Quorum handling and consensus fallbacks
- `src/sharding/shard_load_detector.cpp` — Load detection and imbalance prediction

### Acceptance Criteria ✓ Targeted

1. **Thread-Safety**: All shared state in routing/coordination protected by canonical lock ordering
2. **Timeout Handlers**: All blocking operations must have bounded timeouts
3. **Exception Safety**: Critical paths guarantee strong/no-throw exception safety
4. **Error Logging**: Comprehensive diagnostics for all failure modes
5. **Determinism**: Reproducible behavior under load with seed-42 testability
6. **Lock Ordering**: Validated against canonical order: `state_mutex_ < audit_mutex_ < metrics_mutex_`

---

## Gap Analysis

### File 1: `src/sharding/distributed_coordinator.cpp`

**Current Maturity Score**: 85/100  
**Gap Summary**: TODO=1, Stub=1, Mock=1

#### Identified Gaps

| Gap ID | Category | Severity | Description | Impact |
|--------|----------|----------|-------------|--------|
| DC-01 | Thread-Safety | HIGH | `current_term_` incremented without lock in `startElection()` (line 189) | Data race on term counter |
| DC-02 | Deadlock Risk | HIGH | `electionLoop()` may call `startElection()` while holding implicit state locks | Potential deadlock under contention |
| DC-03 | Lock Ordering | MEDIUM | No canonical lock ordering documented in header; multiple mutexes without order guarantee | Deadlock under concurrent contention |
| DC-04 | Timeout | MEDIUM | Election/heartbeat loops use blocking `std::this_thread::sleep_for()` without abort mechanism | Non-interruptible shutdown |
| DC-05 | Exception Safety | MEDIUM | `taskExecutorLoop()` catches but doesn't log full exception context | Loss of diagnostic information |
| DC-06 | Callback Safety | LOW | `leader_elected_callback_` accessed without holding `callback_mutex_` in some paths | Potential data race on callback field |

**Mitigations Planned**: 
- Protect `current_term_` with `leader_mutex_` atomic transition
- Document canonical lock order: `leader_mutex_ (1) < tasks_mutex_ (2) < callback_mutex_ (3)`
- Add `std::condition_variable` for event-driven shutdown signaling
- Enhance exception logging in `taskExecutorLoop()`
- Validate callback field access under `callback_mutex_`

---

### File 2: `src/sharding/shard_router.cpp`

**Current Maturity Score**: 85/100  
**Gap Summary**: TODO=1, Stub=1, Mock=1

#### Identified Gaps

| Gap ID | Category | Severity | Description | Impact |
|--------|----------|----------|-------------|--------|
| SR-01 | Timeout | HIGH | `executeQuery()` has no per-operation timeout; scatter-gather may block indefinitely | Resource exhaustion under hanging shards |
| SR-02 | Error Logging | MEDIUM | Generic error messages lack shard_id/request context diagnostics | Difficult incident triage |
| SR-03 | Thread-Safety | MEDIUM | Atomic counter increments in `routeRequest()` are not ordered with respect to error updates | Potential counter inconsistencies |
| SR-04 | Exception Safety | MEDIUM | Result merging in `mergeResults()` may throw without cleanup guarantee | Partial state corruption in error paths |
| SR-05 | Determinism | LOW | Version token calculation uses `std::chrono::steady_clock` without seed control | Non-reproducible merge ordering |

**Mitigations Planned**:
- Add `config_.scatter_timeout_ms` enforcement with `std::future::wait_for()` 
- Enhance error logging with structured fields (shard_id, operation, latency)
- Use `std::scoped_lock` for multi-atomic updates with guaranteed ordering
- Add RAII wrapper for `mergeResults()` to guarantee cleanup
- Add seed-aware version token calculation for deterministic testing

---

### File 3: `src/sharding/quorum_manager.cpp`

**Current Maturity Score**: 85/100  
**Gap Summary**: TODO=1, Stub=1, Mock=1

#### Identified Gaps

| Gap ID | Category | Severity | Description | Impact |
|--------|----------|----------|-------------|--------|
| QM-01 | Timeout | MEDIUM | `waitForOperations()` uses `future::wait_for()` but may not enforce timeout on all futures | Some operations may hang past deadline |
| QM-02 | Thread-Safety | LOW | Statistics updates via atomic operations are not formally memory-ordered | Potential memory ordering issues under extreme contention |
| QM-03 | Error Logging | MEDIUM | Failed operations don't log which nodes timed out vs. failed | Difficult to diagnose quorum loss scenarios |
| QM-04 | Exception Safety | LOW | Configuration update in `updateConfig()` is not atomic; readers may see partial updates | Inconsistent quorum policy during updates |

**Mitigations Planned**:
- Verify `waitForOperations()` enforces strict deadline per future
- Upgrade to `std::memory_order_release`/`acquire` for statistics updates
- Add per-node timeout diagnostics in failure paths
- Wrap config update in atomic swap with old config snapshot

---

### File 4: `src/sharding/shard_load_detector.cpp`

**Current Maturity Score**: 85/100  
**Gap Summary**: TODO=1, Stub=1, Mock=1

#### Identified Gaps

| Gap ID | Category | Severity | Description | Impact |
|--------|----------|----------|-------------|--------|
| SLD-01 | Thread-Safety | HIGH | `detectImbalance()` is const but doesn't hold `mutex_` for entire operation; TOCTOU race on shard_loads_ | Stale/inconsistent imbalance results |
| SLD-02 | Lock Scope | MEDIUM | `recordRebalanceTriggered()` writes `last_rebalance_time_` without mutex protection | Data race on cooldown timestamp |
| SLD-03 | Timeout | LOW | `detectImbalance()` has no timeout; malformed metrics may cause unbounded computation | Potential CPU exhaustion during analysis |
| SLD-04 | Error Logging | MEDIUM | Imbalance detection results lack timestamp/shard metadata for incident correlation | Difficult to correlate detections across components |

**Mitigations Planned**:
- Hold `mutex_` for entire duration of `detectImbalance()`; return snapshot copy
- Protect `recordRebalanceTriggered()` with mutex for atomic timestamp update
- Add bounded execution time limit to `detectImbalance()`
- Enhance result structures with detection timestamp and full metrics snapshot

---

## Detailed Findings

### Lock-Ordering Strategy

**Canonical Order for DistributedCoordinator**:
```
Tier 1: leader_mutex_      (manages leader role, term, lease state)
  ↓ cannot acquire while holding Tier 2 or 3
Tier 2: tasks_mutex_       (manages pending task queue)
  ↓ cannot acquire while holding Tier 3
Tier 3: callback_mutex_    (manages registered callbacks)
  ↓ cannot acquire any higher-tier lock
```

**Canonical Order for ShardLoadDetector**:
```
Single tier: mutex_        (protects all load state and history)
```

**Canonical Order for QuorumManager**:
```
Single tier: config_mutex_ (protects configuration)
```

### Timeout Requirements

| Component | Operation | Max Timeout | Rationale |
|-----------|-----------|-------------|-----------|
| ShardRouter | Scatter-gather query | `config_.scatter_timeout_ms` (default 30s) | Bound RPC latency distribution tail |
| DistributedCoordinator | Election round | `config_.election_timeout_ms` (default 10s) | Bound leader-detection latency |
| DistributedCoordinator | Heartbeat emission | `config_.heartbeat_interval_ms` (default 5s) | Bound lease renewal interaction |
| QuorumManager | Quorum wait | `config_.operation_timeout` (default 5s) | Bound quorum consensus time |
| ShardLoadDetector | Imbalance detection | 500ms (new) | Bound analysis under heavy load |

### Exception-Safety Requirements

| Component | Critical Path | Target Level | Implementation |
|-----------|----------------|--------------|-----------------|
| ShardRouter | mergeResults() | Strong | RAII guards for JSON construction; atomic swaps |
| DistributedCoordinator | becomeLeader() | Strong | Separate callback invocation from state mutation |
| QuorumManager | executeWrite/Read | Strong | Future cleanup with scope guards |
| ShardLoadDetector | detectImbalance() | No-throw | Copy-on-read; bounded allocation |

---

## Implementation Plan

### Phase 2A: Lock Ordering and Thread-Safety Hardening
**Target**: Aug 18–20, 2026

**Tasks**:
- [ ] DC-01: Protect `current_term_` with atomic CAS loop in `startElection()`
- [ ] DC-03: Document and validate lock-ordering hierarchy in headers
- [ ] SLD-01: Refactor `detectImbalance()` to hold mutex for entire operation
- [ ] SLD-02: Add mutex protection to `recordRebalanceTriggered()`

**Verification**: Unit tests for concurrent access patterns; no TSAN warnings.

---

### Phase 2B: Timeout and Interruptibility
**Target**: Aug 21–22, 2026

**Tasks**:
- [ ] DC-04: Replace blocking sleeps with `std::condition_variable`-based wait
- [ ] SR-01: Add scatter-gather timeout enforcement with `wait_for()`
- [ ] QM-01: Validate `waitForOperations()` timeout precision
- [ ] SLD-03: Add 500ms analysis deadline to `detectImbalance()`

**Verification**: Timeout tests with mock hanging operations; graceful shutdown under load.

---

### Phase 2C: Exception Safety and Error Logging
**Target**: Aug 23–24, 2026

**Tasks**:
- [ ] DC-05: Enhance exception context in `taskExecutorLoop()`
- [ ] SR-02/SR-04: Add structured error logging; RAII wrappers for merges
- [ ] QM-03: Per-node timeout diagnostics in failure paths
- [ ] SLD-04: Add timestamp/metadata to imbalance result structures

**Verification**: Exception-safety tests; log audit for diagnostic completeness.

---

### Phase 2D: Determinism and Testing
**Target**: Aug 25–26, 2026

**Tasks**:
- [ ] SR-05: Seed-aware version token calculation for deterministic testing
- [ ] Create comprehensive test suite in `test_sharding_phase2_hardening.cpp`
- [ ] Validate all acceptance criteria with deterministic fixtures (seed=42)
- [ ] Code review checklist sign-off

**Verification**: All tests pass with seed=42; reproducible results across runs.

---

## Test Coverage Plan

**Phase 2 test suite** will be located in:
```
tests/sharding/test_sharding_phase2_hardening.cpp
```

### Test Categories

#### 1. Thread-Safety Tests (TS-XX)
- TS-01: Concurrent `startElection()` calls with term counter validation
- TS-02: Concurrent `updateShardLoad()` in load detector
- TS-03: Concurrent `routeRequest()` with atomic counter consistency
- TS-04: Callback registration race under concurrent leader election

#### 2. Lock-Ordering Tests (LO-XX)
- LO-01: DistributedCoordinator lock hierarchy (no deadlock under contention)
- LO-02: ShardLoadDetector single-lock invariant
- LO-03: QuorumManager config lock isolation

#### 3. Timeout Tests (TO-XX)
- TO-01: ShardRouter scatter-gather timeout with hanging shard
- TO-02: DistributedCoordinator election timeout with no quorum
- TO-03: QuorumManager operation timeout under slow nodes
- TO-04: ShardLoadDetector analysis timeout under large metric sets

#### 4. Exception-Safety Tests (ES-XX)
- ES-01: ShardRouter mergeResults() exception recovery
- ES-02: DistributedCoordinator task execution exception handling
- ES-03: QuorumManager future cleanup on exception
- ES-04: ShardLoadDetector memory isolation under exception

#### 5. Error Logging Tests (EL-XX)
- EL-01: ShardRouter comprehensive error context logging
- EL-02: QuorumManager per-node timeout diagnostics
- EL-03: ShardLoadDetector structured imbalance result output

#### 6. Determinism Tests (DT-XX)
- DT-01: Version token generation with seed=42 reproducibility
- DT-02: Load imbalance detection with fixed metric snapshots
- DT-03: Quorum operation determinism under seeded node selection

---

## Sign-Off Criteria

### Code Review Checklist

**Must Pass Before Merge:**

- [ ] **Lock Ordering**: All mutex acquisitions follow documented canonical order
- [ ] **Timeout Bounds**: Every blocking operation has finite timeout with log on breach
- [ ] **Exception Safety**: All critical paths tested for exception safety level
- [ ] **Error Logging**: Every error path includes shard_id, operation, and diagnostics
- [ ] **Thread-Safety**: No TSAN warnings on focused concurrent test suite
- [ ] **Determinism**: Seed-42 test suite passes 10+ consecutive runs identically
- [ ] **Documentation**: All changes documented in headers and CHANGELOG.md

### Test Results Required

- [ ] `TS-01` through `TS-04` all pass
- [ ] `LO-01` through `LO-03` all pass
- [ ] `TO-01` through `TO-04` all pass (timeout enforced ±10%)
- [ ] `ES-01` through `ES-04` all pass (zero leaks, proper RAII)
- [ ] `EL-01` through `EL-03` all pass (audit log review)
- [ ] `DT-01` through `DT-03` all pass with seed=42

### Acceptance Timeline

| Phase | Target Date | Deliverables |
|-------|-------------|--------------|
| 2A | Aug 20 | Lock-ordering hardening complete; TSAN-clean |
| 2B | Aug 22 | Timeout enforcement complete; graceful shutdown verified |
| 2C | Aug 24 | Exception safety and diagnostics complete |
| 2D | Aug 26 | Full test suite passing; code review sign-off ready |

---

## Risk Assessment

### High-Risk Areas

1. **DistributedCoordinator Lock Interactions** (DC-02, DC-03)
   - **Risk**: Deadlock under concurrent election + task execution
   - **Mitigation**: Formal lock-ordering validation; stress tests with 100+ concurrent operations
   - **Backout Plan**: Revert to single-threaded election loop if deadlock observed

2. **ShardRouter Timeout Enforcement** (SR-01)
   - **Risk**: Scatter-gather may drop in-flight RPCs mid-operation; resource leaks
   - **Mitigation**: Explicit future cleanup with `wait_for()` followed by explicit cancellation
   - **Backout Plan**: Fall back to no-timeout behavior (degraded but safe)

3. **ShardLoadDetector Consistency** (SLD-01)
   - **Risk**: Imbalance results race with load updates; recommending wrong shards
   - **Mitigation**: Snapshot-copy semantics; validation tests with concurrent updates
   - **Backout Plan**: Return unverified results with confidence=0 flag

---

## Next Steps

1. **Immediate** (Aug 17–18): Review this report; identify additional concerns from team
2. **Short-term** (Aug 18–26): Implement fixes in phases 2A–2D per schedule
3. **Validation** (Aug 26–27): Full test suite execution; performance regression check
4. **Merge** (Aug 27–28): Code review sign-off; PR merge to `develop`
5. **Release** (Aug 28–31): Wave A rollout readiness validation; release notes

---

## References

- Roadmap: `/home/runner/work/ThemisDB/ThemisDB/src/sharding/ROADMAP.md` §119–121
- Thread-Safety Baseline: `tests/sharding/test_sharding_thread_safety_lock_order_focused.cpp`
- Contract Hardening: `tests/sharding/test_sharding_contract_hardening_focused.cpp`
- Lock-Ordering Guide: `include/sharding/sharding_api_contract.h` §6 Threading

---

**Report Status**: 🟢 COMPLETE  
**Next Review Date**: 2026-08-20  
**Approval Required From**: Architecture Lead, QA Lead, DevOps Lead
