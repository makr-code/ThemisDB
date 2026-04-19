# Observability Module Headers - Future Enhancements

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ../src/observability/README.md · ../src/observability/FUTURE_ENHANCEMENTS.md -->

Planned extensions to ThemisDB observability API headers.

## Scope

- API-level enhancements to `include/observability/` public C++ headers
- OpenTelemetry-compatible span API with W3C Trace Context propagation
- Structured logging interface that avoids heap allocation on the hot path
- Continuous profiling collector API with pprof-format snapshot support
- ML-based anomaly detection hook interface for real-time metric monitoring
- Enhanced histogram and time series APIs for multi-dimensional metrics
- Dynamic configuration interface with hot-reload support

## Design Constraints

- [ ] Tracing API (`EnhancedSpan`, `TraceContext`) must be `noexcept`; span operations never throw
- [ ] Metrics registration (`OTelMetricsProvider`, `MetricsCollector`) is thread-safe; concurrent registration is safe
- [ ] Logging API must not allocate on the hot path; zero-allocation formatting for disabled log levels
- [ ] `AnomalyDetector` is trained offline; `detectPoint()` is `const` and allocation-free on the detection path
- [ ] `ContinuousProfiler` overhead ≤ 1% CPU at default `cpu_sample_rate = 0.01`
- [ ] All observability APIs tolerate being called during shutdown without crashing; no-op fallback after `shutdown()`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `OTelMetricsProvider` | All subsystems | Singleton; thread-safe registration |
| `TraceContext` / `TraceContextPropagator` | Network layer, query engine | W3C propagation |
| `EnhancedSpan` | Query engine, storage layer | `noexcept`, events + links |
| `ContinuousProfiler` | Admin / ops tooling | pprof-format snapshots |
| `AnomalyDetector` / `AnomalyAlerter` | Alerting pipeline | ML-based, const detect path |
| `ConfigManager` | All observability components | Hot-reload, change callbacks |

## Table of Contents

1. [Enhanced Metrics API](#enhanced-metrics-api)
2. [Tracing Enhancements](#tracing-enhancements)
3. [Advanced Profiling](#advanced-profiling)
4. [Analytics and ML](#analytics-and-ml)
5. [New Data Structures](#new-data-structures)
6. [Configuration Extensions](#configuration-extensions)

---

## Enhanced Metrics API

### OpenTelemetry Metrics API
**Priority:** High
**Target Version:** v1.6.0

Add OpenTelemetry-compatible metrics API alongside Prometheus.

**New Headers:**
```cpp
// observability/otel_metrics.h
#pragma once

#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/metrics/meter.h>

namespace themis {
namespace observability {

namespace otel = opentelemetry::metrics;

/**
 * @brief OpenTelemetry metrics provider for ThemisDB
 */
class OTelMetricsProvider {
public:
    static OTelMetricsProvider& getInstance();

    // Initialize with configuration
    void initialize(const OTelMetricsConfig& config);

    // Get meter for component
    std::shared_ptr<otel::Meter> getMeter(const std::string& component);

    // Instrument types
    std::shared_ptr<otel::Counter<int64_t>>
        createCounter(const std::string& name, const std::string& description = "");

    std::shared_ptr<otel::UpDownCounter<int64_t>>
        createUpDownCounter(const std::string& name, const std::string& description = "");

    std::shared_ptr<otel::Histogram<double>>
        createHistogram(const std::string& name, const std::string& description = "");

    std::shared_ptr<otel::ObservableGauge<double>>
        createObservableGauge(const std::string& name,
                            std::function<double()> callback,
                            const std::string& description = "");

    // Shutdown
    void shutdown();
};

struct OTelMetricsConfig {
    std::string service_name;
    std::string service_version;
    std::string endpoint;  // OTLP endpoint
    std::string protocol = "grpc";  // or "http/protobuf"
    std::chrono::seconds export_interval{10};
    std::map<std::string, std::string> resource_attributes;
};

} // namespace observability
} // namespace themis
```

**Usage:**
```cpp
#include "observability/otel_metrics.h"

auto& provider = OTelMetricsProvider::getInstance();
auto meter = provider.getMeter("query_engine");

auto query_counter = meter->CreateCounter("queries_total");
query_counter->Add(1, {{"type", "SELECT"}, {"status", "success"}});

auto latency_histogram = meter->CreateHistogram("query_latency_ms");
latency_histogram->Record(15.5, {{"type", "SELECT"}});
```

---

### Exemplars Support
**Priority:** Medium
**Target Version:** v1.6.0

Link metrics to traces via exemplars.

**Header Extension:**
```cpp
// observability/metrics_collector.h (additions)
class MetricsCollector {
public:
    // ... existing methods ...

    /**
     * @brief Record histogram value with exemplar
     * @param name Metric name
     * @param value Value to record
     * @param trace_id Associated trace ID
     * @param span_id Associated span ID
     * @param labels Metric labels
     */
    void observeHistogramWithExemplar(
        const std::string& name,
        double value,
        const std::string& trace_id,
        const std::string& span_id,
        const std::map<std::string, std::string>& labels = {}
    );

    /**
     * @brief Get Prometheus metrics with exemplars
     */
    std::string getPrometheusMetricsWithExemplars() const;
};
```

---

### Multi-Dimensional Time Series
**Priority:** Medium
**Target Version:** v1.7.0

Support for complex metric dimensions and aggregations.

**New Header:**
```cpp
// observability/timeseries.h
#pragma once

#include <vector>
#include <chrono>
#include <optional>

namespace themis {
namespace observability {

/**
 * @brief Time series data point
 */
struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    double value;
    std::map<std::string, std::string> labels;
};

/**
 * @brief Time series with metadata
 */
class TimeSeries {
public:
    TimeSeries(const std::string& metric_name);

    void addPoint(const TimeSeriesPoint& point);
    void addPoints(const std::vector<TimeSeriesPoint>& points);

    // Query operations
    std::optional<TimeSeriesPoint> getPointAt(std::chrono::system_clock::time_point time) const;
    std::vector<TimeSeriesPoint> getRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;

    // Aggregations
    double mean() const;
    double median() const;
    double percentile(double p) const;
    double sum() const;
    double min() const;
    double max() const;
    double stddev() const;

    // Statistical operations
    TimeSeries movingAverage(std::chrono::seconds window) const;
    TimeSeries exponentialSmoothing(double alpha) const;
    TimeSeries derivative() const;
    TimeSeries rate(std::chrono::seconds interval) const;

    // Comparison
    TimeSeries operator+(const TimeSeries& other) const;
    TimeSeries operator-(const TimeSeries& other) const;
    TimeSeries operator*(double scalar) const;

    // Export
    json toJSON() const;
    std::string toCSV() const;

private:
    std::string metric_name_;
    std::vector<TimeSeriesPoint> points_;
};

/**
 * @brief Time series aggregator
 */
class TimeSeriesAggregator {
public:
    // Aggregate multiple series
    TimeSeries sum(const std::vector<TimeSeries>& series) const;
    TimeSeries mean(const std::vector<TimeSeries>& series) const;
    TimeSeries max(const std::vector<TimeSeries>& series) const;
    TimeSeries min(const std::vector<TimeSeries>& series) const;

    // Group by labels
    std::map<std::string, TimeSeries> groupBy(
        const std::vector<TimeSeries>& series,
        const std::string& label_key) const;

    // Downsample
    TimeSeries downsample(const TimeSeries& series,
                         std::chrono::seconds interval,
                         AggregationType agg) const;
};

} // namespace observability
} // namespace themis
```

---

## Tracing Enhancements

### W3C Trace Context
**Priority:** High
**Target Version:** v1.6.0

Full W3C Trace Context support for distributed tracing.

**New Header:**
```cpp
// observability/trace_context.h
#pragma once

#include <string>
#include <map>
#include <optional>

namespace themis {
namespace observability {

/**
 * @brief W3C Trace Context (https://www.w3.org/TR/trace-context/)
 */
struct TraceContext {
    // traceparent: version-trace_id-parent_id-trace_flags
    std::string trace_id;      // 32 hex chars (128 bits)
    std::string parent_id;     // 16 hex chars (64 bits)
    std::string trace_flags;   // 2 hex chars (8 bits)
    std::string version = "00";

    // tracestate: vendor-specific key=value pairs
    std::map<std::string, std::string> trace_state;

    /**
     * @brief Parse from HTTP headers
     */
    static std::optional<TraceContext> fromHeaders(
        const std::map<std::string, std::string>& headers);

    /**
     * @brief Convert to HTTP headers
     */
    std::map<std::string, std::string> toHeaders() const;

    /**
     * @brief Create new trace context
     */
    static TraceContext create();

    /**
     * @brief Create child span context
     */
    TraceContext createChild() const;

    /**
     * @brief Check if trace is sampled
     */
    bool isSampled() const;

    /**
     * @brief Set sampled flag
     */
    void setSampled(bool sampled);

    /**
     * @brief Validate trace context
     */
    bool isValid() const;
};

/**
 * @brief Trace context propagator
 */
class TraceContextPropagator {
public:
    // Extract context from carrier
    std::optional<TraceContext> extract(
        const std::map<std::string, std::string>& carrier) const;

    // Inject context into carrier
    void inject(const TraceContext& context,
               std::map<std::string, std::string>& carrier) const;
};

} // namespace observability
} // namespace themis
```

---

### Span Events
**Priority:** Medium
**Target Version:** v1.6.0

Rich span context with events and annotations.

**Header Extension:**
```cpp
// observability/span_events.h
#pragma once

#include <string>
#include <chrono>
#include <map>
#include "core/concerns/i_tracer.h"

namespace themis {
namespace observability {

/**
 * @brief Span event
 */
struct SpanEvent {
    std::string name;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> attributes;

    json toJSON() const;
};

/**
 * @brief Span link (relationship to other spans)
 */
struct SpanLink {
    std::string trace_id;
    std::string span_id;
    std::map<std::string, std::string> attributes;

    json toJSON() const;
};

/**
 * @brief Enhanced span with events and links
 */
class EnhancedSpan : public core::concerns::ITracer::ISpan {
public:
    // ... existing methods ...

    /**
     * @brief Add event to span
     */
    void addEvent(const std::string& name,
                 const std::map<std::string, std::string>& attributes = {});

    void addEvent(const SpanEvent& event);

    /**
     * @brief Add link to related span
     */
    void addLink(const std::string& trace_id,
                const std::string& span_id,
                const std::map<std::string, std::string>& attributes = {});

    void addLink(const SpanLink& link);

    /**
     * @brief Get all events
     */
    std::vector<SpanEvent> getEvents() const;

    /**
     * @brief Get all links
     */
    std::vector<SpanLink> getLinks() const;

    /**
     * @brief Export span to JSON
     */
    json toJSON() const;
};

} // namespace observability
} // namespace themis
```

---

## Advanced Profiling

### Continuous Profiling
**Priority:** Medium
**Target Version:** v1.6.0

Always-on profiling with minimal overhead.

**New Header:**
```cpp
// observability/continuous_profiler.h
#pragma once

#include <string>
#include <chrono>
#include <functional>

namespace themis {
namespace observability {

/**
 * @brief Profile types
 */
enum class ProfileType {
    CPU,
    HEAP,
    MUTEX,
    GOROUTINE,
    BLOCK
};

/**
 * @brief Profile snapshot
 */
struct ProfileSnapshot {
    ProfileType type;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::seconds duration;
    std::vector<uint8_t> data;  // pprof format

    // Save to file
    void saveToFile(const std::string& filename) const;

    // Load from file
    static ProfileSnapshot loadFromFile(const std::string& filename);
};

/**
 * @brief Continuous profiler configuration
 */
struct ContinuousProfilerConfig {
    bool enabled = false;
    double cpu_sample_rate = 0.01;  // 1% overhead
    std::chrono::seconds snapshot_interval{60};
    size_t max_snapshots_retained = 1440;  // 24 hours at 1/min
    std::string output_dir = "/var/lib/themisdb/profiles";
    bool enable_cpu_profiling = true;
    bool enable_heap_profiling = true;
    bool enable_mutex_profiling = false;
    bool enable_block_profiling = false;
};

/**
 * @brief Continuous profiler
 */
class ContinuousProfiler {
public:
    explicit ContinuousProfiler(const ContinuousProfilerConfig& config);
    ~ContinuousProfiler();

    // Start profiling
    void start();

    // Stop profiling
    void stop();

    // Get snapshot
    ProfileSnapshot snapshot(ProfileType type);

    // Get all snapshots
    std::vector<ProfileSnapshot> getSnapshots(ProfileType type,
                                             std::chrono::system_clock::time_point start,
                                             std::chrono::system_clock::time_point end);

    // Compare profiles
    ProfileDiff compare(const ProfileSnapshot& baseline,
                       const ProfileSnapshot& current);

    // Register callback for anomalies
    void registerAnomalyCallback(
        std::function<void(const ProfileSnapshot&, const std::string& anomaly)> callback);

    // Enable/disable
    void enable();
    void disable();
    bool isEnabled() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Profile comparison result
 */
struct ProfileDiff {
    double cpu_regression_percent;
    double memory_regression_percent;
    std::vector<std::string> new_hotspots;
    std::vector<std::string> removed_hotspots;
    std::vector<std::string> changed_hotspots;

    json toJSON() const;
};

} // namespace observability
} // namespace themis
```

---

### Memory Leak Detection
**Priority:** High
**Target Version:** v1.6.0

Automated memory leak detection and analysis.

**New Header:**
```cpp
// observability/memory_leak_detector.h
#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace themis {
namespace observability {

/**
 * @brief Memory leak candidate
 */
struct LeakCandidate {
    std::string allocation_site;     // File:line
    size_t total_bytes;
    size_t num_allocations;
    double growth_rate_mb_per_hour;
    std::vector<std::string> stack_traces;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;

    json toJSON() const;
};

/**
 * @brief Memory leak report
 */
struct LeakReport {
    std::vector<LeakCandidate> candidates;
    size_t total_leaked_bytes;
    std::chrono::system_clock::time_point analysis_time;
    std::string summary;
    std::vector<std::string> recommendations;

    json toJSON() const;
    std::string toHumanReadable() const;
};

/**
 * @brief Memory leak detector configuration
 */
struct LeakDetectorConfig {
    bool enabled = false;
    std::chrono::minutes monitoring_interval{5};
    double leak_threshold_mb_per_hour = 10.0;
    size_t min_samples_for_detection = 12;  // 1 hour at 5 min intervals
    std::string output_dir = "/var/log/themisdb/leaks";
};

/**
 * @brief Memory leak detector
 */
class MemoryLeakDetector {
public:
    explicit MemoryLeakDetector(const LeakDetectorConfig& config);
    ~MemoryLeakDetector();

    // Start monitoring
    void startMonitoring();

    // Stop monitoring
    void stopMonitoring();

    // Analyze current state
    std::vector<LeakCandidate> detectLeaks();

    // Generate detailed report
    LeakReport generateReport();

    // Register callback for leak detection
    void registerLeakCallback(
        std::function<void(const LeakCandidate&)> callback);

    // Manual heap analysis
    void captureHeapSnapshot();

    // Compare heap snapshots
    LeakReport compareSnapshots(const std::string& snapshot1,
                               const std::string& snapshot2);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
```

---

## Analytics and ML

### Anomaly Detector
**Priority:** High
**Target Version:** v1.7.0

ML-based anomaly detection for metrics.

**New Header:**
```cpp
// observability/anomaly_detector.h
#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "observability/timeseries.h"

namespace themis {
namespace observability {

/**
 * @brief Anomaly detection method
 */
enum class AnomalyMethod {
    ZSCORE,              // Statistical (z-score)
    MODIFIED_ZSCORE,     // Robust z-score (MAD)
    IQR,                 // Interquartile range
    ISOLATION_FOREST,    // ML-based
    SEASONAL_DECOMP,     // Seasonal decomposition
    PROPHET,             // Time series forecasting
    ARIMA               // AutoRegressive model
};

/**
 * @brief Detected anomaly
 */
struct Anomaly {
    std::string metric_name;
    std::chrono::system_clock::time_point timestamp;
    double actual_value;
    double expected_value;
    double deviation_sigma;
    double confidence_score;  // 0-1
    std::string severity;     // "low", "medium", "high", "critical"
    std::vector<std::string> contributing_factors;
    std::string explanation;

    json toJSON() const;
};

/**
 * @brief Anomaly detector configuration
 */
struct AnomalyDetectorConfig {
    AnomalyMethod method = AnomalyMethod::SEASONAL_DECOMP;
    double threshold_sigma = 3.0;
    size_t min_samples = 100;
    std::chrono::seconds training_window{86400};  // 24 hours
    bool enable_seasonal_adjustment = true;
    bool enable_trend_removal = true;
};

/**
 * @brief Machine learning-based anomaly detector
 */
class AnomalyDetector {
public:
    explicit AnomalyDetector(const AnomalyDetectorConfig& config);
    ~AnomalyDetector();

    // Train on historical data
    void train(const TimeSeries& training_data);

    // Detect anomalies
    std::vector<Anomaly> detect(const TimeSeries& data);

    // Real-time detection (single point)
    std::optional<Anomaly> detectPoint(double value,
                                      std::chrono::system_clock::time_point timestamp);

    // Forecast future values
    TimeSeries forecast(std::chrono::hours horizon);

    // Explain anomaly
    std::string explainAnomaly(const Anomaly& anomaly);

    // Update model with new data
    void update(const TimeSeries& new_data);

    // Save/load model
    void saveModel(const std::string& filename);
    void loadModel(const std::string& filename);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Anomaly alerter (combines detection + alerting)
 */
class AnomalyAlerter {
public:
    AnomalyAlerter(AnomalyDetector& detector, Alertmanager& alertmanager);

    // Monitor metric and auto-alert
    void monitorMetric(const std::string& metric_name,
                      std::chrono::seconds check_interval = std::chrono::seconds(60));

    // Stop monitoring
    void stopMonitoring(const std::string& metric_name);

    // Configure alert rules
    void setAlertRule(const std::string& metric_name,
                     const AlertRule& rule);

private:
    AnomalyDetector& detector_;
    Alertmanager& alertmanager_;
    std::map<std::string, AlertRule> rules_;
};

struct AlertRule {
    double min_confidence = 0.8;
    std::string min_severity = "medium";
    bool auto_resolve = true;
    std::chrono::minutes silence_duration{60};
};

} // namespace observability
} // namespace themis
```

---

### Query Recommendation Engine
**Priority:** Medium
**Target Version:** v1.7.0

ML-powered query optimization recommendations.

**New Header:**
```cpp
// observability/query_recommender.h
#pragma once

#include <string>
#include <vector>
#include "observability/query_profiler.h"

namespace themis {
namespace observability {

/**
 * @brief Recommendation type
 */
enum class RecommendationType {
    CREATE_INDEX,
    REWRITE_QUERY,
    ADJUST_CACHE,
    REPARTITION,
    CHANGE_SCHEMA,
    ADD_MATERIALIZED_VIEW
};

/**
 * @brief Query optimization recommendation
 */
struct QueryRecommendation {
    RecommendationType type;
    std::string title;
    std::string description;
    std::string sql_statement;  // DDL to apply
    double estimated_improvement_percent;
    std::string confidence;  // "low", "medium", "high"
    std::vector<std::string> affected_queries;
    std::vector<std::string> trade_offs;

    json toJSON() const;
};

/**
 * @brief Impact analysis result
 */
struct ImpactAnalysis {
    double latency_improvement_percent;
    double throughput_improvement_percent;
    size_t storage_cost_increase_bytes;
    double cpu_cost_increase_percent;
    std::string risk_level;  // "low", "medium", "high"

    json toJSON() const;
};

/**
 * @brief Query recommendation engine
 */
class QueryRecommendationEngine {
public:
    QueryRecommendationEngine();
    ~QueryRecommendationEngine();

    // Analyze workload
    void analyzeWorkload(const std::vector<QueryProfile>& profiles);

    // Generate recommendations
    std::vector<QueryRecommendation> generateRecommendations();

    // Estimate impact
    ImpactAnalysis estimateImpact(const QueryRecommendation& rec);

    // Apply recommendation (execute SQL)
    Result<void> applyRecommendation(const QueryRecommendation& rec);

    // Revert recommendation
    Result<void> revertRecommendation(const QueryRecommendation& rec);

    // Track effectiveness
    void trackEffectiveness(const QueryRecommendation& rec,
                           const std::vector<QueryProfile>& after_profiles);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
```

---

## New Data Structures

### Enhanced Histogram
**Priority:** Medium
**Target Version:** v1.6.0

More sophisticated histogram implementation.

**New Header:**
```cpp
// observability/histogram.h
#pragma once

#include <vector>
#include <map>

namespace themis {
namespace observability {

/**
 * @brief Histogram bucket
 */
struct HistogramBucket {
    double upper_bound;
    uint64_t count;
};

/**
 * @brief Advanced histogram with configurable buckets
 */
class Histogram {
public:
    // Predefined bucket schemes
    static std::vector<double> exponentialBuckets(double start, double factor, size_t count);
    static std::vector<double> linearBuckets(double start, double width, size_t count);

    explicit Histogram(const std::vector<double>& buckets);

    // Observe value
    void observe(double value);
    void observeMultiple(double value, uint64_t count);

    // Query
    uint64_t count() const;
    double sum() const;
    double mean() const;
    double percentile(double p) const;
    double min() const;
    double max() const;

    // Get buckets
    std::vector<HistogramBucket> getBuckets() const;

    // Reset
    void reset();

    // Merge
    void merge(const Histogram& other);

    // Export
    std::string toPrometheus(const std::string& name,
                            const std::map<std::string, std::string>& labels) const;
    json toJSON() const;

private:
    std::vector<double> buckets_;
    std::vector<uint64_t> counts_;
    uint64_t total_count_ = 0;
    double sum_ = 0.0;
    double min_ = std::numeric_limits<double>::max();
    double max_ = std::numeric_limits<double>::lowest();
};

/**
 * @brief Exponential histogram (for wide value ranges)
 */
class ExponentialHistogram {
public:
    explicit ExponentialHistogram(double scale = 2.0, size_t max_buckets = 160);

    void observe(double value);
    double percentile(double p) const;

    json toJSON() const;

private:
    double scale_;
    size_t max_buckets_;
    std::map<int, uint64_t> positive_buckets_;
    std::map<int, uint64_t> negative_buckets_;
    uint64_t zero_count_ = 0;
};

} // namespace observability
} // namespace themis
```

---

## Configuration Extensions

### Dynamic Configuration
**Priority:** Medium
**Target Version:** v1.6.0

Hot-reloadable observability configuration.

**New Header:**
```cpp
// observability/config_manager.h
#pragma once

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

/**
 * @brief Observability configuration
 */
struct ObservabilityConfig {
    // Metrics
    bool metrics_enabled = true;
    std::string metrics_endpoint = ":9090/metrics";

    // Query profiler
    QueryProfilerConfig query_profiler;

    // Storage profiler
    StorageProfilerConfig storage_profiler;

    // Performance analyzer
    PerformanceAnalyzerConfig performance_analyzer;

    // Alertmanager
    AlertmanagerConfig alertmanager;

    // Tracing
    bool tracing_enabled = false;
    std::string tracing_endpoint;
    double tracing_sample_rate = 0.1;

    // Convert to/from JSON
    json toJSON() const;
    static ObservabilityConfig fromJSON(const json& j);

    // Validate configuration
    std::vector<std::string> validate() const;
};

/**
 * @brief Configuration manager with hot-reload
 */
class ConfigManager {
public:
    static ConfigManager& getInstance();

    // Load configuration
    void loadFromFile(const std::string& filename);
    void loadFromJSON(const json& config);
    void loadFromEnv();

    // Get configuration
    ObservabilityConfig getConfig() const;

    // Watch for changes
    void watchFile(const std::string& filename,
                  std::chrono::seconds check_interval = std::chrono::seconds(30));

    // Register callback for config changes
    void registerCallback(std::function<void(const ObservabilityConfig&)> callback);

    // Apply configuration
    void applyConfig(const ObservabilityConfig& config);

    // Reload configuration
    void reload();

private:
    ConfigManager() = default;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
```

---

## Test Strategy

- `noexcept` compliance tests: all span and trace operations called under exception-prone conditions must not propagate exceptions
- Thread-safety tests: concurrent `OTelMetricsProvider` registration from 16 threads must not race or deadlock
- Zero-allocation tests using instrumented allocators: disabled log-level paths must record zero heap allocations
- `AnomalyDetector` accuracy tests: detection rate ≥ 95% on synthetic anomaly datasets with known ground truth
- Hot-reload tests for `ConfigManager`: config changes applied within one `check_interval` without service restart
- Header-only compilation tests: each planned header compiles in isolation without `src/` includes

## Performance Targets

- `EnhancedSpan` creation overhead: ≤ 5 µs including attribute storage
- `MetricsCollector` / `OTelMetricsProvider` data point recording: ≤ 500 ns per observation
- `TraceContextPropagator::extract` from HTTP headers: ≤ 1 µs
- `AnomalyDetector::detectPoint` (inference path): ≤ 10 µs per data point
- `ContinuousProfiler` CPU overhead at default sample rate: ≤ 1% of total process CPU
- `ConfigManager::getConfig` read path: ≤ 100 ns (lock-free atomic snapshot)

## Security / Reliability

- Trace data never includes PII fields (user IDs, passwords, tokens) without explicit sanitization via a registered `SanitizationFilter`
- Log levels are configurable at runtime via `ConfigManager`; sensitive fields automatically redacted at `INFO` and below
- `MemoryLeakDetector` output is written only to `output_dir`; no network exfiltration of heap data
- `AnomalyDetector` model files are validated for integrity before `loadModel()` proceeds
- `ContinuousProfiler` snapshot files are written with permissions `0600`; readable only by the ThemisDB process owner
- All observability APIs tolerate `nullptr` callback arguments gracefully; a no-op is registered in place of null callbacks

## See Also

- [README.md](README.md) - Current API documentation
- [../../../src/observability/FUTURE_ENHANCEMENTS.md](../../../src/observability/FUTURE_ENHANCEMENTS.md) - Implementation roadmap
- [../../core/concerns/](../../core/concerns/) - Core interfaces
