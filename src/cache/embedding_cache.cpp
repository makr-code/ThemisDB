/**
 * @file embedding_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cache/embedding_cache.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <shared_mutex>
#include <unordered_map>

#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"

namespace themis {

// Cost estimation constants for embedding cache
namespace {
constexpr double EMBEDDING_API_COST_PER_1K_TOKENS = 0.0001; // $0.0001 per 1K tokens
constexpr double TOKENS_PER_EMBEDDING             = 0.75;   // ~750 tokens per embedding
constexpr float EARLY_TERMINATION_THRESHOLD       = 0.99f;  // Stop searching if we find a near-perfect match
} // namespace

// Internal storage for cache entries
struct EmbeddingCacheImpl {
    std::unique_ptr<RocksDBWrapper> db; // Keep DB alive
    std::unique_ptr<VectorIndexManager> vector_index;
    std::unordered_map<std::string, EmbeddingCache::CacheEntry> entries; // pk -> entry

    // F-004: shared_mutex allows concurrent query() calls (all read-only under shared_lock).
    // store() / clearExpired() / clear() take unique_lock.
    mutable std::shared_mutex entry_mutex;
    uint64_t next_id                  = 0;
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE;
    std::string cache_dir             = "/tmp/themis_embedding_cache"; // Configurable cache directory

    // F-009: min-heap for O(log N) eviction. Entry = {timestamp_ms, pk}.
    // Entries are not removed from the heap on erase — use lazy deletion (check if
    // the pk still exists in entries with the same timestamp before evicting).
    using EvictionEntry = std::pair<int64_t, std::string>;
    std::priority_queue<EvictionEntry, std::vector<EvictionEntry>, std::greater<EvictionEntry>> eviction_heap;
};

EmbeddingCache::EmbeddingCache(const Config &config) : config_(config), impl_(std::make_unique<EmbeddingCacheImpl>()) {
    THEMIS_INFO("EmbeddingCache created: max_entries={}, ttl={}s, similarity_threshold={}", config_.max_entries,
                config_.ttl_seconds, config_.similarity_threshold);

    // Initialize vector index for fast similarity search
    if (config_.use_vector_index) {
        try {
            // Use configurable cache directory
            impl_->cache_dir = config_.cache_dir;
            RocksDBWrapper::Config db_config;
            db_config.db_path           = impl_->cache_dir;
            db_config.create_if_missing = true;

            impl_->db           = std::make_unique<RocksDBWrapper>(db_config);
            impl_->vector_index = std::make_unique<VectorIndexManager>(*impl_->db);

            // Store metric for distance-to-similarity conversion
            impl_->metric = VectorIndexManager::Metric::COSINE;

            // Initialize HNSW index with cache namespace
            auto status
                = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metric,
                                            16,  // M
                                            200, // efConstruction
                                            64   // efSearch
                );

            if (status.ok) {
                THEMIS_INFO("Vector index initialized for fast cache lookup (HNSW)");
            } else {
                THEMIS_WARN("Vector index initialization failed: {}", status.message);
                impl_->vector_index.reset();
                impl_->db.reset();
            }
        } catch (const std::exception &e) {
            THEMIS_WARN("Failed to create vector index for cache: {}", e.what());
            impl_->vector_index.reset();
            impl_->db.reset();
        }
    }
}

EmbeddingCache::~EmbeddingCache() = default;

std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
    if (static_cast<int>(query_embedding.size()) != config_.embedding_dim) {
        THEMIS_ERROR("Invalid embedding dimension: {} (expected {})",static_cast<int>(query_embedding.size()), config_.embedding_dim);
        return std::nullopt;
    }

    // F-004: Use shared_lock so multiple concurrent query() calls can run the
    // HNSW search (and brute-force fallback) in parallel.  VectorIndexManager
    // is internally thread-safe for concurrent read calls.
    // write operations (store / clearExpired / clear) take unique_lock.
    std::shared_lock<std::shared_mutex> lock(impl_->entry_mutex);

    // Search using vector index if available
    if (impl_->vector_index) {
        auto [status, results] = impl_->vector_index->searchKnn(query_embedding, 1);

        if (status.ok && !results.empty()) {
            const auto &result = results[0];

            // Convert distance to similarity based on metric
            float similarity = 0;
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
                    auto &entry = it->second;

                    // Check if entry is expired
                    if (isExpired(entry)) {
                        // Expired entry found under shared_lock — upgrade to unique_lock
                        // to remove it.  Release shared_lock first (no shared→unique upgrade
                        // in C++ standard library).
                        lock.unlock();
                        std::unique_lock<std::shared_mutex> wlock(impl_->entry_mutex);
                        auto wit = impl_->entries.find(result.pk);
                        if (wit != impl_->entries.end() && isExpired(wit->second)) {
                            if (impl_->vector_index) {
                                impl_->vector_index->removeByPk(wit->first);
                            }
                            impl_->entries.erase(wit);
                            stats_.miss_count++;
                        }
                        THEMIS_DEBUG("Cache miss (expired entry)");
                        return std::nullopt;
                    }

                    // Update entry stats
                    entry.access_count++;
                    entry.last_similarity = similarity;

                    // Update cache stats
                    stats_.hit_count++;
                    stats_.avg_similarity
                        = (stats_.avg_similarity * (stats_.hit_count - 1) + similarity) / stats_.hit_count;
                    stats_.hit_rate = static_cast<double>(stats_.hit_count) / (stats_.hit_count + stats_.miss_count);

                    // Estimate cost savings
                    stats_.cost_savings_usd += EMBEDDING_API_COST_PER_1K_TOKENS * TOKENS_PER_EMBEDDING;

                    THEMIS_DEBUG("Cache hit: pk={}, similarity={:.4f}, access_count={}", result.pk, similarity,
                                 entry.access_count);
                    return entry;
                }
            }
        }
    } else {
        // Fallback: brute-force search through all entries
        float best_similarity = 0.0f;
        std::string best_pk = {};
        for (const auto &[pk, entry] : impl_->entries) {
            if (isExpired(entry)) {
                continue;
            }

            // Compute cosine similarity
            float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
            for (size_t i = 0; i <static_cast<int>(query_embedding.size()); ++i) {
                dot += query_embedding[i] * entry.embedding[i];
                norm1 += query_embedding[i] * query_embedding[i];
                norm2 += entry.embedding[i] * entry.embedding[i];
            }

            float similarity = dot / (std::sqrt(norm1) * std::sqrt(norm2));

            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_pk         = pk;
                // Note: we intentionally do NOT break early here.
                // The unordered_map iteration order is not sorted by similarity,
                // so breaking on the first entry above EARLY_TERMINATION_THRESHOLD
                // would not guarantee the globally best match.  The full scan
                // is required to find the true maximum.
            }
        }

        if (best_similarity >= config_.similarity_threshold) {
            auto &entry = impl_->entries[best_pk];
            entry.access_count++;
            entry.last_similarity = best_similarity;

            stats_.hit_count++;
            stats_.avg_similarity
                = (stats_.avg_similarity * (stats_.hit_count - 1) + best_similarity) / stats_.hit_count;
            stats_.hit_rate = static_cast<double>(stats_.hit_count) / (stats_.hit_count + stats_.miss_count);
            stats_.cost_savings_usd += EMBEDDING_API_COST_PER_1K_TOKENS * TOKENS_PER_EMBEDDING;

            THEMIS_DEBUG("Cache hit (brute-force): pk={}, similarity={:.4f}", best_pk, best_similarity);
            return entry;
        }
    }

    // Cache miss
    stats_.miss_count++;
    stats_.hit_rate = static_cast<double>(stats_.hit_count) / (stats_.hit_count + stats_.miss_count);

    THEMIS_DEBUG("Cache miss");
    return std::nullopt;
}

bool EmbeddingCache::store(const std::string &query_text, const std::vector<float> &embedding,
                           const std::string &metadata) {
    if (static_cast<int>(embedding.size()) != config_.embedding_dim) {
        THEMIS_ERROR("Invalid embedding dimension: {} (expected {})",static_cast<int>(embedding.size()), config_.embedding_dim);
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(impl_->entry_mutex);

    // F-009: O(log N) eviction via min-heap.  Pop the root (oldest by
    // timestamp) and skip stale heap entries (lazy deletion).
    while (impl_-> static_cast<int>(entries.size()) >= config_.max_entries) {
        if (impl_->eviction_heap.empty()) {
            // Heap and map are out of sync — fall back to O(N) scan once.
            auto oldest_it      = impl_->entries.end();
            int64_t oldest_time = std::numeric_limits<int64_t>::max();
            for (auto it = impl_->entries.begin(); it != impl_->entries.end(); ++it) {
                if (it->second.timestamp_ms < oldest_time) {
                    oldest_time = it->second.timestamp_ms;
                    oldest_it   = it;
                }
            }
            if (oldest_it == impl_->entries.end()) {
                break;
            }
            THEMIS_DEBUG("Cache full, evicting oldest entry: pk={}", oldest_it->first);
            if (impl_->vector_index) {
                impl_->vector_index->removeByPk(oldest_it->first);
            }
            impl_->entries.erase(oldest_it);
            break;
        }

        auto [ts, pk] = impl_->eviction_heap.top();
        impl_->eviction_heap.pop();

        auto it = impl_->entries.find(pk);
        if (it == impl_->entries.end()) {
            // Stale heap entry (already erased by clearExpired or a prior eviction).
            continue;
        }
        if (it->second.timestamp_ms != ts) {
            // Entry was re-inserted with a new timestamp; skip this stale record.
            continue;
        }

        THEMIS_DEBUG("Cache full, evicting oldest entry: pk={}", pk);
        if (impl_->vector_index) {
            impl_->vector_index->removeByPk(pk);
        }
        impl_->entries.erase(it);
        break;
    }

    // Generate unique PK for this entry
    std::string pk = "emb_" + std::to_string(impl_->next_id++);

    // Create cache entry
    CacheEntry entry;
    entry.query_text      = query_text;
    entry.embedding       = cache::AlignedVector<float>(embedding.begin(), embedding.end());
    entry.metadata        = metadata;
    entry.timestamp_ms    = getCurrentTimestampMs();
    entry.access_count    = 0;
    entry.last_similarity = 1.0f;

    // Record in eviction heap before inserting into the map.
    impl_->eviction_heap.push({entry.timestamp_ms, pk});

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
    stats_.total_entries = impl_-> static_cast<int>(entries.size());

    THEMIS_DEBUG("Stored embedding in cache: pk={}, query='{}', metadata='{}'", pk, query_text.substr(0, 50), metadata);
    return true;
}

uint64_t EmbeddingCache::clearExpired() {
    std::unique_lock<std::shared_mutex> lock(impl_->entry_mutex);

    uint64_t cleared = 0;
    // Scan and remove expired entries
    for (auto it = impl_->entries.begin(); it != impl_->entries.end();) {
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
    stats_.total_entries = impl_-> static_cast<int>(entries.size());

    THEMIS_INFO("Cleared {} expired cache entries", cleared);
    return cleared;
}

void EmbeddingCache::clear() {
    std::unique_lock<std::shared_mutex> lock(impl_->entry_mutex);

    impl_->entries.clear();
    // Discard all stale heap entries too.
    impl_->eviction_heap = {};

    // Reinitialize vector index
    if (impl_->vector_index) {
        impl_->vector_index->shutdown();
        auto status = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim),
                                                VectorIndexManager::Metric::COSINE, 16, 200, 64);
        if (!status.ok) {
            THEMIS_WARN("Failed to reinitialize vector index after clear: {}", status.message);
        }
    }

    stats_ = CacheStats{};
    THEMIS_INFO("Cache cleared");
}

bool EmbeddingCache::isExpired(const CacheEntry &entry) const {
    auto now_ms = getCurrentTimestampMs();
    auto age_ms = now_ms - entry.timestamp_ms;
    return age_ms > (config_.ttl_seconds * 1000);
}

int64_t EmbeddingCache::getCurrentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

} // namespace themis
