---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# RPC gRPC Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

`src/rpc_grpc` contains an optional gRPC RPC plugin with lifecycle, TLS/mTLS,
service registration, a bidirectional stream helper, and manual observability
hooks. `THEMIS_PLUGIN_RPC_GRPC` and an available gRPC package are required for
the shared-library target; Protobuf is required after gRPC is found.

The in-tree benchmarks are contract microbenchmarks and in-process simulations,
not end-to-end server measurements. This roadmap does not claim that a
performance threshold has passed.

## In Progress

- [~] Add reproducible live-server benchmarks for request, stream, and TLS
  lifecycle paths, and compare them with representative-hardware baselines
  (Target: Q1 2027).
- [~] Clarify the runtime error contract: `registerService()` returns `void`,
  while the `RpcGrpcError` enum is not wired into plugin results
  (Target: Q1 2027).

## Planned Features

- [ ] Define a cancellation/deadline strategy for blocked stream reads and writes
  before changing `BidiStreamAdapter` behavior (Target: Q1 2027).
- [ ] Add focused tests for any new runtime error or cancellation contract
  before exposing it as a stable API (Target: Q1 2027).

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Define the shared RPC plugin interface and contract header
  (`include/plugins/rpc_plugin_interface.h`,
  `include/rpc_grpc/rpc_grpc_api_contract.h`).
- [x] Document runtime versus contract-only types and configuration behavior
  (`API.md`, `IMPLEMENTATION.md`).

### Phase 2: Core Implementation
- [x] Implement the optional gRPC shared-library target and plugin lifecycle
  (`src/rpc_grpc/CMakeLists.txt`, `grpc_plugin.cpp`).
- [x] Implement listener setup, credentials, service registration, and shutdown.

### Phase 3: Error Handling and Edge Cases
- [x] Validate required host and TLS file inputs; avoid insecure fallback when
  TLS configuration fails.
- [x] Implement TLS reload rollback attempt and explicit handling for null,
  late, and duplicate service registration.
- [ ] Decide whether lifecycle/registration errors need a programmatic result
  channel instead of diagnostic-only reporting (Target: Q1 2027).

### Phase 4: Tests
- [x] Add focused contract and circuit-breaker test sources under
  `tests/rpc_grpc`.
- [x] Add lifecycle/integration and test-local stress cases; gRPC-dependent
  coverage remains conditional on the configured build.
- [ ] Record passing test results for the exact release configuration before
  using them as release evidence (Target: ongoing).

### Phase 5: Performance and Hardening
- [x] Register contract microbenchmarks and dedicated in-process benchmark
  scenarios (`benchmarks/rpc_grpc`).
- [ ] Add live plugin workloads, measured p95/p99 results, and automated
  baseline comparison; source labels alone are not validation evidence
  (Target: Q1 2027).

### Phase 6: Documentation and Acceptance
- [x] Document API, lifecycle, configuration, and test/benchmark boundaries.
- [~] Regenerate the Doxygen reference from current source when Doxygen is
  available; the checked-in generated page is an older snapshot
  (Target: next documentation refresh).

## Production Readiness Checklist

- [x] Runtime build conditions and the plugin/API boundary are documented.
- [x] TLS, insecure admin listener, registration, and reload limitations are
  documented.
- [x] Focused test and benchmark source locations are documented.
- [ ] Current test results and measured performance baselines are attached to a
  specific revision before release claims.

## Known Issues & Limitations

- `registerService()` returns `void`; rejection is reported diagnostically.
- The runtime does not consume all types or invariants declared by
  `rpc_grpc_api_contract.h`.
- TLS reload drains and restarts the listener, causing downtime; rollback can
  also fail.
- A valid `admin_port` binds a second listener with insecure credentials.
- `BidiStreamAdapter` does not interrupt an in-flight gRPC read/write on finish;
  zero queue depth can leave a writer waiting until finish.
- Metrics are manually recorded and are not exposed through a scrape endpoint.
- Module benchmark sources do not establish live gRPC latency or passed
  p95/p99 thresholds.

## Breaking Changes

No breaking change is proposed by this documentation update. Any future
contract change must include compatibility analysis, migration notes, tests,
and a changelog entry.

## Program Context

This module contributes to the repository-level roadmap; program release gates
and branch requirements remain defined by [`../../ROADMAP.md`](../../ROADMAP.md).
This page records module behavior and does not assert current CI or release
status.
