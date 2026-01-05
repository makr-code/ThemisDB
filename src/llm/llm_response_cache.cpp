#include "llm/llm_response_cache.h"
#include "llm/grafana_metrics.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <regex>
#include <cmath>
#include <unordered_set>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace themis {
namespace llm {

LLMResponseCache::LLMResponseCache(const std::string& cache_name, const Config& config)
    : cache_name_(cache_name), config_(config) {
    
    // Initialize EmbeddingCache for semantic similarity
    EmbeddingCache::Config emb_config;
    emb_config.max_entries = config_.max_entries;
    emb_config.ttl_seconds = config_.ttl_seconds;
    emb_config.similarity_threshold = config_.similarity_threshold;
    emb_config.embedding_dim = config_.embedding_dim;
    emb_config.use_vector_index = config_.use_vector_index;
    emb_config.cache_dir = config_.cache_dir;
    
    try {
        embedding_cache_ = std::make_unique<EmbeddingCache>(emb_config);
        THEMIS_INFO("LLMResponseCache '{}' initialized with EmbeddingCache (dim={}, threshold={})",
                    cache_name_, config_.embedding_dim, config_.similarity_threshold);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to initialize EmbeddingCache for LLMResponseCache '{}': {}",
                     cache_name_, e.what());
        // Cache will still work, but without semantic matching (falls back to exact match only)
    }
}

void LLMResponseCache::put(const std::string& prompt, const InferenceResponse& response) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Generate embedding for the prompt
    auto embedding = generateEmbedding(prompt);
    if (embedding.empty()) {
        THEMIS_WARN("Failed to generate embedding for prompt, skipping cache entry");
        return;
    }
    
    // Create metadata JSON with response info
    nlohmann::json metadata;
    metadata["tokens_generated"] = response.tokens_generated;
    metadata["inference_time_ms"] = response.inference_time_ms;
    metadata["model_id"] = response.model_id;
    
    // Store in embedding cache
    if (embedding_cache_) {
        std::string metadata_str = metadata.dump();
        if (embedding_cache_->store(prompt, embedding, metadata_str)) {
            // Store the response in our local map
            // Use a hash of the prompt as the key
            std::string entry_id = std::to_string(std::hash<std::string>{}(prompt));
            
            CachedEntry entry;
            entry.prompt = prompt;
            entry.response = response;
            entry.timestamp = std::chrono::system_clock::now();
            
            response_store_[entry_id] = entry;
            stats_.total_entries = response_store_.size();
            
            // Record cache size metric
            if (metrics_collector_) {
                metrics_collector_->recordCacheSize(cache_name_, stats_.total_entries / 1024.0);
            }
        }
    }
}

std::optional<InferenceResponse> LLMResponseCache::get(const std::string& prompt) {
    auto start = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Generate embedding for the query prompt
    auto query_embedding = generateEmbedding(prompt);
    if (query_embedding.empty()) {
        stats_.misses++;
        THEMIS_DEBUG("Cache miss: failed to generate embedding for prompt");
        if (metrics_collector_) {
            metrics_collector_->recordCacheMiss(cache_name_);
        }
        return std::nullopt;
    }
    
    // Try semantic similarity match using EmbeddingCache
    if (embedding_cache_) {
        auto cache_result = embedding_cache_->query(query_embedding);
        
        if (cache_result) {
            // Found a similar cached entry
            std::string entry_id = std::to_string(std::hash<std::string>{}(cache_result->query_text));
            auto it = response_store_.find(entry_id);
            
            if (it != response_store_.end()) {
                auto& entry = it->second;
                
                // Check if entry is expired
                if (!isExpired(entry)) {
                    stats_.hits++;
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    stats_.avg_lookup_time_ms = (stats_.avg_lookup_time_ms * (stats_.hits + stats_.misses - 1) + 
                                                  duration.count() / 1000.0) / (stats_.hits + stats_.misses);
                    
                    // Record cache hit
                    if (metrics_collector_) {
                        metrics_collector_->recordCacheHit(cache_name_);
                    }
                    
                    THEMIS_DEBUG("Cache hit: similarity={:.4f}, prompt='{}' matched to '{}'",
                                cache_result->last_similarity, 
                                prompt.substr(0, 50), 
                                cache_result->query_text.substr(0, 50));
                    
                    return entry.response;
                } else {
                    // Expired - remove it
                    response_store_.erase(it);
                    stats_.total_entries = response_store_.size();
                    
                    // Update cache size
                    if (metrics_collector_) {
                        metrics_collector_->recordCacheSize(cache_name_, stats_.total_entries / 1024.0);
                    }
                }
            }
        }
    }
    
    // Cache miss
    stats_.misses++;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats_.avg_lookup_time_ms = (stats_.avg_lookup_time_ms * (stats_.hits + stats_.misses - 1) + 
                                  duration.count() / 1000.0) / (stats_.hits + stats_.misses);
    
    // Record cache miss
    if (metrics_collector_) {
        metrics_collector_->recordCacheMiss(cache_name_);
    }
    
    THEMIS_DEBUG("Cache miss for prompt: '{}'", prompt.substr(0, 50));
    return std::nullopt;
}

size_t LLMResponseCache::invalidate(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::regex regex_pattern(pattern);
        size_t count = 0;
        
        for (auto it = response_store_.begin(); it != response_store_.end(); ) {
            if (std::regex_search(it->second.prompt, regex_pattern)) {
                it = response_store_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
        
        stats_.total_entries = response_store_.size();
        return count;
    } catch (const std::regex_error&) {
        return 0;
    }
}

void LLMResponseCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    response_store_.clear();
    if (embedding_cache_) {
        embedding_cache_->clear();
    }
    
    stats_.total_entries = 0;
    
    // Record cache cleared
    if (metrics_collector_) {
        metrics_collector_->recordCacheSize(cache_name_, 0.0);
    }
}

LLMResponseCache::CacheStatistics LLMResponseCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return stats_;
}

std::vector<float> LLMResponseCache::generateEmbedding(const std::string& prompt) const {
    if (prompt.empty()) {
        return {};
    }
    
    // For now, use a simple character-based embedding as a placeholder
    // In production, this should use an actual embedding model (e.g., via llama.cpp)
    std::vector<float> embedding(config_.embedding_dim, 0.0f);
    
    // Create a simple deterministic embedding based on the prompt
    // This is a placeholder - real implementation would use LLM embedding model
    std::hash<std::string> hasher;
    size_t hash = hasher(prompt);
    
    // Distribute hash across embedding dimensions
    for (size_t i = 0; i < config_.embedding_dim; ++i) {
        // Use different parts of the hash for each dimension
        size_t seed = hash ^ (i * 0x9e3779b9);
        // Normalize to [-1, 1] range
        embedding[i] = static_cast<float>((seed % 1000) - 500) / 500.0f;
    }
    
    // Normalize the embedding vector
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 0.0f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

bool LLMResponseCache::isExpired(const CachedEntry& entry) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);
    return age.count() > config_.ttl_seconds;
}

} // namespace llm
} // namespace themis
