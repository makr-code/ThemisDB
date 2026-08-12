/**
 * @file ml_model_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llm_model_storage.h"
#include "llm/model_loader.h"
#include "llm/inference_engine_enhanced.h"
#include "utils/expected.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

/**
 * @brief ML Model types supported by ThemisDB
 */
enum class MLModelType {
    LLM,                    // Large Language Model
    CLASSIFIER,             // Classification model
    REGRESSOR,              // Regression model
    EMBEDDING,              // Embedding/encoder model
    VISION,                 // Vision/image model
    SPEECH,                 // Speech/audio model
    MULTIMODAL,             // Multimodal model
    CUSTOM                  // Custom model type
};

/**
 * @brief ML Model status in its lifecycle
 */
enum class MLModelStatus {
    REGISTERED,             // Model registered but not deployed
    DEPLOYING,              // Model being deployed
    DEPLOYED,               // Model deployed and ready
    UPDATING,               // Model being updated
    DEGRADED,               // Model deployed but unhealthy
    RETIRED,                // Model retired/deprecated
    FAILED                  // Model deployment failed
};

/**
 * @brief ML Model configuration
 */
struct MLModelConfig {
    std::string model_id;
    std::string model_name;
    std::string version;
    MLModelType type;
    
    // Deployment configuration
    std::string file_path;
    std::string format;                 // "gguf", "onnx", "pytorch", "tensorflow"
    json inference_config;              // Model-specific inference config
    
    // Resource allocation
    int gpu_device_id = -1;             // -1 for CPU, >=0 for GPU
    size_t max_batch_size = 32;
    size_t max_concurrent_requests = 100;
    int timeout_ms = 30000;
    
    // Auto-scaling
    bool enable_auto_scaling = false;
    size_t min_instances = 1;
    size_t max_instances = 4;
    float scale_up_threshold = 0.8f;    // Scale up at 80% utilization
    float scale_down_threshold = 0.3f;  // Scale down at 30% utilization
    
    // Health check
    bool enable_health_check = true;
    int health_check_interval_ms = 30000;
    int unhealthy_threshold = 3;        // Failed health checks before marking degraded
    
    json toJSON() const {
        return json{
            {"model_id", model_id},
            {"model_name", model_name},
            {"version", version},
            {"type", static_cast<int>(type)},
            {"file_path", file_path},
            {"format", format},
            {"inference_config", inference_config},
            {"gpu_device_id", gpu_device_id},
            {"max_batch_size", max_batch_size},
            {"max_concurrent_requests", max_concurrent_requests},
            {"timeout_ms", timeout_ms},
            {"enable_auto_scaling", enable_auto_scaling},
            {"min_instances", min_instances},
            {"max_instances", max_instances},
            {"scale_up_threshold", scale_up_threshold},
            {"scale_down_threshold", scale_down_threshold},
            {"enable_health_check", enable_health_check},
            {"health_check_interval_ms", health_check_interval_ms},
            {"unhealthy_threshold", unhealthy_threshold}
        };
    }
};

/**
 * @brief ML Model instance information
 */
struct MLModelInstance {
    virtual ~MLModelInstance() = default;
    std::string instance_id;
    std::string model_id;
    MLModelStatus status;
    
    // Runtime information
    int gpu_device_id = -1;
    size_t active_requests = 0;
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    
    // Performance metrics
    float avg_latency_ms = 0.0f;
    float p95_latency_ms = 0.0f;
    float p99_latency_ms = 0.0f;
    float requests_per_second = 0.0f;

    // Sliding window of recent latency samples for percentile computation
    static constexpr size_t kLatencyWindowSize = 200;
    std::deque<float> latency_window;
    
    // Health
    int consecutive_health_check_failures = 0;
    std::chrono::system_clock::time_point last_health_check;
    std::chrono::system_clock::time_point deployed_at;
    std::chrono::system_clock::time_point last_request_at;
    
    json toJSON() const {
        auto deployed_ts = std::chrono::system_clock::to_time_t(deployed_at);
        auto last_req_ts = std::chrono::system_clock::to_time_t(last_request_at);
        auto last_health_ts = std::chrono::system_clock::to_time_t(last_health_check);
        
        return json{
            {"instance_id", instance_id},
            {"model_id", model_id},
            {"status", static_cast<int>(status)},
            {"gpu_device_id", gpu_device_id},
            {"active_requests", active_requests},
            {"total_requests", total_requests},
            {"successful_requests", successful_requests},
            {"failed_requests", failed_requests},
            {"avg_latency_ms", avg_latency_ms},
            {"p95_latency_ms", p95_latency_ms},
            {"p99_latency_ms", p99_latency_ms},
            {"requests_per_second", requests_per_second},
            {"consecutive_health_check_failures", consecutive_health_check_failures},
            {"last_health_check", last_health_ts},
            {"deployed_at", deployed_ts},
            {"last_request_at", last_req_ts}
        };
    }
};

/**
 * @brief ML Model inference request
 */
struct MLInferenceRequest {
    virtual ~MLInferenceRequest() = default;
    std::string model_id;
    std::string model_version;          // Optional: specific version, or "latest"
    json input_data;                    // Model-specific input format
    json inference_params;              // Model-specific parameters
    int timeout_ms = 30000;
    int priority = 0;
};

/**
 * @brief ML Model inference response
 */
struct MLInferenceResponse {
    bool success = false;
    json output_data;                   // Model-specific output format
    std::string error_message;
    
    // Metadata
    std::string model_id;
    std::string model_version;
    std::string instance_id;
    
    // Performance
    float inference_time_ms = 0.0f;
    float queue_time_ms = 0.0f;
    float total_time_ms = 0.0f;
};

/**
 * @brief Unified ML Model Manager
 * 
 * Manages the lifecycle of different types of ML models:
 * - Registration: Register new models with metadata
 * - Deployment: Deploy models and create instances
 * - Inference: Route inference requests to appropriate instances
 * - Scaling: Auto-scale model instances based on load
 * - Health: Monitor model health and handle failures
 * - Retirement: Gracefully retire old models
 */
class MLModelManager {
public:
    struct Config {
        // Storage
        std::shared_ptr<RocksDBWrapper> db;
        std::shared_ptr<LLMModelStorage> model_storage;
        std::shared_ptr<LazyModelLoader> model_loader;
        std::shared_ptr<InferenceEngineEnhanced> inference_engine;
        
        // Defaults
        size_t default_max_instances = 4;
        size_t default_max_concurrent_requests = 100;
        
        // Health monitoring
        bool enable_health_monitoring = true;
        int health_check_interval_ms = 30000;
        
        // Auto-scaling
        bool enable_auto_scaling = false;
        int scaling_check_interval_ms = 60000;
    };
    
    explicit MLModelManager(const Config& config);
    ~MLModelManager();
    
    // ═══════════════════════════════════════════════════════════
    // Model Lifecycle Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Register a new ML model
     * 
     * Registers a model with its configuration and metadata.
     * The model is not deployed until deploy() is called.
     * 
     * @param config Model configuration
     * @return true if registered successfully
     */
    Result<bool> registerModel(const MLModelConfig& config);
    
    /**
     * @brief Deploy a registered model
     * 
     * Creates one or more instances of the model and makes it available for inference.
     * 
     * @param model_id Model identifier
     * @param num_instances Number of instances to deploy
     * @return Result with deployed instance IDs
     */
    Result<std::vector<std::string>> deployModel(
        const std::string& model_id,
        size_t num_instances = 1
    );
    
    /**
     * @brief Update a deployed model
     * 
     * Updates model configuration (rolling update with zero downtime).
     * 
     * @param model_id Model identifier
     * @param new_config Updated configuration
     * @return true if updated successfully
     */
    Result<bool> updateModel(
        const std::string& model_id,
        const MLModelConfig& new_config
    );
    
    /**
     * @brief Retire a model
     * 
     * Marks a model as retired and stops accepting new requests.
     * Existing requests are allowed to complete.
     * 
     * @param model_id Model identifier
     * @param drain_timeout_ms Time to wait for pending requests
     * @return true if retired successfully
     */
    Result<bool> retireModel(
        const std::string& model_id,
        int drain_timeout_ms = 30000
    );
    
    /**
     * @brief Unregister a model
     * 
     * Completely removes a model and all its instances.
     * Model must be retired first.
     * 
     * @param model_id Model identifier
     * @return true if unregistered successfully
     */
    Result<bool> unregisterModel(const std::string& model_id);
    
    // ═══════════════════════════════════════════════════════════
    // Model Query and Discovery
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief List all registered models
     * 
     * @param filter Optional filter by type or status
     * @return Vector of model IDs
     */
    std::vector<std::string> listModels(const json& filter = {}) const;
    
    /**
     * @brief Get model configuration
     * 
     * @param model_id Model identifier
     * @return Model configuration
     */
    Result<MLModelConfig> getModelConfig(const std::string& model_id) const;
    
    /**
     * @brief Get model status
     * 
     * @param model_id Model identifier
     * @return Model status
     */
    Result<MLModelStatus> getModelStatus(const std::string& model_id) const;
    
    /**
     * @brief List model instances
     * 
     * @param model_id Model identifier
     * @return Vector of model instances
     */
    std::vector<MLModelInstance> listModelInstances(const std::string& model_id) const;
    
    /**
     * @brief Get model metrics
     * 
     * @param model_id Model identifier
     * @return Model metrics as JSON
     */
    json getModelMetrics(const std::string& model_id) const;
    
    // ═══════════════════════════════════════════════════════════
    // Inference Operations
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Run inference on a model (synchronous)
     * 
     * Routes the request to an available model instance and waits for the result.
     * 
     * @param request Inference request
     * @return Inference response
     */
    Result<MLInferenceResponse> infer(const MLInferenceRequest& request);

    // ─── Inference dispatch injection ────────────────────────────────────────
    /**
     * @brief Type alias for an injectable inference dispatch function.
     *
     * When set via @c setInferenceDispatchFn(), @c infer() calls this function
     * instead of the built-in simulated path.  The function receives the
     * original request and a reference to the selected instance and returns
     * the raw output payload (or an empty json on error).
     *
     * Example (test / staging injection):
     * @code
     *   manager.setInferenceDispatchFn(
     *       [](const MLInferenceRequest& req, MLModelInstance&) -> json {
     *           return json{{"result", "real-output-for-" + req.model_id}};
     *       });
     * @endcode
     */
    using InferenceDispatchFn =
        std::function<json(const MLInferenceRequest&, MLModelInstance&)>;

    /**
     * @brief Inject a real inference dispatch function.
     *
     * Replaces the built-in simulated backend with @p fn.  Calling with
     * @c nullptr resets to the simulated fallback.  Thread-safe: guarded by
     * the internal dispatch mutex.
     *
     * @param fn Callable that performs the actual model inference.
     */
    void setInferenceDispatchFn(InferenceDispatchFn fn);
    
    /**
     * @brief Run inference on a model (asynchronous)
     * 
     * Submits inference request and returns immediately.
     * Result is delivered via callback.
     * 
     * @param request Inference request
     * @param callback Callback for result
     * @return Request ID for tracking
     */
    std::string inferAsync(
        const MLInferenceRequest& request,
        std::function<void(const MLInferenceResponse&)> callback
    );
    
    /**
     * @brief Cancel an async inference request
     * 
     * @param request_id Request identifier
     * @return true if cancelled successfully
     */
    bool cancelInference(const std::string& request_id);
    
    // ═══════════════════════════════════════════════════════════
    // Instance Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Scale model instances
     * 
     * Manually adjust the number of model instances.
     * 
     * @param model_id Model identifier
     * @param num_instances Desired number of instances
     * @return true if scaled successfully
     */
    Result<bool> scaleModel(const std::string& model_id, size_t num_instances);
    
    /**
     * @brief Perform health check on model instance
     * 
     * @param instance_id Instance identifier
     * @return true if healthy
     */
    bool healthCheck(const std::string& instance_id);
    
    /**
     * @brief Restart unhealthy instance
     * 
     * @param instance_id Instance identifier
     * @return true if restarted successfully
     */
    Result<bool> restartInstance(const std::string& instance_id);
    
    // ═══════════════════════════════════════════════════════════
    // System Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Start the ML Model Manager
     * 
     * Starts background threads for health monitoring and auto-scaling.
     */
    void start();
    
    /**
     * @brief Shutdown the ML Model Manager
     * 
     * Gracefully shuts down all models and background threads.
     */
    void shutdown();
    
    /**
     * @brief Get system-wide statistics
     * 
     * @return Statistics as JSON
     */
    json getSystemStats() const;
    
private:
    Config config_;
    
    // Model registry
    struct ModelEntry {
        MLModelConfig config;
        MLModelStatus status;
        std::vector<std::unique_ptr<MLModelInstance>> instances;
        std::chrono::system_clock::time_point registered_at;
        std::chrono::system_clock::time_point deployed_at;
    };
    
    std::unordered_map<std::string, std::unique_ptr<ModelEntry>> models_;
    mutable std::mutex models_mutex_;
    
    // Background threads
    std::unique_ptr<std::thread> health_monitor_thread_;
    std::unique_ptr<std::thread> auto_scaler_thread_;
    std::atomic<bool> running_{false};
    
    // Statistics
    std::atomic<size_t> total_requests_{0};
    std::atomic<size_t> successful_requests_{0};
    std::atomic<size_t> failed_requests_{0};
    
    // Internal methods
    void healthMonitorLoop();
    void autoScalerLoop();
    
    Result<std::string> deployInstance(const std::string& model_id, const MLModelConfig& config);
    bool shutdownInstance(const std::string& instance_id);
    
    MLModelInstance* selectInstance(const std::string& model_id);
    /// Selects the least-busy DEPLOYED instance from an already-locked ModelEntry.
    /// Caller MUST hold models_mutex_.  Returns nullptr when no DEPLOYED instance exists.
    [[nodiscard]] MLModelInstance* selectLeastBusy_(const ModelEntry& entry) const noexcept;
    void updateInstanceMetrics(MLModelInstance* instance, float latency_ms, bool success);
    
    std::string generateInstanceId(const std::string& model_id);
    std::atomic<uint64_t> instance_counter_{0};
    
    std::string generateRequestId();
    std::atomic<uint64_t> request_counter_{0};

    // In-flight request cancellation tracking
    std::unordered_set<std::string> cancelled_requests_;
    std::mutex cancel_mutex_;

    // Injection slot for real inference dispatch (stub #250)
    InferenceDispatchFn inference_dispatch_fn_;
    mutable std::mutex dispatch_fn_mutex_;
};

} // namespace llm
} // namespace themis

