# Time Series Metrics

ThemisDB provides comprehensive monitoring and metrics for time series operations, enabling observability into ingestion performance, query latency, compression efficiency, and storage utilization.

## Overview

The `TimeSeriesMetrics` class tracks detailed performance metrics for all time series operations in TSStore. Metrics can be exported in both JSON and Prometheus text formats for integration with monitoring systems.

## Features

- **Ingestion Metrics**: Track data point writes, batch operations, and throughput
- **Query Metrics**: Monitor query count, latency, and result set sizes
- **Aggregation Metrics**: Track aggregation operations and query optimizer effectiveness
- **Storage Metrics**: Monitor data size, compression ratio, and retention operations
- **Per-Metric Statistics**: Optional detailed statistics per metric name
- **Prometheus Integration**: Native Prometheus text format export
- **JSON Export**: Structured JSON format for custom integrations

## HTTP Endpoint

### GET /ts/metrics

Returns time series metrics in JSON or Prometheus format.

**Query Parameters:**
- `format` (optional): Output format, either `json` (default) or `prometheus`

**Example Request (JSON format):**
```bash
curl http://localhost:8080/ts/metrics
```

**Example Request (Prometheus format):**
```bash
curl http://localhost:8080/ts/metrics?format=prometheus
```

**JSON Response Example:**
```json
{
  "ingestion": {
    "data_points_written_total": 15234,
    "batches_written_total": 52,
    "compressed_batches_total": 48,
    "write_errors_total": 0,
    "write_latency_ms_avg": 1.25
  },
  "query": {
    "queries_executed_total": 1432,
    "aggregations_executed_total": 234,
    "data_points_returned_total": 523421,
    "query_latency_ms_avg": 8.45
  },
  "optimizer": {
    "hits_total": 189,
    "misses_total": 45,
    "hit_rate": 0.807
  },
  "storage": {
    "current_data_points": 1523400,
    "current_metrics": 127,
    "current_storage_bytes": 45231890,
    "bytes_written_uncompressed_total": 482193840,
    "bytes_written_compressed_total": 38492834,
    "compression_ratio_avg": 12.53
  },
  "retention": {
    "runs_total": 24,
    "data_points_deleted_total": 82341
  },
  "continuous_aggregates": {
    "refreshes_total": 156,
    "points_generated_total": 3421
  },
  "per_metric_stats": {
    "cpu_usage": {
      "data_points_written": 5234,
      "queries_executed": 432,
      "bytes_written": 1234567,
      "avg_write_latency_ms": 1.12,
      "avg_query_latency_ms": 7.89
    },
    "memory_usage": {
      "data_points_written": 5234,
      "queries_executed": 387,
      "bytes_written": 1198234,
      "avg_write_latency_ms": 1.18,
      "avg_query_latency_ms": 8.12
    }
  }
}
```

**Prometheus Response Example:**
```prometheus
# HELP themis_timeseries_data_points_written_total Total number of time series data points written
# TYPE themis_timeseries_data_points_written_total counter
themis_timeseries_data_points_written_total 15234

# HELP themis_timeseries_queries_executed_total Total number of time series queries executed
# TYPE themis_timeseries_queries_executed_total counter
themis_timeseries_queries_executed_total 1432

# HELP themis_timeseries_compression_ratio_avg Average compression ratio (uncompressed/compressed)
# TYPE themis_timeseries_compression_ratio_avg gauge
themis_timeseries_compression_ratio_avg 12.530000

# HELP themis_timeseries_write_latency_ms_avg Average write operation latency in milliseconds
# TYPE themis_timeseries_write_latency_ms_avg gauge
themis_timeseries_write_latency_ms_avg 1.250000
```

## Available Metrics

### Ingestion Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `data_points_written_total` | Counter | Total number of data points written |
| `batches_written_total` | Counter | Total number of batch write operations |
| `compressed_batches_total` | Counter | Total number of compressed batch writes |
| `write_errors_total` | Counter | Total number of write errors |
| `write_latency_ms_avg` | Gauge | Average write operation latency (ms) |
| `bytes_written_uncompressed_total` | Counter | Total bytes written (before compression) |
| `bytes_written_compressed_total` | Counter | Total bytes written (after compression) |

### Query Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `queries_executed_total` | Counter | Total number of queries executed |
| `aggregations_executed_total` | Counter | Total number of aggregation operations |
| `data_points_returned_total` | Counter | Total number of data points returned |
| `query_latency_ms_avg` | Gauge | Average query latency (ms) |

### Optimizer Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `optimizer_hits_total` | Counter | Number of times query optimizer successfully optimized a query |
| `optimizer_misses_total` | Counter | Number of times query optimizer could not optimize a query |
| `hit_rate` | Gauge | Optimizer hit rate (hits / total) |

### Storage Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `current_data_points` | Gauge | Current number of data points stored |
| `current_metrics` | Gauge | Current number of unique metrics |
| `current_storage_bytes` | Gauge | Current storage size in bytes |
| `compression_ratio_avg` | Gauge | Average compression ratio (uncompressed/compressed) |

### Retention Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `retention_runs_total` | Counter | Total number of retention policy executions |
| `data_points_deleted_total` | Counter | Total number of data points deleted by retention |

### Continuous Aggregate Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `continuous_agg_refreshes_total` | Counter | Total number of continuous aggregate refreshes |
| `continuous_agg_points_generated_total` | Counter | Total number of aggregate points generated |

## Enabling Metrics

Metrics are automatically collected when you attach a `TimeSeriesMetrics` instance to your `TSStore`:

```cpp
#include "timeseries/tsstore.h"
#include "timeseries/timeseries_metrics.h"

// Create metrics collector
TimeSeriesMetrics::Config metrics_config;
metrics_config.enable_per_metric_stats = true;
auto metrics = std::make_shared<TimeSeriesMetrics>(metrics_config);

// Create TSStore
auto ts_store = std::make_shared<TSStore>(db, cf);

// Attach metrics
ts_store->setMetrics(metrics);

// All TSStore operations now record metrics automatically
ts_store->putDataPoint({...});
ts_store->query({...});
```

## Configuration

The `TimeSeriesMetrics::Config` struct provides configuration options:

```cpp
struct Config {
    bool enable_histograms = true;        // Enable histogram metrics (future)
    bool enable_per_metric_stats = true;  // Enable per-metric statistics
    int histogram_buckets = 20;           // Number of histogram buckets (future)
};
```

## Integration with Prometheus

To scrape metrics with Prometheus, add the endpoint to your `prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'themisdb_timeseries'
    static_configs:
      - targets: ['localhost:8080']
    metrics_path: '/ts/metrics'
    params:
      format: ['prometheus']
```

## Grafana Dashboard

A Grafana dashboard template for time series metrics is available at `grafana/dashboards/timeseries_metrics.json` (to be created).

### Key Panels

1. **Ingestion Rate**: Data points written per second
2. **Query Latency**: P50, P95, P99 query latencies
3. **Compression Ratio**: Storage efficiency over time
4. **Optimizer Hit Rate**: Query optimizer effectiveness
5. **Storage Growth**: Data points and bytes over time
6. **Error Rate**: Write errors per second

## Best Practices

1. **Enable metrics in production**: Minimal performance overhead (<1%)
2. **Monitor compression ratio**: Ensure Gorilla compression is effective
3. **Track optimizer hit rate**: High hit rate indicates good pre-computation
4. **Set up alerts**: Alert on high error rates or query latencies
5. **Use per-metric stats selectively**: Enable only when needed for debugging

## Performance Impact

- **Metrics Collection**: <0.5% overhead on write operations
- **Metrics Export**: Negligible, operations are atomic reads
- **Per-Metric Stats**: Additional ~100 bytes per metric tracked
- **Memory Usage**: ~1KB base + per-metric overhead

## See Also

- [Time Series Documentation](features_time_series.md)
- [TSStore API](../../include/timeseries/tsstore.h)
- [Monitoring & Observability](../observability/observability_prometheus.md)
