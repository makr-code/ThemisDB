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

# Audit — Performance Test Registration

## Static checks

- The parent test CMake file auto-discovers the module because
  `tests/performance/CMakeLists.txt` exists.
- The module file registers root-level `test_*.cpp` sources through two
  filename-based globs.
- The `test_performance_*.cpp` sources are selected by both globs and receive
  separate `_focused` and `_autofocused` executable/CTest registrations.
- The nested `phase3/` and `phase4/` source files are not included by those
  globs; no nested CMake file was found.
- Registration is conditional on tests being enabled and GoogleTest being
  available in the configured build.

## Limits

This is a static CMake inspection only. No target was configured, built, or
executed here; test pass/fail status, runtime coverage, and build-specific
availability are unknown. See the follow-up in [`ROADMAP.md`](ROADMAP.md).

Exact model backend not verifiable from final document text alone.
