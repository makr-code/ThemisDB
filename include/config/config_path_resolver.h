/**
 * @file config_path_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * Deployment environment for config overlay resolution.
 *
 * When the active environment is not PROD, the resolver probes an
 * environment-specific overlay directory (`config/<env>/`) before
 * falling back to the standard path hierarchy.  This allows dev and
 * staging deployments to override individual config files without
 * modifying the production set.
 *
 * The active environment can also be set via the `THEMIS_CONFIG_ENV`
 * environment variable (`dev`, `staging`, `prod`; case-insensitive).
 */
enum class ConfigEnvironment {
    DEV,     ///< Development environment (overlay root: config/dev/)
    STAGING, ///< Staging environment     (overlay root: config/staging/)
    PROD,    ///< Production environment  (no overlay; default)
};

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
 *   - No locks are required for read operations on the PATH_MAPPING or Metrics
 *   - ConfigAuditLog uses an internal mutex; audit recording adds a lock acquisition per resolved path when enabled
 *   - File system operations may have platform-specific thread-safety guarantees
 *   - SIGHUP handler only sets a volatile sig_atomic_t flag (async-signal-safe);
 *     the actual cache clear is performed inside tryResolve() on the calling thread
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
     * Return the full legacy-to-new path mapping table.
     *
     * Provides read-only access to PATH_MAPPING for tooling (e.g. migration
     * scanner) that needs to enumerate all known legacy paths.
     *
     * @return Const reference to the static mapping table.
     */
    static const std::map<std::string, std::string>& legacyPathMappings();
    
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
     * Get legacy fallback counts broken down by config category.
     *
     * Categories are inferred from the canonical new path via inferCategory()
     * when a legacy fallback occurs. Counts are stored in per-category atomic
     * counters; the returned vector is a snapshot of (category, count) pairs.
     * No external locking is required once initialization has completed.
     */
    static std::vector<std::pair<std::string, uint64_t>> legacyFallbacksByCategory();

    /**
     * Returns the set of category labels used for legacy fallback counters.
     * Categories are initialized once from PATH_MAPPING to keep the label
     * cardinality stable for Prometheus exports.
     */
    static std::vector<std::string> legacyFallbackCategories();
    
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

    /**
     * Set the active deployment environment.
     *
     * When set to DEV or STAGING the resolver probes the environment-specific
     * overlay directory (`config/<env>/`) before the standard new and legacy
     * paths.  Calling this method clears the cache to prevent stale overlay
     * entries from a previous environment from being returned.
     *
     * @param env The environment to activate
     */
    static void setEnvironment(ConfigEnvironment env);

    /**
     * Get the currently active deployment environment.
     *
     * @return Current ConfigEnvironment value
     */
    static ConfigEnvironment getEnvironment();

    /**
     * Register a SIGHUP signal handler that hot-reloads the resolved path cache.
     *
     * When the process receives SIGHUP, the LRU cache is cleared on the next
     * call to resolve() or tryResolve().  This allows operators to trigger a
     * cache flush at runtime (e.g. after moving config files) without restarting
     * the process.
     *
     * On platforms that do not support SIGHUP (Windows), this function is a
     * no-op.
     *
     * Thread-safety: safe to call from any thread.  The underlying signal
     * handler only sets an async-signal-safe flag (volatile sig_atomic_t);
     * the actual cache.clear() is executed on the next call to tryResolve().
     */
    static void registerSighupHandler();

    /**
     * Start the inotify/kqueue/ReadDirectoryChangesW file watcher that
     * automatically clears the resolved path cache whenever a `.yaml` or
     * `.json` file under @p watch_dir changes on disk.
     *
     * This is an optional enhancement on top of the existing SIGHUP-based
     * hot-reload: both mechanisms can be active simultaneously.  File-system
     * events are debounced with a 200 ms settling window to avoid spurious
     * cache flushes during editor save-then-rename sequences.
     *
     * On unsupported platforms this function is a no-op (logs a warning).
     *
     * @param watch_dir  Directory tree to monitor.  Defaults to "config/".
     * @param debounce   Settling window before the cache is cleared.
     *                   Defaults to 200 ms.
     *
     * @return true if the file watcher was started successfully; false if the
     *         platform does not support file watching or @p watch_dir is not
     *         accessible.
     */
    static bool startHotReload(
        const std::string& watch_dir = "config",
        std::chrono::milliseconds debounce = std::chrono::milliseconds(200));

    /**
     * Stop the file watcher started by startHotReload().
     * Idempotent – safe to call even if startHotReload() was never called.
     */
    static void stopHotReload();

    /**
     * Default LRU cache TTL in seconds.
     * Exposed as a named constant so the metrics exporter and other consumers
     * can reference the single source of truth rather than duplicating the value.
     * The actual runtime value may be overridden by THEMIS_CONFIG_CACHE_TTL_SECONDS.
     */
    static const int kCacheTtlSeconds;

    /**
     * LRU cache maximum capacity (entry count).
     * Initialized from the THEMIS_CONFIG_CACHE_SIZE environment variable at
     * program startup; falls back to 1000 when the variable is absent or
     * invalid.
     */
    static const size_t kCacheSize;

    /**
     * Default LRU cache TTL in seconds (compile-time constant).
     * Used as the fallback when THEMIS_CONFIG_CACHE_TTL is not set.
     */
    static constexpr int kDefaultCacheTtlSeconds = 300;

    /**
     * Default LRU cache capacity (compile-time constant).
     * Used as the fallback when THEMIS_CONFIG_CACHE_SIZE is not set.
     */
    static constexpr size_t kDefaultCacheSize = 1000;

    /**
     * Snapshot of the effective cache configuration.
     * Useful for observability endpoints and tests that need to confirm which
     * values are actually in use.
     */
    struct CacheConfig {
        size_t capacity = 0;
        int    ttl_seconds;
    };

    /**
     * Return the active cache configuration (capacity and TTL).
     *
     * Values reflect what was read from the environment at program startup,
     * or the compile-time defaults when the variables were absent or invalid.
     *
     * @return CacheConfig{capacity, ttl_seconds}
     */
    static CacheConfig currentCacheConfig();

    /**
     * Entry in the deprecation usage report.
     */
    struct DeprecationEntry {
        std::string legacy_path;
        std::string new_path;
        std::string category;
        uint64_t usage_count{0};
        std::optional<std::chrono::system_clock::time_point> removal_date;
        std::optional<std::string> migration_guide_url;
    };

    /**
     * Enable or disable deprecation warning aggregation.
     *
     * When enabled, per-call log warnings are suppressed and replaced by
     * periodic batch reports from the background reporter thread. The
     * reporter fires every @p interval_seconds seconds (default: 300).
     *
     * @param enabled         true to enable aggregation, false to disable
     * @param interval_seconds Reporting interval in seconds (only used when enabling)
     */
    static void setAggregationEnabled(bool enabled, int interval_seconds = 300);

    /**
     * Set the legacy fallback rate warning threshold.
     *
     * When the ratio of legacy_fallbacks to total resolutions
     * (legacy_fallbacks + new_path_hits) meets or exceeds @p threshold,
     * a spdlog::warn is emitted.  Warnings use a doubling strategy to
     * prevent log flooding: after the first warning the next fires only
     * when the fallback count has at least doubled.
     *
     * Set to 0.0 (default) to disable threshold alerting.
     *
     * @param threshold Ratio in [0.0, 1.0].  Values outside this range are
     *                  clamped to [0.0, 1.0].
     */
    static void setLegacyFallbackRateThreshold(double threshold);

    /**
     * Get the current legacy fallback rate warning threshold.
     *
     * @return Threshold in [0.0, 1.0]; 0.0 means threshold alerting is disabled.
     */
    static double getLegacyFallbackRateThreshold();

    /**
     * Get the current deprecation usage report.
     *
     * Returns all legacy paths that have been accessed since the last
     * resetMetrics() call, sorted by descending usage count.
     *
     * @return Vector of deprecation entries with usage counts
     */
    static std::vector<DeprecationEntry> deprecationReport();

    // ── Audit log API ────────────────────────────────────────────────────

    /**
     * Enable or disable config path audit logging.
     *
     * When enabled, every successful path resolution is appended to the
     * audit log with the requested path, the resolved path, a timestamp,
     * and flags indicating whether the result was a legacy fallback or a
     * cache hit.  Audit logging is disabled by default.
     *
     * @param enabled  true to enable, false to disable.
     */
    static void setAuditLogEnabled(bool enabled);

    /**
     * Return a snapshot of all audit entries recorded since the last
     * clearAuditLog() call (oldest entry first).
     *
     * @return Vector of AuditEntry objects.
     */
    static std::vector<AuditEntry> auditLog();

    /**
     * Clear all entries from the audit log.
     */
    static void clearAuditLog();

    /**
     * Set the maximum number of audit entries retained in memory.
     * Entries beyond this limit are evicted oldest-first.
     *
     * @param max  Maximum number of entries (clamped to >= 1).
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
    static void initLegacyFallbackCategoryCounters();
    
    // Helper to normalize path separators
    static std::string normalizePath(const std::string& path);
    
    // Path validation
    static void validatePath(const std::string& path);
    
    // Get category from new path
    static std::string inferCategory(const std::string& new_path);

    // Deprecation aggregator (tracks per-path legacy usage counts)
    class DeprecationAggregator;
    static DeprecationAggregator aggregator_;
    static std::atomic<bool> aggregation_enabled_;

    // Active deployment environment (used for overlay path probing)
    static std::atomic<ConfigEnvironment> current_env_;

    // SIGHUP hot-reload flag and handler (POSIX only; no-op on Windows)
    static volatile sig_atomic_t sighup_pending_;
    static void handleSighup(int sig);

    // Optional inotify/kqueue/ReadDirectoryChangesW file watcher (v1.8.0).
    // Stored as a unique_ptr so the platform headers are not exposed in this
    // public header (include config_file_watcher.h in the .cpp only).
    static std::unique_ptr<ConfigFileWatcher> file_watcher_;

    // Converts a ConfigEnvironment to its lowercase string name
    static std::string envToString(ConfigEnvironment env);

    // Reads and validates THEMIS_CONFIG_ENV at initialisation time
    static ConfigEnvironment envFromEnvironmentVariable();
    // Audit log (records all successful path resolutions with timestamps)
    static ConfigAuditLog audit_log_;
    // Legacy fallback rate threshold alerting
    static std::atomic<double> legacy_fallback_threshold_;
    // Fallback count at which the last threshold warning was emitted.
    // 0 means no warning has been emitted yet in the current metrics window.
    static std::atomic<uint64_t> last_threshold_warn_count_;

    // Check whether the current fallback rate has crossed the threshold and,
    // if so, emit a rate-limited spdlog::warn.
    static void checkFallbackRateThreshold();
};

} // namespace config
} // namespace themis
