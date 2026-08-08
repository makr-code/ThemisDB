> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Transaction Module Roadmap

## Current Status
Production-grade transaction stack with ACID lifecycle management, MVCC integration, savepoints, distributed coordination paths, and audit/batching utilities in active use.

## In Progress
- [~] Transaction hardening wave for distributed safety, timeout semantics, and recovery guarantees (Target: Q3 2026)
  - [x] `ITransactionCoordinator` unified interface for all commit protocols (2PC, 3PC, SAGA, Percolator, Calvin) — Issue #5374
  - [~] Complete remaining cross-shard failure-injection coverage for coordinator and participant transitions (Target: Q3 2026)
    - [x] Phase 1 test suites complete: Lifecycle, Isolation Contention, Error Path Determinism (33 tests)
  - [~] Tighten timeout and rollback determinism under sustained contention and mixed workloads (Target: Q3 2026)
    - [x] Phase 1: Timeout determinism tests with contention loads (10 tests)
    - [ ] Phase 2: Distributed coordinator timeout/retry hardening (pending Q4)

## Planned Features

### Short-term (3-6 months)
- [ ] Harden coordinator crash-recovery and in-doubt transaction reconciliation policies (Target: Q4 2026)
- [ ] Expand transaction diagnostics and explainability for lock/queue/latency bottlenecks (Target: Q4 2026)
- [ ] Strengthen SAGA orchestration safeguards for partial remote failures and retries (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Advance distributed transaction throughput hardening without weakening safety invariants (Target: Q1 2027)
- [ ] Extend OCC and serializable conflict telemetry to improve operator tuning loops (Target: Q1 2027)
- [ ] Expand audit/export integration hardening for large retention windows (Target: Q1 2027)

## Implementation Phases

### Phase 1: Lifecycle and Isolation Safety ✓ IMPLEMENTATION COMPLETE
**Status:** Tests Implemented (Build & Execution Verification Pending)
**Target:** Q3 2026
**Evidence:** src/transaction/PHASE_1_ACCEPTANCE_CHECKLIST.md

Completed Deliverables:
- [x] `test_transaction_lifecycle_phase1.cpp` — 12 focused tests validating AC-1, AC-2, AC-3
- [x] `test_transaction_isolation_contention_phase1.cpp` — 10 focused tests validating AC-3, AC-7
- [x] `test_transaction_error_path_determinism_phase1.cpp` — 11 focused tests validating AC-2, AC-7
- [x] CMakeLists.txt test registration with proper macro pattern
- [x] Acceptance criteria documentation and verification checklist

Acceptance Criteria Coverage:
- [x] AC-1: ACID Lifecycle Isolation Enforcement (concurrent transactions)
- [x] AC-2: Begin/Prepare/Commit/Abort State Machine Correctness (guard and recovery)
- [x] AC-3: Isolation Level Behavior (READ_COMMITTED, SNAPSHOT, SERIALIZABLE)
- [x] AC-7: Timeout Semantics and Deterministic Rollback (consistency under contention)

Next: Build verification and test execution (scheduled immediately)

### Phase 2: Distributed Coordination Hardening
- [ ] Strengthen distributed coordinator timeout, retry, and participant liveness semantics (Target: Q4 2026)
- [ ] Add deterministic regression packs for in-doubt and recovery flows (Target: Q4 2026)
- [ ] Improve observability for distributed transition timelines (Target: Q4 2026)

### Phase 3: SAGA and Compensation Reliability
- [ ] Expand compensation idempotency and ordering verification under fault injection (Target: Q4 2026)
- [ ] Validate remote-step orchestration behavior under partial network degradation (Target: Q4 2026)
- [ ] Harden manual intervention and recovery paths (Target: Q4 2026)

### Phase 4: Performance and Operational Hardening
- [ ] Re-baseline transaction throughput and tail-latency envelopes across representative profiles (Target: Q1 2027)
- [ ] Ensure audit and batching overhead remains bounded under peak ingest and concurrency (Target: Q1 2027)
- [ ] Extend benchmark evidence for production guardrail tuning (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [ ] Keep transaction docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)
- [ ] Finalize coordinator crash-recovery documentation with examples (Target: Q4 2026)
- [ ] Add diagnostics and observability guide for lock/queue/latency bottlenecks (Target: Q4 2026)

## Production Readiness Checklist
- Status: In Progress (Phase 1 implementation complete; verification pending)
- Core Acceptance Criteria: AC-1 through AC-7 (Phase 1: AC-1,2,3,7 tested)
- Nachweise: 
  - [x] 33 Phase 1 focused tests (lifecycle, isolation, contention, error paths)
  - [ ] Phase 1 build and execution verification (pending)
  - [ ] Phase 2-4 test suites (Q4 2026 - Q1 2027)
  - [x] Distributed transaction coordinator interface finalized
  - [ ] In-doubt reconciliation regression pack (Phase 2)
  - [ ] SAGA compensation idempotency suite (Phase 3)
  - [ ] Performance baseline and operational limits (Phase 4)
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Phase 1 test suites implemented; build and execution verification pending
- Some distributed failure envelopes still need broader regression evidence for long-running degraded conditions.
- Coordinator and remote orchestration paths require additional benchmark-backed operational limits.
- Advanced throughput optimizations remain gated behind safety-first verification criteria.

## Breaking Changes
- Transaction public APIs in active major lines remain additive-first.
- Any future behavioral changes requiring migration must be versioned and documented in changelog/migration notes.
