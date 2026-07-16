# Timeseries Troubleshooting Guide

The `timeseries` module provides time-series data management for ThemisDB, including hypertables, Gorilla-compressed chunks, continuous aggregates, retention policies, and time-series query optimization.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `GorillaCoder: bit buffer underflow` | Corrupt chunk data | Run `themisdb-admin timeseries repair-chunk` |
| Chunk rollover stalls | New column family creation fails | Check disk space and RocksDB file limits |
| Continuous aggregate lags behind | Refresh interval too long | Reduce `timeseries.continuous_agg.refresh_interval_ms` |
| Retention policy not deleting old data | Retention job not scheduled | Check `timeseries.retention.enabled: true` |
| Boundary data missing in range query | Chunk boundary alignment issue | Use `INCLUSIVE` range syntax |
| `COUNT(*)` on hypertable very slow | No count cache | Enable `timeseries.count_cache: true` |
| Metrics double-registration | Server restarted without flush | Call `themisdb-admin timeseries reset-metrics` |
| Memtable write stall | Too many open chunks | Limit `timeseries.max_open_chunks` |
| Query returns duplicate timestamps | Data ingested twice | Enable deduplication: `timeseries.dedup: true` |
| Out-of-order data silently dropped | `allow_out_of_order: false` | Enable out-of-order ingestion |

## Common Issues

### Issue 1: Gorilla Compression Corruption

**Description:** Reading a compressed time-series chunk returns corrupt data.

**Symptoms:**
- Log: `Gorilla: bit buffer underflow at offset=8192 in chunk=chunk-20250101`
- Query on time range returns error or garbage values

**Cause:** Chunk was partially written (crash during flush) or disk corruption.

**Solution:**
```bash
# Identify corrupt chunks
themisdb-admin timeseries chunk-health --collection metrics

# Repair or remove corrupt chunks
themisdb-admin timeseries repair-chunk \
  --collection metrics \
  --chunk chunk-20250101 \
  --action truncate-at-error

# Reingest from backup if available
themisdb-admin timeseries reingest \
  --collection metrics \
  --source backup \
  --start 2025-01-01 --end 2025-01-02
```

---

### Issue 2: Chunk Rollover Stall

**Description:** New time-series chunks cannot be created, blocking writes.

**Symptoms:**
- Log: `Hypertable: failed to create new column family for chunk-20250601`
- Write errors for data with timestamps beyond the current chunk

**Cause:** RocksDB has reached `max_open_files` limit; disk is full.

**Solution:**
```bash
# Check disk space
df -h /var/lib/themisdb/

# Check open file handles
lsof -p $(pidof themisdb) | wc -l
ulimit -n
```
```yaml
timeseries:
  chunk_interval: 86400000          # 24 hours per chunk (increase for fewer chunks)
  max_open_chunks: 100              # reduce to limit open column families

storage:
  rocksdb:
    max_open_files: 50000           # increase open files limit
```
```bash
# Also increase system limits
echo "themisdb soft nofile 65536" >> /etc/security/limits.conf
echo "themisdb hard nofile 65536" >> /etc/security/limits.conf
```

---

### Issue 3: Continuous Aggregate Lags Behind Ingestion

**Description:** Continuous aggregates are significantly behind real-time data.

**Symptoms:**
- Materialized view `metrics_hourly` is 6 hours behind current time
- Log: `ContinuousAgg: refresh lag=21600000ms for view=metrics_hourly`

**Cause:** Refresh interval is too long; aggregation job is slow.

**Solution:**
```yaml
timeseries:
  continuous_agg:
    refresh_interval_ms: 60000      # reduce from default 3600000
    batch_size: 10000
    parallel_refresh: true
    max_refresh_workers: 4
    lag_alert_threshold_ms: 300000
```
```bash
# Manually trigger refresh
themisdb-admin timeseries continuous-agg refresh \
  --view metrics_hourly \
  --start "1 day ago"
```

---

### Issue 4: Retention Policy Does Not Delete Old Data

**Description:** Old time-series data is not being deleted according to the retention policy.

**Symptoms:**
- Disk usage keeps growing
- Log: `Retention: no policy found for collection=metrics`

**Cause:** Retention policy is disabled or not associated with the collection.

**Solution:**
```yaml
timeseries:
  retention:
    enabled: true
    default_retention_days: 90
    check_interval_ms: 3600000      # check hourly
    collections:
      metrics:
        retention_days: 30
      events:
        retention_days: 365
```
```bash
# Manually run retention cleanup
themisdb-admin timeseries retention run --collection metrics --dry-run
themisdb-admin timeseries retention run --collection metrics
```

---

### Issue 5: Out-of-Order Data Silently Dropped

**Description:** Historical data ingested with past timestamps is silently ignored.

**Symptoms:**
- Data with timestamp `T-3h` not appearing in queries
- Log: `Hypertable: dropping out-of-order data point (lag=10800s > max_lag=3600s)`

**Cause:** `max_out_of_order_lag_ms` is too short.

**Solution:**
```yaml
timeseries:
  allow_out_of_order: true
  max_out_of_order_lag_ms: 86400000  # accept data up to 24h late
  out_of_order_buffer_size: 100000
  out_of_order_action: insert        # "insert" | "drop" | "error"
```

---

### Issue 6: Slow `COUNT(*)` on Large Hypertable

**Description:** Counting rows in a large time-series collection is very slow.

**Symptoms:**
- `SELECT COUNT(*) FROM metrics` takes > 60 seconds
- Log: `TimeseriesQueryOptimizer: using chunk scan for COUNT (no count cache)`

**Cause:** Count cache is disabled; RocksDB must scan all chunks.

**Solution:**
```yaml
timeseries:
  count_cache:
    enabled: true
    update_interval_ms: 60000       # update count every minute
    approximate: true               # fast approximate count
```
```sql
-- Use approximate count for dashboards
SELECT APPROX_COUNT(*) FROM metrics
  WHERE time > NOW() - INTERVAL 1 DAY;
```

---

### Issue 7: Duplicate Timestamps on Reingest

**Description:** Reingesting data creates duplicate entries for the same timestamp.

**Symptoms:**
- Queries return two data points for the same timestamp after reingest
- Aggregates are doubled

**Cause:** Deduplication is disabled; reingest does not check for existing data.

**Solution:**
```yaml
timeseries:
  dedup:
    enabled: true
    strategy: upsert               # "upsert" | "skip" | "error" | "append"
    dedup_window_ms: 3600000       # deduplicate within 1-hour window
```
```bash
# Remove duplicates from an existing collection
themisdb-admin timeseries dedup \
  --collection metrics \
  --strategy keep-first \
  --dry-run
```

---

### Issue 8: Memtable Write Stall from Too Many Open Chunks

**Description:** Write performance degrades because RocksDB memtables stall.

**Symptoms:**
- Log: `[RocksDB] Write stall: too many column families with pending memtable flush`
- Ingestion latency spikes to > 500ms

**Cause:** Each open timeseries chunk is a separate RocksDB column family; too many open at once.

**Solution:**
```yaml
timeseries:
  max_open_chunks: 50              # reduce from default 200
  chunk_interval: 3600000          # 1 hour chunks → fewer column families

storage:
  rocksdb:
    max_background_flushes: 4
    max_write_buffer_number: 6
```

## Diagnostic Commands

```bash
# Hypertable status
themisdb-admin timeseries status --collection metrics

# Chunk health check
themisdb-admin timeseries chunk-health --collection metrics

# Continuous aggregate lag
themisdb-admin timeseries continuous-agg status

# Retention job status
themisdb-admin timeseries retention status

# Deduplication stats
themisdb-admin timeseries dedup-stats --collection metrics

# Live time-series metrics
curl -s http://localhost:9100/metrics | grep themisdb_timeseries

# Tail timeseries logs
journalctl -u themisdb -f | grep -E "timeseries|hypertable|gorilla|chunk|continuous.agg|retention"
```

## Configuration Reference

```yaml
timeseries:
  enabled: true
  chunk_interval: 86400000          # 24 hours
  max_open_chunks: 100
  compression:
    enabled: true
    algorithm: gorilla
    compress_after_ms: 3600000
  continuous_agg:
    refresh_interval_ms: 300000
    parallel_refresh: true
  retention:
    enabled: true
    default_retention_days: 90
  dedup:
    enabled: true
    strategy: upsert
  allow_out_of_order: false
  count_cache:
    enabled: true
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `max_open_chunks` | `500` | `50–100` |
| `retention.enabled` | `false` | `true` |
| `allow_out_of_order` | `false` | `true` for late data |
| `continuous_agg.refresh_interval_ms` | `3600000` | `60000–300000` |

## Known Limitations

- Gorilla compression only works with `float64` values; integer or string series use raw encoding.
- Continuous aggregates do not support custom aggregation functions defined in plugins.
- Chunk compaction may temporarily double disk usage; plan maintenance windows for large datasets.
- Out-of-order ingestion with `max_lag > 24h` significantly increases memory usage.

## Related Documentation

- [Timeseries Module ROADMAP](../../src/timeseries/ROADMAP.md)
- [Timeseries Roadmap](../timeseries/index.md)
- [Compression and Encoding Strategies](../performance/compression_and_encoding_strategies.md)
- [Compression Configuration](../build-guide/compression_configuration.md)
- [Dynamic Batch Size Adaptation](../llm_orchestration/DYNAMIC_BATCH_SIZE_ADAPTATION.md)
