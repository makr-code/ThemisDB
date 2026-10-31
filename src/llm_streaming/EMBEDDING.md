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

These are formatting and callback-adapter utilities. The source examined here
does not establish that HTTP/SSE responses are wired to a server, that gRPC is
supported, or that model generation invokes this callback in production.

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

The helper takes a `std::function<void(const std::string&)>` sink; the caller
owns that sink, its transport, threading contract, lifecycle, error handling,
and any final-event policy. No configuration keys, server route, gRPC writer,
stream session, cancellation API, or backpressure controller was found in the
header or implementation.

`tests/llm/test_streaming_handler.cpp` directly covers helper formatting,
callback delivery/indexing, and empty-sink rejection. The stress/soak tests and
dedicated benchmark use separate in-process stubs and do not exercise this
helper. No helper-to-transport integration test or owning runtime path was
identified, so the broader embedding model remains unverified.

**TODO / assumption:** link the actual caller and integration test when a
source-backed runtime integration is established.

## Sources

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `tests/llm/test_streaming_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
