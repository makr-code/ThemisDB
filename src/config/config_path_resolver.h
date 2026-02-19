#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <optional>
#include <atomic>
#include "config/config_errors.h"

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
     * Metrics for config path resolution.
     */
    struct Metrics {
        std::atomic<uint64_t> resolution_hits{0};      // Successful resolutions
        std::atomic<uint64_t> resolution_misses{0};    // Failed resolutions
        std::atomic<uint64_t> legacy_fallbacks{0};     // Times legacy path was used
        std::atomic<uint64_t> new_path_hits{0};        // Times new path was used
        std::atomic<uint64_t> unmapped_requests{0};    // Requests for unmapped paths
    };
    
    /**
     * Get current metrics.
     */
    static const Metrics& metrics() { return metrics_; }
    
    /**
     * Reset metrics (primarily for testing).
     */
    static void resetMetrics();

private:
    // Mapping table from legacy paths to new hierarchical paths
    static const std::map<std::string, std::string> PATH_MAPPING;
    
    // Metrics tracking
    static Metrics metrics_;
    
    // Helper to normalize path separators
    static std::string normalizePath(const std::string& path);
    
    // Path validation
    static void validatePath(const std::string& path);
};

} // namespace config
} // namespace themis
