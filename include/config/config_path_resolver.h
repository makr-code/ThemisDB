/**
 * @file config_path_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <optional>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include "config/config_audit_log.h"
#include <csignal>
#include "config/config_errors.h"
#include "config/lru_cache.h"
#include "config/path_mapping_metadata.h"

namespace themis {
namespace config {

// Forward declaration so ConfigPathResolver can hold a unique_ptr without
// pulling in the platform-specific config_file_watcher.h headers.
class ConfigFileWatcher;

enum class ConfigEnvironment {
    DEV,     ///< Development environment (overlay root: config/dev/)
    STAGING, ///< Staging environment     (overlay root: config/staging/)
    PROD,    ///< Production environment  (no overlay; default)
};

class ConfigPathResolver {
public:
    /**
     * @brief Resolve.
     * @param[in] legacy_path Path to the legacy.
     * @return Return value.
     */
    static std::string resolve(const std::string& legacy_path);
    
    /**
     * @brief Try Resolve.
     * @param[in] legacy_path Path to the legacy.
     * @return Return value.
     */
    static std::optional<std::string> tryResolve(const std::string& legacy_path);
    
    /**
     * @brief Map Legacy To New.
     * @param[in] legacy_path Path to the legacy.
     * @return Return value.
     */
    static std::string mapLegacyToNew(const std::string& legacy_path);
    
    /**
     * @brief Is Legacy Path.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isLegacyPath(const std::string& path);
    
    /**
     * @brief Get Metadata.
     * @param[in] legacy_path Path to the legacy.
     * @return Return value.
     */
    static std::optional<PathMappingMetadata> getMetadata(const std::string& legacy_path);

    static const std::map<std::string, std::string>& legacyPathMappings();
    
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
     * @brief Metrics.
     * @return Return value.
     * @details Implements metrics without additional internal calls.
     */
    static const Metrics& metrics() { return metrics_; }

    static std::vector<std::pair<std::string, uint64_t>> legacyFallbacksByCategory();

    /**
     * @brief Legacy Fallback Categories.
     * @return Return value.
     */
    static std::vector<std::string> legacyFallbackCategories();
    
    /**
     * @brief Reset Metrics.
     */
    static void resetMetrics();
    
    /**
     * @brief Set Caching Enabled.
     * @param[in] enabled Input parameter.
     */
    static void setCachingEnabled(bool enabled);
    
    /**
     * @brief Cache Stats.
     * @return Return value.
     * @details Calls: stats().
     */
    static auto cacheStats() { return cache_.stats(); }
    
    /**
     * @brief Clear Cache.
     * @details Calls: clear().
     */
    static void clearCache() { cache_.clear(); }

    /**
     * @brief Set Environment.
     * @param[in] env Input parameter.
     */
    static void setEnvironment(ConfigEnvironment env);

    /**
     * @brief Get Environment.
     * @return Return value.
     */
    static ConfigEnvironment getEnvironment();

    /**
     * @brief Register Sighup Handler.
     */
    static void registerSighupHandler();

    static bool startHotReload(
        const std::string& watch_dir = "config",
        std::chrono::milliseconds debounce = std::chrono::milliseconds(200));

    /**
     * @brief Stop Hot Reload.
     */
    static void stopHotReload();

    static const int kCacheTtlSeconds;

    static const size_t kCacheSize;

    static constexpr int kDefaultCacheTtlSeconds = 300;

    static constexpr size_t kDefaultCacheSize = 1000;

    struct CacheConfig {
        size_t capacity = 0;
        int    ttl_seconds = {};
    };

    /**
     * @brief Current Cache Config.
     * @return Return value.
     */
    static CacheConfig currentCacheConfig();

    struct DeprecationEntry {
        std::string legacy_path;
        std::string new_path;
        std::string category;
        uint64_t usage_count{0};
        std::optional<std::chrono::system_clock::time_point> removal_date;
        std::optional<std::string> migration_guide_url;
    };

    static void setAggregationEnabled(bool enabled, int interval_seconds = 300);

    /**
     * @brief Set Legacy Fallback Rate Threshold.
     * @param[in] threshold Input parameter.
     */
    static void setLegacyFallbackRateThreshold(double threshold);

    /**
     * @brief Get Legacy Fallback Rate Threshold.
     * @return Return value.
     */
    static double getLegacyFallbackRateThreshold();

    /**
     * @brief Deprecation Report.
     * @return Return value.
     */
    static std::vector<DeprecationEntry> deprecationReport();

    /**
     * @brief ── Audit log API ────────────────────────────────────────────────────
     * @param[in] enabled Input parameter.
     */

    static void setAuditLogEnabled(bool enabled);

    /**
     * @brief Audit Log.
     * @return Return value.
     */
    static std::vector<AuditEntry> auditLog();

    /**
     * @brief Clear Audit Log.
     */
    static void clearAuditLog();

    /**
     * @brief Set Audit Log Max Entries.
     * @param[in] max Input parameter.
     */
    static void setAuditLogMaxEntries(std::size_t max);

private:
    // Mapping table from legacy paths to new hierarchical paths
    static const std::map<std::string, std::string> PATH_MAPPING;
    
    // Metadata table with deprecation information
    static const std::map<std::string, PathMappingMetadata> METADATA_TABLE;
    
    // Metrics tracking
    static Metrics metrics_;
    
    // Cache for resolved paths (capacity and TTL configurable via env vars)
    // Cache for resolved paths (capacity: 1000, TTL: 5 minutes by default)
    static LRUCacheWithTTL<std::string, std::string> cache_;
    static std::atomic<bool> caching_enabled_;

    // Active cache configuration (set once at startup from env vars or defaults)
    static CacheConfig cache_config_;

    // Per-category legacy fallback counters (initialized once, then atomically incremented)
    static std::map<std::string, std::atomic<uint64_t>> legacy_fallbacks_by_category_;
    static std::once_flag category_init_flag_;
    /**
     * @brief Init Legacy Fallback Category Counters.
     */
    static void initLegacyFallbackCategoryCounters();
    
    /**
     * @brief Helper to normalize path separators
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string normalizePath(const std::string& path);
    
    // Path validation
    /**
     * @brief Validate Path.
     * @param[in] path Input parameter.
     */
    static void validatePath(const std::string& path);
    
    /**
     * @brief Get category from new path
     * @param[in] new_path Path to the new.
     * @return Return value.
     */
    static std::string inferCategory(const std::string& new_path);

    // Deprecation aggregator (tracks per-path legacy usage counts)
    class DeprecationAggregator;
    static DeprecationAggregator aggregator_;
    static std::atomic<bool> aggregation_enabled_;

    // Active deployment environment (used for overlay path probing)
    static std::atomic<ConfigEnvironment> current_env_;

    // SIGHUP hot-reload flag and handler (POSIX only; no-op on Windows)
    static volatile sig_atomic_t sighup_pending_;
    /**
     * @brief Handle Sighup.
     * @param[in] sig Input parameter.
     */
    static void handleSighup(int sig);

    // Optional inotify/kqueue/ReadDirectoryChangesW file watcher (v1.8.0).
    // Stored as a unique_ptr so the platform headers are not exposed in this
    // public header (include config_file_watcher.h in the .cpp only).
    static std::unique_ptr<ConfigFileWatcher> file_watcher_;

    /**
     * @brief Converts a ConfigEnvironment to its lowercase string name
     * @param[in] env Input parameter.
     * @return Return value.
     */
    static std::string envToString(ConfigEnvironment env);

    /**
     * @brief Reads and validates THEMIS_CONFIG_ENV at initialisation time
     * @return Return value.
     */
    static ConfigEnvironment envFromEnvironmentVariable();
    // Audit log (records all successful path resolutions with timestamps)
    static ConfigAuditLog audit_log_;
    // Legacy fallback rate threshold alerting
    static std::atomic<double> legacy_fallback_threshold_;
    // Fallback count at which the last threshold warning was emitted.
    // 0 means no warning has been emitted yet in the current metrics window.
    static std::atomic<uint64_t> last_threshold_warn_count_;

    /**
     * @brief Check whether the current fallback rate has crossed the threshold and, if so, emit a rate-limited spdlog::warn.
     */
    static void checkFallbackRateThreshold();
};

} // namespace config
} // namespace themis
