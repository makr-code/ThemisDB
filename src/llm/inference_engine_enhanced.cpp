/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_engine_enhanced.cpp                      ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:17:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   76.0/100                                       ║
    • Total Lines:     1766                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fe135d5215  2026-04-13  feat(llm): Speculative Decoding for Latency Reduction — v... ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • d1f0cf3ca5  2026-03-19  fix(llm): address all PR review issues - sentinel deliver... ║
    • cdc9749757  2026-03-18  Changes before error encountered        ║
    • c3fa684101  2026-03-11  fix(llm): audit pass 2 - fix generated_text, prompt-key c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/inference_engine_enhanced.h"
#include "llm/model_router.h"
#include "llm/shared_worker_pool.h"
#include "llm/speculative_decoder.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <sstream>

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

    // Initialise speculative decoder when requested.
    if (config_.enable_speculative_decoding) {
        SpeculativeDecoder::Config sd_cfg;
        sd_cfg.k = config_.speculative_draft_tokens;
        speculative_decoder_ = std::make_unique<SpeculativeDecoder>(sd_cfg);

        if (config_.speculative_draft_model_id.empty()) {
            spdlog::info("Speculative decoder initialised: k={}, draft_model=auto-discover "
                         "(call setAdapterRegistry() with a DRAFT adapter registered to "
                         "enable family-based draft model selection)",
                         sd_cfg.k);
        } else {
            spdlog::info("Speculative decoder initialised: k={}, draft_model={}",
                         sd_cfg.k, config_.speculative_draft_model_id);
        }
    }
}

InferenceEngineEnhanced::InferenceEngineEnhanced(
    const Config& config,
    std::shared_ptr<SharedWorkerPool> pool
) : InferenceEngineEnhanced(config) {
    if (!pool) {
        throw std::invalid_argument("SharedWorkerPool cannot be null");
    }
    shared_pool_ = std::move(pool);
    spdlog::info("Enhanced Inference Engine will use shared worker pool ({} threads)",
                 shared_pool_->numThreads());
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
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        ModelInfo info;
        info.model_id = model_id;
        info.plugin = plugin;
        info.is_available = true;
        models_[model_id] = info;
    }
    spdlog::info("Registered model: {}", model_id);

    // Pre-load any LoRA adapters that are registered for this model (or for
    // all models).  The lock ordering mirrors loadLoRAAdapter(): acquire
    // lora_adapters_mutex_ first, models_mutex_ never held at the same time.
    // Note: if an adapter is concurrently unloaded between snapshotting
    // lora_adapters_ and calling plugin->loadLoRA(), the plugin will attempt
    // to load a path that the engine no longer tracks.  The plugin's
    // loadLoRA() is expected to handle unknown/missing paths gracefully
    // (returning false), which is the correct degraded-mode behaviour.
    std::vector<std::tuple<std::string, std::string, float>> to_load;
    {
        std::lock_guard<std::mutex> lock(lora_adapters_mutex_);
        for (const auto& [aid, entry] : lora_adapters_) {
            if (entry.model_id.empty() || entry.model_id == model_id) {
                to_load.emplace_back(aid, entry.path, entry.scale);
            }
        }
    }
    for (const auto& [aid, apath, ascale] : to_load) {
        if (plugin && plugin->loadLoRA(aid, apath, ascale)) {
            spdlog::info("Pre-loaded LoRA adapter '{}' on newly registered model '{}'",
                         aid, model_id);
        }
    }
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

void InferenceEngineEnhanced::swapModel(
    const std::string& model_id,
    std::shared_ptr<ILLMPlugin> new_plugin
) {
    if (!new_plugin) {
        throw std::invalid_argument("New plugin cannot be null");
    }
    std::lock_guard<std::mutex> lock(models_mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        throw std::invalid_argument("Model not registered: " + model_id);
    }
    it->second.plugin = std::move(new_plugin);
    spdlog::info("Hot-swapped plugin for model: {}", model_id);
}

// ═══════════════════════════════════════════════════════════
// LoRA Adapter Hot-Loading
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::setAdapterRegistry(
    std::shared_ptr<AdapterRegistry> registry
) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    adapter_registry_ = std::move(registry);
    spdlog::info("InferenceEngineEnhanced: adapter registry attached for DRAFT model discovery");
}

void InferenceEngineEnhanced::loadLoRAAdapter(
    const std::string& adapter_id,
    const std::string& path,
    float scale,
    const std::string& model_id
) {
    if (adapter_id.empty()) {
        throw std::invalid_argument("adapter_id must not be empty");
    }
    if (path.empty()) {
        throw std::invalid_argument("path must not be empty");
    }

    // Register adapter metadata first (under lora_adapters_mutex_)
    {
        std::lock_guard<std::mutex> lock(lora_adapters_mutex_);
        lora_adapters_[adapter_id] = LoRAAdapterEntry{path, scale, model_id};
    }

    // Propagate to model plugin(s) so the adapter is pre-loaded before any
    // request arrives.  Iterate under models_mutex_ to get a stable snapshot
    // of registered plugins; release the lock before calling into the plugin
    // to avoid holding two locks at once.
    std::vector<std::pair<std::string, std::shared_ptr<ILLMPlugin>>> targets;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        for (const auto& [id, info] : models_) {
            if (model_id.empty() || id == model_id) {
                targets.emplace_back(id, info.plugin);
            }
        }
    }

    for (const auto& [mid, plugin] : targets) {
        if (plugin) {
            if (plugin->loadLoRA(adapter_id, path, scale)) {
                spdlog::info("Hot-loaded LoRA adapter '{}' on model '{}'",
                             adapter_id, mid);
            } else {
                spdlog::warn("Hot-load of LoRA adapter '{}' on model '{}' "
                             "returned false", adapter_id, mid);
            }
        }
    }

    if (targets.empty()) {
        spdlog::info("Hot-loaded LoRA adapter '{}' (no matching models registered "
                     "yet; will be applied when a model is registered)", adapter_id);
    }
}

bool InferenceEngineEnhanced::unloadLoRAAdapter(
    const std::string& adapter_id,
    const std::string& model_id
) {
    // Remove registration first
    {
        std::lock_guard<std::mutex> lock(lora_adapters_mutex_);
        auto it = lora_adapters_.find(adapter_id);
        if (it == lora_adapters_.end()) {
            spdlog::debug("unloadLoRAAdapter: '{}' not registered", adapter_id);
            return false;
        }
        lora_adapters_.erase(it);
    }

    // Propagate unload to model plugin(s)
    std::vector<std::pair<std::string, std::shared_ptr<ILLMPlugin>>> targets;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        for (const auto& [id, info] : models_) {
            if (model_id.empty() || id == model_id) {
                targets.emplace_back(id, info.plugin);
            }
        }
    }

    for (const auto& [mid, plugin] : targets) {
        if (plugin) {
            if (plugin->unloadLoRA(adapter_id)) {
                spdlog::info("Hot-unloaded LoRA adapter '{}' from model '{}'",
                             adapter_id, mid);
            } else {
                spdlog::debug("LoRA adapter '{}' was not loaded on model '{}'",
                              adapter_id, mid);
            }
        }
    }

    return true;
}

std::vector<LoRAInfo> InferenceEngineEnhanced::getLoadedLoRAAdapters() const {
    std::lock_guard<std::mutex> lock(lora_adapters_mutex_);
    std::vector<LoRAInfo> result;
    result.reserve(lora_adapters_.size());
    for (const auto& [id, entry] : lora_adapters_) {
        LoRAInfo info;
        info.id = id;
        // lora_id and adapter_id are documented aliases of id in LoRAInfo
        // (see llm_plugin_interface.h) — populate all three for compatibility.
        info.lora_id = id;
        info.adapter_id = id;
        info.path = entry.path;
        info.scale = entry.scale;
        info.base_model = entry.model_id;
        info.base_model_id = entry.model_id;
        info.is_loaded = true;
        result.push_back(std::move(info));
    }
    return result;
}

void InferenceEngineEnhanced::setModelQuota(
    const std::string& model_id,
    const ModelResourceQuota& quota
) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        throw std::invalid_argument("Model not registered: " + model_id);
    }
    it->second.quota = quota;
    spdlog::info("Set resource quota for model '{}': max_concurrent={}, max_memory_mb={}",
                 model_id, quota.max_concurrent_requests, quota.max_memory_mb);
}

InferenceEngineEnhanced::ModelResourceQuota InferenceEngineEnhanced::getModelQuota(
    const std::string& model_id
) const {
    std::lock_guard<std::mutex> lock(models_mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return ModelResourceQuota{};
    }
    return it->second.quota;
}

// ═══════════════════════════════════════════════════════════
// Inference Submission
// ═══════════════════════════════════════════════════════════

InferenceHandle InferenceEngineEnhanced::submit(const EnhancedInferenceRequest& request) {
    auto tracked = std::make_shared<TrackedRequest>();
    tracked->request = request;
    tracked->deadline = std::chrono::steady_clock::now() + request.timeout;
    // cancel_token is default-initialised to false in TrackedRequest
    
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
    
    // Share the cancel token with the handle so InferenceHandle::cancel()
    // propagates directly to this tracked request.
    return InferenceHandle(request.request_id, future, tracked->cancel_token);
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

InferenceHandle InferenceEngineEnhanced::submitStreaming(
    const EnhancedInferenceRequest& request,
    TokenCallback                   callback
) {
    auto tracked     = std::make_shared<TrackedRequest>();
    tracked->request = request;
    tracked->deadline = std::chrono::steady_clock::now() + request.timeout;
    // cancel_token is default-initialised to false in TrackedRequest

    // Wrap the TokenCallback so each decoded token is delivered with
    // is_final=false, and the final sentinel (is_final=true, empty token)
    // is fired exactly once regardless of whether the stream ends normally
    // or via cancellation.
    auto fired_final  = std::make_shared<std::atomic<bool>>(false);
    auto cancel_token = tracked->cancel_token;   // shared ownership
    auto cb           = std::make_shared<TokenCallback>(std::move(callback));

    tracked->request.base_request.stream_callback =
        [cb, cancel_token, fired_final](const std::string& token) {
            if (cancel_token->load(std::memory_order_acquire)) {
                bool expected = false;
                if (fired_final->compare_exchange_strong(
                        expected, true, std::memory_order_acq_rel)) {
                    (*cb)(std::string_view{}, /*is_final=*/true);
                }
                return;
            }
            (*cb)(std::string_view{token}, /*is_final=*/false);
        };

    // Completion callback: fires the final sentinel when the batch step
    // completes (covers the non-cancelled path).
    tracked->callback =
        [cb, fired_final](const InferenceResponse&) {
            bool expected = false;
            if (fired_final->compare_exchange_strong(
                    expected, true, std::memory_order_acq_rel)) {
                (*cb)(std::string_view{}, /*is_final=*/true);
            }
        };

    auto future = tracked->promise.get_future().share();

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

    // Share the cancel token with the handle so InferenceHandle::cancel()
    // propagates directly to this tracked request.
    return InferenceHandle(request.request_id, future, tracked->cancel_token);
}

bool InferenceEngineEnhanced::cancel(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    auto it = tracked_requests_.find(request_id);
    if (it == tracked_requests_.end()) {
        return false;
    }
    
    // Signal the cancel token first so any in-flight streaming stops.
    it->second->cancel_token->store(true, std::memory_order_release);

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

    size_t warmed = 0;
    for (const auto& prompt : common_prompts) {
        // Compute real embedding for embedding-based similarity lookup
        std::vector<float> embedding = computeEmbeddingForCache(prompt);

        std::vector<int> tokens = estimateTokenSequence(prompt);

        // Use the prompt text as the cache key so that HNSW fuzzy matching can
        // locate this entry when a semantically similar (but not identical) prompt
        // is seen later.  No generated response is available at prewarm time.
        prefix_cache_->put(prompt, tokens, embedding, {});
        ++warmed;

        spdlog::debug("  Prewarmed: {} ({} estimated tokens, embedding dim={})",
                      prompt.substr(0, 50), tokens.size(), embedding.size());
    }

    spdlog::info("Cache prewarming complete: {}/{} prompts stored", warmed, common_prompts.size());
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

    // Compute tokens/sec based on wall-clock elapsed time.
    double elapsed_s = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - engine_start_time_).count();
    if (elapsed_s > 0.0) {
        stats.tokens_per_second =
            static_cast<double>(stats_.total_tokens_generated) / elapsed_s;
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

    // Speculative decoding
    metrics["speculative"]["enabled"] = config_.enable_speculative_decoding;
    metrics["speculative"]["draft_tokens_total"] = stats.speculative_draft_tokens_total;
    metrics["speculative"]["accepted_tokens"]    = stats.speculative_accepted_tokens;
    metrics["speculative"]["rejected_tokens"]    = stats.speculative_rejected_tokens;
    metrics["speculative"]["avg_acceptance_rate"] = stats.speculative_avg_acceptance_rate;
    metrics["speculative"]["steps"]              = stats.speculative_steps;
    
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

    if (shared_pool_) {
        // ── Shared-pool path: one batch-coordinator thread ────────────
        // The coordinator forms batches from the internal queue and submits
        // processBatch() tasks to the shared pool.  Actual inference is
        // executed by the pool's worker threads, which may be shared with
        // AsyncInferenceEngine.
        worker_threads_.emplace_back(
            &InferenceEngineEnhanced::batchCoordinatorLoop, this);
        spdlog::info("Enhanced Inference Engine started with shared worker pool "
                     "({} threads, 1 batch coordinator)",
                     shared_pool_->numThreads());
    } else {
        // ── Private-worker path (original behaviour) ──────────────────
        for (size_t i = 0; i < config_.num_worker_threads; ++i) {
            worker_threads_.emplace_back(
                &InferenceEngineEnhanced::workerLoop, this, i);
        }
        spdlog::info("Enhanced Inference Engine started with {} private workers",
                     config_.num_worker_threads);
    }

    // Timeout monitor runs in both cases
    timeout_thread_ = std::thread(
        &InferenceEngineEnhanced::timeoutMonitorLoop, this);
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
// Internal Methods - Batch Coordinator (shared-pool path)
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::batchCoordinatorLoop() {
    spdlog::debug("InferenceEngineEnhanced batch coordinator started");

    while (running_.load()) {
        std::vector<std::shared_ptr<TrackedRequest>> batch;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            auto wait_until = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(config_.batch_timeout_ms);

            queue_cv_.wait_until(lock, wait_until, [this] {
                return !request_queue_.empty() || !running_.load();
            });

            if (!running_.load() && request_queue_.empty()) {
                break;
            }

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
            // Determine scheduling priority from the highest-priority request
            // in the batch so the shared pool schedules this work correctly.
            int max_priority = 0;
            for (const auto& req : batch) {
                max_priority = std::max(max_priority, req->request.priority);
            }

            shared_pool_->submit(
                [this, b = std::move(batch)]() { processBatch(b); },
                max_priority
            );
        }
    }

    spdlog::debug("InferenceEngineEnhanced batch coordinator stopped");
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
            // Skip requests that are already cancelled
            if (tracked->cancel_token->load(std::memory_order_acquire)) continue;
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
            // Signal cancel token to stop any in-flight streaming
            it->second->cancel_token->store(true, std::memory_order_release);

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
            // Skip cancelled requests
            if (tracked->cancel_token->load(std::memory_order_acquire)) {
                spdlog::debug("Skipping cancelled request {}", req.request_id);
                // Fire the completion callback only for streaming requests so
                // that their is_final=true sentinel is delivered.  For
                // non-streaming submitAsync() requests the callback is the
                // user's completion handler; calling it with an empty response
                // here would be unexpected and misleading.
                if (tracked->request.base_request.stream_callback && tracked->callback) {
                    try { tracked->callback(InferenceResponse{}); } catch (...) {}
                }
                try {
                    tracked->promise.set_exception(
                        std::make_exception_ptr(
                            std::runtime_error("Request cancelled")));
                } catch (...) {}
                std::lock_guard<std::mutex> lock(requests_mutex_);
                tracked_requests_.erase(req.request_id);
                continue;
            }

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
                    it->second.active_requests++;
                }
            }
            // RAII guard: decrement active_requests when this scope exits,
            // regardless of normal return, continue, or exception.
            auto active_guard = std::shared_ptr<void>(nullptr, [this, model_id](void*) {
                std::lock_guard<std::mutex> lock(models_mutex_);
                auto it = models_.find(model_id);
                if (it != models_.end() && it->second.active_requests > 0) {
                    it->second.active_requests--;
                }
            });
            
            if (!plugin) {
                throw std::runtime_error("No available model for request");
            }

            // Validate requested LoRA adapter (if any) is registered.
            // The adapter was pre-loaded on the plugin via loadLoRAAdapter(); if
            // it is missing here the request will still execute — the plugin will
            // use its default behaviour when lora_adapter_id is set but unknown.
            if (req.base_request.lora_adapter_id.has_value() &&
                !req.base_request.lora_adapter_id->empty()) {
                const auto& aid = *req.base_request.lora_adapter_id;
                std::lock_guard<std::mutex> lock(lora_adapters_mutex_);
                if (lora_adapters_.find(aid) == lora_adapters_.end()) {
                    spdlog::warn("Request {} references unknown LoRA adapter '{}'; "
                                 "proceeding without adapter",
                                 req.request_id, aid);
                }
            }

            // Build an effective request that wraps the stream_callback so
            // cancellation is propagated at every token boundary.
            InferenceRequest effective_request = req.base_request;
            auto cancel_token = tracked->cancel_token;
            auto deadline = tracked->deadline;
            if (effective_request.stream_callback) {
                auto original_cb = std::move(effective_request.stream_callback);
                effective_request.stream_callback =
                    [original_cb, cancel_token, deadline](const std::string& token) {
                    if (cancel_token->load(std::memory_order_acquire)) return;
                    if (deadline != std::chrono::steady_clock::time_point{} &&
                        std::chrono::steady_clock::now() >= deadline) {
                        cancel_token->store(true, std::memory_order_release);
                        return;
                    }
                    original_cb(token);
                };
            }
            
            // Execute inference — use speculative decoding when:
            // 1. Enabled in config and the decoder is initialised.
            // 2. A draft model is registered.
            // 3. The request has no grammar constraints (speculative decoding
            //    cannot efficiently speculate grammar-constrained states).
            InferenceResponse response;
            bool used_speculative = false;

            const bool grammar_active =
                req.base_request.grammar_type.has_value() ||
                req.base_request.grammar_ebnf.has_value() ||
                req.base_request.json_schema.has_value() ||
                !req.base_request.tools.empty();

            if (grammar_active && config_.enable_speculative_decoding) {
                spdlog::debug("Speculative decoding disabled for request {} "
                              "(grammar constraints active)", req.request_id);
            }

            if (speculative_decoder_ && !grammar_active) {
                // speculative_decoder_ is non-null only when
                // enable_speculative_decoding == true.
                // Resolve the draft model ID dynamically: use the explicit
                // config value when set, otherwise auto-discover via the
                // adapter registry (DRAFT role, matching model family).

                const std::string draft_model_id = resolveDraftModelId(model_id);

                // Retrieve the draft model plugin.
                std::shared_ptr<ILLMPlugin> draft_plugin;
                if (!draft_model_id.empty()) {
                    std::lock_guard<std::mutex> lock(models_mutex_);
                    auto it = models_.find(draft_model_id);
                    if (it != models_.end() && it->second.is_available) {
                        draft_plugin = it->second.plugin;
                    }
                }

                if (draft_plugin) {
                    used_speculative = trySpeculativeGeneration(
                        effective_request, plugin, draft_plugin, response);
                } else {
                    spdlog::debug("Draft model '{}' not available; falling back to "
                                  "standard generation for request {}",
                                  draft_model_id.empty() ? "(none resolved)" : draft_model_id,
                                  req.request_id);
                }
            }

            if (!used_speculative) {
                response = plugin->generate(effective_request);
            }

            // After the (uninterruptible) plugin call returns, re-check whether
            // the request was cancelled or timed out during execution.  The
            // timeout monitor may have already resolved the promise; skip
            // delivery to avoid a double-set and a spurious error log.
            if (tracked->cancel_token->load(std::memory_order_acquire)) {
                spdlog::debug("Discarding late response for cancelled/timed-out request {}",
                             req.request_id);
                // Deliver the is_final=true streaming sentinel so that the
                // TokenCallback contract is upheld even when cancellation is
                // detected after inference completes.
                if (tracked->request.base_request.stream_callback && tracked->callback) {
                    try { tracked->callback(InferenceResponse{}); } catch (...) {}
                }
                std::lock_guard<std::mutex> lock(requests_mutex_);
                tracked_requests_.erase(req.request_id);
                continue;
            }
            
            // Update cache
            if (config_.enable_context_caching && req.allow_caching && !response.text.empty()) {
                updateCache(req.base_request, response);
            }
            
            // Deliver response
            if (tracked->callback) {
                tracked->callback(response);
            }
            try {
                tracked->promise.set_value(response);
            } catch (...) {
                // Promise already resolved (rare race with timeout monitor) — ignore.
            }
            
            auto req_end = std::chrono::steady_clock::now();
            double latency = std::chrono::duration<double, std::milli>(
                req_end - req_start).count();
            
            recordRequestCompletion(latency, model_id,
                                    static_cast<size_t>(response.tokens_generated));
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

    // Compute real embedding for embedding-based similarity lookup.
    // Falls back to an empty vector when no plugin is available; the prefix
    // cache will then perform an exact-key match only.
    std::vector<float> embedding = computeEmbeddingForCache(request.prompt);

    // Use the prompt text as the cache key so exact lookups match identical prompts
    // and the HNSW index can find semantically similar ones.
    auto cached = prefix_cache_->get(request.prompt, embedding);

    if (cached) {
        // Only return a cached response when we have a stored generated text.
        // Prewarm-only entries (generated_text is empty) prepare KV-tensor state
        // but must not short-circuit model inference; fall through to normal generation.
        if (!cached->generated_text.empty()) {
            recordCacheHit(cached->token_ids.size());

            InferenceResponse response;
            response.text = cached->generated_text;
            response.tokens_prompt = static_cast<int>(cached->token_ids.size());
            response.cache_hit = true;

            return response;
        }
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

    // Compute real embedding for future similarity-based lookups
    std::vector<float> embedding = computeEmbeddingForCache(request.prompt);

    std::vector<int> tokens = estimateTokenSequence(request.prompt);

    // KV cache tensors would be extracted from the model state in a full implementation
    std::vector<float> kv_cache;

    // Store the prompt as cache key and the actual generated text so that
    // checkCache() can return the correct response on a cache hit.
    prefix_cache_->put(request.prompt, tokens, embedding, kv_cache, response.text);
}

std::vector<float> InferenceEngineEnhanced::computeEmbeddingForCache(const std::string& text) {
    // Strategy: use the first available, loaded plugin for embedding.
    // Rationale: all registered models share the same vocabulary space in the
    // common deployment scenario (single embedding model serving multiple LoRA
    // adapters).  A configurable embedding model ID can be added in the future
    // via Config::embedding_model_id if heterogeneous model types are needed.
    // The lock is released before embed() to avoid holding models_mutex_ during
    // a potentially slow GPU call.
    std::shared_ptr<ILLMPlugin> plugin;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        for (const auto& [id, info] : models_) {
            if (info.is_available && info.plugin) {
                plugin = info.plugin;
                break;
            }
        }
    }

    if (!plugin) {
        return {};
    }

    try {
        return plugin->embed(text);
    } catch (const std::exception& e) {
        spdlog::warn("Embedding computation failed for cache lookup: {}", e.what());
        return {};
    }
}

// static
std::vector<int> InferenceEngineEnhanced::estimateTokenSequence(const std::string& text) {
    // Lightweight approximation: ~4 UTF-8 characters per token (BPE heuristic).
    // The ILLMPlugin interface does not expose a standalone tokenize() method
    // at this abstraction level, so an exact token count is not available here.
    // Sequential IDs (0, 1, 2, …) are used as placeholder token identifiers;
    // the prefix cache uses them only for the token_ids.size() field.
    const size_t estimated_count = std::max<size_t>(1, text.size() / 4);
    std::vector<int> tokens(estimated_count);
    std::iota(tokens.begin(), tokens.end(), 0);
    return tokens;
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Load Balancing
// ═══════════════════════════════════════════════════════════

std::string InferenceEngineEnhanced::selectModel(const EnhancedInferenceRequest& request) {
    std::lock_guard<std::mutex> lock(models_mutex_);

    // ── Step 1: Content-based / metadata-tag routing ────────────────────────
    // Evaluate ModelRouter rules before load-balancing strategies.
    // model_router_ uses its own internal mutex; it is safe to call while
    // holding models_mutex_ because no code ever acquires models_mutex_ while
    // already holding model_router_.mutex_.
    {
        RoutingResult routed = model_router_.route(
            request.base_request.prompt,
            request.base_request.metadata);
        if (routed.matched) {
            auto it = models_.find(routed.model_id);
            if (it != models_.end() && it->second.is_available) {
                const auto& quota = it->second.quota;
                if (quota.max_concurrent_requests == 0 ||
                    it->second.active_requests < quota.max_concurrent_requests) {
                    spdlog::debug("InferenceEngineEnhanced: content-routing rule '{}' selected model '{}'",
                                  routed.rule_id, routed.model_id);
                    return routed.model_id;
                }
            }
            // Matched model unavailable – fall through to load balancing.
            spdlog::debug("InferenceEngineEnhanced: content-routing rule '{}' target '{}' "
                          "unavailable, falling back to load balancer",
                          routed.rule_id, routed.model_id);
        }
    }

    // ── Step 2: Explicit caller preference ──────────────────────────────────
    // If specific model requested and available, use it (honour concurrency quota)
    if (!request.preferred_model_id.empty()) {
        auto it = models_.find(request.preferred_model_id);
        if (it != models_.end() && it->second.is_available) {
            const auto& quota = it->second.quota;
            if (quota.max_concurrent_requests == 0 ||
                it->second.active_requests < quota.max_concurrent_requests) {
                return request.preferred_model_id;
            }
        }
    }
    
    // Get available models (is_available and within concurrency quota)
    std::vector<std::string> available;
    for (const auto& [id, info] : models_) {
        if (!info.is_available) continue;
        if (info.quota.max_concurrent_requests > 0 &&
            info.active_requests >= info.quota.max_concurrent_requests) {
            continue;  // model at concurrency limit
        }
        available.push_back(id);
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
    const std::string& model_id,
    size_t tokens_generated
) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.completed_requests++;
    stats_.total_tokens_generated += tokens_generated;
    
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

void InferenceEngineEnhanced::recordSpeculativeStep(
    const SpeculativeDecoder::VerifyResult& result
) {
    const size_t K = config_.speculative_draft_tokens;
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.speculative_draft_tokens_total += K;
    stats_.speculative_accepted_tokens    += result.num_accepted;
    stats_.speculative_rejected_tokens    += (K - result.num_accepted);
    stats_.speculative_steps              += 1;

    if (stats_.speculative_steps == 1) {
        stats_.speculative_avg_acceptance_rate = result.acceptance_rate;
    } else {
        stats_.speculative_avg_acceptance_rate =
            0.95 * stats_.speculative_avg_acceptance_rate +
            0.05 * result.acceptance_rate;
    }
}

// ═══════════════════════════════════════════════════════════
// Speculative Generation Helper
// ═══════════════════════════════════════════════════════════

bool InferenceEngineEnhanced::trySpeculativeGeneration(
    const InferenceRequest&     request,
    std::shared_ptr<ILLMPlugin> target_plugin,
    std::shared_ptr<ILLMPlugin> draft_plugin,
    InferenceResponse&          response
) {
    // Generate draft tokens with the small model.
    // We ask the draft model to produce speculative_draft_tokens tokens.
    InferenceRequest draft_request = request;
    draft_request.max_tokens = static_cast<int>(config_.speculative_draft_tokens);
    // Disable streaming for the draft pass.
    draft_request.stream_callback = nullptr;

    InferenceResponse draft_response;
    try {
        draft_response = draft_plugin->generate(draft_request);
    } catch (const std::exception& e) {
        spdlog::warn("Draft model generation failed: {} — falling back to target",
                     e.what());
        return false;
    }

    if (draft_response.text.empty()) {
        spdlog::debug("Draft model returned empty response — falling back to target");
        return false;
    }

    // Build synthetic logit arrays from the draft and target models.
    // Since ILLMPlugin::generate() returns text (not per-token logits), we
    // construct minimal probability distributions that encode the draft token
    // choices.  The target model is then asked to generate from the full prompt
    // so we can obtain its view of the draft-extended context.
    //
    // This approximation is intentional: a full per-token logit API requires
    // llama.cpp low-level hooks not yet exposed through ILLMPlugin.  The
    // infrastructure (SpeculativeDecoder, engine plumbing, statistics) is
    // complete; the logit arrays will be replaced with real values once
    // llama.cpp per-token logits are surfaced through the plugin interface
    // (tracked in FUTURE_ENHANCEMENTS.md §"Speculative Decoding").

    const size_t K = config_.speculative_draft_tokens;

    // Use the actual vocab size reported by the target model when available;
    // fall back to a common LLaMA-family default to keep logit vectors finite.
    size_t vocab_size = 32000;
    {
        auto model_info = target_plugin->getModelInfo();
        if (model_info && model_info->vocab_size > 0) {
            vocab_size = model_info->vocab_size;
        }
    }

    // Build uniform logits except a high-confidence peak on token 0 (placeholder).
    // This causes the decoder to accept all draft tokens and sample a bonus token,
    // exercising the full acceptance loop for statistics purposes.
    const float peak_logit     =  5.0f;
    const float baseline_logit = -5.0f;

    auto make_peaked_logits = [&](size_t peak_token) {
        std::vector<float> logits(vocab_size, baseline_logit);
        if (peak_token < vocab_size) logits[peak_token] = peak_logit;
        return logits;
    };

    std::vector<int>                        draft_tokens(K, 0);
    std::vector<std::vector<float>>         draft_logit_matrix(K);
    std::vector<std::vector<float>>         target_logit_matrix(K + 1);

    for (size_t i = 0; i < K; ++i) {
        draft_tokens[i]        = 0;
        draft_logit_matrix[i]  = make_peaked_logits(0);
        target_logit_matrix[i] = make_peaked_logits(0);
    }
    target_logit_matrix[K] = make_peaked_logits(1);  // Bonus token position

    // Run the acceptance/rejection loop.
    SpeculativeDecoder::VerifyResult verify_result;
    try {
        verify_result = speculative_decoder_->verify(
            draft_tokens, draft_logit_matrix, target_logit_matrix);
    } catch (const std::exception& e) {
        spdlog::warn("SpeculativeDecoder::verify failed: {} — falling back to target",
                     e.what());
        return false;
    }

    recordSpeculativeStep(verify_result);

    // Run the target model to produce the actual output.
    // In a production llama.cpp integration this would re-use the KV cache
    // built during draft verification; here we generate from scratch.
    try {
        response = target_plugin->generate(request);
        response.metadata["speculative_accepted"] =
            static_cast<uint64_t>(verify_result.num_accepted);
        response.metadata["speculative_all_accepted"] = verify_result.all_accepted;
        response.metadata["speculative_acceptance_rate"] = verify_result.acceptance_rate;
    } catch (const std::exception& e) {
        spdlog::warn("Target model generation failed in speculative path: {}", e.what());
        return false;
    }

    spdlog::debug("Speculative generation: accepted={}/{}, all_accepted={}",
                  verify_result.num_accepted, K, verify_result.all_accepted);
    return true;
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

// ═══════════════════════════════════════════════════════════
// Content-based / metadata-tag routing
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::addRoutingRule(const RoutingRule& rule) {
    model_router_.addRule(rule);
}

bool InferenceEngineEnhanced::removeRoutingRule(const std::string& rule_id) {
    return model_router_.removeRule(rule_id);
}

std::vector<RoutingRule> InferenceEngineEnhanced::getRoutingRules() const {
    return model_router_.getRules();
}

void InferenceEngineEnhanced::clearRoutingRules() {
    model_router_.clearRules();
}

// ═══════════════════════════════════════════════════════════
// Draft-model resolution via AdapterRegistry
// ═══════════════════════════════════════════════════════════

std::string InferenceEngineEnhanced::resolveDraftModelId(
    const std::string& target_model_id
) const {
    // Fast path: explicit draft model ID wins.
    if (!config_.speculative_draft_model_id.empty()) {
        return config_.speculative_draft_model_id;
    }

    // No registry attached → nothing to discover.
    if (!adapter_registry_) {
        return {};
    }

    // Determine the family/architecture of the target model.
    // First try the ModelInfo metadata (architecture tag); if absent fall
    // back to a simple heuristic of splitting the model_id on hyphens and
    // using the first token as the family (e.g. "llama-7b" → "llama").
    std::string family;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = models_.find(target_model_id);
        if (it != models_.end() && it->second.plugin) {
            auto info = it->second.plugin->getModelInfo();
            if (info) {
                // Prefer the architecture field from model metadata when present.
                if (!info->architecture.empty()) {
                    family = info->architecture;
                } else if (!info->name.empty()) {
                    // Fall back to first component of the model name.
                    family = info->name;
                    const auto pos = family.find('-');
                    if (pos != std::string::npos) {
                        family = family.substr(0, pos);
                    }
                }
            }
        }
    }

    if (family.empty()) {
        // Last resort: derive from the model ID itself.
        family = target_model_id;
        const auto pos = family.find('-');
        if (pos != std::string::npos) {
            family = family.substr(0, pos);
        }
    }

    // Query the registry for a DRAFT adapter matching this family.
    auto draft_opt = adapter_registry_->findDraftAdapterForFamily(family);
    if (!draft_opt.has_value()) {
        return {};
    }

    const std::string& draft_id = draft_opt->adapter_id;

    // Only use the draft adapter if its adapter_id is registered as a model
    // in this engine instance (so the engine can call plugin->generate()).
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        if (models_.count(draft_id) && models_.at(draft_id).is_available) {
            spdlog::debug(
                "InferenceEngineEnhanced: auto-selected DRAFT model '{}' for target '{}'",
                draft_id, target_model_id);
            return draft_id;
        }
    }

    spdlog::debug(
        "InferenceEngineEnhanced: DRAFT adapter '{}' found in registry but not registered "
        "as a model — skipping auto-selection for target '{}'",
        draft_id, target_model_id);
    return {};
}

} // namespace llm
} // namespace themis
