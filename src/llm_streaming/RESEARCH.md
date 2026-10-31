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

The helper's atomic callback index does not establish ordered concurrent sink
delivery. The source reviewed does not establish session lifecycle,
backpressure, cancellation, gRPC, a server route, or runtime metrics.

## Open design questions

1. Which runtime component calls the helper, and what are the sink's ownership
   and thread-safety requirements?
2. Is the callback invoked concurrently? If so, where is delivery order
   serialized or otherwise specified?
3. Which component, if any, owns cancellation, disconnect, buffering, and
   cleanup?
4. Which integration tests exercise the actual helper and transport boundary?
5. Are the benchmark labels intended product requirements, or merely
   illustrative stub thresholds?

## Conclusion

The current evidence supports documentation of a small formatting utility and
separate stub scenarios. It does not support claims about a full streaming
service, deployment behavior, or release performance. The answers above remain
unverified until connected to canonical implementation and test evidence.

**TODO:** add external papers or measured research references only if a
specific design decision requires them; none is asserted here.
