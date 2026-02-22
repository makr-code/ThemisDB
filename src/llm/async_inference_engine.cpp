/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            async_inference_engine.cpp                         ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     451                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/async_inference_engine.h"
#include "llm/shared_worker_pool.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// AsyncInferenceEngine Implementation
// ═══════════════════════════════════════════════════════════

AsyncInferenceEngine::AsyncInferenceEngine(
    ILLMPlugin* plugin,
    const Config& config
) : config_(config), plugin_(plugin) {
    
    if (!plugin_) {
        throw std::invalid_argument("Plugin cannot be null");
    }
    
    spdlog::info("AsyncInferenceEngine starting with {} worker threads",
                 config_.num_worker_threads);
    
    // Start worker threads
    workers_.reserve(config_.num_worker_threads);
    for (size_t i = 0; i < config_.num_worker_threads; ++i) {
        workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
    }

    // Start timeout monitor thread
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
    
    spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
}

AsyncInferenceEngine::AsyncInferenceEngine(
    std::shared_ptr<ILLMPlugin> plugin,
    const Config& config
) : config_(config), plugin_(plugin.get()), owned_plugin_(std::move(plugin)) {
    if (!plugin_) {
        throw std::invalid_argument("Plugin cannot be null");
    }
    spdlog::info("AsyncInferenceEngine starting with {} worker threads",
                 config_.num_worker_threads);
    workers_.reserve(config_.num_worker_threads);
    for (size_t i = 0; i < config_.num_worker_threads; ++i) {
        workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
    }

    // Start timeout monitor thread
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);

    spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
}

// ─── Shared-pool constructors ─────────────────────────────────────────────────

AsyncInferenceEngine::AsyncInferenceEngine(
    ILLMPlugin* plugin,
    const Config& config,
    std::shared_ptr<SharedWorkerPool> pool
) : config_(config), plugin_(plugin), shared_pool_(std::move(pool)) {
    if (!plugin_) {
        throw std::invalid_argument("Plugin cannot be null");
    }
    if (!shared_pool_) {
        throw std::invalid_argument("SharedWorkerPool cannot be null");
    }
    spdlog::info("AsyncInferenceEngine started with shared worker pool ({} threads)",
                 shared_pool_->numThreads());
    // Private worker threads are NOT started; the shared pool is used instead.
    // Only the timeout monitor runs locally.
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
}

AsyncInferenceEngine::AsyncInferenceEngine(
    std::shared_ptr<ILLMPlugin> plugin,
    const Config& config,
    std::shared_ptr<SharedWorkerPool> pool
) : config_(config), plugin_(plugin.get()), owned_plugin_(std::move(plugin)),
    shared_pool_(std::move(pool)) {
    if (!plugin_) {
        throw std::invalid_argument("Plugin cannot be null");
    }
    if (!shared_pool_) {
        throw std::invalid_argument("SharedWorkerPool cannot be null");
    }
    spdlog::info("AsyncInferenceEngine started with shared worker pool ({} threads)",
                 shared_pool_->numThreads());
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
}

AsyncInferenceEngine::~AsyncInferenceEngine() {
    shutdown();
}

InferenceHandle AsyncInferenceEngine::submit(
    const InferenceRequest& request,
    int priority,
    std::chrono::milliseconds timeout
) {
    auto submit_time = std::chrono::steady_clock::now();

    // Create async request
    auto async_req = std::make_shared<AsyncInferenceRequest>();
    async_req->request    = request;
    async_req->priority   = priority;
    async_req->request_id = generateRequestId();
    // cancel_token is default-initialised to false in the struct

    // Set per-request deadline when a positive timeout is given
    if (timeout.count() > 0) {
        async_req->deadline = submit_time + timeout;
    }

    // Add to tracking for cancellation and timeout monitoring
    {
        std::lock_guard<std::mutex> tracking_lock(tracking_mutex_);
        active_requests_[async_req->request_id] = async_req;
    }

    std::shared_future<InferenceResponse> future;

    if (shared_pool_) {
        // ── Shared-pool path: submit directly; no internal queue ─────
        auto promise = std::make_shared<std::promise<InferenceResponse>>();
        future = promise->get_future().share();

        bool queued = shared_pool_->submit(
            [this, async_req, promise, submit_time]() {
                if (async_req->cancel_token->load(std::memory_order_acquire)) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error("Request cancelled")));
                    std::lock_guard<std::mutex> lock(tracking_mutex_);
                    active_requests_.erase(async_req->request_id);
                    return;
                }
                try {
                    auto response = processRequest(*async_req, submit_time);
                    stats_.total_completed++;
                    if (async_req->callback) {
                        async_req->callback(response);
                    }
                    promise->set_value(response);
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
                std::lock_guard<std::mutex> lock(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
            },
            priority
        );

        if (!queued) {
            stats_.total_rejected++;
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            active_requests_.erase(async_req->request_id);
            throw std::runtime_error("SharedWorkerPool queue full, request rejected");
        }
        stats_.total_submitted++;
    } else {
        // ── Private-worker path (original behaviour) ──────────────────
        std::promise<InferenceResponse> local_promise;
        future = local_promise.get_future().share();

        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Check queue size and handle backpressure
        if (request_queue_.size() >= config_.max_queue_size) {
            if (!handleBackpressure(lock)) {
                stats_.total_rejected++;
                std::lock_guard<std::mutex> tl(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
                throw std::runtime_error("Request queue full, request rejected");
            }
        }

        RequestQueueItem item;
        item.request = async_req;
        item.promise = std::move(local_promise);
        request_queue_.push(std::move(item));
        stats_.total_submitted++;
        queue_cv_.notify_one();
    }

    spdlog::debug("Submitted inference request {} (priority={}, via_pool={})",
                  async_req->request_id, priority, (shared_pool_ != nullptr));

    return InferenceHandle(async_req->request_id, future, async_req->cancel_token);
}

std::string AsyncInferenceEngine::submitAsync(
    const InferenceRequest& request,
    std::function<void(const InferenceResponse&)> callback,
    int priority,
    std::chrono::milliseconds timeout
) {
    auto submit_time = std::chrono::steady_clock::now();

    // Create async request with callback
    auto async_req = std::make_shared<AsyncInferenceRequest>();
    async_req->request    = request;
    async_req->priority   = priority;
    async_req->request_id = generateRequestId();
    async_req->callback   = callback;

    // Set per-request deadline when a positive timeout is given
    if (timeout.count() > 0) {
        async_req->deadline = submit_time + timeout;
    }

    // Track for cancellation and timeout monitoring
    {
        std::lock_guard<std::mutex> tracking_lock(tracking_mutex_);
        active_requests_[async_req->request_id] = async_req;
    }

    if (shared_pool_) {
        // ── Shared-pool path ─────────────────────────────────────────
        bool queued = shared_pool_->submit(
            [this, async_req, submit_time]() {
                if (async_req->cancel_token->load(std::memory_order_acquire)) {
                    std::lock_guard<std::mutex> lock(tracking_mutex_);
                    active_requests_.erase(async_req->request_id);
                    return;
                }
                try {
                    auto response = processRequest(*async_req, submit_time);
                    stats_.total_completed++;
                    if (async_req->callback) {
                        async_req->callback(response);
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Async callback request {} failed: {}",
                                  async_req->request_id, e.what());
                }
                std::lock_guard<std::mutex> lock(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
            },
            priority
        );

        if (!queued) {
            stats_.total_rejected++;
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            active_requests_.erase(async_req->request_id);
            throw std::runtime_error("SharedWorkerPool queue full");
        }
        stats_.total_submitted++;
    } else {
        // ── Private-worker path (original behaviour) ──────────────────
        std::promise<InferenceResponse> promise;

        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (request_queue_.size() >= config_.max_queue_size) {
            if (!handleBackpressure(lock)) {
                stats_.total_rejected++;
                std::lock_guard<std::mutex> tl(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
                throw std::runtime_error("Request queue full");
            }
        }

        RequestQueueItem item;
        item.request = async_req;
        item.promise = std::move(promise);
        request_queue_.push(std::move(item));
        stats_.total_submitted++;
        queue_cv_.notify_one();
    }

    spdlog::debug("Submitted async inference request {} (callback mode, via_pool={})",
                  async_req->request_id, (shared_pool_ != nullptr));

    return async_req->request_id;
}

InferenceHandle AsyncInferenceEngine::submitRAG(
    const RAGContext& rag_context,
    const InferenceRequest& request,
    int priority
) {
    // Format RAG request and submit
    InferenceRequest rag_request = request;
    
    // RAG gets higher priority (usually more important)
    int rag_priority = priority + 10;
    
    spdlog::debug("Submitting RAG request with {} documents",
                  rag_context.documents.size());
    
    // Store RAG context in metadata for worker to use
    rag_request.metadata["rag_enabled"] = true;
    rag_request.metadata["num_documents"] = rag_context.documents.size();
    
    // TODO: Properly encode RAG context in request
    // For now, just append to prompt
    std::ostringstream oss;
    oss << "Context:\n";
    for (const auto& doc : rag_context.documents) {
        oss << doc.content << "\n\n";
    }
    oss << "Question: " << rag_context.query << "\n";
    rag_request.prompt = oss.str();
    
    return submit(rag_request, rag_priority);
}

bool AsyncInferenceEngine::cancel(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    
    auto it = active_requests_.find(request_id);
    if (it == active_requests_.end()) {
        return false;  // Not found or already completed
    }
    
    // Set the shared cancellation token so the worker and any streaming
    // callback are notified at the next check point.
    it->second->cancel_token->store(true, std::memory_order_release);
    stats_.total_cancelled++;
    
    spdlog::info("Cancelled inference request: {}", request_id);
    return true;
}

json AsyncInferenceEngine::getQueueStats() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    json stats;
    stats["queue_size"] = request_queue_.size();
    stats["queue_max"] = config_.max_queue_size;
    stats["utilization"] = (config_.max_queue_size > 0) 
        ? (request_queue_.size() * 100.0 / config_.max_queue_size) : 0.0;
    
    return stats;
}

json AsyncInferenceEngine::getWorkerStats() const {
    json stats;
    stats["num_workers"] = config_.num_worker_threads;
    stats["total_submitted"] = stats_.total_submitted.load();
    stats["total_completed"] = stats_.total_completed.load();
    stats["total_cancelled"] = stats_.total_cancelled.load();
    stats["total_rejected"] = stats_.total_rejected.load();
    stats["total_timed_out"] = stats_.total_timed_out.load();
    
    auto completed = stats_.total_completed.load();
    if (completed > 0) {
        stats["avg_inference_time_ms"] = 
            stats_.total_inference_time_ms.load() / completed;
        stats["avg_queue_time_ms"] = 
            stats_.total_queue_time_ms.load() / completed;
    }
    
    return stats;
}

void AsyncInferenceEngine::waitForCompletion() {
    spdlog::info("Waiting for all pending inference requests to complete...");
    
    // Use condition variable for efficient waiting instead of polling with sleep
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cv_.wait(lock, [this] { return request_queue_.empty(); });
    
    spdlog::info("All inference requests completed");
}

void AsyncInferenceEngine::shutdown() {
    if (!running_.load()) {
        return;  // Already shutdown
    }
    
    spdlog::info("Shutting down AsyncInferenceEngine...");
    
    // Signal workers and timeout monitor to stop
    running_.store(false);
    queue_cv_.notify_all();
    
    // Wait for workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    // Wait for timeout monitor
    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }
    
    spdlog::info("AsyncInferenceEngine shutdown complete");
}

// ═══════════════════════════════════════════════════════════
// Private Methods
// ═══════════════════════════════════════════════════════════

void AsyncInferenceEngine::workerLoop(size_t worker_id) {
    spdlog::info("Inference worker {} started", worker_id);
    
    while (running_.load()) {
        RequestQueueItem item;
        
        // Wait for work
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            queue_cv_.wait(lock, [this] {
                return !request_queue_.empty() || !running_.load();
            });
            
            if (!running_.load() && request_queue_.empty()) {
                break;  // Shutdown
            }
            
            if (request_queue_.empty()) {
                continue;
            }
            
            // Get highest priority request
            item = std::move(const_cast<RequestQueueItem&>(request_queue_.top()));
            request_queue_.pop();
        }
        
        auto queue_end_time = std::chrono::steady_clock::now();
        
        // Check if cancelled
        if (item.request->cancel_token->load(std::memory_order_acquire)) {
            spdlog::debug("Skipping cancelled request: {}", 
                         item.request->request_id);
            
            // Set exception in promise
            item.promise.set_exception(
                std::make_exception_ptr(std::runtime_error("Request cancelled"))
            );
            
            // Remove from tracking
            {
                std::lock_guard<std::mutex> lock(tracking_mutex_);
                active_requests_.erase(item.request->request_id);
            }
            
            continue;
        }
        
        // Process request
        spdlog::debug("Worker {} processing request {}", 
                     worker_id, item.request->request_id);
        
        try {
            auto submit_time = std::chrono::steady_clock::now();
            
            // Actual inference (blocking call to plugin)
            auto response = processRequest(*item.request, submit_time);
            
            // Update stats
            stats_.total_completed++;
            
            // Deliver result
            if (item.request->callback) {
                // Call callback on worker thread
                item.request->callback(response);
            }
            item.promise.set_value(response);
            
            spdlog::debug("Worker {} completed request {} in {:.1f}ms",
                         worker_id, item.request->request_id,
                         response.inference_time_ms);
            
        } catch (const std::exception& e) {
            spdlog::error("Worker {} failed to process request {}: {}",
                         worker_id, item.request->request_id, e.what());
            
            // Set exception in promise
            item.promise.set_exception(std::current_exception());
        }
        
        // Remove from tracking
        {
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            active_requests_.erase(item.request->request_id);
        }
    }
    
    spdlog::info("Inference worker {} stopped", worker_id);
}

InferenceResponse AsyncInferenceEngine::processRequest(
    const AsyncInferenceRequest& request,
    std::chrono::steady_clock::time_point submit_time
) {
    auto start_time = std::chrono::steady_clock::now();
    
    // Calculate queue time
    auto queue_time = std::chrono::duration<double, std::milli>(
        start_time - submit_time
    ).count();
    stats_.total_queue_time_ms.fetch_add(queue_time);

    // Build an effective InferenceRequest that wraps the stream_callback so
    // cancellation (and deadline expiry) are checked at every token boundary.
    InferenceRequest effective_request = request.request;
    auto cancel_token = request.cancel_token;  // capture shared ownership
    auto deadline = request.deadline;

    if (effective_request.stream_callback) {
        // Wrap the original callback: stop streaming when cancelled/timed-out.
        auto original_cb = std::move(effective_request.stream_callback);
        effective_request.stream_callback = [original_cb, cancel_token, deadline]
            (const std::string& token) {
            // Abort streaming on cancellation
            if (cancel_token->load(std::memory_order_acquire)) {
                return;  // silently drop token; plugin will finish its call
            }
            // Abort streaming on deadline expiry
            if (deadline != std::chrono::steady_clock::time_point{} &&
                std::chrono::steady_clock::now() >= deadline) {
                cancel_token->store(true, std::memory_order_release);
                return;
            }
            original_cb(token);
        };
    }
    
    // Call plugin (blocking inference)
    InferenceResponse response = plugin_->generate(effective_request);
    
    // Add metadata
    response.metadata["async"] = true;
    response.metadata["queue_time_ms"] = queue_time;
    response.metadata["request_id"] = request.request_id;
    response.metadata["priority"] = request.priority;
    
    // Update stats
    stats_.total_inference_time_ms.fetch_add(response.inference_time_ms);
    
    return response;
}

std::string AsyncInferenceEngine::generateRequestId() {
    static std::atomic<uint64_t> counter{0};
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    uint64_t id = counter.fetch_add(1);
    
    std::ostringstream oss;
    oss << "inf_" << timestamp << "_" << id;
    return oss.str();
}

bool AsyncInferenceEngine::handleBackpressure(std::unique_lock<std::mutex>& lock) {
    // Already have lock on queue_mutex_
    
    switch (config_.backpressure) {
        case Config::BackpressurePolicy::BLOCK:
            // Wait for space (releases lock while waiting)
            queue_cv_.wait(lock, [this] {
                return request_queue_.size() < config_.max_queue_size ||
                       !running_.load();
            });
            return running_.load();
            
        case Config::BackpressurePolicy::DROP_OLDEST:
            // Remove lowest priority item
            if (!request_queue_.empty()) {
                spdlog::warn("Queue full, dropping lowest priority request");
                // std::priority_queue doesn't support removal
                // In production, use custom heap or deque
                // For now, just reject
                return false;
            }
            return true;
            
        case Config::BackpressurePolicy::REJECT:
            return false;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════
// Timeout Monitoring
// ═══════════════════════════════════════════════════════════

void AsyncInferenceEngine::timeoutMonitorLoop() {
    spdlog::debug("AsyncInferenceEngine timeout monitor started");

    while (running_.load()) {
        checkAndHandleTimeouts();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    spdlog::debug("AsyncInferenceEngine timeout monitor stopped");
}

void AsyncInferenceEngine::checkAndHandleTimeouts() {
    auto now = std::chrono::steady_clock::now();
    const auto zero_tp = std::chrono::steady_clock::time_point{};

    std::lock_guard<std::mutex> lock(tracking_mutex_);
    for (auto& [id, req] : active_requests_) {
        // Skip requests with no deadline or already cancelled
        if (req->deadline == zero_tp) continue;
        if (req->cancel_token->load(std::memory_order_acquire)) continue;

        if (now >= req->deadline) {
            spdlog::warn("Request {} exceeded per-request timeout, marking cancelled", id);
            req->cancel_token->store(true, std::memory_order_release);
            stats_.total_timed_out++;
            stats_.total_cancelled++;
        }
    }
}

} // namespace llm
} // namespace themis
