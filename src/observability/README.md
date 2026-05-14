> **Build:** `cmake --preset release && cmake --build build/release --target <target>`

# Observability Module

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/observability/README.md -->
<!-- Primärdokumentation: src/observability/ -->

Comprehensive monitoring, tracing, and performance analysis implementation for ThemisDB with Prometheus and Grafana integration.

## Module Purpose

Provides the metrics, distributed tracing, and structured logging infrastructure for ThemisDB, enabling production monitoring via Prometheus and OpenTelemetry.

## Subsystem Scope

**In scope:** Prometheus-compatible metrics collection and export, OpenTelemetry distributed tracing, structured logging aggregation, health check endpoints, alerting rules, query/storage profiling, performance analysis, eBPF-based kernel tracing, continuous profiling, distributed flame graph generation.

**Out of scope:** Log storage (external Elasticsearch/Loki), alerting backend (external Alertmanager/PagerDuty), dashboarding (external Grafana).

## Relevant Interfaces

| File | Role |
|---|---|
| `metrics_collector.cpp` | Prometheus metric collection and `/metrics` endpoint |
| `alertmanager.cpp` | Alertmanager integration — alert routing and notification webhooks |
| `alerting_engine.cpp` | Rule-based alerting engine with pluggable notification channels (`AlertingEngine`, `INotificationChannel`, `LogNotificationChannel`, `WebhookNotificationChannel`, `SlackNotificationChannel`) |
| `continuous_profiler.cpp` | Continuous profiling (pprof / async-profiler compatible), adaptive sampling |
| `ebpf_tracer.cpp` | eBPF-based kernel-level performance tracing (Linux perf counters) |
| `distributed_flame_graph.cpp` | Distributed flame graph generation across nodes |
| `query_profiler.cpp` | Per-phase and per-operator query timing with index usage tracking |
| `storage_profiler.cpp` | RocksDB stats, write/read amplification, compaction metrics, cache hit rates |
| `performance_analyzer.cpp` | Automated issue detection with optimization recommendations |
| `tracer.cpp` | Standalone `ObservabilityTracer` — W3C Trace Context propagation, span ring buffer, ContinuousProfiler integration, MetricsCollector gauges |
| `opentelemetry_tracer.cpp` | `OpenTelemetryTracer` — OTLP gRPC/HTTP export, Jaeger/Zipkin adapters, W3C Baggage, `recordException()`, `recordMetrics()` |
| `log_aggregator.cpp` | Standalone `LogAggregator` — structured JSON log collection, trace-context correlation, ring buffer, optional file sink |
| `log_search_engine.cpp` | `LogSearchEngine` — query structured logs by level, time range, field filters, message content |
| `metric_aggregator.cpp` | `MetricAggregator` — rate calculation, histogram aggregation, cross-shard pre-aggregation, cardinality rollup |
| `metric_anomaly_detector.cpp` | `MetricAnomalyDetector` — ML-based anomaly detection bridging `analytics::StreamingAnomalyDetector` with MetricsCollector |
| `ml_anomaly_detector.cpp` | `MLAnomalyDetector` — ARIMA/Holt-Winters forecast-based anomaly detection with model lifecycle management |
| `metrics_stream_server.cpp` | `MetricsStreamServer` — real-time metric streaming via WebSocket/SSE with `SendFn` callback delivery |
| `advanced_metrics.cpp` | `AdvancedMetrics` — Summary, ExponentialHistogram, Cardinality, TimeWeightedAverage, Rate metric types |
| `slo_reporter.cpp` | `SloReporter` — SLO/SLA burn-rate alerting (FAST 14.4×, MEDIUM 6×, SLOW 3× windows) |
| `root_cause_analyzer.cpp` | `RootCauseAnalyzer` — automated root cause analysis via Pearson correlation and Granger-inspired causal graph inference |
| `tenant_metrics_namespace.cpp` | `TenantMetricsNamespace` — per-tenant metric namespacing with independent cardinality budgets and `themis_<tenant_id>_` Prometheus prefix |

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Enterprise-grade observability stack operational. Prometheus metrics, query/storage profiling, continuous profiling, eBPF tracing, distributed flame graph, performance analysis, Alertmanager integration, standalone tracer and log aggregator, OpenTelemetry full integration (`OpenTelemetryTracer` with OTLP gRPC/HTTP export), structured log search, per-tenant metric namespacing, real-time metric streaming, advanced custom metric types, root cause analyzer, and SLO reporter are all fully implemented. The OTLP exemplar interface (`otlp_exemplar.h`) is a planned interface header targeting Q3 2026.

**Validated:** 2026-03-11 (Reality-Check against Sourcecode; see [docs/de/observability/MISSING_IMPLEMENTATIONS.md](../../docs/de/observability/MISSING_IMPLEMENTATIONS.md))

## Table of Contents

1. [Overview](#overview)
2. [Components](#components)
3. [Architecture](#architecture)
4. [Configuration](#configuration-file-themisdbyaml)
5. [Integration with Core](#integration-with-core)
6. [Monitoring Best Practices](#monitoring-best-practices)
7. [Alerting Rules](#alerting-rules)
8. [Debugging Workflows](#debugging-workflows)
9. [Performance Profiling](#performance-profiling)

## Overview

The Observability Module provides enterprise-grade monitoring, tracing, and performance analysis for ThemisDB. It implements comprehensive instrumentation across all subsystems, enabling deep visibility into system behavior, resource utilization, and query performance.

### Key Features

- **Metrics Collection**: Prometheus-compatible metrics for all subsystems
- **Query Profiling**: Detailed execution plans with timing and resource usage
- **Storage Profiling**: RocksDB performance metrics and amplification tracking
- **Performance Analysis**: Automated issue detection with optimization recommendations
- **Alert Management**: Integration with Alertmanager for critical events
- **Distributed Tracing**: Span context propagation (OpenTelemetry compatible)
- **Structured Logging**: Integration with Core ILogger interface
- **Health Checks**: Readiness and liveness probes for Kubernetes
- **Telemetry Aggregation**: Centralized collection across shards

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Observability Module                       │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────┐        ┌──────────────────┐          │
│  │ MetricsCollector │◄───────┤  All Subsystems  │          │
│  │   (Singleton)    │        │  (TSStore, Cache │          │
│  │                  │        │   Query, Shard)  │          │
│  └────────┬─────────┘        └──────────────────┘          │
│           │                                                  │
│           │ /metrics endpoint                               │
│           ▼                                                  │
│  ┌──────────────────┐                                       │
│  │   Prometheus     │                                       │
│  │   Text Format    │                                       │
│  └────────┬─────────┘                                       │
│           │                                                  │
│  ┌────────▼─────────┐        ┌──────────────────┐          │
│  │  QueryProfiler   │        │ StorageProfiler  │          │
│  │  - Phases        │        │  - RocksDB Stats │          │
│  │  - Operators     │        │  - Amplification │          │
│  │  - Index Usage   │        │  - Cache Hits    │          │
│  └────────┬─────────┘        └────────┬─────────┘          │
│           │                            │                     │
│           └────────┬───────────────────┘                     │
│                    │                                         │
│           ┌────────▼─────────┐                              │
│           │ PerformanceAnalyzer│                            │
│           │  - Issue Detection │                            │
│           │  - Recommendations │                            │
│           └────────┬───────────┘                            │
│                    │                                         │
│           ┌────────▼─────────┐                              │
│           │   Alertmanager   │                              │
│           │  - Alert Routing │                              │
│           │  - Notifications │                              │
│           └──────────────────┘                              │
└─────────────────────────────────────────────────────────────┘
         │                           │
         ▼                           ▼
   Grafana Dashboards          PagerDuty/Slack
```

## Components

### MetricsCollector (`metrics_collector.cpp`)

**Purpose:** Central metrics aggregation and Prometheus export

Thread-safe singleton that collects metrics from all ThemisDB subsystems and exposes them in Prometheus text format via `/metrics` endpoint.

**Metrics Categories:**

1. **TSStore Metrics**
   - `tsstore_writes_total{metric="cpu_usage"}`: Write operations count
   - `tsstore_write_latency_ms{metric="cpu_usage"}`: Write latency histogram
   - `tsstore_write_batch_size{metric="cpu_usage"}`: Batch size distribution
   - `tsstore_queries_total{metric="cpu_usage"}`: Query count
   - `tsstore_query_latency_ms{metric="cpu_usage"}`: Query latency
   - `tsstore_compression_ratio{type="gorilla"}`: Compression effectiveness

2. **Query Engine Metrics**
   - `query_total{type="select"}`: Query execution count
   - `query_latency_ms{type="select"}`: Query latency histogram
   - `query_result_rows{type="select"}`: Result set size
   - `index_scans_total{type="btree"}`: Index scan operations
   - `full_scans_total{table="metrics"}`: Full table scans (optimization target)

3. **Cache Metrics**
   - `cache_hits_total{type="query"}`: Cache hit count
   - `cache_misses_total{type="query"}`: Cache miss count
   - `cache_hit_rate{type="query"}`: Hit rate percentage
   - `cache_evictions_total{type="query"}`: Eviction count

4. **Sharding Metrics**
   - `shard_requests_total{shard="shard-1",operation="write"}`: Per-shard operations
   - `shard_latency_ms{shard="shard-1"}`: Per-shard latency
   - `rebalance_progress{operation_id="rb-123"}`: Rebalancing progress

5. **Security Metrics**
   - `auth_attempts_total{result="success"}`: Authentication attempts
   - `policy_evaluations_total{result="allowed"}`: Policy decisions
   - `encryption_operations_total{operation="encrypt"}`: Crypto operations

**Thread Safety:**
- All public methods are thread-safe
- Counter/gauge operations use atomic operations
- Histogram operations are mutex-protected
- Safe for concurrent access from multiple threads

**Example:**
```cpp
#include "observability/metrics_collector.h"

using namespace themis::observability;

// Record a write operation
MetricsCollector::getInstance().recordTSStoreWrite(
    "cpu_usage",
    100,      // batch_size
    5.2       // latency_ms
);

// Get Prometheus metrics
std::string metrics = MetricsCollector::getInstance().getPrometheusMetrics();
// Serve via HTTP /metrics endpoint
```

**RAII Latency Tracking:**
```cpp
// Automatic latency measurement
{
    LatencyTracker tracker("tsstore_query", {{"metric", "memory_usage"}});
    // ... perform query ...
} // latency automatically recorded on scope exit
```

### QueryProfiler (`query_profiler.cpp`)

**Purpose:** Query execution profiling and explain plan generation

Profiles query execution at multiple granularities:
- Phase timing (parse, validate, optimize, plan, execute, fetch)
- Operator statistics (scan, filter, join, aggregate, sort)
- Resource usage (memory, disk I/O, network)
- Index and cache utilization

**Query Phases:**
```
PARSE → VALIDATE → OPTIMIZE → PLAN → EXECUTE → FETCH_RESULTS
  ↓         ↓          ↓        ↓        ↓           ↓
  Syntax   Schema    Rules    Query    Operators   Results
  Check    Check     Apply    Tree     Execute     Serialize
```

**Operator Types:**
- `SCAN`: Full table scan (optimization opportunity)
- `INDEX_SCAN`: Index-based retrieval (efficient)
- `FILTER`: Row filtering (predicate evaluation)
- `PROJECT`: Column selection
- `AGGREGATE`: Group by / aggregation
- `JOIN`: Table joins (expensive)
- `SORT`: Order by operations
- `LIMIT`: Result limiting
- `VECTOR_SEARCH`: Similarity search
- `GRAPH_TRAVERSE`: Graph query operations

**Configuration:**
```cpp
QueryProfilerConfig config;
config.enabled = true;
config.profile_all_queries = false;           // Profile only on-demand
config.collect_operator_stats = true;
config.collect_memory_stats = true;
config.collect_io_stats = true;
config.max_profiles_retained = 1000;          // Keep last 1000 queries
config.retention_duration = std::chrono::seconds(3600);  // 1 hour
config.log_slow_queries = true;
config.slow_query_threshold = std::chrono::milliseconds(1000);  // 1s

QueryProfiler profiler(config);
```

**Usage:**
```cpp
// Start profiling
std::string query_id = profiler.start_query("q-123", "SELECT * FROM metrics WHERE tenant_id = 'acme'");

// Record phases
profiler.record_phase(query_id, QueryPhase::PARSE, std::chrono::microseconds(100));
profiler.record_phase(query_id, QueryPhase::OPTIMIZE, std::chrono::microseconds(500));

// Record operator stats
OperatorStats stats;
stats.type = OperatorType::INDEX_SCAN;
stats.name = "idx_tenant_id";
stats.rows_processed = 1000;
stats.bytes_processed = 50000;
stats.cache_hits = 800;
stats.cache_misses = 200;
profiler.record_operator(query_id, stats);

// End profiling
profiler.end_query(query_id);

// Get profile
auto profile = profiler.get_profile(query_id);
std::cout << profile->toSummary() << std::endl;
```

**RAII Profiling:**
```cpp
{
    ScopedQueryProfile profile(profiler, "q-456", query_text);

    // Automatically tracked
    executeQuery();

    // Add context
    profile.add_hint("Consider adding index on column 'timestamp'");
    profile.add_warning("Full table scan detected");
}
```

**Query Analysis:**
```cpp
// Find slow queries
auto slow_queries = profiler.get_slow_queries(std::chrono::milliseconds(500));
for (const auto& profile : slow_queries) {
    std::cout << "Slow query: " << profile->query_text << std::endl;
    std::cout << "Duration: " << profile->total_duration.count() << "μs" << std::endl;
}

// Get top queries by duration
auto top_queries = profiler.get_top_queries(10);
```

**JSON Export:**
```cpp
// Export profiles for analysis
profiler.export_to_json("/var/log/themisdb/query_profiles.json");

// Example output:
// {
//   "query_id": "q-123",
//   "query_text": "SELECT * FROM metrics WHERE tenant_id = 'acme'",
//   "total_duration_us": 15000,
//   "phase_timings": {
//     "PARSE": 100,
//     "VALIDATE": 50,
//     "OPTIMIZE": 500,
//     "PLAN": 200,
//     "EXECUTE": 14000,
//     "FETCH_RESULTS": 150
//   },
//   "operator_stats": [
//     {
//       "type": "INDEX_SCAN",
//       "name": "idx_tenant_id",
//       "duration_us": 12000,
//       "rows_processed": 1000,
//       "cache_hit_rate": 0.8
//     }
//   ],
//   "used_index": true,
//   "indexes_used": ["idx_tenant_id"],
//   "optimization_hints": ["Consider adding covering index"]
// }
```

### StorageProfiler (`storage_profiler.cpp`)

**Purpose:** RocksDB storage layer profiling and amplification tracking

Monitors RocksDB operations, compaction, I/O patterns, and cache performance to identify storage bottlenecks.

**Metrics Collected:**

1. **Operation Statistics**
   - GET/PUT/DELETE latencies
   - Scan operations and iterator performance
   - Batch write efficiency
   - Compaction/flush operations

2. **RocksDB Statistics**
   - Compaction metrics (bytes read/written, time)
   - Write amplification factor
   - Read amplification factor
   - Space amplification factor
   - Block cache hit/miss rates
   - Bloom filter effectiveness
   - SST file statistics per level

3. **Performance Metrics**
   - Memory usage (memtable, block cache)
   - WAL size and sync operations
   - Level statistics and distribution
   - Disk I/O operations

**Configuration:**
```cpp
StorageProfilerConfig config;
config.enabled = true;
config.collect_op_stats = true;
config.collect_rocksdb_stats = true;
config.max_ops_retained = 10000;
config.stats_collection_interval = std::chrono::seconds(60);
config.retention_duration = std::chrono::seconds(3600);
config.log_slow_ops = true;
config.slow_op_threshold = std::chrono::milliseconds(100);

StorageProfiler profiler(config);
```

**Usage:**
```cpp
// Profile storage operation
{
    ScopedStorageOp op(profiler, StorageOpType::GET, "default");

    // Perform operation
    std::string value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);

    // Record details
    op.record_bytes_read(value.size());
    op.set_cache_hit(status.ok() && value.size() > 0);
}

// Collect RocksDB statistics
RocksDBStats stats = profiler.collect_rocksdb_stats("/var/lib/themisdb/data");

std::cout << "Write Amplification: " << stats.write_amplification << std::endl;
std::cout << "Read Amplification: " << stats.read_amplification << std::endl;
std::cout << "Cache Hit Rate: "
          << (100.0 * stats.block_cache_hits / (stats.block_cache_hits + stats.block_cache_misses))
          << "%" << std::endl;
```

**Amplification Analysis:**
```cpp
// Get amplification metrics
json amp_metrics = profiler.get_amplification_metrics();
// {
//   "write_amplification": 3.2,    // Good: < 10
//   "read_amplification": 2.1,     // Good: < 5
//   "space_amplification": 1.15    // Excellent: < 1.5
// }

// Check for issues
if (amp_metrics["write_amplification"].get<double>() > 10.0) {
    // High write amplification - consider:
    // - Increasing memtable size
    // - Adjusting level size multiplier
    // - Enabling direct I/O
}
```

**Cache Performance:**
```cpp
json cache_metrics = profiler.get_cache_metrics();
// {
//   "block_cache_hit_rate": 0.85,
//   "bloom_filter_effectiveness": 0.92,
//   "memtable_hit_rate": 0.15
// }
```

### PerformanceAnalyzer (`performance_analyzer.cpp`)

**Purpose:** Automated performance issue detection and optimization recommendations

Analyzes query and storage profiles to identify bottlenecks, inefficiencies, and optimization opportunities.

**Issue Categories:**
- `QUERY_OPTIMIZATION`: Inefficient query patterns
- `INDEX_USAGE`: Missing or unused indexes
- `CACHE_EFFICIENCY`: Poor cache hit rates
- `STORAGE_AMPLIFICATION`: High write/read amplification
- `RESOURCE_USAGE`: Memory or I/O bottlenecks
- `SLOW_OPERATIONS`: Operations exceeding thresholds

**Configuration:**
```cpp
PerformanceAnalyzerConfig config;
config.slow_query_threshold = std::chrono::milliseconds(1000);
config.slow_storage_op_threshold = std::chrono::milliseconds(100);
config.cache_hit_rate_threshold = 80.0;  // %
config.index_usage_threshold = 50.0;     // %
config.write_amplification_threshold = 10.0;
config.read_amplification_threshold = 5.0;
config.max_full_scan_threshold = 1000;  // rows
config.analyze_queries = true;
config.analyze_storage = true;
config.analyze_cache = true;
config.analyze_indexes = true;
config.generate_recommendations = true;
```

**Usage:**
```cpp
PerformanceAnalyzer analyzer(config);

// Run comprehensive analysis
PerformanceAnalysis analysis = analyzer.analyze(query_profiler, storage_profiler);

// Print summary
std::cout << analysis.toReport() << std::endl;

// Export to JSON
analyzer.export_analysis(analysis, "/var/log/themisdb/perf_analysis.json");

// Export HTML report
analyzer.export_html_report(analysis, "/var/www/themisdb/performance.html");
```

**Issue Detection:**
```cpp
// Analyze specific areas
auto query_issues = analyzer.analyze_queries(query_profiler);
for (const auto& issue : query_issues) {
    std::cout << "[" << to_string(issue.severity) << "] "
              << issue.title << std::endl;
    std::cout << "  " << issue.description << std::endl;

    for (const auto& rec : issue.recommendations) {
        std::cout << "  → " << rec << std::endl;
    }
}

// Example issues detected:
// [CRITICAL] Excessive Full Table Scans
//   5 queries performing full scans on table 'metrics' (avg 50K rows)
//   → Create index on column 'timestamp' for time-range queries
//   → Add covering index (tenant_id, timestamp, value) for common pattern
//
// [WARNING] Low Cache Hit Rate
//   Block cache hit rate: 45% (threshold: 80%)
//   → Increase block cache size from 512MB to 2GB
//   → Enable partition filters in SST files
```

**Automated Recommendations:**
```cpp
json recommendations = analyzer.generate_recommendations(analysis.issues);
// {
//   "indexes": [
//     {
//       "priority": "high",
//       "action": "CREATE INDEX idx_metrics_timestamp ON metrics(timestamp)",
//       "reason": "Frequent time-range queries detected",
//       "estimated_improvement": "70% latency reduction"
//     }
//   ],
//   "configuration": [
//     {
//       "priority": "medium",
//       "setting": "rocksdb.block_cache_size",
//       "current_value": "512MB",
//       "recommended_value": "2GB",
//       "reason": "Cache hit rate below threshold"
//     }
//   ],
//   "query_patterns": [
//     {
//       "priority": "high",
//       "pattern": "SELECT * FROM metrics WHERE ...",
//       "recommendation": "Use column projection instead of SELECT *",
//       "savings": "Reduce network bandwidth by 60%"
//     }
//   ]
// }
```

### Alertmanager (`alertmanager.cpp`)

**Purpose:** Integration with Prometheus Alertmanager for critical event notifications

Sends alerts for system anomalies, resource exhaustion, and performance degradation.

**Alert Severities:**
- `INFO`: Informational (non-urgent)
- `WARNING`: Potential issue (attention recommended)
- `ERROR`: Error condition (requires action)
- `CRITICAL`: Critical failure (immediate action required)

**Alert Statuses:**
- `FIRING`: Alert currently active
- `RESOLVED`: Issue resolved
- `SILENCED`: Alert acknowledged/suppressed

**Configuration:**
```cpp
AlertmanagerConfig config;
config.endpoint_url = "http://alertmanager:9093/api/v2/alerts";
config.auth_token = "Bearer <token>";  // Optional
config.timeout_seconds = 10;
config.enabled = true;
config.receivers = {"pagerduty", "slack", "email"};

Alertmanager alertmanager(config);
alertmanager.initialize(config);
```

**Usage:**
```cpp
// Send alert
Alert alert;
alert.alert_name = "HighQueryLatency";
alert.severity = AlertSeverity::WARNING;
alert.message = "Average query latency exceeded 1000ms";
alert.labels = {
    {"component", "query_engine"},
    {"instance", "themisdb-node-1"},
    {"environment", "production"}
};
alert.annotations = {
    {"summary", "Query latency spike detected"},
    {"description", "P95 latency: 1250ms (threshold: 1000ms)"},
    {"runbook_url", "https://docs.example.com/runbooks/high-query-latency"}
};

auto result = alertmanager.sendAlert(alert);
if (result.isOk()) {
    std::cout << "Alert sent successfully" << std::endl;
}

// Resolve alert
alertmanager.resolveAlert("HighQueryLatency");

// Silence alert temporarily
alertmanager.silenceAlert("HighQueryLatency", 60);  // 60 minutes
```

**Integration with MetricsCollector:**
```cpp
// Check metrics and send alerts
auto metrics = MetricsCollector::getInstance();
auto prometheus_output = metrics.getPrometheusMetrics();

// Parse and evaluate alert rules
if (avg_query_latency > 1000) {
    Alert alert;
    alert.alert_name = "HighQueryLatency";
    alert.severity = AlertSeverity::WARNING;
    alert.message = "Query latency exceeded threshold";
    alertmanager.sendAlert(alert);
}
```

### ContinuousProfiler (`continuous_profiler.cpp`)

**Purpose:** Always-on CPU sampling profiler producing pprof / async-profiler compatible output

A background thread samples call stacks at a configurable rate (default: ~1% CPU overhead)
and accumulates them as pprof folded-stacks text, which is directly consumable by
`go tool pprof`, Brendan Gregg's `flamegraph.pl`, and async-profiler's toolchain.

**Profile Types:**
- `CPU`: Call-stack sampling (implemented)
- `HEAP`, `MUTEX`, `BLOCK`: Reserved for future integration

**Key Operations:**

| Operation | Description |
|-----------|-------------|
| `start()` | Begin background sampling |
| `stop()` | Flush remaining data and join the worker thread |
| `snapshot()` | Capture current accumulated stacks as a `ProfileSnapshot` |
| `getSnapshots()` | Retrieve snapshots within a time range |
| `compare()` | Differential analysis between two snapshots |
| `registerAnomalyCallback()` | Fire callback on >20% CPU regression between flushes |
| `enable()` / `disable()` | Dynamic on/off without restart |

**Configuration:**
```cpp
ContinuousProfilerConfig cfg;
cfg.enabled = true;
cfg.cpu_sample_rate = 0.01;          // 1% CPU overhead (sampling period ≈ 100 ms)
cfg.snapshot_interval = std::chrono::seconds(60);
cfg.max_snapshots_retained = 1440;   // 24 h at 1/min
cfg.output_dir = "/var/lib/themisdb/profiles";
cfg.enable_cpu_profiling = true;
```

**Usage:**
```cpp
#include "observability/continuous_profiler.h"

using namespace themis::observability;

ContinuousProfiler profiler(cfg);
profiler.start();

// ... workload ...

// Capture and persist
auto snap = profiler.snapshot(ProfileType::CPU);
snap.saveToFile("/var/lib/themisdb/profiles/cpu_<timestamp>.folded");

// Analyze with go tool pprof or flamegraph.pl:
//   go tool pprof -http=:8080 cpu_<ts>.folded
//   flamegraph.pl cpu_<ts>.folded > flame.svg

// Differential regression detection
auto diff = profiler.compare(baseline, snap);
if (diff.cpu_regression_percent > 10.0) {
    // emit alert
}

// Register anomaly callback
profiler.registerAnomalyCallback(
    [](const ProfileSnapshot& snap, const std::string& msg) {
        // called from background thread; must be thread-safe
        THEMIS_WARN("Profiler anomaly: {}", msg);
    });

profiler.stop();
```

**pprof Folded-Stacks Format:**
```
frame1;frame2;leaf 42
frame1;frame3;leaf 7
```
Each line is a semicolon-joined call stack followed by a sample count.
Load into `go tool pprof` or convert to SVG with `flamegraph.pl`.

**Output Directory:**
Auto-named files: `<output_dir>/<type>_<unix_timestamp>.folded`

**Thread Safety:** All public methods are thread-safe (pImpl + internal mutex)



### Environment Variables

```bash
# Metrics
export THEMIS_METRICS_ENABLED=true
export THEMIS_METRICS_PORT=9090
export THEMIS_METRICS_PATH=/metrics

# Query Profiling
export THEMIS_QUERY_PROFILER_ENABLED=true
export THEMIS_QUERY_PROFILER_SLOW_THRESHOLD_MS=1000
export THEMIS_QUERY_PROFILER_RETENTION_HOURS=24

# Storage Profiling
export THEMIS_STORAGE_PROFILER_ENABLED=true
export THEMIS_STORAGE_PROFILER_SLOW_THRESHOLD_MS=100
export THEMIS_STORAGE_PROFILER_STATS_INTERVAL_SEC=60

# Performance Analysis
export THEMIS_PERF_ANALYZER_ENABLED=true
export THEMIS_PERF_ANALYZER_ANALYSIS_INTERVAL_SEC=300

# Alerting
export THEMIS_ALERTMANAGER_URL=http://alertmanager:9093
export THEMIS_ALERTMANAGER_ENABLED=true
```

## Configuration File (themisdb.yaml)

```yaml
observability:
  metrics:
    enabled: true
    port: 9090
    path: /metrics

  tracing:
    enabled: true
    service_name: themisdb
    endpoint: http://jaeger:14268/api/traces
    sample_rate: 0.1  # 10% sampling

  query_profiler:
    enabled: true
    profile_all_queries: false
    slow_query_threshold_ms: 1000
    max_profiles_retained: 1000
    retention_hours: 24

  storage_profiler:
    enabled: true
    slow_op_threshold_ms: 100
    stats_collection_interval_sec: 60
    retention_hours: 24

  performance_analyzer:
    enabled: true
    analysis_interval_sec: 300
    thresholds:
      cache_hit_rate_percent: 80
      write_amplification: 10.0
      read_amplification: 5.0
      max_full_scan_rows: 1000

  alertmanager:
    enabled: true
    endpoint: http://alertmanager:9093/api/v2/alerts
    timeout_seconds: 10
    receivers:
      - pagerduty
      - slack
      - email
```

## Integration with Core

### ILogger Integration

```cpp
#include "core/concerns/i_logger.h"
#include "observability/metrics_collector.h"

using namespace themis::core::concerns;
using namespace themis::observability;

class ObservabilityLogger {
    ILogger& logger_;

public:
    explicit ObservabilityLogger(ILogger& logger) : logger_(logger) {}

    void logSlowQuery(const QueryProfile& profile) {
        logger_.warn("Slow query detected: " + profile.query_text +
                    " (duration: " + std::to_string(profile.total_duration.count()) + "μs)");

        MetricsCollector::getInstance().recordQuery(
            "slow_query",
            profile.total_duration.count() / 1000.0,
            profile.result_rows
        );
    }
};
```

### ITracer Integration

```cpp
#include "core/concerns/i_tracer.h"
#include "observability/query_profiler.h"

using namespace themis::core::concerns;
using namespace themis::observability;

class TracedQueryExecution {
    ITracer& tracer_;
    QueryProfiler& profiler_;

public:
    TracedQueryExecution(ITracer& tracer, QueryProfiler& profiler)
        : tracer_(tracer), profiler_(profiler) {}

    void executeQuery(const std::string& query_id, const std::string& query_text) {
        // Create trace span
        auto span = tracer_.startSpan("query_execution");
        span->setAttribute("query_id", query_id);
        span->setAttribute("query_text", query_text);

        // Start profiling
        profiler_.start_query(query_id, query_text);

        try {
            // Execute query phases
            parseQuery(query_text);
            span->setAttribute("phase", "parsed");

            optimizeQuery();
            span->setAttribute("phase", "optimized");

            executeQueryPlan();
            span->setAttribute("phase", "executed");

            span->setStatus(true, "Query completed successfully");
        } catch (const std::exception& e) {
            span->recordError(e.what());
            span->setStatus(false, "Query failed");
            throw;
        }

        // End profiling
        profiler_.end_query(query_id);
        span->end();
    }
};
```

### IMetrics Integration

```cpp
#include "core/concerns/i_metrics.h"
#include "observability/metrics_collector.h"

using namespace themis::core::concerns;
using namespace themis::observability;

class MetricsAdapter : public IMetrics {
    MetricsCollector& collector_;

public:
    MetricsAdapter() : collector_(MetricsCollector::getInstance()) {}

    void incrementCounter(const std::string& name, int64_t value, const Labels& labels) override {
        collector_.incrementCounter(name, labels);
    }

    void setGauge(const std::string& name, double value, const Labels& labels) override {
        collector_.setGauge(name, value, labels);
    }

    void observeHistogram(const std::string& name, double value, const Labels& labels) override {
        collector_.observeHistogram(name, value, labels);
    }

    void recordLatency(const std::string& operation, double latencyMs, const Labels& labels) override {
        collector_.observeHistogram(operation + "_latency_ms", latencyMs, labels);
    }

    std::string exportMetrics() const override {
        return collector_.getPrometheusMetrics();
    }

    void reset() override {
        collector_.reset();
    }
};
```

## Monitoring Best Practices

### 1. Metrics Collection Strategy

**Golden Signals:**
- **Latency**: Query execution time, storage operation latency
- **Traffic**: Queries per second, writes per second
- **Errors**: Failed queries, authentication errors, storage errors
- **Saturation**: CPU usage, memory usage, disk I/O utilization

**Metric Naming Convention:**
```
<namespace>_<subsystem>_<metric>_<unit>{labels}

Examples:
themisdb_query_latency_seconds{type="select",shard="shard-1"}
themisdb_tsstore_writes_total{metric="cpu_usage"}
themisdb_cache_hit_rate{type="query"}
themisdb_rocksdb_write_amplification{db="default"}
```

### 2. Dashboard Organization

**Grafana Dashboard Hierarchy:**

1. **Overview Dashboard** (default)
   - System health indicators
   - Request rate and latency
   - Error rates
   - Resource utilization

2. **Query Performance Dashboard**
   - Query latency percentiles (P50, P95, P99)
   - Slow query log
   - Query types distribution
   - Index usage statistics

3. **Storage Dashboard**
   - RocksDB metrics
   - Compaction statistics
   - Write/read amplification
   - Cache performance

4. **Shard Dashboard**
   - Per-shard metrics
   - Rebalancing progress
   - Cross-shard query performance

5. **Security Dashboard**
   - Authentication attempts
   - Policy evaluation latency
   - Failed access attempts

### 3. Alerting Strategy

**Alert Tiers:**

**Tier 1 - Page Immediately (PagerDuty):**
- Database unavailable
- Critical query latency (P95 > 5s)
- Disk space < 10%
- OOM errors

**Tier 2 - Notify Team (Slack):**
- High query latency (P95 > 1s)
- Cache hit rate < 50%
- Write amplification > 20
- Slow compactions

**Tier 3 - Log for Review (Ticketing):**
- Cache hit rate < 80%
- Full table scans detected
- Unused indexes
- Suboptimal queries

### 4. Performance Profiling Workflow

**Query Optimization:**
```bash
# 1. Enable profiling
curl -X POST http://themisdb:8000/api/v1/config/query_profiler/enable

# 2. Execute query
curl -X POST http://themisdb:8000/api/v1/query \
  -d '{"query": "SELECT * FROM metrics WHERE tenant_id = 'acme'"}'

# 3. Get profile
curl http://themisdb:8000/api/v1/query_profiles/{query_id}

# 4. Analyze recommendations
curl http://themisdb:8000/api/v1/performance/analyze
```

## Alerting Rules

### Prometheus Alert Rules (prometheus-alerts.yaml)

```yaml
groups:
  - name: themisdb_alerts
    interval: 30s
    rules:
      # Query Performance
      - alert: HighQueryLatency
        expr: histogram_quantile(0.95, rate(themisdb_query_latency_seconds_bucket[5m])) > 1
        for: 5m
        labels:
          severity: warning
          component: query_engine
        annotations:
          summary: "High query latency detected"
          description: "P95 query latency is {{ $value }}s (threshold: 1s)"

      - alert: CriticalQueryLatency
        expr: histogram_quantile(0.95, rate(themisdb_query_latency_seconds_bucket[5m])) > 5
        for: 2m
        labels:
          severity: critical
          component: query_engine
        annotations:
          summary: "Critical query latency"
          description: "P95 query latency is {{ $value }}s"

      # Cache Performance
      - alert: LowCacheHitRate
        expr: (themisdb_cache_hits_total / (themisdb_cache_hits_total + themisdb_cache_misses_total)) < 0.5
        for: 10m
        labels:
          severity: warning
          component: cache
        annotations:
          summary: "Low cache hit rate"
          description: "Cache hit rate is {{ $value | humanizePercentage }}"

      # Storage Health
      - alert: HighWriteAmplification
        expr: themisdb_rocksdb_write_amplification > 20
        for: 15m
        labels:
          severity: warning
          component: storage
        annotations:
          summary: "High write amplification"
          description: "Write amplification is {{ $value }} (threshold: 20)"

      # Resource Usage
      - alert: HighMemoryUsage
        expr: themisdb_memory_usage_bytes / themisdb_memory_limit_bytes > 0.9
        for: 5m
        labels:
          severity: critical
          component: system
        annotations:
          summary: "High memory usage"
          description: "Memory usage is {{ $value | humanizePercentage }}"

      - alert: DiskSpaceLow
        expr: themisdb_disk_free_bytes / themisdb_disk_total_bytes < 0.1
        for: 5m
        labels:
          severity: critical
          component: storage
        annotations:
          summary: "Low disk space"
          description: "Only {{ $value | humanizePercentage }} disk space remaining"

      # Authentication
      - alert: HighAuthFailureRate
        expr: rate(themisdb_auth_attempts_total{result="failure"}[5m]) / rate(themisdb_auth_attempts_total[5m]) > 0.5
        for: 5m
        labels:
          severity: warning
          component: auth
        annotations:
          summary: "High authentication failure rate"
          description: "{{ $value | humanizePercentage }} of auth attempts failing"
```

## Debugging Workflows

### 1. Slow Query Investigation

```bash
# Step 1: Identify slow queries
curl http://themisdb:8000/api/v1/query_profiles?slow_threshold_ms=1000

# Step 2: Get detailed profile
curl http://themisdb:8000/api/v1/query_profiles/{query_id}

# Step 3: Check if index is used
# Look for: "used_index": true, "indexes_used": ["idx_name"]

# Step 4: Get optimization recommendations
curl http://themisdb:8000/api/v1/performance/recommendations?query_id={query_id}

# Step 5: Test with EXPLAIN
curl -X POST http://themisdb:8000/api/v1/query/explain \
  -d '{"query": "SELECT ..."}'

# Step 6: Implement recommendation (create index)
curl -X POST http://themisdb:8000/api/v1/admin/indexes \
  -d '{"table": "metrics", "column": "timestamp", "type": "btree"}'

# Step 7: Verify improvement
curl http://themisdb:8000/api/v1/query_profiles/{new_query_id}
```

## 2. Storage Performance Issues

```bash
# Step 1: Check RocksDB statistics
curl http://themisdb:8000/api/v1/storage/stats

# Step 2: Check amplification metrics
curl http://themisdb:8000/api/v1/storage/amplification

# Step 3: Identify slow operations
curl http://themisdb:8000/api/v1/storage/operations?slow_threshold_ms=100

# Step 4: Trigger manual compaction
curl -X POST http://themisdb:8000/api/v1/admin/storage/compact

# Step 5: Verify improvement
curl http://themisdb:8000/api/v1/storage/stats
```

## 3. Cache Performance Tuning

```bash
# Step 1: Check cache metrics
curl http://themisdb:8000/api/v1/metrics | grep cache

# Step 2: Analyze cache patterns
curl http://themisdb:8000/api/v1/performance/cache_analysis

# Step 3: Adjust cache size
curl -X PUT http://themisdb:8000/api/v1/config \
  -d '{"rocksdb.block_cache_size": "2GB"}'

# Step 4: Monitor improvement
watch -n 5 'curl -s http://themisdb:8000/api/v1/metrics | grep cache_hit_rate'
```

## Performance Profiling

### Flame Graph Generation

```bash
# Step 1: Enable CPU profiling
curl -X POST http://themisdb:8000/api/v1/profiling/cpu/start

# Step 2: Run workload
# ... execute queries ...

# Step 3: Stop profiling and get results
curl -X POST http://themisdb:8000/api/v1/profiling/cpu/stop > profile.pb.gz

# Step 4: Generate flame graph
go tool pprof -http=:8080 profile.pb.gz

# Step 5: Analyze hotspots in browser
# Open http://localhost:8080
```

## Memory Profiling

```bash
# Heap snapshot
curl http://themisdb:8000/api/v1/profiling/heap > heap.pb.gz
go tool pprof -http=:8080 heap.pb.gz

# Check for memory leaks
curl http://themisdb:8000/api/v1/profiling/heap/diff
```

## See Also

- [ROADMAP.md](ROADMAP.md) — implementation status and planned features
- [ARCHITECTURE.md](ARCHITECTURE.md) — module architecture guide
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned observability features
- [../../docs/de/observability/README.md](../../docs/de/observability/README.md) — module overview (DE)
- [../../docs/observability/README.md](../../docs/observability/README.md) — observability docs index
- [../../grafana/](../../grafana/) - Grafana dashboard definitions
- [../../prometheus/](../../prometheus/) - Prometheus configuration
- [Core Module](../core/README.md) - ILogger, ITracer, IMetrics interfaces

## Scientific References

1. OpenTelemetry Authors. (2021). **OpenTelemetry Specification**. Cloud Native Computing Foundation. https://opentelemetry.io/docs/specs/otel/

2. Sigelman, B. H., Barroso, L. A., Burrows, M., Stephenson, P., Plakal, M., Beaver, D., … Shanbhag, C. (2010). **Dapper, a Large-Scale Distributed Systems Tracing Infrastructure**. Google Technical Report. https://research.google/pubs/pub36356/

3. W3C Distributed Tracing Working Group. (2021). **Trace Context (Level 1)**. W3C Recommendation. https://www.w3.org/TR/trace-context/

4. Beyer, B., Jones, C., Petoff, J., & Murphy, N. R. (2016). **Site Reliability Engineering: How Google Runs Production Systems**. O'Reilly Media. ISBN: 978-1-491-92912-4

5. Prometheus Authors. (2023). **Prometheus: An Open-Source Monitoring System with a Dimensional Data Model**. CNCF Project. https://prometheus.io/

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
