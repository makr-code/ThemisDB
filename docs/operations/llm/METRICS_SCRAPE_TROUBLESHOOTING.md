# Runbook: Metrics Scrape Troubleshooting

**Component:** ThemisDB LLM module — Observability / Prometheus
**Severity:** Operational
**Last Updated:** April 2026

---

## Overview

This runbook covers diagnosing problems where Prometheus cannot scrape LLM metrics from ThemisDB, or where metrics are missing, stale, or incorrect.

> **Current state (v1.5):** The `MetricsServer::start()` function contains a `// TODO: Start actual HTTP server` placeholder. The `/metrics` HTTP endpoint is **not yet live**. Until Q1 implementation is complete, Prometheus cannot scrape LLM metrics. This document describes the troubleshooting steps for the production state (post-Q1).

---

## Scrape Architecture

```text
Prometheus
    │  scrape every 15 s
    ▼
ThemisDB MetricsServer
    http://localhost:9091/metrics
    │
    └── PrometheusExporter::exportMetrics()
         └── LLMMetricsCollector (in-memory counters/gauges/histograms)
```

Prometheus configuration (`prometheus.yml`):

```yaml
scrape_configs:
  - job_name: 'themisdb_llm'
    static_configs:
      - targets: ['<themisdb_host>:9091']
    scrape_interval: 15s
    metrics_path: /metrics
```

---

## Diagnostic Flowchart

```text
Metrics missing in Grafana?
    │
    ├─► Is the Prometheus target UP?
    │       http://<prometheus>:9090/targets → look for themisdb_llm
    │
    │   YES → Metrics should be scraped. Check Grafana data source config.
    │   NO  → Is the /metrics endpoint reachable?
    │               curl http://localhost:9091/metrics
    │
    │           CONN REFUSED → MetricsServer not started (see Issue 1)
    │           200 OK, body empty → No metrics emitted (see Issue 2)
    │           200 OK, metrics present → Prometheus scrape config wrong
    │                                         (see Issue 3)
    │
    └─► Are specific metrics missing?
            e.g., llm_inference_requests_total not in output
            → Hot path not yet wired (see Issue 2)
```

---

## Issue 1 — `/metrics` endpoint returns "Connection Refused"

**Cause:** `MetricsServer::start()` is not implemented (v1.5 known gap) or the server failed to start.

**Checks:**

```bash
# Confirm the process is listening on port 9091
ss -tlnp | grep 9091
# or
netstat -tlnp | grep 9091

# Check ThemisDB startup logs for metrics server messages
journalctl -u themisdb -n 100 | grep -i "metrics server"
# Expected once implemented:
#   INFO  Metrics Server started: http://0.0.0.0:9091/metrics
```

**Fix:**

Until Q1 (MetricsServer HTTP implementation), LLM metrics are not scrapeable. Use internal log-based monitoring as a stopgap:

```bash
grep "llm_inference_requests" /var/log/themisdb.log | tail -20
```

Once the Q1 fix is deployed and the server starts successfully:

```bash
# Verify endpoint is live
curl -s http://localhost:9091/metrics | head -20
```

---

## Issue 2 — `/metrics` returns 200 but metric is missing

**Possible causes:**

| Cause | Symptom | Fix |
|-------|---------|-----|
| Metric not yet wired into the hot path | Counter stays at 0 or is absent | Check if the metric emission point is implemented (see `grafana_metrics.cpp` and the roadmap checklist) |
| `LLMMetricsCollector` not initialised | No `llm_*` metrics at all | Verify that `LLMMetricsCollector` is constructed and passed to the inference pipeline |
| Metric defined but never incremented | Value is 0 indefinitely | Trace `LLMMetricsCollector::recordInferenceRequest()` call site |
| Histogram bucket mismatch | `_bucket` metrics present, counts look wrong | Verify bucket boundaries in `PrometheusExporter::serializeMetric()` match the workload's latency distribution |

**Diagnostic commands:**

```bash
# List all currently emitted LLM metrics
curl -s http://localhost:9091/metrics | grep ^llm_ | awk -F'{' '{print $1}' | sort -u

# Check if a specific counter is incrementing over time
watch -n 5 'curl -s http://localhost:9091/metrics | grep llm_inference_requests_total'
```

---

## Issue 3 — Prometheus target is DOWN

**Check the Prometheus target status:**

1. Open `http://<prometheus-host>:9090/targets`.
2. Find `themisdb_llm` in the list.
3. Note the error message in the "Error" column.

**Common errors:**

| Error | Cause | Fix |
|-------|-------|-----|
| `dial tcp ... connect: connection refused` | MetricsServer not running | See Issue 1 |
| `context deadline exceeded` | Scrape timeout too short | Increase `scrape_timeout` in `prometheus.yml` (e.g., `10s`) |
| `server returned HTTP status 404` | Wrong `metrics_path` | Verify `metrics_path: /metrics` in Prometheus scrape config |
| `TLS handshake error` | TLS mismatch | If TLS is enabled, configure `scheme: https` and provide `tls_config` |

---

## Issue 4 — Grafana shows "No data"

1. **Confirm the data source is configured correctly:**
   - In Grafana: Configuration → Data Sources → ThemisDB Prometheus → Test.

2. **Check the time range:**
   - Ensure the selected time range covers a period when ThemisDB was running and metrics were being emitted.

3. **Verify the PromQL query:**
   - Open the Grafana Explore tab and run the PromQL query directly:

   ```promql
   llm_inference_requests_total
   ```

   If no series are returned, the metric is not being scraped. Return to Issue 1 or Issue 2.

4. **Check Prometheus for the metric directly:**

   ```bash
   curl -G 'http://localhost:9090/api/v1/query' \
     --data-urlencode 'query=llm_inference_requests_total' \
     | jq '.data.result'
   ```

---

## Issue 5 — Metrics values look incorrect

**Histogram bucket counts always 0:**

- The `serializeMetric()` function in `PrometheusExporter` uses hardcoded buckets `{10, 25, 50, 100, 250, 500, 1000, 2500, 5000}`.
- If all observations fall below the smallest bucket (10 ms) or above the largest (5 000 ms), the bucket counts will be distorted.
- **Fix:** Adjust the bucket boundaries to match your workload's observed latency range.

**Counter resets unexpectedly:**

- Prometheus handles counter resets (process restarts) automatically. If you see a gap in a `rate()` graph, it is likely due to a ThemisDB restart. This is expected behaviour.

**Gauge is stuck at an old value:**

- The `PrometheusExporter` in-memory store persists gauge values until they are explicitly updated. If a model is unloaded but `recordModelUnloaded()` is not called, the gauge will remain at the stale value.
- **Fix:** Ensure `LLMMetricsCollector::recordModelUnloaded()` is called on model eviction.

---

## Port Reference

| Service | Default Port | Endpoint |
|---------|-------------|----------|
| ThemisDB LLM metrics | 9091 | `/metrics` |
| ThemisDB health | 9090 | `/health` (planned) |
| Prometheus | 9092 (host) → 9090 (container) | `/` |
| Grafana | 3000 | `/` |

> **Note:** Port 9090 is shared between the ThemisDB health endpoint and the standard Prometheus internal port. Use the port mapping `9092:9090` for Prometheus Docker containers to avoid the conflict (see `docs/observability/README.md`).

---

## Related Documents

- `docs/llm_roadmap.md` — Section 1.1 (observability gaps) and Q1 items
- `docs/observability/llm_metrics_schema.md` — Canonical metric names and labels
- `prometheus/rules/llm_alerts.yml` — Alerting rules
- `docs/observability/README.md` — General observability and Prometheus setup
- `src/llm/grafana_metrics.cpp` — Prometheus exporter implementation
- `docs/PROMETHEUS_INTEGRATION_COMPLETE.md` — Prometheus integration history
