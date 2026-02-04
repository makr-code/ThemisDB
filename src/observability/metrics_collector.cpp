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

// ===== Tracing Metrics =====

void MetricsCollector::recordSpanDuration(const std::string& span_name, double duration_ms) {
    observeHistogram("trace_span_duration_ms", duration_ms, {{"span", span_name}});
    incrementCounter("trace_spans_total", {{"span", span_name}});
}

void MetricsCollector::recordActiveSpans(int64_t count) {
    setGauge("trace_active_spans", static_cast<double>(count), {});
}

void MetricsCollector::recordTotalSpans(int64_t count) {
    setGauge("trace_total_spans", static_cast<double>(count), {});
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
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[key]++;
}

void MetricsCollector::setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    std::lock_guard<std::mutex> lock(mutex_);
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
