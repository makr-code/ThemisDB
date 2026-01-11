#include "llm/model_loader.h"
#include "utils/error_registry.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <llama.h>

namespace fs = std::filesystem;

namespace themis {
namespace llm {

LazyModelLoader::LazyModelLoader(const Config& config)
    : config_(config) {
    spdlog::info("LazyModelLoader initialized (Ollama-style):");
    spdlog::info("  Max VRAM: {} MB", config_.max_vram_mb);
    spdlog::info("  Max models: {}", config_.max_models);
    spdlog::info("  Model TTL: {} seconds", config_.model_ttl.count());
    spdlog::info("  Lazy loading: {}", config_.enable_lazy_load ? "enabled" : "disabled");
}

LazyModelLoader::~LazyModelLoader() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Unload all models properly
    for (auto& [id, model] : models_) {
        spdlog::info("Unloading model: {}", id);
        
        // Free llama.cpp resources if they exist
        if (model->context_handle) {
            llama_free(reinterpret_cast<llama_context*>(model->context_handle));
            model->context_handle = nullptr;
        }
        if (model->model_handle) {
            llama_free_model(reinterpret_cast<llama_model*>(model->model_handle));
            model->model_handle = nullptr;
        }
    }
    models_.clear();
}

CachedModel* LazyModelLoader::getOrLoadModel(
    const std::string& model_id,
    const std::string& model_path,
    const json& load_config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        spdlog::debug("Model cache hit: {}", model_id);
        cache_hits_++;
        
        // Update usage
        it->second->last_used = std::chrono::system_clock::now();
        it->second->use_count++;
        
        return it->second.get();
    }
    
    spdlog::info("Model cache miss: {} - loading lazily", model_id);
    cache_misses_++;
    
    // Check if we need to evict
    if (models_.size() >= config_.max_models) {
        spdlog::info("Model cache full, evicting LRU");
        evictLRU();
    }
    
    // Load model
    return loadModelInternal(model_id, model_path, load_config);
}

bool LazyModelLoader::preloadModel(
    const std::string& model_id,
    const std::string& model_path,
    const json& load_config
) {
    spdlog::info("Preloading model in background: {}", model_id);
    
    // TODO: Implement async loading in v1.3.0
    // For now, just do synchronous load
    auto* model = getOrLoadModel(model_id, model_path, load_config);
    return model != nullptr;
}

bool LazyModelLoader::unloadModel(const std::string& model_id, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return false;
    }
    
    if (it->second->keep_loaded && !force) {
        spdlog::warn("Model {} is pinned, cannot unload (use force=true)", model_id);
        return false;
    }
    
    spdlog::info("Unloading model: {}", model_id);

    // Free llama.cpp resources
    if (it->second->context_handle) {
        llama_free(reinterpret_cast<llama_context*>(it->second->context_handle));
        it->second->context_handle = nullptr;
    }
    if (it->second->model_handle) {
        llama_free_model(reinterpret_cast<llama_model*>(it->second->model_handle));
        it->second->model_handle = nullptr;
    }

    // Update memory usage
    total_vram_mb_ -= it->second->vram_mb;
    total_ram_mb_ -= it->second->ram_mb;
    
    models_.erase(it);
    return true;
}

void LazyModelLoader::pinModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        it->second->keep_loaded = true;
        spdlog::info("Model pinned in memory: {}", model_id);
    }
}

void LazyModelLoader::unpinModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        it->second->keep_loaded = false;
        spdlog::info("Model unpinned: {}", model_id);
    }
}

bool LazyModelLoader::isModelLoaded(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return models_.find(model_id) != models_.end();
}

std::optional<ModelInfo> LazyModelLoader::getModelInfo(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return std::nullopt;
    }
    
    return it->second->info;
}

std::vector<std::string> LazyModelLoader::listLoadedModels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    result.reserve(models_.size());
    
    for (const auto& [id, _] : models_) {
        result.push_back(id);
    }
    
    return result;
}

size_t LazyModelLoader::evictLRU(size_t target_vram_mb) {
    // Already locked by caller
    
    if (models_.empty()) {
        return 0;
    }
    
    // Find LRU unpinned model
    CachedModel* lru_model = nullptr;
    std::string lru_id;
    auto oldest_time = std::chrono::system_clock::now();
    
    for (auto& [id, model] : models_) {
        if (model->keep_loaded) {
            continue;  // Skip pinned models
        }
        
        if (model->last_used < oldest_time) {
            oldest_time = model->last_used;
            lru_model = model.get();
            lru_id = id;
        }
    }
    
    if (!lru_model) {
        spdlog::warn("All models are pinned, cannot evict");
        return 0;
    }
    
    size_t freed_vram = lru_model->vram_mb;
    
    spdlog::info("Evicting LRU model: {} (freed {} MB VRAM)", lru_id, freed_vram);
    
    total_vram_mb_ -= lru_model->vram_mb;
    total_ram_mb_ -= lru_model->ram_mb;
    evictions_++;
    
    models_.erase(lru_id);
    
    return freed_vram;
}

size_t LazyModelLoader::evictExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t evicted = 0;
    
    std::vector<std::string> to_evict;
    
    for (const auto& [id, model] : models_) {
        if (model->keep_loaded) {
            continue;  // Skip pinned models
        }
        
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - model->last_used
        );
        
        if (age > config_.model_ttl) {
            to_evict.push_back(id);
        }
    }
    
    for (const auto& id : to_evict) {
        spdlog::info("Evicting expired model: {}", id);
        unloadModel(id, true);
        evicted++;
    }
    
    return evicted;
}

json LazyModelLoader::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json stats;
    stats["vram_used_mb"] = total_vram_mb_;
    stats["vram_max_mb"] = config_.max_vram_mb;
    stats["vram_usage_pct"] = (config_.max_vram_mb > 0) 
        ? (total_vram_mb_ * 100.0 / config_.max_vram_mb) : 0.0;
    
    stats["ram_used_mb"] = total_ram_mb_;
    stats["ram_max_mb"] = config_.max_ram_mb;
    
    stats["models_loaded"] = models_.size();
    stats["models_max"] = config_.max_models;
    
    return stats;
}

json LazyModelLoader::getCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json stats;
    stats["cache_hits"] = cache_hits_;
    stats["cache_misses"] = cache_misses_;
    stats["evictions"] = evictions_;
    stats["models_loaded"] = models_loaded_;
    
    if ((cache_hits_ + cache_misses_) > 0) {
        stats["hit_rate"] = static_cast<double>(cache_hits_) / 
                           (cache_hits_ + cache_misses_);
    } else {
        stats["hit_rate"] = 0.0;
    }
    
    return stats;
}

LazyModelLoader::Stats LazyModelLoader::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.cache_hits = cache_hits_;
    s.cache_misses = cache_misses_;
    s.evictions = evictions_;
    s.models_loaded = models_loaded_;
    return s;
}

CachedModel* LazyModelLoader::loadModelInternal(
    const std::string& model_id,
    const std::string& model_path,
    const json& config
) {
    // Already locked by caller
    spdlog::info("Loading model: {} from {}", model_id, model_path);

    auto model = std::make_unique<CachedModel>();
    model->model_id = model_id;
    model->model_path = model_path;
    model->loaded_at = std::chrono::system_clock::now();
    model->last_used = std::chrono::system_clock::now();
    model->use_count = 1;

    // Check if file exists
    if (!fs::exists(model_path)) {
        spdlog::error("Model file not found: {}", model_path);
        return nullptr;
    }

    size_t size_bytes = static_cast<size_t>(fs::file_size(model_path));

    // Initialize llama.cpp model parameters
    llama_model_params model_params = llama_model_default_params();
    
    // Configure GPU layers from config
    int n_gpu_layers = config.value("n_gpu_layers", config_.default_n_gpu_layers);
    model_params.n_gpu_layers = n_gpu_layers;
    
    // Enable Flash Attention if available and configured
    bool use_flash_attn = config.value("use_flash_attn", false);
    #ifdef LLAMA_FLASH_ATTN
    if (use_flash_attn) {
        model_params.flash_attn = true;
        spdlog::info("Flash Attention enabled for model: {}", model_id);
    }
    #else
    if (use_flash_attn) {
        spdlog::warn("Flash Attention requested but not available in this llama.cpp build");
    }
    #endif
    
    // Memory management
    model_params.use_mmap = config.value("use_mmap", true);
    model_params.use_mlock = config.value("use_mlock", false);
    
    // Load the model
    llama_model* lmodel = llama_load_model_from_file(model_path.c_str(), model_params);
    
    if (!lmodel) {
        spdlog::error("Failed to load model from file: {}", model_path);
        return nullptr;
    }
    
    // Initialize context parameters
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config.value("n_ctx", config_.default_n_ctx);
    ctx_params.n_batch = config.value("n_batch", 512);
    ctx_params.n_threads = config.value("n_threads", 8);
    
    // Configure RoPE scaling (Phase 3.1)
    if (config.value("rope_scaling_enabled", false)) {
        std::string method = config.value("rope_scaling_method", "yarn");
        int max_context = config.value("rope_max_context", 32768);
        int original_context = config.value("rope_original_context", 4096);
        
        // Calculate scaling factor
        float scale_factor = static_cast<float>(original_context) / static_cast<float>(max_context);
        
        if (method == "linear") {
            // Linear scaling: simple frequency scaling
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_LINEAR;
            ctx_params.rope_freq_scale = scale_factor;
            spdlog::info("RoPE Linear scaling: {} → {} tokens (scale: {:.4f})",
                        original_context, max_context, scale_factor);
        }
        else if (method == "ntk") {
            // NTK-Aware scaling: adjust base frequency
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_NONE;  // NTK uses freq_base
            float scaling_ratio = static_cast<float>(max_context) / static_cast<float>(original_context);
            ctx_params.rope_freq_base = 10000.0f * std::pow(scaling_ratio, 0.5f);
            spdlog::info("RoPE NTK scaling: {} → {} tokens (freq_base: {:.2f})",
                        original_context, max_context, ctx_params.rope_freq_base);
        }
        else if (method == "yarn") {
            // YaRN scaling: best quality for high factors
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_YARN;
            ctx_params.rope_freq_scale = scale_factor;
            
            // YaRN-specific parameters
            ctx_params.yarn_ext_factor = config.value("rope_yarn_ext_factor", 1.0f);
            ctx_params.yarn_attn_factor = config.value("rope_yarn_attn_factor", 1.0f);
            ctx_params.yarn_beta_fast = config.value("rope_yarn_beta_fast", 32.0f);
            ctx_params.yarn_beta_slow = config.value("rope_yarn_beta_slow", 1.0f);
            
            spdlog::info("RoPE YaRN scaling: {} → {} tokens (scale: {:.4f}, ext: {:.2f}, attn: {:.2f})",
                        original_context, max_context, scale_factor,
                        ctx_params.yarn_ext_factor, ctx_params.yarn_attn_factor);
        }
        else if (method == "dynamic") {
            // Dynamic scaling: adapts to input length
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_LINEAR;  // Use linear as base
            ctx_params.rope_freq_scale = scale_factor;
            spdlog::info("RoPE Dynamic scaling: {} → {} tokens (adaptive)",
                        original_context, max_context);
        }
        else {
            spdlog::warn("Unknown RoPE scaling method: {}, using YaRN", method);
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_YARN;
            ctx_params.rope_freq_scale = scale_factor;
        }
    }
    
    // Check for embeddings mode
    bool enable_embeddings = config.value("enable_embeddings", false);
    if (enable_embeddings) {
        ctx_params.embeddings = true;
        spdlog::info("Embeddings mode enabled for model: {}", model_id);
    }
    
    // Create context
    llama_context* lctx = llama_new_context_with_model(lmodel, ctx_params);
    
    if (!lctx) {
        spdlog::error("Failed to create context for model: {}", model_id);
        llama_free_model(lmodel);
        return nullptr;
    }

    // Populate model info
    model->info.name = model_id;
    model->info.model_id = model_id;
    model->info.path = model_path;
    model->info.format = "gguf";
    model->info.architecture = "llama";
    model->info.context_length = ctx_params.n_ctx;
    model->info.size_bytes = size_bytes;
    model->info.is_loaded = true;

    // Estimate VRAM/RAM usage
    size_t vram_mb = static_cast<size_t>(size_bytes / (1024ull * 1024ull));
    size_t ram_mb = vram_mb / 2;
    model->vram_mb = vram_mb;
    model->ram_mb = ram_mb;

    // Store opaque handles
    model->model_handle = reinterpret_cast<void*>(lmodel);
    model->context_handle = reinterpret_cast<void*>(lctx);

    auto* result = model.get();
    models_[model_id] = std::move(model);

    // Update memory accounting and stats
    total_vram_mb_ += vram_mb;
    total_ram_mb_ += ram_mb;
    models_loaded_++;

    spdlog::info("Model loaded successfully: {} ({} MB VRAM, {} GPU layers, Flash Attention: {})",
                 model_id, vram_mb, n_gpu_layers, use_flash_attn ? "ON" : "OFF");
    return result;
}

bool LazyModelLoader::hasCapacity(size_t vram_mb, size_t ram_mb) const {
    // Respect both memory budgets and max_models when set (0 means unlimited)
    const bool vram_ok = (config_.max_vram_mb == 0) || (total_vram_mb_ + vram_mb <= config_.max_vram_mb);
    const bool ram_ok = (config_.max_ram_mb == 0) || (total_ram_mb_ + ram_mb <= config_.max_ram_mb);
    const bool count_ok = models_.size() + 1 <= config_.max_models;
    return vram_ok && ram_ok && count_ok;
}

void LazyModelLoader::updateMemoryUsage() {
    // Recalculate from scratch
    total_vram_mb_ = 0;
    total_ram_mb_ = 0;
    
    for (const auto& [_, model] : models_) {
        total_vram_mb_ += model->vram_mb;
        total_ram_mb_ += model->ram_mb;
    }
}

} // namespace llm
} // namespace themis
