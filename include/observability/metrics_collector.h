/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics_collector.h                                ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     206                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 177485abe  2026-02-20  Observability Hardening: Structured Logging, Trace Correl... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>

namespace themis {
namespace observability {

class LatencyTracker;

/**
 * Central metrics collector for ThemisDB observability.
 * 
 * Aggregates metrics from all subsystems (TSStore, Query Engine, Sharding, Cache, etc.)
 * and exposes them in Prometheus text format via /metrics endpoint.
 * 
 * Thread-Safety:
 * - All public methods are thread-safe
 * - Counter and gauge operations are protected by mutex during map insertion
 * - Histogram operations are fully synchronized
 * - Safe for concurrent access from multiple threads
 * 
 * Thread-safe singleton pattern for global access.
 */
class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    // Prevent copying
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
    // TSStore Metrics
    void recordTSStoreWrite(const std::string& metric, size_t batch_size, double latency_ms);
    void recordTSStoreQuery(const std::string& metric, size_t result_count, double latency_ms);
    void recordTSStoreAggregate(const std::string& metric, size_t point_count, double latency_ms);
    void recordTSStoreCompression(const std::string& compression_type, double ratio);
    
    // Query Engine Metrics
    void recordQuery(const std::string& query_type, double latency_ms, size_t result_count);
    void recordIndexScan(const std::string& index_type, size_t keys_scanned);
    void recordFullScan(const std::string& table, size_t keys_scanned);
    
    // Cache Metrics
    void recordCacheHit(const std::string& cache_type);
    void recordCacheMiss(const std::string& cache_type);
    void recordCacheEviction(const std::string& cache_type);
    
    // Sharding Metrics
    void recordShardRequest(const std::string& shard_id, const std::string& operation);
    void recordShardLatency(const std::string& shard_id, double latency_ms);
    void recordRebalanceProgress(const std::string& operation_id, int64_t records, double percent);
    
    // Content Processing Metrics
    void recordContentImport(const std::string& mime_type, size_t size_bytes);
    void recordChunkCreation(size_t chunk_count);
    void recordEmbeddingGeneration(size_t count, double latency_ms);
    
    // Security Metrics
    void recordAuthAttempt(bool success);
    void recordPolicyEvaluation(bool allowed, double latency_ms);
    void recordEncryptionOperation(const std::string& operation, double latency_ms);
    
    // System Metrics
    void recordMemoryUsage(size_t bytes);
    void recordCPUUsage(double percent);
    void recordDiskIOps(size_t read_ops, size_t write_ops);
    
    // Tracing Metrics
    void recordSpanDuration(const std::string& span_name, double duration_ms);
    void recordActiveSpans(int64_t count);
    void recordTotalSpans(int64_t count);
    
    // Get metrics in Prometheus text format
    std::string getPrometheusMetrics() const;
    
    // Reset all metrics (for testing)
    void reset();

    // ===== Cardinality control =====
    /**
     * @brief Set the maximum number of unique label-set combinations per metric
     *        name.  When the limit is reached, new series are silently dropped
     *        and a counter is incremented.  Set to 0 to disable.
     */
    void setCardinalityLimit(size_t limit);
    size_t getCardinalityLimit() const;

    /** Returns the number of metric observations dropped due to cardinality overflow. */
    int64_t getDroppedSeriesCount() const;

    // ===== Exporter health =====
    /** Record a transient failure contacting an exporter (OTLP, Pushgateway, etc.). */
    void recordExporterFailure(const std::string& exporter_name);
    /** Record that an exporter has recovered after previous failures. */
    void recordExporterRecovery(const std::string& exporter_name);

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    
    friend class LatencyTracker;
    mutable std::mutex mutex_;
    
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
        
        void observe(double value);
        void reset();
        double percentile(double p) const;
        double mean() const;
    };
    std::map<std::string, std::shared_ptr<Histogram>> histograms_;
    
    // Helper functions
    void incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels = {});
    void setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels = {});
    void observeHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels = {});

    /**
     * @brief Check whether a new series (name + labels combination) is allowed
     *        by the cardinality limit.  Returns true if the observation should
     *        proceed, false if it should be dropped.
     *
     * Caller MUST hold mutex_ before calling this.
     */
    bool checkCardinality(const std::string& name, const std::string& key);
    
    std::string makeKey(const std::string& name, const std::map<std::string, std::string>& labels) const;
    std::string formatLabels(const std::map<std::string, std::string>& labels) const;
    std::string formatMetricLine(const std::string& name, const std::string& labels, double value) const;
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
    
    // Get elapsed time without ending tracker
    double elapsedMs() const;

private:
    std::string metric_name_;
    std::map<std::string, std::string> labels_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace observability
} // namespace themis
