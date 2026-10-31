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

# Base module documentation audit

## Evidence reviewed

- Immediate-scope Markdown inventory at audit time: 21 files in `src/base/`,
  one in `include/base/`, one in `benchmarks/base/`, and none in `tests/base/`.
  This differs from the 17-file `src/base/` inventory supplied in the task.
- Public declarations: `include/base/*.h` and relevant `include/themis/base/*.h`.
- Source inventory: ten `.cpp` files directly under `src/base/`.
- Explicit library source list: `cmake/ModularBuild.cmake`.
- Test declarations: `tests/base/CMakeLists.txt` and base-related registrations
  in `tests/CMakeLists.txt`.
- Benchmark definitions: `benchmarks/base/bench_base_*.cpp` and
  `benchmarks/CMakeLists.txt`.

## Current build/test registration findings

| Area | Finding |
|---|---|
| Module loader | `src/base/module_loader.cpp` is a deprecated legacy copy. In modular mode, `THEMIS_BASE_SOURCES` uses `src/themis/module_loader.cpp` and its platform/security companions. |
| Base target | `cmake/ModularBuild.cmake` creates the `themis_base` library from `THEMIS_BASE_SOURCES`; in modular mode, `cmake/CMakeLists.txt` exposes `themis_core` as an interface target that links the modules. |
| Resource pools | `src/base/buffer_pool.cpp` and `resource_pool_manager.cpp` are registered in modular `THEMIS_BASE_SOURCES` and in the monolithic core through `cmake/MiscellaneousFeatures.cmake`; `tests/base/test_resource_pools.cpp` adds focused allocation, fallback, statistics, and shutdown coverage. |
| Tests | `tests/CMakeLists.txt` uses `file(GLOB CONFIGURE_DEPENDS "*/CMakeLists.txt")` and `add_subdirectory()`, so it includes `tests/base/CMakeLists.txt`. That module file registers the current `test_*.cpp` sources as focused tests: unit-tier targets plus a separately configured `test_base_soak.cpp` soak target with a 7200-second timeout. The helper adds `module:base`, tier, kind, and focus labels; the module file adds `base` and `soak`. Root CMake also explicitly registers focused targets for `test_base_reload_regression.cpp` and `test_base_entity_focused.cpp`. Registration does not establish build or test success. |
| Benchmarks | `bench_base_hot_paths` and `bench_base_wasm_sandbox` are conditionally created from `benchmarks/CMakeLists.txt`; benchmark execution or gate results were not verified. |

## Audit limits and open items

- CMake source registration is verified. A direct `g++ -fsyntax-only` check
  passed for both pool implementations, and the gap-scanner regression test
  passed. CMake configuration stopped at the missing required `fmt` package, so
  the C++ GoogleTest target was not built or run.
- Wiki integration gap: no base-specific curated page was found in
  `ai_context/developer_llm_wiki/MODULES_AND_APIS.md`, and the suggested
  `ai_context/developer_llm_wiki/modules/base/` evidence directory is absent.
  The repository instructions require missing wiki coverage to be reported;
  creating/indexing that material is outside this task's allowed paths.
- Doxygen symbol counts remain a historical snapshot when Doxygen is unavailable;
  see `DOXYGEN.md` for the latest generation attempt and its result.
- Benchmark thresholds exist in source, but measured results and hardware
  sign-off remain open (also noted in `ROADMAP.md`).

## Canonical references

- [`../themis/module_loader.cpp`](../themis/module_loader.cpp)
- [`../../cmake/ModularBuild.cmake`](../../cmake/ModularBuild.cmake)
- [`../../cmake/CMakeLists.txt`](../../cmake/CMakeLists.txt)
- [`../../tests/base/CMakeLists.txt`](../../tests/base/CMakeLists.txt)
- [`../../tests/cmake/RegisterModuleTests.cmake`](../../tests/cmake/RegisterModuleTests.cmake)
- [`../../tests/CMakeLists.txt`](../../tests/CMakeLists.txt)
- [`../../benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt)
