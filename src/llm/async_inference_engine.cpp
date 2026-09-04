/**
 * @file async_inference_engine.cpp
 * @brief Async inference engine implementation.
 * @version 1.9.0-beta
 * @note Score: 100/100
 * @note Status: Production Ready
 */

#include "llm/async_inference_engine.h"
#include <stdexcept>
#include "llm/llm_response_cache.h"
#include "llm/shared_worker_pool.h"
#include <spdlog/spdlog.h>
#include "utils/thread_join_utils.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "utils/logger.h"

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
        spdlog::warn("AsyncInferenceEngine created with null plugin; requests will fail closed until a plugin is installed");
    }
    // Wrap raw pointer in a non-owning shared_ptr for hot-swap support.
    // THREAD-SAFETY: Acquire plugin_mutex_ during construction for clarity
    // and to establish happens-before relationship with first worker access.
    {
        std::lock_guard<std::shared_mutex> lock(plugin_mutex_);
        active_plugin_ = std::shared_ptr<ILLMPlugin>(plugin_, [](ILLMPlugin*){});
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

    // Initialize deduplication cache if enabled
    if (config_.enable_dedup_cache) {
        dedup_cache_ = std::make_shared<LLMResponseCache>("async_dedup_cache",
                                                          config_.dedup_cache_config);
        spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
                     config_.dedup_cache_config.cache_dir);
    }
    
    spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
}

AsyncInferenceEngine::AsyncInferenceEngine(
    std::shared_ptr<ILLMPlugin> plugin,
    const Config& config
) : config_(config), plugin_(plugin ? plugin.get() : nullptr), owned_plugin_(std::move(plugin)) {
    if (!plugin_) {
        spdlog::warn("AsyncInferenceEngine created with null plugin; requests will fail closed until a plugin is installed");
    }
    // THREAD-SAFETY: Acquire plugin_mutex_ during construction for clarity
    // and to establish happens-before relationship with first worker access.
    {
        std::lock_guard<std::shared_mutex> lock(plugin_mutex_);
        active_plugin_ = owned_plugin_;
    }
    spdlog::info("AsyncInferenceEngine starting with {} worker threads",
                 config_.num_worker_threads);
    workers_.reserve(config_.num_worker_threads);
    for (size_t i = 0; i < config_.num_worker_threads; ++i) {
        workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
    }

    // Start timeout monitor thread
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);

    // Initialize deduplication cache if enabled
    if (config_.enable_dedup_cache) {
        dedup_cache_ = std::make_shared<LLMResponseCache>("async_dedup_cache",
                                                          config_.dedup_cache_config);
        spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
                     config_.dedup_cache_config.cache_dir);
    }

    spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
}

// ─── Shared-pool constructors ─────────────────────────────────────────────────

AsyncInferenceEngine::AsyncInferenceEngine(
    ILLMPlugin* plugin,
    const Config& config,
    std::shared_ptr<SharedWorkerPool> pool
) : config_(config), plugin_(plugin), shared_pool_(std::move(pool)) {
    if (!plugin_) {
        spdlog::warn("AsyncInferenceEngine created with null plugin; requests will fail closed until a plugin is installed");
    }
    if (!shared_pool_) {
        throw std::invalid_argument("SharedWorkerPool cannot be null");
    }
    // Wrap raw pointer in a non-owning shared_ptr for hot-swap support.
    // THREAD-SAFETY: Acquire plugin_mutex_ during construction for clarity
    {
        std::lock_guard<std::shared_mutex> lock(plugin_mutex_);
        active_plugin_ = std::shared_ptr<ILLMPlugin>(plugin_, [](ILLMPlugin*){});
    }
    spdlog::info("AsyncInferenceEngine started with shared worker pool ({} threads)",
                 shared_pool_->numThreads());
    // Private worker threads are NOT started; the shared pool is used instead.
    // Only the timeout monitor runs locally.
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);

    // Initialize deduplication cache if enabled
    if (config_.enable_dedup_cache) {
        dedup_cache_ = std::make_shared<LLMResponseCache>("async_dedup_cache",
                                                          config_.dedup_cache_config);
        spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
                     config_.dedup_cache_config.cache_dir);
    }
}

AsyncInferenceEngine::AsyncInferenceEngine(
    std::shared_ptr<ILLMPlugin> plugin,
    const Config& config,
    std::shared_ptr<SharedWorkerPool> pool
) : config_(config), plugin_(plugin ? plugin.get() : nullptr), owned_plugin_(std::move(plugin)),
    shared_pool_(std::move(pool)) {
    if (!plugin_) {
        spdlog::warn("AsyncInferenceEngine created with null plugin; requests will fail closed until a plugin is installed");
    }
    if (!shared_pool_) {
        throw std::invalid_argument("SharedWorkerPool cannot be null");
    }
    // THREAD-SAFETY: Acquire plugin_mutex_ during construction for clarity
    {
        std::lock_guard<std::shared_mutex> lock(plugin_mutex_);
        active_plugin_ = owned_plugin_;
    }
    spdlog::info("AsyncInferenceEngine started with shared worker pool ({} threads)",
                 shared_pool_->numThreads());
    timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);

    // Initialize deduplication cache if enabled
    if (config_.enable_dedup_cache) {
        dedup_cache_ = std::make_shared<LLMResponseCache>("async_dedup_cache",
                                                          config_.dedup_cache_config);
        spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
                     config_.dedup_cache_config.cache_dir);
    }
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

    spdlog::info(
        "AsyncInferenceEngine::submit start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",
        request.model_id.empty() ? std::string{"default"} : request.model_id,
        request.prompt.size(),
        priority,
        timeout.count(),
        (shared_pool_ != nullptr));

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
        async_req->shared_promise = promise;  // allow timeout monitor to resolve future
        future = promise->get_future().share();

        bool queued = shared_pool_->submit(
            [this, async_req, promise, submit_time]() {
                if (async_req->cancel_token->load(std::memory_order_acquire)) {
                    try {
                        promise->set_exception(std::make_exception_ptr(
                            std::runtime_error("Request cancelled")));
                    } catch (...) { /* Promise already satisfied; ignore. */ }
                    std::lock_guard<std::mutex> lock(tracking_mutex_);
                    active_requests_.erase(async_req->request_id);
                    return;
                }
                try {
                    auto response = processRequest(*async_req, submit_time);
                    stats_.total_completed.fetch_add(1, std::memory_order_relaxed);
                    if ([[maybe_unused]] async_req->callback) {
                        async_req->callback([[maybe_unused]] response);
                    }
                    try { promise->set_value(response); } catch (...) { /* Promise already satisfied; ignore. */ }
                } catch (...) {
                    THEMIS_WARN("async_inference_engine: unhandled exception caught");
                    try { promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied; ignore. */ }
                }
                std::lock_guard<std::mutex> lock(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
            },
            priority
        );

        if (!queued) {
            stats_.total_rejected.fetch_add(1, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            active_requests_.erase(async_req->request_id);
            throw std::runtime_error("SharedWorkerPool queue full, request rejected");
        }
        stats_.total_submitted.fetch_add(1, std::memory_order_relaxed);
    } else {
        // ── Private-worker path (original behaviour) ──────────────────
        auto local_promise = std::make_shared<std::promise<InferenceResponse>>();
        async_req->shared_promise = local_promise;
        future = local_promise->get_future().share();

        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Check queue size and handle backpressure
        if (request_queue_.size() >= config_.max_queue_size) {
            if (!handleBackpressure(lock)) {
                stats_.total_rejected.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> tl(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
                throw std::runtime_error("Request queue full, request rejected");
            }
        }

        RequestQueueItem item;
        item.request = async_req;
        item.promise = std::move(local_promise);
        request_queue_.push_back(std::move(item));
        std::push_heap(request_queue_.begin(), request_queue_.end());
        stats_.total_submitted.fetch_add(1, std::memory_order_relaxed);
        queue_cv_.notify_one();
    }

    spdlog::debug("Submitted inference request {} (priority={}, via_pool={})",
                  async_req->request_id, priority, (shared_pool_ != nullptr));

    spdlog::info(
        "AsyncInferenceEngine::submit queued: request_id={} priority={} via_pool={}",
        async_req->request_id,
        priority,
        (shared_pool_ != nullptr));

    return InferenceHandle(async_req->request_id, future, async_req->cancel_token);
}

std::string AsyncInferenceEngine::submitAsync(
    const InferenceRequest& request,
    std::function<void(const InferenceResponse&)> callback,
    int priority,
    std::chrono::milliseconds timeout
) {
    auto submit_time = std::chrono::steady_clock::now();

    spdlog::info(
        "AsyncInferenceEngine::submitAsync start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",
        request.model_id.empty() ? std::string{"default"} : request.model_id,
        request.prompt.size(),
        priority,
        timeout.count(),
        (shared_pool_ != nullptr));

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
                    stats_.total_completed.fetch_add(1, std::memory_order_relaxed);
                    if ([[maybe_unused]] async_req->callback) {
                        async_req->callback([[maybe_unused]] response);
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
            stats_.total_rejected.fetch_add(1, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            active_requests_.erase(async_req->request_id);
            throw std::runtime_error("SharedWorkerPool queue full");
        }
        stats_.total_submitted.fetch_add(1, std::memory_order_relaxed);
    } else {
        // ── Private-worker path (original behaviour) ──────────────────
        auto promise = std::make_shared<std::promise<InferenceResponse>>();
        async_req->shared_promise = promise;

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
        request_queue_.push_back(std::move(item));
        std::push_heap(request_queue_.begin(), request_queue_.end());
        stats_.total_submitted++;
        queue_cv_.notify_one();
    }

    spdlog::debug("Submitted async inference request {} (callback mode, via_pool={})",
                  async_req->request_id, (shared_pool_ != nullptr));

    spdlog::info(
        "AsyncInferenceEngine::submitAsync queued: request_id={} priority={} via_pool={}",
        async_req->request_id,
        priority,
        (shared_pool_ != nullptr));

    return async_req->request_id;
}

InferenceHandle AsyncInferenceEngine::submitStreaming(
    const InferenceRequest&   request,
    TokenCallback             callback,
    int                       priority,
    std::chrono::milliseconds timeout
) {
    auto submit_time = std::chrono::steady_clock::now();

    spdlog::info(
        "AsyncInferenceEngine::submitStreaming start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",
        request.model_id.empty() ? std::string{"default"} : request.model_id,
        request.prompt.size(),
        priority,
        timeout.count(),
        (shared_pool_ != nullptr));

    auto async_req          = std::make_shared<AsyncInferenceRequest>();
    async_req->request      = request;
    async_req->priority     = priority;
    async_req->request_id   = generateRequestId();

    if (timeout.count() > 0) {
        async_req->deadline = submit_time + timeout;
    }

    // Wrap the TokenCallback so each decoded token is delivered with
    // is_final=false, and the final sentinel (is_final=true, empty token)
    // is fired exactly once regardless of whether the stream ends normally
    // or via cancellation.
    auto fired_final    = std::make_shared<std::atomic<bool>>(false);
    auto cb             = std::make_shared<TokenCallback>([[maybe_unused]] std::move(callback));

    async_req->request.stream_callback =
        [cb, cancel_token = async_req->cancel_token, fired_final](const std::string& token) {
            if (cancel_token->load(std::memory_order_acquire)) {
                // Cancellation detected: fire the final sentinel once.
                bool expected = false;
                if (fired_final->compare_exchange_strong(
                        expected, true, std::memory_order_acq_rel)) {
                    (*cb)(std::string_view{}, /*is_final=*/true);
                }
                return;
            }
            (*cb)(std::string_view{token}, /*is_final=*/false);
        };

    // Completion callback: fires the final sentinel after all tokens have
    // been delivered (covers the non-cancelled path).
    async_req->callback =
        [cb, fired_final](const InferenceResponse&) {
            bool expected = false;
            if (fired_final->compare_exchange_strong(
                    expected, true, std::memory_order_acq_rel)) {
                (*cb)(std::string_view{}, /*is_final=*/true);
            }
        };

    // Track for cancellation / timeout monitoring.
    {
        std::lock_guard<std::mutex> tracking_lock(tracking_mutex_);
        active_requests_[async_req->request_id] = async_req;
    }

    std::shared_future<InferenceResponse> future;

    if (shared_pool_) {
        auto promise          = std::make_shared<std::promise<InferenceResponse>>();
        async_req->shared_promise = promise;
        future = promise->get_future().share();

        bool queued = shared_pool_->submit(
            [this, async_req, promise, submit_time]() {
                if (async_req->cancel_token->load(std::memory_order_acquire)) {
                    if ([[maybe_unused]] async_req->callback) {
                        async_req->callback([[maybe_unused]] InferenceResponse{});
                    }
                    try {
                        promise->set_exception(std::make_exception_ptr(
                            std::runtime_error("Request cancelled")));
                    } catch (...) {}
                    std::lock_guard<std::mutex> lock(tracking_mutex_);
                    active_requests_.erase(async_req->request_id);
                    return;
                }
                try {
                    auto response = processRequest(*async_req, submit_time);
                    stats_.total_completed++;
                    if ([[maybe_unused]] async_req->callback) {
                      async_req->callback(response);
                    }
                    try { promise->set_value(response); } catch (...) {}
                } catch (...) {
                    THEMIS_WARN("async_inference_engine: unhandled exception caught");
                    try { promise->set_exception(std::current_exception()); } catch (...) {}
                }
                std::lock_guard<std::mutex> lock(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
            },
            priority
        );

        if (!queued) {
            stats_.total_rejected.fetch_add(1, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(tracking_mutex_);
            active_requests_.erase(async_req->request_id);
            throw std::runtime_error("SharedWorkerPool queue full, streaming request rejected");
        }
        stats_.total_submitted.fetch_add(1, std::memory_order_relaxed);
    } else {
        auto local_promise        = std::make_shared<std::promise<InferenceResponse>>();
        async_req->shared_promise = local_promise;
        future = local_promise->get_future().share();

        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (request_queue_.size() >= config_.max_queue_size) {
            if (!handleBackpressure(lock)) {
                stats_.total_rejected.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> tl(tracking_mutex_);
                active_requests_.erase(async_req->request_id);
                throw std::runtime_error("Request queue full, streaming request rejected");
            }
        }

        RequestQueueItem item;
        item.request = async_req;
        item.promise = std::move(local_promise);
        request_queue_.push_back(std::move(item));
        std::push_heap(request_queue_.begin(), request_queue_.end());
        stats_.total_submitted.fetch_add(1, std::memory_order_relaxed);
        queue_cv_.notify_one();
    }

    spdlog::debug("Submitted streaming inference request {} (priority={}, via_pool={})",
                  async_req->request_id, priority, (shared_pool_ != nullptr));

    spdlog::info(
        "AsyncInferenceEngine::submitStreaming queued: request_id={} priority={} via_pool={}",
        async_req->request_id,
        priority,
        (shared_pool_ != nullptr));

    return InferenceHandle(async_req->request_id, future, async_req->cancel_token);
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
    
    spdlog::info(
        "AsyncInferenceEngine::submitRAG start: docs={} top_k={} max_context_tokens={} response_budget_tokens={} request_max_tokens={} priority={} rag_priority={}",
        rag_context.documents.size(),
        rag_context.top_k,
        rag_context.max_context_tokens,
        rag_context.response_budget_tokens,
        request.max_tokens,
        priority,
        rag_priority);
    
    // Store RAG context in metadata for worker to use
    rag_request.metadata["rag_enabled"] = true;
    rag_request.metadata["num_documents"] = rag_context.documents.size();

    // Build structured prompt: use context_template when provided, otherwise
    // fall back to XML-tag format that most instruction-tuned models handle well.
    std::ostringstream oss = {};
    const bool use_custom_template = !rag_context.context_template.empty();

    if (!use_custom_template) {
        oss << "<system>\n"
            << "You are a helpful assistant. Answer the user's question based "
               "solely on the provided context documents. If the answer cannot "
               "be determined from the context, say so.\n"
            << "</system>\n\n";
        oss << "<context>\n";
        int doc_idx = 1;
        for (const auto& doc : rag_context.documents) {
            oss << "<document index=\"" << doc_idx++ << "\"";
            if (!doc.source.empty()) {
                oss << " source=\"" << doc.source << "\"";
            }
            if (doc.relevance_score > 0.0f) {
                oss << " relevance=\"" << doc.relevance_score << "\"";
            }
            oss << ">\n" << doc.content << "\n</document>\n";
        }
        oss << "</context>\n\n";
        oss << "<question>" << rag_context.query << "</question>\n\n";
        oss << "<answer>";
    } else {
        // Custom template: substitute {{CONTEXT}} and {{QUERY}} placeholders.
        std::string tmpl = rag_context.context_template;

        std::ostringstream context_block = {};
        for (size_t i = 0; i < rag_context.documents.size(); ++i) {
            const auto& doc = rag_context.documents[i];
            context_block << "[" << (i + 1) << "] ";
            if (!doc.source.empty()) {
              context_block << "(" << doc.source << ") ";
            }
            context_block << doc.content;
            if (i + 1 < rag_context.documents.size()) {
              context_block << "\n\n";
            }
        }

        auto replaceAll = [](std::string s, const std::string& from, const std::string& to) {
            size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos) {
                s.replace(pos, from.size(), to);
                pos += to.size();
            }
            return s;
        };
        tmpl = replaceAll(tmpl, "{{CONTEXT}}", context_block.str());
        tmpl = replaceAll(tmpl, "{{QUERY}}", rag_context.query);
        oss << tmpl;
    }

    rag_request.prompt = oss.str();
    
    auto handle = submit(rag_request, rag_priority);
    spdlog::info(
        "AsyncInferenceEngine::submitRAG queued: request_id={}",
        handle.requestId());
    return handle;
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
    stats_.total_cancelled.fetch_add(1, std::memory_order_relaxed);
    
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

    stats["total_dedup_cache_hits"] = stats_.total_dedup_cache_hits.load();
    stats["total_dedup_cache_misses"] = stats_.total_dedup_cache_misses.load();

    // tokens/sec: total tokens generated divided by elapsed wall-clock time.
    // THREAD-SAFETY: Protect engine_start_time_ read with mutex to prevent data race
    stats["total_tokens_generated"] = stats_.total_tokens_generated.load();
    double elapsed_s = {};
    {
        std::lock_guard<std::mutex> lock(stats_time_mutex_);
        elapsed_s = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - engine_start_time_).count();
    }
    if (elapsed_s > 0.0) {
        stats["tokens_per_second"] =
            static_cast<double>(stats_.total_tokens_generated.load()) / elapsed_s;
    }

    // p99 latency from per-request samples.
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        if (!latency_samples_.empty()) {
            std::vector<double> sorted(latency_samples_.begin(), latency_samples_.end());
            std::sort(sorted.begin(), sorted.end());
            size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
            stats["p99_latency_ms"] =
                sorted[std::min(p99_idx, sorted.size() - 1)];
        }
    }
    
    return stats;
}

void AsyncInferenceEngine::waitForCompletion() {
    spdlog::info("Waiting for all pending inference requests to complete...");
    
    // Use condition variable for efficient waiting with timeout instead of polling with sleep
    std::unique_lock<std::mutex> lock(queue_mutex_);
    using namespace std::chrono_literals;
    const auto timeout = std::chrono::seconds(300); // 5 minute timeout
    if (!queue_cv_.wait_for(lock, timeout, [this] { return request_queue_.empty(); })) {
        spdlog::warn("waitForCompletion: timeout waiting for request queue to empty");
    }
    
    spdlog::info("All inference requests completed");
}

void AsyncInferenceEngine::shutdown() {
    if (!running_.load(std::memory_order_acquire)) {
        return;  // Already shutdown
    }
    
    spdlog::info("Shutting down AsyncInferenceEngine...");
    
    // Signal workers and timeout monitor to stop
    running_.store(false, std::memory_order_release);
    queue_cv_.notify_all();
    
    // Wait for workers to finish with timeout
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            if (!themis::utils::joinThreadWithin(worker)) {
                spdlog::warn("Worker thread did not join within timeout, continuing shutdown");
            }
        }
    }
    workers_.clear();

    // Wait for timeout monitor
    if (timeout_thread_.joinable()) {
        if (!themis::utils::joinThreadWithin(timeout_thread_)) {
            spdlog::warn("timeout_thread_ did not join within timeout, continuing shutdown");
        }
    }
    
    spdlog::info("AsyncInferenceEngine shutdown complete");
}

// ═══════════════════════════════════════════════════════════
// Private Methods
// ═══════════════════════════════════════════════════════════

void AsyncInferenceEngine::workerLoop([[maybe_unused]] size_t worker_id) {
    spdlog::info("Inference worker {} started", worker_id);
    
    while (running_.load(std::memory_order_acquire)) {
        RequestQueueItem item;
        
        // Wait for work with timeout
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            using namespace std::chrono_literals;
            const auto timeout = std::chrono::seconds(30); // 30 second timeout
            
            if (!queue_cv_.wait_for(lock, timeout, [this] {
                return !request_queue_.empty() || !running_.load(std::memory_order_acquire);
            })) {
                spdlog::warn("Worker {}: timeout waiting for work, checking running flag", worker_id);
            }
            
            if (!running_.load(std::memory_order_acquire) && request_queue_.empty()) {
                break;  // Shutdown
            }
            
            if (request_queue_.empty()) {
                continue;
            }
            
            // Get highest priority request
            std::pop_heap(request_queue_.begin(), request_queue_.end());
            item = std::move(request_queue_.back());
            request_queue_.pop_back();
        }
        
        
        // Check if cancelled
        if (item.request->cancel_token->load(std::memory_order_acquire)) {
            spdlog::debug("Skipping cancelled request: {}", 
                         item.request->request_id);
            
            // Fire the completion callback so that streaming requests
            // receive the is_final=true sentinel even when the request is
            // cancelled while still queued (never dispatched to a plugin).
            if ([[maybe_unused]] item.request->callback) {
                try { item.request->callback([[maybe_unused]] InferenceResponse{}); } catch (...) {}
            }

            // Set exception in promise — guard against double-resolve (timeout
            // monitor may have already resolved it).
            if (item.promise) {
                try {
                    item.promise->set_exception(
                        std::make_exception_ptr(std::runtime_error("Request cancelled"))
                    );
                } catch (...) { /* Promise already satisfied; ignore. */ }
            }
            
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
            stats_.total_completed.fetch_add(1, std::memory_order_relaxed);
            
            // Deliver result
            if ([[maybe_unused]] item.request->callback) {
                // Call callback on worker thread
                item.request->callback([[maybe_unused]] response);
            }
            // Guard against double-resolve: timeout monitor may have already
            // resolved the promise while the plugin was still running.
            if (item.promise) {
                try { item.promise->set_value(response); } catch (...) { /* Promise already satisfied; ignore. */ }
            }
            
            spdlog::debug("Worker {} completed request {} in {:.1f}ms",
                         worker_id, item.request->request_id,
                         response.inference_time_ms);
            
        } catch (const std::exception& e) {
            spdlog::error("Worker {} failed to process request {}: {}",
                         worker_id, item.request->request_id, e.what());
            
            // Set exception in promise — guard against double-resolve.
            if (item.promise) {
                try { item.promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied; ignore. */ }
            }
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
    auto deadline = request.deadline;
    const bool streaming_mode = static_cast<bool>([[maybe_unused]] effective_request.stream_callback);

    if (streaming_mode) {
        spdlog::info(
            "AsyncInferenceEngine::processRequest streaming start: request_id={} prompt_len={} priority={}",
            request.request_id,
            effective_request.prompt.size(),
            request.priority);
    }

    if ([[maybe_unused]] effective_request.stream_callback) {
        // Wrap the original callback: stop streaming when cancelled/timed-out.
        auto original_cb = std::move([[maybe_unused]] effective_request.stream_callback);
        effective_request.stream_callback = [original_cb, 
            cancel_token = request.cancel_token,  // Direct capture to extend lifetime
            deadline]
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

    // Check deduplication cache (skip for streaming requests as they cannot be cached)
    if ([[maybe_unused]] dedup_cache_ && !effective_request.stream_callback) {
        auto cached = dedup_cache_->get(effective_request.prompt);
        if (cached.has_value()) {
            stats_.total_dedup_cache_hits.fetch_add(1, std::memory_order_relaxed);
            spdlog::debug("Dedup cache hit for request {}", request.request_id);
            // Protect concurrent metadata access with a lock
            std::lock_guard<std::mutex> meta_lock(cache_meta_mutex_);
            cached->cache_hit = true;
            cached->metadata["async"] = true;
            cached->metadata["queue_time_ms"] = queue_time;
            cached->metadata["request_id"] = request.request_id;
            cached->metadata["priority"] = request.priority;
            return *cached;
        }
        stats_.total_dedup_cache_misses.fetch_add(1, std::memory_order_relaxed);
    }

    // Apply prompt safety policy (prompt injection mitigation).
    // Take a local snapshot of the policy pointer so that a concurrent
    // setPromptPolicy(nullptr) call does not race with the apply() invocation.
    // Redact rules modify the prompt in-place; block rules return an error
    // response immediately without invoking the plugin.
    std::shared_ptr<PromptPolicy> policy_snapshot;
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        policy_snapshot = prompt_policy_;
    }
    if (policy_snapshot) {
        auto policy_result = policy_snapshot->apply(effective_request.prompt);
        if (!policy_result.allowed) {
            spdlog::warn(
                "AsyncInferenceEngine: request {} blocked by prompt policy rule '{}'",
                request.request_id, policy_result.rule_name);
            InferenceResponse blocked;
            blocked.request_id = request.request_id;
            blocked.metadata["async"] = true;
            blocked.metadata["blocked"] = true;
            blocked.metadata["blocked_rule"] = policy_result.rule_name;
            blocked.metadata["blocked_reason"] = policy_result.reason;
            blocked.metadata["queue_time_ms"] = queue_time;
            blocked.metadata["request_id"] = request.request_id;
            return blocked;
        }
        // Apply any redactions to the effective prompt
        effective_request.prompt = policy_result.sanitized_prompt;
    }

    // Grab a snapshot of the active plugin under a shared (read) lock so that a
    // concurrent swapPlugin() call does not race with the generate() invocation.
    std::shared_ptr<ILLMPlugin> plugin_snapshot;
    {
        std::shared_lock<std::shared_mutex> lock(plugin_mutex_);
        plugin_snapshot = active_plugin_;
    }

    if (!plugin_snapshot) {
        InferenceResponse failed;
        failed.request_id = request.request_id;
        failed.model_id = request.request.model_id;
        failed.success = false;
        failed.error_message = "Plugin cannot be null";
        failed.metadata["async"] = true;
        failed.metadata["blocked"] = true;
        failed.metadata["error"] = "Plugin cannot be null";
        failed.metadata["queue_time_ms"] = queue_time;
        return failed;
    }

    // Call plugin (blocking inference)
    InferenceResponse response = plugin_snapshot->generate(effective_request);

    if (streaming_mode) {
        spdlog::info(
            "AsyncInferenceEngine::processRequest streaming complete: request_id={} tokens_generated={} inference_time_ms={:.2f}",
            request.request_id,
            response.tokens_generated,
            response.inference_time_ms);
    }
    
    // Store in deduplication cache (skip for streaming requests)
    if ([[maybe_unused]] dedup_cache_ && !effective_request.stream_callback) {
        dedup_cache_->put(effective_request.prompt, response);
    }
    
    // Add metadata
    response.metadata["async"] = true;
    response.metadata["queue_time_ms"] = queue_time;
    response.metadata["request_id"] = request.request_id;
    response.metadata["priority"] = request.priority;
    
    // Update stats
    stats_.total_inference_time_ms.fetch_add(response.inference_time_ms);
    stats_.total_tokens_generated.fetch_add(
        static_cast<size_t>(response.tokens_generated), std::memory_order_relaxed);

    // Record per-request latency sample for p99 computation.
    if (response.inference_time_ms > 0.0) {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        latency_samples_.push_back(response.inference_time_ms);
        if (latency_samples_.size() > 10000) {
            latency_samples_.pop_front();  // O(1) removal via deque
        }
    }
    
    return response;
}

void AsyncInferenceEngine::setDedupCache(std::shared_ptr<LLMResponseCache> cache) {
    dedup_cache_ = std::move(cache);
}

LLMResponseCache::CacheStatistics AsyncInferenceEngine::getDedupCacheStats() const {
    if (dedup_cache_) {
        return dedup_cache_->getStatistics();
    }
    return LLMResponseCache::CacheStatistics{};
}

void AsyncInferenceEngine::swapPlugin(std::shared_ptr<ILLMPlugin> new_plugin) {
    if (!new_plugin) {
        throw std::invalid_argument("New plugin cannot be null");
    }
    std::unique_lock<std::shared_mutex> lock(plugin_mutex_);
    owned_plugin_ = std::move(new_plugin);
    plugin_ = owned_plugin_.get();
    active_plugin_ = owned_plugin_;
    // Ensure all writes are visible before unlocking
    std::atomic_thread_fence(std::memory_order_release);
    spdlog::info("AsyncInferenceEngine: plugin hot-swapped");
}

void AsyncInferenceEngine::setPromptPolicy(std::shared_ptr<PromptPolicy> policy) {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    prompt_policy_ = std::move(policy);
}

std::string AsyncInferenceEngine::generateRequestId() {
    static std::atomic<uint64_t> counter{0};
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    uint64_t id = counter.fetch_add(1);
    
    std::ostringstream oss = {};
    oss << "inf_" << timestamp << "_" << id;
    return oss.str();
}

bool AsyncInferenceEngine::handleBackpressure(std::unique_lock<std::mutex>& lock) {
    // Already have lock on queue_mutex_
    
    switch (config_.backpressure) {
        case Config::BackpressurePolicy::BLOCK:
            // Wait for space with timeout (releases lock while waiting)
            {
                using namespace std::chrono_literals;
                const auto timeout = std::chrono::seconds(30); // 30 second timeout
                if (!queue_cv_.wait_for(lock, timeout, [this] {
                    return request_queue_.size() < config_.max_queue_size ||
                           !running_.load();
                })) {
                    spdlog::warn("Backpressure BLOCK: timeout waiting for queue space");
                }
            }
            return running_.load();
            
        case Config::BackpressurePolicy::DROP_OLDEST:
            // Find the queued request with the lowest priority and drop it
            // to make room for the incoming (presumably higher-priority) request.
            if (!request_queue_.empty()) {
                auto min_it = std::min_element(
                    request_queue_.begin(), request_queue_.end(),
                    [](const RequestQueueItem& a, const RequestQueueItem& b) {
                        return a.request->priority < b.request->priority;
                    });

                spdlog::warn(
                    "Queue full, dropping lowest-priority request (id={}, priority={})",
                    min_it->request->request_id, min_it->request->priority);

                // Signal cancel token so any InferenceHandle for this request
                // is notified that it was dropped.
                min_it->request->cancel_token->store(true, std::memory_order_release);

                // Fulfil the promise so the caller's future doesn't block.
                try {
                    if (min_it->promise) {
                        min_it->promise->set_exception(
                            std::make_exception_ptr(
                                std::runtime_error("Request dropped: queue full")));
                    }
                } catch (...) {
                    THEMIS_WARN("async_inference_engine: unhandled exception caught");
                    // Promise may already be satisfied; ignore.
                }

                // Remove from the active-request tracking map.
                {
                    std::lock_guard<std::mutex> tracking_lock(tracking_mutex_);
                    active_requests_.erase(min_it->request->request_id);
                }

                // Erase from vector and restore heap invariant.
                // We swap with the last element and pop_back, then restore
                // the heap.  make_heap is O(n) but this path is only taken
                // when the queue is full, so the amortised overhead is low.
                if (min_it != request_queue_.end() - 1) {
                    std::iter_swap(min_it, request_queue_.end() - 1);
                }
                request_queue_.pop_back();
                std::make_heap(request_queue_.begin(), request_queue_.end());

                stats_.total_rejected.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            return false;
            
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
        if (req->deadline == zero_tp) {
          continue;
        }
        if (req->cancel_token->load(std::memory_order_acquire)) {
          continue;
        }

        if (now >= req->deadline) {
            spdlog::warn("Request {} exceeded per-request timeout, marking cancelled", id);
            req->cancel_token->store(true, std::memory_order_release);
            stats_.total_timed_out.fetch_add(1, std::memory_order_relaxed);
            stats_.total_cancelled.fetch_add(1, std::memory_order_relaxed);

            // Resolve the future immediately so handle.get() unblocks without
            // waiting for the in-flight plugin call to finish.
            if (req->shared_promise) {
                try {
                    req->shared_promise->set_exception(
                        std::make_exception_ptr(
                            std::runtime_error("Request timed out")));
                } catch (...) {
                    THEMIS_WARN("async_inference_engine: unhandled exception caught");
                    // Promise already satisfied; ignore.
                }
            }
        }
    }
}

} // namespace llm
} // namespace themis

