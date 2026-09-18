/**
 * @file embedding_cache.h
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
#include <optional>
#include <memory>
#include <chrono>
#include "cache/aligned_vector_allocator.h"

namespace themis {

class VectorIndexManager;
struct EmbeddingCacheImpl;  // Forward declaration for pimpl

class EmbeddingCache {
public:
    struct Config {
        size_t max_entries = 100000;         // Max cached embeddings
        int ttl_seconds = 3600;              // Cache TTL (1 hour)
        float similarity_threshold = 0.95f;  // Min similarity for cache hit
        size_t embedding_dim = 1536;         // OpenAI ada-002 dimension
        bool use_vector_index = true;        // Use HNSW for fast lookup
        std::string cache_dir = "/tmp/themis_embedding_cache";  // Cache storage directory
    };
    
    struct CacheEntry {
        std::string query_text;
        // v1.6.0: 32-byte aligned for efficient AVX2/AVX-512 SIMD operations
        // Reduces unaligned load penalties in distance calculations by ~5-15%
        cache::AlignedVector<float> embedding;
        std::string metadata;          // JSON metadata
        int64_t timestamp_ms;
        int64_t access_count = 0;
        float last_similarity = 0.0f;  // Similarity of last hit
    };
    
    struct CacheStats {
        uint64_t hit_count = 0;
        uint64_t miss_count = 0;
        uint64_t total_entries = 0;
        double hit_rate = 0.0;
        double avg_similarity = 0.0;   // Average similarity on cache hits
        double cost_savings_usd = 0.0; // Estimated cost savings
    };
    
    /**
     * @brief Embedding Cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit EmbeddingCache(const Config& config);
    ~EmbeddingCache();
    
    EmbeddingCache(const EmbeddingCache&) = delete;
    EmbeddingCache& operator=(const EmbeddingCache&) = delete;
    EmbeddingCache(EmbeddingCache&&) noexcept = default;
    EmbeddingCache& operator=(EmbeddingCache&&) noexcept = default;
    
    /**
     * @brief Query.
     * @param[in] query_embedding Input parameter.
     * @return Return value.
     */
    std::optional<CacheEntry> query(const std::vector<float>& query_embedding) const;
    
    bool store(const std::string& query_text, 
               const std::vector<float>& embedding,
               const std::string& metadata = "");
    
    CacheStats getStats() const { return stats_; }
    
    /**
     * @brief Clear Expired.
     * @return Return value.
     */
    uint64_t clearExpired();
    
    /**
     * @brief Clear.
     */
    void clear();

private:
    Config config_;
    mutable CacheStats stats_;
    mutable std::unique_ptr<EmbeddingCacheImpl> impl_;  // pimpl for vector index + entries
    
    /**
     * @brief Is Expired.
     * @param[in] entry Input parameter.
     * @return True when the operation succeeds.
     */
    bool isExpired(const CacheEntry& entry) const;
    
    /**
     * @brief Get Current Timestamp Ms.
     * @return Return value.
     */
    int64_t getCurrentTimestampMs() const;
};

} // namespace themis
