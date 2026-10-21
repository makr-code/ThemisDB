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

# Security Considerations: LLM Streaming

This document records the security-relevant characteristics of the LLM streaming contract. It is intentionally grounded in the repository’s existing security posture around streaming components, transport-layer formatters, and operator diagnostics.

## Trust boundaries

The relevant trust boundaries are:

- upstream model or inference layer
- request/session orchestration
- transport boundary (HTTP/SSE/gRPC or other stream boundaries)
- client or sink consumer
- operator diagnostics and log output

A stream crosses more than one trust boundary, so the contract must keep state transitions explicit and auditable.

## Key risks

### 1. Partial or ambiguous stream state

A token stream that continues after cancellation or disconnect can produce partial responses that look valid but are operationally unsafe. The design therefore requires explicit states and cleanup.

### 2. Ordering violations

If a token order is silently re-ordered or lost, downstream clients may receive semantically corrupted content. The repository treats ordering violation as an explicit failure category.

### 3. Queue exhaustion and backpressure failures

If the upstream producer keeps writing while the sink is saturated, the stream becomes unstable. The design uses bounded overflow behavior and explicit diagnostics, not silent truncation.

### 4. Diagnostic leakage

Logs and metadata must not reveal more than necessary about internal session state. Request IDs, stream IDs, and error reasons are useful, but they should remain bounded and deliberate.

## Controls

The repository’s documented controls are straightforward and suitable for the current contract stage:

- explicit state transitions for open/active/cancelled/closed streams
- request-scoped identity and index values for tracking
- fail-closed handling on overflow or cancellation
- JSON escaping for payloads to avoid injection-like malformed output
- operator event names tied to specific failure modes

The actual source evidence for the serialization safeguards is in `src/llm/streaming_handler.cpp`, which escapes control characters and wraps payloads in a structured event format before writing them to the sink.

## Source-grounded control evidence

- `src/llm/streaming_handler.cpp`: escapes JSON control characters and maintains structured token events
- `src/llm_streaming/ARCHITECTURE.md`: documents fail-closed behavior for cancellation and overflow
- `docs/operability/RUNBOOK_LLM_STREAMING.md`: defines diagnostics and escalation for the failure classes
- `tests/integration/test_llm_streaming_soak.cpp`: validates the failure behavior under load via in-process stubs

## Security posture statement

The current posture is not “fully production-verified streaming security”; it is a conservative, source-grounded contract that makes lifecycle and overflow behavior explicit and operator-visible. That is the correct posture for a docs-first module while runtime ownership remains elsewhere.
