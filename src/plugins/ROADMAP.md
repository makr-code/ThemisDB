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

# Plugins Module Roadmap

This page records source-visible implementation surfaces and unresolved
verification work. It does not assert release readiness or test results.
Project-wide readiness and blocker status are governed by
[`../../ROADMAP.md`](../../ROADMAP.md).

## Current Status

The source tree contains a native plugin manager, a separate typed factory
registry, edition policy checks, health and hot-plug monitoring, metrics, OCI
retrieval, a signed-catalog API, and a configurable WASM loader. Source presence
does not establish that a specific build or test has succeeded.

## In Progress

- [!] Verify approved artifact-provenance policy across manifest, explicit-path,
  OCI, signed-catalog, and WASM paths (Target: not assigned).
- [!] Confirm whether native plugins require an isolation boundary beyond
  host-process loading (Target: not assigned).
- [!] Establish durable audit/metrics retention requirements at the system
  boundary (Target: not assigned).

## Planned Features

- [!] Add end-to-end lifecycle, hot-plug, OCI, or WASM benchmarks only after an
  owner defines scope and a reproducible baseline (Target: not assigned).
- [!] Reconcile implementation-specific failures with the normative vocabulary
  in `include/plugins/plugins_api_contract.h` (Target: not assigned).

These are candidates for owner and policy review, not approved commitments.

## Implementation Phases

### Phase 1: Design / API Contract

- [!] Confirm the supported load-path policy and its provenance guarantees
  against `plugin_manager.h`, `oci_registry_client.h`, and
  `signed_plugin_repository.h` (Target: not assigned).

### Phase 2: Core Implementation

- [x] Source declares manager-owned plugin lifecycle and a separate typed
  factory registry; see `plugin_manager.cpp` and `plugin_registry.cpp`.
- [!] Verify build/runtime wiring for each optional WASM backend (Target: not
  assigned).

### Phase 3: Error Handling and Edge Cases

- [!] Compare manager and distribution failure behavior with the API contract
  and document any remaining differences (Target: not assigned).

### Phase 4: Tests

- [x] Focused test sources are registered by `tests/plugins/CMakeLists.txt`
  when the test configuration and GTest are available.
- [!] Record actual test results only after an authorized test run (Target: not
  assigned).

### Phase 5: Performance and Hardening

- [x] `bench_plugins_release_gates` is registered for contract/error-path
  microbenchmarks when benchmarks are enabled.
- [!] Establish reproducible end-to-end plugin lifecycle baselines before
  claiming lifecycle performance gates (Target: not assigned).

### Phase 6: Documentation and Acceptance

- [x] Developer docs describe source-supported API, runtime, trust, and test
  boundaries.
- [!] Obtain human review and policy decisions for the open items in this
  roadmap (Target: not assigned).

## Production Readiness Checklist

- [!] Approved artifact provenance and publisher-authentication policy across
  all load paths.
- [!] Documented native-code isolation decision and operational boundary.
- [!] Approved persistent audit/metrics retention policy.
- [!] Reproducible focused-test and benchmark evidence for the target build.
- [x] Source-level load-path and trust distinctions are documented.

## Known Issues & Limitations

- Native shared-library plugins execute in the host process; this module does
  not provide native process isolation.
- Manifest `.sig` hash comparison, binary verification, pinned-key Ed25519
  catalog verification, OCI layer digests, and WASM expected hashes are distinct
  controls; see [`TRUST.md`](./TRUST.md).
- OCI digest verification does not by itself establish publisher identity.
- The WASM loader callback is application-supplied; this module does not
  establish its validation behavior or a general sandbox guarantee.
- In-process health/metrics APIs do not establish durable audit retention.

## Breaking Changes

No public API change is made by this documentation update. No approved
plugin-contract breaking change was verified in the sources consulted.
