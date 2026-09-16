# Runbook: Observability — Wave D Operability

<!-- Runbook: observability | Wave D | validated: 2026-09-16 -->
<!-- Links: src/observability/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `observability`
module. Use it to diagnose and remediate metric drop storms, tracing buffer
overflows, profiling stalls, and cardinality explosion incidents.

---

## Scenario 1 — Metric Drop Storm

**Log pattern:** `[OBS:MetricDropStorm]`

### Symptoms
- Metrics are being dropped at a high rate due to queue pressure.
- `[OBS:MetricDropStorm]` emitted with `metric_name`, `dropped_count`, `drop_rate_per_sec`, and `queue_depth` fields.
- Monitoring dashboards may show gaps in time-series data.

### Diagnosis
1. Confirm metric drops:
   ```
   grep '\[OBS:MetricDropStorm\]' /var/log/themisdb/observability.log
   ```
2. Review `dropped_count` and `drop_rate_per_sec`.
3. Check the export backend ingestion rate vs. metric production rate.
4. Review `obs_metric_drop_total` and `obs_queue_depth` metrics.

### Remediation
1. Increase metric queue capacity: `observability.metric_queue_capacity`.
2. Scale out metric consumers or export backend.
3. Apply metric sampling or aggregation upstream.
4. Confirm drop rate returns to zero and dashboards show no gaps.

### Escalation
Escalate to the observability team if metric drop rate exceeds 0.1% for more than 2 minutes.

---

## Scenario 2 — Tracing Buffer Overflow

**Log pattern:** `[OBS:TracingBufferOverflow]`

### Symptoms
- Tracing span buffer fills up; spans are dropped before export.
- `[OBS:TracingBufferOverflow]` emitted with `span_buffer_depth`, `overflow_count`, and `exporter_lag_ms` fields.
- Distributed traces may have gaps; sampling may activate automatically.

### Diagnosis
1. Confirm tracing buffer overflow:
   ```
   grep '\[OBS:TracingBufferOverflow\]' /var/log/themisdb/observability.log
   ```
2. Identify `exporter_lag_ms` to determine if exporter is the bottleneck.
3. Check OTLP/Jaeger collector health.
4. Review `obs_trace_buffer_overflow_total` metric.

### Remediation
1. Increase span buffer size: `observability.tracing.buffer_capacity`.
2. Restore OTLP/Jaeger collector connectivity.
3. Activate tail-based sampling to reduce span volume: `observability.tracing.sampling_rate`.
4. Confirm overflow count returns to zero.

### Escalation
Escalate to the infrastructure team if OTLP/Jaeger collector is unavailable for more than
5 minutes.

---

## Scenario 3 — Profiling Stall

**Log pattern:** `[OBS:ProfilingStall]`

### Symptoms
- Continuous profiler stops producing samples.
- `[OBS:ProfilingStall]` emitted with `profiler_id`, `stall_duration_ms`, and `last_sample_age_ms` fields.
- Flame graphs and hot-path data may be stale or missing.

### Diagnosis
1. Confirm profiling stalls:
   ```
   grep '\[OBS:ProfilingStall\]' /var/log/themisdb/observability.log
   ```
2. Identify `stall_duration_ms` and `last_sample_age_ms`.
3. Check profiler process status:
   ```
   systemctl status themisdb-profiler
   ```
4. Review `obs_profiler_sample_age_ms` metric.

### Remediation
1. Restart profiler: `systemctl restart themisdb-profiler`.
2. If perf_event is unavailable: check kernel capabilities and `CAP_SYS_ADMIN`.
3. Verify profiler configuration: `observability.profiling.enabled=true`.
4. Confirm profiler resumes producing samples after restart.

### Escalation
Escalate to the infrastructure team if profiler cannot restart or if kernel capabilities
are missing.

---

## Scenario 4 — Cardinality Explosion

**Log pattern:** `[OBS:CardinalityExplosion]`

### Symptoms
- High-cardinality label combination causes metric storage or export to fail.
- `[OBS:CardinalityExplosion]` emitted with `metric_name`, `label_cardinality`, `threshold`, and `top_labels` fields.
- Prometheus storage may degrade; metric query latency may spike.

### Diagnosis
1. Confirm cardinality explosion:
   ```
   grep '\[OBS:CardinalityExplosion\]' /var/log/themisdb/observability.log
   ```
2. Identify `metric_name` and `top_labels` (highest-cardinality label dimensions).
3. Check Prometheus cardinality:
   ```
   curl http://prometheus:9090/api/v1/label/__name__/values | jq '.data | length'
   ```
4. Review `obs_metric_cardinality` metric.

### Remediation
1. Apply label allowlist to drop high-cardinality labels:
   `observability.cardinality.label_allowlist=["env","service","region"]`.
2. Aggregate high-cardinality series at the producer level.
3. Reduce cardinality by removing unbounded label dimensions (e.g., `request_id`).
4. Confirm cardinality returns below threshold after label reduction.

### Escalation
Escalate to the observability team if cardinality exceeds 10x threshold or if Prometheus
storage health is impacted.

---

## Scenario 5 — Mixed Metrics and Tracing Degradation Under Sustained Load

**Log pattern:** `[OBS:MetricDropStorm]` and `[OBS:TracingBufferOverflow]` in same time window

### Symptoms
- Both metric drops and tracing buffer overflows occur simultaneously.
- Export backend or network is the common bottleneck.
- `obs_export_queue_depth` metric elevated for both metrics and spans.

### Diagnosis
1. Identify concurrent incidents:
   ```
   grep -E 'MetricDropStorm|TracingBufferOverflow' /var/log/themisdb/observability.log | head -30
   ```
2. Check export backend health (Prometheus, Jaeger, OTLP collector).
3. Review network bandwidth utilization between ThemisDB and export backends.

### Remediation
1. Prioritize export backend restoration.
2. Apply temporary sampling for both metrics and traces to reduce pressure.
3. Increase export batch size to improve throughput after backend recovers.
4. Confirm both drop storm and buffer overflow stop after backend restoration.

### Escalation
Escalate to the infrastructure team if both backends are simultaneously unavailable.
