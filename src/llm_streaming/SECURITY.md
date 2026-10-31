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

# Security Notes: LLM Streaming

## Verified implementation behavior

`src/llm/streaming_handler.cpp` escapes backslash, double quote, common JSON
control characters, and other bytes below `0x20` when building the `token`
and `id` JSON string values in `formatSseEvent`. It emits a structured SSE
`data:` line. `formatDoneEvent` logs the request ID at debug level and returns
the fixed `data: [DONE]\n\n` string. `makeStreamCallback` rejects an empty sink
by throwing `std::invalid_argument`.

These are narrow formatting/input checks. They do not establish authentication,
authorization, request-ID validation, payload size limits, rate limits,
transport security, client isolation, or a complete defense against
application-level injection. The caller owns transport and sink behavior.

## Unverified trust boundaries

The helper interface receives a token, request ID, and callback sink. The
checked code does not show who supplies or validates them, whether a network
boundary exists, or how diagnostics are protected. Do not assume HTTP/gRPC
integration, lifecycle checks, cancellation, or backpressure controls.

## Security review requirements for future integration

- Validate or constrain request identifiers and sensitive text before logging.
- Review maximum token/event sizes and the caller's buffering behavior.
- Verify that the actual transport applies the intended authentication and
  authorization policies.
- Exercise JSON escaping and error behavior through direct tests and any real
  transport integration.
- Document concurrency and sink ownership before concurrent use.

`tests/llm/test_streaming_handler.cpp` checks selected escaping and helper
behavior, but does not validate transport security or a production path. The
stress/soak fixtures and benchmark use local stubs and provide no production
security validation.

## Sources

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
