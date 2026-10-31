---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Operations: LLM Streaming

## Status and scope

This is design-time operational guidance, not a complete production runbook.
The HTTP route `GET /api/v1/llm/stream` is implemented, but buffers the complete
SSE body before returning it. The separate `StreamingHandler` helper formats
strings and calls a supplied sink; it does not expose a stream registry, metrics,
queue depth, cancellation, or server connection. The module tests and benchmark
use local stubs.

## Signals currently supported by source

The helper emits a debug log when `formatDoneEvent(request_id)` is called. The
route logs request ID, prompt length, max tokens, completion, response byte
count, and caught generation errors. No stream count, latency, throughput,
buffer-occupancy, or cleanup metric was verified. The route's query parameters
are `prompt`, `request_id`, and `max_tokens` (default 512; maximum accepted
value 4096).

The OpenAI-compatible stream path logs the selected model, prompt length,
requested maximum tokens, and final SSE body size. It uses API-key validation
via `PolicyEngine`, rather than the bearer-token gate used by the other LLM
routes. No streaming-specific metric or per-stream cancellation signal was
verified.

## Proposed incident triage (unverified)

Operators still need an owner-specific procedure to identify the request,
distinguish model and response-construction failures, and verify termination
and cleanup. The available sources do not identify
commands, dashboards, configuration, retry actions, or a supported recovery
procedure. Do not use the checklist below as an operational instruction until
it is populated from the deployed implementation:

- identify the affected request using a verified runtime correlation field;
- inspect documented runtime diagnostics and transport status;
- confirm the actual terminal result and cleanup behavior;
- escalate using the owning service's established procedure.

**TODO:** replace this provisional checklist with verified signals, commands,
owners, and recovery steps for the deployed route; current source evidence does
not establish streaming-specific diagnostics or transport recovery.

## Readiness boundary

The mock/stub stress, soak, and benchmark sources do not establish production
operability or release readiness. See `RUNBOOK.md` for the scenario-level
assumptions and `PRODUCTION_REQUIREMENTS.md` for proposed acceptance gates.
