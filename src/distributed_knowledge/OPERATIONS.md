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

# Distributed Knowledge Operations

## Purpose and operational responsibility

For service owners deploying the in-process federation components. The module
provides bounded component behavior and local counters; the integrating
service must provide authenticated transport, remote authorization, durable
delivery, monitoring export, alerting, and cluster recovery. See
[ARCHITECTURE.md](./ARCHITECTURE.md) for boundaries and
[RUNBOOK.md](./RUNBOOK.md) for incident response.

## Pre-deployment checklist

- Set explicit deployment-specific limits and validate configuration at
  startup. The canonical
  [production requirements](./PRODUCTION_REQUIREMENTS.md) state that defaults
  are not production-safe configuration.
- Configure shard and aggregation timeouts in the service that performs
  fan-out. A zero RAG `shard_timeout_ms` has fail-closed behavior; an unlimited
  default is not a latency objective.
- Set explicit `min_participants`, `max_participants`, privacy parameters,
  maximum rounds, and bounded policy for aggregation/distillation.
- Install and test the policy/trust callbacks required by the deployment.
  Verify that a rejecting callback blocks data before propagation.
- Verify peer authentication and cross-border transfer policy outside the
  module's JSON serialization layer.
- Test behavior when callbacks throw, a shard is slow, all shards fail, and
  privacy or participant limits are reached.
- Ensure that the caller can reconcile in-flight rounds after process restart;
  module state is in-memory and is not a persistence mechanism.

## Observability

Export the component's available counters and structured events through the
service's standard metrics/logging pipeline. Relevant surfaces include:

- feedback `published`, `received`, `deduplicated`,
  `rejected_by_policy`, cached IDs, and skipped publish count;
- LoRA coordinator round, submitted/filtered gradient counts, privacy-budget
  remaining, and last delta;
- distillation round, submitted count, privacy-budget remaining, policy blocks,
  and `DistillationModelCard`;
- `DKDiagnosticEvent` severity/type, shard and operation IDs, cause, and
  metadata.

Counters are local to the object lifetime and are not a durable audit log.
Apply cardinality and data-classification controls before exporting event
fields; diagnostic causes or duplicate keys may contain identifiers.
Benchmark p95/p99 at representative shard fan-out and compare against
[PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md). The roadmap
retains follow-up work for expanded benchmark depth and operator visibility.

## Change, rollout, and rollback

Roll out public-contract or serialization changes with matching tests and a
consumer compatibility review. For algorithm or timeout changes, compare
partial-failure behavior and release baselines before broad rollout. During
rollback, stop new federation rounds, account for already delivered deltas or
distillation rounds, and reconcile in-flight state at the caller. Restarting
the process does not restore pending submissions or reset a privacy budget
durably; budget authority must live in an appropriate persistent control
plane if it must survive process replacement.
