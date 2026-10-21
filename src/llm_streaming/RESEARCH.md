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

# Research Notes: LLM Streaming

This document captures the engineering research and design assumptions behind the `llm_streaming` module contract. The module is still a specification and traceability layer rather than a colocated runtime implementation directory, which means the research base is intentionally anchored to the canonical runtime owners in `src/llm/` and `src/server/` and to the validation artifacts under `tests/llm_streaming/` and `benchmarks/llm_streaming/`.

## Research problem

The core operational challenge is not merely how to emit tokens quickly. It is how to produce a stream that remains observable, ordered, cancellable, and safe under overload. The repository already reflects that concern in the stream-oriented tests and runbooks:

- `tests/integration/test_llm_streaming_soak.cpp` models sustained producer/consumer pressure, token throughput, backpressure, and ordering
- `docs/operability/RUNBOOK_LLM_STREAMING.md` codifies incident response for buffer overflow, stall, cascade, disconnect, and ordering violations
- `src/llm/streaming_handler.cpp` provides the actual low-level formatting helpers for SSE payloads and chunk framing used at the runtime edge

## Source-grounded evidence

### Runtime-adjacent implementation evidence

- `include/llm/streaming_handler.h` defines the public stream-formatting surface used by the runtime
- `src/llm/streaming_handler.cpp` escapes JSON safely, emits SSE `data:` blocks, and wraps token output with a per-request callback adapter
- `tests/integration/test_llm_streaming_soak.cpp` uses in-process stubs to validate throughput and backpressure behavior under sustained load

### Module-level contract evidence

- `src/llm_streaming/ARCHITECTURE.md` defines the lifecycle states `open`, `active`, `cancelled`, and `closed`
- `src/llm_streaming/ROADMAP.md` states that production runtime ownership must be mapped to canonical modules before release-grade claims are made
- `src/llm_streaming/PRODUCTION_REQUIREMENTS.md` explicitly requires source-traceable ownership for stream runtime behavior

## Research questions

1. What lifecycle transitions are safe and observable when a stream is opened, cancelled, or closed?
2. What ordering and backpressure invariants must hold when a stream is exposed to high-cardinality concurrency?
3. Which runtime ownership boundaries are stable enough to support operator diagnosis and releases?
4. How should the implementation bridge from model output to HTTP/SSE or other client transports without silently losing state?

## Model assumptions

The current design assumes the following constraints:

- A stream has exactly one authoritative owner for lifecycle state and cleanup.
- Token ordering is deterministic within a single stream unless a bounded, explicit reordering policy is documented.
- Backpressure is a fail-closed condition: it must surface a defined status, not be silently dropped.
- Client disconnect and request cancellation are terminal conditions that require idempotent cleanup.
- Any production runtime implementation must live outside `src/llm_streaming/` until the canonical module ownership is documented.

## Failure domains

The documented failure taxonomy is deliberately narrow and operationally useful:

- buffer overflow / queue saturation
- generation stall / no progress for a bounded interval
- backpressure cascade across concurrent sessions
- mid-stream disconnect or transport reset
- token sequence violation

These correspond directly to the operator runbook in `docs/operability/RUNBOOK_LLM_STREAMING.md` and to the stress-test warnings in `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`.

## Evaluation and trade-offs

### Why a design contract is necessary

A docs-only module path is useful when product and runtime ownership are intentionally separated. It creates a durable contract for required semantics without over-claiming production status. This is consistent with the repository’s governance stance that `src/llm_streaming/` may remain specification-first until real runtime code is placed in the owning canonical modules.

### Trade-offs

- Specification-first approach: clearer contract and lower implementation risk; limited immediate runtime evidence
- Runtime-first approach: faster operational realism; requires stronger ownership, tests, and release evidence

The repository currently chooses the former until canonical runtime ownership is established, which is consistent with the roadmap and production requirements documents.

## Research conclusions

The streaming design requirement is best framed as a contract around bounded, observable stream behavior rather than as a single implementation artifact. The most credible path for progress is:

1. keep the module contract explicit and evidence-backed
2. map concrete runtime ownership to `llm` and `server`
3. release only after tests and benchmarks prove the behavior in the owning runtime files

This allows ThemisDB to avoid the common trap of documenting a production streaming feature without source-traceable runtime evidence.
