# Runbook: Exporters — Wave D Operability

<!-- Runbook: exporters | Wave D | validated: 2026-09-16 -->
<!-- Links: src/exporters/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `exporters`
module. Use it to diagnose and remediate exporter backend unavailability, metric
drop storms, trace buffer overflow, export queue stall, and cardinality explosion.

---

## Scenario 1 — Exporter Backend Unavailable

**Log pattern:** `[EXPORTER:BackendUnavailable]`

### Symptoms
- Export operations fail with backend connectivity errors.
- `[EXPORTER:BackendUnavailable]` emitted with `backend_type`, `endpoint`, and `error` fields.
- Metric or trace data may queue up or be dropped depending on fail-closed policy.

### Diagnosis
1. Confirm backend unavailability:
   ```
   grep '\[EXPORTER:BackendUnavailable\]' /var/log/themisdb/exporters.log
   ```
2. Identify `backend_type` (Prometheus, Jaeger, OTLP, HuggingFace Hub) and `endpoint`.
3. Test connectivity to the export endpoint:
   ```
   curl -v <endpoint>/health  # or the relevant health check URL
   ```
4. Review retry counters in monitoring (`exporter_hub_upload_failures_total`).

### Remediation
1. Restore connectivity to the export backend.
2. If backend is permanently unavailable, update the exporter endpoint configuration
   (`exporters.backend_endpoint`) and restart the exporter.
3. Flush the in-memory export queue after connectivity is restored to drain buffered data.
4. Confirm `[EXPORTER:BackendConnected]` appears in logs after recovery.

### Escalation
Escalate to the infrastructure team if the backend is unavailable for more than
5 minutes or if export queue depth exceeds the configured high-water mark.

---

## Scenario 2 — Metric Drop Storm

**Log pattern:** `[EXPORTER:MetricDropStorm]`

### Symptoms
- Metrics are being dropped at a high rate due to queue pressure.
- `[EXPORTER:MetricDropStorm]` emitted with `dropped_count`, `drop_rate_per_sec`, and `queue_depth`.
- Monitoring dashboards may show gaps in time-series data.

### Diagnosis
1. Confirm metric drops:
   ```
   grep '\[EXPORTER:MetricDropStorm\]' /var/log/themisdb/exporters.log
   ```
2. Review `dropped_count` and `drop_rate_per_sec`.
3. Check the export backend ingestion rate vs. the ThemisDB metric production rate.
4. Verify `exporter_policy_denials_total` metric for any policy-related drops.

### Remediation
1. Increase export queue capacity (`exporters.metric_queue_capacity`).
2. Scale out the backend (increase Prometheus remote write workers, OTLP collector capacity).
3. Apply metric sampling or aggregation upstream to reduce production rate.
4. After recovery, confirm `dropped_count` returns to `0` and dashboards show no gaps.

### Escalation
Escalate to the observability team if metric drop rate exceeds 0.1 % of total
metrics for more than 2 minutes.

---

## Scenario 3 — Trace Buffer Overflow

**Log pattern:** `[EXPORTER:TraceBufferOverflow]`

### Symptoms
- Trace spans are being dropped because the trace buffer is full.
- `[EXPORTER:TraceBufferOverflow]` emitted with `stream_id`, `dropped_spans`, and `buffer_capacity`.
- Distributed tracing data has gaps; trace reconstruction is incomplete.

### Diagnosis
1. Confirm trace buffer overflow:
   ```
   grep '\[EXPORTER:TraceBufferOverflow\]' /var/log/themisdb/exporters.log
   ```
2. Review `dropped_spans` and `buffer_capacity`.
3. Check Jaeger/OTLP collector health and ingestion rate.
4. Inspect trace span production rate per service.

### Remediation
1. Increase trace buffer capacity (`exporters.trace_buffer_capacity`).
2. Reduce trace sampling rate for high-throughput non-critical paths
   (`exporters.trace_sample_rate`).
3. Scale up the trace collector backend.
4. After buffer pressure subsides, confirm `dropped_spans` returns to `0`.

### Escalation
Escalate to the tracing infrastructure team if dropped spans affect root cause
analysis for active incidents.

---

## Scenario 4 — Export Queue Stall

**Log pattern:** `[EXPORTER:QueueStall]`

### Symptoms
- The export queue stops draining; items accumulate without being processed.
- `[EXPORTER:QueueStall]` emitted with `queue_depth`, `drain_rate`, and `stall_duration_ms`.
- Export latency increases; backend may show no new data.

### Diagnosis
1. Confirm queue stall:
   ```
   grep '\[EXPORTER:QueueStall\]' /var/log/themisdb/exporters.log
   ```
2. Review `stall_duration_ms` and `drain_rate`.
3. Check export worker thread health — are workers blocked or deadlocked?
   ```
   # On Linux, inspect thread states:
   cat /proc/$(pgrep themisdb)/status | grep Threads
   ```
4. Look for backend connection errors that may be blocking the drain loop.

### Remediation
1. If workers are blocked on a backend connection, fix or bypass the backend
   (see Scenario 1 for backend unavailability steps).
2. Restart export workers via the admin API (`POST /admin/exporters/restart`).
3. If the queue depth has grown very large, enable drain-only mode to prioritise
   flushing before accepting new items.
4. After recovery, confirm `drain_rate` returns to the expected baseline.

### Escalation
Escalate to the platform team if the stall exceeds 10 minutes or if a
restart of export workers does not resolve it.

---

## Scenario 5 — Cardinality Explosion

**Log pattern:** `[EXPORTER:CardinalityExplosion]`

### Symptoms
- The number of unique metric label combinations grows unbounded.
- `[EXPORTER:CardinalityExplosion]` emitted with `metric_name`, `unique_series_count`, and `threshold`.
- Backend time-series databases may degrade or OOM due to excessive label space.

### Diagnosis
1. Confirm cardinality explosion:
   ```
   grep '\[EXPORTER:CardinalityExplosion\]' /var/log/themisdb/exporters.log
   ```
2. Review `metric_name` and `unique_series_count` — identify the unbounded label.
3. Inspect the metric generation path for the identified metric to find the
   high-cardinality label (often user IDs, request IDs, or free-form strings).
4. Check Prometheus/OTLP backend cardinality limits.

### Remediation
1. Drop or hash the high-cardinality label in the metric generation path.
2. Apply a cardinality cap in the exporter configuration
   (`exporters.max_series_per_metric`).
3. Evict stale series from the backend if they are no longer needed.
4. After applying the cap, confirm `unique_series_count` falls below `threshold`.

### Escalation
Escalate to the observability engineering team if cardinality explosion is
caused by a production workload characteristic that requires design changes
to the metric labelling scheme.

---

## References

- `src/exporters/ROADMAP.md` — Wave D operability contribution
- `include/exporters/exporters_api_contract.h` — error taxonomy and contracts
- `include/exporters/exporter_errors.h` — `PolicyDeniedException` and error codes
- `tests/integration/test_exporters_soak.cpp` — Wave D soak tests
- `tests/exporters/test_exporters_highcardinality_stress.cpp` — stress tests
- `benchmarks/exporters/bench_exporters_dedicated_gates.cpp` — benchmark gates
