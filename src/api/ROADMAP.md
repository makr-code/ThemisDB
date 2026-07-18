# API Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production API adapter surfaces exist for GraphQL, gRPC, WebSocket, tracing middleware, and OTLP export integration.

## In Progress

- [~] protocol hardening and consistency pass for advanced API transport behaviors (Target: Q3 2026)
- [~] benchmark and release-gate consolidation for API transport paths (Target: Q3 2026)
- [~] observability and transport reliability alignment under sustained concurrency (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] complete remaining API surface specification and contract consistency tasks (Target: Q4 2026)
- [ ] strengthen degraded-mode handling for optional transport features (Target: Q4 2026)
- [ ] extend integration diagnostics for protocol-level failure classes (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] expand direct benchmark coverage for currently proxy-like API goals (Target: Q1 2027)
- [ ] re-baseline API latency and throughput envelopes for representative load profiles (Target: Q1 2027)
- [ ] harden multi-transport operational controls across deployment topologies (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] lock transport-surface contracts for active major line (Target: Q3 2026)
- [ ] define explicit failure contracts across GraphQL/gRPC/WebSocket adaptation paths (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] close remaining hardening deltas in protocol-adapter and middleware surfaces (Target: Q4 2026)
- [ ] align gRPC and WebSocket edge behavior with shared API policy contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed payload and unsupported capability states (Target: Q4 2026)
- [ ] unify error taxonomy across transport adapters and middleware paths (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for high-concurrency and transport-edge scenarios (Target: Q4 2026)
- [ ] extend deterministic integration matrix coverage for protocol combinations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for API parsing/execution/serialization hot paths (Target: Q4 2026)
- [ ] validate p95/p99 envelopes under representative concurrency profiles (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core API docs aligned to source-verifiable behavior
- [x] roadmap/future vs changelog role separation synchronized

## Production Readiness Checklist

- [x] core transport adapter surfaces documented and source-verified
- [x] security and failure handling documented at module level
- [x] benchmark mapping documented in performance expectations
- [ ] remaining API hardening items closed
- [ ] all targeted release-gate benchmarks stabilized

## Known Issues and Limitations

- some transport surfaces remain configuration/capability dependent by deployment profile.
- benchmark and specification hardening remains an active follow-up area.
- continued edge-case hardening is required for high-concurrency protocol scenarios.

## Breaking Changes

No breaking API module contract planned. Any transport contract break requires migration notes and changelog entry before merge.