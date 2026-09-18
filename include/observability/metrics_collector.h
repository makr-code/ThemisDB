/**
 * @file metrics_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

namespace themis {
namespace observability {

class LatencyTracker;

/**
 * @brief An exemplar links a specific trace context to a metric observation.
 *
 * Exemplars are stored on histogram metrics and emitted in the Prometheus
 * OpenMetrics text format alongside the histogram summary output:
 *
 * ```
 * # TYPE some_latency_ms summary
 * some_latency_ms{quantile="0.99"} 42.0 # {traceID="3e561c74cee40c12"} 42.0 1712000000.000
 * ```
 *
 * At most one exemplar is retained per metric series (last-write-wins).
 *
 * @see observeHistogramWithExemplar()
 */
struct Exemplar {
    /// W3C-compatible trace ID string (e.g. 16-hex-char span ID or 32-hex-char trace ID).
    std::string trace_id = {};
    /// The observed metric value that this exemplar is associated with.
    double value{0.0};
    /// Wall-clock timestamp of the observation.
    std::chrono::system_clock::time_point timestamp;

    Exemplar() : timestamp(std::chrono::system_clock::now()) {}
    Exemplar(std::string tid, double v)
        : trace_id(std::move(tid)), value(v),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * Central metrics collector for ThemisDB observability.
 * 
 * Aggregates metrics from all subsystems (TSStore, Query Engine, Sharding, Cache, etc.)
 * and exposes them in Prometheus text format via /metrics endpoint.
 * 
 * Thread-Safety:
 * - All public methods are thread-safe
 * - Read operations (getPrometheusMetrics, getCardinalityLimit) use
 *   std::shared_lock, allowing multiple concurrent readers
 * - Write operations (record*, increment*, setGauge, observeHistogram, reset)
 *   use std::unique_lock for exclusive access
 * - Counter and gauge operations are protected during map insertion
 * - Histogram operations are fully synchronized
 * - Safe for concurrent access from multiple threads
 * 
 * Thread-safe singleton pattern for global access.
 */
class MetricsCollector {
public:
    /**
     * @brief TBD: Describe getInstance.
     * @return Return value.
     */
    static MetricsCollector& getInstance();
    
    // Prevent copying
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
    /**
     * @brief TSStore Metrics
     * @param[in] metric Input parameter.
     * @param[in] batch_size Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordTSStoreWrite(const std::string& metric, size_t batch_size, double latency_ms);
    /**
     * @brief TBD: Describe recordTSStoreQuery.
     * @param[in] metric Input parameter.
     * @param[in] result_count Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordTSStoreQuery(const std::string& metric, size_t result_count, double latency_ms);
    /**
     * @brief TBD: Describe recordTSStoreAggregate.
     * @param[in] metric Input parameter.
     * @param[in] point_count Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordTSStoreAggregate(const std::string& metric, size_t point_count, double latency_ms);
    /**
     * @brief TBD: Describe recordTSStoreCompression.
     * @param[in] compression_type Input parameter.
     * @param[in] ratio Input parameter.
     */
    void recordTSStoreCompression(const std::string& compression_type, double ratio);
    
    /**
     * @brief Query Engine Metrics
     * @param[in] query_type Input parameter.
     * @param[in] latency_ms Input parameter.
     * @param[in] result_count Input parameter.
     */
    void recordQuery(const std::string& query_type, double latency_ms, size_t result_count);
    /**
     * @brief TBD: Describe recordIndexScan.
     * @param[in] index_type Input parameter.
     * @param[in] keys_scanned Input parameter.
     */
    void recordIndexScan(const std::string& index_type, size_t keys_scanned);
    /**
     * @brief TBD: Describe recordFullScan.
     * @param[in] table Input parameter.
     * @param[in] keys_scanned Input parameter.
     */
    void recordFullScan(const std::string& table, size_t keys_scanned);
    
    /**
     * @brief Cache Metrics
     * @param[in] cache_type Input parameter.
     */
    void recordCacheHit(const std::string& cache_type);
    /**
     * @brief TBD: Describe recordCacheMiss.
     * @param[in] cache_type Input parameter.
     */
    void recordCacheMiss(const std::string& cache_type);
    /**
     * @brief TBD: Describe recordCacheEviction.
     * @param[in] cache_type Input parameter.
     */
    void recordCacheEviction(const std::string& cache_type);
    
    /**
     * @brief Sharding Metrics
     * @param[in] shard_id Input parameter.
     * @param[in] operation Input parameter.
     */
    void recordShardRequest(const std::string& shard_id, const std::string& operation);
    /**
     * @brief TBD: Describe recordShardLatency.
     * @param[in] shard_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordShardLatency(const std::string& shard_id, double latency_ms);
    /**
     * @brief TBD: Describe recordRebalanceProgress.
     * @param[in] operation_id Input parameter.
     * @param[in] records Input parameter.
     * @param[in] percent Input parameter.
     */
    void recordRebalanceProgress(const std::string& operation_id, int64_t records, double percent);
    
    /**
     * @brief Content Processing Metrics
     * @param[in] mime_type Input parameter.
     * @param[in] size_bytes Input parameter.
     */
    void recordContentImport(const std::string& mime_type, size_t size_bytes);
    /**
     * @brief TBD: Describe recordChunkCreation.
     * @param[in] chunk_count Input parameter.
     */
    void recordChunkCreation(size_t chunk_count);
    /**
     * @brief TBD: Describe recordEmbeddingGeneration.
     * @param[in] count Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordEmbeddingGeneration(size_t count, double latency_ms);
    
    /**
     * @brief Security Metrics
     * @param[in] success Input parameter.
     */
    void recordAuthAttempt(bool success);
    /**
     * @brief TBD: Describe recordPolicyEvaluation.
     * @param[in] allowed Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPolicyEvaluation(bool allowed, double latency_ms);
    /**
     * @brief TBD: Describe recordEncryptionOperation.
     * @param[in] operation Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordEncryptionOperation(const std::string& operation, double latency_ms);
    
    /**
     * @brief System Metrics
     * @param[in] bytes Input parameter.
     */
    void recordMemoryUsage(size_t bytes);
    /**
     * @brief TBD: Describe recordCPUUsage.
     * @param[in] percent Input parameter.
     */
    void recordCPUUsage(double percent);
    /**
     * @brief TBD: Describe recordDiskIOps.
     * @param[in] read_ops Input parameter.
     * @param[in] write_ops Input parameter.
     */
    void recordDiskIOps(size_t read_ops, size_t write_ops);
    
    /**
     * @brief Tracing Metrics
     * @param[in] span_name Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordSpanDuration(const std::string& span_name, double duration_ms);
    /**
     * @brief TBD: Describe recordActiveSpans.
     * @param[in] count Input parameter.
     */
    void recordActiveSpans(int64_t count);
    /**
     * @brief TBD: Describe recordTotalSpans.
     * @param[in] count Input parameter.
     */
    void recordTotalSpans(int64_t count);
    
    /**
     * @brief Get metrics in Prometheus text format
     * @return Return value.
     */
    std::string getPrometheusMetrics() const;
    
    /**
     * @brief Reset all collected metric state to defaults.
     *
     * Clears counters, gauges, histograms, cardinality tracking, dropped-series
     * counters, and restores the cardinality limit to the default disabled
     * state (`0`). Intended primarily for tests and process-wide reinitialization.
     */
    void reset();

    // ===== Cardinality control =====
    /**
     * @brief Set the maximum number of unique label-set combinations per metric
     *        name.  When the limit is reached, new series are silently dropped
     *        and a counter is incremented.  Set to 0 to disable.
     * @param[in] limit Input parameter.
     */
    void setCardinalityLimit(size_t limit);
    /**
     * @brief TBD: Describe getCardinalityLimit.
     * @return Return value.
     */
    size_t getCardinalityLimit() const;

     * @brief TBD: Describe getDroppedSeriesCount.
     * @return Return value.
    /** Returns the number of metric observations dropped due to cardinality overflow. */
    int64_t getDroppedSeriesCount() const;

    // ===== Exporter health =====
     * @brief TBD: Describe recordExporterFailure.
     * @param[in] exporter_name Input parameter.
    /** Record a transient failure contacting an exporter (OTLP, Pushgateway, etc.). */
    void recordExporterFailure(const std::string& exporter_name);
     * @brief TBD: Describe recordExporterRecovery.
     * @param[in] exporter_name Input parameter.
    /** Record that an exporter has recovered after previous failures. */
    void recordExporterRecovery(const std::string& exporter_name);
     * @brief TBD: Describe recordMalformedTelemetry.
     * @param[in] metric_name Input parameter.
     * @param[in] reason Input parameter.
    /** Record an exporter rejection caused by malformed telemetry input. */
    void recordMalformedTelemetry(const std::string& metric_name,
                                  const std::string& reason);

    /**
     * @brief Snapshot exporter incident counters for a single exporter.
     *
     * The current observability hardening slice tracks exporter reachability
     * incidents only. Malformed telemetry rejections remain metric-scoped and
     * are surfaced via `malformed_telemetry_rejections_total{metric=...,reason=...}`
     * instead of being attributed to an exporter.
     *
     * @param exporter_name Logical exporter name such as `otlp` or `prometheus`.
     * @return Current failure and recovery counters for the exporter. The
     *         malformed-rejection field remains zero until exporter-specific
     *         attribution is implemented as a separate contract change.
     */
    struct ExporterIncidentStats {
        std::int64_t failures{0};
        std::int64_t recoveries{0};
        std::int64_t malformed_rejections{0};
    };
    [[nodiscard]] ExporterIncidentStats getExporterIncidentStats(
        const std::string& exporter_name) const;

    // ===== Generic metric recording (used by adapters) =====

    /**
     * @brief Add @p delta to a named counter.
     *
     * Suitable for use by adapter layers that need a generic counter
     * with an arbitrary increment step (e.g. PrometheusMetricsAdapter).
     */
    void addCounter(const std::string& name, int64_t delta,
                    const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Set a named gauge to an absolute value.
     *
     * Public interface for adapter layers; internally delegates to the
     * same implementation used by domain-specific record*() methods.
     */
    void setGauge(const std::string& name, double value,
                  const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Add @p delta to a named gauge (positive or negative).
     *
     * Performs a thread-safe read-modify-write on the gauge.
     */
    void modifyGauge(const std::string& name, double delta,
                     const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Record an observation in a named histogram.
     *
     * Public interface for adapter layers.
     */
    void observeHistogram(const std::string& name, double value,
                          const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Record an observation in a named histogram and attach an exemplar.
     *
     * Works identically to @c observeHistogram but additionally stores an
     * @c Exemplar that links the observation to a specific trace context.
     * Only the most recent exemplar per metric series is retained.
     *
     * The exemplar is emitted alongside the Prometheus summary output:
     * @code
     * some_latency_ms{quantile="0.99"} 42.0 # {traceID="3e561c74cee40c12"} 42.0 1712000000.000
     * @endcode
     *
     * @param name     Metric name.
     * @param value    Observed value.
     * @param exemplar Exemplar carrying the trace ID and observation context.
     * @param labels   Optional label set.
     */
    void observeHistogramWithExemplar(const std::string& name, double value,
                                      const Exemplar& exemplar,
                                      const std::map<std::string, std::string>& labels = {});

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    
    friend class LatencyTracker;
    mutable std::shared_mutex mutex_;
    
    // Cardinality limit (0 = disabled)
    size_t cardinality_limit_ = 0;
    // Per-metric-name series tracking for cardinality enforcement
    std::map<std::string, size_t> series_count_per_metric_;
    // Total observations dropped due to cardinality overflow
    std::atomic<int64_t> dropped_series_{0};

    // Counters (monotonically increasing)
    std::map<std::string, std::atomic<int64_t>> counters_;
    
    // Gauges (can go up/down)
    std::map<std::string, std::atomic<double>> gauges_;
    
    // Histograms (track distribution)
    struct Histogram {
        std::vector<double> values;
        std::chrono::steady_clock::time_point last_reset;
        size_t max_samples = 1000;
        /// Most recent exemplar attached to this histogram (optional).
        /// Empty trace_id means no exemplar has been recorded yet.
        Exemplar latest_exemplar;
        
        /**
         * @brief TBD: Describe observe.
         * @param[in] value Input parameter.
         */
        void observe(double value);
        /**
         * @brief TBD: Describe reset.
         */
        void reset();
        /**
         * @brief TBD: Describe percentile.
         * @param[in] p Input parameter.
         * @return Return value.
         */
        double percentile(double p) const;
        /**
         * @brief TBD: Describe mean.
         * @return Return value.
         */
        double mean() const;
    };
    std::map<std::string, std::shared_ptr<Histogram>> histograms_;
    
    // Internal helper: increment a counter by exactly 1 (called by domain-specific record*() methods)
    void incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Check whether a new series (name + labels combination) is allowed
     *        by the cardinality limit.  Returns true if the observation should
     *        proceed, false if it should be dropped.
     *
     * Caller MUST hold mutex_ exclusively (unique_lock) before calling this.
     * @param[in] name Input parameter.
     * @param[in] key Input parameter.
     * @return True on success.
     */
    bool checkCardinality(const std::string& name, const std::string& key);
    
    std::string makeKey(const std::string& name, const std::map<std::string, std::string>& labels) const;
    std::string formatLabels(const std::map<std::string, std::string>& labels) const;
    /**
     * @brief TBD: Describe formatMetricLine.
     * @param[in] name Input parameter.
     * @param[in] labels Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::string formatMetricLine(const std::string& name, const std::string& labels, double value) const;
    /**
     * @brief Format an exemplar for Prometheus OpenMetrics output.
     * @param[in] exemplar Input parameter.
     * @return Return value.
     * @details Returns an empty string if the exemplar has no trace_id.
     */
    static std::string formatExemplar(const Exemplar& exemplar);
    static bool areLabelsValid(const std::map<std::string, std::string>& labels,
                               std::string* failure_reason = nullptr);
};

/**
 * RAII helper for automatic latency tracking
 * 
 * Usage:
 *   {
 *       LatencyTracker tracker("tsstore_query", {{"metric", "cpu_usage"}});
 *       // ... work ...
 *   } // latency automatically recorded on destruction
 */
class LatencyTracker {
public:
    LatencyTracker(const std::string& metric_name, 
                   const std::map<std::string, std::string>& labels = {});
    ~LatencyTracker();
    
    /**
     * @brief Get elapsed time without ending tracker
     * @return Return value.
     */
    double elapsedMs() const;

private:
    std::string metric_name_;
    std::map<std::string, std::string> labels_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace observability
} // namespace themis
