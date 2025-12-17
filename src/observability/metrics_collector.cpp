#include "observability/metrics_collector.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace themis {
namespace observability {

// Singleton instance
MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

// ===== TSStore Metrics =====

void MetricsCollector::recordTSStoreWrite(const std::string& metric, size_t batch_size, double latency_ms) {
    incrementCounter("tsstore_writes_total", {{"metric", metric}});
    incrementCounter("tsstore_points_written", {{"metric", metric}});
    observeHistogram("tsstore_write_latency_ms", latency_ms, {{"metric", metric}});
    setGauge("tsstore_write_batch_size", static_cast<double>(batch_size), {{"metric", metric}});
}

void MetricsCollector::recordTSStoreQuery(const std::string& metric, size_t result_count, double latency_ms) {
    incrementCounter("tsstore_queries_total", {{"metric", metric}});
    setGauge("tsstore_query_result_count", static_cast<double>(result_count), {{"metric", metric}});
    observeHistogram("tsstore_query_latency_ms", latency_ms, {{"metric", metric}});
}

void MetricsCollector::recordTSStoreAggregate(const std::string& metric, size_t point_count, double latency_ms) {
    incrementCounter("tsstore_aggregates_total", {{"metric", metric}});
    setGauge("tsstore_aggregate_point_count", static_cast<double>(point_count), {{"metric", metric}});
    observeHistogram("tsstore_aggregate_latency_ms", latency_ms, {{"metric", metric}});
}

void MetricsCollector::recordTSStoreCompression(const std::string& compression_type, double ratio) {
    incrementCounter("tsstore_compression_operations", {{"type", compression_type}});
    observeHistogram("tsstore_compression_ratio", ratio, {{"type", compression_type}});
}

// ===== Query Engine Metrics =====

void MetricsCollector::recordQuery(const std::string& query_type, double latency_ms, size_t result_count) {
    incrementCounter("queries_total", {{"type", query_type}});
    observeHistogram("query_latency_ms", latency_ms, {{"type", query_type}});
    setGauge("query_result_count", static_cast<double>(result_count), {{"type", query_type}});
}

void MetricsCollector::recordIndexScan(const std::string& index_type, size_t keys_scanned) {
    incrementCounter("index_scans_total", {{"type", index_type}});
    incrementCounter("index_keys_scanned", {{"type", index_type}});
}

void MetricsCollector::recordFullScan(const std::string& table, size_t keys_scanned) {
    incrementCounter("full_scans_total", {{"table", table}});
    incrementCounter("full_scan_keys", {{"table", table}});
}

// ===== Cache Metrics =====

void MetricsCollector::recordCacheHit(const std::string& cache_type) {
    incrementCounter("cache_hits_total", {{"type", cache_type}});
}

void MetricsCollector::recordCacheMiss(const std::string& cache_type) {
    incrementCounter("cache_misses_total", {{"type", cache_type}});
}

void MetricsCollector::recordCacheEviction(const std::string& cache_type) {
    incrementCounter("cache_evictions_total", {{"type", cache_type}});
}

// ===== Sharding Metrics =====

void MetricsCollector::recordShardRequest(const std::string& shard_id, const std::string& operation) {
    incrementCounter("shard_requests_total", {{"shard_id", shard_id}, {"operation", operation}});
}

void MetricsCollector::recordShardLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("shard_request_latency_ms", latency_ms, {{"shard_id", shard_id}});
}

void MetricsCollector::recordRebalanceProgress(const std::string& operation_id, int64_t records, double percent) {
    setGauge("rebalance_records_migrated", static_cast<double>(records), {{"operation_id", operation_id}});
    setGauge("rebalance_progress_percent", percent, {{"operation_id", operation_id}});
}

// ===== Content Processing Metrics =====

void MetricsCollector::recordContentImport(const std::string& mime_type, size_t size_bytes) {
    incrementCounter("content_imports_total", {{"mime_type", mime_type}});
    incrementCounter("content_bytes_imported", {{"mime_type", mime_type}});
}

void MetricsCollector::recordChunkCreation(size_t chunk_count) {
    incrementCounter("chunks_created_total", {});
}

void MetricsCollector::recordEmbeddingGeneration(size_t count, double latency_ms) {
    incrementCounter("embeddings_generated_total", {});
    observeHistogram("embedding_generation_latency_ms", latency_ms, {});
}

// ===== Security Metrics =====

void MetricsCollector::recordAuthAttempt(bool success) {
    incrementCounter("auth_attempts_total", {{"result", success ? "success" : "failure"}});
}

void MetricsCollector::recordPolicyEvaluation(bool allowed, double latency_ms) {
    incrementCounter("policy_evaluations_total", {{"result", allowed ? "allowed" : "denied"}});
    observeHistogram("policy_evaluation_latency_ms", latency_ms, {});
}

void MetricsCollector::recordEncryptionOperation(const std::string& operation, double latency_ms) {
    incrementCounter("encryption_operations_total", {{"operation", operation}});
    observeHistogram("encryption_latency_ms", latency_ms, {{"operation", operation}});
}

// ===== System Metrics =====

void MetricsCollector::recordMemoryUsage(size_t bytes) {
    setGauge("memory_usage_bytes", static_cast<double>(bytes), {});
}

void MetricsCollector::recordCPUUsage(double percent) {
    setGauge("cpu_usage_percent", percent, {});
}

void MetricsCollector::recordDiskIOps(size_t read_ops, size_t write_ops) {
    incrementCounter("disk_read_ops_total", {});
    incrementCounter("disk_write_ops_total", {});
}

// ===== Replication Metrics =====

void MetricsCollector::recordReplicationLag(const std::string& replica_id, double lag_ms) {
    setGauge("replication_lag_ms", lag_ms, {{"replica_id", replica_id}});
}

void MetricsCollector::recordWALWrite(size_t bytes, double latency_ms) {
    incrementCounter("wal_writes_total", {});
    incrementCounter("wal_bytes_written", {});
    observeHistogram("wal_write_latency_ms", latency_ms, {});
}

void MetricsCollector::recordReplicationStreamBytes(const std::string& replica_id, size_t bytes) {
    incrementCounter("replication_stream_bytes_total", {{"replica_id", replica_id}});
}

void MetricsCollector::recordFailoverEvent(const std::string& old_leader, const std::string& new_leader) {
    incrementCounter("failover_events_total", {{"old_leader", old_leader}, {"new_leader", new_leader}});
}

void MetricsCollector::recordReplicaSyncStatus(const std::string& replica_id, bool in_sync) {
    setGauge("replica_sync_status", in_sync ? 1.0 : 0.0, {{"replica_id", replica_id}});
}

// ===== Storage Metrics =====

void MetricsCollector::recordBackupOperation(const std::string& backup_type, bool success, double duration_ms) {
    incrementCounter("backup_operations_total", {{"type", backup_type}, {"result", success ? "success" : "failure"}});
    observeHistogram("backup_duration_ms", duration_ms, {{"type", backup_type}});
}

void MetricsCollector::recordBackupSize(const std::string& backup_type, size_t size_bytes) {
    setGauge("backup_size_bytes", static_cast<double>(size_bytes), {{"type", backup_type}});
}

void MetricsCollector::recordRestoreOperation(bool success, double duration_ms) {
    incrementCounter("restore_operations_total", {{"result", success ? "success" : "failure"}});
    observeHistogram("restore_duration_ms", duration_ms, {});
}

void MetricsCollector::recordBlobStorageOperation(const std::string& backend, const std::string& operation, size_t bytes, double latency_ms) {
    incrementCounter("blob_storage_operations_total", {{"backend", backend}, {"operation", operation}});
    incrementCounter("blob_storage_bytes_total", {{"backend", backend}, {"operation", operation}});
    observeHistogram("blob_storage_latency_ms", latency_ms, {{"backend", backend}, {"operation", operation}});
}

void MetricsCollector::recordRocksDBCompaction(size_t bytes_read, size_t bytes_written, double duration_ms) {
    incrementCounter("rocksdb_compactions_total", {});
    incrementCounter("rocksdb_compaction_bytes_read", {});
    incrementCounter("rocksdb_compaction_bytes_written", {});
    observeHistogram("rocksdb_compaction_duration_ms", duration_ms, {});
}

// ===== Index Metrics =====

void MetricsCollector::recordIndexRebuild(const std::string& index_type, size_t entities_processed, double duration_ms) {
    incrementCounter("index_rebuilds_total", {{"type", index_type}});
    setGauge("index_rebuild_entities_processed", static_cast<double>(entities_processed), {{"type", index_type}});
    observeHistogram("index_rebuild_duration_ms", duration_ms, {{"type", index_type}});
}

void MetricsCollector::recordIndexLookup(const std::string& index_type, bool hit, double latency_ms) {
    incrementCounter("index_lookups_total", {{"type", index_type}, {"result", hit ? "hit" : "miss"}});
    observeHistogram("index_lookup_latency_ms", latency_ms, {{"type", index_type}});
}

void MetricsCollector::recordSpatialQuery(const std::string& query_type, size_t candidates, size_t results, double latency_ms) {
    incrementCounter("spatial_queries_total", {{"type", query_type}});
    setGauge("spatial_query_candidates", static_cast<double>(candidates), {{"type", query_type}});
    setGauge("spatial_query_results", static_cast<double>(results), {{"type", query_type}});
    observeHistogram("spatial_query_latency_ms", latency_ms, {{"type", query_type}});
}

void MetricsCollector::recordVectorSearch(const std::string& index_type, size_t dimension, size_t k, double latency_ms) {
    incrementCounter("vector_searches_total", {{"type", index_type}});
    setGauge("vector_search_dimension", static_cast<double>(dimension), {{"type", index_type}});
    setGauge("vector_search_k", static_cast<double>(k), {{"type", index_type}});
    observeHistogram("vector_search_latency_ms", latency_ms, {{"type", index_type}});
}

// ===== Network & Resilience Metrics =====

void MetricsCollector::recordCircuitBreakerState(const std::string& service, const std::string& state) {
    // state can be: closed, open, half_open
    double state_value = 0.0;
    if (state == "closed") state_value = 0.0;
    else if (state == "half_open") state_value = 0.5;
    else if (state == "open") state_value = 1.0;
    setGauge("circuit_breaker_state", state_value, {{"service", service}});
}

void MetricsCollector::recordCircuitBreakerTrip(const std::string& service, const std::string& reason) {
    incrementCounter("circuit_breaker_trips_total", {{"service", service}, {"reason", reason}});
}

void MetricsCollector::recordRetryAttempt(const std::string& operation, int attempt_number, bool success) {
    incrementCounter("retry_attempts_total", {{"operation", operation}, {"result", success ? "success" : "failure"}});
    setGauge("retry_attempt_number", static_cast<double>(attempt_number), {{"operation", operation}});
}

void MetricsCollector::recordConnectionPoolStats(const std::string& pool_name, size_t active, size_t idle, size_t waiting) {
    setGauge("connection_pool_active", static_cast<double>(active), {{"pool", pool_name}});
    setGauge("connection_pool_idle", static_cast<double>(idle), {{"pool", pool_name}});
    setGauge("connection_pool_waiting", static_cast<double>(waiting), {{"pool", pool_name}});
}

// ===== CDC (Change Data Capture) Metrics =====

void MetricsCollector::recordCDCEvent(const std::string& event_type, const std::string& collection) {
    incrementCounter("cdc_events_total", {{"type", event_type}, {"collection", collection}});
}

void MetricsCollector::recordCDCLag(const std::string& subscriber, double lag_ms) {
    setGauge("cdc_lag_ms", lag_ms, {{"subscriber", subscriber}});
}

void MetricsCollector::recordCDCThroughput(size_t events_per_second) {
    setGauge("cdc_throughput_events_per_second", static_cast<double>(events_per_second), {});
}

// ===== Analytics Metrics =====

void MetricsCollector::recordAnalyticsQuery(const std::string& query_type, double latency_ms, size_t rows_processed) {
    incrementCounter("analytics_queries_total", {{"type", query_type}});
    setGauge("analytics_query_rows_processed", static_cast<double>(rows_processed), {{"type", query_type}});
    observeHistogram("analytics_query_latency_ms", latency_ms, {{"type", query_type}});
}

void MetricsCollector::recordOLAPAggregation(const std::string& operation, size_t input_rows, double latency_ms) {
    incrementCounter("olap_aggregations_total", {{"operation", operation}});
    setGauge("olap_aggregation_input_rows", static_cast<double>(input_rows), {{"operation", operation}});
    observeHistogram("olap_aggregation_latency_ms", latency_ms, {{"operation", operation}});
}

void MetricsCollector::recordCEPRuleEvaluation(const std::string& rule_id, bool triggered, double latency_ms) {
    incrementCounter("cep_rule_evaluations_total", {{"rule_id", rule_id}, {"triggered", triggered ? "true" : "false"}});
    observeHistogram("cep_rule_evaluation_latency_ms", latency_ms, {{"rule_id", rule_id}});
}

// ===== LLM & AI Metrics =====

void MetricsCollector::recordLLMRequest(const std::string& model, size_t input_tokens, size_t output_tokens, double latency_ms) {
    incrementCounter("llm_requests_total", {{"model", model}});
    incrementCounter("llm_input_tokens_total", {{"model", model}});
    incrementCounter("llm_output_tokens_total", {{"model", model}});
    observeHistogram("llm_request_latency_ms", latency_ms, {{"model", model}});
}

void MetricsCollector::recordVectorEmbeddingGeneration(const std::string& model, size_t count, double latency_ms) {
    incrementCounter("vector_embeddings_generated_total", {{"model", model}});
    observeHistogram("vector_embedding_latency_ms", latency_ms, {{"model", model}});
}

void MetricsCollector::recordRAGRetrieval(size_t chunks_retrieved, double latency_ms) {
    incrementCounter("rag_retrievals_total", {});
    setGauge("rag_chunks_retrieved", static_cast<double>(chunks_retrieved), {});
    observeHistogram("rag_retrieval_latency_ms", latency_ms, {});
}

// ===== Transaction Metrics =====

void MetricsCollector::recordTransaction(const std::string& isolation_level, bool committed, double duration_ms) {
    incrementCounter("transactions_total", {{"isolation", isolation_level}, {"result", committed ? "committed" : "aborted"}});
    observeHistogram("transaction_duration_ms", duration_ms, {{"isolation", isolation_level}});
}

void MetricsCollector::recordLockAcquisition(const std::string& lock_type, bool success, double wait_time_ms) {
    incrementCounter("lock_acquisitions_total", {{"type", lock_type}, {"result", success ? "success" : "timeout"}});
    observeHistogram("lock_wait_time_ms", wait_time_ms, {{"type", lock_type}});
}

void MetricsCollector::recordDeadlock(const std::string& transaction_id) {
    incrementCounter("deadlocks_total", {{"transaction_id", transaction_id}});
}

// ===== Prometheus Text Format Export =====

std::string MetricsCollector::getPrometheusMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Header
    oss << "# ThemisDB Metrics\n";
    oss << "# HELP themis_build_info Build information\n";
    oss << "# TYPE themis_build_info gauge\n";
    oss << "themis_build_info{version=\"0.1.0\"} 1\n\n";
    
    // Counters
    for (const auto& [key, value] : counters_) {
        size_t pos = key.find('{');
        std::string name = (pos != std::string::npos) ? key.substr(0, pos) : key;
        std::string labels = (pos != std::string::npos) ? key.substr(pos) : "";
        
        oss << "# TYPE " << name << " counter\n";
        oss << name << labels << " " << value.load() << "\n";
    }
    
    // Gauges
    for (const auto& [key, value] : gauges_) {
        size_t pos = key.find('{');
        std::string name = (pos != std::string::npos) ? key.substr(0, pos) : key;
        std::string labels = (pos != std::string::npos) ? key.substr(pos) : "";
        
        oss << "# TYPE " << name << " gauge\n";
        oss << name << labels << " " << std::fixed << std::setprecision(2) << value.load() << "\n";
    }
    
    // Histograms (simplified - show p50, p95, p99)
    for (const auto& [key, hist] : histograms_) {
        if (!hist || hist->values.empty()) continue;
        
        size_t pos = key.find('{');
        std::string name = (pos != std::string::npos) ? key.substr(0, pos) : key;
        std::string labels = (pos != std::string::npos) ? key.substr(pos) : "";
        
        oss << "# TYPE " << name << " summary\n";
        oss << name << labels << "{quantile=\"0.5\"} " << std::fixed << std::setprecision(2) 
            << hist->percentile(0.5) << "\n";
        oss << name << labels << "{quantile=\"0.95\"} " << hist->percentile(0.95) << "\n";
        oss << name << labels << "{quantile=\"0.99\"} " << hist->percentile(0.99) << "\n";
        oss << name << "_count" << labels << " " << hist->values.size() << "\n";
        oss << name << "_sum" << labels << " " << std::accumulate(hist->values.begin(), hist->values.end(), 0.0) << "\n";
    }
    
    return oss.str();
}

void MetricsCollector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
}

// ===== Helper Functions =====

void MetricsCollector::incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    counters_[key]++;
}

void MetricsCollector::setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    gauges_[key].store(value);
}

void MetricsCollector::observeHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeKey(name, labels);
    
    if (histograms_.find(key) == histograms_.end()) {
        histograms_[key] = std::make_shared<Histogram>();
    }
    
    histograms_[key]->observe(value);
}

std::string MetricsCollector::makeKey(const std::string& name, const std::map<std::string, std::string>& labels) const {
    if (labels.empty()) {
        return name;
    }
    return name + formatLabels(labels);
}

std::string MetricsCollector::formatLabels(const std::map<std::string, std::string>& labels) const {
    if (labels.empty()) return "";
    
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) oss << ",";
        oss << key << "=\"" << value << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string MetricsCollector::formatMetricLine(const std::string& name, const std::string& labels, double value) const {
    std::ostringstream oss;
    oss << name << labels << " " << std::fixed << std::setprecision(2) << value;
    return oss.str();
}

// ===== Histogram Implementation =====

void MetricsCollector::Histogram::observe(double value) {
    values.push_back(value);
    
    // Keep only recent samples
    if (values.size() > max_samples) {
        values.erase(values.begin(), values.begin() + (values.size() - max_samples));
    }
}

void MetricsCollector::Histogram::reset() {
    values.clear();
    last_reset = std::chrono::steady_clock::now();
}

double MetricsCollector::Histogram::percentile(double p) const {
    if (values.empty()) return 0.0;
    
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(p * (sorted.size() - 1));
    return sorted[index];
}

double MetricsCollector::Histogram::mean() const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

// ===== LatencyTracker Implementation =====

LatencyTracker::LatencyTracker(const std::string& metric_name, 
                               const std::map<std::string, std::string>& labels)
    : metric_name_(metric_name), labels_(labels), start_(std::chrono::steady_clock::now()) {
}

LatencyTracker::~LatencyTracker() {
    double elapsed = elapsedMs();
    MetricsCollector::getInstance().observeHistogram(metric_name_, elapsed, labels_);
}

double LatencyTracker::elapsedMs() const {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start_).count();
}

} // namespace observability
} // namespace themis
