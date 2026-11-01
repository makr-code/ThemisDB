---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "build-test"
Canonical_References: "tests/timeseries/CMakeLists.txt; tests/timeseries/test_*.cpp; tests/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries focused tests

This directory contains focused C++ tests for timeseries components. Current
test sources are:

- `test_continuous_agg_materialization.cpp`
- `test_timeseries_contract_hardening_focused.cpp`
- `test_timeseries_error_path_phase3.cpp`
- `test_timeseries_highcardinality_stress.cpp`
- `test_timeseries_metrics.cpp`
- `test_timeseries_q4_hardening_focused.cpp`
- `test_timeseries_retention.cpp`

The primary loop in `CMakeLists.txt` discovers each `test_*.cpp` file and
registers a focused unit-test target through
`themis_register_module_focused_test()` with a 120-second timeout. For a source
named `test_<stem>.cpp`, the target is `module_timeseries_<stem>_focused` and
the CTest name is `<stem>_TimeseriesFocusedTests`.

The file also has a separate autogen loop for `test_timeseries_*.cpp`. It
computes distinct `_autofocused` target and `*_timeseries_AutoFocusedTests`
names. Its `if(TARGET ${_target})` guard checks for that `_autofocused` target;
it does not match the primary loop's `_focused` target. Thus, for sources
matching both globs, the primary focused registration does not by itself cause
the autogen loop to skip. The autogen target is skipped only if that exact
`_autofocused` target has already been declared elsewhere.

Example commands, after the repository has been configured. Use the same
configured `<build-dir>` for building the target and selecting its registered
CTest case:

```bash
cmake --build <build-dir> --target module_timeseries_test_continuous_agg_materialization_focused
ctest --test-dir <build-dir> -R test_continuous_agg_materialization_TimeseriesFocusedTests
```

These are registration/build instructions, not evidence that the tests passed.
CTest selects the registered test by its CTest name within the chosen build
tree. Additional storage-backed timeseries tests are registered elsewhere in
`tests/`; this directory is not the complete repository-wide timeseries test
inventory. Consult the corresponding source and CMake registration when
assessing coverage. No test execution is asserted by this document review.
