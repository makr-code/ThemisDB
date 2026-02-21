/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            async_inference_engine.cpp                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:00:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     458                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/async_inference_engine.h"
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
    spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
}

AsyncInferenceEngine::~AsyncInferenceEngine() {
    shutdown();
}

InferenceHandle AsyncInferenceEngine::submit(
    const InferenceRequest& request,
    int priority
) {
    auto submit_time = std::chrono::steady_clock::now();
    
    // Create async request
    auto async_req = std::make_shared<AsyncInferenceRequest>();
    async_req->request = request;
    async_req->priority = priority;
    async_req->request_id = generateRequestId();
    
    // Create promise/future for result
    std::promise<InferenceResponse> promise;
    auto future = promise.get_future().share();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Check queue size and handle backpressure
        if (request_queue_.size() >= config_.max_queue_size) {
            if (!handleBackpressure(lock)) {
                stats_.total_rejected++;
                throw std::runtime_error("Request queue full, request rejected");
            }
        }
        
        // Add to tracking (for cancellation)
        {
            std::lock_guard<std::mutex> tracking_lock(tracking_mutex_);
            active_requests_[async_req->request_id] = async_req;
        }
        
        // Enqueue
        RequestQueueItem item;
        item.request = async_req;
        item.promise = std::move(promise);
        
        request_queue_.push(std::move(item));
        stats_.total_submitted++;
    }
    
    // Notify one worker
    queue_cv_.notify_one();
    
    spdlog::debug("Submitted inference request {} (priority={}, queue_size={})",
                  async_req->request_id, priority, request_queue_.size());
    
    return InferenceHandle(async_req->request_id, future);
}

std::string AsyncInferenceEngine::submitAsync(
    const InferenceRequest& request,
    std::function<void(const InferenceResponse&)> callback,
    int priority
) {
    // Create async request with callback
    auto async_req = std::make_shared<AsyncInferenceRequest>();
    async_req->request = request;
    async_req->priority = priority;
    async_req->request_id = generateRequestId();
    async_req->callback = callback;
    
    // Create promise (result delivered via callback)
    std::promise<InferenceResponse> promise;
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Check queue size
        if (request_queue_.size() >= config_.max_queue_size) {
            if (!handleBackpressure(lock)) {
                stats_.total_rejected++;
                throw std::runtime_error("Request queue full");
            }
        }
        
        // Track and enqueue
        {
            std::lock_guard<std::mutex> tracking_lock(tracking_mutex_);
            active_requests_[async_req->request_id] = async_req;
        }
        
        RequestQueueItem item;
        item.request = async_req;
        item.promise = std::move(promise);
        
        request_queue_.push(std::move(item));
        stats_.total_submitted++;
    }
    
    queue_cv_.notify_one();
    
    spdlog::debug("Submitted async inference request {} (callback mode)",
                  async_req->request_id);
    
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
    
    // Mark as cancelled
    it->second->cancelled.store(true);
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
    
    // Signal workers to stop
    running_.store(false);
    queue_cv_.notify_all();
    
    // Wait for workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    
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
        if (item.request->cancelled.load()) {
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
    
    // Call plugin (blocking inference)
    InferenceResponse response = plugin_->generate(request.request);
    
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

} // namespace llm
} // namespace themis
