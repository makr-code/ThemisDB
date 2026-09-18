/**
 * @file embedding_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <optional>
#include <mutex>

// Forward declaration for llama.cpp types
struct llama_model;
struct llama_context;

namespace themis {
namespace llm {
namespace lora {

struct EmbeddingCache {
    /**
     * @brief Embedding Cache.
     * @return Return value.
     */
    virtual ~EmbeddingCache() = default;
    std::string text;
    std::vector<float> embedding;  // Real embedding from model, not hash-based
    std::chrono::system_clock::time_point cached_at;
    size_t access_count = 0;
    
    // Check if cache entry is expired
    bool isExpired(std::chrono::seconds ttl) const {
        auto now = std::chrono::system_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - cached_at);
        return age > ttl;
    }
};

struct EmbeddingCacheStats {
    /**
     * @brief Embedding Cache Stats.
     * @return Return value.
     */
    virtual ~EmbeddingCacheStats() = default;
    size_t total_requests = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    size_t total_entries = 0;
    size_t memory_bytes = 0;
    
    float hitRate() const {
        if (total_requests == 0) {
          return 0.0f;
        }
        return static_cast<float>(cache_hits) / static_cast<float>(total_requests);
    }
};

class EmbeddingProvider {
public:
    struct Config {
        size_t max_cache_entries = 10000;     // Maximum cached embeddings
        std::chrono::seconds cache_ttl{3600}; // Cache time-to-live
        bool enable_cache = true;              // Enable caching
        std::string cache_file;                // Optional cache persistence file
        size_t batch_size = 32;                // Batch size for embedding generation
    };
    
    /**
     * @brief Embedding Provider.
     * @param[in,out] model Input/output parameter.
     * @param[in,out] context Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit EmbeddingProvider(
        llama_model* model,
        llama_context* context,
        const Config& config
    );
    
    ~EmbeddingProvider();
    
    // Disable copy
    EmbeddingProvider(const EmbeddingProvider&) = delete;
    EmbeddingProvider& operator=(const EmbeddingProvider&) = delete;
    
    /**
     * @brief Get Embedding.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<float> getEmbedding(const std::string& text);
    
    /**
     * @brief Get Embeddings.
     * @param[in] texts Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<float>> getEmbeddings(const std::vector<std::string>& texts);
    
    /**
     * @brief Build Embedding Cache.
     * @param[in] training_texts Input parameter.
     * @param[in,out] cache_out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool buildEmbeddingCache(
        const std::vector<std::string>& training_texts,
        std::vector<EmbeddingCache>& cache_out
    );
    
    /**
     * @brief Get Embedding Dim.
     * @return Return value.
     */
    size_t getEmbeddingDim() const;
    
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    EmbeddingCacheStats getCacheStats() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    /**
     * @brief Save Cache.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveCache(const std::string& filepath);
    
    /**
     * @brief Load Cache.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadCache(const std::string& filepath);
    
    bool isCacheEnabled() const { return config_.enable_cache; }
    
    /**
     * @brief Enable Cache.
     * @param[in] enable Input parameter.
     * @details Implements enableCache without additional internal calls.
     */
    void enableCache(bool enable) { config_.enable_cache = enable; }
    
private:
    llama_model* model_;
    llama_context* context_;
    Config config_;
    
    // Cache: text -> embedding
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, EmbeddingCache> cache_;
    mutable EmbeddingCacheStats cache_stats_;
    
    /**
     * @brief Extract Embedding From Tokens.
     * @param[in] tokens Input parameter.
     * @return Return value.
     */
    std::vector<float> extractEmbeddingFromTokens(const std::vector<int>& tokens);
    
    /**
     * @brief Evict Cache If Needed.
     */
    void evictCacheIfNeeded();
    
    /**
     * @brief Get Cached Embedding.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<float>> getCachedEmbedding(const std::string& text);
    
    /**
     * @brief Add To Cache.
     * @param[in] text Input parameter.
     * @param[in] embedding Input parameter.
     */
    void addToCache(const std::string& text, const std::vector<float>& embedding);
};

} // namespace lora
} // namespace llm
} // namespace themis

