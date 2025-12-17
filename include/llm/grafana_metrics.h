#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <memory>

namespace themis {
namespace llm {
namespace monitoring {

/**
 * @brief Prometheus Metrics Exporter for Grafana Integration
 * 
 * Exports LLM/llama.cpp metrics in Prometheus format for Grafana visualization.
 * Provides comprehensive observability for inference pipeline, GPU usage,
 * and model performance.
 */
class PrometheusExporter {
public:
    enum class MetricType {
        COUNTER,      // Monotonically increasing (e.g., total requests)
        GAUGE,        // Can go up/down (e.g., memory usage)
        HISTOGRAM,    // Distribution of values (e.g., latency)
        SUMMARY       // Similar to histogram with quantiles
    };
    
    struct MetricDefinition {
        std::string name;
        std::string help;
        MetricType type;
        std::vector<std::string> label_names;
    };
    
    PrometheusExporter();
    ~PrometheusExporter();
    
    // Metric registration
    void registerMetric(const MetricDefinition& def);
    
    // Counter operations (always increase)
    void incrementCounter(const std::string& name, 
                         const std::unordered_map<std::string, std::string>& labels = {},
                         double value = 1.0);
    
    // Gauge operations (current value)
    void setGauge(const std::string& name,
                  double value,
                  const std::unordered_map<std::string, std::string>& labels = {});
    
    void incrementGauge(const std::string& name,
                       double delta,
                       const std::unordered_map<std::string, std::string>& labels = {});
    
    // Histogram operations (for latency distributions)
    void observeHistogram(const std::string& name,
                         double value,
                         const std::unordered_map<std::string, std::string>& labels = {});
    
    // Export metrics in Prometheus format
    std::string exportMetrics() const;
    
    // HTTP endpoint handler (for Prometheus scraping)
    std::string handleMetricsRequest() const;
    
    // Reset all metrics
    void reset();
    
private:
    struct MetricValue {
        MetricType type;
        double value;
        std::vector<double> histogram_buckets;  // For histograms
        std::chrono::system_clock::time_point last_updated;
    };
    
    std::unordered_map<std::string, MetricDefinition> registered_metrics_;
    std::unordered_map<std::string, MetricValue> metrics_;
    mutable std::mutex mutex_;
    
    std::string serializeMetric(const std::string& name, const MetricValue& value) const;
    std::string makeMetricKey(const std::string& name,
                             const std::unordered_map<std::string, std::string>& labels) const;
};

/**
 * @brief LLM Metrics Collector
 * 
 * Collects comprehensive metrics from LLM inference pipeline
 * for Grafana dashboards.
 */
class LLMMetricsCollector {
public:
    explicit LLMMetricsCollector(PrometheusExporter* exporter);
    
    // Inference metrics
    void recordInferenceRequest(const std::string& model_id);
    void recordInferenceSuccess(const std::string& model_id, double duration_ms);
    void recordInferenceFailure(const std::string& model_id, const std::string& error);
    
    // Latency metrics
    void recordFirstTokenLatency(const std::string& model_id, double latency_ms);
    void recordPerTokenLatency(const std::string& model_id, double latency_ms);
    void recordEndToEndLatency(const std::string& model_id, double latency_ms);
    
    // Throughput metrics
    void recordTokensGenerated(const std::string& model_id, size_t count);
    void recordBatchSize(size_t batch_size);
    void recordConcurrentRequests(size_t count);
    
    // GPU metrics
    void recordGPUMemoryUsage(size_t vram_mb, size_t total_vram_mb);
    void recordGPUUtilization(double utilization_pct);
    void recordGPUTemperature(double temp_celsius);
    
    // Model metrics
    void recordModelLoaded(const std::string& model_id, size_t vram_mb);
    void recordModelUnloaded(const std::string& model_id);
    void recordModelSwitchLatency(double latency_ms);
    
    // Cache metrics
    void recordCacheHit(const std::string& cache_type);
    void recordCacheMiss(const std::string& cache_type);
    void recordCacheSize(const std::string& cache_type, size_t size_mb);
    
    // Scheduler metrics
    void recordQueueLength(size_t length);
    void recordPreemptions(size_t count);
    void recordSchedulingLatency(double latency_ms);
    
    // Quantization metrics
    void recordQuantizationFormat(const std::string& model_id, const std::string& format);
    void recordDequantizationLatency(double latency_ms);
    
    // Error metrics
    void recordError(const std::string& error_type, const std::string& component);
    
private:
    PrometheusExporter* exporter_;
    
    void initializeMetrics();
};

/**
 * @brief Grafana Dashboard Generator
 * 
 * Generates Grafana dashboard JSON configurations for LLM monitoring.
 */
class GrafanaDashboardGenerator {
public:
    struct DashboardConfig {
        std::string title = "ThemisDB LLM Monitoring";
        std::string datasource = "Prometheus";
        int refresh_interval_sec = 5;
        bool enable_alerts = true;
    };
    
    explicit GrafanaDashboardGenerator(const DashboardConfig& config);
    
    // Generate complete dashboard JSON
    std::string generateDashboard() const;
    
    // Generate individual panels
    std::string generateInferencePanel() const;
    std::string generateLatencyPanel() const;
    std::string generateThroughputPanel() const;
    std::string generateGPUPanel() const;
    std::string generateCachePanel() const;
    std::string generateSchedulerPanel() const;
    std::string generateErrorPanel() const;
    
    // Save dashboard to file
    bool saveDashboard(const std::string& filepath) const;
    
private:
    DashboardConfig config_;
    
    std::string createPanel(const std::string& title,
                           const std::string& query,
                           const std::string& type,
                           int grid_pos_x, int grid_pos_y,
                           int grid_width, int grid_height) const;
};

/**
 * @brief Real-time Metrics Server
 * 
 * HTTP server for serving Prometheus metrics and Grafana dashboards.
 */
class MetricsServer {
public:
    struct ServerConfig {
        std::string host = "0.0.0.0";
        int port = 9090;
        bool enable_cors = true;
        std::string metrics_path = "/metrics";
        std::string dashboard_path = "/dashboard";
    };
    
    explicit MetricsServer(const ServerConfig& config,
                          PrometheusExporter* exporter);
    ~MetricsServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool isRunning() const;
    
    // Get server URL
    std::string getMetricsURL() const;
    std::string getDashboardURL() const;
    
private:
    ServerConfig config_;
    PrometheusExporter* exporter_;
    bool running_ = false;
    
    // HTTP request handling
    void handleRequest(const std::string& path, std::string& response);
};

} // namespace monitoring
} // namespace llm
} // namespace themis
