# Analytics Troubleshooting Guide

The `analytics` module provides OLAP capabilities, Complex Event Processing (CEP), anomaly detection, process mining, time-series forecasting, incremental materialized views, and streaming window aggregations for ThemisDB.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `CepEngine: pattern not matched` | Wrong event ordering or gap in stream | Check CEP pattern syntax; enable `cep.debug_mode` |
| Anomaly detection produces too many false positives | Sensitivity too high | Increase `analytics.anomaly.threshold_sigma` |
| Incremental view lags behind | Refresh too infrequent | Reduce `analytics.incremental_view.refresh_interval_ms` |
| `Forecasting: insufficient data points` | Too few historical samples | Ensure at least 2× the forecast horizon in history |
| Streaming window misses data at chunk boundary | Window alignment issue | Use `HOP` windows instead of `TUMBLING` |
| OLAP query slow despite materialized view | View not up-to-date | Force view refresh; check refresh schedule |
| `AnomalyDetection: model not trained` | No training data available | Trigger manual training with historical data |
| `ArrowExport: schema mismatch` | Arrow schema changed after data insert | Rebuild Arrow export schema |
| Process mining shows incomplete traces | Events missing process ID | Ensure all events include `process_id` field |
| `AutoML: feature selection timeout` | Too many feature candidates | Limit `analytics.automl.max_features` |

## Common Issues

### Issue 1: CEP Pattern Never Fires

**Description:** A complex event processing pattern is defined but never matches any events.

**Symptoms:**
- Log: `CepEngine: pattern 'fraud_alert' defined but 0 matches in 24h`
- Expected alerts not generated

**Cause:** Event field names in pattern do not match actual event schema; pattern window too short.

**Solution:**
```yaml
analytics:
  cep:
    enabled: true
    debug_mode: true               # log all pattern evaluations
    patterns:
      fraud_alert:
        events:
          - type: login
            filter: "source_ip NOT IN ['10.0.0.0/8']"
          - type: high_value_transfer
            filter: "amount > 10000"
        window_ms: 300000           # 5 minutes
        max_gap_ms: 60000
```
```bash
# Test CEP pattern with sample events
themisdb-admin analytics cep test-pattern \
  --pattern fraud_alert \
  --events /tmp/sample_events.jsonl
```

---

### Issue 2: Anomaly Detector Too Sensitive

**Description:** The anomaly detection system triggers alerts on normal data variation.

**Symptoms:**
- Alert: `AnomalyDetected{metric="api_latency_ms"}` fires during normal business hours
- Log: `AnomalyDetection: score=3.1 sigma (threshold=3.0)`

**Cause:** Threshold too low; business hour traffic is mistaken for anomaly.

**Solution:**
```yaml
analytics:
  anomaly:
    enabled: true
    algorithm: isolation_forest    # "isolation_forest" | "z_score" | "iqr"
    threshold_sigma: 4.5           # increase from 3.0
    seasonal_decomposition: true   # account for daily/weekly patterns
    min_training_samples: 10000
    retrain_interval_hours: 24
```
```bash
# Retrain anomaly model on current data
themisdb-admin analytics anomaly retrain \
  --collection metrics \
  --metric api_latency_ms \
  --lookback-days 30
```

---

### Issue 3: Incremental View Lags Behind Source Data

**Description:** An incremental materialized view shows data that is minutes or hours old.

**Symptoms:**
- View `orders_by_region_daily` is 3 hours behind
- Log: `IncrementalView: refresh lag=10800s for view=orders_by_region_daily`

**Cause:** Refresh interval is too long; refresh jobs are queued behind other analytics work.

**Solution:**
```yaml
analytics:
  incremental_view:
    refresh_interval_ms: 60000     # 1 minute instead of 1 hour
    max_refresh_workers: 4
    priority: high
    lag_alert_threshold_ms: 300000
```
```bash
# Manually trigger refresh
themisdb-admin analytics view refresh --view orders_by_region_daily

# Check view lag
themisdb-admin analytics view lag
```

---

### Issue 4: Forecasting Returns Flat Predictions

**Description:** The time-series forecasting model predicts a constant value.

**Symptoms:**
- All forecasted values equal the mean of the training data
- Log: `Forecasting: insufficient trend detected; using naive baseline`

**Cause:** Too few data points; no seasonal pattern detected.

**Solution:**
```yaml
analytics:
  forecasting:
    algorithm: prophet              # "prophet" | "arima" | "lstm" | "naive"
    min_data_points: 168            # at least 1 week of hourly data
    seasonal_periods: [24, 168]     # daily and weekly seasonality
    changepoint_prior_scale: 0.05
    forecast_horizon_hours: 24
```
```bash
# Check data availability for forecasting
themisdb-admin analytics forecast data-check \
  --collection metrics \
  --field value \
  --start "30 days ago"
```

---

### Issue 5: Streaming Window Misses Events at Boundary

**Description:** Sliding window aggregations drop events that fall exactly on the window boundary.

**Symptoms:**
- Event with timestamp exactly at window boundary is counted in neither window
- Aggregation totals do not add up

**Cause:** Window boundary comparison uses exclusive-end by default.

**Solution:**
```sql
-- Use INCLUSIVE end for windows
FOR event IN events
  COLLECT
    window = FLOOR(event.timestamp / 3600000) * 3600000
    WITH COUNT INTO cnt
  RETURN { window: DATE_ISO8601(window), count: cnt }
```
```yaml
analytics:
  streaming:
    window_type: tumbling           # "tumbling" | "sliding" | "session"
    window_size_ms: 3600000
    boundary: inclusive_end         # "exclusive_end" | "inclusive_end"
    late_arrival_tolerance_ms: 5000
```

---

### Issue 6: Anomaly Model Not Trained

**Description:** Anomaly detection is configured but produces no output because no model has been trained.

**Symptoms:**
- Log: `AnomalyDetection: model not trained for metric=cpu_usage; skipping`
- No anomaly alerts ever fire

**Cause:** Auto-training requires minimum data points that have not yet accumulated.

**Solution:**
```bash
# Trigger manual model training
themisdb-admin analytics anomaly train \
  --collection system_metrics \
  --metric cpu_usage \
  --lookback-hours 720

# Check model status
themisdb-admin analytics anomaly model-status
```

---

### Issue 7: OLAP Query Ignores Materialized View

**Description:** An analytical query runs slowly even though a materialized view exists.

**Symptoms:**
- Query takes 60 seconds; view refresh was 1 minute ago
- EXPLAIN shows `CollectionScan` instead of `ViewScan`

**Cause:** View staleness exceeds query's `max_view_age` option; optimizer falls back to source scan.

**Solution:**
```sql
-- Allow slightly stale view reads
FOR row IN orders_by_region_daily
  OPTIONS { maxViewAge: 300 }   -- accept data up to 5 min old
  FILTER row.date >= DATE_SUBTRACT(NOW(), 7, "day")
  RETURN row
```

---

### Issue 8: Process Mining Shows Incomplete Traces

**Description:** Process mining traces are fragmented; activities appear disconnected.

**Symptoms:**
- Process diagram shows isolated nodes
- Traces have <50% completion rate

**Cause:** Events lack a consistent `process_id` (case ID) that links activities.

**Solution:**
```yaml
analytics:
  process_mining:
    case_id_field: process_id       # field linking events to a process instance
    activity_field: activity_name
    timestamp_field: timestamp
    min_trace_events: 2
    max_trace_gap_hours: 24         # close traces with > 24h gap
```
```bash
# Validate process mining data quality
themisdb-admin analytics process-mining validate \
  --collection workflow_events \
  --sample 1000
```

## Diagnostic Commands

```bash
# Analytics module health
themisdb-admin analytics status

# CEP pattern list and match stats
themisdb-admin analytics cep patterns

# Anomaly detection model info
themisdb-admin analytics anomaly model-status

# Incremental view lag
themisdb-admin analytics view lag

# Forecasting model status
themisdb-admin analytics forecast model-status

# Live analytics metrics
curl -s http://localhost:9100/metrics | grep themisdb_analytics

# Tail analytics logs
journalctl -u themisdb -f | grep -E "analytics|cep|anomaly|forecast|incremental|olap"
```

## Configuration Reference

```yaml
analytics:
  cep:
    enabled: true
    max_patterns: 100
    window_ms: 300000
  anomaly:
    enabled: true
    algorithm: isolation_forest
    threshold_sigma: 4.0
    retrain_interval_hours: 24
  incremental_view:
    enabled: true
    refresh_interval_ms: 300000
    max_refresh_workers: 2
  forecasting:
    enabled: true
    algorithm: prophet
    forecast_horizon_hours: 24
  streaming:
    window_type: tumbling
    late_arrival_tolerance_ms: 5000
```

## Known Limitations

- CEP patterns do not support negation (e.g., "event A did NOT occur"); use periodic batch queries instead.
- LSTM forecasting model training requires GPU; falls back to ARIMA on CPU-only deployments.
- Incremental views cannot be defined on collections that change schema frequently.
- Process mining trace reconstruction has O(n²) complexity for very long traces (>10000 events).

## Related Documentation

- [Analytics Module ROADMAP](../../src/analytics/ROADMAP.md)
- [Automatic Full-Text Index](../AUTOMATIC_FULLTEXT_INDEX.md)
- [Dynamic Batch Size Adaptation](../DYNAMIC_BATCH_SIZE_ADAPTATION.md)
- [Performance Regression Detection](../performance/PERFORMANCE_REGRESSION_DETECTION.md)
