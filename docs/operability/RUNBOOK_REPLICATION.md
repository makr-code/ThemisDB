# RUNBOOK: Replication Module — Operator Incident Triage

<!-- Status: current | validated: 2026-09-16 -->
<!-- Module: replication | Wave: D -->

## Overview

This runbook covers the five most common replication incident classes
encountered in production ThemisDB clusters.  Each scenario includes
detection signals, log patterns, triage steps, mitigation, and escalation
criteria.

---

## Scenario 1 — Replication Lag Storm

### Description
Replication lag on one or more secondary replicas grows rapidly, exceeding
the operator-configured `replication.wal_shipping.max_lag_ms` threshold
(default 1 000 ms).

### Detection
- Log pattern: `[REPLICATION:LagStorm]`
- Prometheus: `replication_wal_lag_ms` histogram p99 exceeds threshold
- Alert: `ReplicationLagStormAlert` fires when lag > 2× the SLO window

### Triage Steps
1. Identify which secondary replica(s) are lagging from the log context.
2. Check network bandwidth between primary and lagging replica:
   ```
   themis-admin replication lag-report --replica <id>
   ```
3. Check for write burst on the primary (WAL segment production rate spike).
4. Verify replica is not under resource pressure (CPU / I/O saturation).

### Mitigation
- If network bandwidth is saturated: apply WAL shipping throttle:
  ```
  themis-admin replication set-ship-rate --replica <id> --mbps 40
  ```
- If replica is I/O bound: pause non-critical background tasks on the replica.
- If lag exceeds 10× the SLO: consider temporarily removing the replica from
  the read-replica pool to avoid serving stale data.

### Escalation
Escalate if lag continues to grow after throttle is applied, or if the
lagging replica is the only candidate for promotion (DR exposure).

---

## Scenario 2 — Slot Exhaustion

### Description
The replication slot registry has reached its configured maximum
(`max_replication_slots`), preventing new consumer connections.

### Detection
- Log pattern: `[REPLICATION:SlotExhausted]`
- Prometheus: `replication_slots_active` equals `replication_slots_max`
- Alert: `ReplicationSlotExhaustedAlert`

### Triage Steps
1. List all active slots and their consumer status:
   ```
   themis-admin replication slots list
   ```
2. Identify stale or inactive slots (LSN not advancing for > 10 min).
3. Check if any CDC consumers have disconnected without releasing their slot.

### Mitigation
- Drop stale/inactive slots after confirming no active consumer:
  ```
  themis-admin replication slots drop --slot-name <name>
  ```
- Increase `max_replication_slots` if the current workload legitimately
  requires more slots (requires coordinator restart):
  ```
  themis-admin config set replication.max_replication_slots 64
  ```

### Escalation
Escalate if all slots appear active but the consumer count is lower than
expected — this may indicate ghost slot leaks requiring WAL inspection.

---

## Scenario 3 — CDC Divergence

### Description
CDC event consumers detect that the logical change stream has diverged from
the physical WAL — events are missing, duplicated, or out-of-order.

### Detection
- Log pattern: `[REPLICATION:CDCDivergence]`
- Prometheus: `replication_cdc_divergence_events_total` > 0
- Alert: `ReplicationCDCDivergenceAlert`

### Triage Steps
1. Identify the affected slot and the sequence gap from the log.
2. Compare the slot's confirmed-flush-LSN with the WAL position:
   ```
   themis-admin replication slots status --slot-name <name>
   ```
3. Check for recent coordinator failover — CDC state may not have been
   checkpointed before failover.
4. Verify that the consumer is correctly acknowledging LSNs.

### Mitigation
- If a gap is detected: reset the CDC slot to the last safe checkpoint:
  ```
  themis-admin replication slots reset --slot-name <name> --lsn <safe-lsn>
  ```
- Replay the missed WAL range from the consumer side if idempotent.
- If divergence is caused by consumer ACK lag: increase consumer thread count.

### Escalation
Escalate if the WAL segment containing the gap has already been archived
and is no longer available for replay — data completeness audit required.

---

## Scenario 4 — Failover Promotion Failure

### Description
An automatic or manual failover promotion attempt fails — the selected
candidate is unable to become the new primary.

### Detection
- Log pattern: `[REPLICATION:PromotionFailed]`
- Prometheus: `replication_promotion_failures_total` increments
- Alert: `ReplicationPromotionFailedAlert`

### Triage Steps
1. Check the promotion failure reason from `[REPLICATION:PromotionFailed]` log.
2. Verify the candidate replica's WAL lag at the time of promotion:
   ```
   themis-admin replication lag-report --replica <candidate-id>
   ```
3. Check placement constraints — was the candidate excluded by a DC policy?
   ```
   themis-admin replication placement-policy validate
   ```
4. Verify fencing tokens: ensure the old primary cannot re-join as primary.

### Mitigation
- If WAL lag caused the failure: wait for the candidate to catch up, then
  retry promotion:
  ```
  themis-admin replication promote --replica <candidate-id> --force
  ```
- If placement policy blocked promotion: temporarily relax the DC constraint
  for the duration of the incident.
- Apply manual fencing if the old primary is still reachable:
  ```
  themis-admin replication fence --node <old-primary-id>
  ```

### Escalation
Escalate if all candidates fail promotion — the cluster is in a primary-less
state; human intervention is required to select a safe recovery path.

---

## Scenario 5 — Backpressure Cascade

### Description
Write backpressure propagates from the replication queue to the ingestion
layer, causing client-visible write latency spikes or write rejections.

### Detection
- Log pattern: `[REPLICATION:LagStorm]` (secondary) plus application-level
  timeout errors
- Prometheus: `replication_backpressure_stalls_total` rises sharply
- Alert: `ReplicationBackpressureCascadeAlert`

### Triage Steps
1. Confirm that the backpressure is originating in the replication queue
   (not network or disk I/O saturation on the application side).
2. Check replica catch-up rate:
   ```
   themis-admin replication throughput --replica <id>
   ```
3. Identify the slowest replica causing queue growth.
4. Verify write rate vs. WAL ship capacity ratio.

### Mitigation
- Temporarily reduce the `max_replication_lag_ms` grace period to shed
  the slowest replica earlier.
- If the slow replica is critical: horizontally scale the replica or
  increase its I/O capacity.
- Emergency: enable write-ahead admission control to cap ingestion rate:
  ```
  themis-admin replication admission-control enable --max-queue-depth 256
  ```

### Escalation
Escalate if admission control is already active but client write errors
continue — the backpressure source may be a disk or network hardware fault.

---

## Quick Reference — Log Patterns

| Pattern                          | Scenario                  |
|----------------------------------|---------------------------|
| `[REPLICATION:LagStorm]`         | Replication lag storm      |
| `[REPLICATION:SlotExhausted]`    | Slot exhaustion            |
| `[REPLICATION:CDCDivergence]`    | CDC divergence             |
| `[REPLICATION:PromotionFailed]`  | Failover promotion failure |
| `[REPLICATION:LagStorm]`         | Backpressure cascade       |

## Related Documents
- `src/replication/ROADMAP.md` — module roadmap
- `docs/operability/RUNBOOK_REPLICATION_LAG_FAILOVER.md` — existing lag/failover runbook
- `docs/operability/WAVE_D_ROADMAP.md` — Wave D operability plan
- `include/replication/replication_api_contract.h` — error taxonomy
