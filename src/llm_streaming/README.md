---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# LLM Streaming Documentation Module

## Current status

`src/llm_streaming/` is a documentation-only directory (plus `.gitkeep`), not
a runtime source module. The source-backed helper is declared in
`include/llm/streaming_handler.h` and implemented in
`src/llm/streaming_handler.cpp`. It formats SSE event strings and HTTP chunk
framing and adapts a token callback to a caller-provided string sink.

The checked sources do not establish an end-to-end server/LLM streaming path,
HTTP or gRPC integration, session lifecycle, cancellation, production
backpressure, or runtime metrics. Do not treat requirements elsewhere in this
directory as delivered behavior.

## Evidence paths

| Path | Verified role |
|---|---|
| `include/llm/streaming_handler.h` | Public `themis::llm::StreamingHandler` declarations |
| `src/llm/streaming_handler.cpp` | String formatting and callback adapter implementation |
| `tests/llm/test_streaming_handler.cpp` | Direct helper unit tests (not runtime integration) |
| `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp` | In-process stub stress tests |
| `tests/integration/test_llm_streaming_soak.cpp` | In-process stub soak tests |
| `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` | In-process stub benchmark |

The direct unit test covers helper behavior, not a server/transport path. The
stress/soak/benchmark files do not call `StreamingHandler`; their outcomes are
not runtime integration or performance evidence.

## Build and validation references

CMake registers the helper unit test (when `THEMIS_ENABLE_LLM` is enabled) as
target `test_llm_streaming_handler` and CTest name
`LlmStreamingHandlerFocusedTests`. It registers the module focused stub test as target
`module_llm_streaming_test_llm_streaming_highcardinality_stress_focused`
(CTest name
`test_llm_streaming_highcardinality_stress_LLMStreamingFocusedTests`). The
integration soak is registered as `test_llm_streaming_soak`; its CTest
environment sets `THEMIS_SOAK_DURATION_MS=5000` and a 120-second timeout,
while the test implementation defaults to 60,000 ms if unset. Stress and soak
are stub-based, distinct from the direct helper unit test. The benchmark
target is `bench_llm_streaming_dedicated_gates` and is conditional on benchmark
builds. The module test CMake globs `test_*.cpp` and registers with
`themis_register_module_focused_test`, stress tier, and a 300-second timeout;
its target links test libraries and `Threads::Threads`, not the `llm`
implementation. The benchmark CMake is opt-in via `THEMIS_BUILD_BENCHMARKS`,
globs `bench_*.cpp`, and calls `themis_add_standard_benchmark`; benchmark source
labels are not enforced thresholds or gates. The helper is included in the LLM source list in
`cmake/ModularBuild.cmake`. See the corresponding CMake files for the
repository's normal build procedure. No tests or benchmarks were run for this
documentation update.

## Reading next

- `API.md` — helper signatures and exact behavior
- `ARCHITECTURE.md` — implementation boundary and design topics
- `ROADMAP.md` — planned work and current limitations
- `PRODUCTION_REQUIREMENTS.md` — proposed acceptance evidence

**TODO:** document a runtime owner, caller, and end-to-end integration if source
and test evidence establish them.
