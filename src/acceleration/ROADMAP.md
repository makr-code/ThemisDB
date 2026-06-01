# Acceleration Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade acceleration runtime with backend selection, fallback orchestration, plugin/security guards, and multi-device integration.

## In Progress

- [~] Runtime capability hardening for fail-closed behavior under partial backend availability (Target: Q3 2026)
- [~] Multi-device and resource-management reliability tuning under sustained load (Target: Q3 2026)
- [~] Benchmark and regression gate consolidation for acceleration-critical release profiles (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Expand deterministic regressions for backend-selection and fallback edge cases (Target: Q4 2026)
- [ ] Strengthen diagnostics for plugin/security deny paths and degraded runtime states (Target: Q4 2026)
- [ ] Harden distributed merge/resource behavior under partial device failures (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Re-baseline acceleration latency/throughput envelopes across representative hardware profiles (Target: Q1 2027)
- [ ] Extend capability-matrix coverage for optional backend combinations (Target: Q1 2027)
- [ ] Improve operator-facing observability for backend health and dispatch routing decisions (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] Freeze backend capability/selection contract and fallback semantics for active major lines (Target: Q3 2026)
- [ ] Define explicit failure contracts for unavailable backend, invalid input, and integrity-check failure states (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] Complete hardening for capability-driven dispatch and deterministic fallback selection paths (Target: Q4 2026)
- [ ] Align multi-device and resource manager behavior to shared bounded execution contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] Enforce fail-closed behavior for malformed workload input, plugin/signature failure, and partial device outages (Target: Q4 2026)
- [ ] Standardize fallback semantics when optional acceleration features are unavailable (Target: Q4 2026)

### Phase 4: Tests
- [ ] Expand focused regressions for backend matrix, plugin security, and fallback correctness (Target: Q4 2026)
- [ ] Extend multi-device failure-injection regressions for merge and resource paths (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] Lock benchmark-backed release gates for dispatch, backend throughput, and fallback overhead (Target: Q4 2026)
- [ ] Validate sustained-load behavior for capability probing, queueing, and memory/resource paths (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [ ] Keep acceleration docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist

- [ ] API and behavior contracts verified by focused acceleration regressions
- [ ] Security and integrity checks verified on plugin and shader execution paths
- [ ] Performance expectations validated through mapped release-profile benchmarks
- [ ] Failure handling validated for timeout, degraded backend, and partial device modes
- [ ] Audit and changelog documentation synchronized with implementation deltas

## Known Issues and Limitations

- Hardware and driver differences can alter runtime behavior and performance envelopes.
- Some optional backend combinations remain environment dependent.
- Distributed and plugin-heavy scenarios need continuous hardening evidence.

## Breaking Changes

- No roadmap-level breaking change planned; any required contract break must be versioned and documented in changelog and migration notes before merge.
