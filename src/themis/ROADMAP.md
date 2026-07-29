# Themis Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-usable themis core runtime exists for build identity, edition/license gating, secure module loading and verification, dependency resolution, and wire server operation.

## In Progress

- [~] hardening loader/verification behavior under platform and trust-edge scenarios (Target: Q3 2026)
- [~] improving diagnostics consistency across license, verification, and wire runtime stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for themis core hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic outcomes for dependency and module trust edge cases (Target: Q4 2026)
- [ ] expand stress coverage for wire-session and loader contention scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for load/verify/gating incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for loader/gating/server-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for module lifecycle and wire runtime diversity (Target: Q1 2027)
- [ ] harden long-run reliability under sustained core runtime pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze themis core runtime/gating/lifecycle contracts for current major line (Target: Q3 2026)
- [x] define explicit error taxonomy for license/load/verify incident classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for loader/verifier and wire-server internals (Target: Q4 2026)
- [ ] align platform-specific load behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for signature, dependency, and runtime gate faults (Target: Q4 2026)
- [ ] unify diagnostics across license, lifecycle, and wire incident classes (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for loader/verify and wire-session edge scenarios (Target: Q4 2026)
- [x] extend deterministic stress fixtures for core runtime concurrency (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for themis core hot paths (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core themis module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core themis surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for loader/gating/wire edge paths
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on platform loader constraints and trust material configuration.
- selected verification and wire-session edge scenarios need continued hardening.
- benchmark depth should continue expanding for wider core runtime workloads.

## Breaking Changes

No breaking themis core contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.