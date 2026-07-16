# CDC Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production CDC runtime exists for change capture, buffering, replay, delivery tracking, transport integration, and operations/admin surfaces.

## In Progress

- [~] hardening transport and replay consistency under degraded backend conditions (Target: Q3 2026)
- [~] benchmark stabilization for capture/delivery latency and replication-lag pathways (Target: Q3 2026)
- [~] diagnostics consistency improvements for DLQ/outbox/consumer-group edge behavior (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for partial transport outages and failover transitions (Target: Q4 2026)
- [ ] expand regression coverage for replay, acknowledgement, and delivery-timeout edge permutations (Target: Q4 2026)
- [ ] improve operator diagnostics for lag, redelivery, and stream integrity incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline CDC p95/p99 and throughput envelopes for release profiles (Target: Q1 2027)
- [ ] reduce proxy-like mappings by expanding dedicated CDC microbenchmarks (Target: Q1 2027)
- [ ] harden multi-tenant and multi-transport consistency in sustained production workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze CDC capture/delivery/replay contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for transport, replay, and admin failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for capture buffer, transport, and consumer-group internals (Target: Q4 2026)
- [ ] align DLQ/outbox behavior to bounded runtime contracts across feature flags (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed events and degraded transport states (Target: Q4 2026)
- [ ] unify diagnostics across replay, acknowledgement, and redelivery failure classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for stream integrity, lag, and redelivery edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for transport/backend permutation matrixes (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for CDC capture/delivery hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core CDC module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core CDC surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for transport/replay edge cases
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- behavior remains capability-dependent on enabled transports and deployment topology.
- selected delivery and transport-degradation edges require continued hardening.
- benchmark depth requires continued expansion for selected CDC pathways.

## Breaking Changes

No breaking CDC-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.