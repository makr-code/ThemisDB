# include chaos roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Header contract complete for current runtime implementation (Target: Q2 2026)

## In Progress
- [ ] Add distributed coordination API extensions if required by source roadmap (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] Provide stable fault and scheduler contracts (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Source implementation available in `src/chaos` (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Parameter constraints represented in API documentation (Target: Q2 2026)
### Phase 4: Tests
- [x] Add dedicated API conformance tests at include boundary (`tests/test_chaos_framework.cpp`, `tests/test_chaos_scheduler.cpp`) (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [ ] Review ABI compatibility constraints for external consumers (Target: Q3 2026)
- [x] Extended stress-benchmark coverage (`bench_chaos_stress.cpp`): FaultInjector inject/query/recover throughput, concurrent 1..16 threads, ChaosScheduler overhead (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline include module docs created (Target: Q2 2026)

## Production Readiness Checklist
- [x] Header matches compiled source implementation
- [ ] ABI compatibility test matrix complete

## Known Issues & Limitations
- API models in-process behavior; distributed execution must be layered externally.

## Breaking Changes
- None currently planned.
