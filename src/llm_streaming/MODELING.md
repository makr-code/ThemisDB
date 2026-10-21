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

# Modeling Notes: LLM Streaming

This document describes the state model, concurrency assumptions, and failure semantics for LLM streaming in ThemisDB. It is intentionally aligned with the repository’s current state: the module is a design contract and validation surface; canonical runtime code is owned elsewhere.

## Scope

The model covers:

- stream lifecycle
- token emission model
- backpressure and overflow handling
- cancellation and disconnect semantics
- telemetry and operator visibility

It does not claim that a complete runtime implementation already exists under `src/llm_streaming/`.

## State model

The architecture document defines the lifecycle as:

- `open`: request/session opened and associated with a stream identity
- `active`: token output or backpressure handling is in progress
- `cancelled`: termination requested by client or runtime, cleanup is active
- `closed`: terminal state, resources are released and diagnostics remain available

This state model is consistent with the operational evidence in the runbook and the stress tests. It serves as the canonical contract for future runtime implementation.

## Stream identity and ownership

A stream must carry a single authoritative identity. The repository’s guidance is that the owning runtime path must be explicit and documented. The current low-level implementation evidence is the `StreamingHandler` helper in `src/llm/streaming_handler.cpp`, which formats machine-readable payloads and sink callbacks with a request identifier.

The important modeling pattern is:

- upstream model produces tokens
- a transport adapter catches and serializes each token
- a sink or client observes `data:` blocks or chunked payloads
- termination is explicit and recorded in telemetry rather than inferred silently

## Ordering and delivery invariants

The minimum invariants are straightforward but operationally important:

1. Token order is preserved within a single stream unless the protocol explicitly defines a bounded reordering policy.
2. A per-stream sequence number is required for diagnosis and replay if ordering faults occur.
3. A stream may not silently skip, insert, or reorder tokens during emit/flush.
4. A cancelled or disconnected stream must produce a final terminal report or explicit error signal.

The soak and stress suites model exactly this behavior via `StubChunkAssembler` and sequence validation in `tests/integration/test_llm_streaming_soak.cpp`.

## Backpressure model

The repository’s design language treats backpressure as a bounded flow-control mechanism, not as implicit data loss. Relevant evidence includes:

- `docs/operability/RUNBOOK_LLM_STREAMING.md`: buffer overflow, generation stall, and backpressure cascade scenarios
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`: high-cardinality scenario naming and warnings
- `src/llm_streaming/ARCHITECTURE.md`: fail-closed semantics on overflow and cancellation

A simple operational model is:

- if the queue remains under threshold, stream continues
- if the queue approaches saturation, emit a bounded backpressure event
- if the queue exceeds threshold, fail closed and isolate the stream
- always emit explicit diagnosable telemetry

## Cancellation and disconnect semantics

The design requires that cancellation and disconnect are observably distinct:

- cancellation: explicit stop request from the caller or runtime gate
- disconnect: transport or connection loss before stream completion
- timeout: no progress for a bounded time while stream remains active

These conditions should trigger explicit cleanup and should be represented in operator logs, metrics, or request diagnostics.

## Concurrency assumptions

The model assumes that concurrent streams are independent in state and that stream failure is isolated by request/session identity. The design avoids treating a single stream as a global mutex lock target, which is why backpressure and ordering are modeled per stream and per queue boundary rather than at a global system level.

This aligns with the stress-test scaffolding, which uses in-process stubs to simulate the hot paths without claiming full production behavior.

## Telemetry model

The module’s explicit telemetry names are not merely cosmetic. They are the contract for operator triage:

- `[LLM_STREAM:BufferOverflow]`
- `[LLM_STREAM:GenerationStall]`
- `[LLM_STREAM:BackpressureCascade]`
- `[LLM_STREAM:Disconnected]`
- `[LLM_STREAM:TokenOrderViolation]`

These names appear in the runbook and in the soak/stress tests. They are the operational language for trust and incident handling.

## Risks and limits

This model is intentionally conservative:

- it does not infer that runtime behavior exists in `src/llm_streaming/`
- it expects stream semantics to be implemented and validated in actual canonical runtime modules
- it treats stub-based benchmarks as design evidence, not production evidence

The practical consequence is that model validity is tied directly to implementation ownership and test coverage in the canonical runtime code.
