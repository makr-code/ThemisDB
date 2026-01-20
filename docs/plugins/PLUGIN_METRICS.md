# Plugin Metrics and Monitoring

## Overview

ThemisDB provides comprehensive metrics and monitoring for all loaded plugins. The plugin metrics system tracks timing, counts, resource usage, and performance statistics for each plugin, making it easy to identify bottlenecks and monitor plugin health.

## Features

- **Automatic Tracking**: Metrics are automatically collected when plugins are loaded, reloaded, or called
- **Thread-Safe**: All metrics collection is thread-safe and designed for concurrent access
- **Low Overhead**: Minimal performance impact (<1%) on plugin operations
- **Multiple Formats**: Metrics available in both JSON and Prometheus formats
- **Percentile Tracking**: Accurate P95 and P99 latency percentiles

## Metrics Tracked

### Timing Metrics

- **Load Time**: Duration to load and initialize a plugin
- **Reload Time**: Duration to reload a plugin (hot-reload)
- **Loaded At**: Timestamp when the plugin was loaded
- **Call Latency**: Duration of individual plugin function calls

### Count Metrics

- **Reload Count**: Number of times the plugin has been reloaded
- **Function Calls**: Total number of function calls made to the plugin
- **Errors**: Number of errors encountered by the plugin

### Resource Metrics

- **Memory Usage**: Current memory usage in bytes (if tracked by plugin)

### Performance Metrics

- **Average Latency**: Mean call latency in milliseconds
- **P95 Latency**: 95th percentile call latency
- **P99 Latency**: 99th percentile call latency

## API Endpoints

### JSON Format: `/api/plugins/metrics`

Returns detailed metrics for all loaded plugins in JSON format.

**Example Request:**
```bash
curl http://localhost:8765/api/plugins/metrics
```

**Example Response:**
```json
{
  "onnx_clip": {
    "load_time_ms": 450,
    "last_reload_ms": 0,
    "loaded_at": "2026-01-20T09:00:00Z",
    "reload_count": 0,
    "function_calls": 1234,
    "errors": 0,
    "memory_bytes": 367001600,
    "avg_latency_ms": 12.5,
    "p95_latency_ms": 25.3,
    "p99_latency_ms": 45.7
  },
  "grpc_rpc": {
    "load_time_ms": 120,
    "last_reload_ms": 0,
    "loaded_at": "2026-01-20T09:00:00Z",
    "reload_count": 0,
    "function_calls": 5678,
    "errors": 2,
    "memory_bytes": 104857600,
    "avg_latency_ms": 3.2,
    "p95_latency_ms": 8.5,
    "p99_latency_ms": 15.3
  }
}
```

### Prometheus Format: `/metrics`

Plugin metrics are included in the standard Prometheus metrics endpoint.

**Example Request:**
```bash
curl http://localhost:8765/metrics
```

**Example Response:**
```
# HELP themis_plugin_loads_total Total number of plugin loads
# TYPE themis_plugin_loads_total counter
themis_plugin_loads_total{plugin="onnx_clip"} 1
themis_plugin_loads_total{plugin="grpc_rpc"} 1

# HELP themis_plugin_reloads_total Total number of plugin reloads
# TYPE themis_plugin_reloads_total counter
themis_plugin_reloads_total{plugin="onnx_clip"} 0
themis_plugin_reloads_total{plugin="grpc_rpc"} 0

# HELP themis_plugin_errors_total Total number of plugin errors
# TYPE themis_plugin_errors_total counter
themis_plugin_errors_total{plugin="onnx_clip"} 0
themis_plugin_errors_total{plugin="grpc_rpc"} 2

# HELP themis_plugin_function_calls_total Total number of plugin function calls
# TYPE themis_plugin_function_calls_total counter
themis_plugin_function_calls_total{plugin="onnx_clip"} 1234
themis_plugin_function_calls_total{plugin="grpc_rpc"} 5678

# HELP themis_plugin_load_duration_seconds Plugin load duration
# TYPE themis_plugin_load_duration_seconds histogram
themis_plugin_load_duration_seconds_sum{plugin="onnx_clip"} 0.45
themis_plugin_load_duration_seconds_count{plugin="onnx_clip"} 1
themis_plugin_load_duration_seconds_sum{plugin="grpc_rpc"} 0.12
themis_plugin_load_duration_seconds_count{plugin="grpc_rpc"} 1

# HELP themis_plugin_memory_bytes Plugin memory usage in bytes
# TYPE themis_plugin_memory_bytes gauge
themis_plugin_memory_bytes{plugin="onnx_clip"} 367001600
themis_plugin_memory_bytes{plugin="grpc_rpc"} 104857600

# HELP themis_plugin_call_latency_milliseconds Plugin call latency metrics
# TYPE themis_plugin_call_latency_milliseconds summary
themis_plugin_call_latency_milliseconds{plugin="onnx_clip",quantile="0.95"} 25.3
themis_plugin_call_latency_milliseconds{plugin="onnx_clip",quantile="0.99"} 45.7
themis_plugin_call_latency_milliseconds_sum{plugin="onnx_clip"} 15412.5
themis_plugin_call_latency_milliseconds_count{plugin="onnx_clip"} 1234
themis_plugin_call_latency_milliseconds{plugin="grpc_rpc",quantile="0.95"} 8.5
themis_plugin_call_latency_milliseconds{plugin="grpc_rpc",quantile="0.99"} 15.3
themis_plugin_call_latency_milliseconds_sum{plugin="grpc_rpc"} 18169.6
themis_plugin_call_latency_milliseconds_count{plugin="grpc_rpc"} 5678
```

## Programmatic Access

### C++ API

```cpp
#include "plugins/plugin_manager.h"

// Get plugin manager instance
auto& pm = themis::plugins::PluginManager::instance();

// Access metrics
const auto& metrics = pm.getMetrics();

// Get stats for a specific plugin
auto stats = metrics.getStats("onnx_clip");
std::cout << "Load time: " << stats.load_time.count() << "ms" << std::endl;
std::cout << "Function calls: " << stats.function_calls << std::endl;
std::cout << "Average latency: " << stats.avg_call_latency_ms << "ms" << std::endl;

// Get all plugin stats
auto all_stats = metrics.getAllStats();
for (const auto& [name, stats] : all_stats) {
    std::cout << "Plugin: " << name << std::endl;
    std::cout << "  Errors: " << stats.errors << std::endl;
}
```

### Recording Custom Metrics

If you're developing a plugin and want to record additional metrics:

```cpp
// In your plugin implementation
auto& pm = themis::plugins::PluginManager::instance();

// Record function call latency
auto start = std::chrono::steady_clock::now();
// ... your function logic ...
auto end = std::chrono::steady_clock::now();
auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
pm.getMetricsMutable().recordCall("my_plugin", latency);

// Record an error
pm.getMetricsMutable().recordError("my_plugin");

// Update memory usage
size_t memory_bytes = getCurrentMemoryUsage();
pm.getMetricsMutable().updateMemoryUsage("my_plugin", memory_bytes);
```

## Grafana Dashboard

### Setting up Prometheus

1. Configure Prometheus to scrape ThemisDB metrics:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:8765']
    metrics_path: '/metrics'
    scrape_interval: 15s
```

2. Start Prometheus:
```bash
prometheus --config.file=prometheus.yml
```

### Grafana Queries

**Plugin Load Times:**
```promql
themis_plugin_load_duration_seconds_sum
```

**Plugin Call Rate (calls/sec):**
```promql
rate(themis_plugin_function_calls_total[5m])
```

**Plugin Error Rate:**
```promql
rate(themis_plugin_errors_total[5m])
```

**Plugin P95 Latency:**
```promql
themis_plugin_call_latency_milliseconds{quantile="0.95"}
```

**Plugin P99 Latency:**
```promql
themis_plugin_call_latency_milliseconds{quantile="0.99"}
```

**Plugin Memory Usage:**
```promql
themis_plugin_memory_bytes
```

## Monitoring Best Practices

### Performance Thresholds

Consider setting alerts for:

1. **High Error Rate**: More than 1% errors
   ```promql
   rate(themis_plugin_errors_total[5m]) / rate(themis_plugin_function_calls_total[5m]) > 0.01
   ```

2. **High Latency**: P99 latency > 100ms
   ```promql
   themis_plugin_call_latency_milliseconds{quantile="0.99"} > 100
   ```

3. **Memory Leaks**: Continuously increasing memory
   ```promql
   deriv(themis_plugin_memory_bytes[10m]) > 0
   ```

### Capacity Planning

Use metrics to:
- Identify which plugins are most resource-intensive
- Determine optimal plugin loading strategy
- Plan hardware upgrades based on actual usage patterns

## Troubleshooting

### High Plugin Load Time

If a plugin takes too long to load:

1. Check plugin dependencies and initialization logic
2. Consider lazy initialization for heavy resources
3. Profile plugin initialization code
4. Check for network dependencies during initialization

### High Call Latency

If plugin calls are slow:

1. Check P95/P99 vs average - high percentiles indicate outliers
2. Look for lock contention in thread-safe plugins
3. Profile hot paths in plugin code
4. Consider caching frequently accessed data

### Memory Issues

If memory usage is high or growing:

1. Check for resource leaks in plugin shutdown
2. Verify proper cleanup of allocated resources
3. Consider implementing memory limits
4. Use memory profiling tools

## Implementation Details

### Metric Storage

- Metrics are stored in-memory per plugin
- Latency samples use a circular buffer (max 1000 samples)
- Thread-safe implementation using mutexes
- Minimal overhead (<1% on plugin operations)

### Percentile Calculation

- P95/P99 calculated from sorted latency samples
- Samples stored in circular buffer (FIFO)
- Recalculated on each new sample
- Efficient for monitoring use cases

## Future Enhancements

Planned improvements:

- OpenTelemetry integration for distributed tracing
- Custom metric labels for plugin-specific dimensions
- Metric retention policies and aggregation
- Grafana dashboard templates
- Alert rule templates

## See Also

- [Plugin System Architecture](../plugins/README.md)
- [Plugin Development Guide](../docs/development/plugin-development.md)
- [Monitoring Best Practices](../docs/operations/monitoring.md)
