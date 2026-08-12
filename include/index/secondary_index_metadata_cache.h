/**
 * @file secondary_index_metadata_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <shared_mutex>
#include <optional>
#include <mutex>

namespace themis {

/// SecondaryIndexMetadataCache - In-Memory Cache für Index-Konfigurationen
/// 
/// Optimierung für v1.3.4:
/// Cached Index-Metadaten um DB-Scans bei jedem Insert zu vermeiden
/// Expected Improvement: +60-200x für Metadata-Lookups (600-2000 µs → <10 µs)
///
class SecondaryIndexMetadataCache {
public:
    /// Mirrors SecondaryIndexManager::FulltextConfig — stored here to avoid a
    /// circular include dependency between this header and secondary_index.h.
    struct CachedFulltextConfig {
        bool stemming_enabled   = false;
        std::string language    = "none";
        bool stopwords_enabled  = false;
        std::vector<std::string> stopwords;
        bool normalize_umlauts  = false;
    };

    struct IndexMetadata {
        std::vector<std::string> regular_indexes;       // Equality indexes
        std::vector<std::string> range_indexes;         // Range indexes
        std::vector<std::string> sparse_indexes;        // Sparse indexes
        std::vector<std::string> geo_indexes;           // Geo indexes
        std::vector<std::string> ttl_indexes;           // TTL indexes
        std::vector<std::string> fulltext_indexes;      // Fulltext indexes
        std::vector<std::string> partial_indexes;       // Partial/filtered indexes (column names)
        
        // Unique constraint tracking
        std::unordered_map<std::string, bool> regular_unique;
        std::unordered_map<std::string, bool> sparse_unique;
        std::unordered_map<std::string, std::string> partial_predicates; // column -> predicate
        std::unordered_map<std::string, bool> partial_unique; // column -> unique flag

        // Precomputed sets for O(1) membership lookup in write-path hot loops.
        // Populated alongside the vectors so callers avoid rebuilding sets on
        // every cache hit.
        std::unordered_set<std::string> regular_indexes_set;
        std::unordered_set<std::string> range_indexes_set;

        // Per-column config/metadata cached to eliminate extra db.get() calls on
        // every insert/upsert in the hot write path (v1.3.5 optimization).
        std::unordered_map<std::string, CachedFulltextConfig> fulltext_configs; // column -> config
        std::unordered_map<std::string, int64_t>              ttl_seconds;      // column -> TTL value
        std::unordered_map<std::string, bool>                 composite_unique; // "c1+c2" -> unique flag
    };

    /// Singleton instance
    static SecondaryIndexMetadataCache& instance() {
        static SecondaryIndexMetadataCache cache;
        return cache;
    }

    /// Cache Statistiken
    struct CacheStats {
        size_t total_lookups = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        double hit_rate() const {
            return total_lookups > 0 ? (100.0 * cache_hits / total_lookups) : 0.0;
        }
    };

    /// Get cached metadata für Tabelle
    /// Returns: IndexMetadata if cached, std::nullopt if cache miss
    std::optional<IndexMetadata> get(std::string_view table) {
        // get() updates cache statistics; therefore it must not run under a
        // shared/read lock while mutating stats_.
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        
        stats_.total_lookups++;
        
        auto it = metadata_cache_.find(std::string(table));
        if (it != metadata_cache_.end()) {
            // Check if cache entry is still valid (TTL-based)
            auto now = std::chrono::steady_clock::now();
            if (now - it->second.timestamp < cache_ttl_) {
                stats_.cache_hits++;
                return it->second.metadata;
            }
        }
        
        stats_.cache_misses++;
        return std::nullopt;
    }

    /// Set cached metadata für Tabelle
    void set(std::string_view table, const IndexMetadata& metadata) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        
        CacheEntry entry;
        entry.metadata = metadata;
        entry.timestamp = std::chrono::steady_clock::now();
        
        metadata_cache_[std::string(table)] = entry;
    }

    /// Invalidate cache für Tabelle (z.B. nach createIndex/dropIndex)
    void invalidate(std::string_view table) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        metadata_cache_.erase(std::string(table));
    }

    /// Clear entire cache
    void clear() {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        metadata_cache_.clear();
        stats_.total_lookups = 0;
        stats_.cache_hits = 0;
        stats_.cache_misses = 0;
    }

    /// Get cache statistics
    CacheStats get_stats() const {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        return stats_;
    }

    /// Set cache TTL (default: 60 seconds)
    void set_ttl(std::chrono::seconds ttl) {
        cache_ttl_ = ttl;
    }

private:
    SecondaryIndexMetadataCache() = default;
    
    struct CacheEntry {
        IndexMetadata metadata;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::unordered_map<std::string, CacheEntry> metadata_cache_;
    mutable std::shared_mutex cache_mutex_;
    std::chrono::seconds cache_ttl_ = std::chrono::seconds(60);
    mutable CacheStats stats_;
};

} // namespace themis
