/**
 * @file voice_model_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Model caching layer – Phase 10 production readiness
#pragma once
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>
#include <functional>
#include <cstdint>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Cached model handle (opaque pointer wrapper)
struct CachedModel {
    std::string model_id;
    std::string model_path;
    std::string model_type;   // "stt", "tts", "llm", "embedding"
    void* handle = nullptr;   // Opaque pointer to actual model (may be null for stubs)
    size_t memory_bytes = 0;
    int64_t loaded_at_ms = 0;
    int64_t last_used_ms = 0;
    uint64_t use_count = 0;
    bool is_pinned = false;   // Pinned models are not evicted
    json metadata;
};

// Model loader function type
using ModelLoader = std::function<void*(const std::string& path, const json& config)>;
// Model unloader function type
using ModelUnloader = std::function<void(void* handle)>;

// Cache configuration
struct ModelCacheConfig {
    size_t max_memory_bytes = 4ULL * 1024 * 1024 * 1024;  // 4 GB default
    size_t max_models = 10;
    bool enable_lru_eviction = true;
    bool enable_prefetch = false;
    int64_t ttl_ms = 0;  // 0 = no TTL, never expire by time alone
    bool pin_frequently_used = false;  // auto-pin models used > 100x
    size_t pin_threshold = 100;
};

// Cache statistics
struct ModelCacheStats {
    size_t loaded_models = 0;
    size_t pinned_models = 0;
    size_t total_memory_bytes = 0;
    size_t max_memory_bytes = 0;
    uint64_t cache_hits = 0;
    uint64_t cache_misses = 0;
    uint64_t evictions = 0;
    double hit_rate = 0.0;
};

// VoiceModelCache: Phase 10 – LRU model cache
class VoiceModelCache {
public:
    explicit VoiceModelCache(const ModelCacheConfig& config = {});
    ~VoiceModelCache();

    // Register loaders/unloaders for model types
    void registerLoader(const std::string& model_type, ModelLoader loader, ModelUnloader unloader);

    // Get or load a model. Returns the CachedModel if it exists (or can be loaded).
    std::optional<CachedModel> get(const std::string& model_id, const std::string& model_path,
                                   const std::string& model_type, const json& config = {});

    // Check if model is in cache
    bool isCached(const std::string& model_id) const;

    // Manually insert an already-loaded model
    bool insert(const CachedModel& model);

    // Remove a model from cache (calls unloader)
    bool evict(const std::string& model_id);

    // Pin a model so it won't be evicted
    bool pin(const std::string& model_id);
    bool unpin(const std::string& model_id);

    // Evict models to free memory_needed bytes
    size_t evictToFree(size_t memory_needed);

    // Clear entire cache
    void clear();

    // Statistics
    ModelCacheStats getStats() const;
    json getDetailedStats() const;

    // Path traversal protection: returns false if path contains "..", null bytes,
    // or shell metacharacters that could be used for injection attacks.
    static bool isSafeModelPath(const std::string& path);

private:
    ModelCacheConfig config_;
    mutable std::mutex mutex_;

    std::map<std::string, CachedModel> models_;                         // model_id → CachedModel
    std::list<std::string> lru_order_;                                  // front = most recently used
    std::map<std::string, std::list<std::string>::iterator> lru_map_;   // model_id → iterator

    std::map<std::string, ModelLoader> loaders_;
    std::map<std::string, ModelUnloader> unloaders_;

    size_t current_memory_bytes_ = 0;

    mutable uint64_t cache_hits_ = 0;
    mutable uint64_t cache_misses_ = 0;
    uint64_t evictions_ = 0;

    void touchLRU(const std::string& model_id);
    bool evictLRUOne();
    int64_t nowMs() const;
};

}} // namespace themis::voice
