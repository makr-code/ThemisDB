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

# LLM Streaming — Architecture and Design Boundary

## Current source reality

`src/llm_streaming/` is documentation-only apart from `.gitkeep`. The
source-backed helper is `themis::llm::StreamingHandler` in
`include/llm/streaming_handler.h` and `src/llm/streaming_handler.cpp`. It
formats SSE event strings and HTTP chunk-framing strings and can adapt a token
callback to an SSE-formatted sink call. This is not evidence of an end-to-end
streaming service, server integration, or production lifecycle management.

`tests/llm/test_streaming_handler.cpp` directly tests the formatting helper and
callback adapter. Separately, the stress/soak files in
`tests/llm_streaming/` and `tests/integration/test_llm_streaming_soak.cpp`,
plus the benchmark in `benchmarks/llm_streaming/`, use local stubs; those
fixtures do not call the helper or a network/model backend.

## Concrete helper flow

```text
caller token
    -> StreamingHandler::makeStreamCallback
    -> formatSseEvent(token, request_id, generated_index, false)
    -> caller-provided sink
```

The helper's atomic counter allocates increasing indices across copies of that
callback. It does not serialize sink calls, so concurrent calls are not proven
to arrive in index order. `formatSseEvent` can also be called directly with a
caller-provided index and `done` flag. `formatDoneEvent` and
`formatChunkedData` are independent formatting helpers; no orchestration
between them is implemented here.

## Design topics, not implemented guarantees

Lifecycle ownership, cancellation and disconnect propagation, bounded
backpressure, stream registries, ordering at the transport, retries, metrics,
and operator recovery are design requirements only. The repository evidence
examined for this page does not define their runtime APIs, error-code taxonomy,
timeouts, or production configuration. Do not treat the state names or
failure-handling guidance elsewhere in this directory as delivered behavior.

## Validation boundary

- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp` models
  session counters, a bounded queue, and sequence checking with in-process
  stubs. Its module CMake target links test libraries and Threads, not the `llm`
  implementation.
- `tests/integration/test_llm_streaming_soak.cpp` also uses in-process stubs;
  its implementation default is 60,000 ms, while CTest sets
  `THEMIS_SOAK_DURATION_MS=5000` for this target.
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` measures
  stub operations. Values/labels in that file are not executable gates or
  evidence that runtime performance targets have passed.

These are scaffolding/evidence for the modeled operations, not production
streaming or transport validation.

## Source references

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `cmake/ModularBuild.cmake`
- `tests/llm/test_streaming_handler.cpp`
- `tests/llm_streaming/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/integration/CMakeLists.txt`
- `benchmarks/llm_streaming/CMakeLists.txt`

**TODO / assumption:** identify and document the runtime owner and any actual
server-to-sink integration when source and integration tests establish them.
