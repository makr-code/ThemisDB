# Plugins Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production plugin runtime exists for lifecycle management, manifest/signature validation, hot-plug behavior, health/metrics monitoring, and OCI/RPC integration surfaces.

## In Progress

- [~] hardening plugin edge-case behavior for capability and reload transitions (Target: Q3 2026)
- [~] benchmark stabilization for plugin lifecycle and hot-plug paths (Target: Q3 2026)
- [~] diagnostics consistency for validation/security/integration incidents (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under high plugin-churn and concurrent lifecycle operations (Target: Q4 2026)
- [ ] expand stress coverage for signature/manifest and capability edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for plugin incident triage (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for plugin lifecycle and query paths (Target: Q1 2027)
- [ ] broaden benchmark depth for signed repository and runtime integration scenarios (Target: Q1 2027)
- [ ] harden long-running reliability under sustained hot-plug workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze lifecycle/security/monitoring contracts for active major line (Target: Q3 2026) — evidence: include/plugins/plugins_api_contract.h
- [x] define explicit error taxonomy for plugin failure classes (Target: Q3 2026) — evidence: include/plugins/plugins_api_contract.h

### Phase 2: Core Implementation
- [ ] complete hardening for plugin lifecycle and registry internals (Target: Q4 2026)
- [ ] align hot-plug/health/integration behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for invalid manifests/signatures and reload faults (Target: Q4 2026)
- [ ] unify diagnostics across lifecycle/security/integration incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for plugin churn and capability edge scenarios (Target: Q4 2026) — evidence: tests/plugins/test_plugins_contract_hardening_focused.cpp
- [x] extend deterministic stress fixtures for hot-plug and registry operations (Target: Q4 2026) — evidence: tests/plugins/test_plugins_contract_hardening_focused.cpp

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for plugin hot paths (Target: Q4 2026) — evidence: benchmarks/plugins/bench_plugins_release_gates.cpp
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — evidence: benchmarks/plugins/bench_plugins_release_gates.cpp

### Phase 6: Documentation and Acceptance
- [x] core plugins module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core plugin surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for lifecycle/security/integration edge paths
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on plugin manifest quality, signatures, and enabled runtime features.
- selected lifecycle and hot-plug edge scenarios need continued hardening.
- benchmark breadth should continue expanding for advanced repository/runtime scenarios.

## Breaking Changes

No breaking plugin contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.