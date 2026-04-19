# Observability Troubleshooting Guide

The `observability` module provides metrics collection (Prometheus), performance profiling, query profiling, storage profiling, alerting, and continuous profiling for ThemisDB.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `/metrics` endpoint returns 404 | Prometheus exporter not enabled | Set `observability.metrics.enabled: true` |
| Cardinality explosion OOM | High-cardinality labels | Remove dynamic labels; enable `label_limit` |
| Alert fires but service is healthy | Alert threshold too sensitive | Tune threshold or add `for` duration |
| Profiler adds > 10% CPU overhead | Sampling rate too high | Reduce `observability.profiler.sample_rate_hz` |
| Slow query profiler misses queries | Threshold too high | Lower `observability.query_profiler.threshold_ms` |
| PII in metric labels | User data used as label value | Audit labels; enable `observability.pii_scrubbing` |
| Storage profiler not initialising | RocksDB stats not enabled | Set `storage.rocksdb.enable_statistics: true` |
| Grafana shows no data | Wrong scrape interval or job name | Check Prometheus scrape config |
| `PerformanceAnalyzer: regression detected` | Load changed, not a regression | Set baseline after load change |
| Alert manager silenced by accident | Silence rule too broad | Review silence rules in Alertmanager |

## Common Issues

### Issue 1: Metrics Endpoint Returns 404

**Description:** The Prometheus scrape endpoint is not available.

**Symptoms:**
- `curl http://localhost:9100/metrics` returns `404 Not Found`
- Prometheus shows target as `DOWN`

**Cause:** Metrics exporter is disabled in configuration.

**Solution:**
```yaml
observability:
  metrics:
    enabled: true
    port: 9100
    path: /metrics
    format: prometheus
```
```bash
# Verify port is listening
ss -tlnp | grep 9100

# Test locally
curl -s http://localhost:9100/metrics | head -20
```

---

### Issue 2: Prometheus Cardinality Explosion Causes OOM

**Description:** Prometheus runs out of memory because ThemisDB emits metrics with too many unique label combinations.

**Symptoms:**
- Prometheus log: `out of order sample: type=counter`
- ThemisDB log: `MetricsCollector: active time series count=5000000`

**Cause:** A dynamic value (e.g., `query_hash`, `user_id`) is used as a metric label.

**Solution:**
```yaml
observability:
  metrics:
    label_limit: 100              # reject metrics with too many labels
    pii_scrubbing:
      enabled: true
      scrub_labels: [user_id, query_text, session_id]
    cardinality:
      warn_threshold: 10000
      max_series: 100000          # drop new series above this limit
```

---

### Issue 3: Query Profiler Misses Slow Queries

**Description:** The query profiler does not log queries that are visibly slow.

**Symptoms:**
- Users report slow queries but `slow_query.log` is empty
- Profiler threshold too high

**Cause:** `slow_query_threshold_ms` is too high; slow queries fall below threshold.

**Solution:**
```yaml
observability:
  query_profiler:
    enabled: true
    threshold_ms: 100            # capture queries >100ms (from default 5000ms)
    sample_rate: 1.0             # 100% sampling for slow queries
    capture_plan: true           # capture query plan for slow queries
    log_path: /var/log/themisdb/slow_query.log
    max_log_size_mb: 500
```

---

### Issue 4: Continuous Profiler CPU Overhead

**Description:** The continuous profiler consumes excessive CPU resources.

**Symptoms:**
- `top` shows `themisdb` at 95% CPU even with minimal query load
- Log: `ContinuousProfiler: sampling at 1000 Hz`

**Cause:** Sampling rate is too high.

**Solution:**
```yaml
observability:
  continuous_profiler:
    enabled: true
    sample_rate_hz: 100           # reduce from 1000
    profile_types: [cpu, heap]    # remove "goroutine" if not needed
    upload_interval_ms: 60000
    max_profile_size_mb: 10
```

---

### Issue 5: False Performance Regression Alerts

**Description:** Performance regression detector fires after a planned deployment.

**Symptoms:**
- Alert: `PerformanceRegressionDetected{service="themisdb"}`
- Latency increased as expected due to new feature, not a bug

**Cause:** Baseline was set before the deployment.

**Solution:**
```bash
# Reset performance baseline after planned change
themisdb-admin observability reset-baseline \
  --metric query_duration_p99 \
  --reason "v1.5.0 deployment – new geospatial feature adds ~20ms"

# Or silence alert for planned maintenance
themisdb-admin observability alert silence \
  --alert PerformanceRegressionDetected \
  --duration 2h \
  --reason "Planned deployment"
```
```yaml
observability:
  performance_analyzer:
    regression_threshold_pct: 20    # only alert for > 20% regression
    baseline_window_hours: 24       # use 24h rolling baseline
    min_samples_for_baseline: 1000
```

---

### Issue 6: Alertmanager Receives No Alerts

**Description:** Alerts defined in configuration never reach Alertmanager.

**Symptoms:**
- Alertmanager shows no incoming alerts
- ThemisDB alert conditions are clearly met

**Cause:** Alertmanager URL is wrong or the alert routing is misconfigured.

**Solution:**
```yaml
observability:
  alerting:
    enabled: true
    alertmanager_url: http://alertmanager:9093
    alert_timeout_ms: 5000
    resolve_timeout: 5m
```
```bash
# Test Alertmanager connectivity
curl http://alertmanager:9093/-/healthy

# Send a test alert
curl -X POST http://alertmanager:9093/api/v2/alerts \
     -H "Content-Type: application/json" \
     -d '[{"labels":{"alertname":"TestAlert"}}]'
```

---

### Issue 7: Storage Profiler Not Collecting Data

**Description:** Storage-related metrics show no data despite active writes.

**Symptoms:**
- `themisdb_storage_compaction_*` metrics are all zero
- Log: `StorageProfiler: RocksDB statistics not available`

**Cause:** RocksDB statistics collection is disabled.

**Solution:**
```yaml
storage:
  rocksdb:
    enable_statistics: true        # required for storage profiler

observability:
  storage_profiler:
    enabled: true
    collection_interval_ms: 10000
    include_per_cf_stats: true
```

---

### Issue 8: PII Leaks Into Metric Labels

**Description:** User email addresses or query text appears in Prometheus labels.

**Symptoms:**
- Prometheus label `{user="alice@example.com"}` visible in metrics
- Compliance issue detected in audit

**Cause:** Metrics are emitted with user-identifying information as label values.

**Solution:**
```yaml
observability:
  metrics:
    pii_scrubbing:
      enabled: true
      scrub_labels:
        - user_id
        - user_email
        - query_text
        - session_id
      hash_scrubbed_values: true   # replace with hash for correlation
```

## Diagnostic Commands

```bash
# Check metrics endpoint
curl -s http://localhost:9100/metrics | grep -E "^themisdb" | head -30

# List active alerts
themisdb-admin observability alerts list

# Show performance baseline
themisdb-admin observability baseline show

# Slow query log tail
tail -f /var/log/themisdb/slow_query.log

# Live metrics summary
themisdb-admin observability metrics summary

# Profiler status
themisdb-admin observability profiler status

# Tail observability logs
journalctl -u themisdb -f | grep -E "metrics|profil|alert|observ"
```

## Configuration Reference

```yaml
observability:
  metrics:
    enabled: true
    port: 9100
    path: /metrics
    label_limit: 50
    pii_scrubbing:
      enabled: true
  query_profiler:
    enabled: true
    threshold_ms: 500
    capture_plan: true
  continuous_profiler:
    enabled: false
    sample_rate_hz: 100
  alerting:
    enabled: true
    alertmanager_url: http://localhost:9093
  performance_analyzer:
    regression_threshold_pct: 20
    baseline_window_hours: 24
  storage_profiler:
    enabled: true
    collection_interval_ms: 15000
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `continuous_profiler.sample_rate_hz` | `1000` | `100` |
| `query_profiler.threshold_ms` | `5000` | `100–500` |
| `metrics.pii_scrubbing.enabled` | `false` | `true` |
| `performance_analyzer.regression_threshold_pct` | `5` | `15–25` |

## Known Limitations

- Continuous profiler does not support flame graph export in Community Edition.
- Query profiler captures at most 1000 plans per minute; high-throughput deployments may miss some slow queries.
- Storage profiler adds ~2% write overhead when `include_per_cf_stats: true`.
- Cardinality enforcement drops metrics silently; monitor `themisdb_metrics_dropped_total`.

## Related Documentation

- [Observability Module ROADMAP](../../src/observability/ROADMAP.md)
- [Observability Roadmap](../de/roadmap/observability_roadmap.md)
- [Grafana Metrics Complete](../ARCHIVED/implementation-summaries/GRAFANA_METRICS_COMPLETE.md)
- [Prometheus Integration Complete](../PROMETHEUS_INTEGRATION_COMPLETE.md)
- [Performance Alerting Config](../performance/PERFORMANCE_ALERTING_CONFIG.md)
- [Performance Regression Detection](../performance/PERFORMANCE_REGRESSION_DETECTION.md)
