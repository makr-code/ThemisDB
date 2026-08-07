# Timeseries Module - Operator Guide

<!-- Status: validated 2026-08-07 -->
<!-- Links: README.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_BASELINE.md -->

## Purpose

This guide provides operational procedures, configuration tuning, troubleshooting steps, and incident response playbooks for ThemisDB timeseries module production deployments.

## Table of Contents

1. [Deployment Checklist](#deployment-checklist)
2. [Configuration Guide](#configuration-guide)
3. [Performance Tuning](#performance-tuning)
4. [Capacity Planning](#capacity-planning)
5. [Monitoring Setup](#monitoring-setup)
6. [Troubleshooting Guide](#troubleshooting-guide)
7. [Incident Response Runbooks](#incident-response-runbooks)
8. [SLA and Performance Targets](#sla-and-performance-targets)

---

## Deployment Checklist

### Pre-Deployment Validation

- [ ] Verify system meets minimum CPU/memory requirements
- [ ] Ensure RocksDB backend is configured and accessible
- [ ] Configure all timeseries module environment variables
- [ ] Enable production mode: `export THEMIS_PRODUCTION_MODE=1`
- [ ] Verify security/authorization controls are active
- [ ] Test connectivity to remote-write endpoint (if enabled)
- [ ] Enable audit logging to persistent storage
- [ ] Review and customize retention policy

### Runtime Configuration Validation

- [ ] Adaptive flush controller configured with appropriate buffer sizes
- [ ] Resource limits (CPU, memory) are explicitly set (no unlimited defaults)
- [ ] External dependencies have connection timeouts and retry policies
- [ ] Monitoring and alerting thresholds set per SLA expectations
- [ ] Backup strategy for RocksDB defined
- [ ] Log rotation and retention policy configured

### Post-Deployment Smoke Tests

```bash
# Health check: Can ingest and query
curl -X POST http://localhost:8080/api/timeseries/ingest \
  -H "Content-Type: application/json" \
  -d '{
    "metric": "test.metric",
    "timestamp": 1628100000000000000,
    "value": 42.0
  }'

# Health check: Can query range
curl -X GET 'http://localhost:8080/api/timeseries/query?series=test.metric&start=1628000000000000000&end=1628200000000000000' \
  -H "Authorization: ******"

# Health check: Can perform downsampling
curl -X POST http://localhost:8080/api/timeseries/downsample \
  -H "Content-Type: application/json" \
  -d '{
    "metric": "test.metric",
    "resolution_ns": 1000000000,
    "start": 1628000000000000000,
    "end": 1628200000000000000
  }'
```

---

## Configuration Guide

### Environment Variables

```bash
# Production mode (mandatory)
THEMIS_PRODUCTION_MODE=1
THEMIS_ENVIRONMENT=production

# Timeseries module logging
THEMIS_TS_LOG_LEVEL=INFO              # DEBUG, INFO, WARN, ERROR
THEMIS_TS_LOG_DESTINATION=/var/log/themis/timeseries.log
THEMIS_TS_LOG_MAX_SIZE=1073741824     # 1 GB

# Adaptive flush controller
THEMIS_TS_BUFFER_SIZE_MB=256
THEMIS_TS_FLUSH_THRESHOLD_PERCENT=75
THEMIS_TS_WATERMARK_LOW_PERCENT=25
THEMIS_TS_WATERMARK_HIGH_PERCENT=90

# Retention policy
THEMIS_TS_RETENTION_DAYS=30
THEMIS_TS_CLEANUP_INTERVAL_SECONDS=3600
THEMIS_TS_BATCH_DELETE_SIZE=10000

# Gorilla codec
THEMIS_TS_GORILLA_SIMD_ENABLED=true
THEMIS_TS_COMPRESSION_RATIO_TARGET=0.20  # Target 5:1 compression

# Remote write (if enabled)
THEMIS_TS_REMOTE_WRITE_ENDPOINT=http://prometheus:9009/api/v1/write
THEMIS_TS_REMOTE_WRITE_TIMEOUT_MS=5000
THEMIS_TS_REMOTE_WRITE_BATCH_SIZE=5000
THEMIS_TS_REMOTE_WRITE_RETRY_ATTEMPTS=3

# Encrypted chunk storage
THEMIS_TS_ENCRYPTION_ENABLED=true
THEMIS_TS_KEY_ROTATION_INTERVAL_DAYS=90
THEMIS_TS_KEY_STORAGE_PATH=/etc/themis/keys

# Series index
THEMIS_TS_INDEX_INITIAL_CAPACITY=10000
THEMIS_TS_INDEX_LOAD_FACTOR=0.75
```

### Configuration File Example (YAML)

```yaml
timeseries:
  # Buffer and flush settings
  buffer:
    size_mb: 256
    flush_threshold_percent: 75
    watermark_low_percent: 25
    watermark_high_percent: 90
    pre_allocate: true

  # Gorilla compression
  compression:
    algorithm: gorilla
    use_simd: true
    dictionary_size: 1024

  # Query settings
  query:
    max_points_per_range: 1000000
    timeout_ms: 30000
    cache_ttl_seconds: 300

  # Retention lifecycle
  retention:
    policy: "30d"
    cleanup_interval_seconds: 3600
    batch_delete_size: 10000
    enable_archive: false

  # Downsampling
  downsampling:
    max_series_per_batch: 100
    timeout_ms: 5000

  # Aggregation
  aggregation:
    enable_continuous_agg: true
    scheduler_interval_seconds: 60
    max_pending_aggregates: 1000

  # Remote write
  remote_write:
    enabled: false
    endpoint: "http://prometheus:9009/api/v1/write"
    timeout_ms: 5000
    batch_size: 5000
    retry_attempts: 3
    backoff_ms: 100

  # Encryption
  encryption:
    enabled: false
    algorithm: "aes-256-gcm"
    key_rotation_days: 90
    key_storage: "/etc/themis/keys"

  # Monitoring and diagnostics
  monitoring:
    enable_metrics: true
    enable_audit_logging: true
    metrics_export_interval_seconds: 60
```

---

## Performance Tuning

### Adaptive Flush Tuning

**Goal:** Balance write latency vs compression efficiency.

**Key Parameters:**
- `THEMIS_TS_BUFFER_SIZE_MB`: Larger buffers → higher compression, longer flush latency
- `THEMIS_TS_FLUSH_THRESHOLD_PERCENT`: When to trigger flush (default: 75%)
- `THEMIS_TS_WATERMARK_*_PERCENT`: Backpressure control

**Tuning Matrix:**

| Ingest Rate | Buffer Size | Flush Threshold | Flush Latency (p99) | Recommendation |
|-------------|-------------|-----------------|---------------------|-----------------|
| 100k pts/s | 128 MB | 75% | <10 ms | Conservative, low latency |
| 500k pts/s | 256 MB | 75% | ~30-50 ms | Balanced |
| 1M+ pts/s | 512 MB | 80% | ~100-150 ms | Higher throughput |

**Steps:**
1. Measure baseline flush latency: `THEMIS_TS_FLUSH_THRESHOLD_PERCENT=75`
2. If latency too high, reduce buffer size or lower threshold
3. If compression ratio too low, increase buffer size
4. Monitor actual ingest rate and adjust watermarks accordingly

### Query Performance Tuning

**Goal:** Minimize query latency (target: p99 ≤500 µs per GATE-TSRG-02).

**Key Optimizations:**
- Enable SIMD: `THEMIS_TS_GORILLA_SIMD_ENABLED=true`
- Increase query cache TTL: `query.cache_ttl_seconds=300`
- Limit query range: `query.max_points_per_range=1000000`

**Tuning Steps:**
1. Profile query latency: `SELECT p99_latency FROM timeseries_metrics WHERE operation='range_query'`
2. If p99 > 500 µs, check SIMD status
3. Reduce query window size or enable caching
4. Monitor L3 cache misses and CPU utilization

### Compression Ratio Tuning

**Goal:** Achieve 5-8× compression ratio typical for stable timeseries.

**Key Parameters:**
- Larger buffer → higher compression (more similar values = better delta encoding)
- Longer flush interval → higher compression (same reason)
- Gorilla algorithm optimal for monotonic/near-constant timeseries

**Tuning Steps:**
1. Measure current ratio: `compression_ratio = compressed_size / original_size`
2. If ratio > 0.30 (3:1 compression), increase buffer size
3. Verify Gorilla codec is active (not fallback to raw)
4. Check for pathological data patterns (random walk, high variance)

### Retention Cleanup Tuning

**Goal:** Minimize cleanup latency while maintaining retention SLA.

**Key Parameters:**
- `THEMIS_TS_RETENTION_DAYS`: How much history to keep
- `THEMIS_TS_CLEANUP_INTERVAL_SECONDS`: How often to run cleanup
- `THEMIS_TS_BATCH_DELETE_SIZE`: Points per cleanup pass

**Tuning Steps:**
1. Monitor cleanup duration: `SELECT duration_ms FROM timeseries_metrics WHERE operation='retention_cleanup'`
2. If cleanup takes > 1 minute, split into smaller batches
3. Tune `cleanup_interval_seconds` to run during off-peak hours
4. Consider archiving rather than deletion for long-term storage

---

## Capacity Planning

### Storage Estimation

**Formula:** `storage_gb = (daily_points × retention_days × compression_ratio) / 1e9`

**Example:**
- Daily ingest: 1 billion points/day
- Retention: 30 days
- Compression ratio: 0.20 (5:1 compression)
- Storage needed: (1e9 × 30 × 0.20) / 1e9 = **6 GB**

### Memory Estimation

**Formula:** `memory_mb = buffer_size_mb + (active_series_count × 1_kb) + overhead_mb`

**Example:**
- Buffer size: 256 MB
- Active series: 10,000 (10 MB)
- Index overhead: 50 MB
- Total: ~316 MB

### CPU Estimation

**Base Load:**
- Ingest: ~0.1-0.2 ms CPU per point (1M pts/s = 100-200 CPU ms/s)
- Query: ~0.05 ms per point in range
- Flush: ~10-20 ms per 1M points
- Cleanup: ~5-10 ms per 1k points deleted

**Example for 1M points/sec sustained:**
- CPU for ingest: ~100 ms/s = 10% of 1 CPU core
- CPU for flush (5 sec interval): ~50 ms every 5s = 1% sustained
- CPU for queries: ~5-10% depending on query rate
- Total: Reserve 1-2 full CPU cores for safe operation

### Network Estimation (Remote-Write)

**Formula:** `bandwidth_mbps = (daily_points × point_size_bytes × remote_write_ratio) / (86400 × 1e6)`

**Example:**
- 1B points/day, 20 bytes/point (encoded), 10% sampled for remote-write
- Bandwidth: (1e9 × 20 × 0.1) / 86400e6 ≈ 0.23 Mbps

---

## Monitoring Setup

### Key Metrics to Export

```
# Write path
timeseries_write_throughput_points_per_sec
timeseries_write_latency_p50_us
timeseries_write_latency_p95_us
timeseries_write_latency_p99_us
timeseries_buffer_fill_percent
timeseries_flush_count_total
timeseries_flush_latency_p99_ms

# Query path
timeseries_query_latency_p50_us
timeseries_query_latency_p95_us
timeseries_query_latency_p99_us
timeseries_query_count_total

# Compression
timeseries_compression_ratio
timeseries_gorilla_codec_latency_p99_us
timeseries_downsampling_latency_p99_ms

# Retention
timeseries_retention_cleanup_duration_ms
timeseries_points_deleted_total
timeseries_retention_policy_breaches_total

# Remote write
timeseries_remote_write_batch_count_total
timeseries_remote_write_success_total
timeseries_remote_write_failures_total
timeseries_remote_write_latency_p99_ms
```

### Alert Configuration (Prometheus)

```yaml
groups:
  - name: timeseries_sla
    interval: 30s
    rules:
      - alert: TimeseriesWriteLatencyP99High
        expr: timeseries_write_latency_p99_us > 1000
        for: 2m
        annotations:
          summary: "Timeseries write latency p99 > 1ms"

      - alert: TimeseriesQueryLatencyP99High
        expr: timeseries_query_latency_p99_us > 500
        for: 2m
        annotations:
          summary: "Timeseries query latency p99 > 500µs"

      - alert: TimeseriesBufferAlmostFull
        expr: timeseries_buffer_fill_percent > 90
        for: 30s
        annotations:
          summary: "Timeseries buffer > 90% full"

      - alert: TimeseriesFlushLatencyHigh
        expr: timeseries_flush_latency_p99_ms > 500
        for: 2m
        annotations:
          summary: "Timeseries flush latency p99 > 500ms"

      - alert: TimeseriesRetentionCleanupFailing
        expr: timeseries_retention_cleanup_duration_ms > 60000
        for: 5m
        annotations:
          summary: "Timeseries retention cleanup taking > 60s"

      - alert: TimeseriesRemoteWriteFailures
        expr: rate(timeseries_remote_write_failures_total[5m]) > 0.1
        for: 5m
        annotations:
          summary: "Timeseries remote-write failures > 10%"
```

### Grafana Dashboard Panels

```json
{
  "dashboard": {
    "title": "ThemisDB Timeseries Module",
    "panels": [
      {
        "title": "Write Throughput (pts/sec)",
        "targets": [
          { "expr": "timeseries_write_throughput_points_per_sec" }
        ]
      },
      {
        "title": "Write Latency Distribution",
        "targets": [
          { "expr": "timeseries_write_latency_p50_us" },
          { "expr": "timeseries_write_latency_p95_us" },
          { "expr": "timeseries_write_latency_p99_us" }
        ]
      },
      {
        "title": "Query Latency p99",
        "targets": [
          { "expr": "timeseries_query_latency_p99_us" }
        ]
      },
      {
        "title": "Buffer Fill Percentage",
        "targets": [
          { "expr": "timeseries_buffer_fill_percent" }
        ]
      },
      {
        "title": "Flush Events and Latency",
        "targets": [
          { "expr": "rate(timeseries_flush_count_total[5m])" },
          { "expr": "timeseries_flush_latency_p99_ms" }
        ]
      },
      {
        "title": "Compression Ratio",
        "targets": [
          { "expr": "timeseries_compression_ratio" }
        ]
      },
      {
        "title": "Retention Cleanup Duration",
        "targets": [
          { "expr": "timeseries_retention_cleanup_duration_ms" }
        ]
      },
      {
        "title": "Remote Write Success Rate",
        "targets": [
          { "expr": "rate(timeseries_remote_write_success_total[5m]) / (rate(timeseries_remote_write_success_total[5m]) + rate(timeseries_remote_write_failures_total[5m]))" }
        ]
      }
    ]
  }
}
```

---

## Troubleshooting Guide

### High Write Latency

**Symptoms:** `timeseries_write_latency_p99_us > 1000`

**Root Cause Checklist:**
1. Buffer nearly full? Check `timeseries_buffer_fill_percent > 75%`
   - **Fix:** Increase `THEMIS_TS_BUFFER_SIZE_MB` or decrease `THEMIS_TS_FLUSH_THRESHOLD_PERCENT`
2. Flush I/O slow? Check `timeseries_flush_latency_p99_ms > 100`
   - **Fix:** Check RocksDB compaction, disk I/O, IOPS limits
3. CPU throttled? Check system CPU utilization
   - **Fix:** Reduce concurrent ingest, scale horizontally
4. Memory pressure? Check system memory/swap
   - **Fix:** Reduce buffer size, increase available memory

### High Query Latency

**Symptoms:** `timeseries_query_latency_p99_us > 500`

**Root Cause Checklist:**
1. SIMD disabled? Check `THEMIS_TS_GORILLA_SIMD_ENABLED`
   - **Fix:** Enable SIMD: `export THEMIS_TS_GORILLA_SIMD_ENABLED=true`
2. Large result set? Check query point count
   - **Fix:** Reduce query window, enable downsampling
3. Cache misses? Check CPU profile
   - **Fix:** Increase query cache TTL, reduce series cardinality
4. Contention? Multiple concurrent queries?
   - **Fix:** Use read replicas, scale horizontally

### Buffer Exhaustion

**Symptoms:** `timeseries_buffer_fill_percent >= 100` and write rejection

**Root Cause Checklist:**
1. Flush falling behind? Check `timeseries_flush_latency_p99_ms`
   - **Fix:** Reduce buffer size to trigger flushes earlier
2. Storage I/O bottleneck? Check RocksDB latency
   - **Fix:** Increase storage IOPS, check SSD health
3. Backpressure watermark too high?
   - **Fix:** Lower `THEMIS_TS_WATERMARK_HIGH_PERCENT`

### Low Compression Ratio

**Symptoms:** `timeseries_compression_ratio > 0.30` (less than 3:1 compression)

**Root Cause Checklist:**
1. Buffer too small? Check `THEMIS_TS_BUFFER_SIZE_MB`
   - **Fix:** Increase to 512 MB or higher for better delta encoding
2. Data too random? Check timeseries patterns
   - **Fix:** Verify data is stable/monotonic (typical for timeseries)
3. Gorilla codec disabled? Check codec selection
   - **Fix:** Verify compression algorithm is Gorilla, not raw

### Retention Cleanup Failures

**Symptoms:** Cleanup latency increasing, retention policy violated

**Root Cause Checklist:**
1. Batch size too large? Check `THEMIS_TS_BATCH_DELETE_SIZE`
   - **Fix:** Reduce to 5000 or lower
2. Cleanup interval too long? Check `THEMIS_TS_CLEANUP_INTERVAL_SECONDS`
   - **Fix:** Run cleanup more frequently during off-peak
3. Storage compaction backlog? Check RocksDB stats
   - **Fix:** Manual compaction, tune RocksDB settings

### Remote Write Failures

**Symptoms:** `timeseries_remote_write_failures_total` increasing

**Root Cause Checklist:**
1. Network connectivity? Check Prometheus endpoint
   - **Fix:** `curl http://prometheus:9009/api/v1/write` (should timeout or return 405)
2. Timeout too short? Check `THEMIS_TS_REMOTE_WRITE_TIMEOUT_MS`
   - **Fix:** Increase to 10000 ms
3. Batch size mismatch? Check remote-write endpoint limits
   - **Fix:** Reduce `THEMIS_TS_REMOTE_WRITE_BATCH_SIZE`

---

## Incident Response Runbooks

### IR-01: Write Latency SLA Breach (p99 > 1 ms)

**Severity:** Medium  
**Response Time:** < 5 minutes

**Immediate Actions:**
1. Check buffer status: `SELECT timeseries_buffer_fill_percent`
2. If buffer > 90%, increase flush frequency (reduce threshold by 5%)
3. Check RocksDB compaction status
4. Verify disk IOPS available
5. Monitor recovery over next 2 minutes

**Root Cause Analysis (5-10 min):**
- Review flush latency trend (last 30 minutes)
- Check CPU utilization and memory pressure
- Examine recent config changes
- Verify storage backend health

**Recovery Actions:**
- If buffer issue: adjust `THEMIS_TS_BUFFER_SIZE_MB` +100 MB
- If storage issue: reduce `THEMIS_TS_FLUSH_THRESHOLD_PERCENT` by 10%
- If sustained, scale horizontally (add more ingest nodes)

**Post-Incident:**
- Review buffer tuning guidelines
- Check if retention cleanup is interfering
- Consider caching optimization

### IR-02: Query Latency SLA Breach (p99 > 500 µs)

**Severity:** Medium  
**Response Time:** < 5 minutes

**Immediate Actions:**
1. Verify SIMD is enabled: `echo $THEMIS_TS_GORILLA_SIMD_ENABLED`
2. Check CPU utilization (perf or system tools)
3. Monitor query concurrency (spike check)
4. Clear query cache if stale data suspected
5. Monitor recovery

**Root Cause Analysis (5-10 min):**
- Profile query latency (enable detailed logging)
- Check if related to specific metric or all series
- Verify series cardinality hasn't exploded
- Check for noisy neighbors (heavy writes)

**Recovery Actions:**
- If CPU bound: reduce concurrent queries, scale horizontally
- If cache issue: increase `query.cache_ttl_seconds`
- If large result sets: auto-downsample large ranges

**Post-Incident:**
- Review query patterns in application logs
- Consider caching optimization
- Verify SIMD compilation flags

### IR-03: Buffer Exhaustion

**Severity:** High  
**Response Time:** < 1 minute

**Immediate Actions:**
1. STOP accepting new writes (return HTTP 503)
2. Check flush status (should be maxed out)
3. Monitor storage I/O (should see heavy flush activity)
4. Wait for buffer to drain (typically 10-30 seconds)
5. Gradually re-enable writes

**Root Cause Analysis (5 min):**
- Check if storage is actually slow (iostat -x)
- Review ingest rate spike in logs
- Verify RocksDB compaction not stuck
- Check retention cleanup not running concurrently

**Recovery Actions:**
- If ingest spike: use rate limiting in application layer
- If storage slow: restart RocksDB, check SSD health
- If sustained: increase buffer size permanently
- Consider circuit-breaker pattern in client

**Post-Incident:**
- Review capacity planning calculations
- Adjust watermark thresholds (should trigger earlier)
- Consider auto-scaling policy

### IR-04: Retention Cleanup Stuck

**Severity:** Medium  
**Response Time:** < 10 minutes

**Immediate Actions:**
1. Check cleanup duration: `SELECT MAX(duration_ms) FROM timeseries_metrics WHERE operation='retention_cleanup' AND timestamp > NOW() - 1h`
2. If > 5 minutes, something is wrong
3. Check RocksDB compaction status
4. Monitor disk space (might be full)
5. Verify retention policy is feasible

**Root Cause Analysis (10 min):**
- Estimate data rate × retention days
- Check if disk space exceeded quota
- Verify batch delete size isn't too large
- Review RocksDB write amplification

**Recovery Actions:**
- Reduce `THEMIS_TS_BATCH_DELETE_SIZE` to 5000
- Schedule cleanup during off-peak (e.g., 2 AM)
- Manual cleanup if falling behind: `themis-tsctl cleanup --force`
- Consider shorter retention policy

**Post-Incident:**
- Capacity planning review (storage utilization trend)
- Tune cleanup interval and batch size
- Set up alerts for cleanup duration > 60s

---

## SLA and Performance Targets

### Write Path SLA

| Metric | Target | Notes |
|--------|--------|-------|
| Throughput | ≥ 1M points/sec | GATE-TSRG-01 |
| Latency p50 | < 100 µs | Best-case path |
| Latency p95 | < 500 µs | Normal operation |
| Latency p99 | < 1 ms | Acceptable peak |
| Availability | ≥ 99.9% | Momentary backpressure acceptable |

### Query Path SLA

| Metric | Target | Notes |
|--------|--------|-------|
| Latency p50 | 100–200 µs | GATE-TSRG-02 reference |
| Latency p95 | 300–400 µs | Normal operation |
| Latency p99 | ≤ 500 µs | GATE-TSRG-02 gate |
| Availability | ≥ 99.95% | Read-only, highest priority |
| Cache hit rate | ≥ 60% | Typical for repeated queries |

### Compression SLA

| Metric | Target | Notes |
|--------|--------|-------|
| Compression ratio | 0.15–0.25 | 4–7× compression (typical) |
| Codec latency p99 | ≤ 100 µs | GATE-TSRG-03 (100 points) |
| Availability | ≥ 99.99% | Fallback to raw if failing |

### Retention/Cleanup SLA

| Metric | Target | Notes |
|--------|--------|-------|
| Cleanup latency p99 | ≤ 60 seconds | Per cleanup run |
| Cleanup success rate | ≥ 99% | Retries as needed |
| Policy compliance | 100% | No data older than retention |

---

**Last Updated:** 2026-08-07  
**Document Status:** ✅ COMPLETE  
**Operator Review:** Ready for review and deployment
