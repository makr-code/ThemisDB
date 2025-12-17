#include "llm/model_loader.h"
#include <spdlog/spdlog.h>
#include <algorithm>

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
    
    // Unload all models
    for (auto& [id, model] : models_) {
        spdlog::info("Unloading model: {}", id);
        // TODO: Actual cleanup in v1.3.0
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
    
    // Update memory usage
    total_vram_mb_ -= it->second->vram_mb;
    total_ram_mb_ -= it->second->ram_mb;
    
    // TODO: Actual model cleanup in v1.3.0
    
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
    
    if ((cache_hits_ + cache_misses_) > 0) {
        stats["hit_rate"] = static_cast<double>(cache_hits_) / 
                           (cache_hits_ + cache_misses_);
    } else {
        stats["hit_rate"] = 0.0;
    }
    
    return stats;
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
    
    // TODO: Actual model loading in v1.3.0
    // For now, populate with dummy data
    model->info.name = model_id;
    model->info.path = model_path;
    model->info.format = "gguf";
    model->info.architecture = "llama";
    model->info.parameter_count = 7000000000;
    model->info.context_length = config_.default_n_ctx;
    
    // Estimate memory usage (placeholder)
    model->vram_mb = 4096;  // 4 GB for Q4 model
    model->ram_mb = 1024;   // 1 GB
    model->info.vram_required_mb = model->vram_mb;
    
    // Update totals
    total_vram_mb_ += model->vram_mb;
    total_ram_mb_ += model->ram_mb;
    
    auto* result = model.get();
    models_[model_id] = std::move(model);
    
    spdlog::info("Model loaded successfully: {} ({} MB VRAM)", 
                 model_id, result->vram_mb);
    
    return result;
}

bool LazyModelLoader::hasCapacity(size_t vram_mb, size_t ram_mb) const {
    return (total_vram_mb_ + vram_mb <= config_.max_vram_mb) &&
           (total_ram_mb_ + ram_mb <= config_.max_ram_mb);
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
