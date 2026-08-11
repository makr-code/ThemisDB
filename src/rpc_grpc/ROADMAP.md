# RPC gRPC Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable gRPC RPC plugin runtime exists for server lifecycle operations, TLS/mTLS credentials handling, service registration, stream adapter support, and method-level observability.

## In Progress

- [x] hardening credential reload and runtime service transition edge behavior (Target: Q3 2026) — evidence: grpc_plugin.h:73-75 (reloadTls), tests RPC-05..RPC-07, benchmarks GATE-RPC-03
- [x] improving diagnostics consistency for lifecycle/registration fault classes (Target: Q3 2026) — evidence: include/rpc_grpc/rpc_grpc_api_contract.h (error taxonomy), tests RPC-01..RPC-04
- [x] stabilizing benchmark-backed release guardrails for WAL-apply gRPC path (Target: Q3 2026) — evidence: benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp (GATE-RPC-01..08 with p99 thresholds)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under concurrent service and reload operations (Target: Q4 2026)
- [ ] expand stress coverage for stream adapter and registration edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for transport incident triage (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for gRPC apply and service lifecycle paths (Target: Q1 2027)
- [ ] broaden benchmark depth for additional gRPC transport scenarios (Target: Q1 2027)
- [ ] harden long-run reliability under sustained RPC traffic pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze plugin lifecycle/registration/stream contracts for current major line (Target: Q3 2026) — evidence: include/rpc_grpc/rpc_grpc_api_contract.h
- [x] define explicit error taxonomy for rpc_grpc failure classes (Target: Q3 2026) — evidence: include/rpc_grpc/rpc_grpc_api_contract.h

### Phase 2: Core Implementation
- [x] complete hardening for lifecycle and credential internals (Target: Q4 2026) — evidence: reloadTls fail-safe semantics, start/stop deterministic hardening, credential validation
- [x] align registration/stream behavior to bounded runtime contracts (Target: Q4 2026) — evidence: registerService idempotent checks, bounded service registration, diagnostic logging

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for reload, registration, and stream path faults (Target: Q4 2026) — evidence: reloadTls atomic validation, start precondition checks, registration bounds (src/rpc_grpc/grpc_plugin.cpp Phase 3 hardened)
- [x] unify diagnostics across lifecycle/credentials/registration incidents (Target: Q4 2026) — evidence: structured error messages [RPC-Exxx], error taxonomy integration, fail-closed predicate (src/rpc_grpc/grpc_plugin.cpp)

### Phase 4: Tests
- [x] expand focused regressions for credential and registration edge scenarios (Target: Q4 2026) — evidence: tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp (RPC-01..RPC-16)
- [x] extend deterministic stress fixtures for sustained gRPC traffic paths (Target: Q4 2026) — evidence: tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp (RPC-09..RPC-16 edge cases)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for gRPC WAL-apply hot paths (Target: Q4 2026) — evidence: benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp (GATE-RPC-01..08)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — evidence: GATE-RPC-01..08 thresholds defined and implemented

### Phase 6: Documentation and Acceptance
- [x] core rpc_grpc docs aligned to source-verifiable behavior — evidence: include/rpc_grpc/rpc_grpc_api_contract.h (frozen v1.x contract)
- [x] roadmap/future planning separated from historical changelog entries — evidence: FUTURE_ENHANCEMENTS.md, CHANGELOG.md
- [x] Phase 6 acceptance checklist with sign-off gates — evidence: src/rpc_grpc/PHASE_6_ACCEPTANCE_CHECKLIST.md
- [x] operator runbook for deployment, configuration, troubleshooting — evidence: src/rpc_grpc/OPERATOR_RUNBOOK.md

## Production Readiness Checklist

- [x] core rpc_grpc surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for lifecycle/credentials/stream edge paths (Phase 2-3 hardening complete)
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on deployment credentials and service registration profile.
- selected reload/registration edge scenarios need continued hardening.
- benchmark depth should continue expanding beyond WAL-apply focused coverage.

## Breaking Changes

No breaking rpc_grpc contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `rpc_grpc`
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
