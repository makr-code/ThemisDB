---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Module Guide: LLM Streaming

## Reading order

1. `README.md` — current scope and evidence boundary
2. `API.md` — public helper surface
3. `ARCHITECTURE.md` — helper flow and design topics
4. `ROADMAP.md` and `PRODUCTION_REQUIREMENTS.md` — planned work and acceptance
5. `IMPLEMENTATION.md`, `MODELING.md`, `OPERATIONS.md`, `RUNBOOK.md`,
   `SECURITY.md`, and `TRUST.md` — bounded evidence and unresolved assumptions

## Current ownership and paths

- `src/llm_streaming/` contains documentation and `.gitkeep`, not runtime code.
- The source-backed helper is in `src/llm/streaming_handler.cpp`, declared in
  `include/llm/streaming_handler.h`.
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp` and
  `tests/integration/test_llm_streaming_soak.cpp` use in-process stubs.
- `tests/llm/test_streaming_handler.cpp` directly tests the formatting helper
  and callback adapter, but not transport integration.
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` benchmarks
  stubs rather than the helper.

The checked evidence does not establish a `src/server/` integration path or a
header under `include/llm_streaming/`.

## Build/test registration

The helper unit test is registered in `tests/CMakeLists.txt` (when
`THEMIS_ENABLE_LLM` is enabled) as target `test_llm_streaming_handler` and
CTest name `LlmStreamingHandlerFocusedTests`. The focused stress test is
registered by `tests/llm_streaming/CMakeLists.txt`
under target
`module_llm_streaming_test_llm_streaming_highcardinality_stress_focused` and
CTest name `test_llm_streaming_highcardinality_stress_LLMStreamingFocusedTests`.
The module CMake file globs `test_*.cpp`, registers via
`themis_register_module_focused_test`, and assigns stress tier/300-second
timeout; its target links test libraries and Threads, not the `llm`
implementation. Benchmark registration is opt-in via
`THEMIS_BUILD_BENCHMARKS`; the module file globs `bench_*.cpp` and calls
`themis_add_standard_benchmark`.
The integration soak is listed in `tests/integration/CMakeLists.txt` as
`test_llm_streaming_soak`; CTest sets `THEMIS_SOAK_DURATION_MS=5000`, while the
test implementation defaults to 60,000 ms (CTest timeout 120 seconds). It is
stub-based, distinct from the direct source-helper unit test. The benchmark
target is `bench_llm_streaming_dedicated_gates` and is conditional on benchmark
builds.
See those CMake files for the exact project build workflow.

## How to assess future readiness

Before making a runtime or release claim, identify the production caller and
owner, extend direct helper coverage where needed, test any transport
integration, and verify cancellation, ordering, backpressure, configuration,
and observability in that runtime. The existing stubs alone do not satisfy
these criteria.

**TODO:** update this map when a concrete runtime integration is found and
covered by tests.
