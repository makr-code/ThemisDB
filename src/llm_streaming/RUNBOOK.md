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

# Runbook Notes: LLM Streaming

## Status

This is a placeholder for a future, source-verified operator runbook, not
instructions for a currently verified streaming service. The helper API
formats strings and invokes a supplied sink; it does not expose a stream ID
registry, connection state, metrics, queue, or recovery operations. The
test/benchmark stubs do not establish production incident behavior.

## Scenarios requiring verified procedures

The following are design scenarios only; no verified operational signal,
command, dashboard, alert, or remediation was found for them:

### Sink or transport failure

Document detection, the actual request correlation field, caller behavior,
and cleanup only after confirming the runtime path and its tests.

### Slow consumer or backpressure

Document queue limits, admission/failure semantics, and safe recovery only if
an owning runtime implements them. Stub queue capacity is test data, not a
deployment setting.

### Cancellation or completion

The formatter can produce an independent `[DONE]` string, but the callback
adapter does not emit it automatically. Record who decides terminal behavior
and how it is delivered only after verifying the caller.

### Ordering

The callback assigns indexes atomically, but does not serialize concurrent
sink calls. Establish the sink's concurrency contract before prescribing
ordering checks or recovery.

## Operator readiness TODO

Before using this as an operational runbook, add verified owner/team,
diagnostics, alerts, commands, supported recovery steps, escalation path, and
evidence from the deployed implementation. Until then, consult the owning
service's established procedures rather than inferring actions from this
design document.

## Sources

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
