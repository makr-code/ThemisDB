#include "llm/llm_response_cache.h"
#include <algorithm>
#include <regex>
#include <cmath>

namespace themis {
namespace llm {

LLMResponseCache::LLMResponseCache(const std::string& cache_name, const Config& config)
    : cache_name_(cache_name), config_(config) {
    // TODO: v1.3.0 - Initialize actual SemanticCache here
    // For now, using in-memory map as stub
}

void LLMResponseCache::put(const std::string& prompt, const InferenceResponse& response) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    CachedEntry entry;
    entry.response = response;
    entry.timestamp = std::chrono::system_clock::now();
    
    // TODO: v1.3.0 - Generate actual embedding using EmbeddingCache
    // For now, using simplified hash-based embedding
    std::fill(std::begin(entry.embedding), std::end(entry.embedding), 0.0f);
    
    cache_store_[prompt] = entry;
    stats_.total_entries = cache_store_.size();
    
    // Enforce max_entries limit (LRU eviction)
    if (cache_store_.size() > config_.max_entries) {
        // Find oldest entry
        auto oldest = cache_store_.begin();
        for (auto it = cache_store_.begin(); it != cache_store_.end(); ++it) {
            if (it->second.timestamp < oldest->second.timestamp) {
                oldest = it;
            }
        }
        cache_store_.erase(oldest);
        stats_.total_entries = cache_store_.size();
    }
}

std::optional<InferenceResponse> LLMResponseCache::get(const std::string& prompt) {
    auto start = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Try exact match first
    auto it = cache_store_.find(prompt);
    if (it != cache_store_.end()) {
        if (!isExpired(it->second)) {
            stats_.hits++;
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            stats_.avg_lookup_time_ms = (stats_.avg_lookup_time_ms * (stats_.hits + stats_.misses - 1) + 
                                          duration.count() / 1000.0) / (stats_.hits + stats_.misses);
            return it->second.response;
        } else {
            // Expired - remove it
            cache_store_.erase(it);
            stats_.total_entries = cache_store_.size();
        }
    }
    
    // Try semantic similarity match
    float best_similarity = 0.0f;
    std::optional<InferenceResponse> best_match;
    
    for (const auto& [cached_prompt, entry] : cache_store_) {
        if (isExpired(entry)) continue;
        
        float similarity = calculateSimilarity(prompt, cached_prompt);
        if (similarity >= config_.similarity_threshold && similarity > best_similarity) {
            best_similarity = similarity;
            best_match = entry.response;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    if (best_match) {
        stats_.hits++;
        stats_.avg_lookup_time_ms = (stats_.avg_lookup_time_ms * (stats_.hits + stats_.misses - 1) + 
                                      duration.count() / 1000.0) / (stats_.hits + stats_.misses);
        return best_match;
    }
    
    stats_.misses++;
    stats_.avg_lookup_time_ms = (stats_.avg_lookup_time_ms * (stats_.hits + stats_.misses - 1) + 
                                  duration.count() / 1000.0) / (stats_.hits + stats_.misses);
    return std::nullopt;
}

size_t LLMResponseCache::invalidate(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::regex regex_pattern(pattern);
        size_t count = 0;
        
        for (auto it = cache_store_.begin(); it != cache_store_.end(); ) {
            if (std::regex_search(it->first, regex_pattern)) {
                it = cache_store_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
        
        stats_.total_entries = cache_store_.size();
        return count;
    } catch (const std::regex_error&) {
        return 0;
    }
}

void LLMResponseCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_store_.clear();
    stats_.total_entries = 0;
}

LLMResponseCache::CacheStatistics LLMResponseCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return stats_;
}

float LLMResponseCache::calculateSimilarity(const std::string& prompt1, const std::string& prompt2) const {
    // TODO: v1.3.0 - Use actual embedding similarity (cosine similarity)
    // For now, using simplified Jaccard similarity on words
    
    if (prompt1 == prompt2) return 1.0f;
    
    auto tokenize = [](const std::string& str) {
        std::unordered_set<std::string> tokens;
        std::string token;
        for (char c : str) {
            if (std::isalnum(c)) {
                token += std::tolower(c);
            } else if (!token.empty()) {
                tokens.insert(token);
                token.clear();
            }
        }
        if (!token.empty()) tokens.insert(token);
        return tokens;
    };
    
    auto tokens1 = tokenize(prompt1);
    auto tokens2 = tokenize(prompt2);
    
    if (tokens1.empty() || tokens2.empty()) return 0.0f;
    
    // Jaccard similarity
    size_t intersection = 0;
    for (const auto& token : tokens1) {
        if (tokens2.count(token)) intersection++;
    }
    
    size_t union_size = tokens1.size() + tokens2.size() - intersection;
    return static_cast<float>(intersection) / union_size;
}

bool LLMResponseCache::isExpired(const CachedEntry& entry) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);
    return age.count() > config_.ttl_seconds;
}

} // namespace llm
} // namespace themis
