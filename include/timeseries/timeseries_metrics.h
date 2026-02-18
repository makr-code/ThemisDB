#ifndef THEMIS_TIMESERIES_METRICS_H
#define THEMIS_TIMESERIES_METRICS_H

#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <map>
#include <vector>

namespace themis {

/**
 * @brief Prometheus-style metrics for time series operations
 * 
 * Tracks performance, storage, and operational metrics for TSStore.
 * Provides insights into ingestion rate, query performance, compression
 * efficiency, and storage utilization.
 * 
 * Metrics Categories:
 * - Ingestion: Data point writes, batch operations, throughput
 * - Query: Query count, latency, result set sizes
 * - Aggregation: Aggregation operations, optimization hits
 * - Storage: Data size, compression ratio, retention operations
 * - Performance: Operation latencies, cache hits
 */
class TimeSeriesMetrics {
public:
    struct Config {
        bool enable_histograms = true;
        bool enable_per_metric_stats = true;
        int histogram_buckets = 20;
    };

    TimeSeriesMetrics();
    explicit TimeSeriesMetrics(const Config& config);
    ~TimeSeriesMetrics() = default;

    // ==================== Ingestion Metrics ====================
    
    /**
     * @brief Record a data point write operation
     * @param metric_name Name of the metric
     * @param latency_ms Operation latency in milliseconds
     * @param success Whether the operation succeeded
     */
    void recordDataPointWrite(const std::string& metric_name, double latency_ms, bool success = true);
    
    /**
     * @brief Record a batch write operation
     * @param num_points Number of data points in batch
     * @param latency_ms Operation latency in milliseconds
     * @param compressed Whether compression was used
     * @param success Whether the operation succeeded
     */
    void recordBatchWrite(size_t num_points, double latency_ms, bool compressed, bool success = true);
    
    /**
     * @brief Record compression statistics
     * @param metric_name Name of the metric
     * @param uncompressed_bytes Size before compression
     * @param compressed_bytes Size after compression
     */
    void recordCompression(const std::string& metric_name, size_t uncompressed_bytes, size_t compressed_bytes);

    // ==================== Query Metrics ====================
    
    /**
     * @brief Record a query operation
     * @param metric_name Name of the metric
     * @param latency_ms Query latency in milliseconds
     * @param result_count Number of data points returned
     * @param time_range_ms Time range covered by query
     */
    void recordQuery(const std::string& metric_name, double latency_ms, size_t result_count, int64_t time_range_ms);
    
    /**
     * @brief Record an aggregation operation
     * @param metric_name Name of the metric
     * @param latency_ms Aggregation latency in milliseconds
     * @param data_points_scanned Number of data points scanned
     * @param optimizer_used Whether query optimizer was used
     */
    void recordAggregation(const std::string& metric_name, double latency_ms, size_t data_points_scanned, bool optimizer_used);
    
    /**
     * @brief Record query optimizer hit/miss
     * @param hit Whether optimizer could optimize the query
     */
    void recordOptimizerResult(bool hit);

    // ==================== Storage Metrics ====================
    
    /**
     * @brief Update storage statistics
     * @param total_data_points Total number of data points
     * @param total_metrics Number of unique metrics
     * @param total_size_bytes Total storage size in bytes
     */
    void updateStorageStats(size_t total_data_points, size_t total_metrics, size_t total_size_bytes);
    
    /**
     * @brief Record a retention operation
     * @param metric_name Name of the metric (empty for global)
     * @param deleted_points Number of data points deleted
     * @param latency_ms Operation latency in milliseconds
     */
    void recordRetention(const std::string& metric_name, size_t deleted_points, double latency_ms);

    // ==================== Performance Metrics ====================
    
    /**
     * @brief Record continuous aggregate refresh
     * @param metric_name Source metric name
     * @param window_ms Aggregation window in milliseconds
     * @param latency_ms Refresh latency in milliseconds
     * @param points_processed Number of points processed
     */
    void recordContinuousAggregateRefresh(const std::string& metric_name, int64_t window_ms, 
                                          double latency_ms, size_t points_processed);

    // ==================== Metric Export ====================
    
    /**
     * @brief Export metrics in Prometheus text format
     * @return String containing all metrics in Prometheus format
     */
    std::string exportPrometheus() const;
    
    /**
     * @brief Export metrics as JSON
     * @return String containing all metrics in JSON format
     */
    std::string exportJson() const;
    
    /**
     * @brief Reset all metrics (for testing)
     */
    void reset();

    // ==================== Getters ====================
    
    uint64_t getTotalDataPointsWritten() const { return total_data_points_written_.load(); }
    uint64_t getTotalBatchesWritten() const { return total_batches_written_.load(); }
    uint64_t getTotalQueriesExecuted() const { return total_queries_executed_.load(); }
    uint64_t getTotalAggregationsExecuted() const { return total_aggregations_executed_.load(); }
    uint64_t getOptimizerHits() const { return optimizer_hits_.load(); }
    uint64_t getOptimizerMisses() const { return optimizer_misses_.load(); }
    
    double getAverageWriteLatency() const;
    double getAverageQueryLatency() const;
    double getAverageCompressionRatio() const;

private:
    Config config_;
    
    // Ingestion counters
    std::atomic<uint64_t> total_data_points_written_{0};
    std::atomic<uint64_t> total_batches_written_{0};
    std::atomic<uint64_t> total_compressed_batches_{0};
    std::atomic<uint64_t> write_errors_{0};
    std::atomic<uint64_t> total_bytes_written_uncompressed_{0};
    std::atomic<uint64_t> total_bytes_written_compressed_{0};
    
    // Query counters
    std::atomic<uint64_t> total_queries_executed_{0};
    std::atomic<uint64_t> total_aggregations_executed_{0};
    std::atomic<uint64_t> total_data_points_returned_{0};
    std::atomic<uint64_t> optimizer_hits_{0};
    std::atomic<uint64_t> optimizer_misses_{0};
    
    // Storage counters (updated periodically)
    std::atomic<uint64_t> current_data_points_{0};
    std::atomic<uint64_t> current_metrics_count_{0};
    std::atomic<uint64_t> current_storage_bytes_{0};
    
    // Retention counters
    std::atomic<uint64_t> total_retention_runs_{0};
    std::atomic<uint64_t> total_data_points_deleted_{0};
    
    // Continuous aggregate counters
    std::atomic<uint64_t> total_continuous_agg_refreshes_{0};
    std::atomic<uint64_t> total_continuous_agg_points_generated_{0};
    
    // Latency tracking (simple averages for now)
    mutable std::mutex latency_mutex_;
    double total_write_latency_ms_{0.0};
    uint64_t write_latency_count_{0};
    double total_query_latency_ms_{0.0};
    uint64_t query_latency_count_{0};
    double total_aggregation_latency_ms_{0.0};
    uint64_t aggregation_latency_count_{0};
    
    // Per-metric statistics (optional)
    mutable std::mutex per_metric_mutex_;
    struct PerMetricStats {
        uint64_t data_points_written = 0;
        uint64_t queries_executed = 0;
        uint64_t bytes_written = 0;
        double total_write_latency_ms = 0.0;
        double total_query_latency_ms = 0.0;
    };
    std::map<std::string, PerMetricStats> per_metric_stats_;
    
    // Helper methods
    void recordLatency(double& total_latency, uint64_t& count, double latency_ms);
    double getAverageLatency(double total_latency, uint64_t count) const;
    std::string formatPrometheusMetric(const std::string& name, const std::string& type,
                                       const std::string& help, uint64_t value) const;
    std::string formatPrometheusMetric(const std::string& name, const std::string& type,
                                       const std::string& help, double value) const;
};

} // namespace themis

#endif // THEMIS_TIMESERIES_METRICS_H
