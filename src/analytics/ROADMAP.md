# Analytics Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production analytics runtime exists across OLAP, streaming/CEP, forecasting, anomaly detection, model-serving integration, and distributed analytics coordination.

## In Progress

- [~] hardening of streaming and distributed runtime limits under sustained load (Target: Q3 2026)
  - [x] `max_open_windows` / `max_records_per_window` runtime limits added to TumblingWindow, SlidingWindow, HoppingWindow
  - [x] `max_open_sessions` / `max_records_per_session` runtime limits added to SessionWindow
  - [x] `windows_evicted` counter added to `WindowStats` for all four window types
  - [ ] distributed analytics coordinator safety controls (Target: Q3 2026)
- [~] benchmark and release-gate consolidation for analytics-critical paths (Target: Q3 2026)
  - [x] `benchmarks/analytics/bench_streaming_window.cpp` added (7 benchmarks covering throughput, eviction, flush latency)
  - [ ] remaining proxy-mapped benchmark paths need direct coverage (Target: Q4 2026)
- [~] consistency hardening for optional dependency and fallback behavior (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] strengthen bounded-memory behavior in high-cardinality streaming windows (Target: Q4 2026)
- [ ] extend integration regression coverage for serving and export failure classes (Target: Q4 2026)
- [ ] improve distributed merge diagnostics and operator-facing telemetry (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] add/expand dedicated benchmarks for currently proxy-covered analytics paths (Target: Q1 2027)
- [ ] re-baseline analytics latency and throughput envelopes per representative hardware profile (Target: Q1 2027)
- [ ] harden cross-cluster security and reliability controls in federated analytics scenarios (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze analytics runtime contracts for critical execution paths (Completed 2026-07-29)
- [x] define explicit failure classes for unsupported dependency/capability states (Completed 2026-07-29)

### Phase 2: Core Implementation
- [x] streaming window runtime limits (max_open_windows, max_records_per_window/session, eviction) implemented
- [ ] complete remaining runtime hardening in distributed high-load scenarios (Target: Q4 2026)
- [ ] align serving/export integration behavior to shared bounded execution policy (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior across optional-backend and degraded states (Target: Q4 2026)
- [ ] enforce consistent diagnostics for parse/input/state validation failures (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for high-load streaming, distributed merge, and integration failure paths (Completed 2026-07-29 — test_analytics_contract_hardening_focused.cpp, ANC-01..ANC-16)
- [x] extend deterministic fixture coverage for optional dependency off/on matrixes (Completed 2026-07-29)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for analytics-critical paths (Completed 2026-07-29 — bench_analytics_release_gates.cpp, ARG-01..ARG-06)
- [ ] validate p95/p99 behavior under representative production load profiles (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core analytics module docs aligned to source-verifiable behavior
- [x] roadmap remains forward-looking while changelog captures historical completion
- [x] analytics_api_contract.h frozen contract header published (Completed 2026-07-29)

## Production Readiness Checklist

- [x] core runtime surfaces documented with source verification
- [x] security and failure behavior documented at module level
- [x] mapped benchmark expectations documented
- [x] streaming window runtime limits (max_open_windows, max_records, eviction) implemented
- [x] dedicated streaming window benchmark added (bench_streaming_window.cpp)
- [x] analytics_api_contract.h frozen contract header (Phase 1 closure, 2026-07-29)
- [x] test_analytics_contract_hardening_focused.cpp — ANC-01..ANC-16 (Phase 4 closure, 2026-07-29)
- [x] bench_analytics_release_gates.cpp — ARG-01..ARG-06 gate benchmarks (Phase 5 closure, 2026-07-29)
- [ ] dedicated benchmark coverage complete for all critical paths
- [ ] remaining hardening tasks closed for sustained-load reliability

## Known Issues and Limitations

- benchmark coverage is still mixed between direct and proxy mappings for some targets.
- behavior and availability remain partially capability-dependent on optional integrations.
- continued hardening is required for cross-cluster/federated scenarios.

## Breaking Changes

No breaking module contract planned. Any contract-breaking change requires explicit migration notes and changelog entry before merge.