# LLM Streaming Module — Architecture

<!-- Status: DESIGN_SPECIFICATION | docs-only module status | source-validated: 2026-10-07 -->

## Overview

This file documents the intended architecture for LLM streaming behavior, but it is a design contract, not a source-of-truth implementation record.

The repository currently does not contain a production runtime implementation for `llm_streaming` under `src/llm_streaming/`. The local directory is therefore treated as a specification boundary: it captures the required streaming semantics, lifecycle expectations, and validation strategy, while the actual runtime code lives in other canonical modules such as `src/llm/` and `src/server/`.

## Design Principles

1. **Deterministic token ordering:** a stream must preserve per-session ordering unless the runtime explicitly documents a bounded reordering policy.
2. **Fail-closed behavior:** backpressure, cancellation, and network failures must produce explicit errors and cleanup rather than silent data loss.
3. **Owner-based lifecycle:** each stream must have a clear lifecycle owner and cleanup path.
4. **Observable runtime:** diagnostics must tell whether the failure comes from the upstream model, the transport, or the client.
5. **Source-traceable delivery:** any production claim must map to a real runtime implementation and test artifact.

## Architecture Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│ Canonical runtime ownership (not colocated in src/llm_streaming)   │
│  • src/llm/ - generation, token flow, model lifecycle               │
│  • src/server/ - request/response and network boundary             │
│  • src/query/ or src/rag/ - higher-level orchestration & retrieval │
└───────────────────────────────┬────────────────────────────────────┘
                               │ contract / boundary
                               ▼
┌────────────────────────────────────────────────────────────────────┐
│ llm_streaming specification layer                                   │
│  • stream lifecycle semantics                                        │
│  • token ordering, cancellation, backpressure contract              │
│  • validation plan and sample benchmark assumptions                 │
└───────────────────────────────┬────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────────┐
│ Validation / evidence layer                                          │
│  • tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp │
│  • benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp │
└────────────────────────────────────────────────────────────────────┘
```

## Contract Model

### Stream lifecycle

- `open`: session is created and associated with a request or client connection
- `active`: token emission or backpressure management is in progress
- `cancelled`: explicit stop request received; cleanup must complete deterministically
- `closed`: terminal state reached; resources and diagnostics are finalized

### Error classes

The module specification uses a risk-driven taxonomy rather than a production implementation contract. Critical classes are:
- stream not found / lifecycle mismatch
- cancellation requested during active emission
- backpressure overflow / queue saturation
- token ordering violation
- transport disconnect during active stream

### Required runtime invariants

- no silent token loss on cancellation
- no per-session cross-talk across independent streams
- stream cleanup must be idempotent
- diagnostics must expose the last observed failure mode and boundary

## Concurrency Model

This section is a specification only. The actual implementation must be owned by the canonical runtime modules and must define the actual locking strategy there.

The module-level validation sleds intentionally keep concurrency assumptions small and testable by using in-process stub models instead of claiming production runtime behavior.

## Test and Benchmark Scope

The module validation is intentionally narrow and clearly labeled as simulation/stub coverage:
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`

These prove that the design assumptions are testable, but they are not evidence that a real production streaming implementation exists inside `src/llm_streaming/`.

## Source Traceability Rule

If runtime code is added to this directory in the future, the corresponding roadmap and governance files must be updated at the same time. Until then, the module remains a design-only planning contract and must not be treated as a production implementation surface.
- **Total Memory (100 streams):** ~1 GB

## Error Handling

### Graceful Degradation

1. **Client Disconnection** → Detect via write failure; clean up stream
2. **Network Timeout** → Retry with exponential backoff
3. **Backpressure Timeout** → Close stream with error
4. **Buffer Overflow** → Return backpressure error to LLM producer

### Error Codes (E7300–E7399)

- E7300: Stream not found
- E7301: Cancellation requested
- E7302: Backpressure buffer exceeded
- E7303: Token send timeout
- E7304: Invalid token sequence

## Integration Points

### LLM Inference Engine

Streaming receives tokens from LLM inference as they are produced:
- Token callback: `onToken(token, finish_reason)`
- LLM can check backpressure: `isBackpressured() → bool`

### Client Protocols

- gRPC: ServerWriter<Token> streaming
- HTTP: Server-Sent Events (SSE)

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/llm_streaming/streaming_server.h`](../../include/llm_streaming/streaming_server.h) — Public API

## Implementation Status

> **No source implementation.** This module path contains only `.gitkeep`, `README.md`, and `ARCHITECTURE.md`. Implementation claimed in docs is externalized or planned; all claims must be treated as aspirational until source is delivered.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| llm | `include/llm/` | LLM inference layer produces tokens consumed by the streaming pipeline |
| utils | `include/utils/` | Thread-pool, rate-limiter, and logging helpers for stream management |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/llm_streaming/streaming_server.h` | Server delivers token streams to gRPC (ServerWriter) and HTTP (SSE) clients |

## Integration Points

### Critical Integration: LLM Token Production
**Files:** (planned) streaming pipeline ↔ `llm/`
**Contract:** LLM calls `onToken(token, finish_reason)` callback; streaming layer handles backpressure via `isBackpressured()` check before each token delivery.
**Thread Safety:** Token callbacks must be safe to call from LLM inference thread; streaming layer serialises delivery under per-stream state.

> **All integration claims above are aspirational until source implementation is delivered.**
