/**
 * @file llm_prefix_cache.h
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
#include "utils/clock.h"

namespace themis {
namespace llm {

struct PrefixCacheEntry {
    /**
     * @brief Prefix Cache Entry.
     * @return Return value.
     */
    virtual ~PrefixCacheEntry() = default;
    std::string prefix;
    std::vector<float> embedding;
    std::vector<int> token_ids;
    size_t usage_count = 0;
    std::chrono::system_clock::time_point last_used;
    
    // KV cache data (if precomputed)
    std::vector<float> precomputed_kv;
    bool has_precomputed_kv = false;

    // The generated response text associated with this cached prompt.
    // Empty for prewarm-only entries (no response has been generated yet).
    std::string generated_text;
};

struct PrefixCacheStatistics {
    /**
     * @brief Prefix Cache Statistics.
     * @return Return value.
     */
    virtual ~PrefixCacheStatistics() = default;
    size_t hits = 0;
    size_t misses = 0;
    size_t total_entries = 0;
    size_t total_tokens_saved = 0;
    double avg_similarity = 0.0;
    double avg_lookup_time_ms = 0.0;
    
    double getHitRate() const {
        if (hits + misses == 0) {
          return 0.0;
        }
        return static_cast<double>(hits) / (hits + misses);
    }
};

class LLMPrefixCache {
public:
    struct Config {
        double similarity_threshold = 0.95;  // 95% similarity required
        size_t max_entries = 1000;           // Maximum cached prefixes
        size_t min_prefix_length = 20;       // Minimum prefix length to cache
        int ttl_seconds = 7200;              // 2 hours TTL
        bool enable_kv_caching = true;       // Precompute KV cache
        std::shared_ptr<utils::Clock> clock = nullptr;  // Injectable clock (uses SystemClock if null)
        std::string cache_dir;
    };
    
    /**
     * @brief LLMPrefix Cache.
     * @param[in] cache_name Name of the cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LLMPrefixCache(const std::string& cache_name, const Config& config);
    ~LLMPrefixCache();
    
    void put(const std::string& prefix,
             const std::vector<int>& tokens,
             const std::vector<float>& embedding,
             const std::vector<float>& precomputed_kv = {},
             const std::string& generated_text = {});
    
    /**
     * @brief Get.
     * @param[in] text Input parameter.
     * @param[in] embedding Input parameter.
     * @return Return value.
     */
    std::optional<PrefixCacheEntry> get(const std::string& text,
                                         const std::vector<float>& embedding);
    
    /**
     * @brief Get Longest Match.
     * @param[in] text Input parameter.
     * @param[in] embedding Input parameter.
     * @return Return value.
     */
    std::optional<PrefixCacheEntry> getLongestMatch(const std::string& text,
                                                     const std::vector<float>& embedding);
    
    /**
     * @brief Touch.
     * @param[in] prefix Input parameter.
     */
    void touch(const std::string& prefix);
    
    /**
     * @brief Invalidate By Pattern.
     * @param[in] pattern Input parameter.
     */
    void invalidateByPattern(const std::string& pattern);
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    PrefixCacheStatistics getStatistics() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis

