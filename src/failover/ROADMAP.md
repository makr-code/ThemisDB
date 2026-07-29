# Failover Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production failover runtime exists across automatic failover orchestration, disaster recovery plan execution, and queue/retry telemetry surfaces.

## In Progress

- [~] hardening dependency-degraded and multi-step recovery edge behavior (Target: Q3 2026)
- [~] benchmark stabilization for recovery lifecycle hot paths (Target: Q3 2026)
- [~] diagnostics consistency improvements for failover queue pressure and DR failures (Target: Q3 2026)

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
- [ ] complete hardening for queue/worker orchestration and DR-step internals (Target: Q4 2026)
- [ ] align dependency/fencing integration behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for invalid plans and unsafe transition scenarios (Target: Q4 2026)
- [ ] unify diagnostics across queue saturation, retry, and DR-step failures (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for queue pressure and dependency-degraded recovery scenarios (Delivered: Q3 2026)
- [x] extend deterministic fixture coverage for DR-step permutation and timeout cases (Delivered: Q3 2026)
  - Test file: `tests/failover/test_failover_contract_hardening_focused.cpp`
  - Test cases: FCH-01..FCH-16 (election, handover, recovery, error contract)
  - kFailoverContractSeed = 42; all tests self-contained, no external I/O

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

## Production Readiness Checklist

- [x] core failover surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for dependency/queue/DR-step edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime outcomes partially depend on external manager availability and behavior.
- selected high-pressure queue and dependency edge scenarios require continued hardening.
- dedicated failover-native benchmark coverage is currently limited.

## Breaking Changes

No breaking failover contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.