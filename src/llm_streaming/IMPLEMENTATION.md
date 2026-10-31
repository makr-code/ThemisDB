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

# Implementation Notes: LLM Streaming

## Implemented helper

The implementation checked for this update is `src/llm/streaming_handler.cpp`,
with declarations in `include/llm/streaming_handler.h`. It provides:

1. JSON escaping for string values used by `formatSseEvent`.
2. SSE event string formatting with caller-supplied `token`, `request_id`,
   `index`, and `done`.
3. Formatting of the `data: [DONE]\n\n` sentinel.
4. Formatting of one HTTP chunk-framing string.
5. A callback adapter that uses a shared atomic counter, formats each token as
   an SSE event with `done=false`, and invokes a supplied sink. The counter
   uses `std::memory_order_relaxed`; a null/empty sink throws
   `std::invalid_argument`.

The implementation does not open a stream, invoke inference, provide a server
transport, emit a final event automatically, or implement cancellation,
backpressure, retries, or session cleanup. Concurrent sink invocation ordering
and sink thread safety are not guaranteed by the atomic counter.

## Tests and benchmark: scope

- `tests/llm/test_streaming_handler.cpp` directly tests SSE formatting and
  escaping, the done sentinel, chunk formatting, sequential callback delivery
  and indexes, request-ID propagation, and empty-sink rejection. It does not
  test runtime transport integration, cancellation, backpressure, concurrent
  sink ordering, or callback-copy behavior.
- The CMake registration description mentions cancellation/backpressure, but
  those behaviors are not exercised by this test source.
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp` tests
  locally defined session, queue, and sequence-checking stubs.
- `tests/integration/test_llm_streaming_soak.cpp` tests its own local token,
  queue, and assembler stubs. `THEMIS_SOAK_DURATION_MS` controls duration;
  its implementation default is 60,000 ms. `tests/integration/CMakeLists.txt`
  overrides this CTest target to 5,000 ms. Its file comment also mentions a
  3,600,000 ms release run, but that is not the code default and no such
  pipeline setting was verified. This is stub validation, not runtime evidence.
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` benchmarks
  separate stubs. Its labels/values are not checked thresholds, executable
  gates, or evidence of production performance.

The three stub-based files do not call `StreamingHandler` and are not
integration tests for this implementation.

## Build/test registration references

CMake registers the stress executable as
`module_llm_streaming_test_llm_streaming_highcardinality_stress_focused` with
CTest name `test_llm_streaming_highcardinality_stress_LLMStreamingFocusedTests`.
`tests/llm_streaming/CMakeLists.txt` globs `test_*.cpp` and registers these
module fixtures through `themis_register_module_focused_test` with tier
`stress` and timeout 300 seconds. That target links `${TEST_LIBS}` and
`Threads::Threads`, not the `llm` implementation.
`tests/CMakeLists.txt` explicitly registers the source-helper unit test (when
`THEMIS_ENABLE_LLM` is enabled) as target `test_llm_streaming_handler` and CTest
name `LlmStreamingHandlerFocusedTests`. This is distinct from the stub-only
module stress test and stub-only integration soak. The integration soak CTest
is `test_llm_streaming_soak`; its CMake environment override is
`THEMIS_SOAK_DURATION_MS=5000` with a 120-second CTest timeout. The benchmark is
registered as `bench_llm_streaming_dedicated_gates` only when
`THEMIS_BUILD_BENCHMARKS` is enabled. Its CMake file globs `bench_*.cpp` and
passes sources to `themis_add_standard_benchmark`; benchmark labels in the
source are not enforced thresholds.
The helper implementation is included in the LLM source list in
`cmake/ModularBuild.cmake`; there is no separate runtime target for
`src/llm_streaming/`. Consult the repository's normal configure/build workflow
for invocation; validation was not run as part of this documentation update.

**TODO:** extend helper coverage for callback copies and concurrent invocation,
then identify the runtime caller and the owner for any broader streaming
lifecycle.
