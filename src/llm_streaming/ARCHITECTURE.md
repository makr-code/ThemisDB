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
`include/llm/streaming_handler.h` and `src/llm/streaming_handler.cpp`; it
formats SSE event strings and HTTP chunk-framing strings and adapts a token
callback to a caller-provided sink. Separately, an HTTP/SSE inference route is
implemented in `src/server/llm_api_handler.cpp`. It calls
`EmbeddedLLM::generateStreamingSSE`, formats callback values through
`LlamaWrapper::formatStreamTokenAsSSE`, accumulates the events in a string, and
returns a `http::string_body` response after generation. The route does not use
`StreamingHandler` and does not send each event to the network as it is
generated.

`tests/llm/test_streaming_handler.cpp` directly tests the formatting helper and
callback adapter. Separately, the stress/soak files in
`tests/llm_streaming/` and `tests/integration/test_llm_streaming_soak.cpp`,
plus the benchmark in `benchmarks/llm_streaming/`, use local stubs; those
fixtures do not call the helper or a network/model backend.

## Concrete helper and runtime flows

```text
caller token
    -> StreamingHandler::makeStreamCallback
    -> formatSseEvent(token, request_id, generated_index, false)
    -> caller-provided sink
```

The HTTP route has a separate flow:

```text
GET /api/v1/llm/stream
    -> LLMApiHandler::handleStreamInference
    -> EmbeddedLLM::generateStreamingSSE
    -> EmbeddedLLM::generateStreaming / generateFull
    -> inference backend stream_callback
    -> SSE formatter -> in-memory response string
    -> HTTP string_body response (after generation completes)
```

With a loaded Llama backend, token callbacks can occur during generation. The
server handler nevertheless buffers their SSE events until the complete
`string_body` response is returned. The non-LLM stub implementation is not
token-by-token: it generates once and emits a terminal callback event.

The helper's atomic counter allocates increasing indices across copies of that
callback. It does not serialize sink calls, so concurrent calls are not proven
to arrive in index order. `formatSseEvent` can also be called directly with a
caller-provided index and `done` flag. `formatDoneEvent` and
`formatChunkedData` are independent formatting helpers; no orchestration
between them is implemented here.

## Design topics, not implemented guarantees

The HTTP route has no verified per-stream cancellation or disconnect
propagation, backpressure, retry, or stream registry. Its `max_tokens` query
parameter defaults to 512 and accepts values through 4096; no dedicated
streaming configuration keys or streaming metrics were found. The broader
helper lifecycle, concurrent sink ordering, and operator recovery remain
unestablished. Do not treat the state names or failure-handling guidance
elsewhere in this directory as delivered behavior.

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
- `include/llm/embedded_llm.h`
- `src/llm/embedded_llm.cpp`
- `src/server/llm_api_handler.cpp`
- `src/llm/embedded_llm_stub.cpp`
- `cmake/ModularBuild.cmake`
- `tests/llm/test_streaming_handler.cpp`
- `tests/llm_streaming/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/integration/CMakeLists.txt`
- `benchmarks/llm_streaming/CMakeLists.txt`

**TODO / assumption:** verify the HTTP handler's buffering and failure behavior
with route-level tests, then decide whether incremental transport writes and
disconnect cancellation are required.
