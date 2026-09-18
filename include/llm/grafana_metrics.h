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
    
    /**
     * @brief Metric registration
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
    
    /**
     * @brief Reset all metrics
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
     * @brief TBD: Describe serializeMetric.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Configuration for LLMMetricsCollector.
     *
     * All threshold fields accept values in milliseconds and can be tuned
     * without recompilation:
     *
     *  - Distributed / high-latency deployments: raise thresholds (200–500 ms)
     *  - Local / high-performance deployments: lower thresholds (50–100 ms)
     *
     * Additional per-metric thresholds (e.g. first-token latency alert budget)
     * may be added to this struct in future minor versions without breaking
     * existing call sites.
     */
    struct Config {
        /**
         * @brief Minimum wait time (ms) that counts as a lock-contention event.
         *
         * Increments `llm_context_lock_contention_total` whenever
         * `recordContextLockWait()` is called with a value exceeding this.
         * Default: 100 ms.
         */
        double lock_contention_threshold_ms = 100.0;
    };

    /**
     * @brief TBD: Describe LLMMetricsCollector.
     * @param[in,out] exporter Input/output parameter.
     * @return Return value.
     */
    explicit LLMMetricsCollector(PrometheusExporter* exporter);
    LLMMetricsCollector(PrometheusExporter* exporter, const Config& config);
    
    /**
     * @brief Inference metrics
     * @param[in] model_id Input parameter.
     */
    void recordInferenceRequest(const std::string& model_id);
    /**
     * @brief TBD: Describe recordInferenceSuccess.
     * @param[in] model_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordInferenceSuccess(const std::string& model_id, double duration_ms);
    /**
     * @brief TBD: Describe recordInferenceFailure.
     * @param[in] model_id Input parameter.
     * @param[in] error Input parameter.
     */
    void recordInferenceFailure(const std::string& model_id, const std::string& error);
    
    /**
     * @brief Latency metrics
     * @param[in] model_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordFirstTokenLatency(const std::string& model_id, double latency_ms);
    /**
     * @brief TBD: Describe recordPerTokenLatency.
     * @param[in] model_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPerTokenLatency(const std::string& model_id, double latency_ms);
    /**
     * @brief TBD: Describe recordEndToEndLatency.
     * @param[in] model_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordEndToEndLatency(const std::string& model_id, double latency_ms);
    
    /**
     * @brief Throughput metrics
     * @param[in] model_id Input parameter.
     * @param[in] count Input parameter.
     */
    void recordTokensGenerated(const std::string& model_id, size_t count);
    /**
     * @brief TBD: Describe recordBatchSize.
     * @param[in] batch_size Input parameter.
     */
    void recordBatchSize(size_t batch_size);
    /**
     * @brief TBD: Describe recordConcurrentRequests.
     * @param[in] count Input parameter.
     */
    void recordConcurrentRequests(size_t count);
    
    /**
     * @brief GPU metrics
     * @param[in] vram_mb Input parameter.
     * @param[in] total_vram_mb Input parameter.
     */
    void recordGPUMemoryUsage(size_t vram_mb, size_t total_vram_mb);
    /**
     * @brief TBD: Describe recordGPUUtilization.
     * @param[in] utilization_pct Input parameter.
     */
    void recordGPUUtilization(double utilization_pct);
    /**
     * @brief TBD: Describe recordGPUTemperature.
     * @param[in] temp_celsius Input parameter.
     */
    void recordGPUTemperature(double temp_celsius);
    
    /**
     * @brief Model metrics
     * @param[in] model_id Input parameter.
     * @param[in] vram_mb Input parameter.
     */
    void recordModelLoaded(const std::string& model_id, size_t vram_mb);
    /**
     * @brief TBD: Describe recordModelUnloaded.
     * @param[in] model_id Input parameter.
     */
    void recordModelUnloaded(const std::string& model_id);
    /**
     * @brief TBD: Describe recordModelSwitchLatency.
     * @param[in] latency_ms Input parameter.
     */
    void recordModelSwitchLatency(double latency_ms);
    
    /**
     * @brief Cache metrics
     * @param[in] cache_type Input parameter.
     */
    void recordCacheHit(const std::string& cache_type);
    /**
     * @brief TBD: Describe recordCacheMiss.
     * @param[in] cache_type Input parameter.
     */
    void recordCacheMiss(const std::string& cache_type);
    /**
     * @brief TBD: Describe recordCacheSize.
     * @param[in] cache_type Input parameter.
     * @param[in] size_mb Input parameter.
     */
    void recordCacheSize(const std::string& cache_type, size_t size_mb);
    
    /**
     * @brief Scheduler metrics
     * @param[in] length Input parameter.
     */
    void recordQueueLength(size_t length);
    /**
     * @brief TBD: Describe recordPreemptions.
     * @param[in] count Input parameter.
     */
    void recordPreemptions(size_t count);
    /**
     * @brief TBD: Describe recordSchedulingLatency.
     * @param[in] latency_ms Input parameter.
     */
    void recordSchedulingLatency(double latency_ms);
    /**
     * @brief Increments llm_backpressure_drops_total when the scheduler rejects a request because max_queue_depth has been reached.
     */
    void recordBackpressureDrop();
    
    /**
     * @brief Quantization metrics
     * @param[in] model_id Input parameter.
     * @param[in] format Input parameter.
     */
    void recordQuantizationFormat(const std::string& model_id, const std::string& format);
    /**
     * @brief TBD: Describe recordDequantizationLatency.
     * @param[in] latency_ms Input parameter.
     */
    void recordDequantizationLatency(double latency_ms);
    
    /**
     * @brief Error metrics
     * @param[in] error_type Input parameter.
     * @param[in] component Input parameter.
     */
    void recordError(const std::string& error_type, const std::string& component);
    
    /**
     * @brief Extended Context Window metrics (v1.
     * @param[in] model_id Input parameter.
     * @param[in] context_length Input parameter.
     * @details 4.0+)
     */
    void recordContextLength(const std::string& model_id, size_t context_length);
    /**
     * @brief TBD: Describe recordContextCacheSize.
     * @param[in] model_id Input parameter.
     * @param[in] cache_size_mb Input parameter.
     */
    void recordContextCacheSize(const std::string& model_id, size_t cache_size_mb);
    /**
     * @brief TBD: Describe recordExtendedContextEnabled.
     * @param[in] model_id Input parameter.
     * @param[in] enabled Input parameter.
     */
    void recordExtendedContextEnabled(const std::string& model_id, bool enabled);
    /**
     * @brief TBD: Describe recordContextScalingFactor.
     * @param[in] model_id Input parameter.
     * @param[in] scaling_factor Input parameter.
     */
    void recordContextScalingFactor(const std::string& model_id, double scaling_factor);
    
    /**
     * @brief RoPE/YARN Scaling metrics (v1.
     * @param[in] model_id Input parameter.
     * @param[in] method Input parameter.
     * @details 4.0+)
     */
    void recordRoPEScalingMethod(const std::string& model_id, const std::string& method);
    /**
     * @brief TBD: Describe recordRoPEScalingError.
     * @param[in] model_id Input parameter.
     * @param[in] error Input parameter.
     */
    void recordRoPEScalingError(const std::string& model_id, const std::string& error);
    /**
     * @brief TBD: Describe recordYARNParameters.
     * @param[in] model_id Input parameter.
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
     * @param[in] model_id Input parameter.
     * @param[in] ram_mb Input parameter.
     * @param[in] total_ram_mb Input parameter.
     * @details 4.0+)
     */
    void recordRAMUsage(const std::string& model_id, size_t ram_mb, size_t total_ram_mb);
    /**
     * @brief TBD: Describe recordVRAMUsage.
     * @param[in] model_id Input parameter.
     * @param[in] vram_mb Input parameter.
     * @param[in] total_vram_mb Input parameter.
     */
    void recordVRAMUsage(const std::string& model_id, size_t vram_mb, size_t total_vram_mb);
    /**
     * @brief TBD: Describe recordMemoryPressure.
     * @param[in] model_id Input parameter.
     * @param[in] pressure_pct Input parameter.
     */
    void recordMemoryPressure(const std::string& model_id, double pressure_pct);
    /**
     * @brief TBD: Describe recordOOMEvent.
     * @param[in] model_id Input parameter.
     * @param[in] reason Input parameter.
     */
    void recordOOMEvent(const std::string& model_id, const std::string& reason);
    /**
     * @brief TBD: Describe recordMemoryEstimate.
     * @param[in] model_id Input parameter.
     * @param[in] estimated_mb Input parameter.
     * @param[in] actual_mb Input parameter.
     */
    void recordMemoryEstimate(const std::string& model_id, 
                             size_t estimated_mb, size_t actual_mb);
    
    /**
     * @brief Thread Safety metrics (v1.
     * @param[in] model_id Input parameter.
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
     * @brief TBD: Describe recordContextLockWait.
     * @param[in] model_id Input parameter.
     * @param[in] wait_time_ms Input parameter.
     */
    void recordContextLockWait(const std::string& model_id, double wait_time_ms);
    /**
     * @brief TBD: Describe recordConcurrentLoRAOperation.
     * @param[in] model_id Input parameter.
     * @param[in] sequential_mode Input parameter.
     */
    void recordConcurrentLoRAOperation(const std::string& model_id, bool sequential_mode);

    /**
     * @brief Shared Worker Pool metrics (Phase 2 — Q2 2026) llm_worker_pool_queue_depth : gauge — current pending-task depth llm_worker_pool_tasks_completed_total : counter — tasks finished since start
     * @param[in] depth Input parameter.
     */
    void recordWorkerPoolQueueDepth(size_t depth);
    /**
     * @brief TBD: Describe recordWorkerPoolTasksCompleted.
     * @param[in] total_completed Input parameter.
     */
    void recordWorkerPoolTasksCompleted(uint64_t total_completed);

    /**
     * @brief ── Unified dashboard metrics (Phase 2 — Q3 2026) ──────────────────────── Engine-typed variants for the unified metrics dashboard.
     * @param[in] model_id Input parameter.
     * @param[in] engine_type Input parameter.
     * @details engine_type: "async" → AsyncInferenceEngine "enhanced" → InferenceEngineEnhanced Prometheus metric names used: llm_engine_inference_requests_total{model_id, engine_type} llm_engine_inference_success_total{model_id, engine_type} llm_engine_inference_failures_total{model_id, engine_type, error} llm_engine_inference_duration_ms{model_id, engine_type} llm_engine_tokens_generated_total{model_id, engine_type} llm_engine_queue_depth{engine_type}
     */
    void recordEngineInferenceRequest(const std::string& model_id,
                                      const std::string& engine_type);
    /**
     * @brief TBD: Describe recordEngineInferenceSuccess.
     * @param[in] model_id Input parameter.
     * @param[in] engine_type Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordEngineInferenceSuccess(const std::string& model_id,
                                      const std::string& engine_type,
                                      double duration_ms);
    /**
     * @brief TBD: Describe recordEngineInferenceFailure.
     * @param[in] model_id Input parameter.
     * @param[in] engine_type Input parameter.
     * @param[in] error Input parameter.
     */
    void recordEngineInferenceFailure(const std::string& model_id,
                                      const std::string& engine_type,
                                      const std::string& error);
    /**
     * @brief TBD: Describe recordEngineTokensGenerated.
     * @param[in] model_id Input parameter.
     * @param[in] engine_type Input parameter.
     * @param[in] count Input parameter.
     */
    void recordEngineTokensGenerated(const std::string& model_id,
                                     const std::string& engine_type,
                                     size_t count);
    /**
     * @brief TBD: Describe recordEngineQueueDepth.
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
     * @brief TBD: Describe initializeMetrics.
     */
    void initializeMetrics();
    /**
     * @brief TBD: Describe initializeExtendedContextMetrics.
     */
    void initializeExtendedContextMetrics();  // v1.4.0+ metrics
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
    
    /**
     * @brief TBD: Describe GrafanaDashboardGenerator.
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
     * @brief Generate a unified Grafana dashboard JSON for both engines.
     *
     * Produces a Grafana dashboard that displays engine-typed metrics
     * (label engine_type="async" for AsyncInferenceEngine and
     * engine_type="enhanced" for InferenceEngineEnhanced) side-by-side,
     * together with shared worker-pool and cache panels.
     * @return Return value.
     */
    std::string generateUnifiedDashboard() const;
    
    /**
     * @brief Generate individual panels
     * @return Return value.
     */
    std::string generateInferencePanel() const;
    /**
     * @brief TBD: Describe generateLatencyPanel.
     * @return Return value.
     */
    std::string generateLatencyPanel() const;
    /**
     * @brief TBD: Describe generateThroughputPanel.
     * @return Return value.
     */
    std::string generateThroughputPanel() const;
    /**
     * @brief TBD: Describe generateGPUPanel.
     * @return Return value.
     */
    std::string generateGPUPanel() const;
    /**
     * @brief TBD: Describe generateCachePanel.
     * @return Return value.
     */
    std::string generateCachePanel() const;
    /**
     * @brief TBD: Describe generateSchedulerPanel.
     * @return Return value.
     */
    std::string generateSchedulerPanel() const;
    /**
     * @brief TBD: Describe generateErrorPanel.
     * @return Return value.
     */
    std::string generateErrorPanel() const;
    
    /**
     * @brief Save dashboard to file
     * @param[in] filepath Input parameter.
     * @return True on success.
     */
    bool saveDashboard(const std::string& filepath) const;
    
private:
    DashboardConfig config_;
    
    /**
     * @brief TBD: Describe createPanel.
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
     * @brief TBD: Describe MetricsServer.
     * @param[in] config Input parameter.
     * @param[in,out] exporter Input/output parameter.
     * @return Return value.
     */
    explicit MetricsServer(const ServerConfig& config,
                          PrometheusExporter* exporter);
    ~MetricsServer();
    
    /**
     * @brief Server lifecycle
     * @return True on success.
     */
    bool start();
    /**
     * @brief TBD: Describe stop.
     */
    void stop();
    /**
     * @brief TBD: Describe isRunning.
     * @return True on success.
     */
    bool isRunning() const;
    
    /**
     * @brief Get server URLs
     * @return Return value.
     */
    std::string getMetricsURL() const;
    /**
     * @brief TBD: Describe getDashboardURL.
     * @return Return value.
     */
    std::string getDashboardURL() const;
    /**
     * @brief TBD: Describe getHealthURL.
     * @return Return value.
     */
    std::string getHealthURL() const;
    /**
     * @brief TBD: Describe getReadyURL.
     * @return Return value.
     */
    std::string getReadyURL() const;
    /**
     * @brief TBD: Describe getModelsURL.
     * @return Return value.
     */
    std::string getModelsURL() const;
    /**
     * @brief TBD: Describe getAdminReloadURL.
     * @return Return value.
     */
    std::string getAdminReloadURL() const;
    /**
     * @brief TBD: Describe getAdminSimulateURL.
     * @return Return value.
     */
    std::string getAdminSimulateURL() const;
    /**
     * @brief TBD: Describe getAdminSessionsURL.
     * @return Return value.
     */
    std::string getAdminSessionsURL() const;

    /**
     * @brief Register a callback for GET /models.
     * Callable () -> std::string (JSON array). nullptr = return "[]".
     */
    void setModelInfoCallback(std::function<std::string()> cb) {
        model_info_cb_ = std::move(cb);
    }

    /**
     * @brief Register a callback for GET /dashboard.
     *
     * Invoked with no arguments; should return a Grafana dashboard JSON string.
     * When not set, the server generates a default unified dashboard using
     * GrafanaDashboardGenerator with default config.
     *
     * @param cb  Callable () -> std::string (Grafana dashboard JSON).
     */
    void setDashboardCallback(std::function<std::string()> cb) {
        dashboard_cb_ = std::move(cb);
    }

    /**
     * @brief Register a callback for POST /admin/models/reload.
     *
     * Invoked with the raw POST body.  Should trigger a hot-reload of the
     * model named in the body and return a JSON result string.
     * nullptr = return a "not implemented" JSON body.
     *
     * @param cb  Callable (const std::string& body) -> std::string.
     */
    void setReloadCallback(std::function<std::string(const std::string&)> cb) {
        reload_cb_ = std::move(cb);
    }

    /**
     * @brief Register a callback for POST /admin/prompt/simulate.
     *
     * Invoked with the raw POST body (a JSON object with "prompt" and
     * optionally "model_id").  Should perform a dry-run policy check +
     * tokenization and return a JSON result string.
     * nullptr = return a "not implemented" JSON body.
     *
     * @param cb  Callable (const std::string& body) -> std::string.
     */
    void setSimulateCallback(std::function<std::string(const std::string&)> cb) {
        simulate_cb_ = std::move(cb);
    }

    /**
     * @brief Register a callback for GET /admin/sessions.
     *
     * Should return a JSON array of active inference session objects.
     * Each session object should include at least: session_id, model_id,
     * state, queued_at.
     * nullptr = return "[]".
     *
     * @param cb  Callable () -> std::string (JSON array).
     */
    void setSessionListCallback(std::function<std::string()> cb) {
        session_list_cb_ = std::move(cb);
    }

    /**
     * @brief Register a callback for DELETE /admin/sessions/{id}.
     *
     * Invoked with the session_id extracted from the URL path.
     * Should cancel/remove the named session and return a JSON result.
     * nullptr = return a "not implemented" JSON body.
     *
     * @param cb  Callable (const std::string& session_id) -> std::string.
     */
    void setSessionDeleteCallback(std::function<std::string(const std::string&)> cb) {
        session_delete_cb_ = std::move(cb);
    }

    // ── Test-accessible request dispatch ──────────────────────────────────────
    //
    // These methods are called from the httplib route handlers inside Impl.
    // They are exposed publicly so that unit tests can exercise the callback
    // wiring without starting the HTTP listener.

    /// Dispatch a POST request (reload / simulate) to the registered callback.
    void handlePost(const std::string& path, const std::string& body,
                    std::string& response);
    /// Dispatch a DELETE request (session cancel) to the registered callback.
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
