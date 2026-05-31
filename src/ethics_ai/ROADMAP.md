# Ethics AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade ethics_ai runtime exists for profile loading, discourse orchestration, argument persistence, RAG context assembly, evaluation metrics, and plugin lifecycle integration.

## In Progress

- [~] hardening deterministic behavior for profile-edge and multi-school debate permutations (Target: Q3 2026)
- [~] benchmark stabilization for decision, context, and evaluator hot paths (Target: Q3 2026)
- [~] diagnostics consistency improvements for plugin lifecycle and debate failure classes (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten conflict and convergence semantics for extended debate rounds (Target: Q4 2026)
- [ ] expand regression depth for profile reload and selection-router edge cases (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for context/routing degradation incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for decision and context assembly under sustained load (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced compression/cascade/synthesis workflows (Target: Q1 2027)
- [ ] harden long-running reliability under mixed profile quality and topology states (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze profile/discourse/store/context/evaluator contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for profile, lifecycle, and context failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for discourse orchestration and plugin lifecycle internals (Target: Q4 2026)
- [ ] align profile routing and context assembly behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for invalid profile and debate configuration scenarios (Target: Q4 2026)
- [ ] unify diagnostics across store, context, routing, and evaluator failure paths (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for profile/discourse/context edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for multi-school and long-round workflows (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for decision/context/evaluator hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core ethics_ai module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core ethics_ai surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for profile/discourse/context edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime quality remains dependent on profile quality and coverage.
- selected advanced context generation paths remain configuration-dependent.
- benchmark coverage should continue expanding for advanced ethics workflow helpers.

## Breaking Changes

No breaking ethics_ai contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.