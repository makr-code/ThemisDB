---
name: "📊 Plugin Metrics & Monitoring"
about: Add instrumentation and metrics for plugin operations
title: "[Plugin] Implement Plugin Metrics and Monitoring"
labels: 
  - type:feature
  - area:plugins
  - area:monitoring
  - priority:P3
  - effort:medium
assignees: ''

---

## 📋 Problem / Motivation

The current plugin system has no instrumentation or metrics:

**Missing:**
- ❌ Load time tracking
- ❌ Reload count
- ❌ Error tracking
- ❌ Function call metrics
- ❌ Resource usage (memory, threads)
- ❌ Performance statistics (latency percentiles)
- ❌ OpenTelemetry integration

**Impact:**
- No visibility into plugin performance
- Difficult to diagnose plugin issues
- No data for capacity planning

## 🎯 Proposed Solution

Implement comprehensive plugin metrics with OpenTelemetry integration:

### Metrics to Track

1. **Timing Metrics**
   - Plugin load time
   - Plugin reload time
   - Last reload timestamp
   - Uptime since load

2. **Count Metrics**
   - Reload count
   - Function calls
   - Error count
   - Success/failure counts

3. **Resource Metrics**
   - Memory usage per plugin
   - Thread count
   - File handles

4. **Performance Metrics**
   - Average call latency
   - P95 call latency
   - P99 call latency
   - Throughput (calls/sec)

## 📝 Implementation Details

### PluginMetrics Class

```cpp
class PluginMetrics {
public:
    struct PluginStats {
        // Timing
        std::chrono::milliseconds load_time{0};
        std::chrono::milliseconds last_reload_time{0};
        std::chrono::system_clock::time_point loaded_at;
        
        // Counts
        uint64_t reload_count = 0;
        uint64_t function_calls = 0;
        uint64_t errors = 0;
        
        // Resource usage
        size_t memory_bytes = 0;
        
        // Performance
        double avg_call_latency_ms = 0.0;
        double p95_call_latency_ms = 0.0;
        double p99_call_latency_ms = 0.0;
    };
    
    void recordLoad(const std::string& plugin, std::chrono::milliseconds duration);
    void recordReload(const std::string& plugin, std::chrono::milliseconds duration);
    void recordCall(const std::string& plugin, std::chrono::microseconds latency);
    void recordError(const std::string& plugin);
    
    const PluginStats& getStats(const std::string& plugin) const;
    std::map<std::string, PluginStats> getAllStats() const;
    
private:
    std::map<std::string, PluginStats> stats_;
    mutable std::mutex mutex_;
};
```

### OpenTelemetry Integration

```cpp
// Prometheus metrics
Counter plugin_loads_total;
Counter plugin_reloads_total;
Counter plugin_errors_total;
Histogram plugin_load_duration_seconds;
Histogram plugin_call_duration_seconds;
Gauge plugin_memory_bytes;
```

### Integration with PluginManager

```cpp
class PluginManager {
private:
    PluginMetrics metrics_;
    
public:
    IThemisPlugin* loadPlugin(const std::string& name) {
        auto start = std::chrono::steady_clock::now();
        
        // ... existing load logic ...
        
        auto duration = std::chrono::steady_clock::now() - start;
        metrics_.recordLoad(name, duration);
        
        return plugin;
    }
    
    const PluginMetrics& getMetrics() const { return metrics_; }
};
```

See detailed implementation in: `docs/de/plugins/PLUGIN_SYSTEM_CONSISTENCY_ANALYSIS.md` (lines 761-850)

## ✅ Acceptance Criteria

- [ ] `PluginMetrics` class implemented
- [ ] All timing metrics tracked
- [ ] All count metrics tracked
- [ ] Resource usage tracking (memory)
- [ ] Performance percentiles (P95, P99)
- [ ] OpenTelemetry integration
- [ ] Prometheus exporter
- [ ] Metrics API endpoint (`/api/plugins/metrics`)
- [ ] Unit tests for metrics collection
- [ ] Integration tests with real plugins
- [ ] Grafana dashboard configuration
- [ ] Documentation with examples

## 🔗 Related

- Documentation: `docs/de/plugins/PLUGIN_SYSTEM_CONSISTENCY_ANALYSIS.md`
- Related: Plugin monitoring API endpoint
- Related: Grafana dashboard

## 📊 Impact

**Benefits:**
- Visibility into plugin performance
- Early detection of performance degradation
- Data-driven capacity planning
- Better debugging of plugin issues

**Risks:**
- Small performance overhead from instrumentation (mitigated by efficient collection)
- Memory overhead for metrics storage (mitigated by retention policies)

## 🧪 Testing Strategy

1. **Unit Tests:**
   - Test metrics collection
   - Test thread safety
   - Test metric reset

2. **Integration Tests:**
   - Load plugin and verify load time recorded
   - Call plugin functions and verify latency tracking
   - Trigger error and verify error count

3. **Performance Tests:**
   - Measure instrumentation overhead (should be <1%)
   - Test metrics collection under load

## 📚 Additional Context

This feature was identified during the plugin system consistency analysis (2026-01-20).

**Priority Justification:** P3 (Medium) - Important for observability but not critical for functionality.

**Effort Estimate:** Medium (1-3 days) - Metrics collection is straightforward, OpenTelemetry integration adds complexity.

**Prometheus Metrics Example:**
```
# HELP themis_plugin_loads_total Total number of plugin loads
# TYPE themis_plugin_loads_total counter
themis_plugin_loads_total{plugin="onnx_clip"} 1

# HELP themis_plugin_load_duration_seconds Plugin load duration
# TYPE themis_plugin_load_duration_seconds histogram
themis_plugin_load_duration_seconds_bucket{plugin="onnx_clip",le="0.1"} 0
themis_plugin_load_duration_seconds_bucket{plugin="onnx_clip",le="0.5"} 1
themis_plugin_load_duration_seconds_sum{plugin="onnx_clip"} 0.45
themis_plugin_load_duration_seconds_count{plugin="onnx_clip"} 1
```

**API Endpoint Example:**
```bash
curl http://localhost:8765/api/plugins/metrics
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
  }
}
```
