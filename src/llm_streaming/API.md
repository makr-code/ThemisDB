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

# API Contract: LLM Streaming

This page records the source-backed formatting-helper API in
`include/llm/streaming_handler.h` and identifies the separate inference/HTTP
streaming interfaces. `src/llm_streaming/` contains no runtime implementation;
the helper is not the implementation used by the server route.

## Public helper surface

Namespace: `themis::llm`; class: `StreamingHandler`.

### `formatSseEvent(token, request_id, index, done = false)`

Returns an SSE `data:` block containing a JSON object with the `id`, `token`,
`index`, and `done` fields, followed by two newline characters. String values
are escaped for JSON, including control characters. The caller supplies the
index; this function does not maintain sequence state.

### `formatDoneEvent(request_id)`

Returns `data: [DONE]\n\n`. The request ID is used for a debug log message and
is not included in the returned wire string.

### `formatChunkedData(data)`

Returns one HTTP/1.1 chunk-framing string: hexadecimal byte length, CRLF, data,
then CRLF. An empty input formats a zero-length chunk. The helper does not own
an HTTP connection or manage a complete response.

### `makeStreamCallback(sink, request_id)`

Returns a callback that assigns a zero-based index using a shared
`std::atomic<size_t>` counter with `std::memory_order_relaxed`, then calls
`sink` with an SSE event having `done=false`. An empty `sink` causes
`std::invalid_argument`. Copies of the returned callback share the counter.
This helper does not emit a final `[DONE]` event automatically. Atomic index
allocation does not serialize sink calls or guarantee concurrent calls reach
the sink in index order; sink thread safety and transport behavior are caller
responsibilities.

## Explicitly not established by this API

No public API is present here for opening/closing sessions, cancellation,
backpressure, stream registries, gRPC, error codes, or lifecycle state
transitions. The separate `EmbeddedLLM` API declares
`generateStreaming(prompt, callback, max_tokens=512)` and
`generateStreamingSSE(prompt, callback, request_id="", max_tokens=512)` in
`include/llm/embedded_llm.h`. They synchronously return generated text and
accept callbacks; the SSE method formats callback values, but does not manage a
network transport.

`GET /api/v1/llm/stream` is implemented in
`LLMApiHandler::handleStreamInference`. It accepts `prompt`, optional
`request_id`, and optional `max_tokens` query parameters (default 512, maximum
4096). The handler accumulates the complete SSE body before returning an HTTP
`string_body` response, so this endpoint does not provide incremental network
delivery. The route-level lifecycle, disconnect cancellation, backpressure, and
route integration test coverage remain unverified. The module stress/soak tests
are stubs and do not test this route.

## Source references

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `include/llm/embedded_llm.h`
- `src/llm/embedded_llm.cpp`
- `src/server/llm_api_handler.cpp`
- Direct helper unit tests: `tests/llm/test_streaming_handler.cpp`
- Validation stubs: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
  and `tests/integration/test_llm_streaming_soak.cpp`
