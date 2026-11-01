---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-11-01"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "tests/performance/CMakeLists.txt"
  - "tests/CMakeLists.txt"
  - "tests/cmake/RegisterModuleTests.cmake"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Tests

## Coverage layout

The directory contains 14 root-level test sources and 12 nested sources under
`phase3/` and `phase4/`. The source files cover such components as cycle
metrics, allocator and feature controls, NUMA, workload prediction, hardware
dispatch, RaBitQ, DiskANN, per-query cost, PMEM, PMU counters, and io_uring.
Presence of a test source does not establish that it is configured or has
passed.

## CMake and CTest registration

`tests/CMakeLists.txt` auto-discovers `tests/performance/CMakeLists.txt` when
tests are enabled and GoogleTest is found. The performance module CMake file
uses root-level globs for the primary test set and recursive globs for
`phase3/` and `phase4/`:

- Each root `test_<name>.cpp` gets target
  `module_performance_test_<name>_focused` and CTest name
  `test_<name>_PerformanceFocusedTests`.
- Nested phase3/phase4 files are included in the focused set. Their target and
  CTest names include the source stem and a stable hash of the relative path so
  equal basenames cannot collide.
- A second glob creates `_autofocused` targets for root
  `test_performance_*.cpp` sources. Those have CTest names ending in
  `_performance_AutoFocusedTests`; they are distinct from the first targets,
  not deduplicated by source.

Configured dependencies and build options can change which targets are
available. Inspect the active build:

```bash
ctest --test-dir <build-dir> -N -L 'module:performance'
cmake --build <build-dir> --target module_performance_test_workload_predictor_focused
ctest --test-dir <build-dir> -R test_workload_predictor_PerformanceFocusedTests
```

The sample target is built only when that test source was registered in the
configured tree. Registration does not prove that a test target builds or
passes; the isolated CMake registration check confirmed 31 entries, but did not
compile or execute them.

Exact model backend not verifiable from final document text alone.
