#pragma once

#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>

/**
 * @file async_inference_engine.h
 * @brief Asynchronous inference engine for ThemisDB
 * 
 * Runs LLM inference in separate thread pool, independent from ThemisDB's
 * main operations. Ensures inference doesn't block database operations.
 * 
 * Key features:
 * - Dedicated thread pool for inference
 * - Non-blocking request submission
 * - Priority queue for request scheduling
 * - Backpressure handling
 * - Cancellation support
 * 
 * Architecture:
 * - Main ThemisDB thread submits requests → returns immediately
 * - Inference threads process requests asynchronously
 * - Results returned via std::future or callback
 */

namespace themis {
namespace llm {

/**
 * @brief Inference request with priority and metadata
 */
struct AsyncInferenceRequest {
    InferenceRequest request;
    int priority = 0;              // Higher = more urgent
    std::string request_id;        // Unique request ID
    
    // Callback for async result delivery
    std::function<void(const InferenceResponse&)> callback;
    
    // For cancellation
    std::atomic<bool> cancelled{false};
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
    };
    
    /**
     * @brief Create async inference engine
     * @param plugin LLM plugin to use for inference
     * @param config Configuration
     */
    AsyncInferenceEngine(ILLMPlugin* plugin, const Config& config);
    AsyncInferenceEngine(std::shared_ptr<ILLMPlugin> plugin, const Config& config);
    
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
     * @return Handle to track request and get result
     */
    InferenceHandle submit(
        const InferenceRequest& request,
        int priority = 0
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
     * @return Request ID for tracking
     */
    std::string submitAsync(
        const InferenceRequest& request,
        std::function<void(const InferenceResponse&)> callback,
        int priority = 0
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
    
private:
    Config config_;
    ILLMPlugin* plugin_;
    std::shared_ptr<ILLMPlugin> owned_plugin_;
    
    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{true};
    
    // Request queue (priority-based)
    struct RequestQueueItem {
        std::shared_ptr<AsyncInferenceRequest> request;
        std::promise<InferenceResponse> promise;
        
        // For priority queue ordering
        bool operator<(const RequestQueueItem& other) const {
            return request->priority < other.request->priority;
        }
    };
    
    std::priority_queue<RequestQueueItem> request_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Request tracking (for cancellation)
    std::unordered_map<std::string, std::shared_ptr<AsyncInferenceRequest>> 
        active_requests_;
    mutable std::mutex tracking_mutex_;
    
    // Statistics
    struct Stats {
        std::atomic<size_t> total_submitted{0};
        std::atomic<size_t> total_completed{0};
        std::atomic<size_t> total_cancelled{0};
        std::atomic<size_t> total_rejected{0};
        std::atomic<double> total_inference_time_ms{0.0};
        std::atomic<double> total_queue_time_ms{0.0};
    };
    Stats stats_;
    
    // Worker thread function
    void workerLoop(size_t worker_id);
    
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
