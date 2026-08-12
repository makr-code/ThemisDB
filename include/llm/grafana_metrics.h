/**
 * @file grafana_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=11; TODO=1, Stub=1, Unimpl=3, Mock=1, Sim=5, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
        double value = 0.0;
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

    explicit LLMMetricsCollector(PrometheusExporter* exporter);
    LLMMetricsCollector(PrometheusExporter* exporter, const Config& config);
    
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
    // Increments llm_backpressure_drops_total when the scheduler rejects a
    // request because max_queue_depth has been reached.
    void recordBackpressureDrop();
    
    // Quantization metrics
    void recordQuantizationFormat(const std::string& model_id, const std::string& format);
    void recordDequantizationLatency(double latency_ms);
    
    // Error metrics
    void recordError(const std::string& error_type, const std::string& component);
    
    // Extended Context Window metrics (v1.4.0+)
    void recordContextLength(const std::string& model_id, size_t context_length);
    void recordContextCacheSize(const std::string& model_id, size_t cache_size_mb);
    void recordExtendedContextEnabled(const std::string& model_id, bool enabled);
    void recordContextScalingFactor(const std::string& model_id, double scaling_factor);
    
    // RoPE/YARN Scaling metrics (v1.4.0+)
    void recordRoPEScalingMethod(const std::string& model_id, const std::string& method);
    void recordRoPEScalingError(const std::string& model_id, const std::string& error);
    void recordYARNParameters(const std::string& model_id, 
                              double ext_factor, double attn_factor,
                              double beta_fast, double beta_slow);
    
    // Memory Profiling metrics (v1.4.0+)
    void recordRAMUsage(const std::string& model_id, size_t ram_mb, size_t total_ram_mb);
    void recordVRAMUsage(const std::string& model_id, size_t vram_mb, size_t total_vram_mb);
    void recordMemoryPressure(const std::string& model_id, double pressure_pct);
    void recordOOMEvent(const std::string& model_id, const std::string& reason);
    void recordMemoryEstimate(const std::string& model_id, 
                             size_t estimated_mb, size_t actual_mb);
    
    // Thread Safety metrics (v1.4.0+)
    void recordLoRAAdapterSwitch(const std::string& model_id, 
                                 const std::string& from_adapter,
                                 const std::string& to_adapter,
                                 double duration_ms);
    void recordContextLockWait(const std::string& model_id, double wait_time_ms);
    void recordConcurrentLoRAOperation(const std::string& model_id, bool sequential_mode);

    // Shared Worker Pool metrics (Phase 2 — Q2 2026)
    // llm_worker_pool_queue_depth  : gauge   — current pending-task depth
    // llm_worker_pool_tasks_completed_total : counter — tasks finished since start
    void recordWorkerPoolQueueDepth(size_t depth);
    void recordWorkerPoolTasksCompleted(uint64_t total_completed);

    // ── Unified dashboard metrics (Phase 2 — Q3 2026) ────────────────────────
    // Engine-typed variants for the unified metrics dashboard.
    // engine_type: "async"     → AsyncInferenceEngine
    //              "enhanced"  → InferenceEngineEnhanced
    //
    // Prometheus metric names used:
    //   llm_engine_inference_requests_total{model_id, engine_type}
    //   llm_engine_inference_success_total{model_id, engine_type}
    //   llm_engine_inference_failures_total{model_id, engine_type, error}
    //   llm_engine_inference_duration_ms{model_id, engine_type}
    //   llm_engine_tokens_generated_total{model_id, engine_type}
    //   llm_engine_queue_depth{engine_type}
    void recordEngineInferenceRequest(const std::string& model_id,
                                      const std::string& engine_type);
    void recordEngineInferenceSuccess(const std::string& model_id,
                                      const std::string& engine_type,
                                      double duration_ms);
    void recordEngineInferenceFailure(const std::string& model_id,
                                      const std::string& engine_type,
                                      const std::string& error);
    void recordEngineTokensGenerated(const std::string& model_id,
                                     const std::string& engine_type,
                                     size_t count);
    void recordEngineQueueDepth(const std::string& engine_type, size_t depth);

private:
    PrometheusExporter* exporter_;
    Config config_;

    // Last absolute value reported by recordWorkerPoolTasksCompleted().
    // Used to compute the delta for the Prometheus counter increment.
    std::atomic<uint64_t> last_pool_tasks_completed_{0};

    void initializeMetrics();
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
    
    explicit GrafanaDashboardGenerator(const DashboardConfig& config);
    
    // Generate complete dashboard JSON
    std::string generateDashboard() const;

    /**
     * @brief Generate a unified Grafana dashboard JSON for both engines.
     *
     * Produces a Grafana dashboard that displays engine-typed metrics
     * (label engine_type="async" for AsyncInferenceEngine and
     * engine_type="enhanced" for InferenceEngineEnhanced) side-by-side,
     * together with shared worker-pool and cache panels.
     */
    std::string generateUnifiedDashboard() const;
    
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
        std::string metrics_path        = "/metrics";
        std::string dashboard_path      = "/dashboard";
        std::string health_path         = "/health";
        std::string ready_path          = "/ready";
        std::string models_path         = "/models";
        std::string admin_reload_path   = "/admin/models/reload";
        std::string admin_simulate_path = "/admin/prompt/simulate";
        std::string admin_sessions_path = "/admin/sessions";
    };
    
    explicit MetricsServer(const ServerConfig& config,
                          PrometheusExporter* exporter);
    ~MetricsServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool isRunning() const;
    
    // Get server URLs
    std::string getMetricsURL() const;
    std::string getDashboardURL() const;
    std::string getHealthURL() const;
    std::string getReadyURL() const;
    std::string getModelsURL() const;
    std::string getAdminReloadURL() const;
    std::string getAdminSimulateURL() const;
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
    
    // HTTP GET request handling (called from httplib route handlers inside Impl)
    void handleRequest(const std::string& path, std::string& response);
};

} // namespace monitoring
} // namespace llm
} // namespace themis
