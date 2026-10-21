---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Implementation Guidance: LLM Streaming

This document describes the implementation shape expected for the `llm_streaming` contract. It is not a statement that a production runtime already exists under `src/llm_streaming/`; instead, it explains how the design should be implemented in the canonical runtime modules without violating the repository’s source-traceability rules.

## Implementation objective

The implementation must produce a stream that is:

- observable
- deterministically ordered per session
- bounded under backpressure
- cancellable and cleanup-safe
- diagnosable by operators during incidents

## Canonical implementation boundaries

The repository’s current design documents point to runtime ownership in:

- `src/llm/`
- `src/server/`
- supporting orchestration layers such as `src/query/` and `src/rag/`

That ownership is important because a stream is fundamentally a cross-cutting concern: token generation, transport framing, and client-facing delivery are not all owned by the same code layer.

## Implementation pattern

The current, source-grounded implementation evidence in `src/llm/streaming_handler.cpp` suggests the following pattern:

1. Accept a token stream or callback from the model layer.
2. Format each token into a transport-safe payload.
3. Add request identity and sequence metadata for diagnostics.
4. Deliver the payload through the selected client protocol.
5. Emit an explicit terminal event when generation is complete or cancelled.

This pattern is intentionally minimal but operationally useful.

## Core implementation rules

### 1. Single ownership of stream state

Each stream must have a single, explicit owner for lifecycle transitions and cleanup. That owner must be responsible for:

- open/active/cancelled/closed transitions
- cleanup callback registration
- final error propagation
- termination telemetry

### 2. Sequence-aware delivery

If the transport or runtime can reorder chunks, the implementation must track the sequence number and detect a violation immediately. Reordering is never silently accepted under the operational contract.

### 3. Fail-closed overflow handling

When a queue or sink reaches a threshold, the runtime must not drop data without an operator-visible signal. The implementation should return a clear backpressure or overflow status and then clean up or isolate the stream.

### 4. Cancellation and disconnect invariants

The implementation must treat cancellation and disconnect as events that require cleanup idempotence. The same stream identifier should remain traceable across the failure boundary.

## Edge cases that must be handled

- empty token set before finish reason
- stream closes before first token is emitted
- cancellation arrives just before completion
- disconnected client while buffer still has queued output
- concurrent streams with overlapping backpressure and flush paths
- transport encoding failures on special characters or binary-like data

## Required test alignment

The current validation evidence is in:

- `tests/integration/test_llm_streaming_soak.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`

These should be treated as the minimum design proof until canonical runtime tests are added in the owning modules. The implementation should satisfy the same invariants they assert: throughput, no order violations, no uncaught exceptions, and bounded backpressure behavior.

## Implementation acceptance

The implementation is ready to claim production status only when these conditions are true:

- runtime ownership is documented in canonical module files
- stream lifecycle and ordering invariants are tested in the owning layer
- backpressure and disconnect behavior are validated under load
- operator diagnostics remain explicit and actionable

Until then, the `llm_streaming` module should remain a source-grounded design and governance layer rather than a runtime claim.
