# Observability Module Headers

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ../src/observability/README.md · ../src/observability/ARCHITECTURE.md -->

Public interfaces and declarations for ThemisDB observability system.

## Table of Contents

1. [Overview](#overview)
2. [Header Files](#header-files)
3. [Data Structures](#data-structures)
4. [API Reference](#api-reference)
5. [Integration Guide](#integration-guide)
6. [Thread Safety](#thread-safety)
7. [Performance Considerations](#performance-considerations)

## Overview

This directory contains the public API headers for ThemisDB's observability system. These headers define interfaces for metrics collection, query profiling, storage profiling, performance analysis, and alert management.

### Key Components

- **MetricsCollector**: Centralized Prometheus-compatible metrics aggregation
- **QueryProfiler**: Query execution profiling with explain plans
- **StorageProfiler**: RocksDB operation and statistics profiling
- **PerformanceAnalyzer**: Automated issue detection and recommendations
- **Alertmanager**: Integration with Prometheus Alertmanager

## Header Files

### metrics_collector.h

**Purpose:** Central metrics collection and Prometheus export

**Key Classes:**
- `MetricsCollector`: Singleton metrics collector
- `LatencyTracker`: RAII helper for automatic latency tracking

**Features:**
- Thread-safe metric operations
- Counter, gauge, and histogram support
- Prometheus text format export
- Metrics from all subsystems (TSStore, Query, Cache, Shard, Security)
- RAII latency tracking

**Example:**
```cpp
#include "observability/metrics_collector.h"

using namespace themis::observability;

// Record metrics
MetricsCollector::getInstance().recordTSStoreWrite("cpu_usage", 100, 5.2);
MetricsCollector::getInstance().recordQuery("select", 15.5, 1000);
MetricsCollector::getInstance().recordCacheHit("query");

// Get Prometheus output
std::string metrics = MetricsCollector::getInstance().getPrometheusMetrics();

// RAII latency tracking
{
    LatencyTracker tracker("query_execution", {{"type", "select"}});
    // ... perform operation ...
} // latency automatically recorded
```

**Metric Categories:**

1. **TSStore Metrics:**
   - `recordTSStoreWrite()`: Time-series write operations
   - `recordTSStoreQuery()`: Time-series queries
   - `recordTSStoreAggregate()`: Aggregation operations
   - `recordTSStoreCompression()`: Compression ratios

2. **Query Engine Metrics:**
   - `recordQuery()`: Query execution
   - `recordIndexScan()`: Index scan operations
   - `recordFullScan()`: Full table scans

3. **Cache Metrics:**
   - `recordCacheHit()`: Cache hits
   - `recordCacheMiss()`: Cache misses
   - `recordCacheEviction()`: Eviction events

4. **Sharding Metrics:**
   - `recordShardRequest()`: Per-shard operations
   - `recordShardLatency()`: Shard-level latency
   - `recordRebalanceProgress()`: Rebalancing operations

5. **Security Metrics:**
   - `recordAuthAttempt()`: Authentication attempts
   - `recordPolicyEvaluation()`: Authorization decisions
   - `recordEncryptionOperation()`: Cryptographic operations

6. **System Metrics:**
   - `recordMemoryUsage()`: Memory consumption
   - `recordCPUUsage()`: CPU utilization
   - `recordDiskIOps()`: Disk I/O operations

**Thread Safety:** All methods are thread-safe (mutex-protected map operations, atomic counters)

---

### query_profiler.h

**Purpose:** Query execution profiling and performance analysis

**Key Classes:**
- `QueryProfiler`: Main profiler class
- `QueryProfile`: Profile result with timing and statistics
- `OperatorStats`: Per-operator statistics
- `ScopedQueryProfile`: RAII query profiling helper
- `ScopedOperatorProfile`: RAII operator profiling helper

**Key Enums:**
- `QueryPhase`: PARSE, VALIDATE, OPTIMIZE, PLAN, EXECUTE, FETCH_RESULTS
- `OperatorType`: SCAN, INDEX_SCAN, FILTER, PROJECT, AGGREGATE, JOIN, SORT, LIMIT, VECTOR_SEARCH, GRAPH_TRAVERSE

**Features:**
- Phase-level timing (parse → validate → optimize → plan → execute → fetch)
- Operator-level statistics (rows, bytes, I/O, cache)
- Resource usage tracking (memory, disk, network)
- Index and cache utilization tracking
- Optimization hints and warnings
- JSON export for analysis
- Slow query detection

**Example:**
```cpp
#include "observability/query_profiler.h"

using namespace themis::observability;

// Configure profiler
QueryProfilerConfig config;
config.slow_query_threshold = std::chrono::milliseconds(1000);
config.max_profiles_retained = 1000;

QueryProfiler profiler(config);

// Profile a query
{
    ScopedQueryProfile profile(profiler, "q-123", "SELECT * FROM metrics WHERE ...");

    // Execute query (profiler tracks timing)
    executeQuery();

    // Add context
    profile.add_hint("Consider adding index on timestamp");
    profile.add_warning("Full table scan detected");
}

// Analyze results
auto slow_queries = profiler.get_slow_queries(std::chrono::milliseconds(500));
for (const auto& profile : slow_queries) {
    std::cout << profile->toSummary() << std::endl;
}

// Export to JSON
profiler.export_to_json("/var/log/themisdb/profiles.json");
```

**Configuration Options:**
```cpp
struct QueryProfilerConfig {
    bool enabled = true;
    bool profile_all_queries = false;
    bool collect_operator_stats = true;
    bool collect_memory_stats = true;
    bool collect_io_stats = true;
    size_t max_profiles_retained = 1000;
    std::chrono::seconds retention_duration{3600};
    bool log_slow_queries = true;
    std::chrono::milliseconds slow_query_threshold{1000};
};
```

**Thread Safety:** Not thread-safe (use separate instance per thread or external synchronization)

---

### storage_profiler.h

**Purpose:** RocksDB storage layer profiling

**Key Classes:**
- `StorageProfiler`: Main profiler class
- `StorageOpStats`: Operation statistics
- `RocksDBStats`: RocksDB internal statistics
- `ScopedStorageOp`: RAII storage operation profiling helper

**Key Enums:**
- `StorageOpType`: GET, PUT, DELETE, SCAN, BATCH_WRITE, COMPACT, FLUSH, ITERATOR_SEEK, ITERATOR_NEXT

**Features:**
- Operation-level profiling (GET, PUT, DELETE, SCAN, etc.)
- RocksDB statistics collection (compaction, amplification, cache)
- Write/read/space amplification tracking
- Block cache and bloom filter effectiveness
- SST file statistics per level
- WAL and memtable metrics
- Slow operation detection

**Example:**
```cpp
#include "observability/storage_profiler.h"

using namespace themis::observability;

// Configure profiler
StorageProfilerConfig config;
config.slow_op_threshold = std::chrono::milliseconds(100);
config.stats_collection_interval = std::chrono::seconds(60);

StorageProfiler profiler(config);

// Profile storage operation
{
    ScopedStorageOp op(profiler, StorageOpType::GET, "default");

    // Perform operation
    std::string value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);

    // Record details
    op.record_bytes_read(value.size());
    op.set_cache_hit(true);
}

// Collect RocksDB statistics
RocksDBStats stats = profiler.collect_rocksdb_stats("/var/lib/themisdb/data");

// Check amplification
json amp_metrics = profiler.get_amplification_metrics();
double write_amp = amp_metrics["write_amplification"].get<double>();
if (write_amp > 10.0) {
    std::cout << "High write amplification: " << write_amp << std::endl;
}

// Export statistics
profiler.export_to_json("/var/log/themisdb/storage_stats.json");
```

**RocksDB Statistics:**
```cpp
struct RocksDBStats {
    // Compaction stats
    size_t num_compactions;
    size_t compaction_bytes_read;
    size_t compaction_bytes_written;
    std::chrono::microseconds total_compaction_time;

    // Write stats
    size_t num_writes;
    size_t bytes_written;
    size_t wal_bytes;

    // Read stats
    size_t num_reads;
    size_t bytes_read;
    size_t block_cache_hits;
    size_t block_cache_misses;

    // Performance metrics
    double write_amplification;
    double read_amplification;
    double space_amplification;

    json toJSON() const;
};
```

**Thread Safety:** Not thread-safe (use separate instance per thread or external synchronization)

---

### performance_analyzer.h

**Purpose:** Automated performance issue detection and optimization recommendations

**Key Classes:**
- `PerformanceAnalyzer`: Main analyzer class
- `PerformanceAnalysis`: Analysis result
- `PerformanceIssue`: Detected issue with recommendations

**Key Enums:**
- `IssueSeverity`: INFO, WARNING, CRITICAL
- `IssueCategory`: QUERY_OPTIMIZATION, INDEX_USAGE, CACHE_EFFICIENCY, STORAGE_AMPLIFICATION, RESOURCE_USAGE, SLOW_OPERATIONS

**Features:**
- Automated issue detection
- Slow query analysis
- Index usage analysis
- Cache efficiency evaluation
- Storage amplification checks
- Resource usage monitoring
- Optimization recommendations
- HTML and JSON report generation

**Example:**
```cpp
#include "observability/performance_analyzer.h"

using namespace themis::observability;

// Configure analyzer
PerformanceAnalyzerConfig config;
config.slow_query_threshold = std::chrono::milliseconds(1000);
config.cache_hit_rate_threshold = 80.0;
config.write_amplification_threshold = 10.0;

PerformanceAnalyzer analyzer(config);

// Run comprehensive analysis
PerformanceAnalysis analysis = analyzer.analyze(query_profiler, storage_profiler);

// Review issues
for (const auto& issue : analysis.issues) {
    std::cout << "[" << to_string(issue.severity) << "] " << issue.title << std::endl;
    std::cout << "  " << issue.description << std::endl;

    for (const auto& rec : issue.recommendations) {
        std::cout << "  → " << rec << std::endl;
    }
}

// Export reports
analyzer.export_analysis(analysis, "/var/log/themisdb/perf_analysis.json");
analyzer.export_html_report(analysis, "/var/www/themisdb/performance.html");
```

**Configuration Options:**
```cpp
struct PerformanceAnalyzerConfig {
    std::chrono::milliseconds slow_query_threshold{1000};
    std::chrono::milliseconds slow_storage_op_threshold{100};
    double cache_hit_rate_threshold = 80.0;  // %
    double index_usage_threshold = 50.0;     // %
    double write_amplification_threshold = 10.0;
    double read_amplification_threshold = 5.0;
    size_t max_full_scan_threshold = 1000;   // rows

    bool analyze_queries = true;
    bool analyze_storage = true;
    bool analyze_cache = true;
    bool analyze_indexes = true;
    bool generate_recommendations = true;
};
```

**Thread Safety:** Not thread-safe (use separate instance per thread)

---

### alertmanager.h

**Purpose:** Integration with Prometheus Alertmanager

**Key Classes:**
- `Alertmanager`: Base interface
- `DefaultAlertmanager`: Stub implementation
- `Alert`: Alert structure
- `AlertmanagerConfig`: Configuration

**Key Enums:**
- `AlertSeverity`: INFO, WARNING, ERROR, CRITICAL
- `AlertStatus`: FIRING, RESOLVED, SILENCED

**Features:**
- Send alerts to Alertmanager
- Resolve alerts
- Silence alerts
- Get active alerts
- Test connectivity
- Alert routing and receivers

**Example:**
```cpp
#include "observability/alertmanager.h"

using namespace themis::observability;

// Configure alertmanager
AlertmanagerConfig config;
config.endpoint_url = "http://alertmanager:9093/api/v2/alerts";
config.enabled = true;
config.receivers = {"pagerduty", "slack", "email"};

DefaultAlertmanager alertmanager(config);
alertmanager.initialize(config);

// Send alert
Alert alert;
alert.alert_name = "HighQueryLatency";
alert.severity = AlertSeverity::WARNING;
alert.message = "Average query latency exceeded 1000ms";
alert.labels = {
    {"component", "query_engine"},
    {"instance", "themisdb-node-1"}
};
alert.annotations = {
    {"summary", "Query latency spike"},
    {"runbook_url", "https://docs.example.com/runbooks/high-latency"}
};

auto result = alertmanager.sendAlert(alert);
if (result.isOk()) {
    std::cout << "Alert sent" << std::endl;
}

// Resolve alert
alertmanager.resolveAlert("HighQueryLatency");
```

**Alert Structure:**
```cpp
struct Alert {
    std::string alert_name;
    std::string alert_id;
    AlertSeverity severity;
    AlertStatus status;
    std::string message;
    std::map<std::string, std::string> labels;
    std::map<std::string, std::string> annotations;
    std::chrono::system_clock::time_point fired_at;
    std::chrono::system_clock::time_point resolved_at;
};
```

**Thread Safety:** Yes (implementation-dependent)

---

### continuous_profiler.h

**Purpose:** Always-on, low-overhead CPU profiling with pprof / async-profiler compatible output

**Key Classes:**
- `ContinuousProfiler`: Background sampling profiler (pImpl pattern)
- `ProfileSnapshot`: A captured profile in pprof folded-stacks text format
- `ProfileDiff`: Differential comparison result between two snapshots
- `ContinuousProfilerConfig`: Runtime configuration

**Key Enums:**
- `ProfileType`: CPU, HEAP, MUTEX, BLOCK

**Features:**
- Background sampling thread (configurable overhead, default 1%)
- pprof folded-stacks text format output (compatible with `go tool pprof`)
- async-profiler `-o collapsed` format interoperability
- Snapshot persistence to `.folded` files
- Differential hotspot comparison (new / removed / changed frames)
- CPU regression detection with anomaly callback
- Dynamic enable/disable without restart
- Thread-safe public API

**Example:**
```cpp
#include "observability/continuous_profiler.h"

using namespace themis::observability;

// Configure and start
ContinuousProfilerConfig cfg;
cfg.enabled = true;
cfg.cpu_sample_rate = 0.01;          // ~1% CPU overhead
cfg.snapshot_interval = std::chrono::seconds(60);
cfg.output_dir = "/var/lib/themisdb/profiles";

ContinuousProfiler profiler(cfg);
profiler.start();

// ... run workload ...

// Capture snapshot and persist
auto snap = profiler.snapshot(ProfileType::CPU);
snap.saveToFile("/tmp/profile.folded");

// Compare with a baseline
auto diff = profiler.compare(baseline_snap, snap);
if (diff.cpu_regression_percent > 10.0) {
    // handle regression
}

profiler.stop();
```

**Thread Safety:** All public methods are thread-safe

---

## Data Structures

### Common Structures

#### QueryProfile
```cpp
struct QueryProfile {
    std::string query_id;
    std::string query_text;
    std::chrono::system_clock::time_point start_time;
    std::chrono::microseconds total_duration;

    std::unordered_map<QueryPhase, std::chrono::microseconds> phase_timings;
    std::vector<OperatorStats> operator_stats;

    size_t peak_memory_bytes;
    size_t total_disk_io_bytes;
    size_t total_network_bytes;

    bool used_index;
    bool used_cache;
    std::vector<std::string> indexes_used;
    std::vector<std::string> warnings;
    std::vector<std::string> optimization_hints;

    size_t result_rows;
    size_t result_bytes;

    json toJSON() const;
    std::string toSummary() const;
};
```

#### OperatorStats
```cpp
struct OperatorStats {
    OperatorType type;
    std::string name;
    std::chrono::microseconds duration;
    size_t rows_processed;
    size_t bytes_processed;
    size_t disk_reads;
    size_t cache_hits;
    size_t cache_misses;
    std::string details;

    json toJSON() const;
};
```

#### RocksDBStats
```cpp
struct RocksDBStats {
    std::chrono::system_clock::time_point timestamp;

    // Compaction, write, read stats
    size_t num_compactions;
    size_t num_writes;
    size_t num_reads;
    size_t bytes_written;
    size_t bytes_read;

    // Cache stats
    size_t block_cache_hits;
    size_t block_cache_misses;
    size_t bloom_filter_hits;

    // Amplification metrics
    double write_amplification;
    double read_amplification;
    double space_amplification;

    json toJSON() const;
};
```

#### PerformanceIssue
```cpp
struct PerformanceIssue {
    IssueSeverity severity;
    IssueCategory category;
    std::string title;
    std::string description;
    std::vector<std::string> recommendations;
    json metrics;

    json toJSON() const;
};
```

## API Reference

### MetricsCollector API

**Singleton Access:**
```cpp
static MetricsCollector& getInstance();
```

**Recording Methods:**
```cpp
void recordTSStoreWrite(const std::string& metric, size_t batch_size, double latency_ms);
void recordQuery(const std::string& query_type, double latency_ms, size_t result_count);
void recordCacheHit(const std::string& cache_type);
void recordShardLatency(const std::string& shard_id, double latency_ms);
void recordAuthAttempt(bool success);
```

**Export:**
```cpp
std::string getPrometheusMetrics() const;
```

### QueryProfiler API

**Lifecycle:**
```cpp
std::string start_query(const std::string& query_id, const std::string& query_text);
void end_query(const std::string& query_id);
```

**Recording:**
```cpp
void record_phase(const std::string& query_id, QueryPhase phase, std::chrono::microseconds duration);
void record_operator(const std::string& query_id, const OperatorStats& stats);
void record_index_usage(const std::string& query_id, const std::string& index_name);
void add_hint(const std::string& query_id, const std::string& hint);
```

**Retrieval:**
```cpp
std::shared_ptr<QueryProfile> get_profile(const std::string& query_id) const;
std::vector<std::shared_ptr<QueryProfile>> get_slow_queries(std::chrono::milliseconds threshold) const;
std::vector<std::shared_ptr<QueryProfile>> get_top_queries(size_t limit = 10) const;
```

### StorageProfiler API

**Recording:**
```cpp
void record_operation(const StorageOpStats& stats);
RocksDBStats collect_rocksdb_stats(const std::string& db_path);
```

**Retrieval:**
```cpp
std::vector<StorageOpStats> get_operations(std::optional<StorageOpType> type = std::nullopt) const;
std::vector<StorageOpStats> get_slow_operations(std::chrono::milliseconds threshold) const;
json get_amplification_metrics() const;
json get_cache_metrics() const;
```

### PerformanceAnalyzer API

**Analysis:**
```cpp
PerformanceAnalysis analyze(const QueryProfiler& query_profiler, const StorageProfiler& storage_profiler);
std::vector<PerformanceIssue> analyze_queries(const QueryProfiler& query_profiler);
std::vector<PerformanceIssue> analyze_storage(const StorageProfiler& storage_profiler);
```

**Export:**
```cpp
void export_analysis(const PerformanceAnalysis& analysis, const std::string& filename) const;
void export_html_report(const PerformanceAnalysis& analysis, const std::string& filename) const;
```

### Alertmanager API

**Initialization:**
```cpp
Result<void> initialize(const AlertmanagerConfig& config);
```

**Alert Management:**
```cpp
Result<void> sendAlert(const Alert& alert);
Result<void> resolveAlert(const std::string& alert_id);
Result<void> silenceAlert(const std::string& alert_id, int duration_minutes);
std::vector<Alert> getActiveAlerts();
```

## Integration Guide

### Integration with Core Module

**ILogger Integration:**
```cpp
#include "core/concerns/i_logger.h"
#include "observability/metrics_collector.h"

void logSlowQuery(ILogger& logger, const QueryProfile& profile) {
    if (profile.total_duration.count() > 1000000) {  // 1 second
        logger.warn("Slow query: " + profile.query_text);
        MetricsCollector::getInstance().recordQuery("slow_query",
            profile.total_duration.count() / 1000.0,
            profile.result_rows);
    }
}
```

**ITracer Integration:**
```cpp
#include "core/concerns/i_tracer.h"
#include "observability/query_profiler.h"

void executeTracedQuery(ITracer& tracer, QueryProfiler& profiler,
                       const std::string& query_text) {
    auto span = tracer.startSpan("query_execution");
    profiler.start_query("q-123", query_text);

    try {
        // Execute query
        span->setStatus(true);
    } catch (const std::exception& e) {
        span->recordError(e.what());
        span->setStatus(false);
    }

    profiler.end_query("q-123");
}
```

**IMetrics Integration:**
```cpp
#include "core/concerns/i_metrics.h"
#include "observability/metrics_collector.h"

class MetricsAdapter : public IMetrics {
public:
    void incrementCounter(const std::string& name, int64_t value, const Labels& labels) override {
        MetricsCollector::getInstance().incrementCounter(name, labels);
    }

    std::string exportMetrics() const override {
        return MetricsCollector::getInstance().getPrometheusMetrics();
    }
};
```

### Integration with HTTP Server

```cpp
// Expose /metrics endpoint
http_server.addRoute("/metrics", [](const Request& req) -> Response {
    auto metrics = MetricsCollector::getInstance().getPrometheusMetrics();
    return Response{
        .status = 200,
        .content_type = "text/plain; version=0.0.4",
        .body = metrics
    };
});

// Expose /query_profiles endpoint
http_server.addRoute("/query_profiles", [&profiler](const Request& req) -> Response {
    auto profiles = profiler.get_all_profiles();
    json j = json::array();
    for (const auto& profile : profiles) {
        j.push_back(profile->toJSON());
    }
    return Response{
        .status = 200,
        .content_type = "application/json",
        .body = j.dump()
    };
});
```

## Thread Safety

### Thread-Safe Components

**MetricsCollector:**
- ✅ All public methods are thread-safe
- ✅ Uses mutex for map operations
- ✅ Uses atomic operations for counters/gauges
- ✅ Safe for concurrent access

**LatencyTracker:**
- ✅ Thread-safe (each instance independent)
- ✅ Safe to use in parallel threads

### Not Thread-Safe Components

**QueryProfiler:**
- ❌ Not thread-safe
- ⚠️ Use separate instance per thread OR external synchronization

**StorageProfiler:**
- ❌ Not thread-safe
- ⚠️ Use separate instance per thread OR external synchronization

**PerformanceAnalyzer:**
- ❌ Not thread-safe
- ⚠️ Use separate instance per thread

**Alertmanager:**
- ⚠️ Implementation-dependent (DefaultAlertmanager is thread-safe)

## Performance Considerations

### Overhead

**MetricsCollector:**
- Counter increment: ~50ns (atomic operation)
- Gauge set: ~50ns (atomic operation)
- Histogram observe: ~500ns (mutex + vector append)
- Prometheus export: ~1ms (serialize all metrics)

**QueryProfiler:**
- Start/end query: ~1μs
- Record phase: ~100ns
- Record operator: ~500ns
- Total overhead: <1% for typical queries

**StorageProfiler:**
- Record operation: ~500ns
- Collect RocksDB stats: ~10ms (read from RocksDB)

### Best Practices

1. **Use RAII Helpers:**
   ```cpp
   // Preferred (automatic)
   {
       LatencyTracker tracker("operation");
       doWork();
   }

   // Avoid (manual)
   auto start = std::chrono::steady_clock::now();
   doWork();
   auto end = std::chrono::steady_clock::now();
   metrics.record(/* ... */);
   ```

2. **Batch Operations:**
   ```cpp
   // Avoid high-frequency metrics
   for (int i = 0; i < 1000000; i++) {
       metrics.incrementCounter("loop_iterations");  // BAD: 1M metric calls
   }

   // Prefer batching
   int count = 0;
   for (int i = 0; i < 1000000; i++) {
       count++;
   }
   metrics.incrementCounter("loop_iterations", count);  // GOOD: 1 metric call
   ```

3. **Conditional Profiling:**
   ```cpp
   if (profiler.is_enabled() && should_profile) {
       profiler.start_query(query_id, query_text);
   }
   ```

4. **Limit Label Cardinality:**
   ```cpp
   // BAD: High cardinality
   metrics.record("query_latency", latency, {{"user_id", user_id}});  // Millions of users

   // GOOD: Low cardinality
   metrics.record("query_latency", latency, {{"user_tier", "premium"}});  // Few tiers
   ```

## See Also

- [../src/observability/README.md](../../../src/observability/README.md) - Implementation documentation
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned features
- [../../core/concerns/](../../core/concerns/) - Core interfaces (ILogger, ITracer, IMetrics)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
