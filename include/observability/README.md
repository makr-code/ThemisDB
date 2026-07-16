> **Build:** `cmake --preset release && cmake --build build/release`

# Observability Module Headers

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ../../src/observability/README.md · ../../src/observability/ARCHITECTURE.md · ../../src/observability/ROADMAP.md · ../../src/observability/FUTURE_ENHANCEMENTS.md -->

Public interfaces and declarations for ThemisDB observability system.

## Table of Contents

1. [Overview](#overview)
2. [Header Files](#header-files)
   - [metrics_collector.h](#metrics_collectorh)
   - [query_profiler.h](#query_profilerh)
   - [storage_profiler.h](#storage_profilerh)
   - [performance_analyzer.h](#performance_analyzerh)
   - [alertmanager.h](#alertmanagerh)
   - [alerting_engine.h](#alerting_engineh)
   - [continuous_profiler.h](#continuous_profilerh)
   - [tracer.h](#tracerh)
   - [opentelemetry_tracer.h](#opentelemetry_tracerh)
   - [log_aggregator.h](#log_aggregatorh)
   - [log_search_engine.h](#log_search_engineh)
   - [advanced_metrics.h](#advanced_metricsh)
   - [slo_reporter.h](#slo_reporterh)
   - [ml_anomaly_detector.h](#ml_anomaly_detectorh)
   - [metric_aggregator.h](#metric_aggregatorh)
   - [metric_anomaly_detector.h](#metric_anomaly_detectorh)
   - [metrics_stream_server.h](#metrics_stream_serverh)
   - [root_cause_analyzer.h](#root_cause_analyzerh)
   - [tenant_metrics_namespace.h](#tenant_metrics_namespaceh)
   - [distributed_flame_graph.h](#distributed_flame_graphh)
   - [ebpf_tracer.h](#ebpf_tracerh)
   - [otlp_exemplar.h](#otlp_exemplarh)
3. [Data Structures](#data-structures)
4. [API Reference](#api-reference)
5. [Integration Guide](#integration-guide)
6. [Thread Safety](#thread-safety)
7. [Performance Considerations](#performance-considerations)
8. [Troubleshooting](#troubleshooting)
9. [See Also](#see-also)

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

### alerting_engine.h

**Purpose:** Rule-based alerting engine with pluggable notification channels

**Key Classes:**
- `INotificationChannel`: Abstract interface for notification backends (log, webhook, Slack, …)
- `LogNotificationChannel`: Sends alerts to the ThemisDB structured logger
- `WebhookNotificationChannel`: HTTP POST alerts to an arbitrary webhook endpoint
- `SlackNotificationChannel`: Posts alert payloads to a Slack webhook
- `AlertingEngine`: Owns an `AlertRuleManager`, evaluates rules on each `sendAlert()` call, and dispatches to registered channels; extends `Alertmanager`

**Features:**
- Pluggable `INotificationChannel` interface — add custom channels without changing engine code
- Predefined default rules: CPU >80%, memory >90%, query P99 latency >1000 ms, error rate >5%, disk free <10%, query queue depth >100, cache miss rate >50%, write amplification >20×
- `AlertRuleManager` for CRUD and evaluation of `AlertRule` conditions
- Optional Prometheus Alertmanager backend delegation

**Example:**
```cpp
#include "observability/alerting_engine.h"

using namespace themis::observability;

// Build engine with log + Slack channels
AlertingEngineConfig cfg;
cfg.enabled = true;

AlertingEngine engine(cfg);
engine.addChannel(std::make_unique<LogNotificationChannel>());
engine.addChannel(std::make_unique<SlackNotificationChannel>(
    "https://hooks.slack.com/services/T.../B.../..."));
engine.loadDefaultRules();

// Evaluate and dispatch an alert
Alert alert;
alert.alert_name = "HighQueryLatency";
alert.severity   = AlertSeverity::WARNING;
alert.message    = "P99 query latency exceeded 1 000 ms";
engine.sendAlert(alert);
```

**Thread Safety:** Thread-safe; internal mutex guards channel registry and rule evaluation

---

### tracer.h

**Purpose:** W3C Trace Context–compatible distributed tracer with in-process span ring buffer and ContinuousProfiler integration

**Key Classes:**
- `ObservabilityTracer`: Implements `core::concerns::ITracer`; produces `SpanRecord` entries retained in a ring buffer
- `TracerStats`: Snapshot of `total_spans`, `active_spans`, `dropped_spans`
- `ObservabilityTracerConfig`: Runtime configuration

**Configuration:**
```cpp
struct ObservabilityTracerConfig {
    std::string service_name = "themisdb";
    std::string endpoint;             // OTLP/HTTP endpoint; empty = in-process only
    double      sample_rate = 1.0;    // [0.0, 1.0]
    size_t      max_retained_spans = 1000;
    bool        publish_metrics = true;
    bool        attach_profile_on_span_end = false;
    std::shared_ptr<ContinuousProfiler> profiler; // optional
};
```

**Example:**
```cpp
#include "observability/tracer.h"

using namespace themis::observability;

ObservabilityTracerConfig cfg;
cfg.service_name = "themisdb";
cfg.sample_rate  = 0.1;   // 10% sampling

ObservabilityTracer tracer(cfg);

auto span = tracer.startSpan("db.query");
span->setAttribute("db.operation", "SELECT");
// ... execute query ...
span->end();

TracerStats stats = tracer.getStats();
```

**Thread Safety:** All public methods are thread-safe

---

### opentelemetry_tracer.h

**Purpose:** Full OpenTelemetry SDK integration — OTLP gRPC/HTTP export, Jaeger/Zipkin adapters, W3C Baggage, exception recording, metrics snapshots

**Key Classes:**
- `OpenTelemetryTracer`: Production `ITracer` implementation with multi-exporter dispatch
- `OTelConfig`: Runtime configuration (endpoints, exporters, resource attributes)
- `SpanMetrics`: CPU/memory/disk metrics snapshot attachable to a span

**Features:**
- OTLP gRPC (`otlp`) and HTTP (`otlp-http`) async export
- `JaegerTracerAdapter` and `ZipkinTracerAdapter` delegate sub-tracers
- W3C Baggage for tenant/user context propagation
- `recordException()` — attaches exception type, message, and optional stack trace as span events
- `recordMetrics()` — attaches `SpanMetrics` snapshot as span attributes
- Thread-safe in-process span ring buffer

**Example:**
```cpp
#include "observability/opentelemetry_tracer.h"

using namespace themis::observability;

OTelConfig cfg;
cfg.service_name         = "themisdb";
cfg.endpoint             = "http://otel-collector:4317";
cfg.exporters            = {"otlp", "jaeger"};
cfg.resource_attributes  = {{"deployment.environment", "production"}};

OpenTelemetryTracer tracer(cfg);

// Inject tenant context via W3C Baggage
OpenTelemetryTracer::setBaggageItem("tenant-id", "acme");

auto span = tracer.startSpan("db.query");
span->setAttribute("db.operation", "SELECT");

SpanMetrics snap;
snap.cpu_usage_percent = 45.2;
tracer.recordMetrics(*span, snap);

span->end();

// Propagate to outbound call
std::map<std::string, std::string> headers;
tracer.injectContext(headers);
```

**Thread Safety:** All public methods are thread-safe

---

### log_aggregator.h

**Purpose:** Structured JSON log collection, trace-context correlation, in-process ring buffer with optional file sink

**Key Classes:**
- `LogAggregator`: Implements `core::concerns::IAsyncLogger`; buffers `LogEntry` records in a ring buffer and optionally flushes to a file
- `LogEntry`: Single structured log entry with timestamp, level, message, and arbitrary key/value fields
- `LogAggregatorConfig`: Runtime configuration
- `LogSinkType`: `MEMORY`, `FILE`, `BOTH`

**Configuration:**
```cpp
struct LogAggregatorConfig {
    ILogger::Level min_level{ILogger::Level::INFO};
    size_t         max_entries = 10000;       // ring buffer capacity
    LogSinkType    sink_type{LogSinkType::MEMORY};
    std::string    file_path;                 // required for FILE / BOTH
    bool           correlate_trace_context = true;
};
```

**Example:**
```cpp
#include "observability/log_aggregator.h"

using namespace themis::observability;

LogAggregatorConfig cfg;
cfg.min_level  = core::concerns::ILogger::Level::DEBUG;
cfg.sink_type  = LogSinkType::BOTH;
cfg.file_path  = "/var/log/themisdb/structured.jsonl";

LogAggregator agg(cfg);
agg.logStructured(core::concerns::ILogger::Level::WARN, "Slow query",
                  {{"query_id", "q-42"}, {"latency_ms", "850"}});

auto entries = agg.entries();  // copy of the in-process ring buffer
```

**Thread Safety:** Thread-safe; internal mutex + condition variable guard the ring buffer

---

### log_search_engine.h

**Purpose:** Query structured logs by level, time range, field filters, and message content

**Key Classes:**
- `LogSearchEngine`: Stateless search engine over a `std::vector<LogEntry>`
- `LogSearchQuery`: Compound filter (min_level, time range, field predicates, message substring, limit/offset, sort order)
- `LogSearchResult`: Result with matched `LogEntry` list and total count
- `FieldFilter`: Per-field predicate with `FieldMatchOp` (EQUALS, CONTAINS, STARTS_WITH, REGEX)

**Example:**
```cpp
#include "observability/log_search_engine.h"

using namespace themis::observability;

// Assumes agg is an existing LogAggregator instance
LogSearchEngine engine;

LogSearchQuery q;
q.min_level     = core::concerns::ILogger::Level::WARN;
q.field_filters = {{"query_id", FieldMatchOp::EQUALS, "q-42"}};
q.limit         = 50;

auto result = engine.search(agg.entries(), q);
for (const auto& entry : result.entries) {
    std::cout << entry.toJson() << "\n";
}
```

**Thread Safety:** Stateless; thread-safe as long as the input `LogEntry` vector is not mutated concurrently

---

### advanced_metrics.h

**Purpose:** Extended metric primitives beyond counters, gauges, and histograms

**Key Classes:**
- `AdvancedMetrics`: Provides Summary, ExponentialHistogram, Cardinality, TimeWeightedAverage, and Rate metric types; all methods thread-safe

**Key Methods:**
```cpp
// Summary – sliding-window quantile computation
void   recordSummary(const std::string& name, double value);
SummaryResult getSummary(const std::string& name,
                         const std::vector<double>& quantiles = {0.5, 0.9, 0.99}) const;

// Exponential Histogram
void   recordExponentialHistogram(const std::string& name, double value,
                                  double base = 2.0);
ExponentialHistogramResult getExponentialHistogram(const std::string& name) const;

// Cardinality (exact hash-set)
void   recordCardinality(const std::string& name, const std::string& value);
size_t getCardinalityEstimate(const std::string& name) const;

// Time-Weighted Average (∫value×dt over a sliding window)
void   recordTimeWeightedAverage(const std::string& name, double value,
                                  std::chrono::seconds window = std::chrono::seconds(300));
double getTimeWeightedAverage(const std::string& name) const;

// Rate (per-second change over a configurable window)
void   recordRate(const std::string& name, double value,
                  std::chrono::seconds window = std::chrono::seconds(60));
double getRate(const std::string& name) const;
```

**Example:**
```cpp
#include "observability/advanced_metrics.h"

using namespace themis::observability;

AdvancedMetrics metrics;

// Track unique tenants
metrics.recordCardinality("active_tenants", tenant_id);
size_t unique = metrics.getCardinalityEstimate("active_tenants");

// P99 query latency
metrics.recordSummary("query_latency_ms", latency_ms);
auto summary = metrics.getSummary("query_latency_ms", {0.5, 0.99});
double p99 = summary.quantile_values.at(0.99);
```

**Thread Safety:** All public methods are fully guarded by an internal mutex

---

### slo_reporter.h

**Purpose:** SLO/SLA burn-rate alerting following the Google SRE multi-window model

**Key Classes:**
- `SloReporter`: Evaluates SLO compliance and raises burn-rate alerts
- `SloDefinition`: Defines an SLO (target, window, error budget)
- `BurnRateAlert`: Raised when a window/burn-rate pair is breached
- `BurnRateLevel`: `FAST` (14.4×, critical), `MEDIUM` (6×, warning), `SLOW` (3×, info)

**Burn-Rate Windows:**

| Level  | Burn-rate | Short window | Long window | Severity |
|--------|-----------|-------------|-------------|----------|
| FAST   | 14.4×     | 1 h         | 5 min       | critical |
| MEDIUM | 6×        | 6 h         | 30 min      | warning  |
| SLOW   | 3×        | 24 h        | 2 h         | info     |

**Example:**
```cpp
#include "observability/slo_reporter.h"

using namespace themis::observability;

SloDefinition slo;
slo.name           = "query_availability";
slo.target_percent = 99.9;
slo.window         = std::chrono::hours(24 * 30);  // 30-day rolling window

SloReporter reporter;
reporter.registerSlo(slo);
reporter.recordEvent("query_availability", /*success=*/true);
reporter.recordEvent("query_availability", /*success=*/false);  // error

auto alerts = reporter.evaluate();
for (const auto& alert : alerts) {
    std::cout << "SLO breach: " << alert.slo_name
              << " burn-rate=" << alert.burn_rate_multiplier << "\n";
}
```

**Thread Safety:** Thread-safe; internal mutex guards all state

---

### ml_anomaly_detector.h

**Purpose:** ML-based anomaly detection using ARIMA/Holt-Winters forecasting and Isolation Forest/LOF outlier scoring

**Key Classes:**
- `MLAnomalyDetector`: Trains a forecast model and runs a configurable outlier detector
- `MLConfig`: Configuration (metric name, forecast backend, anomaly threshold, DBSCAN parameters)
- `MLAnomalyResult`: Result with `is_anomaly`, `score`, observed vs. expected values, and description
- `ForecastBackend`: `ARIMA`, `PROPHET` (implemented via Holt-Winters)

**Example:**
```cpp
#include "observability/ml_anomaly_detector.h"

using namespace themis::observability;

MLConfig cfg;
cfg.metric_name         = "query_latency_ms";
cfg.forecast_backend    = ForecastBackend::ARIMA;
cfg.anomaly_threshold   = 0.7;
cfg.min_training_points = 30;

MLAnomalyDetector detector(cfg);

// Feed historical observations
for (const auto& sample : history) {
    detector.addObservation(sample.value, sample.timestamp);
}

// Detect on a new point
auto result = detector.detect(current_value);
if (result.is_anomaly) {
    std::cerr << "Anomaly: score=" << result.score << "\n";
}
```

**Thread Safety:** Not thread-safe; use per-thread instances or external synchronization

---

### metric_aggregator.h

**Purpose:** Rate calculation, histogram aggregation, cross-shard pre-aggregation, and cardinality rollup for multi-source metrics

**Key Classes:**
- `MetricAggregator`: Aggregates raw `MetricSample` observations according to `AggregationRule` policies
- `AggregationRule`: Controls per-metric aggregation type, time window, grouping labels, and label drops
- `AggregationType`: SUM, AVG, MAX, MIN, P50, P95, P99, RATE
- `HistogramSnapshot`: Single-source histogram observations
- `AggregatedHistogram`: Merged histogram across sources

**Example:**
```cpp
#include "observability/metric_aggregator.h"

using namespace themis::observability;

MetricAggregator agg;

// Register a rate rule for a counter
AggregationRule rule;
rule.metric_name    = "query_total";
rule.type           = AggregationType::RATE;
rule.interval       = std::chrono::seconds(60);
rule.drop_labels    = {"user_id"};   // drop high-cardinality label
agg.addRule(rule);

// Feed samples from multiple shards
agg.record("query_total", 120.0, {{"shard", "shard-1"}, {"user_id", "u-99"}});
agg.record("query_total",  80.0, {{"shard", "shard-2"}, {"user_id", "u-17"}});

// Get aggregated result
double qps = agg.getAggregated("query_total");
```

**Thread Safety:** Thread-safe; internal mutex guards the sample store

---

### metric_anomaly_detector.h

**Purpose:** Bridges the `analytics::StreamingAnomalyDetector` with `MetricsCollector` — detects anomalies on live metric streams

**Key Classes:**
- `MetricAnomalyDetector`: Subscribes to a `MetricsCollector` stream and runs a configurable analytics-layer anomaly detector
- `MonitoredMetric`: Descriptor for a metric to monitor (name, detector config, threshold)
- `MetricAnomaly`: Result with `metric_name`, `score`, `is_anomaly`, `observed_value`, and `severity`
- `AnomalyMethod`: `ISOLATION_FOREST`, `LOF`, `ENSEMBLE` (forwarded from `analytics`)

**Example:**
```cpp
#include "observability/metric_anomaly_detector.h"

using namespace themis::observability;

MetricAnomalyDetectorConfig cfg;
cfg.window_size = 100;   // rolling window of observations

MetricAnomalyDetector detector(cfg);

MonitoredMetric m;
m.name      = "query_latency_ms";
m.threshold = 0.8;
detector.monitor(m);

// Feed observations
detector.observe("query_latency_ms", current_latency_ms);

// Get anomalies
auto anomalies = detector.getAnomalies();
for (const auto& a : anomalies) {
    if (a.is_anomaly) {
        std::cerr << a.metric_name << ": severity=" << a.severity << "\n";
    }
}
```

**Thread Safety:** Thread-safe; internal mutex guards all state

---

### metrics_stream_server.h

**Purpose:** Real-time metric streaming via WebSocket or Server-Sent Events (SSE)

**Key Classes:**
- `MetricsStreamServer`: Core dispatch engine; network I/O is decoupled via a `SendFn` callback
- `StreamSubscription`: Per-client subscription (requested metrics, label filters, interval)
- `MetricUpdate`: Payload pushed to clients on each interval tick

**Protocol:**
- WebSocket: `ws://host:port/metrics/stream` — subscribe with JSON action, receive `metric_update` events
- SSE: `GET /metrics/events` — each event is a `data:` JSON line

**Example:**
```cpp
#include "observability/metrics_stream_server.h"

using namespace themis::observability;

// SendFn decouples the engine from actual network transport
auto send_fn = [](const std::string& client_id, const std::string& payload) {
    // write payload to WebSocket / SSE stream for client_id
};

MetricsStreamServer server(send_fn);
server.start();

// Register a subscriber (e.g. on WebSocket CONNECT)
StreamSubscription sub;
sub.client_id     = "cli-1";
sub.metrics       = {"query_latency_ms", "cache_hit_rate"};
sub.interval_ms   = 1000;
server.subscribe(sub);

// Unsubscribe on disconnect
server.unsubscribe("cli-1");
```

**Thread Safety:** All public methods are thread-safe

---

### root_cause_analyzer.h

**Purpose:** Automated root cause analysis via Pearson correlation and Granger-inspired causal graph inference

**Key Classes:**
- `RootCauseAnalyzer`: Accepts `SystemSnapshot` pairs and infers a ranked list of probable root causes
- `SystemSnapshot`: Point-in-time capture of key performance metrics (write amplification, query latency, cache hit rate, etc.)
- `TimeSeries` / `TimeSeriesPoint`: Named metric history for correlation input
- `CausalGraphNode`: Node in the inferred causal graph with metric name and causal confidence
- `RootCauseReport`: Analysis result with ranked root causes, evidence, and recommended mitigations

**Example:**
```cpp
#include "observability/root_cause_analyzer.h"

using namespace themis::observability;

RootCauseAnalyzer analyzer;

SystemSnapshot before, after;
before.write_amplification = 3.0;
before.query_latency_p99_ms = 50.0;
after.write_amplification  = 18.0;
after.query_latency_p99_ms = 980.0;

auto report = analyzer.analyze(before, after);
for (const auto& cause : report.root_causes) {
    std::cout << "[" << cause.confidence << "] " << cause.metric_name
              << ": " << cause.explanation << "\n";
}
```

**Thread Safety:** Not thread-safe; use separate instance per analysis or external synchronization

---

### tenant_metrics_namespace.h

**Purpose:** Per-tenant metric namespacing with independent cardinality budgets and automatic `themis_<tenant_id>_` Prometheus prefix

**Key Classes:**
- `TenantMetricsNamespace`: Registry for per-tenant `MetricsCollector` instances
- `TenantMetricsConfig`: Configures the global cardinality limit per tenant

**Key Methods:**
```cpp
void   registerTenant(const std::string& tenant_id);
void   unregisterTenant(const std::string& tenant_id);

void   increment(const std::string& tenant_id, const std::string& metric,
                 const Labels& labels = {});
void   setGauge(const std::string& tenant_id, const std::string& metric,
                double value, const Labels& labels = {});
void   observeHistogram(const std::string& tenant_id, const std::string& metric,
                        double value, const Labels& labels = {});

std::string exportTenant(const std::string& tenant_id) const;
std::string exportAll() const;
```

**Example:**
```cpp
#include "observability/tenant_metrics_namespace.h"

using namespace themis::observability;

TenantMetricsConfig cfg;
cfg.cardinality_limit_per_tenant = 200;

TenantMetricsNamespace registry(cfg);
registry.registerTenant("acme");

registry.increment("acme", "query_total", {{"type", "select"}});
registry.setGauge("acme", "active_connections", 12.0);

// Returns Prometheus text with prefix "themis_acme_"
std::string prom = registry.exportTenant("acme");
```

**Thread Safety:** Thread-safe; shared/unique mutex guards the tenant registry

---

### distributed_flame_graph.h

**Purpose:** Merge per-node `ContinuousProfiler` snapshots into a cluster-wide flame graph

**Key Classes:**
- `DistributedFlameGraph`: Merges `NodeProfile` contributions into a `MergedFlameGraph`
- `NodeProfile`: Per-node profile (node_id, host, `ProfileSnapshot`)
- `MergedFlameGraph`: Aggregated stacks in pprof folded-stacks text format; `toFoldedText()` returns the merged profile

**Example:**
```cpp
#include "observability/distributed_flame_graph.h"

using namespace themis::observability;

DistributedFlameGraph dfg;

// Collect snapshots from each node (e.g. via gRPC)
dfg.addNodeProfile({"node-1", "10.0.0.1", snapshot_node1});
dfg.addNodeProfile({"node-2", "10.0.0.2", snapshot_node2});

MergedFlameGraph merged = dfg.merge();

// Write to file for Brendan Gregg's flamegraph.pl
std::ofstream out("cluster.folded");
out << merged.toFoldedText();

// Or consume with go tool pprof
// go tool pprof -http=:8080 cluster.folded
```

**Thread Safety:** Not thread-safe; build the graph under external synchronization if used concurrently

---

### ebpf_tracer.h

**Purpose:** eBPF-based kernel-level performance tracing (context switches, page faults, CPU migrations, CPU time)

**Key Classes:**
- `EbpfTracer`: Polls Linux `perf_event_open` at a configurable interval; no-op on non-Linux platforms and when `THEMIS_ENABLE_EBPF` is not defined
- `EbpfTracerConfig`: Configures probe types, collection interval, and event callback
- `KernelEvent`: Single kernel event sample (type, timestamp, delta, description)
- `EbpfTracerStats`: Cumulative statistics snapshot
- `EbpfProbeType`: `CONTEXT_SWITCH`, `PAGE_FAULT`, `CPU_MIGRATION`, `TASK_CLOCK`, `NONE`

**Runtime Behavior:**
- Requires `THEMIS_ENABLE_EBPF` build flag and Linux `perf_event_open` to be available
- On non-Linux platforms or when the flag is absent, `start()` succeeds but collects only `NONE` events
- Fails open: if `perf_event_open` returns an error, the tracer continues running without kernel events

**Example:**
```cpp
#include "observability/ebpf_tracer.h"

using namespace themis::observability;

EbpfTracerConfig cfg;
cfg.probes            = {EbpfProbeType::CONTEXT_SWITCH, EbpfProbeType::PAGE_FAULT};
cfg.collection_interval = std::chrono::seconds(1);
cfg.event_callback    = [](const KernelEvent& ev) {
    std::cout << "kernel event: " << ev.description << " delta=" << ev.delta << "\n";
};

EbpfTracer tracer(cfg);
tracer.start();
// ... run workload ...
EbpfTracerStats stats = tracer.getStats();
tracer.stop();
```

**Thread Safety:** Thread-safe; internal mutex guards the event buffer and stats

---

### otlp_exemplar.h

**Purpose:** OTLP exemplar reservoir interface — planned for Q3 2026; use `MetricsCollector::observeHistogramWithExemplar()` for the currently implemented exemplar path

**Key Classes:**
- `IExemplarReservoir` (interface): `offer()`, `collect()`, `size()`, `reset()`
- `IExemplarSampler` (interface): `shouldSample()`, `recordMeasurement()`
- `MetricExemplar`: Exemplar payload with `TraceContext`, filtered attributes, value, and timestamp
- `ExemplarReservoirConfig`: Strategy (`SIMPLE_FIXED_SIZE`, `ALIGNED_HISTOGRAM`, `TRACE_BASED`), reservoir size
- `TraceContext`: W3C trace\_id, span\_id, trace\_flags

**Status:** Interface header only; no production implementation yet (Target: Q3 2026). For current exemplar support see `MetricsCollector::observeHistogramWithExemplar()` in `metrics_collector.h`.

**Thread Safety:** Implementation-defined (no concrete class shipped yet)

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

## Troubleshooting

### MetricsCollector — no metrics visible at `/metrics`
- Verify `MetricsCollector::getInstance()` is called at least once before the HTTP scrape
- Check that the `/metrics` route calls `getPrometheusMetrics()` and serves `text/plain; version=0.0.4`
- Confirm no label cardinality limit has been exceeded (check `getCardinalityLimit()`)

### QueryProfiler — profiles not appearing in slow query list
- Ensure `profile_all_queries = true` or that query duration exceeded `slow_query_threshold`
- Profiles are retained up to `max_profiles_retained`; older entries are evicted
- QueryProfiler is not thread-safe; confirm you are not sharing an instance across threads without synchronization

### AlertingEngine — alerts not firing
- Call `loadDefaultRules()` or register custom `AlertRule` objects via `AlertRuleManager`
- Add at least one `INotificationChannel` with `addChannel()` before calling `sendAlert()`
- Confirm `AlertingEngineConfig::enabled = true`

### OpenTelemetryTracer — spans not reaching the collector
- Check that `OTelConfig::endpoint` is non-empty and reachable from the ThemisDB process
- The OTLP exporter uses an async bounded queue (default 10 000 spans); if the collector is unreachable, spans are dropped silently
- Verify `OTelConfig::exporters` contains `"otlp"` (for gRPC) or `"otlp-http"` (for HTTP)

### EbpfTracer — no kernel events collected
- Confirm the build was compiled with `THEMIS_ENABLE_EBPF` and is running on Linux
- `perf_event_open` requires either `CAP_PERFMON` (Linux ≥5.8) or `CAP_SYS_ADMIN`
- On restricted environments (containers without the capability) the tracer falls back to `NONE` events silently

### LogAggregator — log file not written
- Set `sink_type = LogSinkType::FILE` or `BOTH` and ensure `file_path` is a writable path
- Confirm the process has write permission to the directory containing the file

### SloReporter — no burn-rate alerts raised
- At least `min_training_points` events must be recorded before evaluation returns alerts
- Check that the SLO target and error budget are correctly configured
- Call `evaluate()` periodically; the reporter does not run a background timer

## See Also

- [`../../src/observability/README.md`](../../src/observability/README.md) — implementation documentation and component internals
- [`../../src/observability/ARCHITECTURE.md`](../../src/observability/ARCHITECTURE.md) — module architecture guide
- [`../../src/observability/ROADMAP.md`](../../src/observability/ROADMAP.md) — implementation status and planned features
- [`../../src/observability/FUTURE_ENHANCEMENTS.md`](../../src/observability/FUTURE_ENHANCEMENTS.md) — planned enhancements and design constraints
- [`../core/concerns/`](../core/concerns/) — Core interfaces: `ILogger`, `ITracer`, `IMetrics`, `IAsyncLogger`
- [`../../docs/de/observability/README.md`](../../docs/de/observability/README.md) — module overview (DE)
- [`../../docs/observability/README.md`](../../docs/observability/README.md) — observability docs index

## All Headers

| Header | Purpose |
|--------|---------|
| `advanced_metrics.h` | Extended metrics types and aggregation helpers |
| `alerting_engine.h` | Rule-based alerting engine |
| `alertmanager.h` | Prometheus Alertmanager integration |
| `continuous_profiler.h` | Always-on CPU profiler with pprof output |
| `distributed_flame_graph.h` | Distributed flame graph aggregation |
| `ebpf_tracer.h` | eBPF-based kernel tracing |
| `log_aggregator.h` | Log aggregation and forwarding |
| `log_search_engine.h` | Full-text search over log entries |
| `metric_aggregator.h` | Multi-source metric aggregation |
| `metric_anomaly_detector.h` | Statistical anomaly detection on metrics |
| `metrics_collector.h` | Central Prometheus-compatible metrics collection |
| `metrics_stream_server.h` | Streaming metrics server |
| `ml_anomaly_detector.h` | ML-based anomaly detection |
| `opentelemetry_tracer.h` | OpenTelemetry tracing integration |
| `otlp_exemplar.h` | OTLP exemplar support for metrics |
| `performance_analyzer.h` | Automated performance issue detection |
| `query_profiler.h` | Query execution profiling with explain plans |
| `root_cause_analyzer.h` | Root cause analysis for incidents |
| `slo_reporter.h` | SLO/SLA compliance reporting |
| `storage_profiler.h` | RocksDB storage layer profiling |
| `tenant_metrics_namespace.h` | Per-tenant metrics namespace isolation |
| `tracer.h` | Core distributed tracing interface |

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
