# Evaluation Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

EPIC 2 remains a mixed-stage module overall, but EPIC 2.5 hybrid query planning is now
implemented, benchmarked, and regression-tested. The active planner contract lives in
`include/query_planner.h`, the production implementation lives in `src/query_planner.cc`,
focused coverage lives in `tests/epic2_evaluation/query_planner_test.cc`, and hardening
benchmarks live in `benchmarks/epic2_evaluation/planner_decision_bench.cc`.

## In Progress

- [~] EPIC 2 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] downstream integration follow-ups for planner decision envelopes and routing diagnostics (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] integrate planner decisions into TensorRAG decision envelopes and explain output (Target: Q4 2026)
- [ ] surface ANN frontdoor routing diagnostics in default evaluation flows (Target: Q4 2026)
- [x] establish phase-5 benchmark/hardening baselines (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] attach graph-truth validation metadata to end-to-end planner decisions (Target: Q1 2027)
- [ ] complete default workflow integration after downstream gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] hybrid planner execution model documented for ANN-only, tensor-advisory, exact-graph, and distributed summary-first flows
- [x] freshness, staleness, advisory-only tensor semantics, and graph-truth finalization captured in `docs/EPIC2_QUERY_PLANNER.md`

### Phase 2: Core Implementation
- [x] `include/query_planner.h` defines typed planner inputs, outputs, policy config, fallback reasons, and observability hooks
- [x] `src/query_planner.cc` implements the five-path decision model and explicit fallback routing

### Phase 3: Error Handling and Edge Cases
- [x] stale, rebuilding, low-residual, high-delta-lag, and rank-cap violations force exact fallback with reason codes
- [x] distributed multi-shard routing falls back to exact graph when shard manifests are unavailable

### Phase 4: Tests
- [x] focused planner regression coverage implemented in `tests/epic2_evaluation/query_planner_test.cc`
- [x] path, fallback, override, distributed, and observer cases are covered by standalone-executable GTests

### Phase 5: Performance and Hardening
- [x] benchmark suite implemented in `benchmarks/epic2_evaluation/`
- [x] planner decision latency and fallback monitoring exposed through `PlannerObserver`

### Phase 6: Documentation and Acceptance
- [x] planner design, routing criteria, fallback rules, and acceptance criteria documented in `docs/EPIC2_QUERY_PLANNER.md`
- [x] module docs updated to reflect live planner code, tests, and benchmarks

### Phase 7: Production Integration
- [x] planner library and focused test target wired into local CMake surfaces
- [ ] downstream retrieval-pipeline decision-envelope integration remains pending

## Production Readiness Checklist

- [x] planner contract and scope boundaries are documented
- [x] stale / invalid tensor artifacts force explicit exact fallback
- [x] graph truth remains the final correctness gate in planner design
- [x] summary-first and exact-on-demand routing are implemented and testable
- [ ] default product-pipeline integration is still pending

## Known Issues & Limitations

- Learned or cost-model-based planning remains out of scope; current selection is heuristic and policy-driven.
- Full decision-envelope integration with downstream TensorRAG / ANN frontdoor flows is deferred to follow-up work.
- Planner benchmarks currently measure decision overhead and routing regressions, not full end-to-end retrieval latency.

## Breaking Changes

- none
