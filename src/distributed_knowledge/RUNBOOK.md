---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Incident Runbook

## Scope and operator boundary

This runbook covers incidents involving the module's local federation
components. It assumes the service integrating the module owns network
transport, durable state, dashboards, alert routing, and shard lifecycle.
Module counters and diagnostic callbacks are available to export; this
repository does not establish a particular monitoring backend or alert rule.
For routine readiness, see [OPERATIONS.md](./OPERATIONS.md).

## Triage sequence

1. Record incident time, affected operation/round, configured strategy,
   participant/shard counts, timeout and privacy settings, and deployment
   revision. Preserve relevant diagnostic events before restarting stateful
   components.
2. Determine which path failed: retrieval merge, LoRA aggregation, feedback
   sync, or distillation. Inspect caller-side per-shard status as well as the
   component counters; a missing callback may be a transport or consumer
   failure, not a component failure.
3. Check policy, trust, authorization, privacy budget, and configuration
   outcomes before retrying. Do not weaken a fail-closed policy to restore
   throughput.
4. If the event could indicate unauthorized access, payload tampering, or
   privacy exposure, contain propagation and follow
   [TRUST.md](./TRUST.md) before resuming.

## Symptoms and safe actions

| Symptom | Diagnose | Safe response |
|---|---|---|
| RAG merge throws `all shards timed out` or result count drops | Check input result flags (`ok`, `timed_out`), timeout config, shard latency, and `shards_responded`. | Restore healthy shard responses or adjust the deployment timeout only after confirming the latency budget. If policy permits partial data, the component can merge responding shards; do not present an empty/all-timeout result as a valid complete response. |
| Aggregation fails for insufficient participants | Compare unique current-round shard submissions with `min_participants`; check stale/future round submissions and outlier filtering. | Restore eligible participants or abort the round. Do not lower the threshold without privacy and model-quality approval. |
| Aggregation or distillation reports budget exhausted | Inspect current round, configured cap, epsilon accounting, and policy-block counters/model card. | Stop further broadcasts and escalate to the privacy owner. Do not reset counters or recreate the coordinator as a budget bypass. |
| Feedback publish count rises but remote consumers see nothing | Inspect `getStats()`, skipped-publish count, gossip callback errors/backpressure, and receiver `received`, `deduplicated`, and `rejected_by_policy` counts. | Repair the callback/transport or policy configuration, then retry only under the caller's idempotency policy. Publishing is best-effort; counters are not delivery acknowledgments. |
| Distillation student is stale or did not receive a round | Check pending submission, policy decision, student registration, callback exceptions, and audit records. | Pause new rounds until recipient state is reconciled. The callback loop is not a durable all-students transaction. |
| Trust/policy rejection or malformed payload | Preserve the event and the minimum necessary payload metadata; verify peer identity and active policy with the responsible security owner. | Keep the request blocked. Do not replay an untrusted payload or use `AlwaysPermitTrustPolicy` as an incident workaround. |

## Restart and rollback

The coordinator objects hold pending submissions, round counters, dedup IDs,
and callbacks in process memory; this module does not persist or restore them.
Before restart, capture the operational evidence and reconcile the active
round with the caller. A restart can discard uncommitted in-memory state and
is not a rollback of already delivered updates. Roll back the integrating
service or configuration only through its normal release procedure; avoid
replaying contributions or broadcasts until their round and idempotency
status are known.

## Escalation

Escalate privacy-budget, cross-border, or policy questions to the privacy and
security owners; shard availability and callback delivery to the federation
service owner; ranking or model utility regressions to the relevant retrieval
or model owner. Record unresolved causes in the incident record and update
the module roadmap when they represent a recurring hardening gap.
