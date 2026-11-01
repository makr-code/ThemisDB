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

# Performance Test Roadmap

## Current registration state

The module CMake file selects root-level `test_*.cpp` files and recursively
registers `phase3/` and `phase4/` sources with path-derived target/test names.
The `test_performance_*.cpp` root sources are still selected twice under
separate target names (`_focused` and `_autofocused`). No statement here asserts
that a test has passed.

## Follow-up

- [x] Register phase3/phase4 test sources through the module CMake file with
  path-derived names to avoid collisions. Registration was verified in an
  isolated CMake harness; compilation and execution remain unverified.
  (Completed: 2026-11-01)
- [ ] Resolve duplicate registrations for the root `test_performance_*.cpp`
  sources, or document why both targets are required.
  (Target: DOC-WEEKLY-2026-44)
- [ ] In a dependency-complete build, compile and execute the newly registered
  nested tests, then retain `ctest -N` and test-run evidence.
  (Target: Q1 2027)

## Validation boundary

The parent CMake file skips test configuration when `THEMIS_BUILD_TESTS` is
off, and returns if GoogleTest is unavailable. Actual registered tests depend
on the configured build. See [`README.md`](README.md) for target and test-name
patterns.

Exact model backend not verifiable from final document text alone.
