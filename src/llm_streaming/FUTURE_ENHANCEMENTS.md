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

# LLM Streaming — Future Work

All items below are proposals, not commitments or descriptions of delivered
runtime behavior. The source-backed surface includes the helper in
`include/llm/streaming_handler.h` and `src/llm/streaming_handler.cpp`, plus a
separate buffered HTTP/SSE inference route at `GET /api/v1/llm/stream`.
`src/llm_streaming/` contains no runtime implementation.

## Scope

**Proposed / unverified:** decide whether the existing route's complete-body
buffering is acceptable or whether incremental transport delivery is required.
Product owner and delivery milestone are TODO.

## Design Constraints

**Proposed / unverified:** any future design should state ownership and sink
concurrency explicitly. The helper's relaxed atomic index does not serialize
sink calls or guarantee delivery order. Do not infer lifecycle, cancellation,
backpressure, disconnect handling, or incremental transport guarantees from
the current API. The existing route returns a `string_body` after generation.

## Required Interfaces

**Proposed / pending owner:** determine whether the owning runtime needs
session/lifecycle, cancellation, backpressure, incremental transport,
configuration, or diagnostic interfaces. No such interfaces are established by the current
`StreamingHandler` API; details remain TODO rather than specified contracts.

## Implementation Notes

**Proposed / unverified:** add route-level tests for current buffered response,
terminal/error behavior, and backend differences. If incremental writes are
required, implement only the behaviors approved for the server owner and
document them from source. No product owner or delivery milestone is verified.

## Test Strategy

The direct unit test in `tests/llm/test_streaming_handler.cpp` covers selected
helper behavior. The module stress and integration soak tests use separate
stubs. **Proposed:** extend helper coverage for callback copies/concurrency and
add end-to-end tests only if a real runtime path is established. These are
recommendations, not evidence that the broader tests exist or pass.

## Performance Targets

**TODO / pending:** no measurable production target, hardware profile, or
performance owner has been agreed or verified. The dedicated benchmark's
numeric labels are attached to stub operations only; they are not executable
gates, accepted thresholds, runtime measurements, or release baselines.
Confirm whether any values should become targets before adopting them.

## Security / Reliability

**Proposed / unverified:** if a runtime is implemented, define and test its
identity handling, sink concurrency, cancellation/disconnect behavior,
backpressure, diagnostics, and cleanup at the actual boundary. No production
security or reliability guarantees for these behaviors are established by the
current helper or stub tests.
