# Access Model Coordinator — Operator Dashboard Guide

**Document Version:** 1.0.0  
**Target Audience:** SREs, Database Operators  
**Date:** 2026-08-17  

---

## Overview

This guide explains how to visualize and interpret Access Model Coordinator metrics in an operator dashboard (Grafana, Datadog, etc.).

---

## Recommended Dashboard Panels

### Panel 1: Promotion Health (Top-Level Status)

**Query (Prometheus):**
```promql
promotion_success_rate = (
  rate(access_model_promotion_successes_total[5m])
  / (rate(access_model_promotion_attempts_total[5m]) + 0.0001)
) * 100
```

**Display:**
- Gauge showing percentage (0-100%)
- Color coding: GREEN >99%, YELLOW 95-99%, RED <95%
- Target: ≥99% success rate

**Interpretation:**
- **GREEN (≥99%):** Promotions executing reliably
- **YELLOW (95-99%):** Some rejections, but acceptable
- **RED (<95%):** Significant failure rate, investigate (see runbooks)

---

### Panel 2: Event Queue Depth (Backlog Indicator)

**Query:**
```promql
event_queue_depth
```

**Display:**
- Line graph, 6-hour time window
- Alert line at 5000 (warning threshold)
- Y-axis: 0-10000 events

**Interpretation:**
- **Flat near 0-1000:** Queue is healthy, no backlog
- **Rising trend:** Events accumulating, workers saturated
- **Spike >5000:** Immediate investigation required (see Symptom 2)

---

### Panel 3: Promotion Latency Percentiles

**Query:**
```promql
{
  p50: histogram_quantile(0.5, access_model_promotion_latency_ms),
  p95: histogram_quantile(0.95, access_model_promotion_latency_ms),
  p99: histogram_quantile(0.99, access_model_promotion_latency_ms)
}
```

**Display:**
- Line graph with 3 lines (p50, p95, p99)
- Overlay gates: p50 ≤10µs, p95 ≤30µs, p99 ≤50µs (green zones)
- Time window: 1 hour

**Interpretation:**
- **All lines in green zone:** Performance nominal
- **p99 approaching 50µs:** Monitor for trend
- **p99 > 50µs sustained:** Investigate storage I/O (see Symptom 4)

---

### Panel 4: Policy Decision Breakdown

**Query:**
```promql
sum by (decision) (
  rate(access_model_policy_decisions_total[5m])
)
```

**Display:**
- Stacked bar chart, 1-hour rolling window
- Segments: PROMOTE, REJECT, DEFER (different colors)
- Y-axis: events/sec

**Interpretation:**
- **High PROMOTE rate:** Active tier transitions (expected during load)
- **Increasing REJECT rate:** Policy thresholds too strict, consider relaxing
- **Increasing DEFER rate:** Queue overload, check event_queue_depth

---

### Panel 5: Memory Usage Trend

**Query:**
```promql
access_model_memory_bytes
```

**Display:**
- Area graph, 24-hour time window
- Alert line at 200MB (warning)
- Y-axis: 0-500MB

**Interpretation:**
- **Stable at 50-100MB:** Normal operation
- **Gradual increase over hours:** Possible memory leak (investigate)
- **Spike >200MB:** Event queue buildup (see Symptom 3)

---

### Panel 6: Worker Thread Utilization

**Query:**
```promql
worker_thread_active_tasks
/ (worker_thread_count * 100)
```

**Display:**
- Gauge showing percentage (0-100%)
- Color coding: GREEN <50%, YELLOW 50-80%, RED >80%

**Interpretation:**
- **GREEN (<50%):** Workers have headroom for more work
- **YELLOW (50-80%):** Approaching saturation, monitor
- **RED (>80%):** Consider increasing worker pool size

---

### Panel 7: Tier Promotion Flow (Sankey Diagram)

**Query (Custom):**
```promql
sum by (from_tier, to_tier) (
  rate(access_model_tier_transitions_total[5m])
)
```

**Display:**
- Sankey diagram showing flow between tiers
- Node sizes proportional to transition rate
- Arrow thickness = event count

**Interpretation:**
- Expected flow: COLD→WARM→L2→L1 (right-to-left)
- Unexpected flow: L1→L2 (demotion) should be rare
- Stalled flow: One tier with no outflow (investigate policy)

---

### Panel 8: Cache Eviction Feedback Loop

**Query:**
```promql
{
  eviction_rate: rate(cache_eviction_events_total[5m]),
  feedback_rate: rate(access_model_eviction_feedback_processed[5m])
}
```

**Display:**
- Dual-line graph, 1-hour window
- Green line (feedback_rate) should track blue line (eviction_rate)

**Interpretation:**
- **Lines aligned:** Cache↔Coordinator integration healthy
- **Feedback lagging eviction:** Events queued, check event_queue_depth
- **Feedback > eviction:** Configuration issue (feedback rate > generation rate)

---

## Sample Grafana Dashboards

### Quick-Win Dashboard (5-minute scan)

```json
{
  "title": "Access Model Coordinator — Status",
  "panels": [
    {
      "title": "Promotion Success Rate",
      "type": "gauge",
      "targets": [{"expr": "promotion_success_rate"}],
      "thresholds": [95, 99]
    },
    {
      "title": "Event Queue Depth",
      "type": "graph",
      "targets": [{"expr": "event_queue_depth"}],
      "alertThreshold": 5000
    },
    {
      "title": "Promotion Latency (p99)",
      "type": "graph",
      "targets": [{"expr": "histogram_quantile(0.99, promotion_latency_ms)"}],
      "threshold": 50
    },
    {
      "title": "Worker Utilization",
      "type": "gauge",
      "targets": [{"expr": "worker_thread_utilization_percent"}],
      "thresholds": [50, 80]
    }
  ]
}
```

### Deep-Dive Dashboard (15-minute analysis)

```json
{
  "title": "Access Model Coordinator — Deep Dive",
  "panels": [
    {
      "title": "Event Processing Pipeline",
      "type": "graph",
      "targets": [
        {"expr": "rate(eviction_events_total[5m])", "legend": "Evictions"},
        {"expr": "rate(promotion_events_total[5m])", "legend": "Promotions"},
        {"expr": "rate(policy_decisions_total[5m])", "legend": "Decisions"}
      ]
    },
    {
      "title": "Latency Distribution",
      "type": "heatmap",
      "targets": [{"expr": "promotion_latency_ms_bucket"}]
    },
    {
      "title": "Tier Migration Rates",
      "type": "graph",
      "targets": [
        {"expr": "rate(l1_to_l2_migrations[5m])", "legend": "L1→L2"},
        {"expr": "rate(l2_to_l3_migrations[5m])", "legend": "L2→L3"},
        {"expr": "rate(cold_to_warm_migrations[5m])", "legend": "COLD→WARM"}
      ]
    },
    {
      "title": "Memory Consumption Breakdown",
      "type": "piechart",
      "targets": [
        {"expr": "event_queue_memory_bytes", "legend": "Event Queue"},
        {"expr": "coordinator_state_memory_bytes", "legend": "Coordinator State"},
        {"expr": "metrics_buffer_memory_bytes", "legend": "Metrics Buffer"}
      ]
    }
  ]
}
```

---

## Alert Configuration

### Recommended Prometheus Alert Rules

```yaml
groups:
  - name: access_model
    interval: 30s
    rules:
      # High event queue depth
      - alert: AccessModelQueueBacklog
        expr: event_queue_depth > 5000
        for: 2m
        annotations:
          summary: "Access Model event queue backed up"
          description: "Queue depth {{ $value }} exceeds threshold"
          runbook: "docs/operations/ACCESS_MODEL_RUNBOOKS.md#symptom-2"

      # Low promotion success rate
      - alert: AccessModelPromotionFailure
        expr: promotion_success_rate < 0.95
        for: 5m
        annotations:
          summary: "Promotion success rate dropped below 95%"
          runbook: "docs/operations/ACCESS_MODEL_RUNBOOKS.md#symptom-1"

      # High promotion latency
      - alert: AccessModelHighLatency
        expr: histogram_quantile(0.99, promotion_latency_ms) > 100
        for: 2m
        annotations:
          summary: "Promotion p99 latency exceeded 100µs"
          runbook: "docs/operations/ACCESS_MODEL_RUNBOOKS.md#symptom-4"

      # Memory spike
      - alert: AccessModelMemorySpike
        expr: access_model_memory_bytes > 200000000
        for: 2m
        annotations:
          summary: "Coordinator memory usage exceeds 200MB"
          runbook: "docs/operations/ACCESS_MODEL_RUNBOOKS.md#symptom-3"

      # Worker threads stuck
      - alert: AccessModelWorkerStuck
        expr: time() - access_model_last_worker_activity_timestamp > 60
        for: 1m
        annotations:
          summary: "Access Model worker threads appear stuck"
          runbook: "docs/operations/ACCESS_MODEL_RUNBOOKS.md#symptom-2"
```

---

## Query Examples for Investigation

### Find Slow Promotions (Last Hour)

```promql
# Promotions taking >100µs
histogram_quantile(0.99, rate(promotion_latency_ms_bucket[1h]))
  > 100

# Or: exact slowest promotions
topk(10, rate(promotion_latency_ms_bucket{le="100"}[1h]))
```

### Identify Policy Conflicts

```promql
# Rapid promote + demote on same key
increase(policy_conflicts_total[5m]) > 0

# or examine decision distribution
sum by (decision) (increase(policy_decisions_total[5m]))
```

### Calculate Promotion Efficiency

```promql
# Successful promotions as % of all events
(rate(promotion_successes_total[5m])
  / (rate(eviction_events_total[5m]) + rate(promotion_events_total[5m])))
* 100
```

### Memory Leak Detection

```promql
# Linear growth over 6 hours?
deriv(access_model_memory_bytes[6h]) > 0

# Peak memory in last 24h
max_over_time(access_model_memory_bytes[24h])
```

---

## Performance Baseline Capture

**Recommended Schedule:** Weekly (Sundays 02:00 UTC, off-peak)

```bash
#!/bin/bash
# Capture performance baseline

DATE=$(date +%Y-%m-%d)
OUTDIR="/var/log/themisdb/baselines/$DATE"
mkdir -p "$OUTDIR"

# Export metrics
curl http://localhost:8080/metrics > "$OUTDIR/metrics.txt"

# Capture detailed stats
curl http://localhost:8080/debug/access_model/stats > "$OUTDIR/detailed_stats.json"

# Gate verification
curl http://localhost:8080/debug/access_model/gates > "$OUTDIR/gates_verification.json"

echo "Baseline captured: $OUTDIR"
```

---

## Dashboard Implementation Checklist

- [ ] Create Prometheus data source (localhost:9090 or remote)
- [ ] Create main status dashboard (Panel 1-4)
- [ ] Create deep-dive dashboard (Panel 5-8)
- [ ] Configure alert rules (5 critical alerts)
- [ ] Add dashboard annotations for deployments/maintenance
- [ ] Create runbook links in alert descriptions
- [ ] Test alerts in staging environment
- [ ] Document custom metrics in team wiki
- [ ] Schedule monthly dashboard review
- [ ] Capture performance baseline (weekly, off-peak)

---

## References

- **Runbooks:** `docs/operations/ACCESS_MODEL_RUNBOOKS.md`
- **Architecture:** `docs/architecture/UNIFIED_ACCESS_MODEL.md`
- **Metrics Reference:** `src/access_model/access_metrics.h`

---

**Status:** 2026-08-17 DRAFT  
**Next Review:** Post-Phase 6 (after testing complete)

