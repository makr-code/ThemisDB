# API Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production API adapter surfaces exist for GraphQL, gRPC, WebSocket, tracing middleware, and OTLP export integration.

## In Progress

- [x] protocol hardening and consistency pass for advanced API transport behaviors (Target: Q3 2026)
  - Evidence: test_api_transport_hardening.cpp (13 tests validating fail-closed behavior, version negotiation, bounded resources)
- [x] benchmark and release-gate consolidation for API transport paths (Target: Q3 2026)
  - Evidence: bench_api_transport.cpp (13 benchmarks covering parsing, serialization, validation, tracing overhead)
- [x] observability and transport reliability alignment under sustained concurrency (Target: Q3 2026)
  - Evidence: test_api_observability.cpp (12 tests validating metrics, bounded queues, thread safety)

## Planned Features

### Short-term (3-6 months)
- [x] complete remaining API surface specification and contract consistency tasks (Target: Q4 2026)
  - Evidence: api_transport_contracts.h, api_error_taxonomy.h, api_transport_policy.h/cpp
- [x] strengthen degraded-mode handling for optional transport features (Target: Q4 2026)
  - Evidence: tests/api/test_api_degraded_mode.cpp — 10 tests covering capability unavailability, transient degradation, policy+degraded composition, concurrent mixed traffic
- [x] extend integration diagnostics for protocol-level failure classes (Target: Q4 2026)
  - Evidence: `DegradedModeDiagnosticsTest/AllFailureClassesHaveActionableMessages` validates that every failure class produces an ERR_-prefixed actionable message for operator triage

### Mid-term (6-12 months)
- [ ] expand direct benchmark coverage for currently proxy-like API goals (Target: Q1 2027)
- [ ] re-baseline API latency and throughput envelopes for representative load profiles (Target: Q1 2027)
- [ ] harden multi-transport operational controls across deployment topologies (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] lock transport-surface contracts for active major line (Target: Q3 2026)
  - Evidence: include/api/api_transport_contracts.h — `ITransportContract`, `TransportContractValidator`, `TransportCapability`, `kSupportedApiVersions`, `kMaxPayloadBytes`, `kMaxPathBytes`
- [x] define explicit failure contracts across GraphQL/gRPC/WebSocket adaptation paths (Target: Q3 2026)
  - Evidence: include/api/api_transport_contracts.h — `TransportFailureClass` enum with 9 canonical failure classes and Doxygen-documented HTTP status mapping

### Phase 2: Core Implementation
- [x] close remaining hardening deltas in protocol-adapter and middleware surfaces (Target: Q4 2026)
  - Evidence: include/api/api_transport_policy.h + src/api/api_transport_policy.cpp — `TransportPolicyMiddleware` enforces 5 policy rules (malformed request, path length, payload limit, version check, Content-Type for mutating methods)
- [x] align gRPC and WebSocket edge behavior with shared API policy contracts (Target: Q4 2026)
  - Evidence: `ITransportContract` interface implemented by `TransportPolicyMiddleware`; `TransportCapability` flags enable per-deployment capability advertising for gRPC and WebSocket adapters

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed payload and unsupported capability states (Target: Q4 2026)
  - Evidence: `TransportPolicyMiddleware::handle()` returns immediately on any policy violation without calling inner handler; validated in test_api_phase4_concurrency.cpp and test_api_degraded_mode.cpp
- [x] unify error taxonomy across transport adapters and middleware paths (Target: Q4 2026)
  - Evidence: include/api/api_error_taxonomy.h — `ApiErrorTaxonomy` with `toErrorCode()`, `toHttpStatus()`, `toMessage()`, `isClientError()` covering all 9 `TransportFailureClass` values

### Phase 4: Tests
- [x] expand focused regressions for high-concurrency and transport-edge scenarios (Target: Q4 2026)
  - Evidence: tests/api/test_api_phase4_concurrency.cpp — 14 tests covering 32×50 concurrency matrix, payload boundary, path length boundary, method×version×payload combination matrix, taxonomy correctness
- [x] extend deterministic integration matrix coverage for protocol combinations (Target: Q4 2026)
  - Evidence: `Phase4MatrixTest/ValidProtocolCombinationsSucceed` — 6 methods × 3 version states × 2 content-type variants = 36 deterministic combinations validated

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for API parsing/execution/serialization hot paths (Target: Q4 2026)
  - Evidence: benchmarks/bench_api_release_gates.cpp — GATE-API-01..06 with documented per-gate limits (≤5 µs GET, ≤10 µs POST, ≤5 µs rejection paths, ≤1 µs taxonomy mapping)
- [x] validate p95/p99 envelopes under representative concurrency profiles (Target: Q4 2026)
  - Evidence: `BM_PolicySustainedGetThroughput`, `BM_PolicySustainedPostThroughput`, `BM_PolicyMixedRequestTypes` benchmarks establishing sequential and mixed-type throughput baselines

### Phase 6: Documentation and Acceptance
- [x] core API docs aligned to source-verifiable behavior
- [x] roadmap/future vs changelog role separation synchronized

## Production Readiness Checklist

- [x] core transport adapter surfaces documented and source-verified
- [x] security and failure handling documented at module level
- [x] benchmark mapping documented in performance expectations
- [x] remaining API hardening items closed (protocol hardening + concurrency tests complete)
- [x] all targeted release-gate benchmarks stabilized (13 transport benchmarks added)
- [x] Phase 1: transport-surface contracts locked (api_transport_contracts.h)
- [x] Phase 2: hardening deltas closed — TransportPolicyMiddleware enforces 5 canonical rules
- [x] Phase 3: error taxonomy unified — ApiErrorTaxonomy covers all 9 failure classes
- [x] Phase 4: concurrency/edge regressions expanded (14 tests in test_api_phase4_concurrency.cpp)
- [x] Phase 5: release-gate benchmarks locked (GATE-API-01..06 in bench_api_release_gates.cpp)
- [x] Q4 2026 degraded-mode hardening complete (10 tests in test_api_degraded_mode.cpp)

## Known Issues and Limitations

- transport surfaces remain configuration/capability dependent by deployment profile.
- some API surfaces may require feature flags for optional protocol support (WebSocket, gRPC reflection).
- future enhancements for extended benchmark coverage targeting Q1 2027 (latency/throughput baselining for representative load profiles).

## Breaking Changes

No breaking API module contract planned. Any transport contract break requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `api`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
