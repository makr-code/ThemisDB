# RUNBOOK: LLM Pipeline — Operator Incident Triage

<!-- Status: current | validated: 2026-09-16 -->
<!-- Module: llm | Wave: D -->

## Overview

This runbook covers the five most common LLM pipeline incident classes
encountered in production ThemisDB clusters.  Each scenario includes
detection signals, log patterns, triage steps, mitigation, and escalation
criteria.

---

## Scenario 1 — Model Routing Failure

### Description
The model router fails to assign a valid model tier to one or more inference
requests, resulting in request rejection or an internal routing error.

### Detection
- Log pattern: `[LLM:RoutingFailed]`
- Prometheus: `llm_routing_failures_total` counter increments
- Alert: `LLMRoutingFailureAlert`

### Triage Steps
1. Check the routing failure reason from `[LLM:RoutingFailed]` log entry.
2. Verify that all model tiers (LARGE, MEDIUM, SMALL, FALLBACK) are loaded:
   ```
   themis-admin llm model-status --all-tiers
   ```
3. Check for policy misconfiguration that may be blocking all tiers:
   ```
   themis-admin llm routing-policy validate
   ```
4. Review token count distribution — an extreme outlier may be hitting an
   unhandled routing branch.

### Mitigation
- If a model tier is unavailable: force fallback to the next available tier:
  ```
  themis-admin llm routing set-fallback --tier SMALL
  ```
- If policy misconfiguration: reload the routing policy:
  ```
  themis-admin llm routing-policy reload
  ```
- If routing is permanently broken: restart the router with default policy:
  ```
  themis-admin llm restart-router --use-defaults
  ```

### Escalation
Escalate if routing failures affect > 1 % of requests for > 5 minutes — the
routing table may have been corrupted by a concurrent policy update.

---

## Scenario 2 — Adapter OOM

### Description
A LoRA adapter or plugin fails to load due to VRAM or system RAM exhaustion,
leaving the adapter in an inconsistent state.

### Detection
- Log pattern: `[LLM:AdapterOOM]`
- Prometheus: `llm_adapter_oom_events_total` counter increments
- Alert: `LLMAdapterOOMAlert`

### Triage Steps
1. Identify the adapter name and size from `[LLM:AdapterOOM]` log.
2. Check current VRAM and system RAM usage:
   ```
   themis-admin llm resource-usage --vram --ram
   ```
3. List currently loaded adapters and their memory footprint:
   ```
   themis-admin llm adapters list --show-memory
   ```
4. Check for adapter leaks — adapters that are loaded but have no active
   requests referencing them.

### Mitigation
- Unload unused adapters to free memory:
  ```
  themis-admin llm adapters unload --idle-for 300s
  ```
- If VRAM is persistently tight: reduce `llm.adapter.max_loaded` to enforce
  a smaller resident set:
  ```
  themis-admin config set llm.adapter.max_loaded 4
  ```
- Apply adapter eviction policy (LRU):
  ```
  themis-admin llm adapters set-eviction-policy lru
  ```

### Escalation
Escalate if OOM events recur immediately after unloading — the model weights
may themselves be too large for the available hardware, requiring offloading
or quantization.

---

## Scenario 3 — Queue Pressure Overload

### Description
The inference request queue depth exceeds the admission-control threshold,
causing new requests to be rejected or delayed significantly.

### Detection
- Log pattern: `[LLM:QueuePressure]`
- Prometheus: `llm_queue_depth` gauge exceeds `llm.queue.max_depth` * 0.8
- Alert: `LLMQueuePressureAlert`

### Triage Steps
1. Check current queue depth and request arrival rate:
   ```
   themis-admin llm queue-stats
   ```
2. Identify the model tier with the longest queue — LARGE requests are most
   commonly the bottleneck.
3. Check for slow/stalled inference workers:
   ```
   themis-admin llm workers status
   ```
4. Verify that the GPU is not in a degraded state causing slow token generation.

### Mitigation
- Scale out inference workers horizontally (if capacity is available):
  ```
  themis-admin llm workers scale --count 2
  ```
- Apply request rate limiting at the ingestion layer:
  ```
  themis-admin llm set-rate-limit --rps 50
  ```
- Temporarily demote LARGE-tier requests to MEDIUM-tier to reduce latency:
  ```
  themis-admin llm routing override-tier --from LARGE --to MEDIUM --duration 300s
  ```

### Escalation
Escalate if queue depth continues to grow despite rate limiting — the
upstream request source may be ignoring backpressure signals.

---

## Scenario 4 — Policy-Deny Cascade

### Description
The safety/policy enforcement layer begins rejecting a high fraction of
inference requests due to a policy misconfiguration or overly aggressive
content filter update.

### Detection
- Log pattern: `[LLM:PolicyDeny]`
- Prometheus: `llm_policy_deny_rate` ratio exceeds 0.05 (5 %)
- Alert: `LLMPolicyDenyCascadeAlert`

### Triage Steps
1. Check the deny reason distribution from `[LLM:PolicyDeny]` logs.
2. Review the most recently deployed policy version:
   ```
   themis-admin llm policy history --last 5
   ```
3. Compare deny rates before and after the last policy deployment.
4. Check if a specific request category (topic, token pattern) accounts for
   the majority of denies.

### Mitigation
- Roll back the policy to the last known-good version:
  ```
  themis-admin llm policy rollback --version <previous-version>
  ```
- If rollback is not feasible: disable the newly added rule temporarily:
  ```
  themis-admin llm policy disable-rule --rule-id <id>
  ```
- Post-incident: audit the new policy rules against a representative request
  corpus before re-enabling.

### Escalation
Escalate if the deny cascade affects all request categories — a global policy
error may have been introduced that requires an emergency policy reset.

---

## Scenario 5 — Speculative Decoding Stall

### Description
The speculative decoding pipeline stalls — the draft model is not producing
tokens fast enough, causing the verification model to wait and overall
throughput to degrade below the target TPS.

### Detection
- Log pattern: `[LLM:SpecDecStall]`
- Prometheus: `llm_spec_dec_acceptance_rate` drops below 0.5
- Alert: `LLMSpecDecStallAlert`

### Triage Steps
1. Check the acceptance rate and stall duration from `[LLM:SpecDecStall]`.
2. Verify draft model health and latency:
   ```
   themis-admin llm spec-dec draft-model-status
   ```
3. Check whether speculative decoding stalls correlate with VRAM pressure
   (draft and verification models competing for memory).
4. Review the configured speculation depth — too many draft tokens increases
   rejection probability under distribution shift.

### Mitigation
- Reduce the speculation depth to improve acceptance rate:
  ```
  themis-admin llm spec-dec set-depth --tokens 3
  ```
- If the draft model is degraded: disable speculative decoding and fall back
  to standard autoregressive decoding:
  ```
  themis-admin llm spec-dec disable
  ```
- Restart the draft model worker if it is in a stalled state:
  ```
  themis-admin llm spec-dec restart-draft-worker
  ```

### Escalation
Escalate if disabling speculative decoding does not restore throughput — the
verification model itself may be stalled, which is a broader inference
pipeline failure.

---

## Quick Reference — Log Patterns

| Pattern                  | Scenario                       |
|--------------------------|--------------------------------|
| `[LLM:RoutingFailed]`    | Model routing failure          |
| `[LLM:AdapterOOM]`       | Adapter OOM                    |
| `[LLM:QueuePressure]`    | Queue pressure overload        |
| `[LLM:PolicyDeny]`       | Policy-deny cascade            |
| `[LLM:SpecDecStall]`     | Speculative decoding stall     |

## Related Documents
- `src/llm/ROADMAP.md` — module roadmap
- `docs/operability/RUNBOOK_AI_GENERATION.md` — AI generation runbook
- `docs/operability/WAVE_D_ROADMAP.md` — Wave D operability plan
- `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` — acceptance criteria
