#pragma once

/**
 * @file inference_engine_enhanced.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Enhanced inference engine with caching, batching, and routing controls.
 *
 * Coordinates model selection, request scheduling, cache management, and
 * optional distributed inference hooks for advanced LLM serving paths.
 * Public methods are intended for lifecycle control, request submission,
 * and runtime tuning of registered model backends.
 */
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
        /// Number of draft tokens generated per speculative step (K).
        size_t speculative_draft_tokens = 4;
        /// Model ID of the draft model registered via registerModel().
        /// The draft model is typically a small, quantised variant of the
        /// target model (e.g., INT4-quantised 0.5 B parameters).
        std::string speculative_draft_model_id;

        /**
         * @brief Remote shard identifier for cross-shard speculative decoding.
         *
         * When non-empty, trySpeculativeGeneration() will attempt to fetch
         * draft tokens from this remote shard via RemoteExecutor::post() before
         * falling back to the locally registered draft model.
         *
         * Format passed through to SpeculativeDecoder::Config::remote_draft_shard_id.
         * Activate by also calling setRemoteExecutor() with a valid executor and
         * the ShardInfo for the target shard.
         */
        std::string speculative_remote_draft_shard_id;

        // Prompt lookup decoding (n-gram based speculation-light path)
        bool enable_lookup_decoding = false;
        size_t lookup_ngram_min = 2;
        size_t lookup_ngram_max = 4;
        /// Maximum draft tokens proposed per lookup step.
        /// Defaults to lookup_ngram_max (one continuation token per key token),
        /// but can be set independently to allow longer continuations than the
        /// key length (e.g., ngram_max=4, max_draft=8).
        size_t lookup_max_draft_tokens = 0;  // 0 = use lookup_ngram_max
    };
    
    /**
     * @brief Statistics for monitoring and optimization
     */
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
    
    /**
     * @brief Per-model resource quota limits.
     *
     * Both fields default to 0, which means "unlimited".
     * Set via setModelQuota() after registering a model.
     */
    struct ModelResourceQuota {
        /// Maximum number of concurrently active requests for this model.
        /// 0 = unlimited.
        size_t max_concurrent_requests = 0;
        /// Maximum memory this model may consume, in megabytes.
        /// 0 = unlimited.  Informational only — the engine does not enforce
        /// memory allocation; callers may use this to avoid scheduling new
        /// requests when the model's reported footprint exceeds the limit.
        size_t max_memory_mb = 0;
    };

    /**
     * @brief Model information for load balancing
     */
    struct ModelInfo {
        std::string model_id;
        std::shared_ptr<ILLMPlugin> plugin;
        size_t active_requests = 0;
        double avg_response_time_ms = 0.0;
        size_t total_requests = 0;
        bool is_available = true;
        ModelResourceQuota quota;  ///< Per-model resource limits (0 = unlimited)
    };
    
    /**
     * @brief Enhanced inference request with caching metadata
     */
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
    
    explicit InferenceEngineEnhanced(const Config& config);

    /**
     * @brief Create enhanced engine backed by a shared worker pool.
     *
     * When @p pool is non-null the engine runs a single batch-coordinator
     * thread (instead of N private worker threads).  The coordinator forms
     * batches and submits them to the shared pool for execution, so both
     * this engine and an AsyncInferenceEngine can share a common thread
     * set and avoid competing for CPU cores.
     *
     * @param config Engine configuration.
     * @param pool   Shared thread pool; must outlive this engine.
     */
    InferenceEngineEnhanced(const Config& config,
                            std::shared_ptr<SharedWorkerPool> pool);

    ~InferenceEngineEnhanced() noexcept;
    
    // Model management
    
    /**
     * @brief Register a model with the inference engine.
     *
     * @param model_id Model identifier (non-empty required).
     * @param plugin   Plugin implementation to associate with @p model_id.
     * @throws std::invalid_argument if @p model_id is empty or @p plugin is null.
     * @note Rejects empty model_id fail-closed to prevent silent model
     *       registration failures and key-collision vulnerabilities in @c models_.
     */
    void registerModel(const std::string& model_id, std::shared_ptr<ILLMPlugin> plugin);
    
    /**
     * @brief Remove a previously registered model from the engine.
     * @param model_id Model identifier to remove.
     */
    void unregisterModel(const std::string& model_id);

    /**
     * @brief Return the identifiers of all registered models.
     * @return List of currently registered model identifiers.
     */
    std::vector<std::string> getAvailableModels() const;

    /**
     * @brief Attach an adapter registry for DRAFT model auto-discovery.
     *
     * When speculative decoding is enabled and @c Config::speculative_draft_model_id
     * is empty, the engine will call
     * @c AdapterRegistry::findDraftAdapterForFamily() with the target model's
     * architecture/family string each time it selects a target model.  The
     * first DRAFT adapter whose architecture matches the target model's family
     * is used as the draft model for that request, provided its adapter_id is
     * also registered as a model via @c registerModel().
     *
     * Thread-safe: the registry reference is stored under @c models_mutex_.
     *
     * @param registry Non-null adapter registry to query for DRAFT adapters.
     */
    void setAdapterRegistry(std::shared_ptr<AdapterRegistry> registry);

    /**
     * @brief Hot-swap the plugin for a registered model without restarting the engine.
     *
     * Atomically replaces the plugin associated with @p model_id.  In-flight
     * requests that have already taken a reference to the old plugin complete
     * normally; requests dispatched after this call returns will use @p new_plugin.
     *
     * Thread-safe: protected by the existing models_mutex_.
     *
     * @param model_id   The model whose plugin should be replaced.
     * @param new_plugin Replacement plugin; must not be null.
     * @throws std::invalid_argument if @p new_plugin is null or @p model_id is
     *         not registered.
     */
    void swapModel(const std::string& model_id, std::shared_ptr<ILLMPlugin> new_plugin);

    /**
     * @brief Hot-load a LoRA adapter into all registered model plugins at inference time.
     *
     * Registers the adapter metadata and immediately calls plugin->loadLoRA() on
     * every model plugin that is currently registered with this engine (or only on
     * @p model_id when non-empty).  Requests submitted after this call that carry
     * the same @p adapter_id in InferenceRequest::lora_adapter_id will use the
     * newly loaded adapter without any engine restart.
     *
     * Thread-safe: protected by lora_adapters_mutex_ and models_mutex_.
     *
     * @param adapter_id  Unique identifier for the adapter.
     * @param path        Filesystem path to the adapter weights file.
     * @param scale       LoRA scaling factor (default 1.0).
     * @param model_id    Optional: restrict loading to this model only.
     *                    When empty the adapter is loaded on all registered models.
     * @throws std::invalid_argument if @p adapter_id or @p path is empty.
     */
    void loadLoRAAdapter(const std::string& adapter_id,
                         const std::string& path,
                         float scale = 1.0f,
                         const std::string& model_id = "");

    /**
     * @brief Hot-unload a LoRA adapter from all registered model plugins.
     *
     * Removes the adapter registration and calls plugin->unloadLoRA() on every
     * model plugin (or only @p model_id when non-empty).  In-flight requests that
     * have already started generating with this adapter will complete normally.
     *
     * Thread-safe: protected by lora_adapters_mutex_ and models_mutex_.
     *
     * @param adapter_id Adapter to unload; no-op when not registered.
     * @param model_id   Optional: restrict unloading to this model only.
     * @return true if the adapter was registered and unloaded, false otherwise.
     */
    bool unloadLoRAAdapter(const std::string& adapter_id,
                           const std::string& model_id = "");

    /**
     * @brief Return metadata for all currently hot-loaded LoRA adapters.
     *
     * @return Vector of LoRAInfo for every adapter registered via loadLoRAAdapter().
     */
    std::vector<LoRAInfo> getLoadedLoRAAdapters() const;

    /**
     * @brief Set (or replace) the resource quota for a registered model.
     *
     * Thread-safe: protected by models_mutex_.
     *
     * @param model_id Model identifier; must be registered.
     * @param quota    Quota to apply.  Fields set to 0 are unlimited.
     * @throws std::invalid_argument if @p model_id is not registered.
     */
    void setModelQuota(const std::string& model_id, const ModelResourceQuota& quota);

    /**
     * @brief Retrieve the current resource quota for a registered model.
     *
     * @param model_id Model identifier.
     * @return Current quota, or a zero-limit quota if @p model_id is unknown.
     */
    ModelResourceQuota getModelQuota(const std::string& model_id) const;

    // ── Content-based / metadata-tag routing ────────────────────────────────

    /**
     * @brief Add (or replace) a routing rule.
     *
     * When a rule matches the prompt or metadata tags of an incoming request
     * its `target_model_id` is preferred before the normal load-balancing
     * strategy runs.  Multiple rules are evaluated in descending priority order.
     *
     * @param rule  Rule to register.  Must have a non-empty `id` and
     *              `target_model_id`, and at least one pattern or tag.
     * @throws std::invalid_argument on invalid rule fields or bad regex.
     */
    void addRoutingRule(const RoutingRule& rule);

    /**
     * @brief Remove a routing rule by ID.
     * @param rule_id Rule identifier to remove.
     * @return true if the rule existed and was removed.
     */
    bool removeRoutingRule(const std::string& rule_id);

    /**
     * @brief Return all registered routing rules in priority order.
     */
    std::vector<RoutingRule> getRoutingRules() const;

    /**
     * @brief Remove all routing rules, restoring pure load-balancing behaviour.
     */
    void clearRoutingRules();

    // Inference submission
    /**
     * @brief Submit a request to the enhanced scheduler.
     * @param request Enhanced inference request to enqueue.
     * @return Handle for awaiting completion or cancelling the request.
     */
    InferenceHandle submit(const EnhancedInferenceRequest& request);

    /**
     * @brief Submit a request and receive the result through a callback.
     * @param request  Enhanced inference request to enqueue.
     * @param callback Completion callback invoked with the final response.
     * @return Request identifier for later cancellation or reprioritization.
     */
    std::string submitAsync(
        const EnhancedInferenceRequest& request,
        std::function<void(const InferenceResponse&)> callback
    );

    /**
     * @brief Token-streaming callback type (mirrors AsyncInferenceEngine::TokenCallback).
     *
     * Called once per decoded token with @p is_final == false, and once more
     * with an empty token string and @p is_final == true when the stream ends
     * (normal completion or cancellation).  Must be thread-safe.
     *
     * @note The @p token view is only valid for the duration of the callback
     *       invocation.  If the value needs to be retained beyond the callback
     *       return, copy it into a @c std::string before returning.
     */
    using TokenCallback = std::function<void(std::string_view token, bool is_final)>;

    /**
     * @brief Submit a streaming inference request.
     *
     * Wraps the provided @p callback into the batch scheduler's streaming
     * path: each decoded token is delivered via @p callback with
     * @p is_final == false; once the stream ends (completion or
     * InferenceHandle::cancel()) a final call with an empty token and
     * @p is_final == true is made exactly once.
     *
     * Thread-safety: @p callback is invoked from the batch-processing thread;
     * implementations must be safe for concurrent access from the HTTP layer.
     *
     * @param request  Enhanced inference request; any existing
     *                 base_request.stream_callback is overwritten.
     * @param callback Per-token callback (see TokenCallback).
     * @return Handle for result retrieval and cancellation.
     */
    InferenceHandle submitStreaming(
        const EnhancedInferenceRequest& request,
        TokenCallback                   callback
    );
    
    // Request management
    /**
     * @brief Cancel an in-flight or queued request.
     * @param request_id Identifier returned when the request was submitted.
     * @return true if the request was found and cancellation was requested.
     */
    bool cancel(const std::string& request_id);

    /**
     * @brief Update the scheduling priority of a queued request.
     * @param request_id    Identifier of the request to reprioritize.
     * @param new_priority  New scheduler priority value.
     * @return true if the queued request was found and updated.
     */
    bool reprioritize(const std::string& request_id, int new_priority);
    
    // Cache management
    /**
     * @brief Remove all cached inference entries.
     */
    void clearCache();

    /**
     * @brief Seed the cache with prompts expected to be requested soon.
     * @param common_prompts Prompt texts to precompute or stage for cache reuse.
     */
    void prewarmCache(const std::vector<std::string>& common_prompts);
    
    // Statistics and monitoring
    Statistics getStatistics() const;
    json getDetailedMetrics() const;
    
    // Lifecycle
    /**
     * @brief Start worker infrastructure for asynchronous request handling.
     */
    void start();

    /**
     * @brief Stop worker infrastructure and reject further queued work.
     */
    void shutdown();
    bool isRunning() const;

    /**
     * @brief Inject a RemoteExecutor for cross-shard speculative decoding.
     *
     * When set and @c Config::speculative_remote_draft_shard_id is non-empty,
     * trySpeculativeGeneration() will POST draft-token requests to the remote
     * shard before falling back to the locally registered draft model.
     *
     * @param exec          Pointer to the executor.  Pass @c nullptr to detach.
     *                      Ownership is NOT transferred.
     * @param draft_shard   ShardInfo describing the remote draft shard's
     *                      endpoint, shard_id, and health status.
     */
    void setRemoteExecutor(sharding::RemoteExecutor* exec,
                           const sharding::ShardInfo& draft_shard);

    /**
     * @brief Attach a federated inference backend for cross-instance fan-out.
     *
     * When a non-null backend is attached **and** an incoming
     * `EnhancedInferenceRequest` carries a non-empty `target_instance_ids`
     * list, the request is delegated to the backend's `execute()` instead of
     * the local model pipeline.  The backend decides how to fan-out and
     * fan-in across the listed instances.
     *
     * The engine adopts a "first-wins" merge strategy: it returns the text of
     * the first successful `FanOutInstanceResult`.  If all instances fail, the
     * response carries `success=false` and an aggregated error message.
     *
     * Pass @c nullptr to detach a previously attached backend.
     */
    void setFederatedBackend(std::shared_ptr<IFederatedInferenceBackend> backend);

    // ── Wave B B1: Self-RAG integration ───────────────────────────────────

    /**
     * @brief Retrieval hook used when request metadata enables Self-RAG.
     *
     * Parameters: (query, top_k, request).
     */
    using SelfRAGRetrievalCallback = std::function<
        std::vector<themis::rag::SelfRAGDocument>(
            const std::string&,
            size_t,
            const InferenceRequest&)>;

    /**
     * @brief Optional critic hook used when request metadata enables Self-RAG.
     *
     * Parameters: (query, document, request) -> score in [0, 1].
     */
    using SelfRAGCriticCallback = std::function<
        double(
            const std::string&,
            const themis::rag::SelfRAGDocument&,
            const InferenceRequest&)>;

    /// Inject the retrieval backend used by Self-RAG-enabled requests.
    void setSelfRAGRetrievalCallback(SelfRAGRetrievalCallback cb);

    /// Inject an optional critic backend used by Self-RAG-enabled requests.
    void setSelfRAGCriticCallback(SelfRAGCriticCallback cb);

    // ── STUB #262 bridge — target logit injection ─────────────────────────

    /// Callback type for injecting real per-position target-model logits into
    /// trySpeculativeGeneration() without requiring a full llama.cpp rewrite.
    ///
    /// Parameters: (request, K, vocab_size, target_plugin)
    /// Must return exactly K+1 rows of vocab_size floats.
    using TargetLogitsFn = std::function<
        std::vector<std::vector<float>>(
            const InferenceRequest&            /*request*/,
            size_t                             /*K*/,
            size_t                             /*vocab_size*/,
            std::shared_ptr<ILLMPlugin>        /*target_plugin*/)>;

    /// Inject a real target-logit computation into trySpeculativeGeneration().
    /// Pass nullptr / empty fn to restore the built-in peaked-distribution
    /// heuristic (STUB #262).  Thread-safe.
    void setTargetLogitsFn(TargetLogitsFn fn);

    // ── STUB #263 bridge — tokenizer injection ────────────────────────────

    /// Callback type for injecting a real tokenizer into
    /// trySpeculativeGeneration().  Called on the remote draft model's raw text
    /// output to produce proper vocabulary token IDs in place of the built-in
    /// byte-modulo heuristic (byte value % vocab_size).
    ///
    /// Parameters: (text, vocab_size)
    /// Returns:    non-empty vector of IDs each in [0, vocab_size).
    ///             An empty return or a thrown exception never fabricates
    ///             synthetic token IDs from raw bytes: the remote draft path
    ///             retries the local draft model, and the local bridge path
    ///             falls back to the target model response path.
    using TokenizerFn = std::function<std::vector<int>(const std::string& text,
                                                       size_t             vocab_size)>;

    /// Inject a real tokenizer into trySpeculativeGeneration() for the remote
    /// draft path.  Pass nullptr / empty fn to restore the byte-modulo
    /// heuristic (STUB #263).  Thread-safe.
    void setTokenizerFn(TokenizerFn fn);

    /**
     * @brief Remove the injected tokenizer override.
     *
     * Equivalent to setTokenizerFn(nullptr) and restores the byte-modulo
     * fallback used by the speculative-draft bridge. Thread-safe.
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
    void workerLoop(size_t worker_id);
    void timeoutMonitorLoop();
    void processBatch(const std::vector<std::shared_ptr<TrackedRequest>>& batch);

    // Batch coordinator used when a shared pool is provided.
    // Forms batches from the internal queue and submits processBatch()
    // tasks to shared_pool_ rather than executing them inline.
    void batchCoordinatorLoop();
    
    // Cache integration
    std::optional<InferenceResponse> checkCache(const InferenceRequest& request);
    void updateCache(const InferenceRequest& request, const InferenceResponse& response);
    
    // Load balancing
    std::string selectModel(const EnhancedInferenceRequest& request);
    void updateModelStats(const std::string& model_id, double latency_ms, bool success);
    
    // Batch formation
    std::vector<std::shared_ptr<TrackedRequest>> formBatch();
    bool canAddToBatch(const std::shared_ptr<TrackedRequest>& req, size_t current_batch_tokens);
    
    // Timeout handling
    void checkAndHandleTimeouts();
    
    // Embedding helper for cache operations.
    // Uses the first available plugin (see implementation for selection rationale).
    // Returns an empty vector when no plugin is registered or embedding fails
    // (graceful degradation: falls back to exact-key matching only).
    std::vector<float> computeEmbeddingForCache(const std::string& text);

    // Build a token-ID sequence for a given prompt.
    // Uses the rough heuristic of 4 chars ≈ 1 token as a lightweight
    // approximation.  A real tokenizer call would be required for exact counts,
    // but the ILLMPlugin interface does not expose a standalone tokenize()
    // method at this level of abstraction.
    static std::vector<int> estimateTokenSequence(const std::string& text);

    // Statistics updates
    void recordCacheHit(size_t tokens_saved);
    void recordCacheMiss();
    void recordBatchCompletion(size_t batch_size);
    void recordRequestCompletion(double latency_ms, const std::string& model_id,
                                 size_t tokens_generated = 0);
    void recordRequestTimeout();
    void recordSpeculativeStep(const SpeculativeDecoder::VerifyResult& result);

    // Speculative decoding helpers
    // Returns true and fills `response` when speculative generation succeeds.
    // Returns false to fall back to standard generation.
    bool trySpeculativeGeneration(
        const InferenceRequest&    request,
        std::shared_ptr<ILLMPlugin> target_plugin,
        std::shared_ptr<ILLMPlugin> draft_plugin,
        InferenceResponse&         response
    );

    // Resolve the draft model ID for a given target model.
    // Returns config_.speculative_draft_model_id when non-empty.
    // Otherwise, if adapter_registry_ is set, queries it for a DRAFT adapter
    // matching the target model's family (architecture field); returns the
    // matching adapter_id when the corresponding model is registered, or
    // an empty string when no suitable draft model is found.
    std::string resolveDraftModelId(const std::string& target_model_id) const;
    
    // Helper methods
    std::string generateRequestId();
    std::atomic<uint64_t> request_counter_{0};
};

} // namespace llm
} // namespace themis
