# AQL Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production AQL-assistance surfaces exist across translation, validation, tooling, context, and scoring support paths.

## In Progress

- [~] hardening of generated-query safety and degraded-mode behaviors (Target: Q3 2026)
- [~] performance gate consolidation for AQL assistance benchmark paths (Target: Q3 2026)
- [~] consistency hardening across helper and bridge integration surfaces (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten validation and policy enforcement for complex generated-query patterns (Target: Q4 2026)
- [ ] expand deterministic integration tests for provider and bridge variability (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for translation confidence and failure classes (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] reduce remaining proxy-like benchmark coverage via dedicated assistance benchmarks (Target: Q1 2027)
- [ ] re-baseline latency and throughput envelopes for high-volume assistance usage (Target: Q1 2027)
- [ ] harden multi-step orchestration reliability under concurrent load (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze assistance contract semantics for translation/validation outputs (Target: Q3 2026)
- [ ] define explicit failure contracts for unsupported provider/capability modes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete remaining hardening in translation and bridge execution paths (Target: Q4 2026)
- [ ] align helper components to shared bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed/generated query edge cases (Target: Q4 2026)
- [ ] unify error taxonomy and diagnostics across assistance components (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for concurrency, degraded-mode, and policy-edge behavior (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for provider and schema-context variability (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for translation/highlighter/scorer/few-shot paths (Target: Q4 2026)
- [ ] validate p95/p99 behavior against release baselines under representative workloads (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core assistance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening items closed across translation/bridge edges
- [ ] release-gate benchmark stabilization complete

## Known Issues and Limitations

- behavior remains partially capability-dependent on configured providers and integrations.
- some benchmark coverage still relies on broader assistance benchmarks rather than fully isolated micro-paths.
- continued hardening remains required for adversarial and concurrency edge profiles.

## Breaking Changes

No breaking module contract planned. Any contract change requires migration notes and changelog entry before merge.