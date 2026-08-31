/**
 * @file grafana_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=5, Debt=0, C=4, H=32, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/grafana_metrics.h"
#ifdef THEMIS_HAS_HTTPLIB
#include <httplib.h>
#endif
#include <spdlog/spdlog.h>
#include "utils/thread_join_utils.h"
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <thread>

namespace themis {
namespace llm {
namespace monitoring {

// Configuration constants
constexpr size_t MAX_HISTOGRAM_SAMPLES = 1000;

// PrometheusExporter Implementation
PrometheusExporter::PrometheusExporter() {
    spdlog::debug("PrometheusExporter initialized");
}

PrometheusExporter::~PrometheusExporter() = default;

void PrometheusExporter::registerMetric(const MetricDefinition& def) {
    std::lock_guard<std::mutex> lock(mutex_);
    registered_metrics_[def.name] = def;
    spdlog::debug("Metric registered: {} (type: {})", def.name, static_cast<int>(def.type));
}

void PrometheusExporter::incrementCounter(const std::string& name,
                                         const std::unordered_map<std::string, std::string>& labels,
                                         double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeMetricKey(name, labels);
    
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        it->second.value += value;
    } else {
        MetricValue mv;
        mv.type = MetricType::COUNTER;
        mv.value = value;
        mv.last_updated = std::chrono::system_clock::now();
        metrics_[key] = mv;
    }
}

void PrometheusExporter::setGauge(const std::string& name, double value,
                                 const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeMetricKey(name, labels);
    
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        it->second.value = value;
    } else {
        MetricValue mv;
        mv.type = MetricType::GAUGE;
        mv.value = value;
        mv.last_updated = std::chrono::system_clock::now();
        metrics_[key] = mv;
    }
}

void PrometheusExporter::incrementGauge(const std::string& name, double delta,
                                       const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeMetricKey(name, labels);
    
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        it->second.value += delta;
    } else {
        MetricValue mv;
        mv.type = MetricType::GAUGE;
        mv.value = delta;
        mv.last_updated = std::chrono::system_clock::now();
        metrics_[key] = mv;
    }
}

void PrometheusExporter::observeHistogram(const std::string& name, double value,
                                         const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeMetricKey(name, labels);
    
    auto it = metrics_.find(key);
    if (it != metrics_.end()) {
        it->second.histogram_buckets.push_back(value);
        // Keep only recent values (max MAX_HISTOGRAM_SAMPLES)
        if (it->second.histogram_buckets.size() > MAX_HISTOGRAM_SAMPLES) {
            it->second.histogram_buckets.erase(it->second.histogram_buckets.begin());
        }
    } else {
        MetricValue mv;
        mv.type = MetricType::HISTOGRAM;
        mv.value = 0;
        mv.histogram_buckets.push_back(value);
        mv.last_updated = std::chrono::system_clock::now();
        metrics_[key] = mv;
    }
}

std::string PrometheusExporter::exportMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Group metrics by base name for HELP and TYPE annotations
    std::map<std::string, MetricType> metric_types;
    for (const auto& [key, value] : metrics_) {
        // Extract base metric name (before any labels)
        size_t brace_pos = key.find('{');
        std::string base_name = (brace_pos != std::string::npos) ? key.substr(0, brace_pos) : key;
        metric_types[base_name] = value.type;
    }
    
    // Output HELP and TYPE annotations
    for (const auto& [name, type] : metric_types) {
        auto reg_it = registered_metrics_.find(name);
        if (reg_it != registered_metrics_.end()) {
            oss << "# HELP " << name << " " << reg_it->second.help << "\n";
        } else {
            oss << "# HELP " << name << " " << name << "\n";
        }
        
        switch (type) {
            case MetricType::COUNTER:
                oss << "# TYPE " << name << " counter\n";
                break;
            case MetricType::GAUGE:
                oss << "# TYPE " << name << " gauge\n";
                break;
            case MetricType::HISTOGRAM:
                oss << "# TYPE " << name << " histogram\n";
                break;
            case MetricType::SUMMARY:
                oss << "# TYPE " << name << " summary\n";
                break;
        }
    }
    
    // Output metric values
    for (const auto& [key, value] : metrics_) {
        if (value.type == MetricType::HISTOGRAM) {
            // For histograms, output quantiles
            if (!value.histogram_buckets.empty()) {
                auto sorted = value.histogram_buckets;
                std::sort(sorted.begin(), sorted.end());
                
                size_t count = sorted.size();
                double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
                
                // Extract base name and labels
                size_t brace_pos = key.find('{');
                std::string base_name = (brace_pos != std::string::npos) ? key.substr(0, brace_pos) : key;
                std::string labels_part = (brace_pos != std::string::npos) ? key.substr(brace_pos) : "";
                
                // Remove trailing } from labels if present
                if (!labels_part.empty() && labels_part.back() == '}') {
                    labels_part.pop_back();
                }
                
                // Output quantiles
                if (count > 0) {
                    // Use safe percentile calculation to avoid out-of-bounds
                    size_t idx_p50 = std::min(count - 1, count * 50 / 100);
                    size_t idx_p95 = std::min(count - 1, count * 95 / 100);
                    size_t idx_p99 = std::min(count - 1, count * 99 / 100);
                    
                    auto p50 = sorted[idx_p50];
                    auto p95 = sorted[idx_p95];
                    auto p99 = sorted[idx_p99];
                    
                    std::string q_labels = labels_part.empty() ? "{" : labels_part + ",";
                    oss << base_name << q_labels << "quantile=\"0.5\"} " << p50 << "\n";
                    oss << base_name << q_labels << "quantile=\"0.95\"} " << p95 << "\n";
                    oss << base_name << q_labels << "quantile=\"0.99\"} " << p99 << "\n";
                }
                
                // Output sum and count
                std::string sum_count_labels = labels_part.empty() ? "" : labels_part + "}";
                oss << base_name << "_sum" << sum_count_labels << " " << sum << "\n";
                oss << base_name << "_count" << sum_count_labels << " " << count << "\n";
            }
        } else {
            // For counters and gauges, output simple value
            oss << key << " " << value.value << "\n";
        }
    }
    
    return oss.str();
}

std::string PrometheusExporter::handleMetricsRequest() const {
    return exportMetrics();
}

void PrometheusExporter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
    spdlog::debug("Metrics reset");
}

std::string PrometheusExporter::makeMetricKey(const std::string& name,
                                              const std::unordered_map<std::string, std::string>& labels) const {
    if (labels.empty()) {
        return name;
    }
    
    std::ostringstream oss;
    oss << name << "{";
    bool first = true;
    
    // Sort labels for consistent key generation
    std::map<std::string, std::string> sorted_labels(labels.begin(), labels.end());
    
    for (const auto& [key, value] : sorted_labels) {
        if (!first) oss << ",";
        oss << key << "=\"" << value << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string PrometheusExporter::serializeMetric(const std::string& name, const MetricValue& value) const {
    std::ostringstream oss;
    oss << name << " " << value.value;
    return oss.str();
}

// LLMMetricsCollector Implementation
LLMMetricsCollector::LLMMetricsCollector(PrometheusExporter* exporter)
    : exporter_(exporter), config_{} {
    spdlog::debug("LLMMetricsCollector initialized");
    initializeMetrics();
}

LLMMetricsCollector::LLMMetricsCollector(PrometheusExporter* exporter, const Config& config)
    : exporter_(exporter), config_(config) {
    spdlog::debug("LLMMetricsCollector initialized (lock_contention_threshold_ms={})",
                  config_.lock_contention_threshold_ms);
    initializeMetrics();
}

void LLMMetricsCollector::initializeMetrics() {
    // Register all metrics with proper definitions
    
    // Inference metrics
    exporter_->registerMetric({
        "llm_inference_requests_total",
        "Total number of LLM inference requests",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_inference_success_total",
        "Total number of successful LLM inference requests",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_inference_failures_total",
        "Total number of failed LLM inference requests",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "error"}
    });
    
    // Latency metrics
    exporter_->registerMetric({
        "llm_first_token_latency_ms",
        "Time to first token in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_per_token_latency_ms",
        "Per-token generation latency in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_end_to_end_latency_ms",
        "End-to-end inference latency in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_inference_duration_ms",
        "Total inference duration in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    // Token metrics
    exporter_->registerMetric({
        "llm_tokens_generated_total",
        "Total number of tokens generated",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
    
    // Throughput metrics
    exporter_->registerMetric({
        "llm_batch_size",
        "Current batch size",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_concurrent_requests",
        "Number of concurrent requests",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    // GPU metrics
    exporter_->registerMetric({
        "llm_gpu_memory_used_mb",
        "GPU memory used in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_gpu_memory_total_mb",
        "Total GPU memory in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_gpu_utilization_percent",
        "GPU utilization percentage",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_gpu_temperature_celsius",
        "GPU temperature in Celsius",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    // Model metrics
    exporter_->registerMetric({
        "llm_models_loaded",
        "Number of loaded models",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_model_memory_mb",
        "Model memory usage in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_model_switch_latency_ms",
        "Model switching latency in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {}
    });
    
    // Cache metrics
    exporter_->registerMetric({
        "llm_cache_hits_total",
        "Total cache hits",
        PrometheusExporter::MetricType::COUNTER,
        {"cache_type"}
    });
    
    exporter_->registerMetric({
        "llm_cache_misses_total",
        "Total cache misses",
        PrometheusExporter::MetricType::COUNTER,
        {"cache_type"}
    });
    
    exporter_->registerMetric({
        "llm_cache_size_mb",
        "Cache size in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"cache_type"}
    });
    
    // Scheduler metrics
    exporter_->registerMetric({
        "llm_queue_length",
        "Current queue length",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_preemptions_total",
        "Total number of preemptions",
        PrometheusExporter::MetricType::COUNTER,
        {}
    });
    
    exporter_->registerMetric({
        "llm_scheduling_latency_ms",
        "Scheduling latency in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {}
    });
    
    // Quantization metrics
    exporter_->registerMetric({
        "llm_quantization_format",
        "Quantization format (as info metric)",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id", "format"}
    });
    
    exporter_->registerMetric({
        "llm_dequantization_latency_ms",
        "Dequantization latency in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {}
    });
    
    // Error metrics
    exporter_->registerMetric({
        "llm_errors_total",
        "Total errors",
        PrometheusExporter::MetricType::COUNTER,
        {"error_type", "component"}
    });
    
    // Backpressure metrics
    exporter_->registerMetric({
        "llm_backpressure_drops_total",
        "Total inference requests rejected due to queue depth limit (backpressure)",
        PrometheusExporter::MetricType::COUNTER,
        {}
    });

    // Shared Worker Pool metrics (Phase 2 — Q2 2026)
    exporter_->registerMetric({
        "llm_worker_pool_queue_depth",
        "Current number of tasks pending in the shared worker pool",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });

    exporter_->registerMetric({
        "llm_worker_pool_tasks_completed_total",
        "Total tasks completed by the shared worker pool since start",
        PrometheusExporter::MetricType::COUNTER,
        {}
    });

    // Unified dashboard / engine-typed metrics (Phase 2 — Q3 2026)
    // engine_type label: "async" = AsyncInferenceEngine,
    //                    "enhanced" = InferenceEngineEnhanced
    exporter_->registerMetric({
        "llm_engine_inference_requests_total",
        "Total inference requests per engine type",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "engine_type"}
    });

    exporter_->registerMetric({
        "llm_engine_inference_success_total",
        "Total successful inference requests per engine type",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "engine_type"}
    });

    exporter_->registerMetric({
        "llm_engine_inference_failures_total",
        "Total failed inference requests per engine type",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "engine_type", "error"}
    });

    exporter_->registerMetric({
        "llm_engine_inference_duration_ms",
        "Inference duration histogram per engine type",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id", "engine_type"}
    });

    exporter_->registerMetric({
        "llm_engine_tokens_generated_total",
        "Total tokens generated per engine type",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "engine_type"}
    });

    exporter_->registerMetric({
        "llm_engine_queue_depth",
        "Current request queue depth per engine type",
        PrometheusExporter::MetricType::GAUGE,
        {"engine_type"}
    });

    // Initialize extended context and RoPE/YARN metrics (v1.4.0+)
    initializeExtendedContextMetrics();
}

void LLMMetricsCollector::recordInferenceRequest(const std::string& model_id) {
    exporter_->incrementCounter("llm_inference_requests_total", {{"model_id", model_id}});
}

void LLMMetricsCollector::recordInferenceSuccess(const std::string& model_id, double duration_ms) {
    exporter_->incrementCounter("llm_inference_success_total", {{"model_id", model_id}});
    exporter_->observeHistogram("llm_inference_duration_ms", duration_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordInferenceFailure(const std::string& model_id, const std::string& error) {
    exporter_->incrementCounter("llm_inference_failures_total", {{"model_id", model_id}, {"error", error}});
}

void LLMMetricsCollector::recordFirstTokenLatency(const std::string& model_id, double latency_ms) {
    exporter_->observeHistogram("llm_first_token_latency_ms", latency_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordPerTokenLatency(const std::string& model_id, double latency_ms) {
    exporter_->observeHistogram("llm_per_token_latency_ms", latency_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordEndToEndLatency(const std::string& model_id, double latency_ms) {
    exporter_->observeHistogram("llm_end_to_end_latency_ms", latency_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordTokensGenerated(const std::string& model_id, size_t count) {
    exporter_->incrementCounter("llm_tokens_generated_total", {{"model_id", model_id}}, static_cast<double>(count));
}

void LLMMetricsCollector::recordBatchSize(size_t batch_size) {
    exporter_->setGauge("llm_batch_size", static_cast<double>(batch_size));
}

void LLMMetricsCollector::recordConcurrentRequests(size_t count) {
    exporter_->setGauge("llm_concurrent_requests", static_cast<double>(count));
}

void LLMMetricsCollector::recordGPUMemoryUsage(size_t vram_mb, size_t total_vram_mb) {
    exporter_->setGauge("llm_gpu_memory_used_mb", static_cast<double>(vram_mb));
    exporter_->setGauge("llm_gpu_memory_total_mb", static_cast<double>(total_vram_mb));
}

void LLMMetricsCollector::recordGPUUtilization(double utilization_pct) {
    exporter_->setGauge("llm_gpu_utilization_percent", utilization_pct);
}

void LLMMetricsCollector::recordGPUTemperature(double temp_celsius) {
    exporter_->setGauge("llm_gpu_temperature_celsius", temp_celsius);
}

void LLMMetricsCollector::recordModelLoaded(const std::string& model_id, size_t vram_mb) {
    exporter_->incrementGauge("llm_models_loaded", 1.0);
    exporter_->setGauge("llm_model_memory_mb", static_cast<double>(vram_mb), {{"model_id", model_id}});
}

void LLMMetricsCollector::recordModelUnloaded(const std::string& model_id) {
    exporter_->incrementGauge("llm_models_loaded", -1.0);
    exporter_->setGauge("llm_model_memory_mb", 0.0, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordModelSwitchLatency(double latency_ms) {
    exporter_->observeHistogram("llm_model_switch_latency_ms", latency_ms);
}

void LLMMetricsCollector::recordCacheHit(const std::string& cache_type) {
    exporter_->incrementCounter("llm_cache_hits_total", {{"cache_type", cache_type}});
}

void LLMMetricsCollector::recordCacheMiss(const std::string& cache_type) {
    exporter_->incrementCounter("llm_cache_misses_total", {{"cache_type", cache_type}});
}

void LLMMetricsCollector::recordCacheSize(const std::string& cache_type, size_t size_mb) {
    exporter_->setGauge("llm_cache_size_mb", static_cast<double>(size_mb), {{"cache_type", cache_type}});
}

void LLMMetricsCollector::recordQueueLength(size_t length) {
    exporter_->setGauge("llm_queue_length", static_cast<double>(length));
}

void LLMMetricsCollector::recordPreemptions(size_t count) {
    exporter_->incrementCounter("llm_preemptions_total", {}, static_cast<double>(count));
}

void LLMMetricsCollector::recordSchedulingLatency(double latency_ms) {
    exporter_->observeHistogram("llm_scheduling_latency_ms", latency_ms);
}

void LLMMetricsCollector::recordBackpressureDrop() {
    exporter_->incrementCounter("llm_backpressure_drops_total");
}

void LLMMetricsCollector::recordQuantizationFormat(const std::string& model_id, const std::string& format) {
    exporter_->setGauge("llm_quantization_format", 1.0, {{"model_id", model_id}, {"format", format}});
}

void LLMMetricsCollector::recordDequantizationLatency(double latency_ms) {
    exporter_->observeHistogram("llm_dequantization_latency_ms", latency_ms);
}

void LLMMetricsCollector::recordError(const std::string& error_type, const std::string& component) {
    exporter_->incrementCounter("llm_errors_total", {{"error_type", error_type}, {"component", component}});
}

// Extended Context Window metrics (v1.4.0+)
void LLMMetricsCollector::recordContextLength(const std::string& model_id, size_t context_length) {
    exporter_->setGauge("llm_context_length", static_cast<double>(context_length), {{"model_id", model_id}});
    exporter_->observeHistogram("llm_context_length_histogram", static_cast<double>(context_length), {{"model_id", model_id}});
}

void LLMMetricsCollector::recordContextCacheSize(const std::string& model_id, size_t cache_size_mb) {
    exporter_->setGauge("llm_context_cache_size_mb", static_cast<double>(cache_size_mb), {{"model_id", model_id}});
}

void LLMMetricsCollector::recordExtendedContextEnabled(const std::string& model_id, bool enabled) {
    exporter_->setGauge("llm_extended_context_enabled", enabled ? 1.0 : 0.0, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordContextScalingFactor(const std::string& model_id, double scaling_factor) {
    exporter_->setGauge("llm_context_scaling_factor", scaling_factor, {{"model_id", model_id}});
}

// RoPE/YARN Scaling metrics (v1.4.0+)
void LLMMetricsCollector::recordRoPEScalingMethod(const std::string& model_id, const std::string& method) {
    exporter_->setGauge("llm_rope_scaling_method", 1.0, {{"model_id", model_id}, {"method", method}});
}

void LLMMetricsCollector::recordRoPEScalingError(const std::string& model_id, const std::string& error) {
    exporter_->incrementCounter("llm_rope_scaling_errors_total", {{"model_id", model_id}, {"error", error}});
}

void LLMMetricsCollector::recordYARNParameters(const std::string& model_id, 
                                               double ext_factor, double attn_factor,
                                               double beta_fast, double beta_slow) {
    exporter_->setGauge("llm_yarn_ext_factor", ext_factor, {{"model_id", model_id}});
    exporter_->setGauge("llm_yarn_attn_factor", attn_factor, {{"model_id", model_id}});
    exporter_->setGauge("llm_yarn_beta_fast", beta_fast, {{"model_id", model_id}});
    exporter_->setGauge("llm_yarn_beta_slow", beta_slow, {{"model_id", model_id}});
}

// Memory Profiling metrics (v1.4.0+)
void LLMMetricsCollector::recordRAMUsage(const std::string& model_id, size_t ram_mb, size_t total_ram_mb) {
    exporter_->setGauge("llm_ram_used_mb", static_cast<double>(ram_mb), {{"model_id", model_id}});
    exporter_->setGauge("llm_ram_total_mb", static_cast<double>(total_ram_mb), {{"model_id", model_id}});
    
    // Calculate and record RAM usage percentage
    if (total_ram_mb > 0) {
        double usage_pct = (static_cast<double>(ram_mb) / static_cast<double>(total_ram_mb)) * 100.0;
        exporter_->setGauge("llm_ram_usage_percent", usage_pct, {{"model_id", model_id}});
    }
}

void LLMMetricsCollector::recordVRAMUsage(const std::string& model_id, size_t vram_mb, size_t total_vram_mb) {
    exporter_->setGauge("llm_vram_used_mb", static_cast<double>(vram_mb), {{"model_id", model_id}});
    exporter_->setGauge("llm_vram_total_mb", static_cast<double>(total_vram_mb), {{"model_id", model_id}});
    
    // Calculate and record VRAM usage percentage
    if (total_vram_mb > 0) {
        double usage_pct = (static_cast<double>(vram_mb) / static_cast<double>(total_vram_mb)) * 100.0;
        exporter_->setGauge("llm_vram_usage_percent", usage_pct, {{"model_id", model_id}});
    }
}

void LLMMetricsCollector::recordMemoryPressure(const std::string& model_id, double pressure_pct) {
    exporter_->setGauge("llm_memory_pressure_percent", pressure_pct, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordOOMEvent(const std::string& model_id, const std::string& reason) {
    exporter_->incrementCounter("llm_oom_events_total", {{"model_id", model_id}, {"reason", reason}});
}

void LLMMetricsCollector::recordMemoryEstimate(const std::string& model_id, 
                                               size_t estimated_mb, size_t actual_mb) {
    exporter_->setGauge("llm_memory_estimated_mb", static_cast<double>(estimated_mb), {{"model_id", model_id}});
    exporter_->setGauge("llm_memory_actual_mb", static_cast<double>(actual_mb), {{"model_id", model_id}});
    
    // Calculate estimation accuracy with proper bounds checking
    if (estimated_mb > 10) {  // Require at least 10MB to avoid precision issues
        double accuracy_pct = (static_cast<double>(actual_mb) / static_cast<double>(estimated_mb)) * 100.0;
        // Cap accuracy at reasonable bounds (50-200%) to avoid misleading values
        accuracy_pct = std::min(std::max(accuracy_pct, 50.0), 200.0);
        exporter_->setGauge("llm_memory_estimation_accuracy_percent", accuracy_pct, {{"model_id", model_id}});
    } else {
        // Set to 100% if estimate is too small to be meaningful
        exporter_->setGauge("llm_memory_estimation_accuracy_percent", 100.0, {{"model_id", model_id}});
    }
}

// Thread Safety metrics (v1.4.0+)
void LLMMetricsCollector::recordLoRAAdapterSwitch(const std::string& model_id, 
                                                  const std::string& from_adapter,
                                                  const std::string& to_adapter,
                                                  double duration_ms) {
    exporter_->incrementCounter("llm_lora_adapter_switches_total", 
                               {{"model_id", model_id}, {"from", from_adapter}, {"to", to_adapter}});
    exporter_->observeHistogram("llm_lora_adapter_switch_duration_ms", duration_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordContextLockWait(const std::string& model_id, double wait_time_ms) {
    exporter_->observeHistogram("llm_context_lock_wait_ms", wait_time_ms, {{"model_id", model_id}});
    
    if (wait_time_ms > config_.lock_contention_threshold_ms) {
        exporter_->incrementCounter("llm_context_lock_contention_total", {{"model_id", model_id}});
    }
}

void LLMMetricsCollector::recordConcurrentLoRAOperation(const std::string& model_id, bool sequential_mode) {
    exporter_->setGauge("llm_lora_sequential_mode", sequential_mode ? 1.0 : 0.0, {{"model_id", model_id}});
    
    if (!sequential_mode) {
        // Warn about potential thread-safety issues
        exporter_->incrementCounter("llm_lora_concurrent_operations_total", {{"model_id", model_id}});
    }
}

// ─── Shared Worker Pool metrics (Phase 2) ────────────────────────────────────

void LLMMetricsCollector::recordWorkerPoolQueueDepth(size_t depth) {
    exporter_->setGauge("llm_worker_pool_queue_depth",
                        static_cast<double>(depth));
}

void LLMMetricsCollector::recordWorkerPoolTasksCompleted(uint64_t total_completed) {
    // Compute delta against last reported total and increment the counter.
    // Uses compare-exchange to avoid losing increments under concurrent callers.
    //
    // If total_completed == prev: delta is 0, no-op — intentional (no new completions).
    // If total_completed < prev:  pool was recreated / counter reset; treat
    //                             total_completed as a fresh delta from zero.
    uint64_t prev = last_pool_tasks_completed_.load(std::memory_order_relaxed);
    while (true) {
        uint64_t new_val;
        double   delta;
        if (total_completed >= prev) {
            delta   = static_cast<double>(total_completed - prev);
            new_val = total_completed;
        } else {
            // Counter reset detected: treat total_completed as the new delta.
            spdlog::debug("SharedWorkerPool tasks counter reset detected "
                          "(prev={}, new={}); recording {} as delta",
                          prev, total_completed, total_completed);
            delta   = static_cast<double>(total_completed);
            new_val = total_completed;
        }

        if (delta == 0.0) break;  // nothing new to report

        if (last_pool_tasks_completed_.compare_exchange_weak(
                prev, new_val,
                std::memory_order_release,
                std::memory_order_relaxed)) {
            exporter_->incrementCounter("llm_worker_pool_tasks_completed_total",
                                        {}, delta);
            break;
        }
        // prev was refreshed by compare_exchange_weak — retry
    }
}

// ─── Unified dashboard / engine-typed metrics (Phase 2 — Q3 2026) ────────────

void LLMMetricsCollector::recordEngineInferenceRequest(const std::string& model_id,
                                                        const std::string& engine_type) {
    exporter_->incrementCounter("llm_engine_inference_requests_total",
                                {{"model_id", model_id}, {"engine_type", engine_type}});
}

void LLMMetricsCollector::recordEngineInferenceSuccess(const std::string& model_id,
                                                        const std::string& engine_type,
                                                        double duration_ms) {
    exporter_->incrementCounter("llm_engine_inference_success_total",
                                {{"model_id", model_id}, {"engine_type", engine_type}});
    exporter_->observeHistogram("llm_engine_inference_duration_ms", duration_ms,
                                {{"model_id", model_id}, {"engine_type", engine_type}});
}

void LLMMetricsCollector::recordEngineInferenceFailure(const std::string& model_id,
                                                        const std::string& engine_type,
                                                        const std::string& error) {
    exporter_->incrementCounter("llm_engine_inference_failures_total",
                                {{"model_id", model_id}, {"engine_type", engine_type},
                                 {"error", error}});
}

void LLMMetricsCollector::recordEngineTokensGenerated(const std::string& model_id,
                                                       const std::string& engine_type,
                                                       size_t count) {
    exporter_->incrementCounter("llm_engine_tokens_generated_total",
                                {{"model_id", model_id}, {"engine_type", engine_type}},
                                static_cast<double>(count));
}

void LLMMetricsCollector::recordEngineQueueDepth(const std::string& engine_type,
                                                  size_t depth) {
    exporter_->setGauge("llm_engine_queue_depth", static_cast<double>(depth),
                        {{"engine_type", engine_type}});
}

// Initialize extended context metrics (v1.4.0+)
void LLMMetricsCollector::initializeExtendedContextMetrics() {
    // Extended Context Window metrics
    exporter_->registerMetric({
        "llm_context_length",
        "Current context window length in tokens",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_context_length_histogram",
        "Distribution of context window lengths",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_context_cache_size_mb",
        "Size of KV cache in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_extended_context_enabled",
        "Whether extended context is enabled (1=yes, 0=no)",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_context_scaling_factor",
        "Context window scaling factor (e.g., 8.0 for 32K/4K)",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    // RoPE/YARN Scaling metrics
    exporter_->registerMetric({
        "llm_rope_scaling_method",
        "RoPE scaling method used (linear, ntk, yarn, dynamic)",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id", "method"}
    });
    
    exporter_->registerMetric({
        "llm_rope_scaling_errors_total",
        "Total number of RoPE scaling errors",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "error"}
    });
    
    exporter_->registerMetric({
        "llm_yarn_ext_factor",
        "YaRN extension factor parameter",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_yarn_attn_factor",
        "YaRN attention factor parameter",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_yarn_beta_fast",
        "YaRN beta fast parameter",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_yarn_beta_slow",
        "YaRN beta slow parameter",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    // Memory Profiling metrics
    exporter_->registerMetric({
        "llm_ram_used_mb",
        "RAM used by model in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_ram_total_mb",
        "Total available RAM in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_ram_usage_percent",
        "RAM usage percentage",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_vram_used_mb",
        "VRAM used by model in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_vram_total_mb",
        "Total available VRAM in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_vram_usage_percent",
        "VRAM usage percentage",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_memory_pressure_percent",
        "Memory pressure percentage (0-100)",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_oom_events_total",
        "Total number of Out-of-Memory events",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "reason"}
    });
    
    exporter_->registerMetric({
        "llm_memory_estimated_mb",
        "Estimated memory requirement in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_memory_actual_mb",
        "Actual memory usage in megabytes",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_memory_estimation_accuracy_percent",
        "Memory estimation accuracy percentage",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    // Thread Safety metrics
    exporter_->registerMetric({
        "llm_lora_adapter_switches_total",
        "Total number of LoRA adapter switches",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id", "from", "to"}
    });
    
    exporter_->registerMetric({
        "llm_lora_adapter_switch_duration_ms",
        "Duration of LoRA adapter switch in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_context_lock_wait_ms",
        "Time waiting for context lock in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_context_lock_contention_total",
        "Total number of context lock contention events (wait exceeds configured threshold)",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_lora_sequential_mode",
        "Whether LoRA operations are in sequential mode (1=yes, 0=no)",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_lora_concurrent_operations_total",
        "Total number of concurrent LoRA operations (potential thread-safety issues)",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
}

// GrafanaDashboardGenerator Implementation
GrafanaDashboardGenerator::GrafanaDashboardGenerator(const DashboardConfig& config)
    : config_(config) {
    spdlog::debug("GrafanaDashboardGenerator initialized");
}

std::string GrafanaDashboardGenerator::generateDashboard() const {
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"dashboard\": {\n";
    oss << "    \"title\": \"" << config_.title << "\",\n";
    oss << "    \"tags\": [\"llm\", \"themisdb\"],\n";
    oss << "    \"timezone\": \"browser\",\n";
    oss << "    \"schemaVersion\": 16,\n";
    oss << "    \"version\": 1,\n";
    oss << "    \"refresh\": \"" << config_.refresh_interval_sec << "s\",\n";
    oss << "    \"panels\": [\n";
    
    // Panel positions
    int y_pos = 0;
    
    // Inference panel
    oss << createPanel("Inference Requests", 
                       "rate(llm_inference_requests_total[5m])",
                       "graph", 0, y_pos, 12, 8);
    oss << ",\n";
    y_pos += 8;
    
    // Latency panel
    oss << createPanel("Request Latency (p95)",
                       "histogram_quantile(0.95, rate(llm_inference_duration_ms_bucket[5m]))",
                       "graph", 0, y_pos, 12, 8);
    oss << ",\n";
    y_pos += 8;
    
    // Throughput panel
    oss << createPanel("Tokens per Second",
                       "rate(llm_tokens_generated_total[5m])",
                       "graph", 0, y_pos, 6, 8);
    oss << ",\n";
    
    // GPU panel
    oss << createPanel("GPU Memory Usage (MB)",
                       "llm_gpu_memory_used_mb",
                       "graph", 6, y_pos, 6, 8);
    oss << ",\n";
    y_pos += 8;
    
    // Cache panel
    oss << createPanel("Cache Hit Rate",
                       "rate(llm_cache_hits_total[5m]) / (rate(llm_cache_hits_total[5m]) + rate(llm_cache_misses_total[5m]))",
                       "gauge", 0, y_pos, 6, 4);
    oss << ",\n";
    
    // Error panel
    oss << createPanel("Error Rate",
                       "rate(llm_errors_total[5m])",
                       "graph", 6, y_pos, 6, 4);
    oss << "\n";
    
    oss << "    ]\n";
    oss << "  }\n";
    oss << "}\n";
    
    return oss.str();
}

std::string GrafanaDashboardGenerator::createPanel(const std::string& title,
                                                   const std::string& query,
                                                   const std::string& type,
                                                   int grid_pos_x, int grid_pos_y,
                                                   int grid_width, int grid_height) const {
    std::ostringstream oss;
    
    oss << "      {\n";
    oss << "        \"title\": \"" << title << "\",\n";
    oss << "        \"type\": \"" << type << "\",\n";
    oss << "        \"datasource\": \"" << config_.datasource << "\",\n";
    oss << "        \"gridPos\": {\n";
    oss << "          \"x\": " << grid_pos_x << ",\n";
    oss << "          \"y\": " << grid_pos_y << ",\n";
    oss << "          \"w\": " << grid_width << ",\n";
    oss << "          \"h\": " << grid_height << "\n";
    oss << "        },\n";
    oss << "        \"targets\": [\n";
    oss << "          {\n";
    oss << "            \"expr\": \"" << query << "\",\n";
    oss << "            \"refId\": \"A\"\n";
    oss << "          }\n";
    oss << "        ]\n";
    oss << "      }";
    
    return oss.str();
}

std::string GrafanaDashboardGenerator::generateInferencePanel() const {
    return createPanel("Inference Requests Rate",
                       "rate(llm_inference_requests_total[5m])",
                       "graph", 0, 0, 12, 8);
}

std::string GrafanaDashboardGenerator::generateLatencyPanel() const {
    return createPanel("First Token Latency (p95)",
                       "histogram_quantile(0.95, rate(llm_first_token_latency_ms_bucket[5m]))",
                       "graph", 0, 8, 12, 8);
}

std::string GrafanaDashboardGenerator::generateThroughputPanel() const {
    return createPanel("Token Generation Rate",
                       "rate(llm_tokens_generated_total[5m])",
                       "graph", 0, 16, 12, 8);
}

std::string GrafanaDashboardGenerator::generateGPUPanel() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"title\": \"GPU Metrics\",\n";
    oss << "  \"panels\": [\n";
    oss << createPanel("GPU Memory", "llm_gpu_memory_used_mb", "graph", 0, 0, 6, 6);
    oss << ",\n";
    oss << createPanel("GPU Utilization", "llm_gpu_utilization_percent", "gauge", 6, 0, 6, 6);
    oss << "\n  ]\n";
    oss << "}\n";
    return oss.str();
}

std::string GrafanaDashboardGenerator::generateCachePanel() const {
    return createPanel("Cache Hit Rate",
                       "rate(llm_cache_hits_total[5m]) / (rate(llm_cache_hits_total[5m]) + rate(llm_cache_misses_total[5m]))",
                       "gauge", 0, 24, 12, 6);
}

std::string GrafanaDashboardGenerator::generateSchedulerPanel() const {
    return createPanel("Queue Length",
                       "llm_queue_length",
                       "graph", 0, 30, 12, 6);
}

std::string GrafanaDashboardGenerator::generateErrorPanel() const {
    return createPanel("Error Rate",
                       "rate(llm_errors_total[5m])",
                       "graph", 0, 36, 12, 6);
}

std::string GrafanaDashboardGenerator::generateUnifiedDashboard() const {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"dashboard\": {\n";
    oss << "    \"title\": \"" << config_.title << " — Unified Engine View\",\n";
    oss << "    \"tags\": [\"llm\", \"themisdb\", \"unified\"],\n";
    oss << "    \"timezone\": \"browser\",\n";
    oss << "    \"schemaVersion\": 16,\n";
    oss << "    \"version\": 1,\n";
    oss << "    \"refresh\": \"" << config_.refresh_interval_sec << "s\",\n";
    oss << "    \"panels\": [\n";

    int y = 0;

    // Row 1: Request rate per engine type (async vs enhanced)
    oss << createPanel("Requests/sec — AsyncInferenceEngine",
                       "rate(llm_engine_inference_requests_total{engine_type=\"async\"}[5m])",
                       "graph", 0, y, 12, 8);
    oss << ",\n";
    oss << createPanel("Requests/sec — InferenceEngineEnhanced",
                       "rate(llm_engine_inference_requests_total{engine_type=\"enhanced\"}[5m])",
                       "graph", 12, y, 12, 8);
    oss << ",\n";
    y += 8;

    // Row 2: Latency p95 per engine type
    oss << createPanel("Latency p95 (ms) — AsyncInferenceEngine",
                       "histogram_quantile(0.95, rate(llm_engine_inference_duration_ms_bucket{engine_type=\"async\"}[5m]))",
                       "graph", 0, y, 12, 8);
    oss << ",\n";
    oss << createPanel("Latency p95 (ms) — InferenceEngineEnhanced",
                       "histogram_quantile(0.95, rate(llm_engine_inference_duration_ms_bucket{engine_type=\"enhanced\"}[5m]))",
                       "graph", 12, y, 12, 8);
    oss << ",\n";
    y += 8;

    // Row 3: Tokens/sec per engine type
    oss << createPanel("Tokens/sec — AsyncInferenceEngine",
                       "rate(llm_engine_tokens_generated_total{engine_type=\"async\"}[5m])",
                       "graph", 0, y, 12, 8);
    oss << ",\n";
    oss << createPanel("Tokens/sec — InferenceEngineEnhanced",
                       "rate(llm_engine_tokens_generated_total{engine_type=\"enhanced\"}[5m])",
                       "graph", 12, y, 12, 8);
    oss << ",\n";
    y += 8;

    // Row 4: Queue depth per engine type + worker pool
    oss << createPanel("Queue Depth — AsyncInferenceEngine",
                       "llm_engine_queue_depth{engine_type=\"async\"}",
                       "graph", 0, y, 8, 6);
    oss << ",\n";
    oss << createPanel("Queue Depth — InferenceEngineEnhanced",
                       "llm_engine_queue_depth{engine_type=\"enhanced\"}",
                       "graph", 8, y, 8, 6);
    oss << ",\n";
    oss << createPanel("Shared Worker Pool Queue Depth",
                       "llm_worker_pool_queue_depth",
                       "graph", 16, y, 8, 6);
    oss << ",\n";
    y += 6;

    // Row 5: Error rates + cache hit rate (enhanced engine)
    oss << createPanel("Error Rate — AsyncInferenceEngine",
                       "rate(llm_engine_inference_failures_total{engine_type=\"async\"}[5m])",
                       "graph", 0, y, 8, 6);
    oss << ",\n";
    oss << createPanel("Error Rate — InferenceEngineEnhanced",
                       "rate(llm_engine_inference_failures_total{engine_type=\"enhanced\"}[5m])",
                       "graph", 8, y, 8, 6);
    oss << ",\n";
    oss << createPanel("Cache Hit Rate (Enhanced Engine)",
                       "rate(llm_cache_hits_total[5m]) / (rate(llm_cache_hits_total[5m]) + rate(llm_cache_misses_total[5m]))",
                       "gauge", 16, y, 8, 6);
    oss << "\n";

    oss << "    ]\n";
    oss << "  }\n";
    oss << "}\n";

    return oss.str();
}

bool GrafanaDashboardGenerator::saveDashboard(const std::string& filepath) const {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            spdlog::error("Failed to open file for writing: {}", filepath);
            return false;
        }
        
        file << generateDashboard();
        file.close();
        
        spdlog::info("Dashboard saved to: {}", filepath);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error saving dashboard: {}", e.what());
        return false;
    }
}


// MetricsServer::Impl — holds the httplib server and its listener thread.
// Defined here (not in the header) to keep <httplib.h> out of grafana_metrics.h.
#ifdef THEMIS_HAS_HTTPLIB
struct MetricsServer::Impl {
    httplib::Server svr;
    std::thread     thread;
};
#else
// Stub Impl when cpp-httplib is not available.
struct MetricsServer::Impl {};
#endif

// MetricsServer Implementation
MetricsServer::MetricsServer(const ServerConfig& config, PrometheusExporter* exporter)
    : config_(config), exporter_(exporter), running_(false),
      impl_(std::make_unique<Impl>()) {
    spdlog::debug("MetricsServer initialized");
}

MetricsServer::~MetricsServer() {
    stop();
}

#ifdef THEMIS_HAS_HTTPLIB
bool MetricsServer::start() {
    if (running_) {
        spdlog::warn("MetricsServer already running");
        return true;
    }

    // Register GET routes -----------------------------------------------
    // /metrics
    impl_->svr.Get(config_.metrics_path.c_str(),
        [this](const httplib::Request& /*req*/, httplib::Response& res) {
            res.set_content(exporter_->handleMetricsRequest(), "text/plain; version=0.0.4");
        });

    // /health
    impl_->svr.Get(config_.health_path.c_str(),
        [this](const httplib::Request& /*req*/, httplib::Response& res) {
            std::string body;
            handleRequest(config_.health_path, body);
            res.set_content(body, "application/json");
        });

    // /ready
    impl_->svr.Get(config_.ready_path.c_str(),
        [this](const httplib::Request& /*req*/, httplib::Response& res) {
            std::string body;
            handleRequest(config_.ready_path, body);
            const int status = (body.find("\"ready\"") != std::string::npos) ? 200 : 503;
            res.status = status;
            res.set_content(body, "application/json");
        });

    // /models
    impl_->svr.Get(config_.models_path.c_str(),
        [this](const httplib::Request& /*req*/, httplib::Response& res) {
            std::string body;
            handleRequest(config_.models_path, body);
            res.set_content(body, "application/json");
        });

    // /dashboard — serve the unified Grafana dashboard JSON
    impl_->svr.Get(config_.dashboard_path.c_str(),
        [this](const httplib::Request& /*req*/, httplib::Response& res) {
            std::string body;
            handleRequest(config_.dashboard_path, body);
            res.set_content(body, "application/json");
        });

    // Register POST routes -----------------------------------------------
    // POST /admin/models/reload
    impl_->svr.Post(config_.admin_reload_path.c_str(),
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string body;
            handlePost(config_.admin_reload_path, req.body, body);
            res.set_content(body, "application/json");
        });

    // POST /admin/prompt/simulate
    impl_->svr.Post(config_.admin_simulate_path.c_str(),
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string body;
            handlePost(config_.admin_simulate_path, req.body, body);
            res.set_content(body, "application/json");
        });

    // GET /admin/sessions — list active inference sessions
    impl_->svr.Get(config_.admin_sessions_path.c_str(),
        [this](const httplib::Request& /*req*/, httplib::Response& res) {
            std::string body;
            handleRequest(config_.admin_sessions_path, body);
            res.set_content(body, "application/json");
        });

    // DELETE /admin/sessions/:id — cancel/remove a specific session
    // httplib captures the :id segment in req.matches[1].
    std::string delete_sessions_pattern = config_.admin_sessions_path + "/(.+)";
    impl_->svr.Delete(delete_sessions_pattern.c_str(),
        [this](const httplib::Request& req, httplib::Response& res) {
            // matches[0] = full path, matches[1] = :id
            const std::string session_id =
                req.matches.size() > 1 ? std::string(req.matches[1]) : "";
            std::string body;
            handleDelete(config_.admin_sessions_path, session_id, body);
            res.set_content(body, "application/json");
        });

    // CORS: add Access-Control-Allow-Origin header to every response
    if (config_.enable_cors) {
        impl_->svr.set_post_routing_handler(
            [](const httplib::Request& /*req*/, httplib::Response& res) {
                res.set_header("Access-Control-Allow-Origin", "*");
            });
    }

    // Start listening on a background thread
    const std::string host = config_.host;
    const int         port = config_.port;

    impl_->thread = std::thread([this, host, port]() {
        spdlog::info("MetricsServer listening on {}:{}", host, port);
        if (!impl_->svr.listen(host.c_str(), port)) {
            spdlog::error("MetricsServer failed to listen on {}:{}", host, port);
        }
    });

    // Give the server a moment to bind before we report success.
    // httplib::Server::is_running() becomes true once listen() has bound.
    for (int i = 0; i < 50 && !impl_->svr.is_running(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!impl_->svr.is_running()) {
        spdlog::error("MetricsServer did not start within 500 ms on {}:{}", host, port);
        impl_->thread.detach();
        return false;
    }

    running_ = true;
    spdlog::info("MetricsServer started — metrics at {}", getMetricsURL());
    return true;
}

void MetricsServer::stop() {
    if (!running_) {
        return;
    }

    impl_->svr.stop();
    if (impl_->thread.joinable()) {
        if (!themis::utils::joinThreadWithin(impl_->thread)) {
            spdlog::warn("Metrics server thread did not join within timeout, continuing shutdown");
        }
    }

    running_ = false;
    spdlog::info("MetricsServer stopped");
}

#else  // !THEMIS_HAS_HTTPLIB

bool MetricsServer::start() {
    spdlog::warn("MetricsServer::start() called but cpp-httplib is not available; HTTP metrics endpoint disabled");
    return false;
}

void MetricsServer::stop() {
    // No-op: server was never started without httplib.
}

#endif  // THEMIS_HAS_HTTPLIB

bool MetricsServer::isRunning() const {
    return running_;
}

std::string MetricsServer::getMetricsURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.metrics_path;
}

std::string MetricsServer::getDashboardURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.dashboard_path;
}

std::string MetricsServer::getHealthURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.health_path;
}

std::string MetricsServer::getReadyURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.ready_path;
}

std::string MetricsServer::getModelsURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.models_path;
}

std::string MetricsServer::getAdminReloadURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.admin_reload_path;
}

std::string MetricsServer::getAdminSimulateURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.admin_simulate_path;
}

std::string MetricsServer::getAdminSessionsURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.admin_sessions_path;
}

void MetricsServer::handleRequest(const std::string& path, std::string& response) {
    if (path == config_.metrics_path) {
        response = exporter_->handleMetricsRequest();
    } else if (path == config_.dashboard_path) {
        // Unified dashboard: delegate to callback if set; otherwise generate
        // a default unified dashboard using GrafanaDashboardGenerator.
        if (dashboard_cb_) {
            response = dashboard_cb_();
        } else {
            GrafanaDashboardGenerator::DashboardConfig dcfg;
            dcfg.title = "ThemisDB LLM Monitoring";
            response = GrafanaDashboardGenerator(dcfg).generateUnifiedDashboard();
        }
    } else if (path == config_.health_path) {
        // Liveness: server is alive as long as the MetricsServer object exists.
        if (running_) {
            response = "{\"status\":\"ok\"}";
        } else {
            response = "{\"status\":\"stopped\"}";
        }
    } else if (path == config_.ready_path) {
        // Readiness: server is ready when running AND exporter is non-null.
        if (running_ && exporter_ != nullptr) {
            response = "{\"status\":\"ready\"}";
        } else {
            response = "{\"status\":\"not_ready\"}";
        }
    } else if (path == config_.models_path) {
        // Model list: delegate to the registered callback if present; otherwise
        // return an empty JSON array so callers get a valid (if empty) response.
        if (model_info_cb_) {
            response = model_info_cb_();
        } else {
            response = "[]";
        }
    } else if (path == config_.admin_sessions_path) {
        // Session list: delegate to the registered callback if present.
        if (session_list_cb_) {
            response = session_list_cb_();
        } else {
            response = "[]";
        }
    } else {
        response = "404 Not Found";
    }
}

void MetricsServer::handlePost(const std::string& path,
                               const std::string& body,
                               std::string& response) {
    // Default responses when no callback is registered — extracted to
    // constants so both branches stay consistent and are easy to update.
    static constexpr const char* k_reload_not_impl =
        R"({"status":"not_implemented","message":"No reload callback registered. Wire setReloadCallback() to LlamaWrapper::loadModel()."})";
    static constexpr const char* k_simulate_not_impl =
        R"({"status":"not_implemented","message":"No simulate callback registered. Wire setSimulateCallback() to PromptPolicy::apply() + tokenizer.estimateTokens()."})";

    if (path == config_.admin_reload_path) {
        // POST /admin/models/reload — trigger a hot-reload of the requested
        // model.  The caller provides the model ID (or file path) in the body.
        if (reload_cb_) {
            response = reload_cb_(body);
        } else {
            response = k_reload_not_impl;
        }
    } else if (path == config_.admin_simulate_path) {
        // POST /admin/prompt/simulate — dry-run policy check + tokenization.
        // Body: JSON {"prompt":"<text>","model_id":"<optional>"}
        if (simulate_cb_) {
            response = simulate_cb_(body);
        } else {
            response = k_simulate_not_impl;
        }
    } else {
        response = "404 Not Found";
    }
}

void MetricsServer::handleDelete(const std::string& path,
                                 const std::string& resource_id,
                                 std::string& response) {
    static constexpr const char* k_sessions_delete_not_impl =
        R"({"status":"not_implemented","message":"No session-delete callback registered. Wire setSessionDeleteCallback() to ContinuousBatchScheduler::cancelRequest()."})";

    if (path == config_.admin_sessions_path) {
        // DELETE /admin/sessions/{id} — cancel or remove the named session.
        if (resource_id.empty()) {
            response = R"({"status":"error","message":"session_id is required"})";
            return;
        }
        if (session_delete_cb_) {
            response = session_delete_cb_(resource_id);
        } else {
            response = k_sessions_delete_not_impl;
        }
    } else {
        response = "404 Not Found";
    }
}

} // namespace monitoring
} // namespace llm
} // namespace themis

