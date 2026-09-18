/**
 * @file grafana_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <memory>
#include <functional>
#include <thread>

namespace themis {
namespace llm {
namespace monitoring {

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
    /**
     * @brief Register Metric.
     * @param[in] def Input parameter.
     */
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
    
    /**
     * @brief Export metrics in Prometheus format
     * @return Return value.
     */
    std::string exportMetrics() const;
    
    /**
     * @brief HTTP endpoint handler (for Prometheus scraping)
     * @return Return value.
     */
    std::string handleMetricsRequest() const;
    
    // Reset all metrics
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
private:
    struct MetricValue {
        MetricType type;
        double value = 0.0;
        std::vector<double> histogram_buckets;  // For histograms
        std::chrono::system_clock::time_point last_updated;
    };
    
    std::unordered_map<std::string, MetricDefinition> registered_metrics_;
    std::unordered_map<std::string, MetricValue> metrics_;
    mutable std::mutex mutex_;
    
    /**
     * @brief Serialize Metric.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::string serializeMetric(const std::string& name, const MetricValue& value) const;
    std::string makeMetricKey(const std::string& name,
                             const std::unordered_map<std::string, std::string>& labels) const;
};

class LLMMetricsCollector {
public:
    struct Config {
        double lock_contention_threshold_ms = 100.0;
    };

    /**
     * @brief LLMMetrics Collector.
     * @param[in,out] exporter Input/output parameter.
     * @return Return value.
     */
    explicit LLMMetricsCollector(PrometheusExporter* exporter);
    LLMMetricsCollector(PrometheusExporter* exporter, const Config& config);
    
    // Inference metrics
    /**
     * @brief Record Inference Request.
     * @param[in] model_id Identifier of the model.
     */
    void recordInferenceRequest(const std::string& model_id);
    /**
     * @brief Record Inference Success.
     * @param[in] model_id Identifier of the model.
     * @param[in] duration_ms Input parameter.
     */
    void recordInferenceSuccess(const std::string& model_id, double duration_ms);
    /**
     * @brief Record Inference Failure.
     * @param[in] model_id Identifier of the model.
     * @param[in] error Input parameter.
     */
    void recordInferenceFailure(const std::string& model_id, const std::string& error);
    
    // Latency metrics
    /**
     * @brief Record First Token Latency.
     * @param[in] model_id Identifier of the model.
     * @param[in] latency_ms Input parameter.
     */
    void recordFirstTokenLatency(const std::string& model_id, double latency_ms);
    /**
     * @brief Record Per Token Latency.
     * @param[in] model_id Identifier of the model.
     * @param[in] latency_ms Input parameter.
     */
    void recordPerTokenLatency(const std::string& model_id, double latency_ms);
    /**
     * @brief Record End To End Latency.
     * @param[in] model_id Identifier of the model.
     * @param[in] latency_ms Input parameter.
     */
    void recordEndToEndLatency(const std::string& model_id, double latency_ms);
    
    // Throughput metrics
    /**
     * @brief Record Tokens Generated.
     * @param[in] model_id Identifier of the model.
     * @param[in] count Input parameter.
     */
    void recordTokensGenerated(const std::string& model_id, size_t count);
    /**
     * @brief Record Batch Size.
     * @param[in] batch_size Input parameter.
     */
    void recordBatchSize(size_t batch_size);
    /**
     * @brief Record Concurrent Requests.
     * @param[in] count Input parameter.
     */
    void recordConcurrentRequests(size_t count);
    
    // GPU metrics
    /**
     * @brief Record GPUMemory Usage.
     * @param[in] vram_mb Input parameter.
     * @param[in] total_vram_mb Input parameter.
     */
    void recordGPUMemoryUsage(size_t vram_mb, size_t total_vram_mb);
    /**
     * @brief Record GPUUtilization.
     * @param[in] utilization_pct Input parameter.
     */
    void recordGPUUtilization(double utilization_pct);
    /**
     * @brief Record GPUTemperature.
     * @param[in] temp_celsius Input parameter.
     */
    void recordGPUTemperature(double temp_celsius);
    
    // Model metrics
    /**
     * @brief Record Model Loaded.
     * @param[in] model_id Identifier of the model.
     * @param[in] vram_mb Input parameter.
     */
    void recordModelLoaded(const std::string& model_id, size_t vram_mb);
    /**
     * @brief Record Model Unloaded.
     * @param[in] model_id Identifier of the model.
     */
    void recordModelUnloaded(const std::string& model_id);
    /**
     * @brief Record Model Switch Latency.
     * @param[in] latency_ms Input parameter.
     */
    void recordModelSwitchLatency(double latency_ms);
    
    // Cache metrics
    /**
     * @brief Record Cache Hit.
     * @param[in] cache_type Input parameter.
     */
    void recordCacheHit(const std::string& cache_type);
    /**
     * @brief Record Cache Miss.
     * @param[in] cache_type Input parameter.
     */
    void recordCacheMiss(const std::string& cache_type);
    /**
     * @brief Record Cache Size.
     * @param[in] cache_type Input parameter.
     * @param[in] size_mb Input parameter.
     */
    void recordCacheSize(const std::string& cache_type, size_t size_mb);
    
    // Scheduler metrics
    /**
     * @brief Record Queue Length.
     * @param[in] length Input parameter.
     */
    void recordQueueLength(size_t length);
    /**
     * @brief Record Preemptions.
     * @param[in] count Input parameter.
     */
    void recordPreemptions(size_t count);
    /**
     * @brief Record Scheduling Latency.
     * @param[in] latency_ms Input parameter.
     */
    void recordSchedulingLatency(double latency_ms);
    /**
     * @brief Increments llm_backpressure_drops_total when the scheduler rejects a request because max_queue_depth has been reached.
     */
    void recordBackpressureDrop();
    
    // Quantization metrics
    /**
     * @brief Record Quantization Format.
     * @param[in] model_id Identifier of the model.
     * @param[in] format Input parameter.
     */
    void recordQuantizationFormat(const std::string& model_id, const std::string& format);
    /**
     * @brief Record Dequantization Latency.
     * @param[in] latency_ms Input parameter.
     */
    void recordDequantizationLatency(double latency_ms);
    
    // Error metrics
    /**
     * @brief Record Error.
     * @param[in] error_type Input parameter.
     * @param[in] component Input parameter.
     */
    void recordError(const std::string& error_type, const std::string& component);
    
    /**
     * @brief Extended Context Window metrics (v1.
     * @param[in] model_id Identifier of the model.
     * @param[in] context_length Input parameter.
     * @details 4.0+)
     */
    void recordContextLength(const std::string& model_id, size_t context_length);
    /**
     * @brief Record Context Cache Size.
     * @param[in] model_id Identifier of the model.
     * @param[in] cache_size_mb Input parameter.
     */
    void recordContextCacheSize(const std::string& model_id, size_t cache_size_mb);
    /**
     * @brief Record Extended Context Enabled.
     * @param[in] model_id Identifier of the model.
     * @param[in] enabled Input parameter.
     */
    void recordExtendedContextEnabled(const std::string& model_id, bool enabled);
    /**
     * @brief Record Context Scaling Factor.
     * @param[in] model_id Identifier of the model.
     * @param[in] scaling_factor Input parameter.
     */
    void recordContextScalingFactor(const std::string& model_id, double scaling_factor);
    
    /**
     * @brief RoPE/YARN Scaling metrics (v1.
     * @param[in] model_id Identifier of the model.
     * @param[in] method Input parameter.
     * @details 4.0+)
     */
    void recordRoPEScalingMethod(const std::string& model_id, const std::string& method);
    /**
     * @brief Record Ro PEScaling Error.
     * @param[in] model_id Identifier of the model.
     * @param[in] error Input parameter.
     */
    void recordRoPEScalingError(const std::string& model_id, const std::string& error);
    /**
     * @brief Record YARNParameters.
     * @param[in] model_id Identifier of the model.
     * @param[in] ext_factor Input parameter.
     * @param[in] attn_factor Input parameter.
     * @param[in] beta_fast Input parameter.
     * @param[in] beta_slow Input parameter.
     */
    void recordYARNParameters(const std::string& model_id, 
                              double ext_factor, double attn_factor,
                              double beta_fast, double beta_slow);
    
    /**
     * @brief Memory Profiling metrics (v1.
     * @param[in] model_id Identifier of the model.
     * @param[in] ram_mb Input parameter.
     * @param[in] total_ram_mb Input parameter.
     * @details 4.0+)
     */
    void recordRAMUsage(const std::string& model_id, size_t ram_mb, size_t total_ram_mb);
    /**
     * @brief Record VRAMUsage.
     * @param[in] model_id Identifier of the model.
     * @param[in] vram_mb Input parameter.
     * @param[in] total_vram_mb Input parameter.
     */
    void recordVRAMUsage(const std::string& model_id, size_t vram_mb, size_t total_vram_mb);
    /**
     * @brief Record Memory Pressure.
     * @param[in] model_id Identifier of the model.
     * @param[in] pressure_pct Input parameter.
     */
    void recordMemoryPressure(const std::string& model_id, double pressure_pct);
    /**
     * @brief Record OOMEvent.
     * @param[in] model_id Identifier of the model.
     * @param[in] reason Input parameter.
     */
    void recordOOMEvent(const std::string& model_id, const std::string& reason);
    /**
     * @brief Record Memory Estimate.
     * @param[in] model_id Identifier of the model.
     * @param[in] estimated_mb Input parameter.
     * @param[in] actual_mb Input parameter.
     */
    void recordMemoryEstimate(const std::string& model_id, 
                             size_t estimated_mb, size_t actual_mb);
    
    /**
     * @brief Thread Safety metrics (v1.
     * @param[in] model_id Identifier of the model.
     * @param[in] from_adapter Input parameter.
     * @param[in] to_adapter Input parameter.
     * @param[in] duration_ms Input parameter.
     * @details 4.0+)
     */
    void recordLoRAAdapterSwitch(const std::string& model_id, 
                                 const std::string& from_adapter,
                                 const std::string& to_adapter,
                                 double duration_ms);
    /**
     * @brief Record Context Lock Wait.
     * @param[in] model_id Identifier of the model.
     * @param[in] wait_time_ms Input parameter.
     */
    void recordContextLockWait(const std::string& model_id, double wait_time_ms);
    /**
     * @brief Record Concurrent Lo RAOperation.
     * @param[in] model_id Identifier of the model.
     * @param[in] sequential_mode Input parameter.
     */
    void recordConcurrentLoRAOperation(const std::string& model_id, bool sequential_mode);

    /**
     * @brief Shared Worker Pool metrics (Phase 2 — Q2 2026) llm_worker_pool_queue_depth : gauge — current pending-task depth llm_worker_pool_tasks_completed_total : counter — tasks finished since start
     * @param[in] depth Input parameter.
     */
    void recordWorkerPoolQueueDepth(size_t depth);
    /**
     * @brief Record Worker Pool Tasks Completed.
     * @param[in] total_completed Input parameter.
     */
    void recordWorkerPoolTasksCompleted(uint64_t total_completed);

    /**
     * @brief ── Unified dashboard metrics (Phase 2 — Q3 2026) ──────────────────────── Engine-typed variants for the unified metrics dashboard.
     * @param[in] model_id Identifier of the model.
     * @param[in] engine_type Input parameter.
     * @details engine_type: "async" → AsyncInferenceEngine "enhanced" → InferenceEngineEnhanced Prometheus metric names used: llm_engine_inference_requests_total{model_id, engine_type} llm_engine_inference_success_total{model_id, engine_type} llm_engine_inference_failures_total{model_id, engine_type, error} llm_engine_inference_duration_ms{model_id, engine_type} llm_engine_tokens_generated_total{model_id, engine_type} llm_engine_queue_depth{engine_type}
     */
    void recordEngineInferenceRequest(const std::string& model_id,
                                      const std::string& engine_type);
    /**
     * @brief Record Engine Inference Success.
     * @param[in] model_id Identifier of the model.
     * @param[in] engine_type Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordEngineInferenceSuccess(const std::string& model_id,
                                      const std::string& engine_type,
                                      double duration_ms);
    /**
     * @brief Record Engine Inference Failure.
     * @param[in] model_id Identifier of the model.
     * @param[in] engine_type Input parameter.
     * @param[in] error Input parameter.
     */
    void recordEngineInferenceFailure(const std::string& model_id,
                                      const std::string& engine_type,
                                      const std::string& error);
    /**
     * @brief Record Engine Tokens Generated.
     * @param[in] model_id Identifier of the model.
     * @param[in] engine_type Input parameter.
     * @param[in] count Input parameter.
     */
    void recordEngineTokensGenerated(const std::string& model_id,
                                     const std::string& engine_type,
                                     size_t count);
    /**
     * @brief Record Engine Queue Depth.
     * @param[in] engine_type Input parameter.
     * @param[in] depth Input parameter.
     */
    void recordEngineQueueDepth(const std::string& engine_type, size_t depth);

private:
    PrometheusExporter* exporter_;
    Config config_;

    // Last absolute value reported by recordWorkerPoolTasksCompleted().
    // Used to compute the delta for the Prometheus counter increment.
    std::atomic<uint64_t> last_pool_tasks_completed_{0};

    /**
     * @brief Initialize Metrics.
     */
    void initializeMetrics();
    /**
     * @brief Initialize Extended Context Metrics.
     */
    void initializeExtendedContextMetrics();  // v1.4.0+ metrics
};

class GrafanaDashboardGenerator {
public:
    struct DashboardConfig {
        std::string title = "ThemisDB LLM Monitoring";
        std::string datasource = "Prometheus";
        int refresh_interval_sec = 5;
        bool enable_alerts = true;
    };
    
    /**
     * @brief Grafana Dashboard Generator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GrafanaDashboardGenerator(const DashboardConfig& config);
    
    /**
     * @brief Generate complete dashboard JSON
     * @return Return value.
     */
    std::string generateDashboard() const;

    /**
     * @brief Generate Unified Dashboard.
     * @return Return value.
     */
    std::string generateUnifiedDashboard() const;
    
    // Generate individual panels
    /**
     * @brief Generate Inference Panel.
     * @return Return value.
     */
    std::string generateInferencePanel() const;
    /**
     * @brief Generate Latency Panel.
     * @return Return value.
     */
    std::string generateLatencyPanel() const;
    /**
     * @brief Generate Throughput Panel.
     * @return Return value.
     */
    std::string generateThroughputPanel() const;
    /**
     * @brief Generate GPUPanel.
     * @return Return value.
     */
    std::string generateGPUPanel() const;
    /**
     * @brief Generate Cache Panel.
     * @return Return value.
     */
    std::string generateCachePanel() const;
    /**
     * @brief Generate Scheduler Panel.
     * @return Return value.
     */
    std::string generateSchedulerPanel() const;
    /**
     * @brief Generate Error Panel.
     * @return Return value.
     */
    std::string generateErrorPanel() const;
    
    /**
     * @brief Save dashboard to file
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveDashboard(const std::string& filepath) const;
    
private:
    DashboardConfig config_;
    
    /**
     * @brief Create Panel.
     * @param[in] title Input parameter.
     * @param[in] query Input parameter.
     * @param[in] type Input parameter.
     * @param[in] grid_pos_x Input parameter.
     * @param[in] grid_pos_y Input parameter.
     * @param[in] grid_width Input parameter.
     * @param[in] grid_height Input parameter.
     * @return Return value.
     */
    std::string createPanel(const std::string& title,
                           const std::string& query,
                           const std::string& type,
                           int grid_pos_x, int grid_pos_y,
                           int grid_width, int grid_height) const;
};

class MetricsServer {
public:
    struct ServerConfig {
        std::string host = "0.0.0.0";
        int port = 9090;
        bool enable_cors = true;
        std::string metrics_path        = "/metrics";
        std::string dashboard_path      = "/dashboard";
        std::string health_path         = "/health";
        std::string ready_path          = "/ready";
        std::string models_path         = "/models";
        std::string admin_reload_path   = "/admin/models/reload";
        std::string admin_simulate_path = "/admin/prompt/simulate";
        std::string admin_sessions_path = "/admin/sessions";
    };
    
    /**
     * @brief Metrics Server.
     * @param[in] config Input parameter.
     * @param[in,out] exporter Input/output parameter.
     * @return Return value.
     */
    explicit MetricsServer(const ServerConfig& config,
                          PrometheusExporter* exporter);
    ~MetricsServer();
    
    // Server lifecycle
    /**
     * @brief Start.
     * @return True when the operation succeeds.
     */
    bool start();
    /**
     * @brief Stop.
     */
    void stop();
    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    bool isRunning() const;
    
    // Get server URLs
    /**
     * @brief Get Metrics URL.
     * @return Return value.
     */
    std::string getMetricsURL() const;
    /**
     * @brief Get Dashboard URL.
     * @return Return value.
     */
    std::string getDashboardURL() const;
    /**
     * @brief Get Health URL.
     * @return Return value.
     */
    std::string getHealthURL() const;
    /**
     * @brief Get Ready URL.
     * @return Return value.
     */
    std::string getReadyURL() const;
    /**
     * @brief Get Models URL.
     * @return Return value.
     */
    std::string getModelsURL() const;
    /**
     * @brief Get Admin Reload URL.
     * @return Return value.
     */
    std::string getAdminReloadURL() const;
    /**
     * @brief Get Admin Simulate URL.
     * @return Return value.
     */
    std::string getAdminSimulateURL() const;
    /**
     * @brief Get Admin Sessions URL.
     * @return Return value.
     */
    std::string getAdminSessionsURL() const;

    void setModelInfoCallback(std::function<std::string()> cb) {
        model_info_cb_ = std::move(cb);
    }

    void setDashboardCallback(std::function<std::string()> cb) {
        dashboard_cb_ = std::move(cb);
    }

    void setReloadCallback(std::function<std::string(const std::string&)> cb) {
        reload_cb_ = std::move(cb);
    }

    void setSimulateCallback(std::function<std::string(const std::string&)> cb) {
        simulate_cb_ = std::move(cb);
    }

    void setSessionListCallback(std::function<std::string()> cb) {
        session_list_cb_ = std::move(cb);
    }

    void setSessionDeleteCallback(std::function<std::string(const std::string&)> cb) {
        session_delete_cb_ = std::move(cb);
    }

    /**
     * @brief ── Test-accessible request dispatch ────────────────────────────────────── These methods are called from the httplib route handlers inside Impl.
     * @param[in] path Input parameter.
     * @param[in] body Input parameter.
     * @param[in,out] response Input/output parameter.
     * @details They are exposed publicly so that unit tests can exercise the callback wiring without starting the HTTP listener.
     */

    void handlePost(const std::string& path, const std::string& body,
                    std::string& response);
    /**
     * @brief Handle Delete.
     * @param[in] path Input parameter.
     * @param[in] resource_id Identifier of the resource.
     * @param[in,out] response Input/output parameter.
     */
    void handleDelete(const std::string& path, const std::string& resource_id,
                      std::string& response);

    const ServerConfig& serverConfig() const { return config_; }

private:
    ServerConfig config_;
    PrometheusExporter* exporter_;
    bool running_ = false;
    std::function<std::string()> model_info_cb_;
    std::function<std::string()> dashboard_cb_;
    std::function<std::string(const std::string&)> reload_cb_;
    std::function<std::string(const std::string&)> simulate_cb_;
    std::function<std::string()> session_list_cb_;
    std::function<std::string(const std::string&)> session_delete_cb_;

    // Pimpl: holds httplib::Server and the background listener thread.
    // Defined in grafana_metrics.cpp to keep <httplib.h> out of this header.
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief HTTP GET request handling (called from httplib route handlers inside Impl)
     * @param[in] path Input parameter.
     * @param[in,out] response Input/output parameter.
     */
    void handleRequest(const std::string& path, std::string& response);
};

} // namespace monitoring
} // namespace llm
} // namespace themis
