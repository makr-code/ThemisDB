---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-11-18"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/base/`, `include/themis/base/`, `src/base/`, `tests/base/CMakeLists.txt`
Related issues: #7093, #7167

# Base module guide

## Audience and ownership

For maintainers changing shared runtime helpers, resource pools, module loading, dependency ordering, or reload. The base module is infrastructure; keep feature-specific behavior in its owning module.

## Find the matching API

- Memory buffers and connection slots: `include/base/`; corresponding `.cpp` files are registered in modular `THEMIS_BASE_SOURCES` and in the monolithic core through `cmake/MiscellaneousFeatures.cmake`.
- Module loading and replacement: `include/themis/base/module_loader.h`, `hot_reload_manager.h`, `plugin_dependency_graph.h`.
- Test declarations: `tests/CMakeLists.txt` auto-discovers module CMake files and adds `tests/base/CMakeLists.txt`. The local file registers focused unit tests and a focused 7200-second soak target; `RegisterModuleTests.cmake` supplies `module:base`, tier, kind, and focus labels. The root file separately registers reload-regression and BaseEntity field tests. Registration does not imply tests were run or passed.

## Maintainer workflow

1. Establish which public header and namespace the caller uses.
2. Trace ownership, lifetime, timeout, shutdown, or staged-load transitions in implementation.
3. Cover exhaustion/timeout and teardown/failure cases, not only successful acquisition or activation.
4. Verify the parent CMake wiring before relying on a module-local test target; the root test CMake does register `test_base_reload_regression.cpp`.
5. Update `API.md` for caller-visible semantics and `IMPLEMENTATION.md` for changed runtime invariants. No build or test run is implied by these instructions.

See `ARCHITECTURE.md`, `ROADMAP.md`, and `SECURITY.md` for wider module context.

## Validation contract

Use the configured release profile and include all required toolchain/dependency
options. A focused resource-pool check is:

```sh
cmake -S . -B <build-dir> -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_BUILD_TESTS=ON
cmake --build <build-dir> --target module_base_test_resource_pools_focused
ctest --test-dir <build-dir> --output-on-failure -R '^test_resource_pools_base_FocusedTests$'
```

For degraded reload paths, build `module_base_test_base_reload_regression_focused`
and run `ctest --test-dir <build-dir> --output-on-failure -R '^BaseReloadRegressionFocusedTests$'`.
For sandbox/WASM and registry failure/degraded paths, build
`module_base_test_base_future_enhancements_focused` and run
`ctest --test-dir <build-dir> --output-on-failure -R '^test_base_future_enhancements_base_FocusedTests$'`.
Run the separately registered soak test only when the longer validation window is
available: `ctest --test-dir <build-dir> --output-on-failure -R '^test_base_soak_base_FocusedTests$'`.

Configure and build the release benchmark smoke targets with:

```sh
cmake -S . -B <bench-build> -DCMAKE_BUILD_TYPE=Release -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build <bench-build> --target bench_base_hot_paths bench_base_wasm_sandbox
ctest --test-dir <bench-build> --output-on-failure -R '^bench_base_(hot_paths|wasm_sandbox)$'
```

These CTest entries are smoke runs. Their GATE-BASE labels and thresholds are
definitions, not measurements. A performance pass requires reproducible
release-profile runs on declared representative hardware, comparison with
`benchmarks/baselines/base/wave_d_baselines.json`, variance/context records, and
human sign-off. The workflows inspected do not consume these base gates for CI
comparison; CMake registration or a local benchmark smoke run alone must not be
reported as a release pass.
