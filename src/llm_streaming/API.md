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

# API Contract: LLM Streaming

This document describes the API contract implied by the existing source and design evidence for the `llm_streaming` layer. It is intentionally grounded in the concrete helper layer already present in `src/llm/streaming_handler.cpp` rather than in generic “best practice” language.

## Core contract

The canonical runtime-facing contract is a token-delivery interface with explicit boundaries:

- request/session identity
- token payload emission
- finish or cancel state
- explicit error or overflow handling

## Concrete helper surface

The current source evidence exposes the following public helper behavior:

### `StreamingHandler::formatSseEvent()`

Purpose:
- format one emitted token as an SSE data payload

Inputs:
- `token`: emitted content
- `request_id`: request or stream identity
- `index`: per-stream sequence number
- `done`: terminal completion signal

Output:
- UTF-8 string in the form `data: {"id":"...","token":"...","index":N,"done":true|false}\n\n`

Behavior:
- escapes JSON special characters safely
- preserves order by using a per-stream index
- remains transport-safe for SSE cursors and client reconstruction

### `StreamingHandler::formatDoneEvent()`

Purpose:
- emit the stream-complete sentinel used by SSE-style clients

Output:
- `data: [DONE]\n\n`

### `StreamingHandler::formatChunkedData()`

Purpose:
- emit HTTP chunked-transfer framing for a payload

Behavior:
- prepends the chunk length in hexadecimal
- ensures the chunk ends with CRLF framing

### `StreamingHandler::makeStreamCallback()`

Purpose:
- wrap a sink callback with request-scoped token formatting

Behavior:
- validates sink is non-null
- maintains per-request token index via an atomic counter
- serializes token emission into SSE payloads before delivering to the sink

## Error semantics

The contract should treat the following as explicit failure modes:

- stream not found / no active stream session
- cancellation requested while stream is active
- backpressure threshold exceeded
- token ordering violation
- client disconnect while stream is still active
- timeout while awaiting completion or drain

The repository documents these semantics in `src/llm_streaming/ARCHITECTURE.md` and `docs/operability/RUNBOOK_LLM_STREAMING.md`.

## API invariants

The implementation should uphold these invariants:

1. each stream has a single identity and a single owner
2. the sink callback is non-null when stream output is enabled
3. token sequences are monotonic within a stream
4. a terminal state is explicit and not inferred
5. stream cleanup is idempotent
6. diagnostics are emitted before resource teardown is complete

## Compatibility note

This file reflects the current source-grounded API contract, not a claim that the full streaming runtime is complete under `src/llm_streaming/`. The repository’s governance requires that any production-ready API be backed by canonical runtime ownership and validation evidence outside this module path.
