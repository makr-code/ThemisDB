# Failover Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production failover runtime exists across automatic failover orchestration, disaster recovery plan execution, and queue/retry telemetry surfaces.

## In Progress

- [x] hardening dependency-degraded and multi-step recovery edge behavior (Delivered: Q3 2026)
- [x] benchmark stabilization for recovery lifecycle hot paths (Delivered: Q3 2026)
- [x] diagnostics consistency improvements for failover queue pressure and DR failures (Delivered: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under concurrent multi-node failover storms (Target: Q4 2026)
- [ ] expand regressions for fencing/quorum dependency edge scenarios (Target: Q4 2026)
- [ ] improve operator diagnostics for DR-step failure and retry escalation (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for failover and recovery orchestration overhead (Target: Q1 2027)
- [ ] add dedicated benchmark coverage for failover manager and DR step pipelines (Target: Q1 2027)
- [ ] harden long-running reliability under repeated failover/recovery cycles (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze failover/recovery manager contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for queue, dependency, and DR-step failure classes (Delivered: Q3 2026)
  - Contract header: `include/failover/failover_api_contract.h`
  - Error codes: ELECTION_TIMEOUT, SPLIT_BRAIN_DETECTED, HANDOVER_INCOMPLETE, NODE_REJOIN_FAILED,
    HEARTBEAT_MISSED, STATE_SYNC_TIMEOUT, QUORUM_UNAVAILABLE, INVALID_EPOCH, INTERNAL_ERROR

### Phase 2: Core Implementation
- [x] complete hardening for queue/worker orchestration and DR-step internals (Delivered: Q4 2026)
  - Bounded stop() with documented worst-case wait per thread (failover_thread_ ≤1 s, monitoring_thread_ ≤health_check_interval)
  - attemptRecovery stats batch-updated: single lock acquisition per call instead of per iteration
  - Canonical lock order documented: failover_mutex_ → stats_mutex_ → callbacks_mutex_
  - executePlan concurrent execution guard (execution_mutex_ + try_to_lock; returns "concurrent execution rejected")
- [x] align dependency/fencing integration behavior to bounded runtime contracts (Delivered: Q4 2026)
  - preventSplitBrain fails closed (returns false + emitDiagnostic) when no EpochFencingManager is configured
  - emitDiagnostic() helper unifies log + event-callback emission for all fencing/quorum contract violations

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for invalid plans and unsafe transition scenarios (Delivered: Q4 2026)
  - canTransition() implements real state machine table: IDLE → VERIFYING_FAILURE → CHECKING_QUORUM → STARTING_LEADER_ELECTION → (LEADER_ELECTION_IN_PROGRESS →) UPDATING_METADATA → COMPLETING_FAILOVER → IDLE; FAILED reachable from any state; back to IDLE always valid
  - transitionState() logs a warning for any transition not in the table (non-blocking, observable)
  - preventSplitBrain fails closed when fencing manager is absent (QUORUM_UNAVAILABLE diagnostic emitted)
- [x] unify diagnostics across queue saturation, retry, and DR-step failures (Delivered: Q4 2026)
  - emitDiagnostic(FailoverErrorCode, node_id, detail) private helper: spdlog::error + emitEvent mapping
  - Used in: preventSplitBrain (QUORUM_UNAVAILABLE), attemptRecovery exhaustion (NODE_REJOIN_FAILED)
  - QUORUM_UNAVAILABLE maps to FailoverEventType::QUORUM_CHECK_FAILED for callback consumers

### Phase 4: Tests
- [x] expand focused regressions for queue pressure and dependency-degraded recovery scenarios (Delivered: Q3 2026)
- [x] extend deterministic fixture coverage for DR-step permutation and timeout cases (Delivered: Q3 2026)
- [x] add DR plan validation and step-isolation focused regressions (Delivered: Q3 2026)
  - Test file: `tests/failover/test_failover_contract_hardening_focused.cpp`
  - Test cases: FCH-01..FCH-16 (election, handover, recovery, error contract)
  - kFailoverContractSeed = 42; all tests self-contained, no external I/O
  - Test file: `tests/failover/test_failover_chaos_scenarios.cpp`
  - Test cases: 17 scenarios (queue saturation, pressure telemetry, concurrent access, lifecycle)
  - Test file: `tests/failover/test_failover_dr_edge_scenarios.cpp`
  - Test cases: DRE-01..DRE-08 (plan validation, dry-run, step hook failure, statistics)
  - kDREdgeSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for failover/recovery hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/failover/bench_failover_release_gates.cpp`
  - Gates: FRG-01..FRG-06 (heartbeat ≤500µs, election ≤5ms, state-sync ≤200µs,
    health-check ≤100µs, buffer-check ≤50µs, epoch-persist ≤1ms)
  - kFailoverCanonicalSeed = 42; Repetitions(5)

### Phase 6: Documentation and Acceptance
- [x] core failover module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (FCH-01..16), 6 release-gate benchmarks (FRG-01..06)
- [x] Q3 2026 status sync: roadmap validated against full module docs (2026-07-29)
  - Test evidence: chaos scenarios (17 tests PASS), contract hardening (FCH-01..16), DR edge (DRE-01..08)
  - Benchmark evidence: FRG-01..FRG-06 (bench_failover_release_gates.cpp)
- [x] Phase 2/3 hardening delivered (2026-07-29)
  - Test file: `tests/failover/test_failover_phase2_phase3_focused.cpp`
  - Test cases: P23-01..P23-08 (canTransition table, fail-closed split-brain, batch stats, concurrent DR rejection, emitDiagnostic callback)
  - kPhase23Seed = 42; all tests self-contained, no external I/O

## Production Readiness Checklist

- [x] core failover surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] dedicated failover benchmark file delivered (FRG-01..FRG-06)
- [x] Q3 2026 hardening, benchmark stabilization, and diagnostics items closed
- [x] remaining hardening tasks closed for dependency/queue/DR-step edge paths (Q4 2026)
- [ ] release benchmark stabilization p95/p99 re-baseline complete (Q1 2027)

## Known Issues and Limitations

- runtime outcomes partially depend on external manager availability and behavior.
- concurrent multi-node failover storm edge cases require continued hardening (Q4 2026).
- fencing/quorum dependency edge scenarios are covered by contract tests but not yet stress-tested at scale.

## Breaking Changes

No breaking failover contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.