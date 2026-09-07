/**
 * @file subagent_factory_impl.cpp
 * @brief Implementation of SubagentFactory â€” creates and manages independent
 *        LLM Inferencing Subagents with isolated configuration.
 *
 * @note **Production-Grade Implementation**: Thread-safe factory with resource
 *       allocation, configuration validation, and lifecycle management.
 */

#include "llm/subagent_factory.h"
#include "llm/subagent.h"
#include "llm/async_inference_engine.h"
#include "utils/expected.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace themis {
namespace llm {

// ============================================================================
// Â§ 1  Subagent Implementation
// ============================================================================

/**
 * @brief Internal Subagent implementation.
 */
class SubagentImpl : public Subagent {
public:
    SubagentImpl(
        const SubagentConfig& config,
        std::shared_ptr<ILLMPlugin> plugin,
        std::shared_ptr<AsyncInferenceEngine> engine,
        std::shared_ptr<TokenQuotaManager> quota_mgr,
        std::shared_ptr<PromptPolicy> prompt_policy)
        : config_(config)
        , plugin_(plugin)
        , engine_(engine)
        , quota_mgr_(quota_mgr)
        , prompt_policy_(prompt_policy)
        , state_(SubagentState::CREATED)
        , last_error_()
        , metrics_() {
        metrics_.load_time = std::chrono::steady_clock::now();
    }

    ~SubagentImpl() override = default;

    // ========================================================================
    // Identity and Configuration
    // ========================================================================

    const std::string& id() const override {
        return config_.id;
    }

    const SubagentConfig& config() const override {
        return config_;
    }

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    SubagentState getState() const override {
        std::shared_lock<std::shared_mutex> lock(state_mutex_);
        return state_;
    }

    SubagentResult<void> load(int timeout_ms) override {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);

        if (state_ != SubagentState::CREATED) {
            return tl::make_unexpected(
                "subagent_load_state_invalid: cannot load from state " +
                std::string(subagentStateToString(state_)));
        }

        state_ = SubagentState::LOADING;
        lock.unlock();

        if (timeout_ms <= 0) {
            lock.lock();
            state_ = SubagentState::ERROR;
            last_error_ = "subagent_load_timeout_invalid: timeout_ms must be > 0";
            return tl::make_unexpected(last_error_);
        }

        if (!plugin_) {
            lock.lock();
            state_ = SubagentState::ERROR;
            last_error_ = "subagent_load_plugin_missing: llm plugin is null";
            return tl::make_unexpected(last_error_);
        }

        if (!plugin_->loadModel(config_.model_id)) {
            lock.lock();
            state_ = SubagentState::ERROR;
            last_error_ = "subagent_load_model_failed: failed to load model '" + config_.model_id + "'";
            return tl::make_unexpected(last_error_);
        }

        if (!config_.lora_adapter_id.empty()) {
            const bool lora_loaded =
                plugin_->loadLoRA(config_.lora_adapter_id, config_.lora_adapter_id, 1.0f);
            if (!lora_loaded) {
                plugin_->unloadModel();
                lock.lock();
                state_ = SubagentState::ERROR;
                last_error_ = "subagent_load_lora_failed: failed to load adapter '" +
                              config_.lora_adapter_id + "'";
                return tl::make_unexpected(last_error_);
            }
        }

        lock.lock();
        state_ = SubagentState::READY;
        metrics_.load_time = std::chrono::steady_clock::now();
        last_error_.clear();
        return make_expected();
    }

    SubagentResult<void> warm(int timeout_ms) override {
        std::shared_lock<std::shared_mutex> lock(state_mutex_);

        if (state_ != SubagentState::READY) {
            return tl::make_unexpected(std::string("Cannot warm: subagent not in READY state"));
        }
        lock.unlock();

        if (!engine_ || !plugin_) {
            std::unique_lock<std::shared_mutex> write_lock(state_mutex_);
            state_ = SubagentState::ERROR;
            last_error_ = "subagent_warm_runtime_missing: inference runtime is unavailable";
            return tl::make_unexpected(last_error_);
        }

        if (!plugin_->isModelLoaded()) {
            std::unique_lock<std::shared_mutex> write_lock(state_mutex_);
            state_ = SubagentState::ERROR;
            last_error_ = "subagent_warm_model_not_loaded: model is not loaded";
            return tl::make_unexpected(last_error_);
        }

        InferenceRequest warm_request;
        warm_request.model_id = config_.model_id;
        warm_request.prompt = "subagent_warmup";
        warm_request.max_tokens = std::max(1, std::min(8, config_.budget.max_tokens_per_request));

        try {
            const auto timeout = timeout_ms > 0
                                     ? std::chrono::milliseconds(timeout_ms)
                                     : std::chrono::milliseconds(0);
            auto handle = engine_->submit(warm_request, config_.budget.priority, timeout);
            auto response = handle.get();
            if (!response.success) {
                std::unique_lock<std::shared_mutex> write_lock(state_mutex_);
                state_ = SubagentState::ERROR;
                last_error_ = "subagent_warm_inference_failed: " + response.error_message;
                return tl::make_unexpected(last_error_);
            }
        } catch (const std::exception& ex) {
            std::unique_lock<std::shared_mutex> write_lock(state_mutex_);
            state_ = SubagentState::ERROR;
            last_error_ = std::string("subagent_warm_exception: ") + ex.what();
            return tl::make_unexpected(last_error_);
        }

        return make_expected();
    }

    SubagentResult<void> unload(int timeout_ms) override {
        (void)timeout_ms;
        std::unique_lock<std::shared_mutex> lock(state_mutex_);

        if (state_ == SubagentState::TERMINATED) {
            return make_expected();  // Idempotent
        }

        state_ = SubagentState::UNLOADING;
        lock.unlock();

        if (engine_) {
            engine_->shutdown();
        }

        if (plugin_) {
            if (!config_.lora_adapter_id.empty()) {
                (void)plugin_->unloadLoRA(config_.lora_adapter_id);
            }
            plugin_->unloadModel();
        }

        lock.lock();
        state_ = SubagentState::TERMINATED;
        last_error_.clear();
        return make_expected();
    }

    // ========================================================================
    // Inference Operations
    // ========================================================================

    SubagentInferenceResult infer(
        const InferenceRequest& request,
        const std::optional<LLMCorrelationContext>& ctx) override {
        auto start = std::chrono::steady_clock::now();

        SubagentInferenceResult result;
        result.trace_id = ctx ? ctx->trace_id : "";

        {
            std::shared_lock<std::shared_mutex> lock(state_mutex_);
            if (state_ != SubagentState::READY) {
                result.success = false;
                result.error = "subagent_infer_state_not_ready: state=" +
                               std::string(subagentStateToString(state_));
                return result;
            }
        }

        // Check quota
        const auto estimated_tokens = static_cast<size_t>(std::max(
            1, std::min(
                request.max_tokens > 0 ? request.max_tokens : config_.budget.max_tokens_per_request,
                config_.budget.max_tokens_per_request)));
        auto quota_check = quota_mgr_->check(
            config_.tenant_id.empty() ? "default" : config_.tenant_id,
            config_.model_id,
            estimated_tokens);

        if (!quota_check.allowed && config_.policy.block_on_quota_violation) {
            std::unique_lock<std::shared_mutex> lock(state_mutex_);
            metrics_.quota_blocks++;
            result.success = false;
            result.error = "subagent_infer_quota_exceeded: " + quota_check.reason;
            metrics_.total_requests++;
            metrics_.failed_inferences++;
            last_error_ = result.error;
            return result;
        }

        // Check policy
        if (prompt_policy_) {
            auto policy_result = prompt_policy_->apply(request.prompt);
            if (!policy_result.allowed && config_.policy.block_on_policy_violation) {
                std::unique_lock<std::shared_mutex> lock(state_mutex_);
                metrics_.policy_blocks++;
                result.success = false;
                result.error = "subagent_infer_policy_blocked: " + policy_result.reason;
                metrics_.total_requests++;
                metrics_.failed_inferences++;
                last_error_ = result.error;
                return result;
            }
        }

        if (!engine_) {
            result.success = false;
            result.error = "subagent_infer_engine_missing: async inference engine is null";
            std::unique_lock<std::shared_mutex> lock(state_mutex_);
            metrics_.total_requests++;
            metrics_.failed_inferences++;
            last_error_ = result.error;
            return result;
        }

        try {
            InferenceRequest runtime_request = request;
            runtime_request.model_id = config_.model_id;
            if (!config_.lora_adapter_id.empty() && !runtime_request.lora_adapter_id.has_value()) {
                runtime_request.lora_adapter_id = config_.lora_adapter_id;
            }

            if (runtime_request.max_tokens <= 0) {
                runtime_request.max_tokens = config_.budget.max_tokens_per_request;
            } else if (runtime_request.max_tokens > config_.budget.max_tokens_per_request) {
                runtime_request.max_tokens = config_.budget.max_tokens_per_request;
            }

            const auto timeout =
                config_.budget.timeout_ms > 0 ? std::chrono::milliseconds(config_.budget.timeout_ms)
                                              : std::chrono::milliseconds(0);
            auto handle = engine_->submit(runtime_request, config_.budget.priority, timeout);
            auto response = handle.get();

            result.success = response.success;
            result.output = response.text;
            result.error = response.success ? std::string() :
                ("subagent_infer_runtime_failed: " + response.error_message);
            result.tokens_consumed = static_cast<size_t>(
                std::max(response.tokens_generated, response.tokens_prompt));

            if (result.tokens_consumed == 0 && response.success) {
                result.tokens_consumed = static_cast<size_t>(
                    std::max(1, std::min(runtime_request.max_tokens, config_.budget.max_tokens_per_request)));
            }

            {
                std::unique_lock<std::shared_mutex> lock(state_mutex_);
                metrics_.total_requests++;
                if (result.success) {
                    metrics_.successful_inferences++;
                    metrics_.tokens_consumed += result.tokens_consumed;
                    metrics_.total_tokens_processed += result.tokens_consumed;
                    last_error_.clear();
                } else {
                    metrics_.failed_inferences++;
                    last_error_ = result.error;
                }
                metrics_.last_request_time = std::chrono::steady_clock::now();
            }

            if (result.success && result.tokens_consumed > 0) {
                quota_mgr_->consume(
                    config_.tenant_id.empty() ? "default" : config_.tenant_id,
                    config_.model_id,
                    result.tokens_consumed);
            }
        } catch (const std::exception& ex) {
            result.success = false;
            result.error = std::string("subagent_infer_exception: ") + ex.what();
            {
                std::unique_lock<std::shared_mutex> lock(state_mutex_);
                metrics_.total_requests++;
                metrics_.failed_inferences++;
                last_error_ = result.error;
            }
        }

        auto end = std::chrono::steady_clock::now();
        result.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        return result;
    }

    std::future<SubagentInferenceResult> inferAsync(
        const InferenceRequest& request,
        const std::optional<LLMCorrelationContext>& ctx) override {
        return std::async(std::launch::async, [this, request, ctx]() {
            return this->infer(request, ctx);
        });
    }

    SubagentInferenceResult inferStream(
        const InferenceRequest& request,
        std::function<void(const std::string&)> on_token,
        const std::optional<LLMCorrelationContext>& ctx) override {
        // Simplified: just call infer() for now
        auto result = infer(request, ctx);
        if (result.success && on_token) {
            on_token(result.output);
        }
        return result;
    }

    std::vector<SubagentInferenceResult> inferBatch(
        const std::vector<InferenceRequest>& requests,
        const std::optional<LLMCorrelationContext>& ctx) override {
        std::vector<SubagentInferenceResult> results = {};

        for (const auto& req : requests) {
            results.push_back(infer(req, ctx));
        }
        return results;
    }

    // ========================================================================
    // Observability
    // ========================================================================

    SubagentMetrics getMetrics() const override {
        std::shared_lock<std::shared_mutex> lock(state_mutex_);
        return metrics_;
    }

    void resetMetrics() override {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        metrics_.tokens_consumed = 0;
        metrics_.policy_blocks = 0;
        metrics_.quota_blocks = 0;
    }

    std::string getLastError() const override {
        std::shared_lock<std::shared_mutex> lock(state_mutex_);
        return last_error_;
    }

    // ========================================================================
    // Resource Management
    // ========================================================================

    bool isReady() const override {
        std::shared_lock<std::shared_mutex> lock(state_mutex_);
        return state_ == SubagentState::READY;
    }

    SubagentResult<void> pause() override {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        if (state_ != SubagentState::READY) {
            return tl::make_unexpected(std::string("Cannot pause: subagent not in READY state"));
        }
        state_ = SubagentState::PAUSED;
        return make_expected();
    }

    SubagentResult<void> resume() override {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        if (state_ != SubagentState::PAUSED) {
            return tl::make_unexpected(std::string("Cannot resume: subagent not in PAUSED state"));
        }
        state_ = SubagentState::READY;
        return make_expected();
    }

    QuotaCheckResult checkQuota(size_t estimated_tokens) const override {
        return quota_mgr_->check(
            config_.tenant_id.empty() ? "default" : config_.tenant_id,
            config_.model_id,
            estimated_tokens);
    }

    QuotaCheckResult consumeQuota(size_t tokens) override {
        quota_mgr_->consume(
            config_.tenant_id.empty() ? "default" : config_.tenant_id,
            config_.model_id,
            tokens);
        return checkQuota(0);
    }

    void resetQuota() override {
        const std::string tenant = config_.tenant_id.empty() ? "default" : config_.tenant_id;
        const auto current_limit = quota_mgr_->getLimit(tenant, config_.model_id);
        if (!current_limit.has_value()) {
            return;
        }

        quota_mgr_->removeQuota(tenant, config_.model_id);
        quota_mgr_->setQuota(tenant, config_.model_id, current_limit.value());
    }

private:
    SubagentConfig config_;
    std::shared_ptr<ILLMPlugin> plugin_;
    std::shared_ptr<AsyncInferenceEngine> engine_;
    std::shared_ptr<TokenQuotaManager> quota_mgr_;
    std::shared_ptr<PromptPolicy> prompt_policy_;

    mutable std::shared_mutex state_mutex_;
    SubagentState state_;
    std::string last_error_;
    SubagentMetrics metrics_;
};

// ============================================================================
// Â§ 2  SubagentFactory Implementation
// ============================================================================

/** @brief Â§ 2  SubagentFactory Implementation. */
class SubagentFactoryImpl : public SubagentFactory {
public:
    SubagentFactoryImpl(
        ILLMPlugin* plugin,
        std::shared_ptr<SharedWorkerPool> worker_pool,
        std::shared_ptr<ModelLoader> model_loader,
        std::shared_ptr<MultiLoRAManager> lora_manager,
        std::shared_ptr<TokenQuotaManager> quota_manager,
        const Config& config)
        : plugin_(plugin)
        , worker_pool_(worker_pool)
        , model_loader_(model_loader)
        , lora_manager_(lora_manager)
        , quota_manager_(quota_manager ? quota_manager : 
                        std::make_shared<TokenQuotaManager>())
        , config_(config)
        , subagent_counter_(0)
        , stats_() {
        stats_.factory_start_time = std::chrono::steady_clock::now();
    }

    std::vector<SubagentValidationError> validateConfig(
        const SubagentConfig& config) override {
        std::vector<SubagentValidationError> errors;

        // Validate model_id
        if (config.model_id.empty()) {
            errors.push_back({
                "model_id",
                "Model ID is required",
                ""
            });
        }

        // Validate isolation level
        if (static_cast<int>(config.isolation_level) < 0 ||
            static_cast<int>(config.isolation_level) > 3) {
            errors.push_back({
                "isolation_level",
                "Invalid isolation level",
                ""
            });
        }

        // In production: check model existence, adapter compatibility, etc.

        return errors;
    }

    SubagentResult<std::shared_ptr<Subagent>> createSubagent(
        const SubagentConfig& config) override {
        // Validate config
        auto errors = validateConfig(config);
        if (!errors.empty()) {
            std::string msg = "Configuration validation failed: ";
            for (const auto& err : errors) {
                msg += err.field + " (" + err.reason + ") ";
            }
            return tl::make_unexpected(msg);
        }

        // Check max subagents limit and duplicate ID
        {
            std::unique_lock<std::mutex> lock(subagents_mutex_);
            if (config_.max_subagents > 0 && 
                static_cast<int>(subagents_.size()) >= config_.max_subagents) {
                return tl::make_unexpected(std::string("Maximum subagents limit reached"));
            }
            if (subagents_.count(config.id) > 0) {
                return tl::make_unexpected(std::string("Subagent already exists with ID: ") + config.id);
            }
        }

        // Create subagent
        auto engine = std::make_shared<AsyncInferenceEngine>(
            plugin_,
            AsyncInferenceEngine::Config{
                .num_worker_threads = 2,
                .max_queue_size = 1000,
            },
            worker_pool_
        );

        std::shared_ptr<PromptPolicy> prompt_policy = {};

        if (!config.policy.prompt_policy_id.empty()) {
            std::unique_lock<std::mutex> plock(policies_mutex_);
            auto it = policies_.find(config.policy.prompt_policy_id);
            if (it != policies_.end()) {
                prompt_policy = it->second;
            }
        }
        if (!prompt_policy) {
            prompt_policy = std::make_shared<PromptPolicy>();
        }

        auto subagent = std::make_shared<SubagentImpl>(
            config,
            std::shared_ptr<ILLMPlugin>(plugin_, [](void*) {}),  // Non-owning
            engine,
            quota_manager_,
            prompt_policy
        );

        // Register in subagent registry
        {
            std::unique_lock<std::mutex> lock(subagents_mutex_);
            subagents_[config.id] = subagent;
            stats_.total_created++;
            stats_.currently_active++;
        }

        // Set up quota
        quota_manager_->setQuota(
            config.tenant_id.empty() ? "default" : config.tenant_id,
            config.model_id,
            config.budget.max_tokens_per_minute
        );

        return make_expected<std::shared_ptr<Subagent>>(std::static_pointer_cast<Subagent>(subagent));
    }

    SubagentResult<void> destroySubagent(
        const std::string& subagent_id,
        int timeout_ms) override {
        std::shared_ptr<Subagent> subagent;

        {
            std::unique_lock<std::mutex> lock(subagents_mutex_);
            auto it = subagents_.find(subagent_id);
            if (it == subagents_.end()) {
                return tl::make_unexpected(std::string("Subagent not found: ") + subagent_id);
            }
            subagent = it->second;
            subagents_.erase(it);
            stats_.total_destroyed++;
            if (stats_.currently_active > 0) {
                stats_.currently_active--;
            }
        }

        // Unload subagent
        auto unload_result = subagent->unload(timeout_ms);
        if (!unload_result) {
            return unload_result;
        }

        return make_expected();
    }

    std::shared_ptr<Subagent> getSubagent(const std::string& subagent_id) override {
        std::unique_lock<std::mutex> lock(subagents_mutex_);
        auto it = subagents_.find(subagent_id);
        return it != subagents_.end() ? it->second : nullptr;
    }

    std::vector<std::string> listSubagents() override {
        std::vector<std::string> ids;
        {
            std::unique_lock<std::mutex> lock(subagents_mutex_);
            for (const auto& [id, _] : subagents_) {
                ids.push_back(id);
            }
        }
        return ids;
    }

    SubagentResult<SubagentMetrics> getSubagentMetrics(
        const std::string& subagent_id) override {
        auto subagent = getSubagent(subagent_id);
        if (!subagent) {
            return tl::make_unexpected("Subagent not found: " + subagent_id);
        }
        return make_expected(subagent->getMetrics());
    }

    SubagentResult<SubagentState> getSubagentState(
        const std::string& subagent_id) override {
        auto subagent = getSubagent(subagent_id);
        if (!subagent) {
            return tl::make_unexpected("Subagent not found: " + subagent_id);
        }
        return make_expected(subagent->getState());
    }

    SubagentResult<void> registerPromptPolicy(
        const std::string& policy_id,
        std::shared_ptr<PromptPolicy> policy) override {
        {
            std::unique_lock<std::mutex> lock(policies_mutex_);
            policies_[policy_id] = policy;
        }
        return make_expected();
    }

    SubagentResult<void> unregisterPromptPolicy(const std::string& policy_id) override {
        {
            std::unique_lock<std::mutex> lock(policies_mutex_);
            auto it = policies_.find(policy_id);
            if (it == policies_.end()) {
                return tl::make_unexpected(std::string("Policy not found: ") + policy_id);
            }
            policies_.erase(it);
        }
        return make_expected();
    }

    FactoryStats getFactoryStats() override {
        std::unique_lock<std::mutex> lock(subagents_mutex_);
        return stats_;
    }

private:
    ILLMPlugin* plugin_;
    std::shared_ptr<SharedWorkerPool> worker_pool_;
    std::shared_ptr<ModelLoader> model_loader_;
    std::shared_ptr<MultiLoRAManager> lora_manager_;
    std::shared_ptr<TokenQuotaManager> quota_manager_;
    Config config_;

    std::mutex subagents_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Subagent>> subagents_;
    std::atomic<uint64_t> subagent_counter_;

    std::mutex policies_mutex_;
    std::unordered_map<std::string, std::shared_ptr<PromptPolicy>> policies_;

    FactoryStats stats_;
};

// ============================================================================
// Â§ 3  Factory Creation
// ============================================================================

SubagentResult<std::unique_ptr<SubagentFactory>> SubagentFactory::create(
    ILLMPlugin* plugin,
    std::shared_ptr<SharedWorkerPool> worker_pool,
    std::shared_ptr<ModelLoader> model_loader,
    std::shared_ptr<MultiLoRAManager> lora_manager,
    std::shared_ptr<TokenQuotaManager> quota_manager,
    const Config& config) {
    
    if (!plugin || !worker_pool || !model_loader || !lora_manager) {
        return tl::make_unexpected(std::string("Required dependencies are null"));
    }

    return make_expected<std::unique_ptr<SubagentFactory>>(
        std::make_unique<SubagentFactoryImpl>(
            plugin, worker_pool, model_loader, lora_manager, quota_manager, config
        )
    );
}

} // namespace llm
} // namespace themis
