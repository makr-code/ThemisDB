/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            secondary_index_metadata_cache.h                   ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     148                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
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
    struct IndexMetadata {
        std::vector<std::string> regular_indexes;       // Equality indexes
        std::vector<std::string> range_indexes;         // Range indexes
        std::vector<std::string> sparse_indexes;        // Sparse indexes
        std::vector<std::string> geo_indexes;           // Geo indexes
        std::vector<std::string> ttl_indexes;           // TTL indexes
        std::vector<std::string> fulltext_indexes;      // Fulltext indexes
        
        // Unique constraint tracking
        std::unordered_map<std::string, bool> regular_unique;
        std::unordered_map<std::string, bool> sparse_unique;
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
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        
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
