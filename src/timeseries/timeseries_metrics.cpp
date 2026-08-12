/**
 * @file timeseries_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/timeseries_metrics.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <nlohmann/json.hpp>

namespace themis {

TimeSeriesMetrics::TimeSeriesMetrics()
    : TimeSeriesMetrics(Config{}) {
}

TimeSeriesMetrics::TimeSeriesMetrics(const Config& config)
    : config_(config) {
}

void TimeSeriesMetrics::recordDataPointWrite(const std::string& metric_name, double latency_ms, bool success) {
    total_data_points_written_.fetch_add(1, std::memory_order_relaxed);
    
    if (!success) {
        write_errors_.fetch_add(1, std::memory_order_relaxed);
    }
    
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        recordLatency(total_write_latency_ms_, write_latency_count_, latency_ms);
    }
    
    if (config_.enable_per_metric_stats && !metric_name.empty()) {
        std::lock_guard<std::mutex> lock(per_metric_mutex_);
        auto& stats = per_metric_stats_[metric_name];
        stats.data_points_written++;
        stats.total_write_latency_ms += latency_ms;
    }
}

void TimeSeriesMetrics::recordOutOfOrderWrite(const std::string& /*metric_name*/, bool rejected) {
    if (rejected) {
        late_arrival_rejected_.fetch_add(1, std::memory_order_relaxed);
    } else {
        out_of_order_accepted_.fetch_add(1, std::memory_order_relaxed);
    }
}

void TimeSeriesMetrics::recordBatchWrite(size_t num_points, double latency_ms, bool compressed, bool success) {
    total_data_points_written_.fetch_add(num_points, std::memory_order_relaxed);
    total_batches_written_.fetch_add(1, std::memory_order_relaxed);
    
    if (compressed) {
        total_compressed_batches_.fetch_add(1, std::memory_order_relaxed);
    }
    
    if (!success) {
        write_errors_.fetch_add(1, std::memory_order_relaxed);
    }
    
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        recordLatency(total_write_latency_ms_, write_latency_count_, latency_ms);
    }
}

void TimeSeriesMetrics::recordCompression(const std::string& metric_name, size_t uncompressed_bytes, size_t compressed_bytes) {
    total_bytes_written_uncompressed_.fetch_add(uncompressed_bytes, std::memory_order_relaxed);
    total_bytes_written_compressed_.fetch_add(compressed_bytes, std::memory_order_relaxed);
    
    if (config_.enable_per_metric_stats && !metric_name.empty()) {
        std::lock_guard<std::mutex> lock(per_metric_mutex_);
        auto& stats = per_metric_stats_[metric_name];
        stats.bytes_written += compressed_bytes;
    }
}

void TimeSeriesMetrics::recordQuery(const std::string& metric_name, double latency_ms, 
                                    size_t result_count, int64_t /*time_range_ms*/) {
    total_queries_executed_.fetch_add(1, std::memory_order_relaxed);
    total_data_points_returned_.fetch_add(result_count, std::memory_order_relaxed);
    
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        recordLatency(total_query_latency_ms_, query_latency_count_, latency_ms);
    }
    
    if (config_.enable_per_metric_stats && !metric_name.empty()) {
        std::lock_guard<std::mutex> lock(per_metric_mutex_);
        auto& stats = per_metric_stats_[metric_name];
        stats.queries_executed++;
        stats.total_query_latency_ms += latency_ms;
    }
}

void TimeSeriesMetrics::recordAggregation(const std::string& /*metric_name*/, double latency_ms, 
                                         size_t /*data_points_scanned*/, bool optimizer_used) {
    total_aggregations_executed_.fetch_add(1, std::memory_order_relaxed);
    
    if (optimizer_used) {
        optimizer_hits_.fetch_add(1, std::memory_order_relaxed);
    } else {
        optimizer_misses_.fetch_add(1, std::memory_order_relaxed);
    }
    
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        recordLatency(total_aggregation_latency_ms_, aggregation_latency_count_, latency_ms);
    }
}

void TimeSeriesMetrics::recordOptimizerResult(bool hit) {
    if (hit) {
        optimizer_hits_.fetch_add(1, std::memory_order_relaxed);
    } else {
        optimizer_misses_.fetch_add(1, std::memory_order_relaxed);
    }
}

void TimeSeriesMetrics::updateStorageStats(size_t total_data_points, size_t total_metrics, size_t total_size_bytes) {
    current_data_points_.store(total_data_points, std::memory_order_relaxed);
    current_metrics_count_.store(total_metrics, std::memory_order_relaxed);
    current_storage_bytes_.store(total_size_bytes, std::memory_order_relaxed);
}

void TimeSeriesMetrics::recordRetention(const std::string& /*metric_name*/, size_t deleted_points, double /*latency_ms*/) {
    total_retention_runs_.fetch_add(1, std::memory_order_relaxed);
    total_data_points_deleted_.fetch_add(deleted_points, std::memory_order_relaxed);
}

void TimeSeriesMetrics::recordBackpressure(const std::string& /*metric_name*/) {
    total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);
}

void TimeSeriesMetrics::recordOverdueFlush(const std::string& /*metric_name*/, double /*age_ms*/) {
    total_overdue_flush_events_.fetch_add(1, std::memory_order_relaxed);
}

void TimeSeriesMetrics::recordContinuousAggregateRefresh(const std::string& /*metric_name*/, int64_t /*window_ms*/, 
                                                         double /*latency_ms*/, size_t points_processed) {
    total_continuous_agg_refreshes_.fetch_add(1, std::memory_order_relaxed);
    total_continuous_agg_points_generated_.fetch_add(points_processed, std::memory_order_relaxed);
}

void TimeSeriesMetrics::recordAggRefreshLatency(const std::string& agg_id, double latency_ms) {
    total_continuous_agg_refreshes_.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
    auto& stats = agg_refresh_stats_[agg_id];
    stats.total_latency_ms += latency_ms;
    stats.latency_count++;
}

void TimeSeriesMetrics::recordAggRefreshLag(const std::string& agg_id, double lag_ms) {
    std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
    agg_refresh_stats_[agg_id].last_lag_ms = lag_ms;
}

double TimeSeriesMetrics::getAggRefreshLatency(const std::string& agg_id) const {
    std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
    auto it = agg_refresh_stats_.find(agg_id);
    if (it == agg_refresh_stats_.end() || it->second.latency_count == 0) return -1.0;
    return it->second.total_latency_ms / static_cast<double>(it->second.latency_count);
}

double TimeSeriesMetrics::getAggRefreshLag(const std::string& agg_id) const {
    std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
    auto it = agg_refresh_stats_.find(agg_id);
    if (it == agg_refresh_stats_.end()) return -1.0;
    return it->second.last_lag_ms;
}

std::string TimeSeriesMetrics::exportPrometheus() const {
    std::ostringstream oss;
    
    // Ingestion metrics
    oss << formatPrometheusMetric("themis_timeseries_data_points_written_total", "counter",
                                  "Total number of time series data points written",
                                  total_data_points_written_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_batches_written_total", "counter",
                                  "Total number of batch write operations",
                                  total_batches_written_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_compressed_batches_total", "counter",
                                  "Total number of compressed batch writes",
                                  total_compressed_batches_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_write_errors_total", "counter",
                                  "Total number of write errors",
                                  write_errors_.load());

    oss << formatPrometheusMetric("themis_timeseries_out_of_order_accepted_total", "counter",
                                  "Total out-of-order data points accepted within the late-arrival window",
                                  out_of_order_accepted_.load());

    oss << formatPrometheusMetric("themis_timeseries_late_arrival_rejected_total", "counter",
                                  "Total data points rejected as outside the late-arrival window",
                                  late_arrival_rejected_.load());

    oss << formatPrometheusMetric("themis_timeseries_autobuffer_backpressure_total", "counter",
                                  "Total number of TSAutoBuffer backpressure events (producers blocked)",
                                  total_backpressure_events_.load());

    oss << formatPrometheusMetric("themis_timeseries_autobuffer_overdue_flush_total", "counter",
                                  "Total number of TSAutoBuffer overdue flush events (data held beyond flush interval)",
                                  total_overdue_flush_events_.load());
    
    // Query metrics
    oss << formatPrometheusMetric("themis_timeseries_queries_executed_total", "counter",
                                  "Total number of time series queries executed",
                                  total_queries_executed_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_aggregations_executed_total", "counter",
                                  "Total number of aggregation operations",
                                  total_aggregations_executed_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_data_points_returned_total", "counter",
                                  "Total number of data points returned by queries",
                                  total_data_points_returned_.load());
    
    // Optimizer metrics
    oss << formatPrometheusMetric("themis_timeseries_optimizer_hits_total", "counter",
                                  "Number of times query optimizer successfully optimized a query",
                                  optimizer_hits_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_optimizer_misses_total", "counter",
                                  "Number of times query optimizer could not optimize a query",
                                  optimizer_misses_.load());
    
    // Storage metrics
    oss << formatPrometheusMetric("themis_timeseries_current_data_points", "gauge",
                                  "Current number of data points stored",
                                  current_data_points_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_current_metrics", "gauge",
                                  "Current number of unique metrics",
                                  current_metrics_count_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_current_storage_bytes", "gauge",
                                  "Current storage size in bytes",
                                  current_storage_bytes_.load());
    
    // Retention metrics
    oss << formatPrometheusMetric("themis_timeseries_retention_runs_total", "counter",
                                  "Total number of retention policy executions",
                                  total_retention_runs_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_data_points_deleted_total", "counter",
                                  "Total number of data points deleted by retention",
                                  total_data_points_deleted_.load());
    
    // Continuous aggregate metrics
    oss << formatPrometheusMetric("themis_timeseries_continuous_agg_refreshes_total", "counter",
                                  "Total number of continuous aggregate refreshes",
                                  total_continuous_agg_refreshes_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_continuous_agg_points_generated_total", "counter",
                                  "Total number of aggregate points generated",
                                  total_continuous_agg_points_generated_.load());

    // Per-aggregate refresh latency and lag metrics (labeled by agg_id)
    {
        std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
        bool latency_header_emitted = false;
        bool lag_header_emitted = false;
        for (const auto& [agg_id, stats] : agg_refresh_stats_) {
            if (stats.latency_count > 0) {
                if (!latency_header_emitted) {
                    oss << "# HELP themis_cagg_refresh_latency_ms_avg"
                           " Average incremental refresh latency per aggregate\n"
                           "# TYPE themis_cagg_refresh_latency_ms_avg gauge\n";
                    latency_header_emitted = true;
                }
                double avg_lat = stats.total_latency_ms / static_cast<double>(stats.latency_count);
                oss << "themis_cagg_refresh_latency_ms_avg{agg_id=\"" << agg_id << "\"} "
                    << avg_lat << "\n";
            }
            if (stats.last_lag_ms >= 0.0) {
                if (!lag_header_emitted) {
                    oss << "# HELP themis_cagg_refresh_lag_ms"
                           " Lag between aggregate watermark and wall-clock now\n"
                           "# TYPE themis_cagg_refresh_lag_ms gauge\n";
                    lag_header_emitted = true;
                }
                oss << "themis_cagg_refresh_lag_ms{agg_id=\"" << agg_id << "\"} "
                    << stats.last_lag_ms << "\n";
            }
        }
    }

    // Latency metrics
    oss << formatPrometheusMetric("themis_timeseries_write_latency_ms_avg", "gauge",
                                  "Average write operation latency in milliseconds",
                                  getAverageWriteLatency());
    
    oss << formatPrometheusMetric("themis_timeseries_query_latency_ms_avg", "gauge",
                                  "Average query latency in milliseconds",
                                  getAverageQueryLatency());
    
    // Compression metrics
    double compression_ratio = getAverageCompressionRatio();
    if (compression_ratio > 0) {
        oss << formatPrometheusMetric("themis_timeseries_compression_ratio_avg", "gauge",
                                      "Average compression ratio (uncompressed/compressed)",
                                      compression_ratio);
    }
    
    oss << formatPrometheusMetric("themis_timeseries_bytes_written_uncompressed_total", "counter",
                                  "Total bytes written (uncompressed)",
                                  total_bytes_written_uncompressed_.load());
    
    oss << formatPrometheusMetric("themis_timeseries_bytes_written_compressed_total", "counter",
                                  "Total bytes written (after compression)",
                                  total_bytes_written_compressed_.load());
    
    return oss.str();
}

std::string TimeSeriesMetrics::exportJson() const {
    nlohmann::json j;
    
    // Ingestion metrics
    j["ingestion"]["data_points_written_total"] = total_data_points_written_.load();
    j["ingestion"]["batches_written_total"] = total_batches_written_.load();
    j["ingestion"]["compressed_batches_total"] = total_compressed_batches_.load();
    j["ingestion"]["write_errors_total"] = write_errors_.load();
    j["ingestion"]["write_latency_ms_avg"] = getAverageWriteLatency();
    j["ingestion"]["out_of_order_accepted_total"] = out_of_order_accepted_.load();
    j["ingestion"]["late_arrival_rejected_total"] = late_arrival_rejected_.load();
    
    // Query metrics
    j["query"]["queries_executed_total"] = total_queries_executed_.load();
    j["query"]["aggregations_executed_total"] = total_aggregations_executed_.load();
    j["query"]["data_points_returned_total"] = total_data_points_returned_.load();
    j["query"]["query_latency_ms_avg"] = getAverageQueryLatency();
    
    // Optimizer metrics
    j["optimizer"]["hits_total"] = optimizer_hits_.load();
    j["optimizer"]["misses_total"] = optimizer_misses_.load();
    uint64_t total_opts = optimizer_hits_.load() + optimizer_misses_.load();
    j["optimizer"]["hit_rate"] = total_opts > 0 ? 
        static_cast<double>(optimizer_hits_.load()) / total_opts : 0.0;
    
    // Storage metrics
    j["storage"]["current_data_points"] = current_data_points_.load();
    j["storage"]["current_metrics"] = current_metrics_count_.load();
    j["storage"]["current_storage_bytes"] = current_storage_bytes_.load();
    j["storage"]["bytes_written_uncompressed_total"] = total_bytes_written_uncompressed_.load();
    j["storage"]["bytes_written_compressed_total"] = total_bytes_written_compressed_.load();
    
    double compression_ratio = getAverageCompressionRatio();
    if (compression_ratio > 0) {
        j["storage"]["compression_ratio_avg"] = compression_ratio;
    }
    
    // Retention metrics
    j["retention"]["runs_total"] = total_retention_runs_.load();
    j["retention"]["data_points_deleted_total"] = total_data_points_deleted_.load();
    
    // Continuous aggregate metrics
    j["continuous_aggregates"]["refreshes_total"] = total_continuous_agg_refreshes_.load();
    j["continuous_aggregates"]["points_generated_total"] = total_continuous_agg_points_generated_.load();

    // Per-aggregate refresh latency and lag (incremental path)
    {
        std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
        if (!agg_refresh_stats_.empty()) {
            nlohmann::json per_agg;
            for (const auto& [agg_id, stats] : agg_refresh_stats_) {
                nlohmann::json entry;
                entry["avg_refresh_latency_ms"] = stats.latency_count > 0
                    ? stats.total_latency_ms / static_cast<double>(stats.latency_count)
                    : 0.0;
                entry["refresh_count"] = stats.latency_count;
                entry["last_lag_ms"] = stats.last_lag_ms;
                per_agg[agg_id] = entry;
            }
            j["continuous_aggregates"]["per_aggregate"] = per_agg;
        }
    }
    
    // Per-metric statistics
    if (config_.enable_per_metric_stats) {
        std::lock_guard<std::mutex> lock(per_metric_mutex_);
        nlohmann::json per_metric_json;
        for (const auto& [metric_name, stats] : per_metric_stats_) {
            nlohmann::json metric_stats;
            metric_stats["data_points_written"] = stats.data_points_written;
            metric_stats["queries_executed"] = stats.queries_executed;
            metric_stats["bytes_written"] = stats.bytes_written;
            metric_stats["avg_write_latency_ms"] = stats.data_points_written > 0 ?
                stats.total_write_latency_ms / stats.data_points_written : 0.0;
            metric_stats["avg_query_latency_ms"] = stats.queries_executed > 0 ?
                stats.total_query_latency_ms / stats.queries_executed : 0.0;
            per_metric_json[metric_name] = metric_stats;
        }
        j["per_metric_stats"] = per_metric_json;
    }
    
    return j.dump(2);
}

void TimeSeriesMetrics::reset() {
    total_data_points_written_.store(0);
    total_batches_written_.store(0);
    total_compressed_batches_.store(0);
    write_errors_.store(0);
    total_bytes_written_uncompressed_.store(0);
    total_bytes_written_compressed_.store(0);
    out_of_order_accepted_.store(0);
    late_arrival_rejected_.store(0);
    total_backpressure_events_.store(0);
    total_overdue_flush_events_.store(0);
    
    total_queries_executed_.store(0);
    total_aggregations_executed_.store(0);
    total_data_points_returned_.store(0);
    optimizer_hits_.store(0);
    optimizer_misses_.store(0);
    
    current_data_points_.store(0);
    current_metrics_count_.store(0);
    current_storage_bytes_.store(0);
    
    total_retention_runs_.store(0);
    total_data_points_deleted_.store(0);
    
    total_continuous_agg_refreshes_.store(0);
    total_continuous_agg_points_generated_.store(0);
    
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        total_write_latency_ms_ = 0.0;
        write_latency_count_ = 0;
        total_query_latency_ms_ = 0.0;
        query_latency_count_ = 0;
        total_aggregation_latency_ms_ = 0.0;
        aggregation_latency_count_ = 0;
    }
    
    {
        std::lock_guard<std::mutex> lock(per_metric_mutex_);
        per_metric_stats_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(agg_metrics_mutex_);
        agg_refresh_stats_.clear();
    }
}

double TimeSeriesMetrics::getAverageWriteLatency() const {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    return getAverageLatency(total_write_latency_ms_, write_latency_count_);
}

double TimeSeriesMetrics::getAverageQueryLatency() const {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    return getAverageLatency(total_query_latency_ms_, query_latency_count_);
}

double TimeSeriesMetrics::getAverageCompressionRatio() const {
    uint64_t compressed = total_bytes_written_compressed_.load();
    uint64_t uncompressed = total_bytes_written_uncompressed_.load();
    
    if (compressed > 0) {
        return static_cast<double>(uncompressed) / compressed;
    }
    return 0.0;
}

void TimeSeriesMetrics::recordLatency(double& total_latency, uint64_t& count, double latency_ms) {
    total_latency += latency_ms;
    count++;
}

double TimeSeriesMetrics::getAverageLatency(double total_latency, uint64_t count) const {
    return count > 0 ? total_latency / count : 0.0;
}

std::string TimeSeriesMetrics::formatPrometheusMetric(const std::string& name, const std::string& type,
                                                      const std::string& help, uint64_t value) const {
    std::ostringstream oss;
    oss << "# HELP " << name << " " << help << "\n";
    oss << "# TYPE " << name << " " << type << "\n";
    oss << name << " " << value << "\n\n";
    return oss.str();
}

std::string TimeSeriesMetrics::formatPrometheusMetric(const std::string& name, const std::string& type,
                                                      const std::string& help, double value) const {
    std::ostringstream oss;
    oss << "# HELP " << name << " " << help << "\n";
    oss << "# TYPE " << name << " " << type << "\n";
    oss << name << " " << std::fixed << std::setprecision(6) << value << "\n\n";
    return oss.str();
}

} // namespace themis
