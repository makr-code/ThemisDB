/**
 * @file inference_engine_enhanced.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=10; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=76, M=19, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/inference_engine_enhanced.h"
#include <stdexcept>
#include "llm/llama_wrapper.h"
#include "llm/lookup_decoder.h"
#include "llm/model_router.h"
#include "llm/prompt_safety_utils.h"
#include "llm/scoped_db_connection.h"
#include "llm/shared_worker_pool.h"
#include "llm/speculative_decoder.h"
#include "sharding/remote_executor.h"
#include <spdlog/spdlog.h>
#include "utils/thread_join_utils.h"
#include <algorithm>
#include <limits>
#include <numeric>
#include <sstream>
#include <utility>
#include "utils/logger.h"

namespace themis {
namespace llm {

namespace {

void applySelfRAGSizeT(const json& cfg_json, const char* key, size_t& target) {
    const auto it = cfg_json.find(key);
    if (it != cfg_json.end() && it->is_number_unsigned()) {
        target = it->get<size_t>();
    } else if (it != cfg_json.end() && it->is_number_integer()) {
        const auto value = it->get<int64_t>();
        if (value >= 0) {
            target = static_cast<size_t>(value);
        }
    }
}

void applySelfRAGDouble(const json& cfg_json, const char* key, double& target) {
    const auto it = cfg_json.find(key);
    if (it != cfg_json.end() && it->is_number()) {
        target = it->get<double>();
    }
}

json makeSelfRAGMetadataObject() {
    return json::object();
}

InferenceEngineEnhanced::TokenizerFn makeTokenizerBridgeForPlugin(
    const std::shared_ptr<ILLMPlugin>& plugin
) {
    const auto llama_plugin = std::dynamic_pointer_cast<LlamaWrapper>(plugin);
    if (!llama_plugin) {
        return {};
    }

    return [llama_plugin](const std::string& text, size_t vocab_size) {
        std::vector<int> tokens = llama_plugin->tokenizeForBridge(text, true);
        if (tokens.empty()) {
            return tokens;
        }

        if (vocab_size == 0) {
            return tokens;
        }

        std::vector<int> clamped;
        clamped.reserve(tokens.size());
        const int max_token = static_cast<int>(
            std::min(vocab_size - 1,
                     static_cast<size_t>(std::numeric_limits<int>::max())));
        for (const int token : tokens) {
            clamped.push_back(std::max(0, std::min(token, max_token)));
        }
        return clamped;
    };
}

} // namespace

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
        sched_config.enable_adaptive_batch_retry = config_.enable_adaptive_batch_retry;
        
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
        sd_cfg.remote_draft_shard_id = config_.speculative_remote_draft_shard_id;
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

    // Initialise n-gram lookup decoder when requested (draft-model-free path).
    if (config_.enable_lookup_decoding) {
        LookupDecoder::Config ld_cfg;
        ld_cfg.ngram_min = config_.lookup_ngram_min;
        ld_cfg.ngram_max = config_.lookup_ngram_max;
        // Use the explicit max_draft_tokens config when set; otherwise default
        // to ngram_max (the maximum key length equals the default continuation
        // budget, which is the correct coupling for prompt lookup decoding).
        ld_cfg.max_draft_tokens = (config_.lookup_max_draft_tokens > 0)
                                      ? config_.lookup_max_draft_tokens
                                      : config_.lookup_ngram_max;
        lookup_decoder_ = std::make_unique<LookupDecoder>(ld_cfg);
        spdlog::info("Lookup decoder initialised: ngram_min={}, ngram_max={}, max_draft={}",
                     ld_cfg.ngram_min, ld_cfg.ngram_max, ld_cfg.max_draft_tokens);
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

InferenceEngineEnhanced::~InferenceEngineEnhanced() noexcept {
    // Phase2-LLM-B1: exception_in_destructor — shutdown() may throw; must not
    // propagate out of the destructor or std::terminate() is invoked.
    try {
        shutdown();
    } catch (const std::exception& e) {
        spdlog::error("InferenceEngineEnhanced::~InferenceEngineEnhanced: exception during shutdown (suppressed): {}", e.what());
    } catch (...) {
        spdlog::error("InferenceEngineEnhanced::~InferenceEngineEnhanced: unknown exception during shutdown (suppressed)");
    }
}

void InferenceEngineEnhanced::setRemoteExecutor(
    sharding::RemoteExecutor* exec,
    const sharding::ShardInfo& draft_shard)
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    remote_executor_          = exec;
    remote_draft_shard_info_  = draft_shard;
    if (exec) {
        spdlog::info("InferenceEngineEnhanced: remote draft executor set "
                     "(shard='{}', endpoint='{}')",
                     draft_shard.shard_id, draft_shard.primary_endpoint);
    } else {
        spdlog::info("InferenceEngineEnhanced: remote draft executor detached");
    }
}

void InferenceEngineEnhanced::setFederatedBackend(
    std::shared_ptr<IFederatedInferenceBackend> backend)
{
    std::lock_guard<std::mutex> lock(federated_backend_mutex_);
    federated_backend_ = std::move(backend);
    if (federated_backend_) {
        spdlog::info("InferenceEngineEnhanced: federated inference backend attached "
                     "(cross-instance fan-out enabled)");
    } else {
        spdlog::info("InferenceEngineEnhanced: federated inference backend detached");
    }
}

void InferenceEngineEnhanced::setSelfRAGRetrievalCallback(SelfRAGRetrievalCallback cb) {
    std::lock_guard<std::mutex> lock(self_rag_mutex_);
    self_rag_retrieval_cb_ = std::move(cb);
}

void InferenceEngineEnhanced::setSelfRAGCriticCallback(SelfRAGCriticCallback cb) {
    std::lock_guard<std::mutex> lock(self_rag_mutex_);
    self_rag_critic_cb_ = std::move(cb);
}

// ── setTargetLogitsFn ────────────────────────────────────────────────────────
void InferenceEngineEnhanced::setTargetLogitsFn(TargetLogitsFn fn) {
    std::lock_guard<std::mutex> lock(target_logits_fn_mutex_);
    target_logits_fn_ = std::move(fn);
}

// ── setTokenizerFn / clearTokenizerFn ────────────────────────────────────────
void InferenceEngineEnhanced::setTokenizerFn(TokenizerFn fn) {
    std::lock_guard<std::mutex> lock(tokenizer_fn_mutex_);
    tokenizer_fn_ = std::move(fn);
}

void InferenceEngineEnhanced::clearTokenizerFn() {
    std::lock_guard<std::mutex> lock(tokenizer_fn_mutex_);
    tokenizer_fn_ = nullptr;
}

// ═══════════════════════════════════════════════════════════
// Model Management
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::registerModel(
    const std::string& model_id,
    std::shared_ptr<ILLMPlugin> plugin
) {
    // Fail-closed: reject empty model_id
    if (model_id.empty()) {
        spdlog::error("InferenceEngineEnhanced::registerModel: model_id is empty");
        return;
    }

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
    auto cb           = std::make_shared<TokenCallback>(std::move(callback));

    tracked->request.base_request.stream_callback =
        [cb, cancel_token = tracked->cancel_token, fired_final](const std::string& token) {
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
        THEMIS_WARN("inference_engine_enhanced: unhandled exception caught");
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
    auto* cache = prefix_cache_.get();
    if (!cache) {
        return;
    }
    cache->clear();
    spdlog::info("Cleared inference cache");
}

void InferenceEngineEnhanced::prewarmCache(const std::vector<std::string>& common_prompts) {
    auto* cache = prefix_cache_.get();
    if (!cache || !config_.enable_context_caching) {
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
        cache->put(prompt, tokens, embedding, {});
        ++warmed;

        spdlog::debug("  Prewarmed prompt (length: {}, {} estimated tokens, embedding dim={})",
                      prompt.length(), tokens.size(), embedding.size());
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
    size_t total_cache_ops = stats.cache_hits + stats.cache_misses;
    if (total_cache_ops > 0) {
        stats.cache_hit_rate = static_cast<double>(stats.cache_hits) / total_cache_ops;
    }
    
    // Calculate load balance fairness (1.0 = perfectly balanced)
    if (!stats.requests_per_model.empty()) {
        double mean = 0.0;
        for (const auto& [model, count] : stats.requests_per_model) {
            mean += count;
        }
        mean /= stats.requests_per_model.size();
        
        double variance = 0.0;
        for (const auto& [model, count] : stats.requests_per_model) {
            variance += (count - mean) * (count - mean);
        }
        variance /= stats.requests_per_model.size();
        
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
            static_cast<double>(stats.total_tokens_generated) / elapsed_s;
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
    metrics["cache"]["cache_miss"] = stats.cache_misses;
    metrics["cache"]["tokens_saved"] = stats.tokens_saved_by_cache;
    
    // Batch metrics
    metrics["batch"]["total_batches"] = stats.total_batches;
    metrics["batch"]["avg_size"] = stats.avg_batch_size;
    metrics["batch"]["max_size"] = stats.max_batch_size_seen;
    metrics["batch"]["throughput_improvement"] = stats.throughput_improvement;
    metrics["batch"]["batch_retry_count"] = batch_scheduler_
        ? batch_scheduler_->getStats().batch_retry_count
        : 0;
    
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
    metrics["speculative"]["drafted_tokens"] = stats.speculative_draft_tokens_total;
    metrics["speculative"]["accepted_tokens"]    = stats.speculative_accepted_tokens;
    metrics["speculative"]["rejected_tokens"]    = stats.speculative_rejected_tokens;
    metrics["speculative"]["avg_acceptance_rate"] = stats.speculative_avg_acceptance_rate;
    metrics["speculative"]["accept_rate"] = stats.speculative_avg_acceptance_rate;
    metrics["speculative"]["steps"]              = stats.speculative_steps;
    metrics["feature_flags"]["lookup_decoding"] = config_.enable_lookup_decoding;
    metrics["feature_flags"]["adaptive_batch_retry"] = config_.enable_adaptive_batch_retry;

    // Lookup decoder statistics (if enabled)
    if (lookup_decoder_) {
        const auto ls = lookup_decoder_->getStats();
        metrics["lookup_decoding"]["probe_calls"]           = ls.total_probe_calls;
        metrics["lookup_decoding"]["hits"]                  = ls.total_hits;
        metrics["lookup_decoding"]["hit_rate"]              = ls.hit_rate();
        metrics["lookup_decoding"]["draft_tokens_proposed"] = ls.total_draft_tokens_proposed;
    } else {
        metrics["lookup_decoding"]["probe_calls"]           = 0;
        metrics["lookup_decoding"]["hits"]                  = 0;
        metrics["lookup_decoding"]["hit_rate"]              = 0.0;
        metrics["lookup_decoding"]["draft_tokens_proposed"] = 0;
    }
    
    return metrics;
}

// ═══════════════════════════════════════════════════════════
// Lifecycle
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(true, std::memory_order_release);

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
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    spdlog::info("Shutting down Enhanced Inference Engine...");
    
    running_.store(false, std::memory_order_release);
    queue_cv_.notify_all();
    
    // Join worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            if (!themis::utils::joinThreadWithin(thread)) {
                spdlog::warn("Worker thread did not join within timeout, continuing shutdown");
            }
        }
    }
    worker_threads_.clear();
    
    // Join timeout thread
    if (timeout_thread_.joinable()) {
        if (!themis::utils::joinThreadWithin(timeout_thread_)) {
            spdlog::warn("Timeout thread did not join within timeout, continuing shutdown");
        }
    }
    
    spdlog::info("Enhanced Inference Engine shutdown complete");
}

bool InferenceEngineEnhanced::isRunning() const {
    return running_.load(std::memory_order_acquire);
}

// ═══════════════════════════════════════════════════════════
// Internal Methods - Worker Loop
// ═══════════════════════════════════════════════════════════

void InferenceEngineEnhanced::workerLoop(size_t worker_id) {
    spdlog::debug("Worker {} started", worker_id);
    
    while (running_.load(std::memory_order_acquire)) {
        std::vector<std::shared_ptr<TrackedRequest>> batch;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for work or batch timeout
            auto wait_until = std::chrono::steady_clock::now() + 
                             std::chrono::milliseconds(config_.batch_timeout_ms);
            
            queue_cv_.wait_until(lock, wait_until, [this] {
                return !request_queue_.empty() || !running_.load(std::memory_order_acquire);
            });
            
            if (!running_.load(std::memory_order_acquire) && request_queue_.empty()) {
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

    while (running_.load(std::memory_order_acquire)) {
        std::vector<std::shared_ptr<TrackedRequest>> batch;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            auto wait_until = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(config_.batch_timeout_ms);

            queue_cv_.wait_until(lock, wait_until, [this] {
                return !request_queue_.empty() || !running_.load(std::memory_order_acquire);
            });

            if (!running_.load(std::memory_order_acquire) && request_queue_.empty()) {
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
    
    while (running_.load(std::memory_order_acquire)) {
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
                THEMIS_WARN("inference_engine_enhanced: unhandled exception caught");
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
    // Thread-safe: member accesses protected by respective mutexes
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
            // Wave-B L2: replaced shared_ptr<void> hack with ScopedDbConnection.
            ScopedDbConnection active_guard([this, model_id]() noexcept {
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
            if (!effective_request.metadata.is_object()) {
                effective_request.metadata = json::object();
            }
            auto raid_sharding = effective_request.metadata.value("raid_sharding", json::object());
            if (!req.shard_routing_key.empty()) {
                raid_sharding["routing_key"] = req.shard_routing_key;
            }
            if (!req.target_instance_ids.empty()) {
                raid_sharding["target_instance_ids"] = req.target_instance_ids;
            }
            // Keep this boolean always present so downstream coordinators can
            // distinguish "explicitly disabled" from "field omitted".
            raid_sharding["allow_cross_instance_batching"] = req.allow_cross_instance_batching;
            effective_request.metadata["raid_sharding"] = std::move(raid_sharding);
            auto deadline = tracked->deadline;
            if (effective_request.stream_callback) {
                auto original_cb = std::move(effective_request.stream_callback);
                effective_request.stream_callback =
                    [original_cb, cancel_token = tracked->cancel_token, deadline](const std::string& token) {
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
            struct SelfRAGExecution {
                bool enabled = false;
                bool used_rag_context = false;
                std::string query;
                themis::rag::SelfRAGResult result;
                std::optional<RAGContext> rag_context;
            } self_rag;

            InferenceResponse response;
            bool used_speculative = false;

            const bool grammar_active =
                req.base_request.grammar_type.has_value() ||
                req.base_request.grammar_ebnf.has_value() ||
                req.base_request.json_schema.has_value() ||
                !req.base_request.tools.empty();

            // ── RAID fan-out: delegate to federated backend when requested ──
            // If a federated backend is attached and the request lists specific
            // target instances, delegate the request instead of running locally.
            // Merge strategy: first-wins (first successful instance result).
            {
                std::shared_ptr<IFederatedInferenceBackend> fed_backend;
                {
                    std::lock_guard<std::mutex> fb_lock(federated_backend_mutex_);
                    fed_backend = federated_backend_;
                }

                if (fed_backend && !req.target_instance_ids.empty()) {
                    spdlog::debug("InferenceEngineEnhanced: delegating request '{}' "
                                  "to federated backend ({} instance(s))",
                                  req.request_id, req.target_instance_ids.size());

                    const auto fan_results =
                        fed_backend->execute(req.target_instance_ids, effective_request);

                    // First-wins merge: pick first successful result.
                    bool merged = false;
                    for (const auto& fr : fan_results) {
                        if (fr.success) {
                            response = fr.response;
                            response.metadata["fan_out_instance"] = fr.instance_id;
                            response.metadata["fan_out_total"]    = fan_results.size();
                            merged = true;
                            break;
                        }
                    }

                    if (!merged) {
                        // All instances failed — build aggregated error.
                        std::string agg;
                        for (const auto& fr : fan_results) {
                            agg += "[" + fr.instance_id + ": " + fr.error + "] ";
                        }
                        spdlog::error("InferenceEngineEnhanced: all {} fan-out instances "
                                      "failed for request '{}': {}",
                                      fan_results.size(), req.request_id, agg);
                        response.success       = false;
                        response.error_message = "All fan-out instances failed: " + agg;
                    }

                    // Skip local inference for fan-out requests.
                    // Jump to the result handling below.
                    goto fan_out_done; // NOLINT(cppcoreguidelines-avoid-goto)
                }
            }

            if (effective_request.metadata.is_object()) {
                const auto self_rag_it = effective_request.metadata.find("self_rag");
                if (self_rag_it != effective_request.metadata.end() &&
                    self_rag_it->is_object() &&
                    self_rag_it->value("enabled", false)) {
                    SelfRAGRetrievalCallback retrieval_cb;
                    SelfRAGCriticCallback critic_cb;
                    {
                        std::lock_guard<std::mutex> lock(self_rag_mutex_);
                        retrieval_cb = self_rag_retrieval_cb_;
                        critic_cb = self_rag_critic_cb_;
                    }

                    if (!retrieval_cb) {
                        throw std::runtime_error(
                            "InferenceEngineEnhanced: self_rag enabled but no retrieval callback set");
                    }

                    self_rag.enabled = true;
                    // Sanitize prompt to prevent injection attacks
                    std::string sanitized_prompt = effective_request.prompt;
                    std::string blocked_rule, blocked_reason;
                    if (!themis::llm::prompt_safety::sanitizePromptWithSharedPolicy(
                            sanitized_prompt, sanitized_prompt, &blocked_rule, &blocked_reason)) {
                        spdlog::warn("Prompt sanitization failed for self_rag query: rule={}, reason={}",
                                   blocked_rule, blocked_reason);
                    }
                    self_rag.query = self_rag_it->value("query", sanitized_prompt);

                    themis::rag::SelfRAGConfig self_rag_cfg;
                    const auto cfg_json = self_rag_it->value("config", json::object());
                    if (cfg_json.is_object()) {
                        applySelfRAGSizeT(cfg_json, "max_rounds", self_rag_cfg.max_rounds);
                        applySelfRAGSizeT(cfg_json, "top_k", self_rag_cfg.top_k);
                        applySelfRAGDouble(cfg_json, "relevant_threshold", self_rag_cfg.relevant_threshold);
                        applySelfRAGDouble(cfg_json, "partial_threshold", self_rag_cfg.partial_threshold);
                        applySelfRAGSizeT(cfg_json, "target_relevant_docs",
                                          self_rag_cfg.target_relevant_docs);
                        applySelfRAGDouble(cfg_json, "retrieval_confidence_threshold",
                                           self_rag_cfg.retrieval_confidence_threshold);
                    }

                    const double query_confidence = self_rag_it->value("query_confidence", 0.0);

                    themis::rag::SelfRAGController controller(self_rag_cfg);
                    controller.setRetrievalCallback(
                        [retrieval_cb, request = effective_request](
                            const std::string& query,
                            size_t             top_k) {
                            return retrieval_cb(query, top_k, request);
                        });
                    if (critic_cb) {
                        controller.setCriticCallback(
                            [critic_cb, request = effective_request](
                                const std::string& query,
                                const themis::rag::SelfRAGDocument& doc) {
                                return critic_cb(query, doc, request);
                            });
                    }

                    self_rag.result = controller.runRefinementLoop(self_rag.query, query_confidence);

                    if (self_rag.result.retrieval_triggered) {
                        RAGContext rag_context;
                        rag_context.query = self_rag.query;
                        rag_context.collection_name =
                            self_rag_it->value("collection_name", std::string{});
                        rag_context.top_k = static_cast<int>(self_rag_cfg.top_k);

                        const auto context_template_it = self_rag_it->find("context_template");
                        if (context_template_it != self_rag_it->end() &&
                            context_template_it->is_string()) {
                            rag_context.context_template =
                                context_template_it->get<std::string>();
                        }
                        const auto max_context_tokens_it =
                            self_rag_it->find("max_context_tokens");
                        if (max_context_tokens_it != self_rag_it->end() &&
                            max_context_tokens_it->is_number_integer()) {
                            rag_context.max_context_tokens =
                                max_context_tokens_it->get<int>();
                        }
                        const auto response_budget_tokens_it =
                            self_rag_it->find("response_budget_tokens");
                        if (response_budget_tokens_it != self_rag_it->end() &&
                            response_budget_tokens_it->is_number_integer()) {
                            rag_context.response_budget_tokens =
                                response_budget_tokens_it->get<int>();
                        }

                        auto append_docs =
                            [&rag_context](const std::vector<themis::rag::RatedDocument>& docs,
                                           const char* verdict) {
                            for (const auto& rated : docs) {
                                RAGContext::Document doc;
                                doc.content = rated.document.content;
                                doc.source = rated.document.id;
                                doc.relevance_score =
                                    static_cast<float>(rated.critic_score);
                                doc.metadata = makeSelfRAGMetadataObject();
                                doc.metadata["self_rag_verdict"] = verdict;
                                doc.metadata["critic_score"] = rated.critic_score;
                                doc.metadata["retrieval_score"] = rated.document.score;
                                rag_context.documents.push_back(std::move(doc));
                            }
                        };

                        append_docs(self_rag.result.relevant_docs, "relevant");
                        append_docs(self_rag.result.partial_docs, "partial");

                        if (!rag_context.documents.empty()) {
                            self_rag.rag_context = std::move(rag_context);
                            self_rag.used_rag_context = true;
                            effective_request.metadata["rag_enabled"] = true;
                        }
                    }
                }
            }

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

            // Lookup decoder (n-gram based, draft-model-free).
            // Active when enable_lookup_decoding == true and neither grammar
            // constraints nor draft-model speculative decoding are engaged.
            if (!used_speculative && lookup_decoder_ && !grammar_active) {
                // Sanitize prompt to prevent injection attacks
                std::string sanitized_prompt = effective_request.prompt;
                std::string blocked_rule, blocked_reason;
                if (!themis::llm::prompt_safety::sanitizePromptWithSharedPolicy(
                            sanitized_prompt, sanitized_prompt, &blocked_rule, &blocked_reason)) {
                    spdlog::warn("Prompt sanitization failed for lookup decoder: rule={}, reason={}",
                               blocked_rule, blocked_reason);
                }
                const auto& prompt = sanitized_prompt;
                // Build the prompt n-gram index for this request.
                // Note: estimateTokenSequence() uses a 4-chars-per-token heuristic
                // (the ILLMPlugin interface does not expose a standalone tokenize()
                // method).  The draft proposals are informational hints only;
                // the plugin may accept or ignore them, and standard generation
                // is always used as the fallback.  Accuracy improves when actual
                // tokenization is available through plugin-level integration.
                const auto prompt_tokens = estimateTokenSequence(prompt);
                lookup_decoder_->buildFromPrompt(prompt_tokens);

                // Propose draft tokens from the prompt context.
                const auto max_draft = (config_.lookup_max_draft_tokens > 0)
                                           ? config_.lookup_max_draft_tokens
                                           : config_.lookup_ngram_max;
                const auto drafts = lookup_decoder_->proposeDraftTokens(prompt_tokens, max_draft);

                if (!drafts.empty()) {
                    spdlog::debug("Lookup decoder proposed {} draft tokens for request {}",
                                  drafts.size(), req.request_id);
                    // Attach draft hints into the request metadata for the plugin.
                    // The plugin may or may not use them; standard generation
                    // is used as fallback regardless.
                    auto lookup_decoding =
                        effective_request.metadata.value("lookup_decoding", json::object());
                    lookup_decoding["draft_tokens"] = drafts;
                    lookup_decoding["ngram_hit"] = true;
                    effective_request.metadata["lookup_decoding"] = std::move(lookup_decoding);
                }
            }

            if (!used_speculative) {
                if (self_rag.rag_context.has_value()) {
                    response = plugin->generateRAG(*self_rag.rag_context, effective_request);
                } else {
                    response = plugin->generate(effective_request);
                }
            }

            fan_out_done: // Label for fan-out path (skips local inference)

            if (self_rag.enabled) {
                json self_rag_meta = makeSelfRAGMetadataObject();
                self_rag_meta["enabled"] = true;
                self_rag_meta["query"] = self_rag.query;
                self_rag_meta["retrieval_triggered"] = self_rag.result.retrieval_triggered;
                self_rag_meta["used_rag_context"] = self_rag.used_rag_context;
                self_rag_meta["relevant_docs"] = self_rag.result.relevant_docs.size();
                self_rag_meta["partial_docs"] = self_rag.result.partial_docs.size();
                self_rag_meta["rounds_used"] = self_rag.result.total_rounds_used;
                response.metadata["self_rag"] = std::move(self_rag_meta);
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
                THEMIS_WARN("inference_engine_enhanced: unhandled exception caught");
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
                THEMIS_WARN("inference_engine_enhanced: unhandled exception caught");
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
    auto* cache = prefix_cache_.get();
    if (!cache) {
        return std::nullopt;
    }

    // Compute real embedding for embedding-based similarity lookup.
    // Falls back to an empty vector when no plugin is available; the prefix
    // cache will then perform an exact-key match only.
    std::vector<float> embedding = computeEmbeddingForCache(request.prompt);

    // IV-03: Validate embedding dimension consistency before passing to the
    // cache index.  Typical LLM embeddings are at least 64-dimensional; a
    // smaller vector indicates a corrupted or stub embedding that must not be
    // fed into the HNSW similarity index (wrong dimensionality would silently
    // corrupt similarity scores).  Fall back to exact-key matching only.
    constexpr size_t MIN_EMBEDDING_DIM = 64;
    if (!embedding.empty() && embedding.size() < MIN_EMBEDDING_DIM) {
        spdlog::warn("checkCache: embedding dimension {} is below minimum {}; "
                     "falling back to exact-key lookup",
                     embedding.size(), MIN_EMBEDDING_DIM);
        embedding.clear();
    }

    // Use the prompt text as the cache key so exact lookups match identical prompts
    // and the HNSW index can find semantically similar ones.
    auto cached = cache->get(request.prompt, embedding);

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
    auto* cache = prefix_cache_.get();
    if (!cache || response.text.empty()) {
        return;
    }

    // Compute real embedding for future similarity-based lookups
    std::vector<float> embedding = computeEmbeddingForCache(request.prompt);

    // IV-03: Reject stub/corrupted embeddings (see checkCache for rationale).
    constexpr size_t MIN_EMBEDDING_DIM = 64;
    if (!embedding.empty() && embedding.size() < MIN_EMBEDDING_DIM) {
        spdlog::warn("updateCache: embedding dimension {} is below minimum {}; "
                     "storing without embedding (exact-key lookup only)",
                     embedding.size(), MIN_EMBEDDING_DIM);
        embedding.clear();
    }

    std::vector<int> tokens = estimateTokenSequence(request.prompt);

    // KV cache tensors would be extracted from the model state in a full implementation
    std::vector<float> kv_cache;

    // Store the prompt as cache key and the actual generated text so that
    // checkCache() can return the correct response on a cache hit.
    cache->put(request.prompt, tokens, embedding, kv_cache, response.text);
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
        stats_.avg_batch_size = static_cast<double>(batch_size);
    } else {
        stats_.avg_batch_size = 
            0.95 * stats_.avg_batch_size + 0.05 * static_cast<double>(batch_size);
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
    if (!target_plugin || !draft_plugin || !speculative_decoder_) {
        spdlog::warn("Speculative decoding prerequisites not met (null plugin/decoder)");
        return false;
    }

    const size_t K = config_.speculative_draft_tokens;

    // Use the actual vocab size reported by the target model when available;
    // fall back to a common LLaMA-family default to keep logit vectors finite.
    size_t vocab_size = 32000u;
    {
        auto model_info = target_plugin->getModelInfo();
        if (model_info && model_info->vocab_size > 0) {
            vocab_size = model_info->vocab_size;
        }
    }
    if (vocab_size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        spdlog::warn("Speculative decoding vocab size {} exceeds int range; using fallback 32000",
                     vocab_size);
        vocab_size = 32000u;
    }
    const int vocab_size_int = static_cast<int>(vocab_size);

    // ── Remote draft path ─────────────────────────────────────────────────
    // When a remote draft shard is configured and a RemoteExecutor is injected,
    // attempt to fetch draft tokens from the remote shard first.  On any
    // failure (network error, circuit-breaker open, empty response) we fall
    // through to the local draft model below.
    sharding::RemoteExecutor* remote_exec = nullptr;
    sharding::ShardInfo        remote_shard;
    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        remote_exec  = remote_executor_;
        remote_shard = remote_draft_shard_info_;
    }
    const bool use_remote =
        remote_exec != nullptr &&
        speculative_decoder_ &&
        !speculative_decoder_->getConfig().remote_draft_shard_id.empty();

    // draft_result holds the token IDs + logit distributions obtained from the
    // draft model (either remote text or local generateDraftTokens()).
    ILLMPlugin::DraftTokensResult draft_result;

    if (use_remote) {
        // Fetch draft text from the remote shard and convert to token IDs +
        // peaked logit distributions using the same byte-modulo heuristic as
        // the local generateDraftTokens() path (see STUB/SIMULATION NOTE below).
        std::string remote_text;
        try {
            const nlohmann::json body = {
                {"prompt",     request.prompt},
                {"max_tokens", static_cast<int>(K)},
                {"model_id",   config_.speculative_draft_model_id}
            };
            const auto result = remote_exec->post(
                remote_shard,
                "/api/v1/llm/speculative/draft",
                body
            );
            if (result.success && result.data.contains("text") &&
                !result.data["text"].get<std::string>().empty())
            {
                remote_text = result.data["text"].get<std::string>();
                spdlog::debug("Remote draft tokens fetched from shard '{}' ({} chars)",
                              remote_shard.shard_id, remote_text.size());
            } else {
                spdlog::debug("Remote draft shard '{}' returned no tokens — "
                              "falling back to local draft model",
                              remote_shard.shard_id);
            }
        } catch (const std::exception& e) {
            spdlog::warn("Remote draft dispatch to shard '{}' failed: {} — "
                         "falling back to local draft model",
                         remote_shard.shard_id, e.what());
        }

        if (!remote_text.empty()) {
            // Convert remote text to token IDs + logit distributions.
            // ── Primary path: use the injected TokenizerFn when available ────
            // ── Fallback:     retry the local draft model when no tokenizer
            //                   bridge is available for the remote text ─────────
            TokenizerFn tok_fn_copy;
            {
                std::lock_guard<std::mutex> lk(tokenizer_fn_mutex_);
                tok_fn_copy = tokenizer_fn_;
            }
            if (!tok_fn_copy) {
                tok_fn_copy = makeTokenizerBridgeForPlugin(draft_plugin);
            }
            if (!tok_fn_copy) {
                tok_fn_copy = makeTokenizerBridgeForPlugin(target_plugin);
            }

            constexpr float kPeak     =  5.0f;
            constexpr float kBaseline = -5.0f;
            draft_result.vocab_size = vocab_size;

            if (!tok_fn_copy) {
                spdlog::info("Remote draft shard '{}' returned text but no tokenizer bridge "
                             "is available; retrying with the local draft model",
                             remote_shard.shard_id);
            } else {
                try {
                    const std::vector<int> tok_ids =
                        tok_fn_copy(remote_text, vocab_size);
                    if (!tok_ids.empty()) {
                        for (size_t i = 0; i < K; ++i) {
                            const int tid = (i < tok_ids.size())
                                ? tok_ids[i] : 0;
                            draft_result.tokens.push_back(tid);
                            std::vector<float> row(vocab_size, kBaseline);
                            const size_t idx = static_cast<size_t>(
                                std::max(0, std::min(tid,
                                    static_cast<int>(vocab_size) - 1)));
                            row[idx] = kPeak;
                            draft_result.logits.push_back(std::move(row));
                        }
                        spdlog::debug("Remote draft: TokenizerFn produced {} "
                                      "token IDs", tok_ids.size());
                    } else {
                        spdlog::warn("TokenizerFn returned empty token list for remote draft "
                                     "text — retrying with the local draft model");
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("TokenizerFn threw for remote draft text: {} — retrying "
                                 "with the local draft model", e.what());
                }
            }
        }
    }

    // ── Local draft path (fallback or primary) ────────────────────────────
    // STUB #261 — Production Injection Point (wired by
    //   InferenceEngineEnhanced::trySpeculativeGeneration, 2026-08-27)
    //
    // When a TokenizerFn is registered on this engine, it is bridged into
    // ILLMPlugin::setDefaultGenerateDraftTokensFn() so that the plugin's
    // generateDraftTokens() calls the real tokenizer instead of the byte-modulo
    // heuristic.  The bridge lambda:
    //   1. Captures draft_plugin by value (shared_ptr copy) — safe across threads.
    //   2. Calls draft_plugin->generate() to obtain draft text.
    //   3. Runs the TokenizerFn over that text to produce real token IDs.
    //   4. Falls back to byte-modulo if the TokenizerFn throws or returns empty.
    // After the call the injected fn is cleared to avoid global-state pollution
    // (ILLMPlugin::s_default_draft_fn_ is process-wide).
    if (draft_result.tokens.empty()) {
        InferenceRequest draft_request = request;
        draft_request.stream_callback  = nullptr;

        // ── Bridge: inject TokenizerFn into ILLMPlugin when available ────
        TokenizerFn tok_fn_copy;
        {
            std::lock_guard<std::mutex> lk(tokenizer_fn_mutex_);
            tok_fn_copy = tokenizer_fn_;
        }
        if (!tok_fn_copy) {
            tok_fn_copy = makeTokenizerBridgeForPlugin(draft_plugin);
        }
        if (!tok_fn_copy) {
            tok_fn_copy = makeTokenizerBridgeForPlugin(target_plugin);
        }

        if (tok_fn_copy) {
            // Capture draft_plugin by value so the lambda is self-contained and
            // safe even if this method returns before the fn is cleared.
            auto local_draft_plugin   = draft_plugin;
            const size_t vocab_cap    = vocab_size;
            constexpr float kPeak     =  5.0f;
            constexpr float kBaseline = -5.0f;

            ILLMPlugin::setDefaultGenerateDraftTokensFn(
                [local_draft_plugin, tok_fn_copy, vocab_cap,
                 kPeak, kBaseline](
                    const InferenceRequest& req,
                    size_t k,
                    size_t vocab_size_hint) -> ILLMPlugin::DraftTokensResult
                {
                    const size_t vocab = (vocab_size_hint > 0)
                        ? vocab_size_hint : vocab_cap;

                    // Step 1: obtain draft text from the plugin.
                    InferenceRequest gen_req = req;
                    gen_req.max_tokens      = static_cast<int>(k);
                    gen_req.stream_callback = nullptr;
                    const auto resp = local_draft_plugin->generate(gen_req);

                    // Step 2: tokenize draft text.
                    ILLMPlugin::DraftTokensResult result;
                    result.vocab_size = vocab;
                    bool used_real_tokenizer = false;

                    try {
                        const auto ids = tok_fn_copy(resp.text, vocab);
                        if (!ids.empty()) {
                            result.tokens.reserve(k);
                            result.logits.reserve(k);
                            for (size_t i = 0; i < k; ++i) {
                                const int tid = (i < ids.size()) ? ids[i] : 0;
                                result.tokens.push_back(tid);
                                std::vector<float> row(vocab, kBaseline);
                                const size_t idx = static_cast<size_t>(
                                    std::max(0, std::min(tid,
                                        static_cast<int>(vocab) - 1)));
                                row[idx] = kPeak;
                                result.logits.push_back(std::move(row));
                            }
                            used_real_tokenizer = true;
                            spdlog::debug("Local draft (STUB #261 bridge): "
                                          "TokenizerFn produced {} token IDs",
                                          ids.size());
                        } else {
                            spdlog::warn("Local draft (STUB #261 bridge): "
                                         "TokenizerFn returned empty list — "
                                         "falling back to byte-modulo");
                        }
                    } catch (const std::exception& ex) {
                        spdlog::warn("Local draft (STUB #261 bridge): "
                                     "TokenizerFn threw: {} — falling back to "
                                     "byte-modulo", ex.what());
                    }

                    // Step 3: byte-modulo fallback when tokenizer unavailable.
                    if (!used_real_tokenizer) {
                        const std::string& text = resp.text;
                        result.tokens.clear();
                        result.logits.clear();
                        result.tokens.reserve(k);
                        result.logits.reserve(k);
                        for (size_t i = 0; i < k; ++i) {
                            const size_t tid_raw = (i < text.size())
                                ? (static_cast<size_t>(
                                       static_cast<unsigned char>(text[i])) %
                                   vocab)
                                : 0u;
                            const int tid = static_cast<int>(std::min(
                                tid_raw,
                                static_cast<size_t>(
                                    std::numeric_limits<int>::max())));
                            result.tokens.push_back(tid);
                            std::vector<float> row(vocab, kBaseline);
                            row[static_cast<size_t>(tid)] = kPeak;
                            result.logits.push_back(std::move(row));
                        }
                    }
                    return result;
                });
        }

        try {
            draft_result = draft_plugin->generateDraftTokens(
                draft_request, K, vocab_size);
        } catch (const std::exception& e) {
            // Always clear the injected fn even on failure.
            if (tok_fn_copy) {
                ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);
            }
            spdlog::warn("Draft model generateDraftTokens failed: {} — "
                         "falling back to target", e.what());
            return false;
        }

        // Clear the injected fn to prevent global state pollution.
        if (tok_fn_copy) {
            ILLMPlugin::setDefaultGenerateDraftTokensFn(nullptr);
        }
    }

    if (draft_result.tokens.empty()) {
        spdlog::debug("Draft model returned no tokens — falling back to target");
        return false;
    }

    // ── Target logit estimation ───────────────────────────────────────────
    // STUB/SIMULATION NOTE:
    // Purpose: When no TargetLogitsFn is injected, a single generate(max_tokens=1)
    //          call is used to obtain the target's most-likely next token, and
    //          peaked distributions (kPeak / kBaseline) are synthesised for all
    //          K+1 positions as a placeholder logit matrix.
    // Activation: Active when setTargetLogitsFn() has not been called, or when
    //             the injected function returns a wrong-shape result.
    // Production Delta: The peaked heuristic overstates target confidence and
    //                   skips the K-token forward pass; a real implementation
    //                   would run a single batched forward pass over all K draft
    //                   tokens and return the true conditional logit matrix.
    // Production Injection Point (W9-17, 2026-08-26):
    //   Inject via setTargetLogitsFn() at engine startup.  The registered fn is
    //   called FIRST; the peaked-distribution heuristic is the documented fallback
    //   only when no fn is set or the fn returns a wrong-shape result or throws.
    //   Shape contract: exactly K+1 rows of vocab_size floats.
    // Try the injected TargetLogitsFn first; fall back to the single-token
    // peaked-distribution heuristic when no fn is set.
    TargetLogitsFn target_logits_fn_copy;
    {
        std::lock_guard<std::mutex> lk(target_logits_fn_mutex_);
        target_logits_fn_copy = target_logits_fn_;
    }

    std::vector<std::vector<float>> target_logit_matrix;
    bool used_injected_logits = false;
    if (target_logits_fn_copy) {
        try {
            target_logit_matrix = target_logits_fn_copy(request, K, vocab_size, target_plugin);
            // Validate output size: must be exactly K+1 rows of vocab_size columns.
            if (target_logit_matrix.size() == K + 1) {
                bool valid = true;
                for (const auto& row : target_logit_matrix) {
                    if (row.size() != vocab_size) { valid = false; break; }
                }
                used_injected_logits = valid;
                if (!valid) {
                    target_logit_matrix.clear();
                    spdlog::warn("TargetLogitsFn returned wrong shape — "
                                 "falling back to heuristic");
                }
            } else {
                const size_t actual_rows = target_logit_matrix.size();
                target_logit_matrix.clear();
                spdlog::warn("TargetLogitsFn returned {} rows (expected {}) — "
                             "falling back to heuristic",
                             actual_rows, K + 1);
            }
        } catch (const std::exception& e) {
            target_logit_matrix.clear();
            spdlog::warn("TargetLogitsFn threw: {} — falling back to heuristic",
                         e.what());
        }
    }

    if (!used_injected_logits) {
        // Built-in heuristic: single generate(max_tokens=1) call to obtain the
        // target's most-likely next token; peaked distributions for all K+1 rows.
        constexpr float kTargetPeak     =  5.0f;
        constexpr float kTargetBaseline = -5.0f;

        int target_pred_token = 0;
        {
            InferenceRequest one_tok_req = request;
            one_tok_req.max_tokens      = 1;
            one_tok_req.stream_callback = nullptr;
            try {
                const auto tgt_resp = target_plugin->generate(one_tok_req);
                if (!tgt_resp.text.empty()) {
                    target_pred_token =
                        static_cast<int>(static_cast<unsigned char>(tgt_resp.text[0])) %
                        vocab_size_int;
                }
            } catch (...) {
                THEMIS_WARN("inference_engine_enhanced: unhandled exception caught");
                // Non-fatal: keep target_pred_token = 0.
            }
        }

        auto make_target_row = [&](int peak_token) {
            std::vector<float> row(vocab_size, kTargetBaseline);
            row[static_cast<size_t>(
                std::max(0, peak_token) % vocab_size_int)] = kTargetPeak;
            return row;
        };

        target_logit_matrix.resize(K + 1);
        for (size_t i = 0; i < K; ++i) {
            target_logit_matrix[i] = make_target_row(target_pred_token);
        }
        // Bonus position: use a token shifted by 1 to distinguish from verification.
        target_logit_matrix[K] = make_target_row(
            (target_pred_token + 1) % vocab_size_int);
    }

    // ── Acceptance / rejection loop ───────────────────────────────────────
    SpeculativeDecoder::VerifyResult verify_result;
    try {
        verify_result = speculative_decoder_->verify(
            draft_result.tokens, draft_result.logits, target_logit_matrix);
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
