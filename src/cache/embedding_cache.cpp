/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            embedding_cache.cpp                                ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:11:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     374                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cache/embedding_cache.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace themis {

// Cost estimation constants for embedding cache
namespace {
    constexpr double EMBEDDING_API_COST_PER_1K_TOKENS = 0.0001;  // $0.0001 per 1K tokens
    constexpr double TOKENS_PER_EMBEDDING = 0.75;                 // ~750 tokens per embedding
    constexpr float EARLY_TERMINATION_THRESHOLD = 0.99f;          // Stop searching if we find a near-perfect match
}

// Internal storage for cache entries
struct EmbeddingCacheImpl {
    std::unique_ptr<RocksDBWrapper> db;  // Keep DB alive
    std::unique_ptr<VectorIndexManager> vector_index;
    std::unordered_map<std::string, EmbeddingCache::CacheEntry> entries; // pk -> entry
    mutable std::mutex mutex;
    uint64_t next_id = 0;
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE;
    std::string cache_dir = "/tmp/themis_embedding_cache";  // Configurable cache directory
};

EmbeddingCache::EmbeddingCache(const Config& config)
    : config_(config)
    , impl_(std::make_unique<EmbeddingCacheImpl>()) {
    
    THEMIS_INFO("EmbeddingCache created: max_entries={}, ttl={}s, similarity_threshold={}",
                config_.max_entries, config_.ttl_seconds, config_.similarity_threshold);
    
    // Initialize vector index for fast similarity search
    if (config_.use_vector_index) {
        try {
            // Use configurable cache directory
            impl_->cache_dir = config_.cache_dir;
            RocksDBWrapper::Config db_config;
            db_config.db_path = impl_->cache_dir;
            db_config.create_if_missing = true;
            
            impl_->db = std::make_unique<RocksDBWrapper>(db_config);
            impl_->vector_index = std::make_unique<VectorIndexManager>(*impl_->db);
            
            // Store metric for distance-to-similarity conversion
            impl_->metric = VectorIndexManager::Metric::COSINE;
            
            // Initialize HNSW index with cache namespace
            auto status = impl_->vector_index->init(
                "embedding_cache",
                config_.embedding_dim,
                impl_->metric,
                16,   // M
                200,  // efConstruction
                64    // efSearch
            );
            
            if (status.ok) {
                THEMIS_INFO("Vector index initialized for fast cache lookup (HNSW)");
            } else {
                THEMIS_WARN("Vector index initialization failed: {}", status.message);
                impl_->vector_index.reset();
                impl_->db.reset();
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to create vector index for cache: {}", e.what());
            impl_->vector_index.reset();
            impl_->db.reset();
        }
    }
}

EmbeddingCache::~EmbeddingCache() = default;

std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(
    const std::vector<float>& query_embedding
) const {
    if (query_embedding.size() != config_.embedding_dim) {
        THEMIS_ERROR("Invalid embedding dimension: {} (expected {})",
                    query_embedding.size(), config_.embedding_dim);
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Search using vector index if available
    if (impl_->vector_index) {
        auto [status, results] = impl_->vector_index->searchKnn(query_embedding, 1);
        
        if (status.ok && !results.empty()) {
            const auto& result = results[0];
            
            // Convert distance to similarity based on metric
            float similarity;
            if (impl_->metric == VectorIndexManager::Metric::COSINE) {
                // For cosine: similarity = 1 - distance
                similarity = 1.0f - result.distance;
            } else if (impl_->metric == VectorIndexManager::Metric::DOT) {
                // For dot product: higher is better (already similarity)
                similarity = result.distance;
            } else {
                // For L2: convert to similarity (inverse distance)
                similarity = 1.0f / (1.0f + result.distance);
            }
            
            if (similarity >= config_.similarity_threshold) {
                // Cache hit!
                auto it = impl_->entries.find(result.pk);
                if (it != impl_->entries.end()) {
                    auto& entry = it->second;
                    
                    // Check if entry is expired
                    if (isExpired(entry)) {
                        // RACE CONDITION FIX #2: Remove from vector index before erasing from map
                        if (impl_->vector_index) {
                            impl_->vector_index->removeByPk(it->first);
                        }
                        impl_->entries.erase(it);
                        stats_.miss_count++;
                        THEMIS_DEBUG("Cache miss (expired entry)");
                        return std::nullopt;
                    }
                    
                    // Update entry stats
                    entry.access_count++;
                    entry.last_similarity = similarity;
                    
                    // Update cache stats
                    stats_.hit_count++;
                    stats_.avg_similarity = 
                        (stats_.avg_similarity * (stats_.hit_count - 1) + similarity) 
                        / stats_.hit_count;
                    stats_.hit_rate = static_cast<double>(stats_.hit_count) 
                        / (stats_.hit_count + stats_.miss_count);
                    
                    // Estimate cost savings
                    stats_.cost_savings_usd += EMBEDDING_API_COST_PER_1K_TOKENS * TOKENS_PER_EMBEDDING;
                    
                    THEMIS_DEBUG("Cache hit: pk={}, similarity={:.4f}, access_count={}",
                                result.pk, similarity, entry.access_count);
                    return entry;
                }
            }
        }
    } else {
        // Fallback: brute-force search through all entries
        float best_similarity = 0.0f;
        std::string best_pk;
        const float threshold = config_.similarity_threshold;
        
        for (const auto& [pk, entry] : impl_->entries) {
            if (isExpired(entry)) {
                continue;
            }
            
            // Compute cosine similarity
            float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
            for (size_t i = 0; i < query_embedding.size(); ++i) {
                dot += query_embedding[i] * entry.embedding[i];
                norm1 += query_embedding[i] * query_embedding[i];
                norm2 += entry.embedding[i] * entry.embedding[i];
            }
            
            float similarity = dot / (std::sqrt(norm1) * std::sqrt(norm2));
            
            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_pk = pk;
                
                // Early termination if we found a near-perfect match
                if (best_similarity >= EARLY_TERMINATION_THRESHOLD) {
                    break;
                }
            }
        }
        
        if (best_similarity >= config_.similarity_threshold) {
            auto& entry = impl_->entries[best_pk];
            entry.access_count++;
            entry.last_similarity = best_similarity;
            
            stats_.hit_count++;
            stats_.avg_similarity = 
                (stats_.avg_similarity * (stats_.hit_count - 1) + best_similarity) 
                / stats_.hit_count;
            stats_.hit_rate = static_cast<double>(stats_.hit_count) 
                / (stats_.hit_count + stats_.miss_count);
            stats_.cost_savings_usd += EMBEDDING_API_COST_PER_1K_TOKENS * TOKENS_PER_EMBEDDING;
            
            THEMIS_DEBUG("Cache hit (brute-force): pk={}, similarity={:.4f}",
                        best_pk, best_similarity);
            return entry;
        }
    }
    
    // Cache miss
    stats_.miss_count++;
    stats_.hit_rate = static_cast<double>(stats_.hit_count) 
        / (stats_.hit_count + stats_.miss_count);
    
    THEMIS_DEBUG("Cache miss");
    return std::nullopt;
}

bool EmbeddingCache::store(
    const std::string& query_text,
    const std::vector<float>& embedding,
    const std::string& metadata
) {
    if (embedding.size() != config_.embedding_dim) {
        THEMIS_ERROR("Invalid embedding dimension: {} (expected {})",
                    embedding.size(), config_.embedding_dim);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Check if cache is full
    if (impl_->entries.size() >= config_.max_entries) {
        // Evict oldest entry
        auto oldest_it = impl_->entries.end();
        int64_t oldest_time = std::numeric_limits<int64_t>::max();
        
        for (auto it = impl_->entries.begin(); it != impl_->entries.end(); ++it) {
            if (it->second.timestamp_ms < oldest_time) {
                oldest_time = it->second.timestamp_ms;
                oldest_it = it;
            }
        }
        
        if (oldest_it != impl_->entries.end()) {
            THEMIS_DEBUG("Cache full, evicting oldest entry: pk={}", oldest_it->first);
            
            // Remove from vector index
            if (impl_->vector_index) {
                impl_->vector_index->removeByPk(oldest_it->first);
            }
            
            impl_->entries.erase(oldest_it);
        }
    }
    
    // Generate unique PK for this entry
    std::string pk = "emb_" + std::to_string(impl_->next_id++);
    
    // Create cache entry
    CacheEntry entry;
    entry.query_text = query_text;
    entry.embedding = cache::AlignedVector<float>(embedding.begin(), embedding.end());
    entry.metadata = metadata;
    entry.timestamp_ms = getCurrentTimestampMs();
    entry.access_count = 0;
    entry.last_similarity = 1.0f;
    
    // Store in map
    impl_->entries[pk] = entry;
    
    // Add to vector index if available
    if (impl_->vector_index) {
        BaseEntity entity;
        entity.setPrimaryKey(pk);
        entity.setField("query_text", Value{std::string(query_text)});
        entity.setField("metadata", Value{std::string(metadata)});
        entity.setField("timestamp_ms", Value{static_cast<int64_t>(entry.timestamp_ms)});
        // Store embedding as vector<float>
        entity.setField("embedding", Value{embedding});
        
        auto status = impl_->vector_index->addEntity(entity, "embedding");
        if (!status.ok) {
            THEMIS_WARN("Failed to add entry to vector index: {}", status.message);
            // Continue anyway - entry is in map
        }
    }
    
    // Update stats
    stats_.total_entries = impl_->entries.size();
    
    THEMIS_DEBUG("Stored embedding in cache: pk={}, query='{}', metadata='{}'",
                pk, query_text.substr(0, 50), metadata);
    return true;
}

uint64_t EmbeddingCache::clearExpired() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    uint64_t cleared = 0;
    auto now_ms = getCurrentTimestampMs();
    
    // Scan and remove expired entries
    for (auto it = impl_->entries.begin(); it != impl_->entries.end(); ) {
        if (isExpired(it->second)) {
            // Remove from vector index
            if (impl_->vector_index) {
                impl_->vector_index->removeByPk(it->first);
            }
            
            it = impl_->entries.erase(it);
            cleared++;
        } else {
            ++it;
        }
    }
    
    // Update stats
    stats_.total_entries = impl_->entries.size();
    
    THEMIS_INFO("Cleared {} expired cache entries", cleared);
    return cleared;
}

void EmbeddingCache::clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->entries.clear();
    
    // Reinitialize vector index
    if (impl_->vector_index) {
        impl_->vector_index->shutdown();
        auto status = impl_->vector_index->init(
            "embedding_cache",
            config_.embedding_dim,
            VectorIndexManager::Metric::COSINE,
            16, 200, 64
        );
        if (!status.ok) {
            THEMIS_WARN("Failed to reinitialize vector index after clear: {}", status.message);
        }
    }
    
    stats_ = CacheStats{};
    THEMIS_INFO("Cache cleared");
}

bool EmbeddingCache::isExpired(const CacheEntry& entry) const {
    auto now_ms = getCurrentTimestampMs();
    auto age_ms = now_ms - entry.timestamp_ms;
    return age_ms > (config_.ttl_seconds * 1000);
}

int64_t EmbeddingCache::getCurrentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
}

} // namespace themis
