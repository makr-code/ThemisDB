---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Sharding Runbook

## Purpose

This runbook is the operator-facing guide for the principal failure modes in the sharding module. It is grounded in the behavior encoded in `shard_router.cpp`, `cross_shard_transaction.cpp`, `wal_manager.cpp`, `health_monitor.cpp`, `auto_rebalancer.cpp`, and `sharding_api_contract.h`.

## Scenario 1: quorum loss

### Symptoms

- writes start failing with `QUORUM_LOST`,
- health metrics show missing replicas or degraded cluster state,
- routing decisions become limited or read-only by policy.

### Immediate actions

1. Confirm shard reachability and determining quorum count.
2. Check whether the primary and required replicas are reachable.
3. Inspect the coordinator and health monitor logs for failover decisions.
4. Stop write traffic to the affected shard family until quorum is restored.

### Recovery

- restore the required replica coverage,
- validate the topology and coordination state,
- resume writes only after quorum health is stable.

## Scenario 2: shard unavailable or read-only path

### Symptoms

- route-level failure indicates `SHARD_UNAVAILABLE`,
- health monitor marks the node as `DOWN` or `RECOVERING`,
- the client sees explicit routing or availability errors instead of silent fallback.

### Immediate actions

1. Check whether a node is disconnected or overloaded.
2. Verify the shard's health endpoint and failover action.
3. Confirm whether the request can be retried using the fallback route policy.
4. Ensure the fallback path remains inside the configured safety envelope.

### Recovery

- restore service at the failed shard,
- verify that the topology view is consistent,
- review the topology-change and rebalancer logs before returning to normal traffic.

## Scenario 3: 2PC in-doubt state

### Symptoms

- transactions remain in an unresolved `IN_DOUBT` state,
- participants block conflicting writes,
- coordinator recovery or WAL replay is required.

### Immediate actions

1. Confirm that the coordinator is the source of the ambiguity.
2. Inspect transaction WAL entries and participant state.
3. Determine whether the decision was interrupted before commit or abort.
4. Use the recovery path rather than force-committing a partially prepared transaction.

### Recovery

- replay the transaction state from WAL,
- decide the final commit or abort from the authoritative decision record,
- release blocked participants only after resolution and state validation.

## Scenario 4: migration conflict or rebalance fault

### Symptoms

- `MIGRATION_CONFLICT` or `MIGRATION_FAULT` is returned,
- shard movement or rebalance throughput falls below target,
- a partial or failed migration leaves the topology in a degraded state.

### Immediate actions

1. Confirm whether a second migration is targeting the same shard.
2. Inspect rebalance progress and rollback state.
3. Validate whether the source or target shard remains readable and writable.
4. Do not resume new migrations until the current fault is resolved.

### Recovery

- enforce serial execution for conflicting migration jobs,
- trigger rollback if the migration failed before verification,
- restore topology trust before allowing any new write traffic to rely on the new range assignment.

## Scenario 5: WAL corruption or replay failure

### Symptoms

- `WAL_CORRUPTION` is raised,
- replay stops operating on an ordered entry stream,
- the module requests operator intervention instead of continuing in uncertain state.

### Immediate actions

1. Stop automatic recovery procedures that assume the WAL is valid.
2. Retain the current WAL files before any repair attempt.
3. Validate the file integrity and checksum state.
4. Route the system into read-only or maintenance mode if required.

### Recovery

- repair or restore the WAL from a trusted checkpoint,
- replay only valid LSN ranges,
- re-enable traffic only after consistency checks pass.

## Operational notes

- health, quorum, and transaction checks should be treated as safety signals, not merely observability signals,
- error codes should be mapped to operator actions rather than hidden under generic error text,
- topology changes and repair jobs should be serialized when they touch the same shard group,
- degraded modes are acceptable only when the module remains explicit and fail-closed.

## Escalation

Escalate to the distributed systems or database reliability owner when any of the following is true:

- quorum cannot be restored within the configured service window,
- transaction in-doubt state persists beyond the allowed recovery timeout,
- migration rollback fails and leaves the topology uncertain,
- a WAL replay or corruption event affects multiple shards or a shared coordinator.
