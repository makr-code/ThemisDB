#pragma once

/**
 * @file async_inference_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Inference request with priority and metadata
 */
struct AsyncInferenceRequest {
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

/**
 * @brief Asynchronous inference engine
 * 
 * Runs LLM inference in background threads, completely independent
 * from ThemisDB's main database operations.
 * 
 * Thread architecture:
 * - N worker threads for inference (configurable, default: 2)
 * - Separate queue per priority level
 * - Thread-safe submission from any ThemisDB thread
 * 
 * Usage:
 * ```cpp
 * AsyncInferenceEngine engine(plugin, 4);  // 4 worker threads
 * 
 * // Submit from main DB thread - returns immediately
 * InferenceRequest req;
 * req.prompt = "What is ThemisDB?";
 * 
 * auto handle = engine.submit(req);
 * 
 * // Continue DB work...
 * // Later, get result:
 * auto response = handle.get();  // Blocks until ready
 * ```
 */
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
    
    /**
     * @brief Create async inference engine
     * @param plugin LLM plugin to use for inference
     * @param config Configuration
     */
    AsyncInferenceEngine(ILLMPlugin* plugin, const Config& config);
    AsyncInferenceEngine(std::shared_ptr<ILLMPlugin> plugin, const Config& config);

    /**
     * @brief Create async inference engine backed by a shared worker pool.
     *
     * When @p pool is non-null the engine does NOT start its own worker
     * threads; instead, each inference request is submitted directly to the
     * shared pool.  This allows AsyncInferenceEngine and
     * InferenceEngineEnhanced to share a common set of threads and avoid
     * competing for CPU cores.
     *
     * @param plugin LLM plugin to use for inference.
     * @param config Engine configuration.
     * @param pool   Shared thread pool; must outlive this engine.
     */
    AsyncInferenceEngine(ILLMPlugin* plugin, const Config& config,
                         std::shared_ptr<SharedWorkerPool> pool);
    AsyncInferenceEngine(std::shared_ptr<ILLMPlugin> plugin, const Config& config,
                         std::shared_ptr<SharedWorkerPool> pool);
    
    ~AsyncInferenceEngine();
    
    // Prevent copying
    AsyncInferenceEngine(const AsyncInferenceEngine&) = delete;
    AsyncInferenceEngine& operator=(const AsyncInferenceEngine&) = delete;
    
    /**
     * @brief Submit inference request (non-blocking)
     * 
     * Submits request to queue and returns immediately.
     * Request will be processed by worker thread.
     * 
     * @param request Inference request
     * @param priority Higher = more urgent (default: 0)
     * @param timeout Per-request timeout; zero means no timeout (default: 0)
     * @return Handle to track request and get result
     */
    InferenceHandle submit(
        const InferenceRequest& request,
        int priority = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
    );
    
    /**
     * @brief Submit with callback (fire-and-forget)
     * 
     * Result delivered via callback on worker thread.
     * Caller doesn't need to wait for result.
     * 
     * @param request Inference request
     * @param callback Called when inference completes
     * @param priority Request priority
     * @param timeout Per-request timeout; zero means no timeout (default: 0)
     * @return Request ID for tracking / cancellation
     */
    std::string submitAsync(
        const InferenceRequest& request,
        std::function<void(const InferenceResponse&)> callback,
        int priority = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
    );

    /**
     * @brief Token-streaming callback type.
     *
     * Called once per decoded token during streaming inference.  When
     * @p is_final is true the token string is empty and no further calls
     * will be made for this request (normal completion or cancellation).
     *
     * The callback is invoked from the worker thread; implementations must
     * be thread-safe.  SSE framing is applied at the HTTP layer – the
     * engine emits raw token strings.
     *
     * @note The @p token view is only valid for the duration of the callback
     *       invocation.  If the value needs to be retained beyond the callback
     *       return, copy it into a @c std::string before returning.
     */
    using TokenCallback = std::function<void(std::string_view token, bool is_final)>;

    /**
     * @brief Submit a streaming inference request.
     *
     * Submits the request to the worker queue and returns an InferenceHandle
     * immediately.  The @p callback is invoked from the worker thread for
     * each generated token (@p is_final == false) and once more with an
     * empty token string and @p is_final == true when the stream ends
     * (either on normal completion or on cancellation via
     * InferenceHandle::cancel()).
     *
     * Thread-safety: @p callback must be safe to call from a worker thread
     * concurrently with the HTTP layer consuming the tokens.
     *
     * @param request  Inference request; any existing stream_callback is
     *                 overwritten by the internal wrapper.
     * @param callback Per-token callback (see TokenCallback).
     * @param priority Higher = more urgent (default: 0).
     * @param timeout  Per-request timeout; zero means no timeout (default: 0).
     * @return Handle for result retrieval and cancellation.
     */
    InferenceHandle submitStreaming(
        const InferenceRequest& request,
        TokenCallback           callback,
        int                     priority = 0,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
    );
    
    /**
     * @brief Submit RAG request (non-blocking)
     */
    InferenceHandle submitRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request,
        int priority = 0
    );
    
    /**
     * @brief Cancel pending request
     * 
     * Best-effort cancellation. If inference already started,
     * it will complete.
     * 
     * @param request_id Request ID to cancel
     * @return true if cancelled, false if not found or already started
     */
    bool cancel(const std::string& request_id);
    
    /**
     * @brief Get queue statistics
     */
    json getQueueStats() const;
    
    /**
     * @brief Get worker thread statistics
     */
    json getWorkerStats() const;
    
    /**
     * @brief Wait for all pending requests to complete
     * 
     * Blocks until queue is empty. Useful for shutdown.
     */
    void waitForCompletion();
    
    /**
     * @brief Stop all worker threads
     * 
     * Graceful shutdown. Completes pending requests.
     */
    void shutdown();
    
    /**
     * @brief Attach an external deduplication cache.
     *
     * Allows sharing a single LLMResponseCache instance across engines.
     * Overrides any cache created from Config::enable_dedup_cache.
     *
     * Thread-safety: must be called before inference requests are submitted
     * (i.e. during engine setup, not while worker threads are active).
     *
     * @param cache Shared LLMResponseCache instance; may be nullptr to disable.
     */
    void setDedupCache(std::shared_ptr<LLMResponseCache> cache);

    /**
     * @brief Get deduplication cache statistics.
     * @return Cache statistics, or default-constructed stats if cache is disabled.
     */
    LLMResponseCache::CacheStatistics getDedupCacheStats() const;

    /**
     * @brief Hot-swap the underlying LLM plugin without restarting the engine.
     *
     * Atomically replaces the plugin used for new inference requests.
     * In-flight requests that have already acquired a reference to the old plugin
     * will complete normally with the old plugin; requests submitted after this
     * call returns will be routed to @p new_plugin.
     *
     * Thread-safe: uses an internal read-write lock so concurrent worker threads
     * and the calling thread do not race on the plugin pointer.
     *
     * @param new_plugin Replacement plugin; must not be null.
     * @throws std::invalid_argument if @p new_plugin is null.
     */
    void swapPlugin(std::shared_ptr<ILLMPlugin> new_plugin);

    /**
     * @brief Attach a prompt safety policy to the engine.
     *
     * When a non-null policy is set, every inference request is validated
     * against the policy before being dispatched to the plugin.  Blocked
     * prompts receive an immediate error response with
     * @c metadata["blocked"] == true and no plugin call is made.  Redact
     * rules are applied to the prompt in-place before inference.
     *
     * Thread-safety: must be called before inference requests are submitted
     * (i.e. during engine setup, not while worker threads are active).
     *
     * @param policy Shared PromptPolicy instance; nullptr disables the check.
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
    void workerLoop(size_t worker_id);

    // Timeout monitor — runs in a separate thread, marks requests cancelled
    // when their deadline expires.
    void timeoutMonitorLoop();
    void checkAndHandleTimeouts();
    
    // Process single request
    InferenceResponse processRequest(
        const AsyncInferenceRequest& request,
        std::chrono::steady_clock::time_point submit_time
    );
    
    // Generate unique request ID
    std::string generateRequestId();
    
    // Handle backpressure (expects queue_mutex_ locked)
    bool handleBackpressure(std::unique_lock<std::mutex>& lock);
};

} // namespace llm
} // namespace themis
