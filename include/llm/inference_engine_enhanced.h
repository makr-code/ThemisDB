#pragma once

/**
 * @file inference_engine_enhanced.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/adapter_registry.h"
#include "llm/continuous_batch_scheduler.h"
#include "llm/i_federated_inference_backend.h"
#include "llm/inference_handle.h"
#include "llm/llm_prefix_cache.h"
#include "llm/llm_plugin_interface.h"
#include "llm/model_router.h"
#include "llm/paged_block_manager.h"
#include "llm/paged_kv_cache.h"
#include "llm/shared_worker_pool.h"
#include "llm/speculative_decoder.h"
#include "llm/lookup_decoder.h"
#include "rag/self_rag.h"
#include "sharding/remote_executor.h"

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {

class InferenceEngineEnhanced {
public:
    struct Config {
        // Context caching
        bool enable_context_caching = true;
        size_t max_cache_entries = 10000;
        double cache_similarity_threshold = 0.95;
        int cache_ttl_seconds = 7200;  // 2 hours
        
        // Batch processing
        bool enable_batch_processing = true;
        size_t min_batch_size = 1;
        size_t max_batch_size = 256;
        size_t batch_timeout_ms = 100;  // Wait up to 100ms to form batch
        size_t max_tokens_per_batch = 8192;
        bool enable_adaptive_batch_retry = false;
        
        // Request queuing
        size_t max_queue_size = 1000;
        size_t request_timeout_ms = 30000;  // 30 seconds
        bool enable_priority_scheduling = true;
        
        // Load balancing
        bool enable_load_balancing = true;
        enum class LoadBalanceStrategy {
            ROUND_ROBIN,
            LEAST_LOADED,
            RESPONSE_TIME_WEIGHTED
        };
        LoadBalanceStrategy load_balance_strategy = LoadBalanceStrategy::LEAST_LOADED;
        
        // Worker threads
        size_t num_worker_threads = 4;

        // Speculative decoding (Phase 3 — latency reduction)
        // Requires a draft model registered under speculative_draft_model_id.
        // Automatically disabled when grammar constraints are active on a request.
        bool enable_speculative_decoding = false;
        size_t speculative_draft_tokens = 4;
        std::string speculative_draft_model_id;

        std::string speculative_remote_draft_shard_id;

        // Prompt lookup decoding (n-gram based speculation-light path)
        bool enable_lookup_decoding = false;
        size_t lookup_ngram_min = 2;
        size_t lookup_ngram_max = 4;
        size_t lookup_max_draft_tokens = 0;  // 0 = use lookup_ngram_max
    };
    
    struct Statistics {
        // Context caching metrics
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        double cache_hit_rate = 0.0;
        size_t tokens_saved_by_cache = 0;
        
        // Batch processing metrics
        size_t total_batches = 0;
        double avg_batch_size = 0.0;
        size_t max_batch_size_seen = 0;
        double throughput_improvement = 0.0;
        
        // Queue metrics
        size_t total_requests = 0;
        size_t completed_requests = 0;
        size_t timed_out_requests = 0;
        size_t rejected_requests = 0;
        size_t current_queue_size = 0;
        
        // Load balancing metrics
        std::unordered_map<std::string, size_t> requests_per_model;
        std::unordered_map<std::string, double> avg_latency_per_model;
        double load_balance_fairness = 0.0;  // 1.0 = perfectly balanced
        
        // Overall performance
        double avg_latency_ms = 0.0;
        double p95_latency_ms = 0.0;
        double p99_latency_ms = 0.0;
        double tokens_per_second = 0.0;
        size_t total_tokens_generated = 0;

        // Speculative decoding metrics
        size_t speculative_draft_tokens_total = 0;    ///< Total draft tokens proposed.
        size_t speculative_accepted_tokens = 0;       ///< Draft tokens accepted.
        size_t speculative_rejected_tokens = 0;       ///< Draft tokens rejected.
        double speculative_avg_acceptance_rate = 0.0; ///< Running avg acceptance rate (0-1).
        size_t speculative_steps = 0;                 ///< Total verify() calls.
    };
    
    struct ModelResourceQuota {
        size_t max_concurrent_requests = 0;
        size_t max_memory_mb = 0;
    };

    struct ModelInfo {
        std::string model_id;
        std::shared_ptr<ILLMPlugin> plugin;
        size_t active_requests = 0;
        double avg_response_time_ms = 0.0;
        size_t total_requests = 0;
        bool is_available = true;
        ModelResourceQuota quota;  ///< Per-model resource limits (0 = unlimited)
    };
    
    struct EnhancedInferenceRequest {
        InferenceRequest base_request;
        int priority = 0;
        std::chrono::milliseconds timeout{30000};
        bool allow_caching = true;
        std::string preferred_model_id;  // Optional model preference
        // RAID-sharding orchestration hints (optional, see src/llm/FUTURE_ENHANCEMENTS.md):
        // - shard_routing_key: deterministic placement hint for shard routers.
        // - target_instance_ids: explicit shard/instance fan-out subset.
        // - allow_cross_instance_batching: opt-in guard for distributed co-batching.
        std::string shard_routing_key;        // Set alone for deterministic shard placement.
        std::vector<std::string> target_instance_ids; // Optional explicit fan-out subset; empty = router decides.
        bool allow_cross_instance_batching = false;   // Explicit opt-in for coordinator-side co-batching.
        
        // For result tracking
        std::string request_id;
        std::chrono::steady_clock::time_point submitted_at;
    };
    
    /**
     * @brief Inference Engine Enhanced.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit InferenceEngineEnhanced(const Config& config);

    InferenceEngineEnhanced(const Config& config,
                            std::shared_ptr<SharedWorkerPool> pool);

    ~InferenceEngineEnhanced() noexcept;
    
    // Model management
    
    /**
     * @brief Register Model.
     * @param[in] model_id Identifier of the model.
     * @param[in] plugin Input parameter.
     */
    void registerModel(const std::string& model_id, std::shared_ptr<ILLMPlugin> plugin);
    
    /**
     * @brief Unregister Model.
     * @param[in] model_id Identifier of the model.
     */
    void unregisterModel(const std::string& model_id);

    /**
     * @brief Get Available Models.
     * @return Return value.
     */
    std::vector<std::string> getAvailableModels() const;

    /**
     * @brief Set Adapter Registry.
     * @param[in] registry Input parameter.
     */
    void setAdapterRegistry(std::shared_ptr<AdapterRegistry> registry);

    /**
     * @brief Swap Model.
     * @param[in] model_id Identifier of the model.
     * @param[in] new_plugin Input parameter.
     */
    void swapModel(const std::string& model_id, std::shared_ptr<ILLMPlugin> new_plugin);

    void loadLoRAAdapter(const std::string& adapter_id,
                         const std::string& path,
                         float scale = 1.0f,
                         const std::string& model_id = "");

    bool unloadLoRAAdapter(const std::string& adapter_id,
                           const std::string& model_id = "");

    /**
     * @brief Get Loaded Lo RAAdapters.
     * @return Return value.
     */
    std::vector<LoRAInfo> getLoadedLoRAAdapters() const;

    /**
     * @brief Set Model Quota.
     * @param[in] model_id Identifier of the model.
     * @param[in] quota Input parameter.
     */
    void setModelQuota(const std::string& model_id, const ModelResourceQuota& quota);

    /**
     * @brief Get Model Quota.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    ModelResourceQuota getModelQuota(const std::string& model_id) const;

    /**
     * @brief ── Content-based / metadata-tag routing ────────────────────────────────
     * @param[in] rule Input parameter.
     */

    void addRoutingRule(const RoutingRule& rule);

    /**
     * @brief Remove Routing Rule.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool removeRoutingRule(const std::string& rule_id);

    /**
     * @brief Get Routing Rules.
     * @return Return value.
     */
    std::vector<RoutingRule> getRoutingRules() const;

    /**
     * @brief Clear Routing Rules.
     */
    void clearRoutingRules();

    // Inference submission
    /**
     * @brief Submit.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    InferenceHandle submit(const EnhancedInferenceRequest& request);

    std::string submitAsync(
        const EnhancedInferenceRequest& request,
        std::function<void(const InferenceResponse&)> callback
    );

    using TokenCallback = std::function<void(std::string_view token, bool is_final)>;

    /**
     * @brief Submit Streaming.
     * @param[in] request Input parameter.
     * @param[in] callback Input parameter.
     * @return Return value.
     */
    InferenceHandle submitStreaming(
        const EnhancedInferenceRequest& request,
        TokenCallback                   callback
    );
    
    // Request management
    /**
     * @brief Cancel.
     * @param[in] request_id Identifier of the request.
     * @return True when the operation succeeds.
     */
    bool cancel(const std::string& request_id);

    /**
     * @brief Reprioritize.
     * @param[in] request_id Identifier of the request.
     * @param[in] new_priority Input parameter.
     * @return True when the operation succeeds.
     */
    bool reprioritize(const std::string& request_id, int new_priority);
    
    // Cache management
    /**
     * @brief Clear Cache.
     */
    void clearCache();

    /**
     * @brief Prewarm Cache.
     * @param[in] common_prompts Input parameter.
     */
    void prewarmCache(const std::vector<std::string>& common_prompts);
    
    // Statistics and monitoring
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    /**
     * @brief Get Detailed Metrics.
     * @return Return value.
     */
    json getDetailedMetrics() const;
    
    // Lifecycle
    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Shutdown.
     */
    void shutdown();
    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    bool isRunning() const;

    /**
     * @brief Set Remote Executor.
     * @param[in,out] exec Input/output parameter.
     * @param[in] draft_shard Input parameter.
     */
    void setRemoteExecutor(sharding::RemoteExecutor* exec,
                           const sharding::ShardInfo& draft_shard);

    /**
     * @brief Set Federated Backend.
     * @param[in] backend Input parameter.
     */
    void setFederatedBackend(std::shared_ptr<IFederatedInferenceBackend> backend);

    // ── Wave B B1: Self-RAG integration ───────────────────────────────────

    using SelfRAGRetrievalCallback = std::function<
        std::vector<themis::rag::SelfRAGDocument>(
            const std::string&,
            size_t,
            const InferenceRequest&)>;

    using SelfRAGCriticCallback = std::function<
        double(
            const std::string&,
            const themis::rag::SelfRAGDocument&,
            const InferenceRequest&)>;

    /**
     * @brief Set Self RAGRetrieval Callback.
     * @param[in] cb Input parameter.
     */
    void setSelfRAGRetrievalCallback(SelfRAGRetrievalCallback cb);

    /**
     * @brief Set Self RAGCritic Callback.
     * @param[in] cb Input parameter.
     */
    void setSelfRAGCriticCallback(SelfRAGCriticCallback cb);

    // ── STUB #262 bridge — target logit injection ─────────────────────────

    using TargetLogitsFn = std::function<
        std::vector<std::vector<float>>(
            const InferenceRequest&            /*request*/,
            size_t                             /*K*/,
            size_t                             /*vocab_size*/,
            std::shared_ptr<ILLMPlugin>        /*target_plugin*/)>;

    /**
     * @brief Set Target Logits Fn.
     * @param[in] fn Input parameter.
     */
    void setTargetLogitsFn(TargetLogitsFn fn);

    // ── STUB #263 bridge — tokenizer injection ────────────────────────────

    using TokenizerFn = std::function<std::vector<int>(const std::string& text,
                                                       size_t             vocab_size)>;

    /**
     * @brief Set Tokenizer Fn.
     * @param[in] fn Input parameter.
     */
    void setTokenizerFn(TokenizerFn fn);

    /**
     * @brief Clear Tokenizer Fn.
     */
    void clearTokenizerFn();

private:
    Config config_;
    std::atomic<bool> running_{false};

    // Optional shared worker pool (nullptr → private worker threads used)
    std::shared_ptr<SharedWorkerPool> shared_pool_;

    // Core components
    std::unique_ptr<LLMPrefixCache> prefix_cache_;
    mutable std::mutex cache_mutex_;  // Protects prefix_cache_ access
    std::shared_ptr<PagedKVCache> kv_cache_;
    std::shared_ptr<PagedBlockManager> block_manager_;
    std::unique_ptr<ContinuousBatchScheduler> batch_scheduler_;

    // Speculative decoding — one decoder per engine instance.
    // nullptr when enable_speculative_decoding == false.
    std::unique_ptr<SpeculativeDecoder> speculative_decoder_;

    // Optional RemoteExecutor for cross-shard speculative draft dispatch.
    // nullptr when setRemoteExecutor() has not been called.  Not owned.
    sharding::RemoteExecutor* remote_executor_ = nullptr;
    // ShardInfo for the remote draft shard (valid only when remote_executor_ != nullptr).
    sharding::ShardInfo remote_draft_shard_info_;

    // Optional federated backend for cross-instance fan-out (Issue #1928).
    // Protected by federated_backend_mutex_.
    std::shared_ptr<IFederatedInferenceBackend> federated_backend_;
    mutable std::mutex federated_backend_mutex_;
    SelfRAGRetrievalCallback self_rag_retrieval_cb_;
    SelfRAGCriticCallback self_rag_critic_cb_;
    mutable std::mutex self_rag_mutex_;
    // STUB #262 bridge — target logit injection.
    TargetLogitsFn target_logits_fn_;
    mutable std::mutex target_logits_fn_mutex_;
    // STUB #263 bridge — tokenizer injection.
    TokenizerFn tokenizer_fn_;
    mutable std::mutex tokenizer_fn_mutex_;

    // Lookup decoder (n-gram based, draft-model-free).
    // nullptr when enable_lookup_decoding == false.
    std::unique_ptr<LookupDecoder> lookup_decoder_;

    // Optional adapter registry for DRAFT model auto-discovery.
    // When set and speculative_draft_model_id is empty, the engine queries
    // this registry via findDraftAdapterForFamily() to auto-select the draft
    // model based on the target model's architecture/family.
    std::shared_ptr<AdapterRegistry> adapter_registry_;

    // Content-based / metadata-tag model router (Phase 3).
    // Evaluated in selectModel() before load-balancing strategies.
    ModelRouter model_router_;
    
    // Model registry for load balancing
    std::unordered_map<std::string, ModelInfo> models_;
    mutable std::mutex models_mutex_;
    std::atomic<size_t> round_robin_index_{0};

    // LoRA adapter registry for hot-loading
    struct LoRAAdapterEntry {
        std::string path;          ///< Filesystem path to adapter weights
        float       scale = 1.0f; ///< LoRA scaling factor
        std::string model_id;      ///< Pinned model (empty = all models)
    };
    std::unordered_map<std::string, LoRAAdapterEntry> lora_adapters_;
    mutable std::mutex lora_adapters_mutex_;

    // Request tracking
    struct TrackedRequest {
        EnhancedInferenceRequest request;
        std::chrono::steady_clock::time_point deadline;
        std::promise<InferenceResponse> promise;
        std::function<void(const InferenceResponse&)> callback;
        // Shared cancellation token — held by the InferenceHandle so
        // InferenceHandle::cancel() propagates here immediately.
        std::shared_ptr<std::atomic<bool>> cancel_token =
            std::make_shared<std::atomic<bool>>(false);
    };
    std::unordered_map<std::string, std::shared_ptr<TrackedRequest>> tracked_requests_;
    mutable std::mutex requests_mutex_;
    
    // Statistics
    Statistics stats_;
    mutable std::mutex stats_mutex_;
    std::vector<double> latency_samples_;  // For percentile calculation
    std::chrono::steady_clock::time_point engine_start_time_{std::chrono::steady_clock::now()};
    
    // Worker threads for request processing
    std::vector<std::thread> worker_threads_;
    std::queue<std::shared_ptr<TrackedRequest>> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Timeout monitoring thread
    std::thread timeout_thread_;
    
    // Internal methods
    /**
     * @brief Worker Loop.
     * @param[in] worker_id Identifier of the worker.
     */
    void workerLoop(size_t worker_id);
    /**
     * @brief Timeout Monitor Loop.
     */
    void timeoutMonitorLoop();
    /**
     * @brief Process Batch.
     * @param[in] batch Input parameter.
     */
    void processBatch(const std::vector<std::shared_ptr<TrackedRequest>>& batch);

    /**
     * @brief Batch coordinator used when a shared pool is provided.
     * @details Forms batches from the internal queue and submits processBatch() tasks to shared_pool_ rather than executing them inline.
     */
    void batchCoordinatorLoop();
    
    // Cache integration
    /**
     * @brief Check Cache.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::optional<InferenceResponse> checkCache(const InferenceRequest& request);
    /**
     * @brief Update Cache.
     * @param[in] request Input parameter.
     * @param[in] response Input parameter.
     */
    void updateCache(const InferenceRequest& request, const InferenceResponse& response);
    
    // Load balancing
    /**
     * @brief Select Model.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::string selectModel(const EnhancedInferenceRequest& request);
    /**
     * @brief Update Model Stats.
     * @param[in] model_id Identifier of the model.
     * @param[in] latency_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void updateModelStats(const std::string& model_id, double latency_ms, bool success);
    
    // Batch formation
    /**
     * @brief Form Batch.
     * @return Return value.
     */
    std::vector<std::shared_ptr<TrackedRequest>> formBatch();
    /**
     * @brief Can Add To Batch.
     * @param[in] req Input parameter.
     * @param[in] current_batch_tokens Input parameter.
     * @return True when the operation succeeds.
     */
    bool canAddToBatch(const std::shared_ptr<TrackedRequest>& req, size_t current_batch_tokens);
    
    // Timeout handling
    /**
     * @brief Check And Handle Timeouts.
     */
    void checkAndHandleTimeouts();
    
    /**
     * @brief Embedding helper for cache operations.
     * @param[in] text Input parameter.
     * @return Return value.
     * @details Uses the first available plugin (see implementation for selection rationale). Returns an empty vector when no plugin is registered or embedding fails (graceful degradation: falls back to exact-key matching only).
     */
    std::vector<float> computeEmbeddingForCache(const std::string& text);

    /**
     * @brief Build a token-ID sequence for a given prompt.
     * @param[in] text Input parameter.
     * @return Return value.
     * @details Uses the rough heuristic of 4 chars ≈ 1 token as a lightweight approximation. A real tokenizer call would be required for exact counts, but the ILLMPlugin interface does not expose a standalone tokenize() method at this level of abstraction.
     */
    static std::vector<int> estimateTokenSequence(const std::string& text);

    // Statistics updates
    /**
     * @brief Record Cache Hit.
     * @param[in] tokens_saved Input parameter.
     */
    void recordCacheHit(size_t tokens_saved);
    /**
     * @brief Record Cache Miss.
     */
    void recordCacheMiss();
    /**
     * @brief Record Batch Completion.
     * @param[in] batch_size Input parameter.
     */
    void recordBatchCompletion(size_t batch_size);
    void recordRequestCompletion(double latency_ms, const std::string& model_id,
                                 size_t tokens_generated = 0);
    /**
     * @brief Record Request Timeout.
     */
    void recordRequestTimeout();
    /**
     * @brief Record Speculative Step.
     * @param[in] result Input parameter.
     */
    void recordSpeculativeStep(const SpeculativeDecoder::VerifyResult& result);

    /**
     * @brief Speculative decoding helpers Returns true and fills `response` when speculative generation succeeds.
     * @param[in] request Input parameter.
     * @param[in] target_plugin Input parameter.
     * @param[in] draft_plugin Input parameter.
     * @param[in,out] response Input/output parameter.
     * @return True when the operation succeeds.
     * @details Returns false to fall back to standard generation.
     */
    bool trySpeculativeGeneration(
        const InferenceRequest&    request,
        std::shared_ptr<ILLMPlugin> target_plugin,
        std::shared_ptr<ILLMPlugin> draft_plugin,
        InferenceResponse&         response
    );

    /**
     * @brief Resolve the draft model ID for a given target model.
     * @param[in] target_model_id Identifier of the target model.
     * @return Return value.
     * @details Returns config_.speculative_draft_model_id when non-empty. Otherwise, if adapter_registry_ is set, queries it for a DRAFT adapter matching the target model's family (architecture field); returns the matching adapter_id when the corresponding model is registered, or an empty string when no suitable draft model is found.
     */
    std::string resolveDraftModelId(const std::string& target_model_id) const;
    
    // Helper methods
    /**
     * @brief Generate Request Id.
     * @return Return value.
     */
    std::string generateRequestId();
    std::atomic<uint64_t> request_counter_{0};
};

} // namespace llm
} // namespace themis
