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

# Runbook: LLM Streaming

This runbook condenses the repository’s operational guidance for streaming failure modes. The primary source for the scenarios below is `docs/operability/RUNBOOK_LLM_STREAMING.md`, while the design constraints come from `src/llm_streaming/ARCHITECTURE.md` and `PRODUCTION_REQUIREMENTS.md`.

## Scenario: buffer overflow

Symptoms:
- tokens accumulate faster than the downstream consumer can drain them
- queue depth spikes and throughput drops
- `[LLM_STREAM:BufferOverflow]` appears in logs

Diagnosis:
- confirm the `stream_id` and buffer occupancy
- compare throughput to consumer drain rate
- check if a single stream or a cluster of streams is saturating queue capacity

Remediation:
- isolate the stream with the heaviest backlog
- fail closed instead of continuing to enqueue silently
- reduce concurrency or increase drain capacity if the deployment profile allows it

Escalation:
- escalate when overflow persists across multiple sessions or when it correlates with broader request latency degradation

## Scenario: generation stall

Symptoms:
- no token output for a sustained interval
- latency increases without a clear terminal signal
- `[LLM_STREAM:GenerationStall]` appears

Diagnosis:
- inspect the last emitted token sequence and timestamp
- confirm whether the upstream model or the transport layer is blocked
- review memory pressure, queue saturation, or worker starvation

Remediation:
- cancel the stale stream and return a terminal error to the caller
- isolate the upstream worker or model instance generating the stall
- verify that restart or reroute logic is safe and idempotent

Escalation:
- escalate to runtime and model operations if the stall reproduces on otherwise healthy hardware

## Scenario: backpressure cascade

Symptoms:
- one bounded issue causes multiple streams to degrade together
- `[LLM_STREAM:BackpressureCascade]` fires
- queue depths rise across sessions concurrently

Diagnosis:
- identify the root trigger stream and which streams are affected
- confirm that the trigger is not caused by a shared global lock or common queue
- assess whether a single slow consumer is starving the rest of the deployment

Remediation:
- isolate and cancel the triggering session
- ensure queue boundaries are per-stream or per-worker when possible
- re-balance or reduce concurrency until queue occupancy stabilizes

## Scenario: mid-stream disconnect

Symptoms:
- stream is interrupted before completion
- partial output remains visible but no clean final signal is emitted
- `[LLM_STREAM:Disconnected]` appears

Diagnosis:
- check the disconnect reason and request identity
- confirm whether the client disconnected or the server timed out the stream
- verify whether in-flight cleanup happens for all buffers and resources

Remediation:
- return a clear terminal status to the client when possible
- complete cleanup of any stale session or buffer state
- validate retry or reconnection logic if the platform supports resumable streams

## Scenario: token ordering violation

Symptoms:
- tokens arrive out of order
- UI output becomes semantically inconsistent
- `[LLM_STREAM:TokenOrderViolation]` appears

Diagnosis:
- compare expected and actual sequence numbers
- determine whether a single producer, shared dispatcher, or downstream buffer is reordering tokens
- validate that the stream is using a single owner for order enforcement

Remediation:
- stop the stream, discard invalid output, and re-establish a known-good sequence boundary
- enforce ordering before the sink or client receives the payload
- verify the fix with stress and soak coverage

## Operator checklist

Before declaring the issue closed:

- the stream identity is known and traceable
- the failure mode is classified as overload, cancellation, disconnect, or ordering violation
- the final state is explicit: closed, cancelled, or failed
- cleanup was idempotent and did not leave leaked resources
- diagnostic telemetry is preserved for post-incident review

This runbook is intentionally operationally oriented and should stay consistent with the design docs and test evidence in the module.
