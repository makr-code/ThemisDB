# API Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production API adapter surfaces exist for GraphQL, gRPC, WebSocket, tracing middleware, and OTLP export integration.

## In Progress

- [x] protocol hardening and consistency pass for advanced API transport behaviors (Target: Q3 2026)
  - Evidence: test_api_transport_hardening.cpp (19 tests validating fail-closed behavior, version negotiation, bounded resources)
- [x] benchmark and release-gate consolidation for API transport paths (Target: Q3 2026)
  - Evidence: bench_api_transport.cpp (18 benchmarks covering parsing, serialization, validation, tracing overhead)
- [x] observability and transport reliability alignment under sustained concurrency (Target: Q3 2026)
  - Evidence: test_api_observability.cpp (16 tests validating metrics, bounded queues, thread safety)

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
- [x] remaining API hardening items closed (protocol hardening + concurrency tests complete)
- [x] all targeted release-gate benchmarks stabilized (18 transport benchmarks added)

## Known Issues and Limitations

- transport surfaces remain configuration/capability dependent by deployment profile.
- some API surfaces may require feature flags for optional protocol support (WebSocket, gRPC reflection).
- future enhancements for extended benchmark coverage targeting Q1 2027 (latency/throughput baselining for representative load profiles).

## Breaking Changes

No breaking API module contract planned. Any transport contract break requires migration notes and changelog entry before merge.