# Acceleration Metrics System

**Version:** 1.0  
**Status:** Production-ready  
**Last Updated:** 2026-04-06

---

## Overview

The ThemisDB acceleration metrics system provides comprehensive, production-grade observability for all acceleration backends (CUDA, HIP, OpenCL, Metal, CPU). It offers:

- **Real-time metrics** with sub-microsecond overhead
- **Prometheus-compatible** export format
- **JSON export** for custom integrations
- **Thread-safe** operations
- **Zero-allocation** in hot paths (atomic operations)

---

## Architecture

### Components

1. **MetricsCollector**: Singleton collector for all metrics
2. **Metric Types**: Counter, Gauge, Histogram, Summary
3. **BackendMetrics**: Pre-configured metrics for backends
4. **Timer**: RAII timer for automatic duration measurement

### Metric Types

#### Counter
Monotonically increasing value (never decreases).

**Use cases:**
- Total operations count
- Error counts
- Successful initializations

**Example:**
```cpp
auto* counter = MetricsCollector::instance().registerCounter(
    "operations_total", "Total number of operations");
counter->increment();  // Atomic, lock-free
counter->increment(10);  // Bulk increment
```

#### Gauge
Value that can go up or down.

**Use cases:**
- Memory usage
- Queue depth
- Active connections

**Example:**
```cpp
auto* gauge = MetricsCollector::instance().registerGauge(
    "memory_used_bytes", "Current memory usage");
gauge->set(1024*1024);  // Set to 1MB
gauge->increment(512);  // Add 512 bytes
gauge->decrement(256);  // Free 256 bytes
```

#### Histogram
Distribution of observed values.

**Use cases:**
- Operation duration
- Request latency
- Batch sizes

**Example:**
```cpp
std::vector<double> buckets = {0.001, 0.01, 0.1, 1.0};
auto* histogram = MetricsCollector::instance().registerHistogram(
    "operation_duration_seconds", 
    "Duration of operations",
    buckets);

histogram->observe(0.05);  // 50ms operation
```

---

## Backend Metrics

Each backend automatically gets a standard set of metrics:

### Initialization Metrics
- `{backend}_init_success_total` - Successful inits
- `{backend}_init_failures_total` - Failed inits
- `{backend}_init_duration_seconds` - Init duration histogram

### Operation Metrics
- `{backend}_l2_distance_duration_seconds` - L2 distance timing
- `{backend}_l2_distance_operations_total` - L2 operation count
- `{backend}_l2_distance_vectors_total` - Vectors processed
- `{backend}_cosine_duration_seconds` - Cosine similarity timing
- `{backend}_cosine_operations_total` - Cosine operation count
- `{backend}_cosine_vectors_total` - Vectors processed

### Resource Metrics
- `{backend}_device_memory_used_bytes` - Current memory usage
- `{backend}_device_memory_available_bytes` - Available memory
- `{backend}_queue_depth` - Command queue depth

### Error Metrics
- `{backend}_errors_total` - Total errors
- `{backend}_kernel_launch_failures_total` - Kernel failures
- `{backend}_memory_allocation_failures_total` - Allocation failures

### Device Metrics
- `{backend}_device_count` - Number of devices
- `{backend}_active_device_index` - Active device

---

## Usage

### Basic Usage

```cpp
#include "acceleration/metrics/backend_metrics.h"

class MyCUDABackend {
public:
    MyCUDABackend() : metrics_("CUDA") {}
    
    bool initialize() {
        Timer timer(&init_duration_);
        
        // ... initialization code ...
        
        if (success) {
            metrics_.recordInitSuccess();
            metrics_.setDeviceCount(deviceCount);
            return true;
        } else {
            metrics_.recordInitFailure();
            return false;
        }
    }
    
    void computeL2Distance(/* args */) {
        Timer timer(&l2_duration_);
        
        // ... computation ...
        
        metrics_.recordL2DistanceOperation(
            timer.elapsed(), vectorCount);
    }
    
private:
    BackendMetrics metrics_;
};
```

### RAII Timer

The `Timer` class automatically measures duration:

```cpp
auto* histogram = MetricsCollector::instance().registerHistogram(
    "my_operation_duration", "My operation timing");

{
    Timer timer(histogram);
    // ... operation ...
}  // Duration automatically recorded on destruction
```

### Custom Metrics

```cpp
auto& collector = MetricsCollector::instance();

// Counter
auto* requests = collector.registerCounter(
    "api_requests_total", "Total API requests");
requests->increment();

// Gauge
auto* active_connections = collector.registerGauge(
    "active_connections", "Number of active connections");
active_connections->set(42);

// Histogram with custom buckets
auto* latency = collector.registerHistogram(
    "request_latency_seconds",
    "Request latency in seconds",
    {0.001, 0.01, 0.1, 1.0, 10.0});
latency->observe(0.05);
```

---

## Export Formats

### Prometheus Format

```cpp
std::string metrics = MetricsCollector::instance().exportPrometheus();
std::cout << metrics << std::endl;
```

**Output:**
```
# HELP themis_acceleration_CUDA_init_success_total Number of successful backend initializations
# TYPE themis_acceleration_CUDA_init_success_total counter
themis_acceleration_CUDA_init_success_total{} 1

# HELP themis_acceleration_CUDA_l2_distance_duration_seconds Duration of L2 distance operations in seconds
# TYPE themis_acceleration_CUDA_l2_distance_duration_seconds histogram
themis_acceleration_CUDA_l2_distance_duration_seconds_sum 0.123
themis_acceleration_CUDA_l2_distance_duration_seconds_count 100
themis_acceleration_CUDA_l2_distance_duration_seconds_bucket{le="0.001"} 10
themis_acceleration_CUDA_l2_distance_duration_seconds_bucket{le="0.01"} 50
themis_acceleration_CUDA_l2_distance_duration_seconds_bucket{le="0.1"} 90
themis_acceleration_CUDA_l2_distance_duration_seconds_bucket{le="+Inf"} 100
```

### JSON Format

```cpp
std::string metrics = MetricsCollector::instance().exportJSON();
std::cout << metrics << std::endl;
```

**Output:**
```json
{
  "counters": {
    "themis_acceleration_CUDA_init_success_total": 1,
    "themis_acceleration_CUDA_l2_distance_operations_total": 100
  },
  "gauges": {
    "themis_acceleration_CUDA_device_memory_used_bytes": 536870912,
    "themis_acceleration_CUDA_device_count": 2
  },
  "histograms": {
    "themis_acceleration_CUDA_l2_distance_duration_seconds": {
      "count": 100,
      "sum": 0.123,
      "mean": 0.00123
    }
  }
}
```

---

## Performance

### Overhead Measurements

| Operation | Time | Overhead |
|-----------|------|----------|
| Counter increment | ~10-20 ns | <0.001% |
| Gauge set | ~30-50 ns | <0.001% |
| Histogram observe | ~100-200 ns | <0.01% |
| Timer (RAII) | ~50-100 ns | <0.01% |

**Benchmark:** Intel Core i7-9700K @ 3.60GHz

### Design Principles

1. **Lock-free counters**: Atomic operations for zero contention
2. **Minimal locking**: Gauges/histograms use fine-grained locks
3. **Zero allocation**: No heap allocations in hot paths
4. **Header-only**: Inline for optimization
5. **Thread-safe**: All operations are thread-safe

---

## Integration

### Prometheus Integration

1. **Configure Prometheus** to scrape metrics endpoint:
```yaml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
    scrape_interval: 15s
```

2. **Expose metrics** in your HTTP server:
```cpp
// In HTTP handler
if (request.path == "/metrics") {
    response.body = MetricsCollector::instance().exportPrometheus();
    response.content_type = "text/plain; version=0.0.4";
}
```

### Grafana Dashboards

See `grafana/dashboards/acceleration_performance.json` for pre-built dashboards.

**Key visualizations:**
- Operations per second (rate)
- Latency percentiles (p50, p95, p99)
- Error rates
- Memory usage
- Device utilization

---

## Best Practices

### DO ✅

1. **Use BackendMetrics** for standard metrics
2. **Use Timer** for duration measurement (RAII)
3. **Name metrics consistently**: `{namespace}_{component}_{metric}_{unit}`
4. **Add descriptions**: Clear, concise metric descriptions
5. **Choose appropriate type**: Counter for cumulative, Gauge for current
6. **Use appropriate buckets**: Histogram buckets should span expected range

### DON'T ❌

1. **Don't create metrics in hot paths** - Register once, use many times
2. **Don't use string operations** in metric recording
3. **Don't create too many metrics** - Limit cardinality
4. **Don't forget units** - Always specify units in name (bytes, seconds)
5. **Don't log metrics** - Metrics are not logs

---

## Troubleshooting

### High Memory Usage

**Symptom:** MetricsCollector consuming excessive memory

**Causes:**
- Too many unique metrics
- Histogram buckets too fine-grained
- Metrics not being cleaned up

**Solutions:**
1. Review metric cardinality
2. Use coarser histogram buckets
3. Call `MetricsCollector::clear()` to reset

### Missing Metrics

**Symptom:** Metrics not appearing in Prometheus

**Checks:**
1. Verify metric is registered: `getCounter(name) != nullptr`
2. Check metric name for typos
3. Verify Prometheus scraping configuration
4. Check HTTP endpoint returns metrics

### Performance Impact

**Symptom:** Metrics causing slowdown

**Analysis:**
1. Benchmark with metrics disabled
2. Check for excessive histogram observations
3. Profile to find hot spots

**Solutions:**
1. Use sampling for high-frequency operations
2. Increase observation interval
3. Use counters instead of histograms where possible

---

## API Reference

### MetricsCollector

```cpp
class MetricsCollector {
public:
    static MetricsCollector& instance();
    
    Counter* registerCounter(const std::string& name, const std::string& desc);
    Gauge* registerGauge(const std::string& name, const std::string& desc);
    Histogram* registerHistogram(const std::string& name, const std::string& desc,
                                 const std::vector<double>& buckets = {});
    
    Counter* getCounter(const std::string& name);
    Gauge* getGauge(const std::string& name);
    Histogram* getHistogram(const std::string& name);
    
    std::string exportPrometheus() const;
    std::string exportJSON() const;
    
    void reset();  // Reset counters to zero
    void clear();  // Remove all metrics
};
```

### BackendMetrics

```cpp
class BackendMetrics {
public:
    explicit BackendMetrics(const std::string& backend_name);
    
    // Initialization
    void recordInitSuccess();
    void recordInitFailure();
    void recordInitDuration(double seconds);
    
    // Operations
    void recordL2DistanceOperation(double duration, size_t vectors);
    void recordCosineOperation(double duration, size_t vectors);
    
    // Resources
    void setDeviceMemoryUsed(double bytes);
    void setDeviceMemoryAvailable(double bytes);
    void setQueueDepth(double depth);
    
    // Errors
    void recordError(const std::string& error_code);
    void recordKernelLaunchFailure();
    void recordMemoryAllocationFailure();
    
    // Devices
    void setDeviceCount(int count);
    void setActiveDeviceIndex(int index);
};
```

---

## Future Enhancements

### Planned (Phase 3.2)
- [ ] Metric labels/tags for multi-dimensional metrics
- [ ] Exemplars for traces-metrics linking
- [ ] Summary metric type (quantiles)
- [ ] Push gateway support

### Under Consideration
- [ ] Metric sampling for high-cardinality
- [ ] Distributed tracing integration (OpenTelemetry)
- [ ] Metric federation
- [ ] Custom exporters (StatsD, InfluxDB)

---

## References

- [Prometheus Exposition Format](https://prometheus.io/docs/instrumenting/exposition_formats/)
- [Metric Types](https://prometheus.io/docs/concepts/metric_types/)
- [Best Practices](https://prometheus.io/docs/practices/naming/)

---

**Authors:** ThemisDB Team  
**License:** See LICENSE file  
**Support:** See SUPPORT.md