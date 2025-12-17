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
    
    // Replication Metrics
    void recordReplicationLag(const std::string& replica_id, double lag_ms);
    void recordWALWrite(size_t bytes, double latency_ms);
    void recordReplicationStreamBytes(const std::string& replica_id, size_t bytes);
    void recordFailoverEvent(const std::string& old_leader, const std::string& new_leader);
    void recordReplicaSyncStatus(const std::string& replica_id, bool in_sync);
    
    // Storage Metrics
    void recordBackupOperation(const std::string& backup_type, bool success, double duration_ms);
    void recordBackupSize(const std::string& backup_type, size_t size_bytes);
    void recordRestoreOperation(bool success, double duration_ms);
    void recordBlobStorageOperation(const std::string& backend, const std::string& operation, size_t bytes, double latency_ms);
    void recordRocksDBCompaction(size_t bytes_read, size_t bytes_written, double duration_ms);
    
    // Index Metrics
    void recordIndexRebuild(const std::string& index_type, size_t entities_processed, double duration_ms);
    void recordIndexLookup(const std::string& index_type, bool hit, double latency_ms);
    void recordSpatialQuery(const std::string& query_type, size_t candidates, size_t results, double latency_ms);
    void recordVectorSearch(const std::string& index_type, size_t dimension, size_t k, double latency_ms);
    
    // Network & Resilience Metrics
    void recordCircuitBreakerState(const std::string& service, const std::string& state);
    void recordCircuitBreakerTrip(const std::string& service, const std::string& reason);
    void recordRetryAttempt(const std::string& operation, int attempt_number, bool success);
    void recordConnectionPoolStats(const std::string& pool_name, size_t active, size_t idle, size_t waiting);
    
    // CDC (Change Data Capture) Metrics
    void recordCDCEvent(const std::string& event_type, const std::string& collection);
    void recordCDCLag(const std::string& subscriber, double lag_ms);
    void recordCDCThroughput(size_t events_per_second);
    
    // Analytics Metrics
    void recordAnalyticsQuery(const std::string& query_type, double latency_ms, size_t rows_processed);
    void recordOLAPAggregation(const std::string& operation, size_t input_rows, double latency_ms);
    void recordCEPRuleEvaluation(const std::string& rule_id, bool triggered, double latency_ms);
    
    // LLM & AI Metrics
    void recordLLMRequest(const std::string& model, size_t input_tokens, size_t output_tokens, double latency_ms);
    void recordVectorEmbeddingGeneration(const std::string& model, size_t count, double latency_ms);
    void recordRAGRetrieval(size_t chunks_retrieved, double latency_ms);
    
    // Transaction Metrics
    void recordTransaction(const std::string& isolation_level, bool committed, double duration_ms);
    void recordLockAcquisition(const std::string& lock_type, bool success, double wait_time_ms);
    void recordDeadlock(const std::string& transaction_id);
    
    // Get metrics in Prometheus text format
    std::string getPrometheusMetrics() const;
    
    // Reset all metrics (for testing)
    void reset();

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    
    friend class LatencyTracker;
    mutable std::mutex mutex_;
    
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
