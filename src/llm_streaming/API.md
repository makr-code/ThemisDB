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

This page records the small, source-backed formatting-helper API in
`include/llm/streaming_handler.h`. It is not a contract for a complete streaming
service: `src/llm_streaming/` contains no runtime implementation, and this
header does not establish server, network, cancellation, or backpressure APIs.

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
transitions. Those remain design topics unless implemented and tested in an
owning runtime path. The current stub tests are not proof of those behaviors.

## Source references

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- Direct helper unit tests: `tests/llm/test_streaming_handler.cpp`
- Validation stubs: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
  and `tests/integration/test_llm_streaming_soak.cpp`
