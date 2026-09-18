#pragma once

/**
 * @file async_inference_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llm_response_cache.h"
#include "llm/prompt_policy.h"
#include "llm/shared_worker_pool.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace llm {

struct AsyncInferenceRequest {
    /**
     * @brief Async Inference Request.
     * @return Return value.
     */
    virtual ~AsyncInferenceRequest() = default;
    InferenceRequest request;
    int priority = 0;              // Higher = more urgent
    std::string request_id;        // Unique request ID
    
    // Callback for async result delivery
    std::function<void(const InferenceResponse&)> callback;
    
    // Shared cancellation token — also held by the InferenceHandle so
    // calling InferenceHandle::cancel() propagates here immediately.
    std::shared_ptr<std::atomic<bool>> cancel_token =
        std::make_shared<std::atomic<bool>>(false);

    // Per-request deadline (steady_clock); zero() means no timeout.
    std::chrono::steady_clock::time_point deadline;

    // Shared promise — owned jointly by the queue item (or pool task lambda)
    // and the timeout monitor so that the monitor can resolve the future
    // immediately when the deadline expires, even while the worker is still
    // executing the plugin call.
    std::shared_ptr<std::promise<InferenceResponse>> shared_promise;
};

class AsyncInferenceEngine {
public:
    struct Config {
        size_t num_worker_threads = 2;     // Number of inference threads
        size_t max_queue_size = 1000;      // Max pending requests
        bool enable_priorities = true;     // Priority scheduling
        
        // Backpressure: what to do when queue is full
        enum class BackpressurePolicy {
            BLOCK,          // Block until space available
            DROP_OLDEST,    // Drop oldest low-priority request
            REJECT          // Reject new request
        };
        BackpressurePolicy backpressure = BackpressurePolicy::BLOCK;
        
        // Deduplication cache: return cached response for identical prompts
        bool enable_dedup_cache = false;
        LLMResponseCache::Config dedup_cache_config;  // Cache config (set cache_dir before use)
    };
    
    AsyncInferenceEngine(ILLMPlugin* plugin, const Config& config);
    AsyncInferenceEngine(std::shared_ptr<ILLMPlugin> plugin, const Config& config);

    AsyncInferenceEngine(ILLMPlugin* plugin, const Config& config,
                         std::shared_ptr<SharedWorkerPool> pool);
    AsyncInferenceEngine(std::shared_ptr<ILLMPlugin> plugin, const Config& config,
                         std::shared_ptr<SharedWorkerPool> pool);
    
    ~AsyncInferenceEngine();
    
    // Prevent copying
    AsyncInferenceEngine(const AsyncInferenceEngine&) = delete;
    AsyncInferenceEngine& operator=(const AsyncInferenceEngine&) = delete;
    
    InferenceHandle submit(
        const InferenceRequest& request,
        int priority = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
    );
    
    std::string submitAsync(
        const InferenceRequest& request,
        std::function<void(const InferenceResponse&)> callback,
        int priority = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
    );

    using TokenCallback = std::function<void(std::string_view token, bool is_final)>;

    InferenceHandle submitStreaming(
        const InferenceRequest& request,
        TokenCallback           callback,
        int                     priority = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
    );
    
    InferenceHandle submitRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request,
        int priority = 0
    );
    
    /**
     * @brief Cancel.
     * @param[in] request_id Identifier of the request.
     * @return True when the operation succeeds.
     */
    bool cancel(const std::string& request_id);
    
    /**
     * @brief Get Queue Stats.
     * @return Return value.
     */
    json getQueueStats() const;
    
    /**
     * @brief Get Worker Stats.
     * @return Return value.
     */
    json getWorkerStats() const;
    
    /**
     * @brief Wait For Completion.
     */
    void waitForCompletion();
    
    /**
     * @brief Shutdown.
     */
    void shutdown();
    
    /**
     * @brief Set Dedup Cache.
     * @param[in] cache Input parameter.
     */
    void setDedupCache(std::shared_ptr<LLMResponseCache> cache);

    /**
     * @brief Get Dedup Cache Stats.
     * @return Return value.
     */
    LLMResponseCache::CacheStatistics getDedupCacheStats() const;

    /**
     * @brief Swap Plugin.
     * @param[in] new_plugin Input parameter.
     */
    void swapPlugin(std::shared_ptr<ILLMPlugin> new_plugin);

    /**
     * @brief Set Prompt Policy.
     * @param[in] policy Input parameter.
     */
    void setPromptPolicy(std::shared_ptr<PromptPolicy> policy);

private:
    // ─────────────────────────────────────────────────────────────────────
    // LOCK ORDERING INVARIANTS (prevent circular waits and deadlocks)
    // ─────────────────────────────────────────────────────────────────────
    // 
    // When multiple locks must be held, acquire them in this order:
    // 1. plugin_mutex_       (R/W lock for hot-swap)
    // 2. queue_mutex_        (protects request_queue_)
    // 3. tracking_mutex_     (protects active_requests_)
    // 4. latency_mutex_      (protects latency_samples_)
    // 5. cache_meta_mutex_   (protects cache metadata)
    // 6. policy_mutex_       (protects prompt_policy_)
    // 7. stats_time_mutex_   (protects engine_start_time_)
    //
    // NEVER acquire mutexes in reverse order. If you need multiple locks,
    // always follow this sequence to prevent deadlock.
    // ─────────────────────────────────────────────────────────────────────
    
    Config config_;
    ILLMPlugin* plugin_;
    std::shared_ptr<ILLMPlugin> owned_plugin_;

    // Read-write lock protecting active_plugin_ for hot-swap support.
    // Worker threads take shared (read) locks; swapPlugin() takes an exclusive lock.
    mutable std::shared_mutex plugin_mutex_;
    // The currently active plugin snapshot — swapped atomically by swapPlugin().
    std::shared_ptr<ILLMPlugin> active_plugin_;

    // Optional shared worker pool (nullptr → private workers used instead)
    std::shared_ptr<SharedWorkerPool> shared_pool_;

    // Optional deduplication cache (nullptr if disabled)
    std::shared_ptr<LLMResponseCache> dedup_cache_;
    mutable std::mutex cache_meta_mutex_;  // Protects metadata access on cached responses

    // Optional prompt safety policy (nullptr → no prompt validation)
    std::shared_ptr<PromptPolicy> prompt_policy_;
    mutable std::mutex policy_mutex_;  // Protects prompt_policy_ for thread-safe access

    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{true};
    
    // Request queue (priority-based)
    struct RequestQueueItem {
        std::shared_ptr<AsyncInferenceRequest> request;
        // Shared with async_req->shared_promise so the timeout monitor can
        // resolve the future early even while this item is being processed.
        std::shared_ptr<std::promise<InferenceResponse>> promise;
        
        // For priority queue ordering
        bool operator<(const RequestQueueItem& other) const {
            return request->priority < other.request->priority;
        }
    };
    
    std::vector<RequestQueueItem> request_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Request tracking (for cancellation and timeout)
    std::unordered_map<std::string, std::shared_ptr<AsyncInferenceRequest>> 
        active_requests_;
    mutable std::mutex tracking_mutex_;

    // Timeout monitor thread — fires deadline-based cancellation
    std::thread timeout_thread_;
    
    // Statistics
    struct Stats {
        std::atomic<size_t> total_submitted{0};
        std::atomic<size_t> total_completed{0};
        std::atomic<size_t> total_cancelled{0};
        std::atomic<size_t> total_rejected{0};
        std::atomic<size_t> total_timed_out{0};
        std::atomic<double> total_inference_time_ms{0.0};
        std::atomic<double> total_queue_time_ms{0.0};
        std::atomic<size_t> total_dedup_cache_hits{0};
        std::atomic<size_t> total_dedup_cache_misses{0};
        std::atomic<size_t> total_tokens_generated{0};
    };
    Stats stats_;

    // Engine start time for tokens/sec wall-clock calculation.
    // Protected by stats_mutex_ to avoid data races when read from getQueueStats().
    std::chrono::steady_clock::time_point engine_start_time_{std::chrono::steady_clock::now()};
    mutable std::mutex stats_time_mutex_;  // Protects engine_start_time_ access

    // Per-request latency samples for p99 computation (protected by latency_mutex_).
    // Using deque for O(1) front-removal when the window exceeds 10 000 samples.
    std::deque<double> latency_samples_;
    mutable std::mutex latency_mutex_;
    
    // Worker thread function
    /**
     * @brief Worker Loop.
     * @param[in] worker_id Identifier of the worker.
     */
    void workerLoop(size_t worker_id);

    /**
     * @brief Timeout monitor — runs in a separate thread, marks requests cancelled when their deadline expires.
     */
    void timeoutMonitorLoop();
    /**
     * @brief Check And Handle Timeouts.
     */
    void checkAndHandleTimeouts();
    
    // Process single request
    /**
     * @brief Process Request.
     * @param[in] request Input parameter.
     * @param[in] submit_time Input parameter.
     * @return Return value.
     */
    InferenceResponse processRequest(
        const AsyncInferenceRequest& request,
        std::chrono::steady_clock::time_point submit_time
    );
    
    /**
     * @brief Generate unique request ID
     * @return Return value.
     */
    std::string generateRequestId();
    
    /**
     * @brief Handle backpressure (expects queue_mutex_ locked)
     * @param[in,out] lock Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool handleBackpressure(std::unique_lock<std::mutex>& lock);
};

} // namespace llm
} // namespace themis
