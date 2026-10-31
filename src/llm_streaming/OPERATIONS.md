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

This is design-time operational guidance, not a verified production runbook.
The source-backed `StreamingHandler` formats strings and calls a supplied
sink; it does not expose a stream registry, metrics, queue depth, cancellation,
or server connection. The tests and benchmark use local stubs.

## Signals currently supported by source

The helper emits a debug log when `formatDoneEvent(request_id)` is called. It
does not define the `[LLM_STREAM:...]` event names listed in older drafts, nor
does the current test source emit those as runtime logs; similar strings occur
in test assertion messages only. No stream count, latency, throughput,
buffer-occupancy, or cleanup metric was verified.

## Proposed incident triage (unverified)

If a future runtime provides streaming, operators will need an owner-specific
procedure to identify the request, distinguish model/sink/transport failures,
and verify termination and cleanup. The available sources do not identify
commands, dashboards, configuration, retry actions, or a supported recovery
procedure. Do not use the checklist below as an operational instruction until
it is populated from the deployed implementation:

- identify the affected request using a verified runtime correlation field;
- inspect documented runtime diagnostics and transport status;
- confirm the actual terminal result and cleanup behavior;
- escalate using the owning service's established procedure.

**TODO:** replace this provisional checklist with verified signals, commands,
owners, and recovery steps after runtime integration exists.

## Readiness boundary

The mock/stub stress, soak, and benchmark sources do not establish production
operability or release readiness. See `RUNBOOK.md` for the scenario-level
assumptions and `PRODUCTION_REQUIREMENTS.md` for proposed acceptance gates.
