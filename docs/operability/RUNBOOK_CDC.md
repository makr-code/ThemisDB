# Runbook: CDC Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `cdc` (Change Data Capture) module.
Covers the five most critical incident classes, their log patterns, triage
steps, and recommended remediation actions.

---

## Scenario 1 — Transport Lost

**Log pattern:** `[CDC:TransportLost]`

**Symptoms**
- The CDC transport layer loses connectivity to the broker (Kafka, WebSocket, or internal).
- Log lines contain `[CDC:TransportLost]` with transport name and connection error.
- Change event publishing halts; downstream consumers stop receiving updates.

**Triage**
1. Identify the transport type and endpoint from the log.
2. Check broker health: `themis_admin cdc transport status`.
3. Verify network connectivity to the broker endpoint.
4. Review recent broker restarts, network partitions, or TLS certificate renewals.

**Remediation**
1. Trigger a transport reconnect: `themis_admin cdc transport reconnect`.
2. If the broker is unreachable, switch to the secondary transport:
   `themis_admin cdc transport failover`.
3. Verify broker credentials and TLS certificates are still valid.
4. Drain the in-memory event buffer to disk to prevent loss during reconnection:
   `themis_admin cdc buffer flush`.

**Escalation**
If transport cannot be restored within 5 minutes, escalate to the infrastructure
team with broker endpoint, TLS status, and the last 200 CDC transport log entries.

---

## Scenario 2 — Replay Failed

**Log pattern:** `[CDC:ReplayFailed]`

**Symptoms**
- A CDC event replay request fails to retrieve or re-publish historical events.
- Log lines contain `[CDC:ReplayFailed]` with sequence range and failure reason.
- Consumer groups that rely on replay fall behind or skip events.

**Triage**
1. Identify the failing sequence range and consumer group from the log.
2. Check replay controller health: `themis_admin cdc replay status`.
3. Verify that the replay source (WAL, event store, or backup) is accessible.
4. Check for retention policy expiry that may have deleted the replay source events.

**Remediation**
1. Retry the replay from the last successful sequence:
   `themis_admin cdc replay retry --from <seq_id>`.
2. If the source is missing due to retention, restore from backup or accept the gap.
3. Notify affected consumers of the sequence gap so they can apply compensating logic.
4. Adjust retention policy if events are expiring before all consumers can replay:
   set `cdc.retention.min_replay_window_hours`.

**Escalation**
Replay failures due to data loss require escalation to the data reliability
team with sequence range, retention policy, and consumer group identifiers.

---

## Scenario 3 — Lag Storm

**Log pattern:** `[CDC:LagStorm]`

**Symptoms**
- Consumer lag spikes to an unsustainable level across all consumer groups.
- Log lines contain `[CDC:LagStorm]` with consumer group name and lag measurement.
- Change propagation latency exceeds SLO; downstream views are stale.

**Triage**
1. Identify which consumer groups are lagging from the log and metrics.
2. Check if the lag is due to a producer throughput spike or a consumer slowdown.
3. Review consumer CPU and memory utilisation for resource exhaustion.
4. Check for a backlog in the transport broker partition queue.

**Remediation**
1. Scale up consumer threads: `themis_admin cdc consumer scale-up <group>`.
2. If the spike is temporary, allow consumers to catch up without intervention.
3. If a single consumer is bottlenecked, rebalance partitions:
   `themis_admin cdc consumer rebalance <group>`.
4. Temporarily increase producer batch intervals to reduce event rate:
   set `cdc.producer.batch_interval_ms`.

**Escalation**
A lag storm that cannot be resolved by scaling within 15 minutes should be
escalated to the capacity planning team with lag metrics, throughput graphs,
and consumer group configuration.

---

## Scenario 4 — Delivery Timeout

**Log pattern:** `[CDC:DeliveryTimeout]`

**Symptoms**
- Change events are not acknowledged by consumers within the delivery timeout.
- Log lines contain `[CDC:DeliveryTimeout]` with event ID and consumer group.
- Unacknowledged events accumulate in the dead-letter queue.

**Triage**
1. Identify the timed-out event IDs and the responsible consumer group.
2. Check dead-letter queue depth: `themis_admin cdc dlq status`.
3. Review consumer health and processing latency.
4. Check for downstream dependency failures that are blocking consumer processing.

**Remediation**
1. Increase the delivery timeout if consumers are legitimately slow:
   set `cdc.delivery.ack_timeout_ms`.
2. Retry delivery for events in the dead-letter queue:
   `themis_admin cdc dlq retry`.
3. Investigate and resolve the downstream dependency failure blocking consumers.
4. If dead-letter events cannot be replayed, apply compensating transactions.

**Escalation**
A growing dead-letter queue that cannot be drained indicates a persistent
consumer failure — escalate to the consumer application team with DLQ size,
event samples, and consumer error logs.

---

## Scenario 5 — Schema Registry Conflict

**Log pattern:** `[CDC:TransportLost]` + `[CDC:ReplayFailed]` (combined, schema mismatch)

**Symptoms**
- Events fail to deserialise at consumer due to schema version mismatch.
- Both transport errors and replay failures appear after a schema deployment.
- Log lines show `[CDC:ReplayFailed]` with `schema_mismatch` tag.

**Triage**
1. Identify the event schema version from the log.
2. Check registered schema versions: `themis_admin cdc schema list`.
3. Determine which producers and consumers are on incompatible versions.
4. Check the schema compatibility mode (`BACKWARD`, `FORWARD`, `FULL`).

**Remediation**
1. Register the missing schema version: `themis_admin cdc schema register`.
2. Roll back the producer to the previous schema version if forward compatibility is broken.
3. Update consumers to handle the new schema version before re-enabling producers.
4. Enforce compatibility checks in the CI pipeline to prevent future mismatches.

**Escalation**
Schema conflicts affecting production event streams require immediate
coordination between the producer and consumer teams — escalate with the
schema versions in conflict and the compatibility mode config.

---

## Reference

| Log Pattern             | Severity | SLO Impact | Owner |
|-------------------------|----------|------------|-------|
| `[CDC:TransportLost]`   | Critical | Yes        | cdc   |
| `[CDC:ReplayFailed]`    | High     | Partial    | cdc   |
| `[CDC:LagStorm]`        | High     | Yes        | cdc   |
| `[CDC:DeliveryTimeout]` | Medium   | Partial    | cdc   |

---

*Wave D operability deliverable — see `src/cdc/ROADMAP.md`.*
