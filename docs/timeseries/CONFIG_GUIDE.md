# TimeSeries Module – Configuration Guide

## Overview

This guide covers all configuration options for the ThemisDB TimeSeries module,
including TSStore, the Gorilla codec, continuous aggregation, retention, auto-buffer,
the query optimizer, and hypertable storage.

---

## 1. TSStore – Core Configuration

```cpp
TSStore::Config config;
config.compression  = TSStore::CompressionType::Gorilla; // Gorilla (default) | None | Snappy
config.chunk_size_hours = 24;   // One RocksDB chunk per 24h
config.enable_gorilla    = true; // Use Gorilla codec for batch writes
```

| Option | Default | Description |
|--------|---------|-------------|
| `compression` | `Gorilla` | Compression codec for batch inserts |
| `chunk_size_hours` | `24` | Hours per storage chunk |
| `enable_gorilla` | `true` | Enable Gorilla codec |

---

## 2. Gorilla Codec

The Gorilla codec uses:
- **Timestamps**: Delta-of-delta encoding with ZigZag + varint (best for uniform cadence)
- **Values**: XOR of IEEE-754 bit patterns with leading/trailing zero optimization

### Error Recovery (v1.5.0+)

```cpp
GorillaDecoder dec(bytes);
while (auto point = dec.next()) {
    // process point
}
if (dec.hasError()) {
    // truncated or corrupted data detected
}
size_t good_points = dec.decodedCount();
```

### Compression Ratio Guidelines

| Data Pattern | Typical Ratio |
|---|---|
| Constant values, uniform timestamps | 20:1 – 50:1 |
| Smooth signal (sine, temperature) | 5:1 – 15:1 |
| Random IEEE-754 values | 1:1 – 2:1 |

---

## 3. Continuous Aggregation

### Single-Window Refresh

```cpp
ContinuousAggregateManager mgr(store);
AggConfig cfg;
cfg.metric      = "cpu_usage";
cfg.entity      = "server01";
cfg.window.size = std::chrono::minutes(5);
mgr.refresh(cfg, from_ms, to_ms);
```

### Rollup Hierarchy (1m → 5m → 1h → 1d)

```cpp
auto hierarchy = RollupHierarchy::defaultHierarchy("cpu_usage", "server01");
mgr.refreshHierarchy(hierarchy, from_ms, to_ms);

// Produces derived metrics:
//   cpu_usage__agg_60000ms
//   cpu_usage__agg_60000ms__agg_300000ms
//   cpu_usage__agg_60000ms__agg_300000ms__agg_3600000ms
//   ...
```

### Aggregate Scheduler

```cpp
AggregateScheduler sched(store);
AggregateScheduler::Config cfg;
cfg.check_interval = std::chrono::seconds(60);
sched.start();
auto id = sched.registerAggregate(agg_cfg, std::chrono::minutes(5));
sched.refreshNow(id);  // Manual catch-up
sched.stop();
```

---

## 4. Hypertable

```cpp
Hypertable::Config cfg;
cfg.table_name             = "metrics";
cfg.chunk_interval_seconds = 86400;   // 1-day chunks (default)
cfg.retention_days         = 90;      // Keep 90 days
cfg.auto_create_chunks     = true;    // Create chunks on first write
cfg.compress_old_chunks    = true;    // ZSTD-compress chunks >7 days

Hypertable ht(db.get(), cfg);
ht.insert(timestamp_s, json_data);
```

### Chunk Health Monitoring (v1.5.0+)

```cpp
for (const auto& health : ht.getChunkHealth()) {
    switch (health.status) {
        case Hypertable::ChunkStatus::Active:       // Writing to this chunk
        case Hypertable::ChunkStatus::Frozen:       // Read-only, within retention
        case Hypertable::ChunkStatus::Compressible: // Eligible for compression
        case Hypertable::ChunkStatus::Compressed:   // Already compressed
        case Hypertable::ChunkStatus::Expired:      // Past retention → delete
    }
    if (!health.is_healthy) {
        // Alert / schedule cleanup
    }
}
```

---

## 5. Query Optimizer

```cpp
TSQueryOptimizer opt(store);

TSQueryOptimizer::OptimizationHint hint;
hint.use_aggregates        = true;
hint.min_window_for_agg_ms = 3600000;  // Use aggregates for queries >1h
hint.max_raw_points        = 10000;    // Force aggregates when >10k raw points
hint.explain               = true;    // Include explanation in plan

// Predicate filter (v1.5.0+)
hint.predicates.push_back(
    TSQueryOptimizer::PredicateFilter::eq("region", "us-east")
);

// Query plan cache (v1.5.0+)
hint.use_cache = true;

auto plan = opt.optimizeAggregateQuery("cpu", "server01", from_ms, to_ms, hint);
// plan.source_metric → use this metric name for the actual query
// plan.explanation   → human-readable optimization notes
// plan.estimated_speedup → speedup factor vs raw query
```

### Cache Management

```cpp
opt.clearCache();          // Clear all cached plans
opt.cacheSize();           // Current cache entries
opt.cacheHits();           // Total hits since creation
opt.cacheMisses();         // Total misses since creation
```

---

## 6. Auto-Buffer (TSAutoBuffer)

```cpp
TSAutoBufferConfig cfg;
cfg.max_points_per_buffer      = 1000;     // Flush when buffer reaches 1k points
cfg.max_total_points           = 10000;    // Global point limit
cfg.flush_interval             = std::chrono::seconds(5);
cfg.max_memory_bytes           = 100 * 1024 * 1024; // 100 MB global limit
cfg.max_memory_per_metric_bytes = 1 * 1024 * 1024;  // 1 MB per metric (v1.5.0+)
cfg.enable_dedup               = true;    // Dedup by timestamp (v1.5.0+)
cfg.async_flush                = true;    // Background flush thread
cfg.compression                = TSStore::CompressionType::Gorilla;

TSAutoBuffer buf(store, cfg);
buf.start();
buf.add(point);   // ERR_API_RESOURCE_EXHAUSTED if per-metric limit hit
buf.flush();
buf.stop();
```

### Stats

```cpp
auto stats = buf.getStats();
stats.points_buffered.load()
stats.points_flushed.load()
stats.dedup_dropped_count.load()       // Points dropped (duplicate timestamp)
stats.memory_limit_rejected_count.load() // Points rejected (per-metric limit)
```

---

## 7. Retention & Cleanup

### Simple Retention

```cpp
RetentionPolicy policy;
policy.per_metric["cpu"]  = std::chrono::days(90);
policy.per_metric["logs"] = std::chrono::days(7);

RetentionManager mgr(store, policy);
mgr.apply();  // Synchronous run
```

### Async Background Cleanup

```cpp
mgr.startAsync(std::chrono::hours(6));  // Every 6 hours
mgr.stopAsync();
```

### Staged/Graduated Deletion (v1.5.0+)

```cpp
StagedDeletionPolicy staged;
staged.mark_after        = std::chrono::days(30);  // Flag after 30d
staged.soft_delete_after = std::chrono::days(60);  // Move to cold after 60d
staged.hard_delete_after = std::chrono::days(90);  // Permanently delete after 90d
mgr.setStagedDeletion(staged);
```

### Compliance Audit Logging (v1.5.0+)

```cpp
mgr.setAuditCallback([](const RetentionAuditEntry& e) {
    // Write to audit log, SIEM, or compliance system
    fmt::print("[AUDIT] {} action={} records={} metric={}\n",
               e.timestamp_ms, e.action, e.records_affected, e.metric);
});

auto audit_log = mgr.getAuditLog();  // In-memory rolling audit log
mgr.clearAuditLog();
```

---

## 8. Observability / Prometheus Export

```cpp
TimeSeriesMetrics metrics;
metrics.recordDataPointWrite("cpu", latency_ms, success);
metrics.recordQuery("cpu", latency_ms, result_count, time_range_ms);
metrics.recordContinuousAggregateRefresh("cpu", 60000, latency_ms, points_processed);

// Export in Prometheus text format
std::string prom_output = metrics.exportPrometheus();

// Key metric names:
// themis_timeseries_data_points_written_total
// themis_timeseries_queries_executed_total
// themis_timeseries_optimizer_hits_total
// themis_timeseries_optimizer_misses_total
// themis_timeseries_write_latency_avg_ms
// themis_timeseries_query_latency_avg_ms
// themis_timeseries_compression_ratio
```

---

## 9. Tuning Manual

### High-Throughput Ingestion

```
max_points_per_buffer      = 5000
flush_interval             = 10s
async_flush                = true
compression                = Gorilla
chunk_size_hours           = 24
```

### Low-Latency Queries

```
use_aggregates             = true
min_window_for_agg_ms      = 0
max_raw_points             = 100
use_cache                  = true
```

### Long-Term Storage (90+ days)

```
retention_days             = 90
compress_old_chunks        = true
chunk_interval_seconds     = 86400  (1 day)
staged_deletion.mark_after = 30d
staged_deletion.hard_delete_after = 90d
```

### Deduplication (IoT / unreliable sources)

```
enable_dedup               = true
max_memory_per_metric_bytes = 10 MB
```
