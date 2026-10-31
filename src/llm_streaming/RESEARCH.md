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

# Research Notes: LLM Streaming

This page records source observations and open design questions; it does not
claim a complete runtime or an external research review.

## Source-grounded observations

- `include/llm/streaming_handler.h` declares `themis::llm::StreamingHandler`.
- `src/llm/streaming_handler.cpp` escapes JSON string values, formats SSE
  events, the `data: [DONE]\n\n` sentinel, one HTTP chunk-framing string, and a
  callback adapter that invokes a caller-provided sink.
- `tests/llm/test_streaming_handler.cpp` directly tests formatting and basic
  sequential callback behavior, but not transport/runtime integration.
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`,
  `tests/integration/test_llm_streaming_soak.cpp`, and
  `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` define
  separate in-process stubs. They do not use the helper or real network/model
  backends.
- `GET /api/v1/llm/stream` in `src/server/llm_api_handler.cpp` invokes
  `EmbeddedLLM::generateStreamingSSE`, which formats inference callback values
  as SSE and accumulates them into a string response body.
- The route returns `http::string_body` after generation completes; this is
  not incremental network delivery and it does not call `StreamingHandler`.
- The route parses `prompt`, `request_id`, and `max_tokens`; the default token
  limit is 512 and accepted query values are capped at 4096.

The helper's atomic callback index does not establish ordered concurrent sink
delivery. The source reviewed does not establish session lifecycle,
backpressure, disconnect cancellation, gRPC, or streaming metrics.

## Open design questions

1. Is buffered SSE sufficient, or must the route write events incrementally?
2. Which route-level tests verify the actual inference-to-response path?
3. Which component should own disconnect cancellation, backpressure, and
   cleanup if incremental delivery is required?
4. Is the helper callback invoked concurrently, and what sink ordering contract
   should callers observe?
5. Are the benchmark labels intended product requirements, or merely
   illustrative stub thresholds?

## Conclusion

The current evidence supports a formatting utility, a buffered HTTP/SSE
inference route, and separate stub scenarios. It does not support claims about
incremental network delivery, cancellation/backpressure guarantees, deployment
behavior, or release performance. The open questions require canonical
implementation and test evidence.

**TODO:** add external papers or measured research references only if a
specific design decision requires them; none is asserted here.
