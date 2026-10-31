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

# Embedding the Streaming Helper in ThemisDB

## What is present

The concrete source evidence is the lightweight `themis::llm::StreamingHandler`
API in `include/llm/streaming_handler.h` and
`src/llm/streaming_handler.cpp`. A caller may use `makeStreamCallback` to wrap a
sink: each callback invocation formats a token as an SSE `data:` JSON event
with request ID and an atomically allocated index. Other static helpers format
a `[DONE]` sentinel or one HTTP chunk-framing string.

These are formatting and callback-adapter utilities. A separate path connects
model generation to HTTP/SSE: `LLMApiHandler::handleStreamInference` invokes
`EmbeddedLLM::generateStreamingSSE`, whose callback formats tokens using
`LlamaWrapper::formatStreamTokenAsSSE`. The server appends events to a
`std::string` and returns a `http::string_body` response only after generation
finishes. It does not use `StreamingHandler`, and this is not incremental
network delivery. No gRPC integration was verified.

## Example data shape

`formatSseEvent` returns a string shaped like:

```text
data: {"id":"request-123","token":"hello","index":0,"done":false}

```

The JSON string values are escaped by the implementation. The `index` is
supplied by the caller for direct formatting, or assigned by the callback
adapter. Atomic index allocation is not a guarantee of sink delivery order
under concurrent invocations.

## Integration boundary

The helper takes a `std::function<void(const std::string&)>` sink; its caller
owns that sink and its transport/threading contract. The HTTP route parses
`prompt`, `request_id`, and `max_tokens` from the query string; `max_tokens`
defaults to 512 and values above 4096 are ignored. It appends a terminal done
event on the normal path and an error event if generation throws. The route
does not expose per-stream cancellation, disconnect propagation, or bounded
backpressure, and no stream-specific configuration keys were verified.

`tests/llm/test_streaming_handler.cpp` directly covers helper formatting,
callback delivery/indexing, and empty-sink rejection. The stress/soak tests and
dedicated benchmark use separate in-process stubs and do not exercise the HTTP
route. The route exists in source, but route-level integration test coverage was
not identified; broader cancellation, backpressure, and network-flush behavior
remain unverified.

**TODO / assumption:** add route-level tests and decide whether the current
buffered response satisfies the required transport behavior.

## Sources

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `include/llm/embedded_llm.h`
- `src/llm/embedded_llm.cpp`
- `src/llm/embedded_llm_stub.cpp`
- `src/server/llm_api_handler.cpp`
- `tests/llm/test_streaming_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
