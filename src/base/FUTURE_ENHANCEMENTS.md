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

- tighten compatibility and rollback consistency under stressed reload scenarios.
- standardize diagnostics for load, dependency, and registry failure classes.
- expand runtime-backend resilience for sandbox and wasm paths.
- continue replacing proxy-like mappings with dedicated base benchmarks.

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