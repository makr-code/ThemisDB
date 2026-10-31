---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Geo test architecture

## Context and ownership

`tests/geo/` contains test sources for geo
public interfaces and concrete implementations. Test targets are generated
from `test_*.cpp` by
`tests/geo/CMakeLists.txt`; the tests link
to `themis_core` and selected geo support sources.

## Test flow and boundaries

Each source is registered as a focused test target and CTest entry. Sources
with the `test_geo_` prefix are also considered by the CMake autogen-prefix
loop. The contract hardening suite covers stated contract fixtures with mocks;
other suites exercise concrete implementations or integrations, subject to
their compile-time dependencies and runtime environment.

The test directory does not own production authorization, persistent storage,
or device availability. A passing mock test does not establish production
behavior, and a test configured with CPU fallback does not establish GPU
execution.

## Failure domains and non-goals

- Build/configuration failures may arise from the parent build or external test
  dependencies; this directory does not define a standalone build preset.
- CUDA/HIP tests need the corresponding build and runtime/device conditions for
  GPU-path evidence.
- The test source set is not a security certification or proof of universal
  contract enforcement.

## Canonical references

- Target and CTest registration:
  `tests/geo/CMakeLists.txt`.
- Public API contract:
  `include/geo/geo_api_contract.h`.
- Geo module implementation and focused validation guidance:
  `src/geo/IMPLEMENTATION.md`.
