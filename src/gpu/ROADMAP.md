# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production GPU runtime exists across device discovery, allocation/governance, backend execution, stream/launcher orchestration, fallback management, and accelerated query/training paths.

## In Progress

- [~] hardening topology-aware and peer-transfer edge behavior under mixed runtime capabilities (Target: Q3 2026)
- [~] benchmark stabilization for core allocation, backend, and acceleration hot paths (Target: Q3 2026)
- [~] diagnostics consistency for quota denials, backend degradation, and fallback incidents (Target: Q3 2026)
- [~] GPU query accelerator kernel launchers for common query types (Target: Q4 2026)
- [~] GPU vector index CUDA backend integration and optimization (Target: Q4 2026)
- [~] GPU vector index HIP backend feature-parity (Target: Q4 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for multi-device dispatch under heterogeneous hardware states (Target: Q4 2026)
- [ ] extend stress coverage for sustained mixed query/training acceleration workloads (Target: Q4 2026)
- [ ] improve operator-facing incident diagnostics for fallback and capability mismatch scenarios (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for backend and acceleration pathways (Target: Q1 2027)
- [ ] broaden benchmark depth for topology, partition, and high-volume concurrency scenarios (Target: Q1 2027)
- [ ] harden long-running reliability under sustained multi-tenant acceleration pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze resource/backend/acceleration/operations contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for quota, degradation, and fallback classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for allocation, backend selection, and dispatch internals (Target: Q4 2026)
- [ ] align advanced topology/partition/transfer behavior with bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for capability mismatch and backend errors (Target: Q4 2026)
- [ ] unify diagnostics across denial, fallback, and degraded execution incidents (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for mixed-backend/mixed-capability edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for multi-tenant and multi-device workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for GPU hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core GPU module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core GPU surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for topology/partition/fallback edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on available hardware capability and configured feature gates.
- advanced topology and partition surfaces need continued hardening in mixed environments.
- benchmark breadth should continue expanding for complex multi-device scenarios.

## Breaking Changes

No breaking GPU contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.