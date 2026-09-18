/**
 * @file ml_model_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class MLModelStatus {
    REGISTERED,             // Model registered but not deployed
    DEPLOYING,              // Model being deployed
    DEPLOYED,               // Model deployed and ready
    UPDATING,               // Model being updated
    DEGRADED,               // Model deployed but unhealthy
    RETIRED,                // Model retired/deprecated
    FAILED                  // Model deployment failed
};

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

struct MLModelInstance {
    /**
     * @brief MLModel Instance.
     * @return Return value.
     */
    virtual ~MLModelInstance() = default;

    // Wave-B L7: thread-safety audit — explicit copy constructor required because
    // active_requests is std::atomic<size_t> (non-copyable by default).
    // Snapshot the loaded value so that copied instances (e.g. listModelInstances())
    // get a consistent point-in-time view.
    MLModelInstance() = default;
    MLModelInstance(const MLModelInstance& o)
        : instance_id(o.instance_id)
        , model_id(o.model_id)
        , status(o.status)
        , gpu_device_id(o.gpu_device_id)
        , active_requests(o.active_requests.load(std::memory_order_relaxed))
        , total_requests(o.total_requests)
        , successful_requests(o.successful_requests)
        , failed_requests(o.failed_requests)
        , avg_latency_ms(o.avg_latency_ms)
        , p95_latency_ms(o.p95_latency_ms)
        , p99_latency_ms(o.p99_latency_ms)
        , requests_per_second(o.requests_per_second)
        , latency_window(o.latency_window)
        , consecutive_health_check_failures(o.consecutive_health_check_failures)
        , last_health_check(o.last_health_check)
        , deployed_at(o.deployed_at)
        , last_request_at(o.last_request_at)
    {}

    std::string instance_id;
    std::string model_id;
    MLModelStatus status;
    
    // Runtime information
    int gpu_device_id = -1;
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    // active_requests is incremented/decremented by concurrent infer() calls without a
    // global lock; must be atomic to prevent data races (UB under C++11 memory model).
    std::atomic<size_t> active_requests{0};
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
            {"active_requests", active_requests.load(std::memory_order_relaxed)},
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

struct MLInferenceRequest {
    /**
     * @brief MLInference Request.
     * @return Return value.
     */
    virtual ~MLInferenceRequest() = default;
    std::string model_id;
    std::string model_version;          // Optional: specific version, or "latest"
    json input_data;                    // Model-specific input format
    json inference_params;              // Model-specific parameters
    int timeout_ms = 30000;
    int priority = 0;
};

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
    
    /**
     * @brief MLModel Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MLModelManager(const Config& config);
    // B1-EXCEPTION-SAFETY(2026-08-26): noexcept — shutdown() exceptions swallowed.
    ~MLModelManager() noexcept;
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Model Lifecycle Management ═══════════════════════════════════════════════════════════
     * @param[in] config Input parameter.
     * @return Return value.
     */
    
    Result<bool> registerModel(const MLModelConfig& config);
    
    Result<std::vector<std::string>> deployModel(
        const std::string& model_id,
        size_t num_instances = 1
    );
    
    /**
     * @brief Update Model.
     * @param[in] model_id Identifier of the model.
     * @param[in] new_config Input parameter.
     * @return Return value.
     */
    Result<bool> updateModel(
        const std::string& model_id,
        const MLModelConfig& new_config
    );
    
    Result<bool> retireModel(
        const std::string& model_id,
        int drain_timeout_ms = 30000
    );
    
    /**
     * @brief Unregister Model.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    Result<bool> unregisterModel(const std::string& model_id);
    
    // ═══════════════════════════════════════════════════════════
    // Model Query and Discovery
    // ═══════════════════════════════════════════════════════════
    
    std::vector<std::string> listModels(const json& filter = {}) const;
    
    /**
     * @brief Get Model Config.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    Result<MLModelConfig> getModelConfig(const std::string& model_id) const;
    
    /**
     * @brief Get Model Status.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    Result<MLModelStatus> getModelStatus(const std::string& model_id) const;
    
    /**
     * @brief List Model Instances.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::vector<MLModelInstance> listModelInstances(const std::string& model_id) const;
    
    /**
     * @brief Get Model Metrics.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    json getModelMetrics(const std::string& model_id) const;
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Inference Operations ═══════════════════════════════════════════════════════════
     * @param[in] request Input parameter.
     * @return Return value.
     */
    
    Result<MLInferenceResponse> infer(const MLInferenceRequest& request);

    // ─── Inference dispatch injection ────────────────────────────────────────
    using InferenceDispatchFn =
        std::function<json(const MLInferenceRequest&, MLModelInstance&)>;

    /**
     * @brief Set Inference Dispatch Fn.
     * @param[in] fn Input parameter.
     */
    void setInferenceDispatchFn(InferenceDispatchFn fn);
    
    std::string inferAsync(
        const MLInferenceRequest& request,
        std::function<void(const MLInferenceResponse&)> callback
    );
    
    /**
     * @brief Cancel Inference.
     * @param[in] request_id Identifier of the request.
     * @return True when the operation succeeds.
     */
    bool cancelInference(const std::string& request_id);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Instance Management ═══════════════════════════════════════════════════════════
     * @param[in] model_id Identifier of the model.
     * @param[in] num_instances Input parameter.
     * @return Return value.
     */
    
    Result<bool> scaleModel(const std::string& model_id, size_t num_instances);
    
    /**
     * @brief Health Check.
     * @param[in] instance_id Identifier of the instance.
     * @return True when the operation succeeds.
     */
    bool healthCheck(const std::string& instance_id);
    
    /**
     * @brief Restart Instance.
     * @param[in] instance_id Identifier of the instance.
     * @return Return value.
     */
    Result<bool> restartInstance(const std::string& instance_id);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ System Management ═══════════════════════════════════════════════════════════
     */
    
    void start();
    
    /**
     * @brief Shutdown.
     */
    void shutdown();
    
    /**
     * @brief Get System Stats.
     * @return Return value.
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
    
    // LOCK HIERARCHY ENFORCEMENT (§3.2):
    // ┌─ model_lifecycle_lock_ : std::mutex
    // │  └─ model_cache_lock_ : std::shared_mutex (for cache reads)
    // │     └─ metrics_lock_ : std::mutex
    // └─ models_mutex_ : std::mutex  (flat lock used by non-lifecycle accessors)
    // └─ dispatch_fn_mutex_ : std::mutex (independent)
    // └─ cancel_mutex_ : std::mutex (independent)
    
    mutable std::mutex model_lifecycle_lock_;
    
    mutable std::shared_mutex model_cache_lock_;
    
    mutable std::mutex metrics_lock_;

    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    // models_mutex_ guards all accessors that read or modify models_ outside the
    // lifecycle/cache-lock hierarchy (updateModel, retireModel, unregisterModel,
    // listModels, getModelConfig, getModelStatus, listModelInstances, getModelMetrics,
    // scaleModel, healthCheck, restartInstance, shutdown, getSystemStats, selectInstance,
    // shutdownInstance, healthMonitorLoop, autoScalerLoop).
    mutable std::mutex models_mutex_;
    
    // Background threads
    std::unique_ptr<std::thread> health_monitor_thread_;
    std::unique_ptr<std::thread> auto_scaler_thread_;
    
    std::atomic<bool> running_{false};
    
    // Statistics (protected by metrics_lock_; may use std::memory_order_relaxed for increment-only ops)
    std::atomic<size_t> total_requests_{0};
    std::atomic<size_t> successful_requests_{0};
    std::atomic<size_t> failed_requests_{0};
    
    // Internal methods
    /**
     * @brief Health Monitor Loop.
     */
    void healthMonitorLoop();
    /**
     * @brief Auto Scaler Loop.
     */
    void autoScalerLoop();
    
    /**
     * @brief Deploy Instance.
     * @param[in] model_id Identifier of the model.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<std::string> deployInstance(const std::string& model_id, const MLModelConfig& config);
    /**
     * @brief Shutdown Instance.
     * @param[in] instance_id Identifier of the instance.
     * @return True when the operation succeeds.
     */
    bool shutdownInstance(const std::string& instance_id);
    
    /**
     * @brief Select Instance.
     * @param[in] model_id Identifier of the model.
     * @return Pointer to the result.
     */
    MLModelInstance* selectInstance(const std::string& model_id);
    [[nodiscard]] MLModelInstance* selectLeastBusy_(const ModelEntry& entry) const noexcept;
    /**
     * @brief Update Instance Metrics.
     * @param[in,out] instance Input/output parameter.
     * @param[in] latency_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void updateInstanceMetrics(MLModelInstance* instance, float latency_ms, bool success);
    
    /**
     * @brief Generate Instance Id.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::string generateInstanceId(const std::string& model_id);
    std::atomic<uint64_t> instance_counter_{0};
    
    /**
     * @brief Generate Request Id.
     * @return Return value.
     */
    std::string generateRequestId();
    std::atomic<uint64_t> request_counter_{0};

    // In-flight request cancellation tracking (protected by cancel_mutex_)
    std::unordered_set<std::string> cancelled_requests_;
    std::mutex cancel_mutex_;

    // Injection slot for real inference dispatch (stub #250) (protected by dispatch_fn_mutex_)
    InferenceDispatchFn inference_dispatch_fn_;
    mutable std::mutex dispatch_fn_mutex_;
};

} // namespace llm
} // namespace themis

