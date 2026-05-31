# Chaos Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-ready in-process fault injection and scheduler surfaces are available for deterministic resilience simulation workflows.

## In Progress

- [~] hardening concurrency and callback edge behavior under sustained stress profiles (Target: Q3 2026)
- [~] benchmark stabilization for scheduler and concurrent stress pathways (Target: Q3 2026)
- [~] diagnostics consistency improvements for injected/recovered fault lifecycle events (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for callback-heavy and pending-queue edge permutations (Target: Q4 2026)
- [ ] expand resilience regressions for schedule/stop/restart timing races (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for fault lifecycle and scheduler state transitions (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for scheduler and concurrent stress benchmarks (Target: Q1 2027)
- [ ] add dedicated chaos microbenchmarks for additional fault classes and timing modes (Target: Q1 2027)
- [ ] evaluate controlled distributed-chaos coordination strategy for future extension (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze fault descriptor and scheduler contract semantics for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for inject/recover/schedule failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete remaining hardening for registry and scheduler internals (Target: Q4 2026)
- [ ] align callback/state transition behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for invalid timing and descriptor states (Target: Q4 2026)
- [ ] unify diagnostics across scheduler queue and callback failure classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for high-concurrency scheduler and callback scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for wake-strategy and queue edge matrixes (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for scheduler/concurrency hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core chaos module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core chaos surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for concurrency/callback edge cases
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- behavior is intentionally process-local and non-persistent.
- direct external sabotage is outside scope of the in-process simulation model.
- benchmark depth remains limited to current chaos stress coverage and needs expansion.

## Breaking Changes

No breaking chaos-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.