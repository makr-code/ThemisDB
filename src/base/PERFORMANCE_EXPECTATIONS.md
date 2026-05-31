# PERFORMANCE_EXPECTATIONS - src/base

## Scope

- Module: src/base
- This file defines measurable base module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_module_load_hot_reload.cpp
  - benchmarks/bench_themis_core.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| BAS-1 | plugin directory scan with populated fixtures remains within release baseline budget | BM_ScanDirectoryWithPlugins |
| BAS-2 | plugin metadata query paths remain bounded | BM_GetPluginInfo, BM_GetAllPlugins |
| BAS-3 | plugin load/unload and reload cycle remains bounded | BM_LoadUnloadPlugin, BM_ReloadPlugin |
| BAS-4 | module list growth behavior remains bounded | BM_GetAllLoadedModules_Growth |
| BAS-5 | module load with hash verification remains bounded | BM_ModuleLoad_WithHashVerify |
| BAS-6 | concurrent plugin query path remains bounded under configured thread ranges | BM_ConcurrentQueries, BM_ConcurrentGetAllPlugins |
| BAS-7 | typical plugin workflow path remains bounded in release profile | BM_TypicalWorkflow |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| BG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| BG-2 | loader and query path p99 <= release threshold | p99 from mapped plugin-system benchmark cases |
| BG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: base/performance)

- Verified benchmark sources:
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_module_load_hot_reload.cpp
  - benchmarks/bench_themis_core.cpp
- Verified mapping surfaces:
  - plugin scan, query, load/unload, reload paths
  - module growth and hash-verified load path
  - concurrent and workflow benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.