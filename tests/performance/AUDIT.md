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

# Audit — Performance Test Registration

> The initial static findings below were recorded on 2026-10-31, before the
> recursive registration change. Current registration behavior is summarized
> separately below.

## Static checks

- The parent test CMake file auto-discovers the module because
  `tests/performance/CMakeLists.txt` exists.
- The module file registers root-level `test_*.cpp` sources and recursively
  includes `phase3/` and `phase4/` test sources.
- The `test_performance_*.cpp` sources are selected by both globs and receive
  separate `_focused` and `_autofocused` executable/CTest registrations.
- Nested tests use a `nested_` prefix and a stable hash of their relative path
  in target and CTest names to avoid collisions.
- Registration is conditional on tests being enabled and GoogleTest being
  available in the configured build.

## Limits

An isolated CMake harness confirmed 31 CTest registrations, including nested
sources. It did not compile or execute test binaries; test pass/fail status and
build-specific availability in a production configuration remain unknown.
See [`ROADMAP.md`](ROADMAP.md) for remaining work.

Exact model backend not verifiable from final document text alone.
