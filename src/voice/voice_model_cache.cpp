/**
 * @file voice_model_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_model_cache.h"
#include <chrono>
#include <stdexcept>

namespace themis { namespace voice {

VoiceModelCache::VoiceModelCache(const ModelCacheConfig& config)
    : config_(config) {}

VoiceModelCache::~VoiceModelCache() {
    clear();
}

void VoiceModelCache::registerLoader(
    const std::string& model_type,
    ModelLoader loader,
    ModelUnloader unloader)
{
    std::lock_guard<std::mutex> lock(mutex_);
    loaders_[model_type] = std::move(loader);
    unloaders_[model_type] = std::move(unloader);
}

std::optional<CachedModel> VoiceModelCache::get(
    const std::string& model_id,
    const std::string& model_path,
    const std::string& model_type,
    const json& config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Cache hit
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        ++cache_hits_;
        it->second.last_used_ms = nowMs();
        ++it->second.use_count;

        // Auto-pin if configured and frequently used
        if (config_.pin_frequently_used && it->second.use_count >= config_.pin_threshold) {
            it->second.is_pinned = true;
        }

        touchLRU(model_id);
        return it->second;
    }

    // Cache miss
    ++cache_misses_;

    // Path traversal protection: reject unsafe model paths before loading
    if (!isSafeModelPath(model_path)) {
        return std::nullopt;
    }

    // Attempt to load via registered loader
    auto loader_it = loaders_.find(model_type);
    if (loader_it == loaders_.end()) {
        // No loader registered; return empty optional
        return std::nullopt;
    }

    void* handle = loader_it->second(model_path, config);

    CachedModel model;
    model.model_id   = model_id;
    model.model_path = model_path;
    model.model_type = model_type;
    model.handle     = handle;
    model.loaded_at_ms = nowMs();
    model.last_used_ms = model.loaded_at_ms;
    model.use_count  = 1;

    // Enforce memory limit (no memory_bytes set by default for loaded models)
    if (current_memory_bytes_ + model.memory_bytes > config_.max_memory_bytes && model.memory_bytes > 0) {
        size_t needed = (current_memory_bytes_ + model.memory_bytes) - config_.max_memory_bytes;
        size_t freed = 0;
        while (freed < needed) {
            size_t before = current_memory_bytes_;
            if (!evictLRUOne()) break;
            // current_memory_bytes_ is only decremented inside evictLRUOne() while the
            // same mutex is held, so before >= current_memory_bytes_ is guaranteed here.
            freed += before - current_memory_bytes_;
        }
    }

    // Enforce model count limit
    while (models_.size() >= config_.max_models) {
        if (!evictLRUOne()) break;
    }

    // Insert
    models_[model_id] = model;
    lru_order_.push_front(model_id);
    lru_map_[model_id] = lru_order_.begin();
    current_memory_bytes_ += model.memory_bytes;

    return models_[model_id];
}

bool VoiceModelCache::isCached(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return models_.count(model_id) > 0;
}

bool VoiceModelCache::insert(const CachedModel& model) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (models_.count(model.model_id)) {
        return false; // Already present
    }

    // Enforce count limit
    while (models_.size() >= config_.max_models) {
        if (!evictLRUOne()) break;
    }

    // Enforce memory limit
    if (model.memory_bytes > 0) {
        while (current_memory_bytes_ + model.memory_bytes > config_.max_memory_bytes) {
            if (!evictLRUOne()) break;
        }
    }

    models_[model.model_id] = model;
    lru_order_.push_front(model.model_id);
    lru_map_[model.model_id] = lru_order_.begin();
    current_memory_bytes_ += model.memory_bytes;
    return true;
}

bool VoiceModelCache::evict(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = models_.find(model_id);
    if (it == models_.end()) return false;

    // Call unloader if available
    auto unloader_it = unloaders_.find(it->second.model_type);
    if (unloader_it != unloaders_.end() && it->second.handle) {
        unloader_it->second(it->second.handle);
    }

    current_memory_bytes_ -= it->second.memory_bytes;
    models_.erase(it);

    auto lru_it = lru_map_.find(model_id);
    if (lru_it != lru_map_.end()) {
        lru_order_.erase(lru_it->second);
        lru_map_.erase(lru_it);
    }

    ++evictions_;
    return true;
}

bool VoiceModelCache::pin(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) return false;
    it->second.is_pinned = true;
    return true;
}

bool VoiceModelCache::unpin(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) return false;
    it->second.is_pinned = false;
    return true;
}

size_t VoiceModelCache::evictToFree([[maybe_unused]] size_t memory_needed) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t freed = 0;
    while (freed < memory_needed) {
        // Find least recently used non-pinned model
        size_t before = current_memory_bytes_;
        if (!evictLRUOne()) break;
        freed += before - current_memory_bytes_;
    }
    return freed;
}

void VoiceModelCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [id, model] : models_) {
        auto unloader_it = unloaders_.find(model.model_type);
        if (unloader_it != unloaders_.end() && model.handle) {
            unloader_it->second(model.handle);
        }
        ++evictions_;
    }

    models_.clear();
    lru_order_.clear();
    lru_map_.clear();
    current_memory_bytes_ = 0;
}

ModelCacheStats VoiceModelCache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    ModelCacheStats stats;
    stats.loaded_models = models_.size();
    stats.total_memory_bytes = current_memory_bytes_;
    stats.max_memory_bytes = config_.max_memory_bytes;
    stats.cache_hits = cache_hits_;
    stats.cache_misses = cache_misses_;
    stats.evictions = evictions_;

    uint64_t total_requests = cache_hits_ + cache_misses_;
    stats.hit_rate = (total_requests > 0) ? static_cast<double>(cache_hits_) / static_cast<double>(total_requests) : 0.0;

    for (const auto& [id, model] : models_) {
        if (model.is_pinned) ++stats.pinned_models;
    }

    return stats;
}

json VoiceModelCache::getDetailedStats() const {
    auto stats = getStats();
    json j;
    j["loaded_models"] = stats.loaded_models;
    j["pinned_models"] = stats.pinned_models;
    j["total_memory_bytes"] = stats.total_memory_bytes;
    j["max_memory_bytes"] = stats.max_memory_bytes;
    j["cache_hits"] = stats.cache_hits;
    j["cache_misses"] = stats.cache_misses;
    j["evictions"] = stats.evictions;
    j["hit_rate"] = stats.hit_rate;

    json models_arr = json::array();
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, model] : models_) {
        json m;
        m["model_id"] = model.model_id;
        m["model_type"] = model.model_type;
        m["memory_bytes"] = model.memory_bytes;
        m["use_count"] = model.use_count;
        m["is_pinned"] = model.is_pinned;
        models_arr.push_back(m);
    }
    j["models"] = models_arr;
    return j;
}

// ---- Private helpers ----

void VoiceModelCache::touchLRU(const std::string& model_id) {
    auto it = lru_map_.find(model_id);
    if (it != lru_map_.end()) {
        lru_order_.erase(it->second);
        lru_order_.push_front(model_id);
        it->second = lru_order_.begin();
    }
}

bool VoiceModelCache::evictLRUOne() {
    // Find the LRU non-pinned model (from back of list)
    for (auto rit = lru_order_.rbegin(); rit != lru_order_.rend(); ++rit) {
        const std::string& id = *rit;
        auto it = models_.find(id);
        if (it != models_.end() && !it->second.is_pinned) {
            // Call unloader
            auto unloader_it = unloaders_.find(it->second.model_type);
            if (unloader_it != unloaders_.end() && it->second.handle) {
                unloader_it->second(it->second.handle);
            }
            current_memory_bytes_ -= it->second.memory_bytes;
            models_.erase(it);
            lru_map_.erase(id);
            lru_order_.erase(std::next(rit).base());
            ++evictions_;
            return true;
        }
    }
    return false; // All models are pinned
}

int64_t VoiceModelCache::nowMs() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

bool VoiceModelCache::isSafeModelPath(const std::string& path) {
    if (path.empty()) return false;
    // Reject path traversal sequences
    if (path.find("..") != std::string::npos) return false;
    // Reject null bytes
    if (path.find('\0') != std::string::npos) return false;
    // Reject shell metacharacters that could be used in injection attacks
    static const std::string kForbiddenChars = ";|&$`!{}()\\";
    for (unsigned char c : path) {
        if (kForbiddenChars.find(static_cast<char>(c)) != std::string::npos) return false;
    }
    return true;
}

}} // namespace themis::voice
