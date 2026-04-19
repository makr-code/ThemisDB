> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/observability/ -->

# Observability Module - Future Enhancements
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · docs/de/observability/README.md -->

## Scope

- Prometheus metrics export (`/metrics`, `themis_*` namespace) via `MetricsCollector` singleton
- Distributed tracing with span context propagation (OpenTelemetry-compatible; OTLP gRPC/HTTP export planned)
- Structured logging via Core `ILogger` interface with log-level filtering
- `QueryProfiler`: per-phase and per-operator timing with index usage tracking
- `StorageProfiler`: RocksDB stats, write/read amplification, compaction metrics, cache hit rates
- Continuous profiling (pprof / async-profiler compatible) and adaptive sampling for high-frequency spans
- eBPF-based kernel-level tracing (perf counters; guarded by `THEMIS_ENABLE_EBPF`)
- Anomaly detection on metrics time-series (ML-based, planned) and SLO/SLA burn-rate alerting

## Design Constraints

- [ ] Metrics collection overhead must be < 1% CPU at steady-state (1,000 req/s workload)
- [ ] Span sampling rate must be adaptive: reduce to ≤ 1% at > 10,000 spans/sec to cap trace export overhead
- [ ] OTLP export failures must not block the request path; use a bounded async queue (default: 10,000 spans)
- [ ] eBPF probes must be no-ops on non-Linux platforms and fail-open when `perf_event_open` fails
- [ ] Prometheus histogram bucket boundaries must be stable across releases to preserve Grafana dashboard compatibility
- [ ] Per-tenant metric namespacing must not leak cross-tenant cardinality via label sets
- [ ] Structured log entries must not contain PII (user data, query result values); only query IDs, latencies, and operator names
- [ ] Alertmanager webhook payloads must be retried up to 3 times with exponential backoff before dropping

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `MetricsCollector::increment(name, labels)` | All modules | Thread-safe; label set must be pre-registered to cap cardinality |
| `MetricsCollector::exportPrometheus()` | HTTP `/metrics` handler | Returns Prometheus text format; called by scrape endpoint |
| `Tracer::startSpan(name, parent)` | Query executor, storage, network | Returns `Span`; propagates W3C Trace Context headers |
| `Tracer::exportOTLP(endpoint)` | Background exporter thread | OTLP gRPC/HTTP; async bounded queue; requires `THEMIS_ENABLE_OTLP` |
| `QueryProfiler::profile(query, plan)` | AQL execution engine | Records per-operator timing; attaches to active trace span |
| `StorageProfiler::collect()` | Background metrics job | Reads RocksDB statistics; publishes to MetricsCollector |
| `PerformanceAnalyzer::analyze(snapshot)` | Alertmanager integration | Returns ranked list of detected issues with recommendations |
| `EbpfTracer::start(config)` | Server bootstrap | Polls `perf_event_open` at 1 s interval; guarded by `THEMIS_ENABLE_EBPF` |

Planned monitoring, tracing, and performance analysis features for ThemisDB.

## Table of Contents

1. [Distributed Tracing](#distributed-tracing)
2. [Advanced Metrics](#advanced-metrics)
3. [AI-Powered Analysis](#ai-powered-analysis)
4. [Real-Time Monitoring](#real-time-monitoring)
5. [Continuous Profiling](#continuous-profiling)
6. [Anomaly Detection](#anomaly-detection)
7. [Cost Analysis](#cost-analysis)
8. [Predictive Analytics](#predictive-analytics)
9. [Enhanced Visualization](#enhanced-visualization)
10. [Integration Enhancements](#integration-enhancements)

---

## Source Code Audit Findings (2026-03-12)

### [x] `MetricsCollector`: Upgrade to `shared_mutex` for Metric Read Path
**Priority:** Medium
**Target Version:** v1.8.0

`metrics_collector.cpp` previously used `std::lock_guard<std::mutex>` (exclusive) for all read operations. This has been upgraded: read paths now use `std::shared_lock<std::shared_mutex>` and write paths use `std::unique_lock<std::shared_mutex>`, allowing multiple concurrent Prometheus scrapers without serialization.

**Implementation Notes:**
- `[x]` Replace `std::mutex mutex_` with `std::shared_mutex` in `MetricsCollector`.
- `[x]` Upgrade `getPrometheusMetrics` (scrape) and `getCardinalityLimit` (series count) to `std::shared_lock`. `getDroppedSeriesCount()` remains lock-free via `std::atomic<int64_t>`.
- `[x]` Keep `record`, `increment`, `setGauge`, `observeHistogram`, `reset` on `std::unique_lock`.
- `[x]` TSAN-enabled stress test added: `TSANStress_16ScrapersAnd8Writers` — 16 Prometheus scrape threads + 8 metric write threads concurrently.

**Performance Targets:**
- `scrapePrometheus()` throughput under 16 concurrent scrapers: ≥ 3× improvement vs. exclusive-mutex baseline. Structurally guaranteed: `shared_lock` allows all 16 scrapers to proceed concurrently instead of serializing.

---

## Distributed Tracing

### OpenTelemetry Full Integration
**Priority:** High
**Target Version:** v1.6.0

Complete OpenTelemetry implementation with automatic span propagation across distributed components.

**Features:**
- Automatic instrumentation for all database operations
- W3C Trace Context propagation
- Baggage support for tenant/user context
- Multiple exporter support (Jaeger, Zipkin, OTLP)

**Implementation:**
```cpp
class OpenTelemetryTracer : public ITracer {
public:
    OpenTelemetryTracer(const OTelConfig& config);

    // Automatic span creation with context propagation
    std::unique_ptr<ISpan> startSpan(const std::string& name) override;

    // Extract context from incoming request
    SpanContext extractContext(const std::map<std::string, std::string>& headers);

    // Inject context into outgoing request
    void injectContext(const ISpan& span, std::map<std::string, std::string>& headers);

    // Span attributes
    void recordException(const ISpan& span, const std::exception& ex);
    void recordMetrics(const ISpan& span, const MetricSnapshot& metrics);
};

// Configuration
struct OTelConfig {
    std::string service_name = "themisdb";
    std::string service_version = "1.6.0";
    std::string endpoint = "http://otel-collector:4317";
    std::string protocol = "grpc";  // or "http"
    double sample_rate = 1.0;  // 100% sampling
    std::map<std::string, std::string> resource_attributes;
    std::vector<std::string> exporters = {"otlp", "jaeger"};
};
```

**Usage:**
```cpp
// Initialize tracer
OTelConfig config;
config.service_name = "themisdb";
config.endpoint = "http://otel-collector:4317";
config.resource_attributes = {
    {"deployment.environment", "production"},
    {"service.instance.id", "themisdb-node-1"}
};

OpenTelemetryTracer tracer(config);

// Query with distributed tracing
{
    auto span = tracer.startSpan("query_execution");
    span->setAttribute("query.text", query_text);
    span->setAttribute("db.system", "themisdb");
    span->setAttribute("db.operation", "SELECT");

    // Child spans automatically inherit context
    executeQuery(query_text);

    span->setAttribute("query.result_rows", result_count);
    span->setStatus(true);
}
```

**Benefits:**
- End-to-end request tracing across microservices
- Cross-shard query visualization
- Root cause analysis for distributed issues
- Integration with existing observability platforms

---

### Span Events and Links
**Priority:** Medium
**Target Version:** v1.6.0

Rich span context with events and inter-span relationships.

**Implementation:**
```cpp
class EnhancedSpan : public ITracer::ISpan {
public:
    // Record events within span
    void addEvent(const std::string& name,
                  const std::map<std::string, std::string>& attributes = {},
                  std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

    // Link to related spans
    void addLink(const SpanContext& context,
                 const std::map<std::string, std::string>& attributes = {});

    // Structured attributes
    void setAttributes(const std::map<std::string, AttributeValue>& attributes);
};

// Example: Query execution with events
auto span = tracer.startSpan("distributed_query");
span->addEvent("query_parsed", {{"syntax", "valid"}});
span->addEvent("shards_contacted", {{"count", "5"}});
span->addEvent("partial_results_received", {{"shard", "shard-3"}});
span->addEvent("all_results_merged");

// Link to related operations
span->addLink(compaction_span_context, {{"relation", "blocking_operation"}});
```

---

### Service Mesh Integration
**Priority:** Medium
**Target Version:** v1.7.0

Automatic tracing via Envoy/Istio sidecar injection.

**Features:**
- No code changes required for basic tracing
- Automatic retry and timeout tracking
- Circuit breaker state in spans
- Load balancing decision tracking

**Configuration:**
```yaml
apiVersion: networking.istio.io/v1alpha3
kind: VirtualService
metadata:
  name: themisdb
spec:
  hosts:
  - themisdb
  http:
  - match:
    - headers:
        x-trace-enabled:
          exact: "true"
    route:
    - destination:
        host: themisdb
      headers:
        request:
          add:
            x-b3-sampled: "1"
```

---

## Advanced Metrics

### Custom Metric Types
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (v1.6.0) — `include/observability/advanced_metrics.h` + `src/observability/advanced_metrics.cpp`

Extended metric types beyond counters, gauges, and histograms.

**Implementation:**
```cpp
class AdvancedMetrics {
public:
    // Summary (like histogram but with quantiles)
    // Quantiles are specified at query time, not at record time.
    void recordSummary(const std::string& name, double value);
    SummaryResult getSummary(const std::string& name,
                             const std::vector<double>& quantiles = {0.5, 0.9, 0.95, 0.99}) const;

    // Exponential histogram (efficient for wide value ranges)
    void recordExponentialHistogram(const std::string& name, double value,
                                   double scale = 2.0);
    ExponentialHistogramResult getExponentialHistogram(const std::string& name) const;

    // Cardinality metrics
    void recordCardinality(const std::string& name, const std::string& value);
    size_t getCardinalityEstimate(const std::string& name) const;

    // Time-weighted average
    void recordTimeWeightedAverage(const std::string& name, double value,
                                   std::chrono::seconds window);
    double getTimeWeightedAverage(const std::string& name) const;

    // Rate metrics (automatically computed)
    void recordRate(const std::string& name, double value,
                   std::chrono::seconds interval);
    double getRate(const std::string& name) const;
};

// Example: Track unique tenant access patterns
metrics.recordCardinality("active_tenants", tenant_id);
metrics.recordTimeWeightedAverage("tenant_qps", qps, std::chrono::minutes(5));
double avg_qps = metrics.getTimeWeightedAverage("tenant_qps");

// Example: P99 latency summary
metrics.recordSummary("query_latency_ms", latency_ms);
auto summary = metrics.getSummary("query_latency_ms", {0.5, 0.99});
double p99 = summary.quantile_values.at(0.99);
```

---

### Exemplars Support
**Priority:** Medium
**Target Version:** v1.6.0

Link metrics to traces via exemplars for drill-down analysis.

**Implementation:**
```cpp
struct Exemplar {
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::string trace_id;
    std::string span_id;
    std::map<std::string, std::string> labels;
};

class ExemplarEnabledMetrics {
public:
    void observeHistogramWithExemplar(const std::string& name,
                                     double value,
                                     const Exemplar& exemplar);
};

// Example: Link slow query metric to trace
Exemplar exemplar;
exemplar.value = 1250.0;  // ms
exemplar.trace_id = span->getTraceId();
exemplar.span_id = span->getSpanId();
exemplar.labels = {{"query_type", "aggregate"}};

metrics.observeHistogramWithExemplar("query_latency_ms", 1250.0, exemplar);
```

**Benefits:**
- Click on metric spike in Grafana → jump to relevant trace
- Root cause analysis from metrics
- Reduced MTTR (mean time to resolution)

---

### Multi-Dimensional Metrics
**Priority:** Medium
**Target Version:** v1.7.0

Rich label dimensions for detailed metric segmentation.

**Implementation:**
```cpp
// Record query latency with multiple dimensions
metrics.recordLatency("query_latency_ms", latency_ms, {
    {"query_type", "SELECT"},
    {"tenant_id", "acme"},
    {"shard_id", "shard-1"},
    {"cache_status", "hit"},
    {"index_used", "idx_timestamp"},
    {"user_tier", "premium"},
    {"region", "us-west-2"}
});

// Query metrics with flexible aggregation
// - avg(query_latency_ms{query_type="SELECT"}) by (tenant_id)
// - histogram_quantile(0.95, query_latency_ms{shard_id="shard-1"})
// - sum(rate(query_latency_ms{user_tier="premium"}[5m])) by (region)
```

---

### Metric Aggregation Pipeline
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented

Pre-aggregate metrics across shards for efficient querying.

**Implementation:**
```cpp
class MetricAggregator {
public:
    // Configure aggregation rules
    void addAggregationRule(const AggregationRule& rule);

    // Aggregate metrics from multiple shards
    MetricSnapshot aggregateShardMetrics(const std::vector<ShardMetrics>& shard_metrics);

    // Rollup metrics to reduce cardinality
    void rollupMetrics(std::chrono::minutes window);
};

struct AggregationRule {
    std::string metric_name;
    AggregationType type;  // SUM, AVG, MAX, MIN, P99
    std::chrono::seconds interval;
    std::vector<std::string> group_by_labels;
    std::vector<std::string> drop_labels;  // Reduce cardinality
};

// Example: Aggregate query latency across shards
AggregationRule rule;
rule.metric_name = "query_latency_ms";
rule.type = AggregationType::P95;
rule.interval = std::chrono::seconds(60);
rule.group_by_labels = {"tenant_id", "query_type"};
rule.drop_labels = {"shard_id", "instance_id"};  // Drop high-cardinality labels

aggregator.addAggregationRule(rule);
```

**Files:**
- `include/observability/metric_aggregator.h` — `ShardMetrics`, `MetricSnapshot` structs; `aggregateShardMetrics()`, `rollupMetrics()` declarations
- `src/observability/metric_aggregator.cpp` — stateless cross-shard aggregation and window-based rollup
- `tests/test_metrics_aggregation.cpp` — `AggregateShardMetrics_*` and `RollupMetrics_*` test cases

---

## AI-Powered Analysis

### Machine Learning-Based Anomaly Detection
**Priority:** High
**Target Version:** v1.7.0

Automated detection of performance anomalies using ML models.

**Features:**
- Time-series forecasting (ARIMA, Prophet)
- Outlier detection (Isolation Forest, DBSCAN)
- Seasonal pattern recognition
- Change point detection

**Implementation:**
```cpp
class MLAnomalyDetector {
public:
    explicit MLAnomalyDetector(const MLConfig& config);

    // Train model on historical data
    void train(const std::vector<TimeSeries>& training_data);

    // Detect anomalies in real-time
    std::vector<Anomaly> detectAnomalies(const TimeSeries& current_data);

    // Predict future values
    TimeSeries forecast(std::chrono::hours horizon);

    // Explain anomaly (feature importance)
    AnomalyExplanation explainAnomaly(const Anomaly& anomaly);
};

struct Anomaly {
    std::chrono::system_clock::time_point timestamp;
    std::string metric_name;
    double actual_value;
    double expected_value;
    double confidence_score;  // 0-1
    std::string severity;     // low, medium, high, critical
    std::vector<std::string> contributing_factors;
};

// Example usage
MLAnomalyDetector detector(config);
detector.train(historical_query_latencies);

auto anomalies = detector.detectAnomalies(current_query_latencies);
for (const auto& anomaly : anomalies) {
    if (anomaly.confidence_score > 0.8) {
        alertmanager.sendAlert({
            .alert_name = "MLAnomalyDetected",
            .severity = AlertSeverity::WARNING,
            .message = "Unusual pattern detected: " + anomaly.metric_name,
            .annotations = {
                {"expected", std::to_string(anomaly.expected_value)},
                {"actual", std::to_string(anomaly.actual_value)},
                {"confidence", std::to_string(anomaly.confidence_score)}
            }
        });
    }
}
```

---

### Intelligent Query Recommendations
**Priority:** Medium
**Target Version:** v1.7.0

ML-powered query optimization suggestions based on workload patterns.

**Features:**
- Index recommendation engine
- Query rewrite suggestions
- Caching strategy recommendations
- Shard key optimization

**Implementation:**
```cpp
class QueryRecommendationEngine {
public:
    // Analyze query patterns
    void analyzeWorkload(const std::vector<QueryProfile>& profiles);

    // Generate recommendations
    std::vector<Recommendation> generateRecommendations();

    // Estimate impact
    ImpactAnalysis estimateImpact(const Recommendation& rec);
};

struct Recommendation {
    enum Type { CREATE_INDEX, REWRITE_QUERY, ADJUST_CACHE, REPARTITION };

    Type type;
    std::string title;
    std::string description;
    std::string sql_statement;  // For CREATE INDEX, etc.
    double estimated_improvement_percent;
    std::string confidence;  // low, medium, high
    std::vector<std::string> affected_queries;
};

// Example output
// Recommendation: Create Index
// Title: "Add composite index on (tenant_id, timestamp)"
// Confidence: High
// Estimated Improvement: 65% latency reduction
// Affected Queries: 234 queries per day
// SQL: CREATE INDEX idx_tenant_timestamp ON metrics(tenant_id, timestamp)
```

---

### Root Cause Analysis
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented (v1.7.0, Issue #84)

> **Implementation files:** `include/observability/root_cause_analyzer.h`, `src/observability/root_cause_analyzer.cpp`
> **Tests:** `tests/test_root_cause_analyzer.cpp` (RootCauseAnalyzerFocusedTests — 34 tests)
> **CI:** `.github/workflows/root-cause-analyzer-ci.yml`

Automated root cause identification for performance issues.

**Implementation:**
```cpp
class RootCauseAnalyzer {
public:
    // Analyze performance degradation
    RootCauseReport analyzeIssue(const PerformanceIssue& issue,
                                 const SystemSnapshot& before,
                                 const SystemSnapshot& after);

    // Correlation analysis
    std::vector<CorrelatedMetric> findCorrelations(const std::string& metric_name);

    // Causal inference
    CausalGraph buildCausalGraph(const std::vector<TimeSeries>& metrics);
};

struct RootCauseReport {
    std::string primary_cause;  // "High compaction rate"
    double confidence;          // 0.87
    std::vector<std::string> contributing_factors;
    std::vector<std::string> remediation_steps;
    std::map<std::string, double> metric_impacts;  // metric -> change %
};

// Example: Why did query latency spike?
auto report = analyzer.analyzeIssue(latency_spike, before_snapshot, after_snapshot);
// Primary Cause: High compaction rate (87% confidence)
// Contributing Factors:
//   - Write amplification increased from 3.2 to 15.1
//   - Memtable flush rate doubled
//   - Block cache hit rate dropped from 85% to 45%
// Remediation:
//   1. Increase memtable size to reduce flush frequency
//   2. Tune compaction trigger threshold
//   3. Add more cache capacity
```

---

## Real-Time Monitoring

### Streaming Metrics
**Priority:** High
**Target Version:** v1.6.0

Real-time metric streaming via WebSocket or Server-Sent Events.

**Implementation:**
```cpp
class MetricsStreamServer {
public:
    // Start streaming server
    void start(const std::string& bind_address, uint16_t port);

    // Client subscription
    void subscribe(const StreamSubscription& subscription);

    // Push metrics to subscribers
    void pushMetrics(const MetricUpdate& update);
};

struct StreamSubscription {
    std::string client_id;
    std::vector<std::string> metric_names;
    std::vector<MetricFilter> filters;
    std::chrono::milliseconds update_interval;
};

// Client-side usage (JavaScript)
const ws = new WebSocket('ws://themisdb:8001/metrics/stream');
ws.send(JSON.stringify({
    subscribe: {
        metrics: ['query_latency_ms', 'cache_hit_rate'],
        filters: [{label: 'tenant_id', value: 'acme'}],
        interval_ms: 1000
    }
}));

ws.onmessage = (event) => {
    const update = JSON.parse(event.data);
    updateDashboard(update.metrics);
};
```

---

### Live Query Profiling
**Priority:** Medium
**Target Version:** v1.6.0

Real-time query execution visualization.

**Features:**
- Live query plan updates
- Operator progress tracking
- Resource usage monitoring
- Estimated completion time

**Implementation:**
```cpp
class LiveQueryProfiler {
public:
    // Start profiling with callback
    void startLiveProfile(const std::string& query_id,
                         std::function<void(const ProfileUpdate&)> callback);

    // Push updates during execution
    void updateProgress(const std::string& query_id,
                       const OperatorProgress& progress);
};

struct ProfileUpdate {
    std::string query_id;
    QueryPhase current_phase;
    std::vector<OperatorProgress> operator_progress;
    ResourceUsage current_resources;
    std::chrono::seconds estimated_remaining_time;
};

// Web UI displays live updates:
// Query: SELECT * FROM metrics WHERE ...
// Phase: EXECUTE (78% complete)
// Operator: INDEX_SCAN (45K / 60K rows)
// Memory: 512 MB / 2 GB
// ETA: 5 seconds
```

---

### Dashboard Auto-Refresh
**Priority:** Low
**Target Version:** v1.7.0

Intelligent dashboard refresh based on data volatility.

**Features:**
- Adaptive refresh rates
- Suspend refresh when window inactive
- Smart caching for expensive queries
- Delta updates (only changed metrics)

---

## Continuous Profiling

### Always-On CPU Profiling
**Priority:** Medium
**Target Version:** v1.6.0

Low-overhead continuous CPU profiling in production.

**Implementation:**
```cpp
class ContinuousProfiler {
public:
    ContinuousProfiler(const ProfilerConfig& config);

    // Start continuous profiling
    void start();

    // Collect profile snapshot
    Profile snapshot();

    // Compare profiles
    ProfileDiff compare(const Profile& baseline, const Profile& current);
};

struct ProfilerConfig {
    double sampling_rate = 0.01;  // 1% overhead
    std::chrono::seconds snapshot_interval{60};
    std::string output_dir = "/var/lib/themisdb/profiles";
    bool enable_heap_profiling = false;
    bool enable_mutex_profiling = false;
};

// Automatic regression detection
auto diff = profiler.compare(last_week_profile, current_profile);
if (diff.cpu_regression_percent > 10.0) {
    alertmanager.sendAlert({
        .alert_name = "PerformanceRegression",
        .severity = AlertSeverity::WARNING,
        .message = "CPU usage increased by " + std::to_string(diff.cpu_regression_percent) + "%"
    });
}
```

---

### Memory Leak Detection
**Priority:** High
**Target Version:** v1.6.0

Automatic memory leak detection and reporting.

**Implementation:**
```cpp
class MemoryLeakDetector {
public:
    // Start monitoring
    void startMonitoring(std::chrono::minutes interval = std::chrono::minutes(5));

    // Analyze heap growth
    std::vector<LeakCandidate> detectLeaks();

    // Generate detailed report
    LeakReport generateReport(const LeakCandidate& candidate);
};

struct LeakCandidate {
    std::string allocation_site;  // File:line
    size_t total_bytes;
    size_t num_allocations;
    double growth_rate_mb_per_hour;
    std::vector<std::string> stack_traces;
};

// Automatic leak detection
auto leaks = detector.detectLeaks();
for (const auto& leak : leaks) {
    if (leak.growth_rate_mb_per_hour > 10.0) {
        auto report = detector.generateReport(leak);
        logger.critical("Memory leak detected: " + report.summary);
    }
}
```

---

### Lock Contention Analysis
**Priority:** Medium
**Target Version:** v1.7.0

Identify and analyze lock contention hotspots.

**Implementation:**
```cpp
class LockContentionAnalyzer {
public:
    // Track lock acquisitions
    void recordLockAcquisition(const std::string& lock_name,
                              std::chrono::microseconds wait_time);

    // Generate contention report
    ContentionReport analyzeContention();
};

struct ContentionReport {
    std::vector<HotLock> hot_locks;
    std::map<std::string, std::vector<std::string>> lock_ordering;
    std::vector<DeadlockRisk> potential_deadlocks;
};

// Example output:
// Hot Locks:
//   1. MetricsCollector::mutex_ - 45% CPU time in lock wait
//      Contention points: 12 call sites
//      Recommendation: Use lock-free atomic operations
//   2. QueryCache::cache_lock_ - 23% CPU time in lock wait
//      Recommendation: Implement sharded locking
```

---

## Anomaly Detection

### Statistical Anomaly Detection
**Priority:** High
**Target Version:** v1.6.0

Statistical methods for anomaly detection without ML training.

**Algorithms:**
- Z-score (standard deviation)
- Modified Z-score (MAD - median absolute deviation)
- Grubbs' test
- Dixon's Q test
- Tukey's fences (IQR method)

**Implementation:**
```cpp
class StatisticalAnomalyDetector {
public:
    // Configure detection
    void setThreshold(double num_std_devs = 3.0);
    void setMethod(AnomalyMethod method);

    // Detect anomalies
    std::vector<Anomaly> detect(const TimeSeries& data);

    // Seasonal decomposition
    SeasonalComponents decompose(const TimeSeries& data);
};

enum class AnomalyMethod {
    ZSCORE,           // (x - mean) / std > threshold
    MODIFIED_ZSCORE,  // (x - median) / MAD > threshold
    IQR,              // x < Q1 - 1.5*IQR || x > Q3 + 1.5*IQR
    GRUBBS,           // Statistical test for outliers
    SEASONAL          // Decompose + detect on residuals
};

// Example: Detect query latency spikes
StatisticalAnomalyDetector detector;
detector.setMethod(AnomalyMethod::SEASONAL);
detector.setThreshold(3.0);  // 3 sigma

auto anomalies = detector.detect(query_latencies);
// Detected 3 anomalies:
//   - 2024-02-10 14:23:15: latency 1250ms (expected 150ms, 5.2σ)
//   - 2024-02-10 14:45:32: latency 980ms (expected 150ms, 4.1σ)
//   - 2024-02-10 15:12:08: latency 1450ms (expected 150ms, 6.1σ)
```

---

### Baseline Comparison
**Priority:** Medium
**Target Version:** v1.6.0

Compare current metrics against historical baselines.

**Implementation:**
```cpp
class BaselineComparator {
public:
    // Create baseline from historical data
    void createBaseline(const std::string& name,
                       const TimeSeries& historical_data,
                       std::chrono::hours window = std::chrono::hours(168));  // 1 week

    // Compare against baseline
    ComparisonReport compare(const std::string& baseline_name,
                            const TimeSeries& current_data);
};

struct ComparisonReport {
    std::string baseline_name;
    std::map<std::string, MetricComparison> metric_comparisons;
    double overall_deviation_percent;
    std::vector<std::string> significant_changes;
};

// Example: Compare today vs. last week
comparator.createBaseline("last_week", last_week_data);
auto report = comparator.compare("last_week", today_data);

// Query latency: +35% vs. baseline (p=0.001, significant)
// Cache hit rate: -12% vs. baseline (p=0.05, significant)
// Memory usage: +3% vs. baseline (p=0.4, not significant)
```

---

### Alerting with Adaptive Thresholds
**Priority:** High
**Target Version:** v1.6.0

Dynamic alert thresholds that adapt to workload patterns.

**Implementation:**
```cpp
class AdaptiveThresholdAlerts {
public:
    // Learn thresholds from data
    void learnThresholds(const std::string& metric_name,
                        const TimeSeries& training_data);

    // Evaluate with adaptive thresholds
    std::optional<Alert> evaluate(const std::string& metric_name,
                                 double current_value,
                                 std::chrono::system_clock::time_point timestamp);

    // Account for time-of-day, day-of-week patterns
    void enableSeasonalAdjustment(bool enabled);
};

// Example: Query latency varies by time of day
// - Morning (8am-12pm): baseline 50ms, alert > 200ms
// - Afternoon (12pm-5pm): baseline 100ms, alert > 350ms
// - Evening (5pm-11pm): baseline 30ms, alert > 150ms
// - Night (11pm-8am): baseline 10ms, alert > 50ms
```

---

## Cost Analysis

### Query Cost Estimation
**Priority:** Medium
**Target Version:** v1.7.0

Estimate resource costs for queries before execution.

**Implementation:**
```cpp
class QueryCostEstimator {
public:
    // Estimate query cost
    QueryCost estimateCost(const std::string& query_text);

    // Cost breakdown
    CostBreakdown getBreakdown(const QueryCost& cost);
};

struct QueryCost {
    double cpu_cost;        // CPU seconds
    double memory_cost;     // GB-seconds
    double io_cost;         // I/O operations
    double network_cost;    // GB transferred
    double total_cost_usd;  // Estimated monetary cost
    std::chrono::milliseconds estimated_duration;
};

// Example output:
// Query Cost Estimate:
//   CPU: 2.5 seconds ($0.001)
//   Memory: 1.2 GB-seconds ($0.0005)
//   I/O: 5000 operations ($0.005)
//   Network: 0.5 GB ($0.002)
//   Total: $0.0085
//   Estimated Duration: 850ms
```

---

### Cost Monitoring Dashboard
**Priority:** Medium
**Target Version:** v1.7.0

Track resource costs per tenant/team/project.

**Features:**
- Cost allocation by tenant
- Budget alerts and quotas
- Cost optimization recommendations
- Showback/chargeback reports

---

### Resource Optimization Advisor
**Priority:** Medium
**Target Version:** v1.7.0

Recommend configuration changes to optimize cost/performance.

**Example Recommendations:**
- "Reduce block cache size by 20% (saves $50/month, minimal impact)"
- "Enable compression (saves 30% storage, +5% CPU)"
- "Migrate cold data to cheaper storage tier (saves $200/month)"

---

## Predictive Analytics

### Capacity Planning
**Priority:** High
**Target Version:** v1.7.0

Predict future resource needs based on growth trends.

**Implementation:**
```cpp
class CapacityPlanner {
public:
    // Forecast resource usage
    CapacityForecast forecast(std::chrono::days horizon);

    // Recommend scaling actions
    std::vector<ScalingRecommendation> recommendScaling(const CapacityForecast& forecast);
};

struct CapacityForecast {
    TimeSeries predicted_qps;
    TimeSeries predicted_storage_gb;
    TimeSeries predicted_memory_gb;
    std::chrono::system_clock::time_point capacity_exhaustion_date;
};

// Example output:
// Capacity Forecast (90 days):
//   QPS: Growing 15% month-over-month
//   Storage: Growing 200 GB/week
//   Memory: Stable at 32 GB
//
// Recommendations:
//   - Add 2 nodes in 45 days (before 80% capacity)
//   - Provision 5 TB additional storage in 60 days
```

---

### Workload Forecasting
**Priority:** Medium
**Target Version:** v1.7.0

Predict future query patterns and load.

**Features:**
- Daily/weekly/seasonal patterns
- Special event detection
- Holiday adjustments
- Gradual trend changes

---

### Failure Prediction
**Priority:** High
**Target Version:** v1.8.0

Predict potential failures before they occur.

**Implementation:**
```cpp
class FailurePredictor {
public:
    // Analyze system health
    HealthScore assessHealth(const SystemMetrics& metrics);

    // Predict failures
    std::vector<FailurePrediction> predictFailures(std::chrono::hours horizon);
};

struct FailurePrediction {
    std::string component;  // "disk-1", "node-3", etc.
    std::string failure_type;  // "disk_full", "oom", "crash"
    double probability;  // 0-1
    std::chrono::system_clock::time_point predicted_time;
    std::vector<std::string> warning_signs;
    std::vector<std::string> prevention_actions;
};

// Example:
// Failure Prediction:
//   Component: disk-1
//   Type: disk_full
//   Probability: 85%
//   ETA: 2024-02-15 14:30:00 (72 hours)
//   Warning Signs:
//     - Disk usage growing 5% per day
//     - Compaction backlog increasing
//   Prevention:
//     - Enable auto-compaction
//     - Archive old data
//     - Add storage capacity
```

---

## Enhanced Visualization

### Interactive Query Plans
**Priority:** Medium
**Target Version:** v1.6.0

Visual query execution plan with interactive exploration.

**Features:**
- Tree/graph visualization of query operators
- Hover for operator details
- Click to drill down into statistics
- Compare plans side-by-side

**Technologies:**
- D3.js for visualization
- React for interactive UI
- WebSocket for real-time updates

---

### Flame Graphs
**Priority:** High
**Target Version:** v1.6.0

Interactive flame graphs for CPU/memory profiling.

**Features:**
- Differential flame graphs (compare profiles)
- Subsecond granularity
- Collapsible stack frames
- Search within flame graph

---

### Distributed Trace Waterfall
**Priority:** Medium
**Target Version:** v1.6.0

Visualize distributed traces as waterfall diagrams.

**Features:**
- Span duration bars
- Critical path highlighting
- Service dependency graph
- Latency breakdown by component

---

### Custom Dashboard Builder
**Priority:** Low
**Target Version:** v1.8.0

Drag-and-drop dashboard builder for custom visualizations.

**Features:**
- Widget library (graph, table, single-stat, heatmap)
- Query builder UI
- Template variables
- Dashboard sharing and export

---

## Integration Enhancements

### Grafana Loki Integration
**Priority:** Medium
**Target Version:** v1.6.0

Stream logs to Grafana Loki for unified log aggregation.

**Implementation:**
```cpp
class LokiLogExporter : public ILogger {
public:
    LokiLogExporter(const LokiConfig& config);

    void log(Level level, const std::string& message) override;

    // Add structured labels
    void addLabel(const std::string& key, const std::string& value);
};

struct LokiConfig {
    std::string endpoint = "http://loki:3100/loki/api/v1/push";
    std::map<std::string, std::string> static_labels;
    int batch_size = 100;
    std::chrono::seconds flush_interval{5};
};

// Usage
LokiLogExporter exporter(config);
exporter.addLabel("component", "query_engine");
exporter.addLabel("environment", "production");
exporter.info("Query executed successfully");
```

---

### DataDog Integration
**Priority:** Low
**Target Version:** v1.7.0

Native DataDog APM and metrics integration.

**Features:**
- DataDog tracer
- StatsD metric export
- Custom service checks
- Log forwarding

---

### New Relic Integration
**Priority:** Low
**Target Version:** v1.7.0

New Relic APM and infrastructure monitoring.

---

### AWS CloudWatch Integration
**Priority:** Medium
**Target Version:** v1.7.0

Export metrics and logs to CloudWatch for AWS deployments.

**Features:**
- CloudWatch Metrics publishing
- CloudWatch Logs integration
- X-Ray tracing
- CloudWatch Alarms

---

### Elastic APM Integration
**Priority:** Low
**Target Version:** v1.8.0

Integration with Elastic Observability stack.

## Test Strategy

- Unit test coverage ≥ 80% across `metrics_collector.cpp`, `tracer.cpp`, `query_profiler.cpp`, `storage_profiler.cpp`
- Prometheus scrape test: exported `/metrics` output must parse without error in Prometheus text format; all registered metric families must appear
- Trace propagation test: W3C `traceparent` header injected by client must be preserved end-to-end through query execution and storage layers
- Adaptive sampling test: at > 10,000 synthetic spans/sec, effective sample rate must drop to ≤ 1% within 5 s of rate increase
- eBPF tracer tests: lifecycle (start/stop), config defaults, stats accumulation, ring-buffer callback, and metrics publishing (unit tests in `tests/test_ebpf_tracer.cpp`)
- Overhead benchmark: metrics collection overhead must be < 1% CPU measured via wall-clock comparison at 1,000 req/s workload

## Performance Targets

- Prometheus `/metrics` scrape response time: < 50 ms p99 with 10,000 active metric series
- Span creation and in-process propagation overhead: < 5 µs per span
- OTLP export latency (async queue to exporter flush): < 5 ms p99 at 1,000 spans/sec
- `QueryProfiler` per-operator timing overhead: < 1 µs per operator boundary
- eBPF perf counter collection cycle: < 0.1% CPU overhead per probe type at 1-second polling interval
- Alertmanager webhook delivery latency: < 2 s p99 from alert trigger to first delivery attempt

## Security / Reliability

- `/metrics` endpoint must require authentication (Bearer token or mTLS) in production mode; unauthenticated access returns HTTP 401
- Trace data must not include query result values or document content; only structural metadata (operator names, durations, record counts) is permitted in span attributes
- OTLP export endpoint URL and credentials must be read from environment variables or a secrets store; never hardcoded or logged
- eBPF program attachment requires `CAP_PERFMON` (Linux ≥ 5.8) or `CAP_SYS_ADMIN`; absence of capability must cause graceful degradation, not a startup failure
- Alertmanager webhook payloads must be retried up to 3 times with exponential backoff; failed deliveries must be counted in `themis_alertmanager_delivery_failures_total`
- Structured logs must be validated at write-time to strip fields matching a configurable PII pattern list before persisting to log storage

---

## See Also

- [README.md](README.md) - Current observability features
- [../../docs/roadmap.md](../../docs/roadmap.md) - Product roadmap
- [../core/README.md](../core/README.md) - Core interfaces

---

## Scientific References

### Prometheus / Metrics Collection

[1] B. Volz, "Prometheus: Monitoring at SoundCloud," *SoundCloud Engineering Blog*, 2012. [Online]. Available: https://developers.soundcloud.com/blog/prometheus-monitoring-at-soundcloud

[2] J. Turnbull, "Monitoring with Prometheus," *Turnbull Press*, 2018, ISBN 978-0-9889405-6-4.

[3] T. Vaillancourt, B. Sherif, and C. Larsen, "Scalable Metric Storage and Query in Practice: Facebook's Gorilla," in *Proc. USENIX OSDI*, 2015, pp. 617–630.

### Distributed Tracing / OpenTelemetry

[4] B. H. Sigelman, L. A. Barroso, M. Burrows, P. Stephenson, M. Plakal, D. Beaver, S. Jaspan, and C. Shanbhag, "Dapper, a Large-Scale Distributed Systems Tracing Infrastructure," *Google Technical Report TR-2010-003*, 2010. [Online]. Available: https://research.google/pubs/pub36356/

[5] Y. Wu, J. Yin, J. Chen, X. Chen, and Z. Li, "MicroRCA: Root Cause Localization of Performance Issues in Microservices," in *Proc. IEEE/IFIP Network Operations and Management Symposium (NOMS)*, 2020, pp. 1–9. doi: 10.1109/NOMS47738.2020.9110353.

[6] OpenTelemetry Authors, "OpenTelemetry Specification," Cloud Native Computing Foundation, 2024. [Online]. Available: https://opentelemetry.io/docs/specs/otel/

### eBPF-Based Kernel Tracing

[7] T. Høiland-Jørgensen, M. B. Brouer, D. Ahern, J. Fastabend, T. Herbert, D. Johansen, and D. Taht, "The eXpress Data Path: Fast Programmable Packet Processing in the Operating System Kernel," in *Proc. ACM CoNEXT*, 2018, pp. 54–66. doi: 10.1145/3281411.3281443.

[8] B. Gregg, "BPF Performance Tools: Linux System and Application Observability," *Addison-Wesley Professional*, 2019, ISBN 0-13-655482-4.

[9] A. Miano, F. Risso, and M. V. Bernal, "Creating Complex Network Services with eBPF: Experience and Lessons Learned," in *Proc. IEEE International Symposium on Local and Metropolitan Area Networks (LANMAN)*, 2018. doi: 10.1109/LANMAN.2018.8475067.

### Continuous Profiling

[10] J. Gutierrez, E. Heisig, and A. Boekholt, "Google-Wide Profiling: A Continuous Profiling Infrastructure for Data Centers," *IEEE Micro*, vol. 30, no. 4, pp. 65–79, Jul. 2010. doi: 10.1109/MM.2010.68.

[11] M. Ayers, J. Leners, and M. Aguilera, "Iago Attacks: Why the System Call API is a Bad Untrusted RPC Interface," in *Proc. ACM ASPLOS*, 2013, pp. 253–264.

### Performance Analysis / Anomaly Detection

[12] A. Dean, S. Zhong, and C. Pang, "Benchmarking Cloud Serving Systems with YCSB," in *Proc. ACM SoCC*, 2010, pp. 143–154. doi: 10.1145/1807128.1807152.

[13] H. Nguyen, Y. Xu, G. Bai, and A. Gu, "FChain: Toward Black-box Online Fault Localization for Cloud Systems," in *Proc. IEEE ICDCS*, 2013, pp. 110–119. doi: 10.1109/ICDCS.2013.26.

[14] M. Ma, S. Lin, J. Xu, and Z. Li, "MS-Rank: Multi-Metric and Self-Adaptive Root Cause Diagnosis for Microservice Applications," in *Proc. IEEE ICWS*, 2019, pp. 60–67. doi: 10.1109/ICWS.2019.00022.

### Distributed Flame Graphs

[15] B. Gregg, "The Flame Graph," *Communications of the ACM*, vol. 59, no. 6, pp. 48–57, Jun. 2016. doi: 10.1145/2912563.

[16] B. Gregg, "FlameGraph: Stack Trace Visualizer," 2016. [Online]. Available: https://github.com/brendangregg/FlameGraph

### Query Profiling / Storage Monitoring

[17] A. Pavlo, G. Angulo, J. Arulraj, H. Lin, J. Lin, L. Ma, P. Menon, T. C. Mowry, M. Perron, I. Quah, S. Santurkar, A. Tomasic, S. Toor, D. Van Aken, Z. Wang, Y. Wu, R. Xian, and T. Zhang, "Self-Driving Database Management Systems," in *Proc. CIDR*, 2017.

[18] D. Lomet, "A Review of Recent Work on Multi-Version Concurrency Control," *ACM SIGMOD Record*, vol. 41, no. 4, pp. 56–63, 2012.

### SLO / Alerting

[19] N. Murphy, B. Beyer, C. Jones, and J. Petoff, "Site Reliability Engineering: How Google Runs Production Systems," *O'Reilly Media*, 2016, ISBN 978-1-4919-2909-4.

[20] A. Iosup, S. Ostermann, M. N. Yigitbasi, R. Prodan, T. Fahringer, and D. Epema, "Performance Analysis of Cloud Computing Services for Many-Tasks Scientific Computing," *IEEE Transactions on Parallel and Distributed Systems*, vol. 22, no. 6, pp. 931–945, 2011. doi: 10.1109/TPDS.2011.66.
