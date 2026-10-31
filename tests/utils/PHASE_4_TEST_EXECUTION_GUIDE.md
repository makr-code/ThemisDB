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

# Utils Test Execution Guide

This guide describes the current CMake/CTest registration, not a fixed test
count. Test targets are conditionally registered when their source files and
required dependencies are present; `tests/utils/CMakeLists.txt` is the source
of truth for the active build.

## Configure and run one focused test

From the repository root:

```bash
cmake --preset community-debug
cmake --build build-community-debug --target module_utils_test_utils_pii_unicode_edge_cases_autofocused -j4
ctest --test-dir build-community-debug -R test_utils_pii_unicode_edge_cases_utils_AutoFocusedTests --output-on-failure
```

The target and test name above come from the CMake rule for
`test_utils_pii_unicode_edge_cases.cpp`. For other `test_utils_*.cpp` files,
the rule forms target names as
`module_utils_<source-stem>_autofocused` and CTest names as
`<source-stem>_utils_AutoFocusedTests`.

## Run the registered utils suite

Build the registered targets, then select tests by their mandatory module
label:

```bash
cmake --build build-community-debug -j4
ctest --test-dir build-community-debug -L module:utils --output-on-failure
```

There is no aggregate `module_utils_test` target. Use CTest's listing mode to
see which utils tests the configured build actually registered:

```bash
ctest --test-dir build-community-debug -N -L module:utils
```

Some explicit CMake entries are guarded by `if(EXISTS ...)`, and the LZ4/Zstd
test compile definitions depend on detected libraries. An absent test or
optional-codec case in a build is therefore not evidence that it passed.

## Benchmark target

The utils benchmark directory registers `bench_utils_release_gates` when
`THEMIS_BUILD_BENCHMARKS` is enabled. Its cases cover configuration-object
construction/copy and error-enum dispatch; they do not measure the privacy,
codec, crypto, or concurrency workloads described in the historical Phase A.3
runner. See [`../../src/utils/PERFORMANCE_EXPECTATIONS.md`](../../src/utils/PERFORMANCE_EXPECTATIONS.md)
before interpreting benchmark output.
