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

# Embedding the Streaming Contract in ThemisDB

This document explains how the `llm_streaming` contract fits into the surrounding runtime architecture. It is source-grounded in the observable implementation and operability evidence that currently exists: `src/llm/streaming_handler.cpp`, stress and soak tests, and the module documentation in `src/llm_streaming/`.

## Runtime embedding model

At a high level, the streaming flow is:

1. a request is initiated and assigned a stream identity
2. the LLM runtime emits tokens
3. the adapter serializes them into a transport-safe payload
4. the stream sink writes the payload through HTTP/SSE or another transport boundary
5. cancellation or disconnect triggers explicit fail-closed cleanup

This matches the design in `src/llm_streaming/ARCHITECTURE.md` and the actual `StreamingHandler` helper in `src/llm/streaming_handler.cpp`.

## Concrete transport contract

The actual helper layer already encodes the wire contract used by the runtime:

- `StreamingHandler::formatSseEvent()` emits a JSON payload wrapped in `data:`
- `StreamingHandler::formatDoneEvent()` emits the terminal sentinel `data: [DONE]\n\n`
- `StreamingHandler::formatChunkedData()` emits a chunked transfer payload in HTTP-style framing
- `StreamingHandler::makeStreamCallback()` creates a callback that formats each token before it reaches the sink

This is the clearest evidence that the module is not abstract; it is a real transport-facing concern that must remain deterministic and observable.

## Embedding within server and LLM layers

The repository explicitly says that the canonical runtime ownership should be mapped to the owning runtime modules, especially `src/llm/` and `src/server/`.

That is a crucial architectural point: the streaming design is embedded in the broader request lifecycle but is not meant to be a hidden implementation layer in `src/llm_streaming/` itself.

Expected embedding responsibilities:

- `llm`: token production and termination signal generation
- `server`: inbound request lifecycle, HTTP/gRPC/SSE transport adaptation
- `query` / `rag`: orchestration layers that consume and shape context while streaming responses are produced
- `observability`: metrics and incident reporting for failure and throughput monitoring

## Event model

The canonical streaming event model is intentionally simple:

```json
{"id":"request-123","token":"hello","index":0,"done":false}
```

The runtime helper uses this pattern and escapes JSON string content before building the payload. That means the stream payload remains safe and schema-like even when token text contains quotes, control characters, or line breaks.

## Operational embedding

The operational semantics are deliberately exposed to operators rather than hidden under a generic “it just works” model:

- buffer overflow at the sink or queue boundary
- generation stall in model output
- backpressure cascade across streams
- disconnect in the middle of delivery
- ordering violations in reassembled chunks

This is the same set of conditions captured in `docs/operability/RUNBOOK_LLM_STREAMING.md` and modeled by the soak/stress tests.

## Limits to the current embedding

The repository does not currently present a complete production streaming runtime under `src/llm_streaming/`. That means the embedding is intentionally conceptual and governance-driven today:

- canonical ownership is documented instead of hidden
- design assumptions are strong and explicit
- runtime evidence must still be collected in the real module files before any release claim is accepted

This is the repository’s intended balance between product clarity and source-traceable runtime discipline.
