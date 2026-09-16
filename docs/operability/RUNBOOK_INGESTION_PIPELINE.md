# Runbook: Ingestion Pipeline

<!-- Wave D operability deliverable — ingestion module -->
<!-- Source: src/ingestion/ROADMAP.md § Wave D Contribution -->

**Module:** `src/ingestion/`  
**Version:** 1.0.0 (Wave D, 2026-Q1)  
**Owner:** ThemisDB Ingestion Team  
**Labels:** `wave_d;operability;runbook`

---

## Overview

This runbook covers operator response procedures for the five most critical
ingestion pipeline incident classes. Each scenario includes detection signals
(log patterns and metrics), immediate mitigations, and escalation paths.

---

## Scenario 1 — Ingest Overflow

**Log pattern:** `[INGESTION:IngestOverflow]`

### Detection

- Log line: `[INGESTION:IngestOverflow] queue_depth=<N> capacity=<C> source=<S>`
- Metric: `ingestion_queue_depth` reaches `ingestion_queue_capacity`
- Alert: `IngestionQueueNearCapacity` fires at 90 % fill.

### Immediate actions

1. **Check producer rate.** Run `ctest -L ingestion` or inspect connector metrics
   to confirm which source is emitting records above the consumer drain rate.
2. **Enable backpressure on the source connector.** Set
   `connector.<name>.backpressure_enabled=true` in the connector config.
3. **Scale consumers.** Add ingestion worker threads via
   `ingestion.consumer_threads=<N>` in the runtime config.
4. **Temporarily reduce producer batch size** on the offending source if
   backpressure cannot be applied immediately.

### Escalation

If the queue does not drain within 10 minutes after applying the above steps,
escalate to the on-call ingestion lead. Include the full log excerpt and the
connector name.

---

## Scenario 2 — Schema Validation Failure

**Log pattern:** `[INGESTION:SchemaValidationFailed]`

### Detection

- Log line: `[INGESTION:SchemaValidationFailed] record_id=<ID> field=<F> reason=<R>`
- Metric: `ingestion_schema_validation_failures_total` rising
- Alert: `IngestionSchemaValidationErrorRate` fires when failure rate > 1 %.

### Immediate actions

1. **Identify the offending field and source.** Check the log's `field=` and
   `source=` tags.
2. **Quarantine the affected batch.** Set `ingestion.quarantine_on_schema_error=true`
   to route failing records to the dead-letter queue instead of dropping silently.
3. **Inspect the schema definition.** Confirm `include/ingestion/ingestion_api_contract.h`
   reflects the expected field types.
4. **Replay the quarantined batch** after correcting the source schema.

### Escalation

Schema failures that affect > 5 % of records for a given source should be
escalated immediately — this typically signals a breaking change in the upstream
data contract.

---

## Scenario 3 — Dead-Letter Queue Full

**Log pattern:** `[INGESTION:DLQFull]`

### Detection

- Log line: `[INGESTION:DLQFull] dlq_size=<N> max_size=<M>`
- Metric: `ingestion_dlq_depth` at or above `ingestion_dlq_max_depth`
- Alert: `IngestionDLQNearCapacity`

### Immediate actions

1. **Do not discard records.** Confirm `ingestion.dlq_overflow_policy=block` is set
   to prevent silent loss.
2. **Triage the DLQ contents.** Run the DLQ inspector script:
   ```
   scripts/ingestion/dlq_inspect.sh --tail 100
   ```
3. **Replay or archive** correctable records.
4. **Increase DLQ capacity temporarily** via `ingestion.dlq_max_depth=<N>` if
   correctable volume is high.

### Escalation

A full DLQ with no drain progress after 30 minutes is a P1 incident.

---

## Scenario 4 — Backpressure Cascade

**Log pattern:** `[INGESTION:BackpressureCascade]`

### Detection

- Log line: `[INGESTION:BackpressureCascade] affected_connectors=<LIST>`
- Metric: multiple connector queues near capacity simultaneously
- Alert: `IngestionBackpressureCascade`

### Immediate actions

1. **Isolate the slowest consumer.** Check `ingestion_consumer_lag_ms` per
   connector to find the bottleneck.
2. **Reduce ingest rate globally.** Apply `ingestion.global_rate_limit_rps=<N>`.
3. **Check downstream storage health.** A storage backend outage is the most
   common root cause; verify with `themis_admin status storage`.
4. **Restore consumer capacity** once the downstream pressure is resolved.

### Escalation

Cascade lasting > 15 minutes with consumers failing to drain requires escalation
to the storage on-call as well as the ingestion lead.

---

## Scenario 5 — Source Connector Loss

**Log pattern:** `[INGESTION:SourceLost]`

### Detection

- Log line: `[INGESTION:SourceLost] connector=<NAME> last_record_at=<TS>`
- Metric: `ingestion_connector_active` drops to 0 for the affected source
- Alert: `IngestionSourceConnectorDown`

### Immediate actions

1. **Confirm connectivity.** Attempt a manual probe of the source endpoint.
2. **Check connector retry state.** Ingestion connectors use
   `toRetryTimeoutSource` / `toRetryExhaustionReason` taxonomy; check logs for
   `INGESTION_TIMEOUT` or `INGESTION_CONNECTOR_FAILED` codes.
3. **Enable fail-closed mode.** If the source cannot be recovered within SLA,
   set `connector.<name>.fail_closed=true` to stop silently missing records.
4. **Switch to a standby source** if one is configured under
   `connector.<name>.fallback_source`.

### Escalation

Connector loss with no recovery path after 5 minutes is a P2 incident for any
`release_critical` ingestion source.

---

## Related resources

- `src/ingestion/ROADMAP.md` — module roadmap and Wave D closure
- `tests/integration/test_ingestion_pipeline_soak.cpp` — soak test
- `tests/ingestion/test_ingestion_highcardinality_stress.cpp` — stress test
- `benchmarks/ingestion/bench_ingestion_dedicated_gates.cpp` — IN-BM-01..04
- `include/ingestion/ingestion_api_contract.h` — error taxonomy
