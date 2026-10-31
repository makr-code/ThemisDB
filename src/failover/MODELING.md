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

Exact model backend not verifiable from final document text alone.

# Failover Decision Model

## Purpose and scope

This document describes the deterministic rules used by the failover module. It
does not describe a statistical, machine-learning, or predictive failure model.
The source of truth is `include/failover/failover_api_contract.h` and the
implementations in `src/failover/`.

## Automatic failover inputs

The manager's decision inputs are:

| Input | Use | Important limit |
|---|---|---|
| Health-check result per node | Updates per-node consecutive-failure tracking | A timeout is a failed observation, not proof that the node is permanently lost. |
| `consecutive_failures_before_action` | Failure threshold before a request is considered | Default is 3; tune with the health-check interval and deployment latency. |
| Replication manager quorum | Promotion prerequisite | Quorum semantics are supplied by the replication manager. |
| Replica health and role | Candidate eligibility; unhealthy and witness replicas are excluded | Candidate state can change after observation, so promotion must still pass fencing. |
| Fencing manager epoch token | Split-brain guard | Epoch 0 is invalid and blocks promotion. |
| Queue capacity and pressure threshold | Bounds pending work and reports saturation | Queue-full submission returns `false`; it is counted as dropped. |

The `FailoverErrorCode` contract identifies quorum, fencing, timeout, and
split-brain conditions. Contract helpers define a quorum size of
`floor(cluster_size / 2) + 1`; runtime quorum checks are delegated to the
configured replication manager rather than calculated by this module.

## State and recovery model

Automatic failover progresses through failure verification, quorum checking,
leader-election/promotion, optional spare handling, metadata handling, and
completion verification. Any failed safety gate stops promotion. A failure
after a promotion or another external action does not imply that prior actions
were undone.

Disaster recovery executes the fixed sequence
`PRECHECKS → SNAPSHOT_VALIDATION → EPOCH_FENCING → RESTORE → REPLICA_CATCHUP
→ TRAFFIC_SHIFT → VERIFICATION`. The first unsuccessful step ends the run.
Successful results for non-empty `plan_id` values are cached for the lifetime
of the manager instance; a repeated ID returns that result instead of retrying.
Concurrent calls that are not satisfied from the cache are rejected.

## Dry-run semantics

With the built-in step handlers, a dry-run plan passes plan-field validation and
each step reports a dry-run outcome without performing the corresponding
external operation. In particular, built-in prechecks return before querying
the replication manager. A dry run therefore does not prove quorum, snapshot
availability, restoration, traffic movement, or recovered-state correctness.
Registered step hooks replace built-in handlers and are responsible for their
own side effects; do not assume a hook is harmless in dry-run mode.

## Limits and validation

This model is deterministic rather than probabilistic. It has no embedding,
classifier, or learned score. Validate changes with the focused health-timeout,
fencing, quorum-persistence, topology-versioning, and DR-edge tests listed in
[RESEARCH.md](RESEARCH.md). Production validation additionally requires tests
against the deployment's real manager implementations and failure timing.

## Related documentation

- [API.md](API.md) — exposed configuration, inputs, and results
- [IMPLEMENTATION.md](IMPLEMENTATION.md) — implementation sequence and caveats
- [TRUST.md](TRUST.md) — trust assumptions for decision inputs
