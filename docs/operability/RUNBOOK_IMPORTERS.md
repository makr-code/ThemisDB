# Runbook: Importers — Wave D Operability

<!-- Runbook: importers | Wave D | validated: 2026-09-16 -->
<!-- Links: src/importers/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `importers`
module. Use it to diagnose and remediate connector loss, schema drift failures,
unresolved conflict strategies, and ingest overflow incidents.

---

## Scenario 1 — Connector Lost

**Log pattern:** `[IMPORTER:ConnectorLost]`

### Symptoms
- Connector loses connectivity to the upstream data source.
- `[IMPORTER:ConnectorLost]` emitted with `connector_id`, `source_type`, `endpoint`, and `retry_count` fields.
- Ingest pipeline stalls; queue depth may grow.

### Diagnosis
1. Confirm connector loss:
   ```
   grep '\[IMPORTER:ConnectorLost\]' /var/log/themisdb/importers.log
   ```
2. Identify `connector_id` and `source_type` (CDC, stream, file, API).
3. Test upstream endpoint connectivity:
   ```
   curl -v <endpoint>/health
   ```
4. Review `importers_connector_disconnect_total` metric.

### Remediation
1. Restore upstream connectivity; verify network path.
2. Trigger connector reconnect: `themisdb-admin importers reconnect --connector-id <id>`.
3. Drain ingest queue backlog after reconnect.
4. Confirm `[IMPORTER:ConnectorLost]` stops appearing.

### Escalation
Escalate to the infrastructure team if connector remains disconnected for more than 5 minutes
or if queue depth exceeds the configured high-water mark.

---

## Scenario 2 — Schema Drift Failed

**Log pattern:** `[IMPORTER:SchemaDriftFailed]`

### Symptoms
- Schema drift detection or adaptation fails for an incoming record.
- `[IMPORTER:SchemaDriftFailed]` emitted with `connector_id`, `schema_version`, `expected_version`, and `drift_type` fields.
- Records may be quarantined or dropped.

### Diagnosis
1. Confirm schema drift failures:
   ```
   grep '\[IMPORTER:SchemaDriftFailed\]' /var/log/themisdb/importers.log
   ```
2. Identify `drift_type` (new_field, missing_field, type_change, renamed_field).
3. Compare current schema against registry:
   ```
   themisdb-admin importers show-schema --connector-id <id>
   ```
4. Review `importers_schema_drift_failure_total` metric.

### Remediation
1. For backward-compatible drifts: update schema registry:
   `themisdb-admin importers update-schema --connector-id <id> --version <new>`.
2. For breaking drifts: engage the upstream data team to align schema.
3. Replay quarantined records after schema update.
4. Confirm `[IMPORTER:SchemaDriftFailed]` stops appearing.

### Escalation
Escalate to the data engineering team if drift involves breaking changes or if quarantined
record count exceeds acceptable threshold.

---

## Scenario 3 — Conflict Unresolved

**Log pattern:** `[IMPORTER:ConflictUnresolved]`

### Symptoms
- Conflict resolution strategy fails to produce a deterministic outcome.
- `[IMPORTER:ConflictUnresolved]` emitted with `record_id`, `conflict_strategy`, and `conflict_type` fields.
- Records may be quarantined; data consistency may be at risk.

### Diagnosis
1. Confirm unresolved conflicts:
   ```
   grep '\[IMPORTER:ConflictUnresolved\]' /var/log/themisdb/importers.log
   ```
2. Identify `conflict_strategy` (last-write-wins, merge, reject) and `conflict_type`.
3. Inspect quarantined records:
   ```
   themisdb-admin importers list-quarantine --connector-id <id>
   ```
4. Review `importers_conflict_unresolved_total` metric.

### Remediation
1. For last-write-wins failures: check timestamp accuracy on source records.
2. For merge failures: inspect merge function implementation in connector config.
3. Manually resolve quarantined records: `themisdb-admin importers resolve-conflict --record-id <id>`.
4. Confirm conflict resolution rate returns to zero.

### Escalation
Escalate to the data team if more than 100 records remain unresolved or if conflict
strategy configuration is ambiguous.

---

## Scenario 4 — Ingest Overflow

**Log pattern:** `[IMPORTER:IngestOverflow]`

### Symptoms
- Ingest queue depth exceeds the configured high-water mark.
- `[IMPORTER:IngestOverflow]` emitted with `connector_id`, `queue_depth`, and `drop_count` fields.
- Records may be dropped or rejected; downstream consumers may receive incomplete data.

### Diagnosis
1. Confirm ingest overflow:
   ```
   grep '\[IMPORTER:IngestOverflow\]' /var/log/themisdb/importers.log
   ```
2. Identify `connector_id`, `queue_depth`, and `drop_count`.
3. Check ingest throughput vs. consumer throughput:
   ```
   grep 'ingest_rate' /var/log/themisdb/importers.log | tail -10
   ```
4. Review `importers_queue_depth` and `importers_drop_rate` metrics.

### Remediation
1. Scale up consumers: `themisdb-admin importers scale --connector-id <id> --consumers +2`.
2. Increase queue capacity: `importers.queue.max_depth` in config.
3. Apply back-pressure to upstream source if scaling is not immediate.
4. Confirm `importers_queue_depth` returns below high-water mark.

### Escalation
Escalate to the platform team if drop rate exceeds 0.1% for more than 2 minutes.

---

## Scenario 5 — Multi-Connector Throughput Degradation Under Schema Drift Pressure

**Log pattern:** `[IMPORTER:SchemaDriftFailed]` with `connector_count>1`

### Symptoms
- Multiple connectors simultaneously experience schema drift.
- Ingest throughput drops across all affected connectors.
- `importers_connector_throughput_p99_ms` metric elevated.

### Diagnosis
1. Identify connectors with concurrent drift:
   ```
   grep 'SchemaDriftFailed' /var/log/themisdb/importers.log | awk '{print $4}' | sort | uniq -c
   ```
2. Check whether a schema registry outage is contributing.
3. Review connector schema version mismatch rate.

### Remediation
1. If schema registry is unavailable: fail-open with cached schemas until registry recovers.
2. Prioritize resolving drifts for highest-throughput connectors first.
3. Apply schema migrations in a coordinated batch:
   `themisdb-admin importers batch-migrate-schemas --file <migrations.json>`.
4. Confirm all connectors return to normal throughput.

### Escalation
Escalate to the data engineering team if schema registry is unavailable for more than
5 minutes.
