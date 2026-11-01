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

These are narrow formatting/input checks. The route is protected by the LLM
handler's bearer-token validation and accepts prompt/request data in URL query
parameters; this does not establish request-ID validation, payload size limits,
rate limits, transport encryption, client isolation, or a complete defense
against application-level injection. The handler buffers the full SSE response,
and the caller/HTTP server owns transport behavior.

The separate OpenAI-compatible `POST /v1/chat/completions` path uses API-key
validation via `PolicyEngine`, not the JWT bearer-token gate. It also buffers
SSE output. No route-level evidence was found for request-ID validation,
disconnect cancellation, or bounded response-buffer limits.

## Unverified trust boundaries

The helper interface receives a token, request ID, and callback sink, but the
server's HTTP/SSE path uses `EmbeddedLLM` and `LlamaWrapper` directly rather than
this helper. The route validates bearer authentication before dispatch; no
route-level evidence was found for request-ID validation, cancellation,
backpressure controls, or diagnostic redaction.

## Security review requirements for future integration

- Validate or constrain request identifiers and sensitive text before logging.
- Review maximum token/event sizes and the caller's buffering behavior.
- Review prompt and request-ID exposure through the query string and logs.
- Verify that the actual transport applies the intended authentication and
  authorization policies.
- Exercise JSON escaping and error behavior through direct tests and any real
  transport integration.
- Document concurrency and sink ownership before concurrent use.

`tests/llm/test_streaming_handler.cpp` checks selected escaping and helper
behavior. `tests/llm/test_llm_aql_explain_stream_api.cpp` checks unauthenticated
route behavior but not successful authenticated delivery or transport security.
The stress/soak fixtures and benchmark use local stubs and provide no production
security validation.

## Sources

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `src/server/llm_api_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
