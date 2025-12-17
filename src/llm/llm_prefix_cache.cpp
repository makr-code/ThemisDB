#include "llm/llm_prefix_cache.h"
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <regex>

namespace themis {
namespace llm {

/**
 * @brief Implementation using stub for EmbeddingCache
 * 
 * In production, this would use ThemisDB's actual EmbeddingCache
 * which provides HNSW-based similarity search over embeddings.
 */
class LLMPrefixCache::Impl {
public:
    explicit Impl(const std::string& name, const Config& cfg)
        : cache_name_(name), config_(cfg) {}
    
    void put(const std::string& prefix,
             const std::vector<int>& tokens,
             const std::vector<float>& embedding,
             const std::vector<float>& precomputed_kv) {
        if (prefix.length() < config_.min_prefix_length) {
            return;  // Too short to cache
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        PrefixCacheEntry entry;
        entry.prefix = prefix;
        entry.token_ids = tokens;
        entry.embedding = embedding;
        entry.usage_count = 1;
        entry.last_used = std::chrono::system_clock::now();
        
        if (config_.enable_kv_caching && !precomputed_kv.empty()) {
            entry.precomputed_kv = precomputed_kv;
            entry.has_precomputed_kv = true;
        }
        
        // Evict if at capacity
        if (cache_.size() >= config_.max_entries) {
            evictLRU();
        }
        
        cache_[prefix] = entry;
        
        // TODO: In production, add to EmbeddingCache HNSW index
        // embedding_cache_->addVector(prefix, embedding);
    }
    
    std::optional<PrefixCacheEntry> get(const std::string& text,
                                         const std::vector<float>& embedding) {
        auto start = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check for exact match first
        auto it = cache_.find(text);
        if (it != cache_.end()) {
            if (!isExpired(it->second)) {
                it->second.usage_count++;
                it->second.last_used = std::chrono::system_clock::now();
                stats_.hits++;
                updateLookupTime(start);
                return it->second;
            } else {
                cache_.erase(it);
            }
        }
        
        // TODO: In production, use EmbeddingCache for similarity search
        // auto similar = embedding_cache_->searchSimilar(embedding, 1, config_.similarity_threshold);
        // if (!similar.empty()) {
        //     auto& entry = cache_[similar[0].id];
        //     entry.usage_count++;
        //     entry.last_used = std::chrono::system_clock::now();
        //     stats_.hits++;
        //     updateLookupTime(start);
        //     return entry;
        // }
        
        // Stub: Linear search for similar embeddings
        double best_similarity = 0.0;
        std::optional<PrefixCacheEntry> best_match;
        
        for (auto& [key, entry] : cache_) {
            if (isExpired(entry)) continue;
            
            double similarity = computeSimilarity(embedding, entry.embedding);
            if (similarity >= config_.similarity_threshold && similarity > best_similarity) {
                best_similarity = similarity;
                best_match = entry;
            }
        }
        
        if (best_match) {
            // Update usage stats
            for (auto& [key, entry] : cache_) {
                if (entry.prefix == best_match->prefix) {
                    entry.usage_count++;
                    entry.last_used = std::chrono::system_clock::now();
                    break;
                }
            }
            stats_.hits++;
            stats_.avg_similarity = (stats_.avg_similarity * stats_.hits + best_similarity) / (stats_.hits + 1);
            updateLookupTime(start);
            return best_match;
        }
        
        stats_.misses++;
        updateLookupTime(start);
        return std::nullopt;
    }
    
    std::optional<PrefixCacheEntry> getLongestMatch(const std::string& text,
                                                     const std::vector<float>& embedding) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::optional<PrefixCacheEntry> longest;
        size_t longest_length = 0;
        
        for (auto& [key, entry] : cache_) {
            if (isExpired(entry)) continue;
            
            // Check if entry.prefix is a prefix of text
            if (text.length() >= entry.prefix.length() &&
                text.substr(0, entry.prefix.length()) == entry.prefix) {
                if (entry.prefix.length() > longest_length) {
                    longest_length = entry.prefix.length();
                    longest = entry;
                }
            }
        }
        
        if (longest) {
            stats_.hits++;
            stats_.total_tokens_saved += longest->token_ids.size();
        } else {
            stats_.misses++;
        }
        
        return longest;
    }
    
    void touch(const std::string& prefix) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(prefix);
        if (it != cache_.end()) {
            it->second.usage_count++;
            it->second.last_used = std::chrono::system_clock::now();
        }
    }
    
    void invalidateByPattern(const std::string& pattern) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::regex regex_pattern(pattern);
        
        auto it = cache_.begin();
        while (it != cache_.end()) {
            if (std::regex_search(it->first, regex_pattern)) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        stats_ = PrefixCacheStatistics{};
    }
    
    PrefixCacheStatistics getStatistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto stats = stats_;
        stats.total_entries = cache_.size();
        return stats;
    }
    
private:
    bool isExpired(const PrefixCacheEntry& entry) const {
        auto now = std::chrono::system_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - entry.last_used);
        return age.count() > config_.ttl_seconds;
    }
    
    void evictLRU() {
        if (cache_.empty()) return;
        
        auto oldest = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.last_used < oldest->second.last_used) {
                oldest = it;
            }
        }
        cache_.erase(oldest);
    }
    
    double computeSimilarity(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size() || a.empty()) return 0.0;
        
        // Cosine similarity
        double dot = 0.0, mag_a = 0.0, mag_b = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            mag_a += a[i] * a[i];
            mag_b += b[i] * b[i];
        }
        
        if (mag_a == 0.0 || mag_b == 0.0) return 0.0;
        return dot / (std::sqrt(mag_a) * std::sqrt(mag_b));
    }
    
    void updateLookupTime(const std::chrono::steady_clock::time_point& start) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double ms = duration.count() / 1000.0;
        
        size_t total = stats_.hits + stats_.misses;
        stats_.avg_lookup_time_ms = (stats_.avg_lookup_time_ms * (total - 1) + ms) / total;
    }
    
    std::string cache_name_;
    Config config_;
    std::unordered_map<std::string, PrefixCacheEntry> cache_;
    mutable std::mutex mutex_;
    PrefixCacheStatistics stats_;
};

LLMPrefixCache::LLMPrefixCache(const std::string& cache_name, const Config& config)
    : impl_(std::make_unique<Impl>(cache_name, config)) {}

void LLMPrefixCache::put(const std::string& prefix,
                          const std::vector<int>& tokens,
                          const std::vector<float>& embedding,
                          const std::vector<float>& precomputed_kv) {
    impl_->put(prefix, tokens, embedding, precomputed_kv);
}

std::optional<PrefixCacheEntry> LLMPrefixCache::get(const std::string& text,
                                                     const std::vector<float>& embedding) {
    return impl_->get(text, embedding);
}

std::optional<PrefixCacheEntry> LLMPrefixCache::getLongestMatch(const std::string& text,
                                                                 const std::vector<float>& embedding) {
    return impl_->getLongestMatch(text, embedding);
}

void LLMPrefixCache::touch(const std::string& prefix) {
    impl_->touch(prefix);
}

void LLMPrefixCache::invalidateByPattern(const std::string& pattern) {
    impl_->invalidateByPattern(pattern);
}

void LLMPrefixCache::clear() {
    impl_->clear();
}

PrefixCacheStatistics LLMPrefixCache::getStatistics() const {
    return impl_->getStatistics();
}

} // namespace llm
} // namespace themis
