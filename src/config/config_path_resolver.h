/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_path_resolver.h                             ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     167                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <optional>
#include <atomic>
#include "config/config_errors.h"
#include "config/lru_cache.h"
#include "config/path_mapping_metadata.h"

namespace themis {
namespace config {

/**
 * ConfigPathResolver provides backward-compatible config path resolution.
 * 
 * This utility maps legacy config paths to their new hierarchical locations
 * and provides fallback mechanisms to support both old and new paths during
 * the migration period.
 * 
 * Thread Safety:
 *   - All public methods are thread-safe for concurrent reads
 *   - The PATH_MAPPING table is const and initialized at compile-time
 *   - Metrics use atomic operations for thread-safe updates
 *   - No locks are required for read operations
 *   - File system operations may have platform-specific thread-safety guarantees
 * 
 * Usage:
 *   std::string path = ConfigPathResolver::resolve("config/lora_training_config.yaml");
 *   // Returns "config/ai_ml/lora_training_config.yaml" (new) or falls back to old path
 */
class ConfigPathResolver {
public:
    /**
     * Resolves a config path, trying new location first, then falling back to legacy.
     * 
     * @param legacy_path The original/legacy config path
     * @return Resolved path that exists on filesystem
     * @throws ConfigNotFoundException if neither new nor legacy path exists
     */
    static std::string resolve(const std::string& legacy_path);
    
    /**
     * Resolves a config path, returning optional instead of throwing.
     * 
     * @param legacy_path The original/legacy config path
     * @return Resolved path if found, std::nullopt otherwise
     */
    static std::optional<std::string> tryResolve(const std::string& legacy_path);
    
    /**
     * Maps a legacy path to its new hierarchical location.
     * 
     * @param legacy_path The original/legacy config path
     * @return New path location, or empty string if no mapping exists
     */
    static std::string mapLegacyToNew(const std::string& legacy_path);
    
    /**
     * Checks if a path is a legacy path that should be migrated.
     * 
     * @param path The path to check
     * @return true if this is a known legacy path
     */
    static bool isLegacyPath(const std::string& path);
    
    /**
     * Get metadata for a path mapping (if it exists).
     * 
     * @param legacy_path The legacy path to look up
     * @return Metadata if mapping exists, std::nullopt otherwise
     */
    static std::optional<PathMappingMetadata> getMetadata(const std::string& legacy_path);
    
    /**
     * Metrics for config path resolution.
     */
    struct Metrics {
        std::atomic<uint64_t> resolution_hits{0};      // Successful resolutions
        std::atomic<uint64_t> resolution_misses{0};    // Failed resolutions
        std::atomic<uint64_t> legacy_fallbacks{0};     // Times legacy path was used
        std::atomic<uint64_t> new_path_hits{0};        // Times new path was used
        std::atomic<uint64_t> unmapped_requests{0};    // Requests for unmapped paths
        std::atomic<uint64_t> cache_hits{0};           // Cache hits
        std::atomic<uint64_t> cache_misses{0};         // Cache misses
    };
    
    /**
     * Get current metrics.
     */
    static const Metrics& metrics() { return metrics_; }
    
    /**
     * Reset metrics (primarily for testing).
     */
    static void resetMetrics();
    
    /**
     * Enable or disable caching.
     * 
     * @param enabled true to enable caching, false to disable
     */
    static void setCachingEnabled(bool enabled);
    
    /**
     * Get cache statistics.
     */
    static auto cacheStats() { return cache_.stats(); }
    
    /**
     * Clear the cache.
     */
    static void clearCache() { cache_.clear(); }

private:
    // Mapping table from legacy paths to new hierarchical paths
    static const std::map<std::string, std::string> PATH_MAPPING;
    
    // Metadata table with deprecation information
    static const std::map<std::string, PathMappingMetadata> METADATA_TABLE;
    
    // Metrics tracking
    static Metrics metrics_;
    
    // Cache for resolved paths (capacity: 1000, TTL: 5 minutes)
    static LRUCacheWithTTL<std::string, std::string> cache_;
    static std::atomic<bool> caching_enabled_;
    
    // Helper to normalize path separators
    static std::string normalizePath(const std::string& path);
    
    // Path validation
    static void validatePath(const std::string& path);
    
    // Get category from new path
    static std::string inferCategory(const std::string& new_path);
};

} // namespace config
} // namespace themis
