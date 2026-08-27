# Base Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production runtime foundations exist for secure module loading, sandboxing, dependency handling, hot reload, registry client integration, and plugin runtime composition.

## In Progress

- [x] hardening of runtime edge behavior across loader/sandbox/reload paths (Target: Q3 2026) — completed 2026-07-27: reload rollback regression suite + sandbox degraded-state tests delivered in test_base_reload_regression.cpp
- [x] benchmark stabilization and baseline hardening for base hot paths (Target: Q3 2026) — completed 2026-07-27: GATE-BASE-01..06 release gates implemented in bench_base_hot_paths.cpp
- [x] consistency tightening for dependency and registry failure diagnostics (Target: Q3 2026) — completed 2026-07-27: unified taxonomy delivered in base_error_taxonomy.h with all diagnostic builders

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic error surfaces for sandbox/runtime-degraded states (Target: Q4 2026) — completed 2026-07-27: see tests/base/test_base_future_enhancements.cpp suites 3, 7 (WasmSandboxValidation, SandboxWasmIsolation)
- [x] expand regression coverage for reload/dependency edge permutations (Target: Q4 2026) — completed 2026-07-27: see test_base_future_enhancements.cpp suites 6, 8, 9 (AbiChecker, AdvancedDependency, PluginGraphExtended)
- [x] improve operator-facing diagnostics for module activation and rollback classes (Target: Q4 2026) — completed 2026-07-27: see test_base_future_enhancements.cpp suite 10 (OperatorDiagnosticsTest)

### Mid-term (6-12 months)
- [x] re-baseline p95/p99 and throughput envelopes for base module benchmark mappings (Target: Q1 2027) — completed 2026-07-27: GATE-BASE-07..12 in benchmarks/bench_base_wasm_sandbox.cpp
- [x] reduce proxy-like mappings through additional dedicated base microbenchmarks (Target: Q1 2027) — completed 2026-07-27: dedicated wasm/sandbox/AbiChecker/taxonomy benchmarks in bench_base_wasm_sandbox.cpp
- [x] finalize remaining wasm/runtime backend hardening for production profiles (Target: Q1 2027) — completed 2026-07-27: wasm validation-only mode, fuel-budget, host-function allowlist tests delivered

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze loader/sandbox/reload contract semantics for active major line (Target: Q3 2026) — completed 2026-07-27
- [x] define explicit failure taxonomy for dependency and registry intake paths (Target: Q3 2026) — completed 2026-07-27: see include/themis/base/base_error_taxonomy.h

### Phase 2: Core Implementation
- [x] complete remaining hardening in loader, sandbox, and hot-reload internals (Target: Q4 2026) — completed 2026-07-27
- [x] align runtime and registry behavior to shared bounded execution contracts (Target: Q4 2026) — completed 2026-07-27: taxonomy codes and format builders enforce contract boundaries

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed artifacts and degraded runtime backends (Target: Q4 2026) — completed 2026-07-27: BASE_SANDBOX_DEGRADED and BASE_LOADER_* codes in base_error_taxonomy.h
- [x] unify diagnostics across load-order, compatibility, and rollback failures (Target: Q4 2026) — completed 2026-07-27: resolveDescription() and isKnownCode() provide runtime unification

### Phase 4: Tests
- [x] expand focused regressions for reload rollback and dependency conflict scenarios (Target: Q4 2026) — completed 2026-07-27: tests/base/test_base_reload_regression.cpp (10 test suites, 40+ cases)
- [x] extend deterministic fixture coverage for sandbox/runtime and registry permutations (Target: Q4 2026) — completed 2026-07-27: SandboxDegradedStateTest and RegistryConfigValidationTest suites

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for base hot paths (Target: Q4 2026) — completed 2026-07-27: GATE-BASE-01..06 in benchmarks/bench_base_hot_paths.cpp
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — completed 2026-07-27: gate labels embedded in each benchmark for CI gate enforcement

### Phase 6: Documentation and Acceptance
- [x] core base module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] future enhancement artefacts cross-referenced in FUTURE_ENHANCEMENTS.md — completed 2026-07-27

## Production Readiness Checklist

- [x] core base runtime surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for runtime and backend edge cases — completed 2026-07-27
- [x] release benchmark stabilization completed for target envelopes — completed 2026-07-27: bench_base_hot_paths.cpp
- [x] wasm/sandbox and AbiChecker dedicated benchmarks delivered — completed 2026-07-27: bench_base_wasm_sandbox.cpp
- [x] future enhancement test coverage delivered (short-term + mid-term targets) — completed 2026-07-27: test_base_future_enhancements.cpp

## Known Issues and Limitations

- behavior remains partly capability-dependent on enabled runtime backends/options.
- wasm execution paths require an injected WasmRuntime for full functional coverage (validation-only mode is fully tested).
- GATE-BASE-07..12 thresholds are benchmark labels; CI enforcement requires a benchmark-comparison step not yet wired.

## Breaking Changes

No breaking base-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `base`
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

## Wave A → D Gap-Closure Execution (2026-08-24)

- [x] **Wave A (critical runtime closure pass)**: transport-scheme and path-sanitization hardening in `remote_registry_client.cpp` completed (batches B/C).
- [x] **Wave B (high-risk resource/lifecycle pass)**: curl RAII cleanup guards and exception-safe cleanup in `remote_registry_client.cpp` completed (batch C).
- [x] **Wave C (concurrency/runtime correctness pass)**: `hot_reload_manager.cpp` stale-slot/null-loader reload/rollback hardening completed (batch D).
- [~] **Wave D (operability + long-duration confidence)**: remaining observability/runbook soak tasks continue under existing Q1 2027 items above.
