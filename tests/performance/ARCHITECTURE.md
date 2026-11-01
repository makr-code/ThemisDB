---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
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

# Performance Test Architecture

The test directory is organized as standalone C++ test sources grouped by
component. Root-level sources are selected by two CMake globs in
`tests/performance/CMakeLists.txt`; each selected source becomes an executable
linked to `themis_core`, GoogleTest, spdlog, and threads, then is registered
through `themis_register_module_focused_test`.

The registration helper attaches module, tier, kind, and focus labels to the
CTest entry. The root `test_*.cpp` glob creates `_focused` targets. The
additional root `test_performance_*.cpp` glob creates `_autofocused` targets
with different target and test names; it does not skip the same source already
selected by the first glob.

The `phase3/` and `phase4/` directories contain test sources but no nested
`CMakeLists.txt`. Their source files are therefore not part of the current
performance module registration. Tests validate behavior only when built and
run in a configured environment; source presence alone is not test evidence.

Exact model backend not verifiable from final document text alone.
