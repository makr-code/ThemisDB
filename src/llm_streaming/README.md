# ThemisDB LLM Streaming Module

<!-- Status: DESIGN_ONLY | docs-only module path | source-validated: 2026-10-07 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · PRODUCTION_REQUIREMENTS.md -->

## Module Purpose

This directory is a specification and planning surface for the LLM streaming capability, not a colocated production implementation directory.

The current repository state confirms that `src/llm_streaming/` contains documentation and validation scaffolding, while the canonical runtime logic is implemented in other source locations (notably the `llm`, `server`, and `query`/`rag` paths). The module therefore acts as a design contract and traceability anchor for streaming requirements, not as the runtime source of truth.

## Source-Validated Status

- Runtime implementation is not colocated in `src/llm_streaming/` today.
- Active evidence is restricted to:
  - `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
  - `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`
  - module-level design documents in this directory
- Any production claim about streaming behavior must map back to the canonical runtime sources outside this directory before it is treated as release evidence.

## Relevant Interfaces and Canonical Mapping

| Canonical source | Role |
|---|---|
| `src/llm/` | LLM backend integration, prompt/routing, safety and model lifecycle paths |
| `src/server/` | incoming request handling, HTTP/gRPC boundaries, and streaming-facing runtime integration |
| `tests/llm_streaming/` | stress and edge-case validation for streaming behavior |
| `benchmarks/llm_streaming/` | benchmark gate coverage for throughput and latency assumptions |

## Scope

In scope:
- specification of streaming requirements and failure semantics
- deterministic backpressure, ordering, and cancellation contract design
- validation of concurrency and performance assumptions in test/benchmark stubs
- source-traceability from module roadmap claims to canonical runtime implementation

Out of scope:
- production release claims based solely on docs-only module artifacts
- implicit runtime implementation in this directory without corresponding canonical source ownership
- direct feature completion claims that cannot be mapped to runtime files and tests

## Runtime Behavior and Limits

- design behavior depends on the canonical runtime backend selected outside this directory
- streaming tokens, cancellation, and backpressure semantics are governed by the owning runtime module contracts, not by this directory alone
- test/benchmark files here intentionally model hot paths in-process and are not production code paths

## Documentation and Governance References

- `ROADMAP.md` — phased plan and current evidence status
- `ARCHITECTURE.md` — design-level architecture contract
- `PRODUCTION_REQUIREMENTS.md` — minimum production and governance expectations
- `docs/architecture/DATA_FLOW_PATHS.md` — cross-module release traceability notes
- `ROOT ROADMAP.md` — project-level source-validated status and gating context

## Sourcecode Verification Notes

This module is currently a design/planning artifact. Any future runtime implementation must be colocated in a canonical module directory and must be linked from this file, the roadmap, and the relevant architecture documents before it is treated as production evidence.
