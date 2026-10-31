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

Exact model backend not verifiable from final document text alone.

# Analytics focused tests

This directory contains GoogleTest sources for analytics behavior. It tests the
public interfaces in [`../../include/analytics/`](../../include/analytics/)
and does not define runtime APIs.

## CMake registration

[`CMakeLists.txt`](./CMakeLists.txt) discovers each top-level `test_*.cpp` and
creates one executable named
`module_analytics_<source-stem>_focused`. It registers a CTest case named
`<source-stem>_AnalyticsFocusedTests` with the `AnalyticsFocusedTests` label,
unit tier, and a 120-second timeout. CMake may add implementation sources to a
test target where required; consult the target definition for exact linkage.

For example, `test_analytics_memory_pool.cpp` produces
`module_analytics_test_analytics_memory_pool_focused`.

## Build and run

Configure a build with tests enabled (and the repository's required
dependencies), then build a focused target and run its CTest case:

```bash
cmake -S . -B build-analytics-tests -DTHEMIS_BUILD_TESTS=ON
cmake --build build-analytics-tests --target module_analytics_test_analytics_memory_pool_focused
ctest --test-dir build-analytics-tests -R 'test_analytics_memory_pool_AnalyticsFocusedTests' --output-on-failure
```

List the registered analytics tests with:

```bash
ctest --test-dir build-analytics-tests -N -L AnalyticsFocusedTests
```

Use the test source and owning public header as the authority for fixtures,
assertions, and exact API behavior. The configured build determines which
optional integrations are available. For runtime contracts and recommended
validation sequence, see [`../../src/analytics/MODULE_GUIDE.md`](../../src/analytics/MODULE_GUIDE.md).
