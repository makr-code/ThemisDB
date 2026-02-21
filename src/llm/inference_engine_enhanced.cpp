/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_engine_enhanced.cpp                      ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   79.0/100                                       ║
    • Total Lines:     955                                            ║
    • Open Issues:     TODOs: 3, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/inference_engine_enhanced.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════

InferenceEngineEnhanced::InferenceEngineEnhanced(const Config& config)
    : config_(config) {
    
    spdlog::info("Initializing Enhanced Inference Engine");
    spdlog::info("  Context Caching: {}", config_.enable_context_caching);
    spdlog::info("  Batch Processing: {}", config_.enable_batch_processing);
    spdlog::info("  Load Balancing: {}", config_.enable_load_balancing);
    spdlog::info("  Worker Threads: {}", config_.num_worker_threads);
    
    // Initialize prefix cache for context caching
    if (config_.enable_context_caching) {
        LLMPrefixCache::Config cache_config;
        cache_config.max_entries = config_.max_cache_entries;
        cache_config.similarity_threshold = config_.cache_similarity_threshold;
        cache_config.ttl_seconds = config_.cache_ttl_seconds;
        cache_config.enable_kv_caching = true;
        
        prefix_cache_ = std::make_unique<LLMPrefixCache>("inference_cache", cache_config);
        spdlog::info("  Prefix cache initialized with {} max entries", cache_config.max_entries);
    }
    
    // Initialize KV cache for paged attention
    PagedBlockManager::Config block_config;
    block_config.max_blocks = 4096;
    block_config.block_size_tokens = 16;
    block_manager_ = std::make_shared<PagedBlockManager>(block_config);
    
    PagedKVCache::Config kv_config;
    kv_config.enable_prefix_caching = config_.enable_context_caching;
    kv_config.num_blocks = 4096;
    kv_config.block_size = 16;
    
    kv_cache_ = std::make_shared<PagedKVCache>(kv_config, block_manager_);
    
    // Initialize batch scheduler if enabled
    if (config_.enable_batch_processing) {
        ContinuousBatchScheduler::SchedulerConfig sched_config;
        sched_config.max_batch_size = config_.max_batch_size;
        sched_config.max_tokens_per_batch = config_.max_tokens_per_batch;
        sched_config.enable_priority_scheduling = config_.enable_priority_scheduling;
        
        batch_scheduler_ = std::make_unique<ContinuousBatchScheduler>(
            sched_config, kv_cache_.get());
        spdlog::info("  Batch scheduler initialized (max batch size: {})", 
                     sched_config.max_batch_size);
    }
    
    spdlog::info("Enhanced Inference Engine initialized successfully");
}

InferenceEngineEnhanced::~InferenceEngineEnhanced() {
    shutdown();
}

// ═══════════════════════════════════════════════════════════
// Model Management
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::registerModel(
    const std::string& model_id,
    std::shared_ptr<ILLMPlugin> plugin
) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    ModelInfo info;
    info.model_id = model_id;
    info.plugin = plugin;
    info.is_available = true;
    
    models_[model_id] = info;
    
    spdlog::info("Registered model: {}", model_id);
}

void InferenceEngineEnhanced::unregisterModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    models_.erase(model_id);
    
    spdlog::info("Unregistered model: {}", model_id);
}

std::vector<std::string> InferenceEngineEnhanced::getAvailableModels() const {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    std::vector<std::string> available;
    for (const auto& [id, info] : models_) {
        if (info.is_available) {
            available.push_back(id);
        }
    }
    
    return available;
}

// ═══════════════════════════════════════════════════════════
// Inference Submission
// ═══════════════════════════════════════════════════════════

InferenceHandle InferenceEngineEnhanced::submit(const EnhancedInferenceRequest& request) {
    auto tracked = std::make_shared<TrackedRequest>();
    tracked->request = request;
    tracked->deadline = std::chrono::steady_clock::now() + request.timeout;
    
    auto future = tracked->promise.get_future().share();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Check queue capacity
        if (request_queue_.size() >= config_.max_queue_size) {
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.rejected_requests++;
            }
            throw std::runtime_error("Request queue full");
        }
        
        request_queue_.push(tracked);
        
        // Track request
        {
            std::lock_guard<std::mutex> req_lock(requests_mutex_);
            tracked_requests_[request.request_id] = tracked;
        }
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.total_requests++;
            stats_.current_queue_size = request_queue_.size();
        }
    }
    
    queue_cv_.notify_one();
    
    return InferenceHandle(request.request_id, future);
}

std::string InferenceEngineEnhanced::submitAsync(
    const EnhancedInferenceRequest& request,
    std::function<void(const InferenceResponse&)> callback
) {
    auto tracked = std::make_shared<TrackedRequest>();
    tracked->request = request;
    tracked->deadline = std::chrono::steady_clock::now() + request.timeout;
    tracked->callback = callback;
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (request_queue_.size() >= config_.max_queue_size) {
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.rejected_requests++;
            }
            throw std::runtime_error("Request queue full");
        }
        
        request_queue_.push(tracked);
        
        {
            std::lock_guard<std::mutex> req_lock(requests_mutex_);
            tracked_requests_[request.request_id] = tracked;
        }
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.total_requests++;
            stats_.current_queue_size = request_queue_.size();
        }
    }
    
    queue_cv_.notify_one();
    
    return request.request_id;
}

// ═══════════════════════════════════════════════════════════
// Request Management
// ═══════════════════════════════════════════════════════════

bool InferenceEngineEnhanced::cancel(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    auto it = tracked_requests_.find(request_id);
    if (it == tracked_requests_.end()) {
        return false;
    }
    
    // Set exception in promise
    try {
        it->second->promise.set_exception(
            std::make_exception_ptr(std::runtime_error("Request cancelled"))
        );
    } catch (...) {
        // Promise already satisfied
    }
    
    tracked_requests_.erase(it);
    
    spdlog::debug("Cancelled request: {}", request_id);
    return true;
}

bool InferenceEngineEnhanced::reprioritize(const std::string& request_id, int new_priority) {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    auto it = tracked_requests_.find(request_id);
    if (it == tracked_requests_.end()) {
        return false;
    }
    
    it->second->request.priority = new_priority;
    
    spdlog::debug("Reprioritized request {} to priority {}", request_id, new_priority);
    return true;
}

// ═══════════════════════════════════════════════════════════
// Cache Management
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::clearCache() {
    if (prefix_cache_) {
        prefix_cache_->clear();
        spdlog::info("Cleared inference cache");
    }
}

void InferenceEngineEnhanced::prewarmCache(const std::vector<std::string>& common_prompts) {
    if (!prefix_cache_ || !config_.enable_context_caching) {
        return;
    }
    
    spdlog::info("Prewarming cache with {} common prompts", common_prompts.size());
    
    // TODO: In production, implement actual cache prewarming:
    // 1. Use embedding model to compute embeddings for each prompt
    //    auto embedding = embedding_model_->encode(prompt);
    // 2. Pre-compute KV cache for frequent prompts
    //    auto kv_cache = computeKVCache(prompt);
    // 3. Store in prefix cache for fast retrieval
    //    prefix_cache_->store(prompt, kv_cache);
    
    for (const auto& prompt : common_prompts) {
        spdlog::debug("  Prewarming: {}", prompt.substr(0, 50));
        // Actual implementation would pre-process these prompts
    }
}

// ═══════════════════════════════════════════════════════════
// Statistics and Monitoring
// ═══════════════════════════════════════════════════════════

InferenceEngineEnhanced::Statistics InferenceEngineEnhanced::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto stats = stats_;
    
    // Calculate cache hit rate
    size_t total_cache_ops = stats_.cache_hits + stats_.cache_misses;
    if (total_cache_ops > 0) {
        stats.cache_hit_rate = static_cast<double>(stats_.cache_hits) / total_cache_ops;
    }
    
    // Calculate load balance fairness (1.0 = perfectly balanced)
    if (!stats_.requests_per_model.empty()) {
        double mean = 0.0;
        for (const auto& [model, count] : stats_.requests_per_model) {
            mean += count;
        }
        mean /= stats_.requests_per_model.size();
        
        double variance = 0.0;
        for (const auto& [model, count] : stats_.requests_per_model) {
            variance += (count - mean) * (count - mean);
        }
        variance /= stats_.requests_per_model.size();
        
        // Fairness: 1 - (stddev / mean), closer to 1 is more fair
        if (mean > 0) {
            stats.load_balance_fairness = 1.0 - (std::sqrt(variance) / mean);
        }
    }
    
    // Calculate percentiles
    if (!latency_samples_.empty()) {
        std::vector<double> sorted = latency_samples_;
        std::sort(sorted.begin(), sorted.end());
        
        size_t p95_idx = static_cast<size_t>(sorted.size() * 0.95);
        size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
        
        stats.p95_latency_ms = sorted[std::min(p95_idx, sorted.size() - 1)];
        stats.p99_latency_ms = sorted[std::min(p99_idx, sorted.size() - 1)];
    }
    
    return stats;
}

json InferenceEngineEnhanced::getDetailedMetrics() const {
    auto stats = getStatistics();
    
    json metrics;
    
    // Cache metrics
    metrics["cache"]["hit_rate"] = stats.cache_hit_rate;
    metrics["cache"]["hits"] = stats.cache_hits;
    metrics["cache"]["misses"] = stats.cache_misses;
    metrics["cache"]["tokens_saved"] = stats.tokens_saved_by_cache;
    
    // Batch metrics
    metrics["batch"]["total_batches"] = stats.total_batches;
    metrics["batch"]["avg_size"] = stats.avg_batch_size;
    metrics["batch"]["max_size"] = stats.max_batch_size_seen;
    metrics["batch"]["throughput_improvement"] = stats.throughput_improvement;
    
    // Queue metrics
    metrics["queue"]["total_requests"] = stats.total_requests;
    metrics["queue"]["completed"] = stats.completed_requests;
    metrics["queue"]["timed_out"] = stats.timed_out_requests;
    metrics["queue"]["rejected"] = stats.rejected_requests;
    metrics["queue"]["current_size"] = stats.current_queue_size;
    
    // Load balancing
    metrics["load_balance"]["fairness"] = stats.load_balance_fairness;
    metrics["load_balance"]["requests_per_model"] = stats.requests_per_model;
    metrics["load_balance"]["avg_latency_per_model"] = stats.avg_latency_per_model;
    
    // Performance
    metrics["performance"]["avg_latency_ms"] = stats.avg_latency_ms;
    metrics["performance"]["p95_latency_ms"] = stats.p95_latency_ms;
    metrics["performance"]["p99_latency_ms"] = stats.p99_latency_ms;
    metrics["performance"]["tokens_per_second"] = stats.tokens_per_second;
    
    return metrics;
}

// ═══════════════════════════════════════════════════════════
// Lifecycle
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::start() {
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    
    // Start worker threads
    for (size_t i = 0; i < config_.num_worker_threads; ++i) {
        worker_threads_.emplace_back(&InferenceEngineEnhanced::workerLoop, this, i);
    }
    
    // Start timeout monitor
    timeout_thread_ = std::thread(&InferenceEngineEnhanced::timeoutMonitorLoop, this);
    
    spdlog::info("Enhanced Inference Engine started with {} workers", 
                 config_.num_worker_threads);
}

void InferenceEngineEnhanced::shutdown() {
    if (!running_.load()) {
        return;
    }
    
    spdlog::info("Shutting down Enhanced Inference Engine...");
    
    running_.store(false);
    queue_cv_.notify_all();
    
    // Join worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // Join timeout thread
    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }
    
    spdlog::info("Enhanced Inference Engine shutdown complete");
}

bool InferenceEngineEnhanced::isRunning() const {
    return running_.load();
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Worker Loop
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::workerLoop(size_t worker_id) {
    spdlog::debug("Worker {} started", worker_id);
    
    while (running_.load()) {
        std::vector<std::shared_ptr<TrackedRequest>> batch;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for work or batch timeout
            auto wait_until = std::chrono::steady_clock::now() + 
                             std::chrono::milliseconds(config_.batch_timeout_ms);
            
            queue_cv_.wait_until(lock, wait_until, [this] {
                return !request_queue_.empty() || !running_.load();
            });
            
            if (!running_.load() && request_queue_.empty()) {
                break;
            }
            
            // Form batch
            if (config_.enable_batch_processing) {
                batch = formBatch();
            } else if (!request_queue_.empty()) {
                batch.push_back(request_queue_.front());
                request_queue_.pop();
            }
            
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.current_queue_size = request_queue_.size();
            }
        }
        
        if (!batch.empty()) {
            processBatch(batch);
        }
    }
    
    spdlog::debug("Worker {} stopped", worker_id);
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Timeout Monitoring
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::timeoutMonitorLoop() {
    spdlog::debug("Timeout monitor started");
    
    while (running_.load()) {
        checkAndHandleTimeouts();
        
        // Brief sleep to avoid busy-waiting while still being responsive
        // NOTE: This polling approach is acceptable for monitoring threads.
        // For high-frequency systems, consider using condition variables or event-driven timeouts.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    spdlog::debug("Timeout monitor stopped");
}

void InferenceEngineEnhanced::checkAndHandleTimeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> timed_out;
    
    {
        std::lock_guard<std::mutex> lock(requests_mutex_);
        
        for (auto& [id, tracked] : tracked_requests_) {
            if (now >= tracked->deadline) {
                timed_out.push_back(id);
            }
        }
    }
    
    // Handle timed out requests
    for (const auto& id : timed_out) {
        spdlog::warn("Request {} timed out", id);
        
        std::lock_guard<std::mutex> lock(requests_mutex_);
        auto it = tracked_requests_.find(id);
        if (it != tracked_requests_.end()) {
            try {
                InferenceResponse timeout_response;
                timeout_response.text = "";
                timeout_response.model_id = "";
                
                if (it->second->callback) {
                    it->second->callback(timeout_response);
                }
                
                it->second->promise.set_value(timeout_response);
            } catch (...) {
                // Promise already satisfied
            }
            
            tracked_requests_.erase(it);
            recordRequestTimeout();
        }
    }
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Batch Processing
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::processBatch(
    const std::vector<std::shared_ptr<TrackedRequest>>& batch
) {
    spdlog::debug("Processing batch of {} requests", batch.size());
    
    auto batch_start = std::chrono::steady_clock::now();
    
    for (const auto& tracked : batch) {
        auto& req = tracked->request;
        
        try {
            auto req_start = std::chrono::steady_clock::now();
            
            // Check cache first
            std::optional<InferenceResponse> cached_response;
            if (config_.enable_context_caching && req.allow_caching) {
                cached_response = checkCache(req.base_request);
                
                if (cached_response) {
                    spdlog::debug("Cache hit for request {}", req.request_id);
                    
                    // Deliver cached response
                    if (tracked->callback) {
                        tracked->callback(*cached_response);
                    }
                    tracked->promise.set_value(*cached_response);
                    
                    auto req_end = std::chrono::steady_clock::now();
                    double latency = std::chrono::duration<double, std::milli>(
                        req_end - req_start).count();
                    
                    recordRequestCompletion(latency, "cache");
                    continue;
                }
            }
            
            // Select model for load balancing
            std::string model_id = selectModel(req);
            
            // Get model plugin
            std::shared_ptr<ILLMPlugin> plugin;
            {
                std::lock_guard<std::mutex> lock(models_mutex_);
                auto it = models_.find(model_id);
                if (it != models_.end()) {
                    plugin = it->second.plugin;
                }
            }
            
            if (!plugin) {
                throw std::runtime_error("No available model for request");
            }
            
            // Execute inference
            auto response = plugin->generate(req.base_request);
            
            // Update cache
            if (config_.enable_context_caching && req.allow_caching && !response.text.empty()) {
                updateCache(req.base_request, response);
            }
            
            // Deliver response
            if (tracked->callback) {
                tracked->callback(response);
            }
            tracked->promise.set_value(response);
            
            auto req_end = std::chrono::steady_clock::now();
            double latency = std::chrono::duration<double, std::milli>(
                req_end - req_start).count();
            
            recordRequestCompletion(latency, model_id);
            updateModelStats(model_id, latency, !response.text.empty());
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to process request {}: {}", req.request_id, e.what());
            
            InferenceResponse error_response;
            error_response.text = "Error: " + std::string(e.what());
            error_response.model_id = "";
            
            if (tracked->callback) {
                tracked->callback(error_response);
            }
            
            try {
                tracked->promise.set_value(error_response);
            } catch (...) {
                // Promise already satisfied
            }
        }
        
        // Remove from tracked requests
        {
            std::lock_guard<std::mutex> lock(requests_mutex_);
            tracked_requests_.erase(req.request_id);
        }
    }
    
    // Record batch completion
    recordBatchCompletion(batch.size());
    
    auto batch_end = std::chrono::steady_clock::now();
    double batch_time = std::chrono::duration<double, std::milli>(
        batch_end - batch_start).count();
    
    spdlog::debug("Batch of {} completed in {:.2f}ms", batch.size(), batch_time);
}

std::vector<std::shared_ptr<InferenceEngineEnhanced::TrackedRequest>> 
InferenceEngineEnhanced::formBatch() {
    // Already holding queue_mutex_
    
    std::vector<std::shared_ptr<TrackedRequest>> batch;
    size_t batch_tokens = 0;
    
    while (!request_queue_.empty() && 
           batch.size() < config_.max_batch_size) {
        
        auto req = request_queue_.front();
        
        // Check if we can add this request to batch
        if (!canAddToBatch(req, batch_tokens)) {
            break;
        }
        
        request_queue_.pop();
        batch.push_back(req);
        
        // Estimate tokens (rough estimate based on prompt length)
        batch_tokens += req->request.base_request.prompt.length() / 4;  // ~4 chars per token
    }
    
    return batch;
}

bool InferenceEngineEnhanced::canAddToBatch(
    const std::shared_ptr<TrackedRequest>& req,
    size_t current_batch_tokens
) {
    // Estimate tokens for this request
    size_t req_tokens = req->request.base_request.prompt.length() / 4;
    
    return (current_batch_tokens + req_tokens) <= config_.max_tokens_per_batch;
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Cache Operations
// ═══════════════════════════════════════════════════════════

std::optional<InferenceResponse> InferenceEngineEnhanced::checkCache(
    const InferenceRequest& request
) {
    if (!prefix_cache_) {
        return std::nullopt;
    }
    
    // Generate cache key
    std::string cache_key = generateCacheKey(request);
    
    // TODO: In production, compute embeddings for similarity-based cache lookup
    // Real implementation would:
    // 1. Use an embedding model (e.g., sentence-transformers, all-MiniLM-L6-v2)
    //    auto embedding = embedding_model_->encode(request.prompt);
    // 2. Perform similarity search in the prefix cache
    //    auto cached = prefix_cache_->findSimilar(embedding, similarity_threshold);
    // 
    // For now, use simple string-based exact matching with a placeholder embedding
    std::vector<float> dummy_embedding(128, 0.0f);  // Placeholder - not used in current implementation
    
    auto cached = prefix_cache_->get(cache_key, dummy_embedding);
    
    if (cached) {
        recordCacheHit(cached->token_ids.size());
        
        // Reconstruct response from cache
        InferenceResponse response;
        response.text = cached->prefix;  // Simplified - would be actual generated text
        response.tokens_prompt = static_cast<int>(cached->token_ids.size());
        response.cache_hit = true;
        
        return response;
    }
    
    recordCacheMiss();
    return std::nullopt;
}

void InferenceEngineEnhanced::updateCache(
    const InferenceRequest& request,
    const InferenceResponse& response
) {
    if (!prefix_cache_ || response.text.empty()) {
        return;
    }
    
    std::string cache_key = generateCacheKey(request);
    
    // TODO: In production, compute actual embeddings and KV cache
    std::vector<int> tokens;  // Would tokenize response
    std::vector<float> embedding(128, 0.0f);
    std::vector<float> kv_cache;  // Would extract from model
    
    prefix_cache_->put(cache_key, tokens, embedding, kv_cache);
}

std::string InferenceEngineEnhanced::generateCacheKey(const InferenceRequest& request) {
    // Generate SHA256 hash of request parameters
    std::ostringstream oss;
    oss << request.prompt;
    oss << "|" << request.max_tokens;
    oss << "|" << request.temperature;
    oss << "|" << request.top_p;
    
    std::string input = oss.str();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned char* result = SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
                                   input.length(), hash);
    
    // Check for SHA256 failure
    if (result == nullptr) {
        spdlog::error("SHA256 hash generation failed");
        // Fallback to simple hash
        return "cache_" + std::to_string(std::hash<std::string>{}(input));
    }
    
    std::ostringstream hash_str;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hash_str << std::hex << std::setw(2) << std::setfill('0') 
                 << static_cast<int>(hash[i]);
    }
    
    return hash_str.str();
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Load Balancing
// ═══════════════════════════════════════════════════════════

std::string InferenceEngineEnhanced::selectModel(const EnhancedInferenceRequest& request) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    // If specific model requested and available, use it
    if (!request.preferred_model_id.empty()) {
        auto it = models_.find(request.preferred_model_id);
        if (it != models_.end() && it->second.is_available) {
            return request.preferred_model_id;
        }
    }
    
    // Get available models
    std::vector<std::string> available;
    for (const auto& [id, info] : models_) {
        if (info.is_available) {
            available.push_back(id);
        }
    }
    
    if (available.empty()) {
        throw std::runtime_error("No available models");
    }
    
    // Apply load balancing strategy
    switch (config_.load_balance_strategy) {
        case Config::LoadBalanceStrategy::ROUND_ROBIN: {
            // Use atomic fetch_add for thread-safe round-robin
            size_t index = round_robin_index_.fetch_add(1, std::memory_order_relaxed) % available.size();
            return available[index];
        }
            
        case Config::LoadBalanceStrategy::LEAST_LOADED: {
            auto least_loaded = available[0];
            size_t min_load = models_[least_loaded].active_requests;
            
            for (const auto& model_id : available) {
                auto load = models_[model_id].active_requests;
                if (load < min_load) {
                    min_load = load;
                    least_loaded = model_id;
                }
            }
            return least_loaded;
        }
        
        case Config::LoadBalanceStrategy::RESPONSE_TIME_WEIGHTED: {
            auto fastest = available[0];
            double min_time = models_[fastest].avg_response_time_ms;
            
            for (const auto& model_id : available) {
                auto time = models_[model_id].avg_response_time_ms;
                if (time < min_time && time > 0) {
                    min_time = time;
                    fastest = model_id;
                }
            }
            return fastest;
        }
    }
    
    return available[0];
}

void InferenceEngineEnhanced::updateModelStats(
    const std::string& model_id,
    double latency_ms,
    bool success
) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        auto& info = it->second;
        
        if (success) {
            info.total_requests++;
            
            // Update moving average of response time
            if (info.avg_response_time_ms == 0.0) {
                info.avg_response_time_ms = latency_ms;
            } else {
                info.avg_response_time_ms = 
                    0.9 * info.avg_response_time_ms + 0.1 * latency_ms;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Statistics
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::recordCacheHit(size_t tokens_saved) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.cache_hits++;
    stats_.tokens_saved_by_cache += tokens_saved;
}

void InferenceEngineEnhanced::recordCacheMiss() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.cache_misses++;
}

void InferenceEngineEnhanced::recordBatchCompletion(size_t batch_size) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_batches++;
    
    // Update moving average
    if (stats_.avg_batch_size == 0.0) {
        stats_.avg_batch_size = batch_size;
    } else {
        stats_.avg_batch_size = 
            0.95 * stats_.avg_batch_size + 0.05 * batch_size;
    }
    
    if (batch_size > stats_.max_batch_size_seen) {
        stats_.max_batch_size_seen = batch_size;
    }
}

void InferenceEngineEnhanced::recordRequestCompletion(
    double latency_ms,
    const std::string& model_id
) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.completed_requests++;
    
    // Update latency stats
    latency_samples_.push_back(latency_ms);
    if (latency_samples_.size() > 10000) {
        latency_samples_.erase(latency_samples_.begin());
    }
    
    stats_.avg_latency_ms = 
        std::accumulate(latency_samples_.begin(), latency_samples_.end(), 0.0) / 
        latency_samples_.size();
    
    // Update per-model stats
    stats_.requests_per_model[model_id]++;
    
    auto& model_latency = stats_.avg_latency_per_model[model_id];
    if (model_latency == 0.0) {
        model_latency = latency_ms;
    } else {
        model_latency = 0.9 * model_latency + 0.1 * latency_ms;
    }
}

void InferenceEngineEnhanced::recordRequestTimeout() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.timed_out_requests++;
}

// ═══════════════════════════════════════════════════════════
// Helper Methods
// ═══════════════════════════════════════════════════════════

std::string InferenceEngineEnhanced::generateRequestId() {
    auto count = request_counter_.fetch_add(1);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::ostringstream oss;
    oss << "req_" << timestamp << "_" << count;
    return oss.str();
}

} // namespace llm
} // namespace themis
