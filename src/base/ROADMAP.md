# Base Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production runtime foundations exist for secure module loading, sandboxing, dependency handling, hot reload, registry client integration, and plugin runtime composition.

## In Progress

- [~] hardening of runtime edge behavior across loader/sandbox/reload paths (Target: Q3 2026)
- [~] benchmark stabilization and baseline hardening for base hot paths (Target: Q3 2026)
- [~] consistency tightening for dependency and registry failure diagnostics (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic error surfaces for sandbox/runtime-degraded states (Target: Q4 2026)
- [ ] expand regression coverage for reload/dependency edge permutations (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for module activation and rollback classes (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 and throughput envelopes for base module benchmark mappings (Target: Q1 2027)
- [ ] reduce proxy-like mappings through additional dedicated base microbenchmarks (Target: Q1 2027)
- [ ] finalize remaining wasm/runtime backend hardening for production profiles (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze loader/sandbox/reload contract semantics for active major line (Target: Q3 2026)
- [ ] define explicit failure taxonomy for dependency and registry intake paths (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete remaining hardening in loader, sandbox, and hot-reload internals (Target: Q4 2026)
- [ ] align runtime and registry behavior to shared bounded execution contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed artifacts and degraded runtime backends (Target: Q4 2026)
- [ ] unify diagnostics across load-order, compatibility, and rollback failures (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for reload rollback and dependency conflict scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for sandbox/runtime and registry permutations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for base hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core base module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core base runtime surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for runtime and backend edge cases
- [ ] release benchmark stabilization completed for target envelopes

## Known Issues and Limitations

- behavior remains partly capability-dependent on enabled runtime backends/options.
- selected edge scenarios still require additional hardening and diagnostics tightening.
- benchmark baseline depth requires continued hardening for selected base targets.

## Breaking Changes

No breaking base-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.