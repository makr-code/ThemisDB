/**
 * @file ml_model_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=11, H=48, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/ml_model_manager.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_interface.h"
#include "llm/model_loader.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <random>
#include <thread>
#include <chrono>
#include <shared_mutex>
#include <condition_variable>

namespace themis {
namespace llm {

// Make Result/Ok/Err helpers from themis namespace visible here
using themis::Result;
using themis::Ok;
using themis::Err;
using themis::OkVoid;
using themis::ErrVoid;

MLModelManager::MLModelManager(const Config& config)
    : config_(config)
    , running_(false) {
    THEMIS_INFO("MLModelManager initialized");
}

MLModelManager::~MLModelManager() noexcept {
    // B1-EXCEPTION-SAFETY(2026-08-26): destructor must be noexcept; shutdown()
    // may throw (e.g. health-monitor thread join or RocksDB flush); swallow all
    // exceptions to avoid std::terminate in destructors.
    try {
        shutdown();
    } catch (...) {
        // Swallow — cannot safely propagate from destructor.
    }
}

// ═══════════════════════════════════════════════════════════
// Model Lifecycle Management
// ═══════════════════════════════════════════════════════════

Result<bool> MLModelManager::registerModel(const MLModelConfig& config) {
    // LOCK HIERARCHY: model_lifecycle_lock_ (exclusive for state changes)
    std::lock_guard<std::mutex> lifecycle_lock(model_lifecycle_lock_);
    
    // Check with read lock on cache
    {
        std::shared_lock<std::shared_mutex> cache_lock(model_cache_lock_);
        if (models_.find(config.model_id) != models_.end()) {
            return themis::Err<bool>(
                themis::errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
                "Model already registered: " + config.model_id
            );
        }
    }
    
    auto entry = std::make_unique<ModelEntry>();
    entry->config = config;
    entry->status = MLModelStatus::REGISTERED;
    entry->registered_at = std::chrono::system_clock::now();
    
    // Write to cache with exclusive lock
    {
        std::unique_lock<std::shared_mutex> cache_lock(model_cache_lock_);
        models_[config.model_id] = std::move(entry);
    }
    
    THEMIS_INFO("Registered model: " + config.model_id + " (type: " + std::to_string(static_cast<int>(config.type)) + ")");
    
    return Ok(true);
}

Result<std::vector<std::string>> MLModelManager::deployModel(
    const std::string& model_id,
    size_t num_instances
) {
    // LOCK HIERARCHY: model_lifecycle_lock_ (exclusive for state transitions)
    std::lock_guard<std::mutex> lifecycle_lock(model_lifecycle_lock_);
    
    std::unique_lock<std::shared_mutex> cache_lock(model_cache_lock_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<std::vector<std::string>>(
            themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Model not found: " + model_id
        );
    }
    
    auto& entry = it->second;
    if (entry->status == MLModelStatus::DEPLOYED) {
        return themis::Err<std::vector<std::string>>(
            themis::errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
            "Model already deployed: " + model_id
        );
    }
    
    entry->status = MLModelStatus::DEPLOYING;
    std::vector<std::string> instance_ids;
    
    // B1-EXCEPTION-SAFETY(2026-08-26): wrap the instance deployment loop so that
    // any unexpected exception (not just error-Result) rolls back status to FAILED
    // and propagates.  Without this, an exception mid-loop leaves status=DEPLOYING
    // permanently, which the health-monitor never recovers from.
    try {
        for (size_t i = 0; i < num_instances; ++i) {
            auto result = deployInstance(model_id, entry->config);
            if (!result.has_value()) {
                THEMIS_ERROR("Failed to deploy instance " + std::to_string(i) + " for model " + model_id + ": " + result.error().message());
                // Rollback: shutdown already deployed instances
                for (const auto& inst_id : instance_ids) {
                    shutdownInstance(inst_id);
                }
                entry->status = MLModelStatus::FAILED;
                return themis::Err<std::vector<std::string>>(
                    themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                    "Deployment failed: " + result.error().message()
                );
            }
            instance_ids.push_back(result.value());
        }
    } catch (...) {
        THEMIS_ERROR("deployModel: unexpected exception during instance deployment for model '" + model_id + "'; rolling back");
        for (const auto& inst_id : instance_ids) {
            try { shutdownInstance(inst_id); } catch (...) {}
        }
        entry->status = MLModelStatus::FAILED;
        throw;
    }
    
    entry->status = MLModelStatus::DEPLOYED;
    entry->deployed_at = std::chrono::system_clock::now();
    
    THEMIS_INFO("Deployed model: " + model_id + " with " + std::to_string(num_instances) + " instances");
    
    return themis::Ok(instance_ids);
}

Result<bool> MLModelManager::updateModel(
    const std::string& model_id,
    const MLModelConfig& new_config
) {
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<bool>(
            themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Model not found: " + model_id
        );
    }
    
    auto& entry = it->second;
    
    // Rolling update: deploy new instances, then shutdown old ones
    entry->status = MLModelStatus::UPDATING;
    
    std::vector<std::unique_ptr<MLModelInstance>> old_instances = std::move(entry->instances);
    entry->instances.clear();
    
    // Deploy new instances
    size_t num_instances = old_instances.size();
    if (num_instances == 0) {
        num_instances = new_config.min_instances;
    }
    
    std::vector<std::string> new_instance_ids;
    // B1-EXCEPTION-SAFETY(2026-08-26): exception mid-deployment would leave
    // entry->instances empty and old_instances moved-away — no recovery possible.
    // Wrap in try/catch to restore old instances and set status before propagating.
    try {
        for (size_t i = 0; i < num_instances; ++i) {
            auto result = deployInstance(model_id, new_config);
            if (!result.has_value()) {
                // Rollback
                for (const auto& inst_id : new_instance_ids) {
                    shutdownInstance(inst_id);
                }
                entry->instances = std::move(old_instances);
                entry->status = MLModelStatus::DEPLOYED;
                return themis::Err<bool>(
                    themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                    "Update failed: " + result.error().message()
                );
            }
            new_instance_ids.push_back(result.value());
        }
    } catch (...) {
        // Rollback new instances and restore old ones before propagating.
        for (const auto& inst_id : new_instance_ids) {
            try { shutdownInstance(inst_id); } catch (...) {}
        }
        entry->instances = std::move(old_instances);
        entry->status = MLModelStatus::DEPLOYED;
        throw;
    }
    
    // Shutdown old instances
    for (const auto& old_inst : old_instances) {
        shutdownInstance(old_inst->instance_id);
    }
    
    entry->config = new_config;
    entry->status = MLModelStatus::DEPLOYED;
    
    THEMIS_INFO("Updated model: " + model_id);
    
    return Ok(true);
}

Result<bool> MLModelManager::retireModel(
    const std::string& model_id,
    int drain_timeout_ms
) {
    // Step 1: mark as retired under the lock, then release before sleeping.
    {
        // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = models_.find(model_id);
        if (it == models_.end()) {
            return themis::Err<bool>(
                themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "Model not found: " + model_id
            );
        }
        it->second->status = MLModelStatus::RETIRED;
    }

    // Step 2: poll active_requests outside the lock so other threads can
    // decrement the counter without contending on models_mutex_.
    auto start   = std::chrono::steady_clock::now();
    bool drained = false;

    while (true) {
        {
            std::lock_guard<std::mutex> lock(models_mutex_);
            auto it = models_.find(model_id);
            if (it == models_.end()) {
                drained = true;  // model was removed; treat as fully drained
                break;
            }
            size_t active = 0;
            for (const auto& inst : it->second->instances) {
                active += inst->active_requests;
            }
            if (active == 0) {
                drained = true;
                break;
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        if (elapsed >= drain_timeout_ms) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    THEMIS_INFO("Retired model: " + model_id + " (drained: " + std::to_string(drained) + ")");
    return Ok(drained);
}

Result<bool> MLModelManager::unregisterModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<bool>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "Model not found: " + model_id
        );
    }
    
    const auto& entry = it->second;
    if (entry->status != MLModelStatus::RETIRED) {
        return themis::Err<bool>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "Model must be retired before unregistering: " + model_id
        );
    }
    
    // Shutdown all instances
    for (const auto& inst : entry->instances) {
        shutdownInstance(inst->instance_id);
    }
    
    models_.erase(it);
    
    THEMIS_INFO("Unregistered model: " + model_id);
    
    return Ok(true);
}

// ═══════════════════════════════════════════════════════════
// Model Query and Discovery
// ═══════════════════════════════════════════════════════════

std::vector<std::string> MLModelManager::listModels(const json& filter) const {
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    std::vector<std::string> result;
    
    for (const auto& [model_id, entry] : models_) {
        bool matches = true;
        
        if (filter.contains("type")) {
            int type_filter = filter["type"];
            if (static_cast<int>(entry->config.type) != type_filter) {
                matches = false;
            }
        }
        
        if (filter.contains("status")) {
            int status_filter = filter["status"];
            if (static_cast<int>(entry->status) != status_filter) {
                matches = false;
            }
        }
        
        if (matches) {
            result.push_back(model_id);
        }
    }
    
    return result;
}

Result<MLModelConfig> MLModelManager::getModelConfig(const std::string& model_id) const {
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<MLModelConfig>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "Model not found: " + model_id
        );
    }
    
    return Ok(it->second->config);
}

Result<MLModelStatus> MLModelManager::getModelStatus(const std::string& model_id) const {
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<MLModelStatus>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "Model not found: " + model_id
        );
    }
    
    return Ok(it->second->status);
}

std::vector<MLModelInstance> MLModelManager::listModelInstances(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    std::vector<MLModelInstance> result;
    
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        for (const auto& inst : it->second->instances) {
            result.push_back(*inst);
        }
    }
    
    return result;
}

json MLModelManager::getModelMetrics(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return json{{"error", "Model not found"}};
    }
    
    const auto& entry = it->second;
    
    json metrics;
    metrics["model_id"] = model_id;
    metrics["status"] = static_cast<int>(entry->status);
    metrics["num_instances"] = entry->instances.size();
    
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    size_t active_requests = 0;
    float total_latency = 0.0f;
    
    json instances = json::array();
    for (const auto& inst : entry->instances) {
        instances.push_back(inst->toJSON());
        total_requests += inst->total_requests;
        successful_requests += inst->successful_requests;
        failed_requests += inst->failed_requests;
        active_requests += inst->active_requests;
        total_latency += inst->avg_latency_ms * inst->total_requests;
    }
    
    metrics["instances"] = instances;
    metrics["total_requests"] = total_requests;
    metrics["successful_requests"] = successful_requests;
    metrics["failed_requests"] = failed_requests;
    metrics["active_requests"] = active_requests;
    metrics["success_rate"] = total_requests > 0 ? static_cast<float>(successful_requests) / static_cast<float>(total_requests) : 0.0f;
    metrics["avg_latency_ms"] = total_requests > 0 ? total_latency / total_requests : 0.0f;
    
    return metrics;
}

// ═══════════════════════════════════════════════════════════
// Inference Operations
// ═══════════════════════════════════════════════════════════

Result<MLInferenceResponse> MLModelManager::infer(const MLInferenceRequest& request) {
    auto start = std::chrono::steady_clock::now();
    
    // Select instance
    MLModelInstance* instance = selectInstance(request.model_id);
    if (!instance) {
        MLInferenceResponse response;
        response.success = false;
        response.error_message = "No available instance for model: " + request.model_id;
        return Ok(response);
    }
    
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    // active_requests is std::atomic<size_t>; fetch_add is sequentially consistent
    // by default, ensuring the in-flight counter is always consistent across threads.
    instance->active_requests.fetch_add(1, std::memory_order_relaxed);
    
    auto queue_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();
    
    MLInferenceResponse response;
    response.model_id = request.model_id;
    response.instance_id = instance->instance_id;
    response.queue_time_ms = static_cast<float>(queue_time);
    
    auto infer_start = std::chrono::steady_clock::now();

    InferenceDispatchFn dispatch_fn;
    {
        std::lock_guard<std::mutex> lock(dispatch_fn_mutex_);
        dispatch_fn = inference_dispatch_fn_;
    }

    if (dispatch_fn) {
        try {
            response.output_data = dispatch_fn(request, *instance);
            response.success = true;
        } catch (const std::exception& ex) {
            response.success = false;
            response.error_message = std::string("Inference dispatch error: ") + ex.what();
        }
    } else if (config_.inference_engine) {

        // Route via the configured InferenceEngineEnhanced.
        InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;
        eng_req.base_request.model_id   = request.model_id;
        eng_req.base_request.max_tokens =
            request.inference_params.value("max_tokens", 512);
        eng_req.base_request.temperature =
            static_cast<float>(request.inference_params.value("temperature", 0.7));
        if (request.input_data.is_object()) {
            if (request.input_data.contains("prompt")) {
                eng_req.base_request.prompt =
                    request.input_data["prompt"].get<std::string>();
            } else if (request.input_data.contains("text")) {
                eng_req.base_request.prompt =
                    request.input_data["text"].get<std::string>();
            }
        } else if (request.input_data.is_string()) {
            eng_req.base_request.prompt = request.input_data.get<std::string>();
        }
        eng_req.priority           = request.priority;
        eng_req.timeout            = std::chrono::milliseconds(request.timeout_ms);
        eng_req.preferred_model_id = request.model_id;
        try {
            auto handle   = config_.inference_engine->submit(eng_req);
            auto eng_resp = handle.get();
            response.success     = true;
            response.output_data = json{
                {"text",              eng_resp.text},
                {"tokens_generated",  eng_resp.tokens_generated},
                {"tokens_prompt",     eng_resp.tokens_prompt},
                {"tokens_per_second", eng_resp.tokens_per_second}
            };
        } catch (const std::exception& ex) {
            response.success       = false;
            response.error_message = std::string("InferenceEngine error: ") + ex.what();
        }
    } else {
        // Test/offline fallback path: keep infer() functional when no
        // dispatch function and no inference engine are configured.
        response.success = true;
        response.output_data = json{
            {"result", "simulated"},
            {"model_id", request.model_id}
        };
    }

    auto infer_end = std::chrono::steady_clock::now();
    auto inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        infer_end - infer_start
    ).count();
    
    response.inference_time_ms = static_cast<float>(inference_time);
    response.total_time_ms = response.queue_time_ms + response.inference_time_ms;
    
    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    instance->active_requests.fetch_sub(1, std::memory_order_relaxed);
    
    updateInstanceMetrics(instance, response.total_time_ms, response.success);
    
    if (response.success) {
        successful_requests_++;
    } else {
        failed_requests_++;
    }
    total_requests_++;
    
    return Ok(response);
}

void MLModelManager::setInferenceDispatchFn(InferenceDispatchFn fn) {
    std::lock_guard<std::mutex> lock(dispatch_fn_mutex_);
    inference_dispatch_fn_ = std::move(fn);
}

std::string MLModelManager::inferAsync(
    const MLInferenceRequest& request,
    std::function<void(const MLInferenceResponse&)> callback
) {
    std::string request_id = generateRequestId();
    
    // B3-COPY-ELIM(2026-08-26): capture callback by move so the std::function
    // object is moved into the lambda rather than copied (std::function copy can
    // be expensive for closures with captured heap state).
    std::thread([this, request, cb = std::move(callback)]() {
        auto result = this->infer(request);
        if (!result.has_value()) {
            MLInferenceResponse error_response{};
            error_response.success = false;
            error_response.error_message = result.error().message();
            cb(error_response);
            return;
        }

        cb(result.value());
    }).detach();
    
    return request_id;
}

bool MLModelManager::cancelInference(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(cancel_mutex_);
    cancelled_requests_.insert(request_id);
    THEMIS_INFO("Cancellation requested for request: " + request_id);
    return true;
}

// ═══════════════════════════════════════════════════════════
// Instance Management
// ═══════════════════════════════════════════════════════════

Result<bool> MLModelManager::scaleModel(const std::string& model_id, size_t num_instances) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<bool>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "Model not found: " + model_id
        );
    }
    
    auto& entry = it->second;
    size_t current_instances = entry->instances.size();
    
    if (num_instances == current_instances) {
        return Ok(true);
    }
    
    if (num_instances > current_instances) {
        // Scale up
        for (size_t i = current_instances; i < num_instances; ++i) {
            auto result = deployInstance(model_id, entry->config);
            if (!result.has_value()) {
                return themis::Err<bool>(
                    themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                    "Failed to scale up"
                );
            }
        }
    } else {
        // Scale down
        for (size_t i = current_instances; i > num_instances; --i) {
            if (!entry->instances.empty()) {
                auto& last_inst = entry->instances.back();
                shutdownInstance(last_inst->instance_id);
                entry->instances.pop_back();
            }
        }
    }
    
    THEMIS_INFO("Scaled model " + model_id + " from " + std::to_string(current_instances) + 
             " to " + std::to_string(num_instances) + " instances");
    
    return Ok(true);
}

bool MLModelManager::healthCheck(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    for (auto& [model_id, entry] : models_) {
        for (auto& inst : entry->instances) {
            if (inst->instance_id == instance_id) {
                // Perform basic health check
                bool healthy = (inst->status == MLModelStatus::DEPLOYED);
                
                inst->last_health_check = std::chrono::system_clock::now();
                
                if (healthy) {
                    inst->consecutive_health_check_failures = 0;
                    if (inst->status == MLModelStatus::DEGRADED) {
                        inst->status = MLModelStatus::DEPLOYED;
                        THEMIS_INFO("Instance " + instance_id + " recovered");
                    }
                } else {
                    inst->consecutive_health_check_failures++;
                    if (inst->consecutive_health_check_failures >= entry->config.unhealthy_threshold) {
                        inst->status = MLModelStatus::DEGRADED;
                        THEMIS_WARN("Instance " + instance_id + " marked as degraded");
                    }
                }
                
                return healthy;
            }
        }
    }
    
    return false;
}

Result<bool> MLModelManager::restartInstance(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    for (auto& [model_id, entry] : models_) {
        for (size_t i = 0; i < entry->instances.size(); ++i) {
            if (entry->instances[i]->instance_id == instance_id) {
                // Shutdown old instance
                shutdownInstance(instance_id);
                
                // Deploy new instance
                auto result = deployInstance(model_id, entry->config);
                if (!result.has_value()) {
                    return themis::Err<bool>(
                        themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                        "Failed to restart instance: " + result.error().message()
                    );
                }
                
                THEMIS_INFO("Restarted instance: " + instance_id);
                return Ok(true);
            }
        }
    }
    
    return themis::Err<bool>(
        themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
        "Instance not found: " + instance_id
    );
}

// ═══════════════════════════════════════════════════════════
// System Management
// ═══════════════════════════════════════════════════════════

void MLModelManager::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    if (config_.enable_health_monitoring) {
        health_monitor_thread_ = std::make_unique<std::thread>([this]() {
            healthMonitorLoop();
        });
    }
    
    if (config_.enable_auto_scaling) {
        auto_scaler_thread_ = std::make_unique<std::thread>([this]() {
            autoScalerLoop();
        });
    }
    
    THEMIS_INFO("MLModelManager started");
}

void MLModelManager::shutdown() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    // Stop background threads
    if (health_monitor_thread_ && health_monitor_thread_->joinable()) {
        if (!themis::utils::joinThreadWithin(*health_monitor_thread_)) {
            THEMIS_WARN("Health monitor thread did not join within timeout, continuing shutdown");
        }
    }
    
    if (auto_scaler_thread_ && auto_scaler_thread_->joinable()) {
        if (!themis::utils::joinThreadWithin(*auto_scaler_thread_)) {
            THEMIS_WARN("Auto scaler thread did not join within timeout, continuing shutdown");
        }
    }
    
    // Shutdown all models
    std::lock_guard<std::mutex> lock(models_mutex_);
    for (auto& [model_id, entry] : models_) {
        for (auto& inst : entry->instances) {
            shutdownInstance(inst->instance_id);
        }
    }
    
    THEMIS_INFO("MLModelManager shutdown");
}

json MLModelManager::getSystemStats() const {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    json stats;
    stats["total_models"] = models_.size();
    stats["total_requests"] = total_requests_.load(std::memory_order_acquire);
    stats["successful_requests"] = successful_requests_.load(std::memory_order_acquire);
    stats["failed_requests"] = failed_requests_.load(std::memory_order_acquire);
    stats["success_rate"] = total_requests_ > 0 ? 
        static_cast<float>(successful_requests_.load(std::memory_order_acquire)) / static_cast<float>(total_requests_.load(std::memory_order_acquire)) : 0.0f;
    
    size_t total_instances = 0;
    size_t healthy_instances = 0;
    size_t active_requests = 0;
    
    for (const auto& [model_id, entry] : models_) {
        total_instances += entry->instances.size();
        for (const auto& inst : entry->instances) {
            if (inst->status == MLModelStatus::DEPLOYED) {
                healthy_instances++;
            }
            active_requests += inst->active_requests.load(std::memory_order_relaxed);
        }
    }
    
    stats["total_instances"] = total_instances;
    stats["healthy_instances"] = healthy_instances;
    stats["active_requests"] = active_requests;
    
    return stats;
}

// ═══════════════════════════════════════════════════════════
// Internal Methods
// ═══════════════════════════════════════════════════════════

void MLModelManager::healthMonitorLoop() {
    while (running_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.health_check_interval_ms)
        );

        // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
        // Collect instance IDs under models_mutex_, then RELEASE the lock before calling
        // healthCheck().  The previous pattern held models_mutex_ while calling
        // healthCheck() which re-acquires models_mutex_ → deadlock on non-recursive mutex.
        // Fix: snapshot IDs inside the locked block, then do per-instance health checks
        // outside it (each healthCheck() call acquires/releases models_mutex_ on its own).
        std::vector<std::string> instance_ids;
        {
            std::lock_guard<std::mutex> lock(models_mutex_);
            for (auto& [model_id, entry] : models_) {
                if (!entry->config.enable_health_check) {
                    continue;
                }
                for (auto& inst : entry->instances) {
                    instance_ids.push_back(inst->instance_id);
                }
            }
        }

        for (const auto& id : instance_ids) {
            healthCheck(id);
        }
    }
}

void MLModelManager::autoScalerLoop() {
    while (running_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.scaling_check_interval_ms)
        );
        
        std::lock_guard<std::mutex> lock(models_mutex_);
        
        for (auto& [model_id, entry] : models_) {
            if (!entry->config.enable_auto_scaling) {
                continue;
            }
            
            // Calculate average utilization
            float total_utilization = 0.0f;
            for (const auto& inst : entry->instances) {
                float utilization = static_cast<float>(inst->active_requests.load(std::memory_order_relaxed)) / 
                                   entry->config.max_concurrent_requests;
                total_utilization += utilization;
            }
            
            float avg_utilization = entry->instances.empty() ? 0.0f : 
                                   total_utilization / entry->instances.size();
            
            // Scale decision
            size_t current_instances = entry->instances.size();
            size_t target_instances = current_instances;
            
            if (avg_utilization > entry->config.scale_up_threshold && 
                current_instances < entry->config.max_instances) {
                target_instances = current_instances + 1;
                THEMIS_INFO("Auto-scaling up model " + model_id + " (utilization: " + 
                        std::to_string(avg_utilization) + ")");
            } else if (avg_utilization < entry->config.scale_down_threshold && 
                      current_instances > entry->config.min_instances) {
                target_instances = current_instances - 1;
                THEMIS_INFO("Auto-scaling down model " + model_id + " (utilization: " + 
                        std::to_string(avg_utilization) + ")");
            }
            
            if (target_instances != current_instances) {
                // Note: We can't call scaleModel here because it would deadlock
                // Instead, do the scaling inline
                if (target_instances > current_instances) {
                    auto result = deployInstance(model_id, entry->config);
                    if (!result.has_value()) {
                        THEMIS_ERROR("Auto-scale up failed: " + result.error().message());
                    }
                } else if (target_instances < current_instances && !entry->instances.empty()) {
                    auto& last_inst = entry->instances.back();
                    shutdownInstance(last_inst->instance_id);
                    entry->instances.pop_back();
                }
            }
        }
    }
}

Result<std::string> MLModelManager::deployInstance(
    const std::string& model_id,
    const MLModelConfig& config
) {
    // Find the model entry (assumes caller holds lock)
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return themis::Err<std::string>(
            themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Model not found"
        );
    }
    
    auto& entry = it->second;
    
    auto instance = std::make_unique<MLModelInstance>();
    instance->instance_id = generateInstanceId(model_id);
    instance->model_id = model_id;
    instance->status = MLModelStatus::DEPLOYED;
    instance->gpu_device_id = config.gpu_device_id;
    instance->deployed_at = std::chrono::system_clock::now();
    instance->last_health_check = std::chrono::system_clock::now();
    
    // Use LazyModelLoader when available and a file path is configured.
    if (config_.model_loader && !config.file_path.empty()) {
        json load_cfg = config.inference_config;
        if (config.gpu_device_id >= 0) {
            load_cfg["n_gpu_layers"] = 99;
        }
        CachedModel* cached = config_.model_loader->getOrLoadModel(
            model_id, config.file_path, load_cfg);
        if (!cached) {
            return themis::Err<std::string>(
                themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "LazyModelLoader failed to load model: " + config.file_path
            );
        }
        THEMIS_INFO("Model loaded via LazyModelLoader: " + model_id);
    } else if (!config.file_path.empty()) {
        THEMIS_INFO("No model_loader configured; skipping file load for: " + config.file_path);
    }

    std::string instance_id = instance->instance_id;
    entry->instances.push_back(std::move(instance));
    
    return Ok(instance_id);
}

bool MLModelManager::shutdownInstance(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(models_mutex_);

    for (auto& [model_id, entry] : models_) {
        auto& instances = entry->instances;
        auto it = std::find_if(instances.begin(), instances.end(),
            [&instance_id](const std::unique_ptr<MLModelInstance>& inst) {
                return inst->instance_id == instance_id;
            });

        if (it != instances.end()) {
            (*it)->status = MLModelStatus::RETIRED;
            instances.erase(it);
            // Only unload model weights from LazyModelLoader when this is the
            // last instance; other active instances still need the loaded model.
            if (config_.model_loader && instances.empty()) {
                config_.model_loader->unloadModel(model_id);
            }
            THEMIS_INFO("Shutdown instance: " + instance_id + " (model: " + model_id + ")"
                        + (instances.empty() ? ", model weights unloaded" : ""));
            return true;
        }
    }

    THEMIS_WARN("shutdownInstance: instance not found: " + instance_id);
    return false;
}

MLModelInstance* MLModelManager::selectLeastBusy_(const ModelEntry& entry) const noexcept {
    MLModelInstance* selected = nullptr;
    size_t min_active = SIZE_MAX;
    for (const auto& inst : entry.instances) {
        if (inst->status == MLModelStatus::DEPLOYED &&
            inst->active_requests < min_active) {
            selected = inst.get();
            min_active = inst->active_requests;
        }
    }
    return selected;
    // Future enhancement: replace with Weighted-Round-Robin or P2C (Power-of-Two-Choices)
    // once per-instance capacity weights are available (FUTURE_ENHANCEMENTS.md §"Load Balancing").
}

MLModelInstance* MLModelManager::selectInstance(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(models_mutex_);

    auto it = models_.find(model_id);
    if (it == models_.end() || it->second->instances.empty()) {
        return nullptr;
    }

    return selectLeastBusy_(*it->second);
}

void MLModelManager::updateInstanceMetrics(
    MLModelInstance* instance,
    float latency_ms,
    bool success
) {
    if (!instance) return;

    // Wave-B L7: thread-safety audit — added std::atomic/mutex for concurrent access
    // metrics_lock_ guards per-instance mutable statistics (total_requests,
    // successful_requests, failed_requests, avg_latency_ms, latency_window, p95/p99)
    // that are written here from the infer() caller thread and read concurrently by
    // getModelMetrics() / listModelInstances() under models_mutex_.
    std::lock_guard<std::mutex> lock(metrics_lock_);
    
    instance->total_requests++;
    instance->last_request_at = std::chrono::system_clock::now();
    
    if (success) {
        instance->successful_requests++;
    } else {
        instance->failed_requests++;
    }
    
    // Update rolling average latency (exponential moving average)
    float alpha = 0.1f;
    instance->avg_latency_ms = instance->avg_latency_ms * (1.0f - alpha) + latency_ms * alpha;

    // Update p95/p99 from a fixed-size sliding window of recent latency samples
    instance->latency_window.push_back(latency_ms);
    if (instance->latency_window.size() > MLModelInstance::kLatencyWindowSize) {
        instance->latency_window.pop_front();  // O(1) with deque
    }
    if (instance->latency_window.size() >= 2) {
        std::vector<float> sorted(instance->latency_window.begin(),
                                  instance->latency_window.end());
        std::sort(sorted.begin(), sorted.end());
        size_t n = sorted.size();
        // Use std::min to guard against out-of-bounds on tiny windows
        size_t idx95 = std::min(static_cast<size_t>(std::ceil(0.95 * n)) - 1, n - 1);
        size_t idx99 = std::min(static_cast<size_t>(std::ceil(0.99 * n)) - 1, n - 1);
        instance->p95_latency_ms = sorted[idx95];
        instance->p99_latency_ms = sorted[idx99];
    }
}

std::string MLModelManager::generateInstanceId(const std::string& model_id) {
    return model_id + "-inst-" + std::to_string(instance_counter_++);
}

std::string MLModelManager::generateRequestId() {
    return "req-" + std::to_string(request_counter_++);
}

} // namespace llm
} // namespace themis

