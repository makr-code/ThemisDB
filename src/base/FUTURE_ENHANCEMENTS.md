# Base Module - Future Enhancements

<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of module loading, sandbox, dependency, reload, and registry runtime paths
- expansion of deterministic reliability and observability behavior for edge conditions
- stricter benchmark-backed guardrails for base hot paths

## Design Constraints

- base runtime contracts remain backward compatible within major release line.
- module activation paths remain fail-closed for trust/integrity violations.
- sandbox/runtime execution remains bounded and observable.
- reload transitions remain deterministic and rollback-capable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| loader interfaces | deterministic activation and deactivation behavior |
| sandbox/runtime interfaces | bounded isolation behavior with explicit failure states |
| dependency interfaces | stable compatibility and ordering semantics |
| reload/registry interfaces | explicit rollback and intake diagnostics |

## Implementation Notes

- `include/themis/base/base_error_taxonomy.h` provides the standardized failure taxonomy
  for all base module subsystems (loader 1100–1149, sandbox 1150–1199, reload 1200–1249,
  dependency 1250–1299, registry 1300–1349). Each error code includes a `description()`
  and a `format(...)` diagnostic builder. Completed 2026-07-27 (Issue #5631).

- `tests/base/test_base_reload_regression.cpp` covers reload rollback scenarios, dependency
  conflict edge cases via PluginDependencyGraph / ModuleDependencyResolver, sandbox
  degraded-state paths, RegistryConfig validation edge cases, reload phase ordering
  (BEFORE_UNLOAD → AFTER_UNLOAD → AFTER_LOAD, ROLLBACK), stats tracking across
  success/failure cycles, multiple modules registered with callbacks invoked in order,
  and state save/restore callback error isolation. Completed 2026-07-27 (Issue #5631).

- `benchmarks/bench_base_hot_paths.cpp` provides the base hot-path release gates:
  GATE-BASE-01 (isModuleLoaded ≥500k ops/s), GATE-BASE-02 (getMetrics ≥100k ops/s),
  GATE-BASE-03 (registeredModules p99 ≤5µs at 10 modules), GATE-BASE-04
  (isRollbackAvailable p99 ≤1µs), GATE-BASE-05 (buildFromResolver 100-node chain ≤1ms),
  GATE-BASE-06 (reloadModule fast-fail path ≤50µs). Completed 2026-07-27 (Issue #5631).

- `tests/base/test_base_future_enhancements.cpp` delivers the short-term hardening
  targets (Q4 2026): AbiChecker version-compatibility edge cases (major/minor mismatch,
  patch tolerance, forward/backward compatibility), WasmPluginSandbox validation-only
  mode (magic validation, invalid bytes, no-runtime callExport, fuel-budget behavior,
  host-function allowlist), WasmModuleInfo invariants, ModuleSandbox WASM-isolation
  state predicates, advanced reload/dependency edge permutations (diamond, multi-root,
  5-level chain, indirect cycle, re-register), and operator-facing diagnostic completeness
  (format prefix consistency, resolveDescription/isKnownCode coverage). 10 test suites,
  55+ GTest cases. Completed 2026-07-27 (Issue #5631).

- `benchmarks/bench_base_wasm_sandbox.cpp` delivers the mid-term dedicated microbenchmark
  targets (Q1 2027): GATE-BASE-07 (WasmPluginSandbox loadFromBytes valid WASM),
  GATE-BASE-08 (loadFromBytes invalid bytes fast-fail), GATE-BASE-09 (AbiChecker
  checkVersions throughput), GATE-BASE-10 (BaseErrorTaxonomy resolveDescription/format),
  GATE-BASE-11 (isKnownCode throughput), GATE-BASE-12 (ModuleSandbox stats inactive),
  plus dedicated microbenchmarks for addHostFunction, hostFunctionCount, callExport
  no-runtime, PluginDependencyGraph DOT/JSON/ASCII export, detectCycles, and sandbox
  construction/destruction. Eliminates proxy-like benchmark mappings for wasm/sandbox
  surfaces. Completed 2026-07-27 (Issue #5631).

## Test Strategy

- unit and integration suites across loader/sandbox/reload/dependency flows.
- dependency conflict and rollback regression scenarios.
- degraded backend and registry fault-path deterministic tests.
- release-profile benchmark runs for mapped base targets.

## Performance Targets

- loader/reload and module query hot paths remain within release regression budgets.
- dependency and module list operations remain stable at p95/p99 versus baseline.
- benchmark manifests for mapped base targets reach no-missing-case status.

## Security / Reliability

- maintain strict fail-closed behavior for untrusted/incompatible module artifacts.
- preserve auditable runtime decisions for activation and rollback outcomes.
- enforce bounded resource behavior in sandbox and runtime surfaces.
- keep diagnostics actionable for production incident response.