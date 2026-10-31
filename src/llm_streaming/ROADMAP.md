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

# LLM Streaming Module Roadmap

## Current Status

`src/llm_streaming/` is docs-only. `StreamingHandler` is a formatting and
callback helper in `src/llm/streaming_handler.cpp`, declared in
`include/llm/streaming_handler.h`. A separate production HTTP/SSE path exists:
`GET /api/v1/llm/stream` calls `EmbeddedLLM::generateStreamingSSE`, buffers
events in memory, and returns a `string_body` response after generation. It
does not use `StreamingHandler` and does not provide incremental network
delivery. The OpenAI-compatible `POST /v1/chat/completions` path also buffers
SSE chunks when `stream: true`. The module stress test, integration soak, and
dedicated benchmark use in-process stubs and do not test either route.

- [~] Document the helper API and its limits.
- [x] Identify the HTTP route, inference callback, and buffered response path.
- [ ] Decide whether incremental network delivery is required and implement it
      only with explicit cancellation/backpressure semantics.
- [~] Maintain direct helper unit coverage; add route-level integration tests
      for the existing HTTP path.
- [ ] Establish verified runtime behavior and release evidence before making
      production-readiness claims.

### Source Reality Check (2026-10-31)

- `src/llm_streaming/` contains Markdown files and `.gitkeep`, not `.cpp` or
  `.h` implementation files.
- The helper supports SSE string formatting, a `[DONE]` string, HTTP chunk
  framing, and a sink callback adapter. It does not implement session
  lifecycle, cancellation, backpressure, or gRPC. The separate server route
  buffers callback events and returns the completed body.
- The stub tests model counters, a bounded queue, and synthetic sequence checks.
  Their metrics and outcomes are not evidence of runtime performance.
- `tests/llm/test_streaming_handler.cpp` tests the helper directly, but not
  end-to-end transport, cancellation, backpressure, or concurrent sink order.
- `tests/llm/test_llm_aql_explain_stream_api.cpp` checks unauthenticated route
  handling, but not successful authenticated streaming generation/delivery.
- The module-focused test CMake globs local `test_*.cpp` stubs, registers them
  at stress tier with a 300-second timeout, and does not link the `llm`
  implementation.

## In Progress / Planned

- [~] Maintain source-backed API documentation and explicitly mark unknowns.
- [x] Trace the server route and inference callback; the route does not call
      `StreamingHandler`.
- [ ] Determine whether the buffered HTTP response meets product requirements.
- [ ] Specify callback concurrency and whether the sink must preserve order.
- [ ] Decide whether lifecycle, cancellation, disconnect, backpressure, and
      diagnostics are in scope for the owning runtime.
- [x] Basic unit tests exercise the helper's public formatting/callback API.
- [ ] Add tests for callback-copy/concurrency semantics and any established
      end-to-end path.

## Implementation Phases

### Phase 1: API and ownership clarification

- [x] Inventory the public `StreamingHandler` declarations and implementation.
- [x] Record the route owner (`LLMApiHandler`) and inference boundary
      (`EmbeddedLLM`); helper ownership remains caller-provided.
- [ ] Confirm callback concurrency and sink contract.

### Phase 2: Transport behavior (planned)

- [x] Document the existing GET route, query parameters, inference callback,
      response buffering, and terminal/error events.
- [ ] Decide whether incremental network writes are required; if so, assign the
      transport lifecycle to `LLMApiHandler` and define cancellation/backpressure.
- [ ] Test the actual request-to-response path and backend/stub behavior.

### Phase 3: Reliability and security validation (planned)

- [ ] Test JSON/SSE formatting, sink errors, callback copies, and concurrent use.
- [ ] If runtime features exist, validate cancellation, disconnect,
      backpressure, cleanup, and delivery ordering at their actual boundary.
- [ ] Document configuration and observability only when implemented.

### Phase 4: Tests (planned)

- [x] Direct unit tests for `StreamingHandler` exist.
- [ ] Extend direct helper tests for callback copies and concurrent invocation.
- [ ] Add route integration coverage for the existing request-to-response path
      and any future incremental transport implementation.
- [ ] Keep stub stress and soak coverage clearly separate from helper/runtime
      evidence.

### Phase 5: Performance / Hardening (planned)

- [ ] Confirm whether benchmark source labels are intended requirements; they
      are currently stub labels, not executable gates or verified thresholds.
- [ ] Define measurable performance criteria and a hardware profile after
      product ownership and runtime behavior are established.
- [ ] Capture representative runtime measurements and hardening evidence before
      setting release thresholds.
- [ ] Validate any implemented concurrency, cancellation, backpressure, and
      cleanup behavior at its actual runtime boundary.

### Phase 6: Documentation & Acceptance (planned)

- [ ] Align public API and operational docs with verified runtime behavior.
- [x] Document the current HTTP route, its query parameters, and response mode.
- [ ] Document any future streaming-specific configuration only after it is
      implemented and tested.
- [ ] Require traceable implementation, tests, and operational evidence for
      any production claim.

## Production Readiness Checklist

- [x] Current HTTP route, inference caller, and buffered response are
      source-traceable.
- [ ] Public helper is directly tested; integration behavior is tested where
      applicable.
- [ ] Any lifecycle, cancellation, backpressure, and ordering guarantees are
      implemented and validated in the owning path.
- [ ] Runtime performance criteria and representative measurements are
      documented.
- [ ] Configuration, diagnostics, and operational procedures are verified.

## Known Issues & Limitations

1. `StreamingHandler` is not used by the server route; the route buffers its
   SSE body instead of delivering events incrementally.
2. The helper's atomic index does not serialize concurrent sink delivery.
3. Stress/soak fixtures and benchmark operate on local stubs, not the helper.
4. No stream-specific runtime configuration, disconnect cancellation,
   backpressure controller, streaming metrics, or deployment limits were
   established by the sources reviewed.
5. Values printed by the stub benchmark are labels, not executable gates,
   verified thresholds, release targets, or results.

## Breaking Changes

No runtime contract change is established by this documentation. Any future
API or runtime changes must be recorded against the canonical implementation
and its tests.
