#include "llm/grafana_metrics.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>

namespace themis {
namespace llm {
namespace monitoring {

// PrometheusExporter Implementation
PrometheusExporter::PrometheusExporter() {
    spdlog::info("Prometheus Exporter initialized for Grafana integration");
}

PrometheusExporter::~PrometheusExporter() {
    spdlog::info("Prometheus Exporter shutdown");
}

void PrometheusExporter::registerMetric(const MetricDefinition& def) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    registered_metrics_[def.name] = def;
    spdlog::debug("Registered metric: {} (type: {})", def.name, static_cast<int>(def.type));
}

void PrometheusExporter::incrementCounter(
    const std::string& name,
    const std::unordered_map<std::string, std::string>& labels,
    double value
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    
    if (metrics_.find(key) == metrics_.end()) {
        metrics_[key] = MetricValue{MetricType::COUNTER, 0.0, {}, std::chrono::system_clock::now()};
    }
    
    metrics_[key].value += value;
    metrics_[key].last_updated = std::chrono::system_clock::now();
}

void PrometheusExporter::setGauge(
    const std::string& name,
    double value,
    const std::unordered_map<std::string, std::string>& labels
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    
    metrics_[key] = MetricValue{MetricType::GAUGE, value, {}, std::chrono::system_clock::now()};
}

void PrometheusExporter::incrementGauge(
    const std::string& name,
    double delta,
    const std::unordered_map<std::string, std::string>& labels
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    
    if (metrics_.find(key) == metrics_.end()) {
        metrics_[key] = MetricValue{MetricType::GAUGE, 0.0, {}, std::chrono::system_clock::now()};
    }
    
    metrics_[key].value += delta;
    metrics_[key].last_updated = std::chrono::system_clock::now();
}

void PrometheusExporter::observeHistogram(
    const std::string& name,
    double value,
    const std::unordered_map<std::string, std::string>& labels
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeMetricKey(name, labels);
    
    if (metrics_.find(key) == metrics_.end()) {
        metrics_[key] = MetricValue{MetricType::HISTOGRAM, 0.0, {}, std::chrono::system_clock::now()};
    }
    
    metrics_[key].histogram_buckets.push_back(value);
    metrics_[key].value += value;  // Sum
    metrics_[key].last_updated = std::chrono::system_clock::now();
}

std::string PrometheusExporter::exportMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    
    // Group metrics by base name
    std::unordered_map<std::string, std::vector<std::pair<std::string, MetricValue>>> grouped;
    
    for (const auto& [key, value] : metrics_) {
        // Extract base metric name (before labels)
        size_t brace_pos = key.find('{');
        std::string base_name = (brace_pos != std::string::npos) 
            ? key.substr(0, brace_pos) 
            : key;
        
        grouped[base_name].push_back({key, value});
    }
    
    // Export each metric group
    for (const auto& [base_name, values] : grouped) {
        auto it = registered_metrics_.find(base_name);
        if (it != registered_metrics_.end()) {
            ss << "# HELP " << base_name << " " << it->second.help << "\n";
            ss << "# TYPE " << base_name << " ";
            
            switch (it->second.type) {
                case MetricType::COUNTER: ss << "counter"; break;
                case MetricType::GAUGE: ss << "gauge"; break;
                case MetricType::HISTOGRAM: ss << "histogram"; break;
                case MetricType::SUMMARY: ss << "summary"; break;
            }
            ss << "\n";
        }
        
        for (const auto& [key, value] : values) {
            ss << serializeMetric(key, value);
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

std::string PrometheusExporter::handleMetricsRequest() const {
    return exportMetrics();
}

void PrometheusExporter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
    spdlog::info("All metrics reset");
}

std::string PrometheusExporter::serializeMetric(
    const std::string& name,
    const MetricValue& value
) const {
    std::stringstream ss;
    
    if (value.type == MetricType::HISTOGRAM) {
        // For histograms, export bucket counts
        std::vector<double> buckets = {10, 25, 50, 100, 250, 500, 1000, 2500, 5000};
        
        for (double bucket : buckets) {
            size_t count = std::count_if(
                value.histogram_buckets.begin(),
                value.histogram_buckets.end(),
                [bucket](double v) { return v <= bucket; }
            );
            
            ss << name << "_bucket{le=\"" << bucket << "\"} " << count << "\n";
        }
        
        ss << name << "_bucket{le=\"+Inf\"} " << value.histogram_buckets.size() << "\n";
        ss << name << "_sum " << value.value << "\n";
        ss << name << "_count " << value.histogram_buckets.size() << "\n";
    } else {
        ss << name << " " << std::fixed << std::setprecision(2) << value.value << "\n";
    }
    
    return ss.str();
}

std::string PrometheusExporter::makeMetricKey(
    const std::string& name,
    const std::unordered_map<std::string, std::string>& labels
) const {
    if (labels.empty()) {
        return name;
    }
    
    std::stringstream ss;
    ss << name << "{";
    
    bool first = true;
    for (const auto& [key, val] : labels) {
        if (!first) ss << ",";
        ss << key << "=\"" << val << "\"";
        first = false;
    }
    
    ss << "}";
    return ss.str();
}

// LLMMetricsCollector Implementation
LLMMetricsCollector::LLMMetricsCollector(PrometheusExporter* exporter)
    : exporter_(exporter) {
    initializeMetrics();
    spdlog::info("LLM Metrics Collector initialized");
}

void LLMMetricsCollector::initializeMetrics() {
    // Inference metrics
    exporter_->registerMetric({
        "llm_inference_requests_total",
        "Total number of inference requests",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_inference_duration_ms",
        "Inference duration in milliseconds",
        PrometheusExporter::MetricType::HISTOGRAM,
        {"model_id"}
    });
    
    exporter_->registerMetric({
        "llm_inference_failures_total",
        "Total number of failed inference requests",
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
    
    // Throughput metrics
    exporter_->registerMetric({
        "llm_tokens_generated_total",
        "Total tokens generated",
        PrometheusExporter::MetricType::COUNTER,
        {"model_id"}
    });
    
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
        "GPU memory used in MB",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_gpu_memory_total_mb",
        "Total GPU memory in MB",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_gpu_utilization_pct",
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
        "Number of models currently loaded",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_model_memory_mb",
        "Memory used by model in MB",
        PrometheusExporter::MetricType::GAUGE,
        {"model_id"}
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
    
    // Scheduler metrics
    exporter_->registerMetric({
        "llm_scheduler_queue_length",
        "Number of requests in scheduler queue",
        PrometheusExporter::MetricType::GAUGE,
        {}
    });
    
    exporter_->registerMetric({
        "llm_scheduler_preemptions_total",
        "Total number of request preemptions",
        PrometheusExporter::MetricType::COUNTER,
        {}
    });
    
    // Error metrics
    exporter_->registerMetric({
        "llm_errors_total",
        "Total errors by type",
        PrometheusExporter::MetricType::COUNTER,
        {"error_type", "component"}
    });
}

void LLMMetricsCollector::recordInferenceRequest(const std::string& model_id) {
    exporter_->incrementCounter("llm_inference_requests_total", {{"model_id", model_id}});
}

void LLMMetricsCollector::recordInferenceSuccess(const std::string& model_id, double duration_ms) {
    exporter_->observeHistogram("llm_inference_duration_ms", duration_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordInferenceFailure(const std::string& model_id, const std::string& error) {
    exporter_->incrementCounter("llm_inference_failures_total", 
                               {{"model_id", model_id}, {"error", error}});
}

void LLMMetricsCollector::recordFirstTokenLatency(const std::string& model_id, double latency_ms) {
    exporter_->observeHistogram("llm_first_token_latency_ms", latency_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordPerTokenLatency(const std::string& model_id, double latency_ms) {
    exporter_->observeHistogram("llm_per_token_latency_ms", latency_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordEndToEndLatency(const std::string& model_id, double latency_ms) {
    // Can reuse inference_duration or create separate metric
    exporter_->observeHistogram("llm_inference_duration_ms", latency_ms, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordTokensGenerated(const std::string& model_id, size_t count) {
    exporter_->incrementCounter("llm_tokens_generated_total", {{"model_id", model_id}}, count);
}

void LLMMetricsCollector::recordBatchSize(size_t batch_size) {
    exporter_->setGauge("llm_batch_size", batch_size);
}

void LLMMetricsCollector::recordConcurrentRequests(size_t count) {
    exporter_->setGauge("llm_concurrent_requests", count);
}

void LLMMetricsCollector::recordGPUMemoryUsage(size_t vram_mb, size_t total_vram_mb) {
    exporter_->setGauge("llm_gpu_memory_used_mb", vram_mb);
    exporter_->setGauge("llm_gpu_memory_total_mb", total_vram_mb);
}

void LLMMetricsCollector::recordGPUUtilization(double utilization_pct) {
    exporter_->setGauge("llm_gpu_utilization_pct", utilization_pct);
}

void LLMMetricsCollector::recordGPUTemperature(double temp_celsius) {
    exporter_->setGauge("llm_gpu_temperature_celsius", temp_celsius);
}

void LLMMetricsCollector::recordModelLoaded(const std::string& model_id, size_t vram_mb) {
    exporter_->incrementGauge("llm_models_loaded", 1.0);
    exporter_->setGauge("llm_model_memory_mb", vram_mb, {{"model_id", model_id}});
}

void LLMMetricsCollector::recordModelUnloaded(const std::string& model_id) {
    exporter_->incrementGauge("llm_models_loaded", -1.0);
    exporter_->setGauge("llm_model_memory_mb", 0, {{"model_id", model_id}});
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
    exporter_->setGauge("llm_cache_size_mb", size_mb, {{"cache_type", cache_type}});
}

void LLMMetricsCollector::recordQueueLength(size_t length) {
    exporter_->setGauge("llm_scheduler_queue_length", length);
}

void LLMMetricsCollector::recordPreemptions(size_t count) {
    exporter_->incrementCounter("llm_scheduler_preemptions_total", {}, count);
}

void LLMMetricsCollector::recordSchedulingLatency(double latency_ms) {
    exporter_->observeHistogram("llm_scheduling_latency_ms", latency_ms);
}

void LLMMetricsCollector::recordQuantizationFormat(const std::string& model_id, const std::string& format) {
    exporter_->setGauge("llm_quantization_format", 1.0, 
                       {{"model_id", model_id}, {"format", format}});
}

void LLMMetricsCollector::recordDequantizationLatency(double latency_ms) {
    exporter_->observeHistogram("llm_dequantization_latency_ms", latency_ms);
}

void LLMMetricsCollector::recordError(const std::string& error_type, const std::string& component) {
    exporter_->incrementCounter("llm_errors_total", 
                               {{"error_type", error_type}, {"component", component}});
}

// GrafanaDashboardGenerator Implementation
GrafanaDashboardGenerator::GrafanaDashboardGenerator(const DashboardConfig& config)
    : config_(config) {
    spdlog::info("Grafana Dashboard Generator initialized");
}

std::string GrafanaDashboardGenerator::generateDashboard() const {
    std::stringstream ss;
    
    ss << "{\n";
    ss << "  \"title\": \"" << config_.title << "\",\n";
    ss << "  \"tags\": [\"llm\", \"themisdb\", \"llama.cpp\"],\n";
    ss << "  \"timezone\": \"browser\",\n";
    ss << "  \"refresh\": \"" << config_.refresh_interval_sec << "s\",\n";
    ss << "  \"panels\": [\n";
    
    // Add panels
    ss << "    " << generateInferencePanel() << ",\n";
    ss << "    " << generateLatencyPanel() << ",\n";
    ss << "    " << generateThroughputPanel() << ",\n";
    ss << "    " << generateGPUPanel() << ",\n";
    ss << "    " << generateCachePanel() << ",\n";
    ss << "    " << generateSchedulerPanel() << ",\n";
    ss << "    " << generateErrorPanel() << "\n";
    
    ss << "  ]\n";
    ss << "}\n";
    
    return ss.str();
}

std::string GrafanaDashboardGenerator::generateInferencePanel() const {
    return R"({
      "title": "Inference Requests",
      "type": "graph",
      "targets": [{
        "expr": "rate(llm_inference_requests_total[5m])",
        "legendFormat": "{{model_id}}"
      }],
      "gridPos": {"x": 0, "y": 0, "w": 12, "h": 8}
    })";
}

std::string GrafanaDashboardGenerator::generateLatencyPanel() const {
    return R"({
      "title": "Latency Distribution",
      "type": "graph",
      "targets": [
        {
          "expr": "histogram_quantile(0.50, llm_first_token_latency_ms)",
          "legendFormat": "p50 First Token"
        },
        {
          "expr": "histogram_quantile(0.95, llm_first_token_latency_ms)",
          "legendFormat": "p95 First Token"
        },
        {
          "expr": "histogram_quantile(0.99, llm_first_token_latency_ms)",
          "legendFormat": "p99 First Token"
        }
      ],
      "gridPos": {"x": 12, "y": 0, "w": 12, "h": 8}
    })";
}

std::string GrafanaDashboardGenerator::generateThroughputPanel() const {
    return R"({
      "title": "Throughput (tokens/sec)",
      "type": "graph",
      "targets": [{
        "expr": "rate(llm_tokens_generated_total[1m])",
        "legendFormat": "{{model_id}}"
      }],
      "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8}
    })";
}

std::string GrafanaDashboardGenerator::generateGPUPanel() const {
    return R"({
      "title": "GPU Metrics",
      "type": "graph",
      "targets": [
        {
          "expr": "llm_gpu_memory_used_mb / llm_gpu_memory_total_mb * 100",
          "legendFormat": "Memory Usage %"
        },
        {
          "expr": "llm_gpu_utilization_pct",
          "legendFormat": "GPU Utilization %"
        },
        {
          "expr": "llm_gpu_temperature_celsius",
          "legendFormat": "Temperature °C"
        }
      ],
      "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8}
    })";
}

std::string GrafanaDashboardGenerator::generateCachePanel() const {
    return R"({
      "title": "Cache Hit Rate",
      "type": "graph",
      "targets": [{
        "expr": "rate(llm_cache_hits_total[5m]) / (rate(llm_cache_hits_total[5m]) + rate(llm_cache_misses_total[5m])) * 100",
        "legendFormat": "{{cache_type}} hit rate %"
      }],
      "gridPos": {"x": 0, "y": 16, "w": 12, "h": 8}
    })";
}

std::string GrafanaDashboardGenerator::generateSchedulerPanel() const {
    return R"({
      "title": "Scheduler Queue",
      "type": "graph",
      "targets": [
        {
          "expr": "llm_scheduler_queue_length",
          "legendFormat": "Queue Length"
        },
        {
          "expr": "llm_concurrent_requests",
          "legendFormat": "Concurrent Requests"
        }
      ],
      "gridPos": {"x": 12, "y": 16, "w": 12, "h": 8}
    })";
}

std::string GrafanaDashboardGenerator::generateErrorPanel() const {
    return R"({
      "title": "Errors",
      "type": "graph",
      "targets": [{
        "expr": "rate(llm_errors_total[5m])",
        "legendFormat": "{{error_type}} in {{component}}"
      }],
      "gridPos": {"x": 0, "y": 24, "w": 24, "h": 8}
    })";
}

bool GrafanaDashboardGenerator::saveDashboard(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open file for dashboard: {}", filepath);
        return false;
    }
    
    file << generateDashboard();
    file.close();
    
    spdlog::info("Grafana dashboard saved to: {}", filepath);
    return true;
}

// MetricsServer Implementation
MetricsServer::MetricsServer(const ServerConfig& config, PrometheusExporter* exporter)
    : config_(config), exporter_(exporter) {
    spdlog::info("Metrics Server initialized on {}:{}", config_.host, config_.port);
}

MetricsServer::~MetricsServer() {
    stop();
}

bool MetricsServer::start() {
    if (running_) {
        return false;
    }
    
    running_ = true;
    
    // TODO: Start actual HTTP server (using beast/asio or similar)
    // For now, placeholder
    
    spdlog::info("Metrics Server started: {}", getMetricsURL());
    return true;
}

void MetricsServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    spdlog::info("Metrics Server stopped");
}

bool MetricsServer::isRunning() const {
    return running_;
}

std::string MetricsServer::getMetricsURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.metrics_path;
}

std::string MetricsServer::getDashboardURL() const {
    return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.dashboard_path;
}

void MetricsServer::handleRequest(const std::string& path, std::string& response) {
    if (path == config_.metrics_path) {
        response = exporter_->handleMetricsRequest();
    } else if (path == config_.dashboard_path) {
        GrafanaDashboardGenerator gen({});
        response = gen.generateDashboard();
    } else {
        response = "404 Not Found";
    }
}

} // namespace monitoring
} // namespace llm
} // namespace themis
