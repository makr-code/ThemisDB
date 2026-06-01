# Evaluation Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

EPIC 2 evaluation contract ownership is documented for sub-issues 2.1–2.7.
Header and source files are deferred to a dedicated implementation PR.
Runtime policy behavior, full tests, and benchmark evidence remain pending.

## In Progress

- [~] EPIC 2 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for planner and benchmark readiness (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] implement phase-3 runtime error/policy behavior (Target: Q4 2026)
- [ ] deliver phase-4 verification and regression suites (Target: Q4 2026)
- [ ] establish phase-5 benchmark/hardening baselines (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured evidence (Target: Q1 2027)
- [ ] complete phase-7 default workflow integration after gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] EPIC 2 contract ownership mapped and documented

### Phase 2: Core Implementation
- [ ] scaffold translation units to be added with the implementation PR

### Phase 3: Error Handling and Edge Cases
- [ ] runtime policy and failure semantics implemented and verified

### Phase 4: Tests
- [ ] contract and integration tests implemented in `tests/epic2_evaluation/`

### Phase 5: Performance and Hardening
- [ ] benchmark suite implemented in `benchmarks/epic2_evaluation/`

### Phase 6: Documentation and Acceptance
- [x] core evaluation docs aligned to source-verifiable scaffold state
- [ ] acceptance docs tied to measured runtime behavior

### Phase 7: Production Integration
- [ ] default pipeline integration enabled after gates are met

## Production Readiness Checklist

- [x] contract ownership and scope boundaries documented
- [x] security and performance expectations documented for scaffold phase
- [ ] runtime policy hardening completed
- [ ] test and benchmark gates implemented and passing
