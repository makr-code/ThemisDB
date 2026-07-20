# Field Diagnostics Runbook

**Audience**: Customer operations teams, support engineers, SRE teams  
**Version**: 1.0  
**Last Updated**: 2026-07-19  

## Overview

Field Diagnostics is a structured observability framework built into ThemisDB to provide real-time feedback from production deployments. Unlike traditional logs or metrics, field diagnostics capture **structured semantic events** from core components—NLI inference, mTLS connection pools, query execution, and more—enabling rapid diagnosis of performance and reliability issues.

### Why Field Diagnostics?

1. **Rapid Root Cause Analysis**: Structured events with context data enable faster pinpointing of issues
2. **Deployment Feedback Loop**: Customer ops teams can report diagnostics with support tickets
3. **Privacy-First**: Automatic PII masking ensures customer data is never exposed
4. **Production-Safe**: <1% CPU overhead; does not impact query latency

---

## Diagnostic Event Categories

Field diagnostics are classified into the following categories:

| Category | Description | Module | Example |
|----------|-------------|--------|---------|
| **NLI_INFERENCE** | Natural Language Inference prediction failures, model load errors, low confidence scores | `rag` | Model inference latency >5s, ONNX unavailable fallback to heuristic |
| **MTLS_CONNECTION** | Secure connection pool issues: SSL errors, connection acquisition timeouts, endpoint health transitions | `sharding` | Connection pool exhausted, SSL certificate validation failed, endpoint down |
| **QUERY_TIMEOUT** | Query execution exceeds SLA, slow query detection, coordinator timeout | `query_engine` | Query exceeded 30s SLA, coordinator wait timeout |
| **SHARD_ROUTING** | Shard selection failures, rebalancing issues, inconsistent routing | `sharding` | Shard unavailable during routing, rebalance queue full |
| **CACHE_DEGRADATION** | Cache miss rate spike, memory pressure, eviction policy triggered | `cache` | Cache memory 95% full, miss rate doubled in last 5min |
| **STORAGE_ERROR** | Persistent storage I/O errors, fsync failures, disk full | `storage` | Disk 98% full, I/O latency spiked 10x |
| **RPC_ERROR** | Inter-service communication failures, timeouts, protocol errors | `services` | RPC timeout to tensor service, malformed response |
| **RESOURCE_PRESSURE** | Memory, CPU, file descriptor exhaustion | `system` | Goroutine limit reached, TCP backlog full |
| **CONFIG_ERROR** | Configuration validation failures, deployment issues | `config` | Invalid TLS cert path, misconfigured replica count |

---

## Enabling Field Diagnostics

### Configuration

Field diagnostics are **enabled by default** in production. Customize behavior via `config/themis_diagnostics.yaml`:

```yaml
field_diagnostics:
  enabled: true                    # Enable/disable collection
  collection_mode: "active"        # "active" (always), "passive" (errors only), "sampled" (5%)
  max_buffer_size: 1000            # Max events to keep in memory
  enable_pii_masking: true         # Always mask PII before storage
  enable_metrics_emission: true    # Emit Prometheus metrics for each event
  batch_size: 100                  # Batch events for export
  batch_flush_interval_ms: 5000    # Flush batched events every 5s

  # Per-module configuration
  modules:
    rag:
      enabled: true
      min_severity: "INFO"         # Only collect INFO+ severity
    sharding:
      enabled: true
      min_severity: "WARN"         # Only collect WARN+ severity
    query_engine:
      enabled: true
      min_severity: "WARN"

  # Sampling for high-volume events (optional)
  sampling:
    NLI_INFERENCE: 1.0             # 100% sampling (no sampling)
    MTLS_CONNECTION: 0.5           # 50% sampling
    QUERY_TIMEOUT: 0.1             # 10% sampling
```

### Log Level Configuration

Field diagnostics honor the global log level setting:

```bash
# Set log level via environment variable
export THEMIS_LOG_LEVEL=info    # Minimum severity to collect

# Or via command-line flag
themis --log-level=warn
```

---

## Interpreting Diagnostics

### Common Patterns and Investigation Steps

#### Pattern: NLI Inference Latency Spike

**Observed metrics**:
- `rag_nli_inference_latency_ms` histogram > 5000ms (P99)
- `rag_nli_model_load_failures_total` counter increasing

**Investigation**:
1. Check ONNX model availability on disk
2. Verify memory pressure (may be swapping)
3. Check network I/O if using remote model storage
4. Collect diagnostic events with `GET /diag/events?category=NLI_INFERENCE&since=1h`

**Remediation**:
```bash
# Verify model is available and loadable
themis-diag validate-onnx-model /path/to/model.onnx

# Pre-load model into memory
themis-diag preload-onnx-model /path/to/model.onnx

# Check fallback behavior is working
themis-diag check-nli-fallback
```

#### Pattern: mTLS Connection Pool Exhaustion

**Observed metrics**:
- `mtls_connection_pool_acquisition_latency_ms` histogram increasing
- `mtls_connection_pool_ssl_errors_total` counter spike
- Diagnostic events with `MTLS_CONNECTION` category

**Investigation**:
1. Check target endpoint health (SSL, TLS version, certificate validity)
2. Verify connection reuse rate is >70%
3. Look for endpoint health transitions (available → degraded)
4. Inspect certificate expiration dates

**Remediation**:
```bash
# Check connection pool status
themis-diag pool-status --endpoint=<shard-address>

# Verify TLS certificates
themis-diag verify-tls --cert=/path/to/cert.pem

# Rotate certificates if expired
themis-diag rotate-tls --cert=/path/to/new-cert.pem

# Restart connection pool (drains gracefully)
themis-diag restart-pool --endpoint=<shard-address>
```

#### Pattern: Query Timeout Events

**Observed metrics**:
- `field_diagnostic_events_total{category="QUERY_TIMEOUT"}` counter increasing
- Query P99 latency > SLA
- Context data shows `shard_latency_ms` is high

**Investigation**:
1. Identify slow shard (via context data in diagnostic event)
2. Check if shard is overloaded (CPU, memory, I/O)
3. Look for concurrent long-running queries
4. Inspect query plan complexity

**Remediation**:
```bash
# Get detailed diagnostic events for the period
themis-diag export-events --category=QUERY_TIMEOUT \
    --since=2h --until=1h > /tmp/query_timeouts.json

# Analyze slow queries
themis-diag analyze-slow-queries /tmp/query_timeouts.json

# Consider query plan optimization or shard rebalancing
themis-diag rebalance-shards --dry-run
```

---

## Reporting Issues with Diagnostic Bundles

When reporting issues to ThemisDB support, include a diagnostic bundle:

### Collecting a Diagnostic Bundle

```bash
# Export all events from last hour (automatically redacted for PII)
themis-diag export-bundle --since=1h --output=/tmp/themis-diag-bundle.tar.gz

# The bundle includes:
# - Diagnostic events (JSON)
# - System metrics (CPU, memory, I/O)
# - Configuration summary (redacted)
# - Prometheus metrics snapshot
# - Error logs (last hour, PII-masked)
```

### Attaching to Support Ticket

1. Collect bundle: `themis-diag export-bundle --since=1h --output=/tmp/bundle.tar.gz`
2. Verify no PII leaked: `tar -tzf /tmp/bundle.tar.gz | head -20`
3. Attach to support ticket with description of issue and timeline
4. Include relevant diagnostic event category (NLI_INFERENCE, MTLS_CONNECTION, etc.)

### Support Team Analysis

Support engineers can:
```bash
# Unpack and analyze bundle
tar -xzf themis-diag-bundle.tar.gz -C /tmp/analysis
cd /tmp/analysis

# Run analysis scripts
./analyze_nli_performance.sh
./analyze_connection_pool.sh
./check_pii_redaction.sh
```

---

## Privacy: PII Masking Strategy

### Automatic PII Masking

Field diagnostics automatically mask the following field types:

| Field Pattern | Example Original | Example Masked |
|---------------|------------------|----------------|
| `user_*` | `user_id=12345` | `user_id=12***` |
| `query` | `SELECT * FROM users` | `SELECT ** F**M **ERS` |
| `email` | `alice@company.com` | `al***@company.com` |
| `api_key` | `sk-12345abcde` | `sk-****bcd*` |
| `token` | `eyJhbGc...xyz` | `eyJ***...xy*` |
| `password` | `myPassword123` | `my*******123` |
| `response_body` | `{"data": {...}}` | `{"da**": {...}}` |

### Verification

ThemisDB never emits diagnostics with unmasked PII. To verify:

```bash
# Check no PII in diagnostic buffer (in-memory check)
themis-diag check-pii-masking

# Export events and validate
themis-diag export-events --since=1h | \
    themis-diag validate-pii --strict

# Output: PASS or lists any detected PII patterns
```

### Configuration

Enable strict PII checking (recommended for sensitive deployments):

```yaml
field_diagnostics:
  enable_pii_masking: true
  pii_masking_mode: "strict"     # "lenient", "strict", "paranoid"
  mask_char: '*'
  log_pii_violations: true       # Alert if PII detection fails
```

---

## Alerting Rules

Use the following Prometheus/Grafana rules to detect diagnostic anomalies:

### NLI Inference Degradation

```promql
# Alert if NLI latency P99 > 5s
alert_nli_slow:
  expr: |
    histogram_quantile(0.99, rate(rag_nli_inference_latency_ms_bucket[5m])) > 5000
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "NLI inference latency high ({{ $value }}ms)"
```

### mTLS Connection Issues

```promql
# Alert if connection acquisition latency > 1s
alert_mtls_slow:
  expr: |
    histogram_quantile(0.95, rate(mtls_connection_pool_acquisition_latency_ms_bucket[5m])) > 1000
  for: 5m
  labels:
    severity: warning

# Alert on SSL errors
alert_mtls_ssl_errors:
  expr: |
    increase(mtls_connection_pool_ssl_errors_total[5m]) > 10
  for: 2m
  labels:
    severity: critical
```

### Query Timeouts

```promql
# Alert if query timeout rate > 1%
alert_query_timeout_rate:
  expr: |
    rate(field_diagnostic_events_total{category="QUERY_TIMEOUT"}[5m]) / 
    rate(field_diagnostic_events_total[5m]) > 0.01
  for: 5m
  labels:
    severity: warning
```

### Resource Pressure

```promql
# Alert on resource pressure events
alert_resource_pressure:
  expr: |
    increase(field_diagnostic_events_total{category="RESOURCE_PRESSURE"}[5m]) > 5
  for: 2m
  labels:
    severity: critical
```

---

## Grafana Dashboards

ThemisDB includes pre-built Grafana dashboards for field diagnostics. Import them via:

```bash
# Grafana API import
curl -X POST http://grafana:3000/api/dashboards/db \
  -H "Authorization: ******" \
  -H "Content-Type: application/json" \
  -d @dashboards/field_diagnostics_overview.json

# Or via Grafana UI:
# Settings → Dashboards → Import → Upload JSON file
```

**Available dashboards**:
1. **Field Diagnostics Overview**: Event rates, categories, severity distribution
2. **NLI Performance**: Inference latency, model load failures, confidence scores
3. **mTLS Connection Pool**: Acquisition latency, SSL errors, endpoint health
4. **Query Performance**: Query timeouts, slow query analysis, P99 latency
5. **Resource Utilization**: Memory, CPU, I/O pressure, correlation with diagnostics

---

## CLI Commands Reference

```bash
# Export diagnostic events
themis-diag export-events [--category=<cat>] [--since=<dur>] [--until=<dur>]

# Check diagnostics are being collected
themis-diag check-collection-status

# Validate PII masking
themis-diag validate-pii [--strict]

# Create diagnostic bundle for support
themis-diag export-bundle [--since=<dur>] --output=<file>

# Analyze specific issue type
themis-diag analyze-slow-queries <events.json>
themis-diag analyze-connection-pool <events.json>
themis-diag analyze-resource-pressure <events.json>

# Check component health based on diagnostics
themis-diag health-check [--verbose]

# Real-time diagnostics stream (like 'tail -f' for events)
themis-diag stream-events [--category=<cat>] [--min-severity=<sev>]
```

---

## Troubleshooting

### Diagnostics Not Being Collected

1. **Check if enabled**: `themis-diag check-collection-status`
2. **Check log level**: Ensure `THEMIS_LOG_LEVEL >= INFO` (or configured min severity)
3. **Verify config**: Check `config/themis_diagnostics.yaml` exists and is valid
4. **Check buffer size**: `curl http://localhost:9090/diag/stats | jq .current_buffer_size`

### PII Appearing in Diagnostics

1. **Verify masking enabled**: Check `enable_pii_masking: true` in config
2. **Run validation**: `themis-diag validate-pii --strict`
3. **Check custom fields**: If adding custom context data, pre-sanitize before emit
4. **Contact support**: If PII validation fails, this is a security issue

### High Overhead from Diagnostics Collection

1. **Reduce sampling**: Lower sampling rates for high-volume categories
2. **Increase buffer size**: Larger buffer reduces contention
3. **Disable per-module**: Selectively disable modules with high event rates
4. **Check callback overhead**: If custom callbacks registered, profile them

---

## Performance Expectations

| Metric | Target | Typical | Notes |
|--------|--------|---------|-------|
| Event emission latency | <100µs | 10-50µs | Lock-free buffer with std::shared_mutex |
| CPU overhead | <1% | 0.1-0.5% | Minimal even under high event rate |
| Memory per event | ~500B | 400-600B | Variable with context data size |
| Buffer size | 1000 events | <1MB | Per event ~1KB average |
| Callback overhead | <10µs per callback | 5µs | Async recommended for heavy processing |

---

## Support & Feedback

For issues, questions, or feature requests:

1. **Generate diagnostic bundle**: `themis-diag export-bundle --since=1h`
2. **Attach to support ticket** with description and timeline
3. **Include relevant logs**: Error logs, config summary (redacted)
4. **Mention environment**: Deployment size, query patterns, hardware

Support team SLA: 24h response time for critical alerts (CRITICAL severity), 48h for warnings.

---

**Document Version**: 1.0  
**Last Updated**: 2026-07-19  
**Status**: Production Ready
