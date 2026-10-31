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

# Modeling Notes: LLM Streaming

This page separates the source-backed formatter behavior from proposed models
for a future end-to-end streaming runtime.

## Implemented behavior

`StreamingHandler` in `include/llm/streaming_handler.h` /
`src/llm/streaming_handler.cpp` formats SSE JSON events and a `[DONE]`
sentinel, formats one chunked-transfer string, and adapts a token callback to a
sink. Its shared atomic counter assigns indices to callback invocations.
Neither lifecycle state nor queue, ordering enforcement, or transport state is
managed by this helper. Concurrent invocation may call the sink out of index
order.

## Proposed runtime model (not established)

The states `open`, `active`, `cancelled`, and `closed`, plus per-stream
ownership, ordered delivery, cancellation/disconnect behavior, bounded
backpressure, and diagnostic retention, are design proposals in this
documentation set. No source-backed implementation or public API for these
concepts was identified in the checked header and implementation. Do not
describe these as current runtime guarantees.

## Validation boundary

`tests/llm/test_streaming_handler.cpp` directly tests the helper's formatting
and basic sequential callback behavior, but not a transport/runtime path.
Separately, the high-cardinality stress test, integration soak, and dedicated
benchmark define in-process stub classes. They count tokens, model a bounded
deque, or count/check synthetic chunks; they do not exercise inference,
networking, the public helper, or real stream ownership. The chunk-order checks
are serialized by test-side mutexes. Their results support only the specific
unit or stub scenarios described.

## Unverified items

- Runtime owner and production call path for `StreamingHandler`.
- Whether callback invocations can be concurrent and the sink's thread-safety
  contract.
- Whether any runtime performs backpressure, cancellation, retry, cleanup, or
  telemetry, and what its configuration and limits are.

These details require implementation and integration-test evidence before
being added to this model.
