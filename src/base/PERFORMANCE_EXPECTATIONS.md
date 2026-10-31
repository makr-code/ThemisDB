---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base performance expectations

## Current benchmark mapping

The base benchmark sources are in `benchmarks/base/` and are registered from
`benchmarks/CMakeLists.txt` when the files exist:

| Target | Source | Current defined coverage |
|---|---|---|
| `bench_base_hot_paths` | `bench_base_hot_paths.cpp` | GATE-BASE-01..06: `ModuleLoader` query/metrics, `HotReloadManager` registered modules and rollback availability, 100-node `PluginDependencyGraph` construction, and reload fast-fail. |
| `bench_base_wasm_sandbox` | `bench_base_wasm_sandbox.cpp` | GATE-BASE-07..12: WASM valid/invalid loading, `AbiChecker`, taxonomy functions, and inactive `ModuleSandbox` statistics; additional sandbox/graph microbenchmarks are present. |

These are benchmarked lifecycle/WASM surfaces. The buffer-pool and resource
pool implementations are not part of these two benchmark targets.

## Interpretation and limits

- The source and CMake definitions establish that benchmark targets and labels
  are defined; they do not establish that the gates have passed.
- The target values are benchmark labels/expectations. No current
  representative-hardware measurements or CI comparison results were verified
  in this documentation audit.
- `src/base/ROADMAP.md` states that the baseline JSON contains thresholds but
  hardware measurement/sign-off remains pending.
- `src/base/buffer_pool.cpp` and `resource_pool_manager.cpp` are registered in
  modular and monolithic builds. Performance measurements for their integrated
  build path remain unverified; source registration does not establish benchmark
  results.

## Canonical references

- [`benchmarks/base/README.md`](../../benchmarks/base/README.md)
- [`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt)
- [`src/base/ROADMAP.md`](ROADMAP.md), Program-Level Success Criteria
