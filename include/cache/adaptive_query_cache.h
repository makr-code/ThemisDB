/**
 * @file adaptive_query_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: adaptive_query_cache.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 743
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4949 [Docs][cache] Update module... (2026-05-11) | #4293 Implement predictive prefet... (2026-03-19) | #4306 docs: Release Aggregation f... (2026-03-17) | #4285 feat(server): Versioned API... (2026-03-17) | #3550 docs(cache): sync primary d... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "cache/cache_metrics.h"
#include "cache/cache_interfaces.h"
#include "cache/cache_replication.h"
#include "cache/eviction_policy.h"
#include "cache/predictive_prefetcher.h"
#include "cache/cache_replication_coordinator.h"
#include "core/concerns/eviction_strategies.h"
#include "access_model/access_coordinator.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;

/**
 * @brief Adaptive Multi-Level Query Cache
 * 
 * Three-tier cache architecture optimized for 10B+ record datasets:
 * - Level 1 (HOT):  In-Memory HashMap, <1KB entries, TTL 5min
 * - Level 2 (WARM): Compressed (Zstd), <10KB entries, TTL 30min
 * - Level 3 (COLD): RocksDB, unbounded, TTL 24h
 * 
 * Features:
 * - Query fingerprinting (SHA256 hash of query + parameters)
 * - Adaptive TTL based on query frequency
 * - LRU eviction with frequency weighting
 * - Automatic level promotion/demotion
 * - Cache-aware query optimization hints
 * 
 * Performance Goals:
 * - 40-60% cache hit rate for typical OLAP workloads
 * - +60% throughput improvement for cached queries
 * - <1ms latency for L1 hits (HOT tier)
 * - <10ms latency for L2 hits (WARM tier)
 * 
 * Thread-Safety:
 * - All operations are thread-safe
 * - Internal mutexes protect cache structures
 * - Lock-free fast path for L1 hits
 */
class AdaptiveQueryCache : public cache::ICacheBackend<std::string, nlohmann::json> {
public:
    enum class CacheLevel {
        HOT,   // L1: In-memory, fast, small
        WARM,  // L2: Compressed in-memory
        COLD   // L3: RocksDB persistent
    };
    
    struct Config {
        // L1 (HOT) configuration
        size_t l1_max_entries = 10000;         // Max entries in L1
        size_t l1_max_entry_size = 1024;       // 1KB max per entry
        int l1_ttl_seconds = 300;              // 5 minutes
        
        // L2 (WARM) configuration
        size_t l2_max_entries = 50000;         // Max entries in L2
        size_t l2_max_entry_size = 10240;      // 10KB max per entry
        int l2_ttl_seconds = 1800;             // 30 minutes
        int l2_compression_level = 3;          // Zstd compression level
        
        // L3 (COLD) configuration
        int l3_ttl_seconds = 86400;            // 24 hours
        std::string l3_db_path = "./themis_query_cache";
        
        // Eviction policy
        bool enable_frequency_weighting = true;
        float frequency_weight = 0.3f;         // Weight for frequency in LRU score

        // Configurable eviction policies (L1 and L2 can use LRU, LFU, or ARC)
        cache::EvictionPolicy l1_eviction_policy = cache::EvictionPolicy::LRU;
        cache::EvictionPolicy l2_eviction_policy = cache::EvictionPolicy::LRU;
        
        // Size limits (Phase 1: Security)
        size_t max_total_entry_size = 10485760; // 10MB absolute max per entry
        bool enable_size_limits = true;         // Enable size validation
        
        // Circuit breaker configuration (Phase 1: Fault Isolation)
        bool enable_circuit_breaker = true;
        uint32_t cb_failure_threshold = 5;      // Failures before opening
        uint32_t cb_timeout_ms = 60000;         // 1 minute timeout
        
        // Phase 2: Rate limiting & backpressure
        bool enable_rate_limiting = false;       // Enable rate limiting (opt-in)
        uint32_t max_requests_per_second = 10000; // Global rate limit
        bool enable_backpressure = true;         // Enable backpressure
        size_t l3_write_queue_size = 1000;       // Max queued L3 writes
        
        // Phase 2: Tenant isolation
        bool enable_tenant_isolation = false;    // Enable tenant namespacing (opt-in)
        size_t per_tenant_max_bytes = 104857600; // 100MB per tenant default
        
        // Phase 3: Adaptive TTL tuning
        bool enable_adaptive_ttl = false;        // Enable adaptive TTL based on access patterns
        int min_ttl_seconds = 60;                // Legacy alias for adaptive_ttl_min_seconds
        int max_ttl_seconds = 86400;             // Legacy alias for adaptive_ttl_max_seconds
        int adaptive_ttl_min_seconds = 60;       // Minimum TTL (1 minute)
        int adaptive_ttl_max_seconds = 86400;    // Maximum TTL (24 hours)
        double adaptive_ttl_scaling_factor = 5.0; // Scaling factor for logarithmic growth

        // Phase 4: Predictive pre-fetching based on query sequence history
        bool enable_predictive_prefetch = false; // Enable Markov-chain prefetch predictor
        size_t prefetch_max_tracked_keys = 5000; // Max distinct source keys in transition table
        size_t prefetch_max_predictions = 3;     // Max candidate fingerprints per prediction
        uint32_t prefetch_min_transition_count = 2; // Min observed transitions for a candidate
        double prefetch_min_confidence = 0.0;    // Min transition confidence (0.0 = disabled)
        bool prefetch_enable_time_of_day_weighting = false; // Weight predictions by hour-of-day
        bool prefetch_enable_ab_test = false;    // Route 50% tenants to Markov, 50% to baseline
        // Phase 4: Cache replication for high-availability multi-node deployments
        bool enable_replication = false;         // Enable cache replication via coordinator
        
        // Phase 4: Write-through cache mode for read-heavy workloads
        // When enabled, put() writes to ALL applicable tiers simultaneously (L1+L2+L3)
        // instead of selecting a single tier based on entry size.
        // This increases write cost but guarantees that every entry is immediately
        // available at the closest tier, eliminating inter-tier promotion latency for
        // subsequent reads. Recommended for read-heavy workloads where writes are
        // infrequent relative to reads.
        bool enable_write_through = false;

        // Warmup: Parallel Bulk Load
        // Maximum number of parallel worker threads used by warmupFromLog().
        // Each worker processes an independent chunk of the NDJSON log and inserts
        // entries concurrently, exploiting all available CPU cores.
        // 0 is treated as 1 (single-threaded). Defaults to hardware concurrency.
        uint32_t max_parallel_workers = static_cast<uint32_t>(
            std::thread::hardware_concurrency() > 0
                ? std::thread::hardware_concurrency()
                : 1u);

        /**
         * @brief Validate configuration parameters
         * @return true if config is valid, false otherwise
         */
        bool validate(std::string* error_msg = nullptr) const;
    };
    
    struct CacheEntry {
        std::string query_fingerprint;
        nlohmann::json query_params;           // Original query parameters
        nlohmann::json result;                 // Cached query result
        CacheLevel level;
        int64_t created_at_ms;
        int64_t last_accessed_ms;
        int64_t access_count = 0;
        int ttl_seconds;
        size_t result_size_bytes = 0;
    };
    
    struct CacheStats {
        uint64_t l1_hits = 0;
        uint64_t l2_hits = 0;
        uint64_t l3_hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        uint64_t promotions = 0;
        uint64_t demotions = 0;
        
        double getHitRate() const {
            uint64_t total = l1_hits + l2_hits + l3_hits + misses;
            return total > 0 ? static_cast<double>(l1_hits + l2_hits + l3_hits) / total : 0.0;
        }
        
        double getL1HitRate() const {
            uint64_t total = l1_hits + l2_hits + l3_hits + misses;
            return total > 0 ? static_cast<double>(l1_hits) / total : 0.0;
        }
    };
    
    explicit AdaptiveQueryCache(const Config& config);
    ~AdaptiveQueryCache();
    
    // Non-copyable, moveable
    AdaptiveQueryCache(const AdaptiveQueryCache&) = delete;
    AdaptiveQueryCache& operator=(const AdaptiveQueryCache&) = delete;
    AdaptiveQueryCache(AdaptiveQueryCache&&) = default;
    AdaptiveQueryCache& operator=(AdaptiveQueryCache&&) = default;
    
    /**
     * @brief Generate query fingerprint from query string and parameters
     * 
     * Uses SHA256 hash for consistent fingerprinting across runs.
     * 
     * @param query Query string (AQL, SQL, etc.)
     * @param params Query parameters (bind variables, limits, etc.)
     * @param tenant_id Optional tenant ID for namespace isolation (Phase 2)
     * @return SHA256 fingerprint as hex string
     */
    std::string generateFingerprint(const std::string& query, 
                                    const nlohmann::json& params = {},
                                    const std::string& tenant_id = "") const;
    
    /**
     * @brief Get cached query result
     * 
     * Searches all three cache levels (L1 -> L2 -> L3).
     * Automatically promotes frequently accessed entries to higher levels.
     * 
     * @param fingerprint Query fingerprint (from generateFingerprint)
     * @param tenant_id Optional tenant ID for namespace isolation (Phase 2)
     * @return Cached result if found and not expired, nullopt otherwise
     */
    std::optional<CacheEntry> get(const std::string& fingerprint,
                                   const std::string& tenant_id);
    
    /**
     * @brief Store query result in cache
     * 
     * Automatically selects cache level based on result size and config.
     * L1 for <1KB, L2 for <10KB (compressed), L3 for larger results.
     * 
     * @param fingerprint Query fingerprint
     * @param query_params Original query parameters (for debugging)
     * @param result Query result to cache
     * @param tenant_id Optional tenant ID for namespace isolation (Phase 2)
     * @param pii_uuids Optional list of PII UUIDs whose data appears in the
     *                  cached result.  When non-empty, the entry is registered
     *                  in the GDPR PII index so that invalidatePII() can
     *                  remove it upon a right-to-erasure request.
     * @return True if successfully cached
     */
    bool put(const std::string& fingerprint,
             const nlohmann::json& query_params,
             const nlohmann::json& result,
             const std::string& tenant_id = "",
             const std::vector<std::string>& pii_uuids = {});
    
    /**
     * @brief Invalidate cache entries matching a pattern
     * 
     * Useful for invalidating queries on a specific collection/table.
     * 
     * @param pattern Regex pattern to match query parameters
     * @return Number of entries invalidated
     */
    size_t invalidate(const std::string& pattern);
    
    /**
     * @brief Clear all cache entries
     */
    void clear() override;
    
    // ========================================================================
    // ICacheBackend<std::string, nlohmann::json> — simple adapter interface
    //
    // Maps the rich multi-level, multi-tenant API to a plain K→V facade so
    // that cache-agnostic consumers (query optimisers, RAG pipelines, etc.)
    // can use AdaptiveQueryCache through the uniform ICacheBackend contract.
    //
    // The adapter always operates against the default (empty) tenant and
    // forwards to the rich API.  Prefer the rich API for production paths.
    // ========================================================================

    /// Adapter get: returns `entry.result` for the default tenant.
    std::optional<nlohmann::json> get(const std::string& fingerprint) override;

    /// Adapter put: stores @p result in the default tenant with empty query params.
    /// @note @p ttl_seconds is **not honoured** — AdaptiveQueryCache applies its own
    ///       tier-based TTL policy (Config::l1/l2/l3_ttl_seconds).  Use the rich
    ///       `put(fp, params, result, tenant_id)` overload when per-call TTL control
    ///       is required.
    void put(const std::string& fingerprint, nlohmann::json result,
             uint32_t ttl_seconds = 0) override;

    /// Adapter remove: removes @p fingerprint from all cache tiers.
    /// @return true if the key was found in at least one tier.
    bool remove(const std::string& fingerprint) override;

    /// Adapter contains: checks presence in all tiers without updating LRU.
    bool contains(const std::string& fingerprint) const override;

    /// Adapter size: returns the number of entries in L1 + L2.
    /// @note L3 (RocksDB) entries are **not counted** to avoid an O(n) full-scan.
    ///       The returned value is therefore a lower bound on the total cache size.
    std::size_t size() const override;
    
    /**
     * @brief Clear expired entries from all levels
     * 
     * @return Number of entries cleared
     */
    uint64_t clearExpired();
    
    /**
     * @brief Get cache statistics
     */
    CacheStats getStats() const;
    
    /**
     * @brief Get enhanced metrics (Phase 1: Observability)
     */
    const cache::CacheMetrics& getEnhancedMetrics() const {
        return enhanced_metrics_;
    }
    
    /**
     * @brief Get detailed cache information (for monitoring)
     */
    nlohmann::json getDetailedInfo() const;
    
    // ========================================================================
    // Phase 3: Admin API & Operational Tooling
    // ========================================================================
    
    /**
     * @brief Get statistics by cache tier
     * @return JSON with per-tier statistics
     */
    nlohmann::json getStatsByTier() const;
    
    /**
     * @brief Get cache health status
     * @return JSON with health information and warnings
     */
    nlohmann::json getHealthStatus() const;
    
    /**
     * @brief Export cache keys for debugging
     * @param max_keys Maximum number of keys to export (default: 100)
     * @return Vector of cache keys
     */
    std::vector<std::string> exportKeys(size_t max_keys = 100) const;
    
    /**
     * @brief Get tenant usage statistics (all tenants)
     * @return JSON with per-tenant size, hit/miss, and eviction statistics
     */
    nlohmann::json getTenantStats() const;

    /**
     * @brief Get cache statistics for a single tenant
     * @param tenant_id Tenant identifier
     * @return JSON with hit/miss/eviction/bytes statistics for the tenant,
     *         or {"found": false} if the tenant has no recorded activity
     */
    nlohmann::json getTenantStatsForTenant(const std::string& tenant_id) const;
    
    /**
     * @brief Bulk put for cache warmup
     * @param entries Vector of {fingerprint, params, result, tenant_id} tuples
     * @return Number of successfully cached entries
     */
    size_t bulkPut(const std::vector<std::tuple<std::string, nlohmann::json, nlohmann::json, std::string>>& entries);
    
    /**
     * @brief Invalidate all entries for a specific tenant
     * @param tenant_id Tenant ID to invalidate
     * @return Number of entries invalidated
     */
    size_t invalidateTenant(const std::string& tenant_id);

    /**
     * @brief Invalidate all cache entries associated with a PII UUID.
     *
     * Implements GDPR Art. 17 ("Right to Erasure") cache propagation: when a
     * data subject's PII is erased from the underlying store, any query result
     * that was tagged with the corresponding PII UUID during put() must be
     * removed from all three cache tiers immediately.
     *
     * - L1 / L2: removed via the in-memory PII reverse index.
     * - L3 (RocksDB): removed by scanning the `pii_ref:{pii_uuid}:` prefix
     *   that was written alongside the original cache entry.
     * - A structured log entry (THEMIS_INFO) is emitted for every call,
     *   regardless of how many entries were actually purged, to provide an
     *   operational trace.  For a formal GDPR audit trail, the caller
     *   (e.g. PIIPseudonymizer::erasePII) is responsible for logging to the
     *   dedicated AuditLogger before invoking this method.
     *
     * @param pii_uuid  UUID that identifies the erased data-subject record.
     * @return Number of cache entries purged across all tiers.
     */
    size_t invalidatePII(const std::string& pii_uuid);

    /**
     * @brief Update the cache quota for a specific tenant.
     *
     * Overrides the global `config_.per_tenant_max_bytes` for the given
     * tenant.  The new quota is enforced immediately on the next `put()`
     * that is attributed to that tenant.
     *
     * - A quota of 0 restores the global default (`config_.per_tenant_max_bytes`).
     * - Reducing the quota below the current `bytes_used` does NOT evict
     *   existing entries; it only prevents new ones from being accepted.
     *
     * @param tenant_id   Tenant identifier (must not be empty).
     * @param quota_bytes New quota in bytes (0 = revert to global default).
     * @return true on success; false when tenant_id is empty or tenant
     *         isolation is disabled.
     */
    bool updateTenantQuota(const std::string& tenant_id, size_t quota_bytes);

    /**
     * @brief Get L3 circuit breaker status as JSON.
     *
     * Returns an object with keys:
     *   - state: "CLOSED" | "OPEN" | "HALF_OPEN"
     *   - failure_count: uint32
     *   - enabled: bool (false when circuit breaker is not configured)
     */
    nlohmann::json getCircuitBreakerStatus() const;

    /**
     * @brief Force the L3 circuit breaker to CLOSED state.
     *
     * Resets failure counters. No-op when the circuit breaker is not configured.
     */
    void resetCircuitBreaker();

    // ========================================================================
    // Phase 4: Cache Replication for High-Availability Multi-Node Deployments
    // ========================================================================

    /**
     * @brief Register a replication coordinator for multi-node cache synchronisation.
     *
     * Once a coordinator is registered the cache will:
     *  - Call `coordinator->publishEntry()` after every successful `put()` when
     *    `config_.enable_replication` is true.
     *  - Call `coordinator->publishInvalidation()` inside `invalidate()` and
     *    `invalidateTenant()` so peer nodes evict the same entries.
     *  - Subscribe to incoming entry/invalidation messages from remote peers
     *    and apply them to the local L1/L2 cache.
     *
     * Graceful degradation: any exception thrown by the coordinator is caught
     * and demoted to a warning log; the local cache operation always completes.
     *
     * @param coordinator  Shared coordinator instance (nullptr removes current).
     */
    void setCoordinator(std::shared_ptr<cache::ICacheCoordinator> coordinator);

    /**
     * @brief Return replication coordinator statistics, or an empty JSON object
     *        when no coordinator is registered.
     */
    nlohmann::json getReplicationStats() const;

    // ========================================================================
    // Phase 3: Cache Warmup and Snapshot
    // ========================================================================

    /**
     * @brief Result returned by warmupFromLog() and exportSnapshot().
     */
    struct WarmupResult {
        size_t entries_loaded = 0;   ///< entries successfully inserted (warmupFromLog)
        size_t entries_written = 0;  ///< entries successfully written   (exportSnapshot)
        size_t entries_skipped = 0;  ///< malformed or quota-rejected entries
        size_t entries_total = 0;    ///< total lines read / entries considered
        bool   ok = true;            ///< false if the log file could not be opened
        std::string error;           ///< error message when ok == false
        int64_t warmup_duration_ms = 0;          ///< wall-clock time for warmupFromLog (ms)
        double  warmup_entries_per_second = 0.0; ///< throughput: entries loaded / second
    };

    /**
     * @brief Warm up L1/L2 cache from a newline-delimited JSON log file.
     *
     * Each line of the log must be a JSON object with the following fields:
     * @code
     * {"key":"<sha256_hex>","value_b64":"<base64_result>","ttl_remaining_s":300,"tenant":"acme"}
     * @endcode
     *
     * - "key"            – 64-char SHA-256 hex fingerprint (required)
     * - "value_b64"      – base64-encoded JSON result string (required)
     * - "ttl_remaining_s"– remaining TTL in seconds; if omitted defaults to l1_ttl_seconds
     * - "tenant"         – tenant_id; empty string or omitted means no-tenant
     *
     * Warm-up rules:
     * - Bypasses the rate limiter (internal operation).
     * - Respects per-tenant quota checks.
     * - Capped at `l1_max_entries / 2` entries total to reserve headroom for
     *   live traffic; excess entries overflow to L2.
     * - Validates SHA-256 hex format and entry size limits before insertion.
     * - Reports progress to the global MetricsCollector gauge
     *   `themis_cache_warmup_entries_loaded_total`.
     * - Partitions the log into `config_.max_parallel_workers` chunks and
     *   processes them concurrently using `std::async`, reducing startup
     *   latency for large warmup logs.
     *
     * @param log_path    Path to the NDJSON warmup log.
     * @param max_entries Hard cap on total entries loaded (0 = no extra cap).
     * @return WarmupResult with counts, timing, throughput, and error info.
     */
    WarmupResult warmupFromLog(const std::string& log_path, size_t max_entries = 0);

    /**
     * @brief Export all live (non-expired) L1 and L2 entries to an NDJSON file.
     *
     * Each output line matches the format expected by warmupFromLog().
     * The file can be used to pre-populate the cache after a restart.
     *
     * @param out_path  Destination file path (created/overwritten).
     * @return WarmupResult: entries_loaded = entries written; ok = false on I/O error.
     */
    WarmupResult exportSnapshot(const std::string& out_path) const;

    // ========================================================================
    // Phase 4: Predictive Pre-Fetching
    // ========================================================================

    /**
     * @brief Record a query access in the predictive pre-fetcher.
     *
     * Should be called each time a query is executed (hit or miss) so the
     * Markov-chain model can learn query sequence patterns.
     *
     * This is a no-op when `config_.enable_predictive_prefetch` is false.
     *
     * @param fingerprint  SHA-256 hex fingerprint of the query.
     * @param tenant_id    Optional tenant identifier.
     */
    void recordQueryAccess(const std::string& fingerprint,
                           const std::string& tenant_id = "");

    /**
     * @brief Return candidate fingerprints likely to be accessed next.
     *
     * Uses the Markov-chain model built by recordQueryAccess() to predict
     * which queries are likely to follow the current one.
     *
     * Returns an empty vector when `config_.enable_predictive_prefetch` is
     * false or when there is insufficient history for the given fingerprint.
     *
     * @param fingerprint  Current query fingerprint.
     * @param tenant_id    Optional tenant identifier.
     * @return Up to `config_.prefetch_max_predictions` candidate fingerprints.
     */
    std::vector<std::string> getPrefetchCandidates(
        const std::string& fingerprint,
        const std::string& tenant_id = "") const;

    /**
     * @brief Get predictive pre-fetcher statistics as JSON.
     *
     * Returns {"enabled": false} when `config_.enable_predictive_prefetch` is
     * false.
     */
    nlohmann::json getPrefetchStats() const;

    /**
     * @brief Account for prefetch overhead bytes (entries fetched but never hit).
     *
     * Callers should invoke this when a prefetched cache entry is evicted or
     * expires without having been accessed.  The accumulated total is exported
     * via the `cache.prefetch.overhead_bytes` metric.
     *
     * @param bytes Estimated byte size of the wasted prefetch.
     */
    void recordPrefetchOverheadBytes(uint64_t bytes);

    /**
     * @brief Persist the prefetch Markov model to the L3 RocksDB instance.
     *
     * No-op when the prefetcher is disabled or L3 is unavailable.
     */
    void savePrefetchModel();

    /**
     * @brief Restore the prefetch Markov model from the L3 RocksDB instance.
     *
     * No-op when the prefetcher is disabled or L3 is unavailable.
     */
    void loadPrefetchModel();
    // Phase 4: Cache Replication for High-Availability
    // ========================================================================

    /**
     * @brief Register a replication listener for high-availability deployments.
     *
     * Once registered, every successful put() and every invalidate() /
     * invalidateTenant() call notifies the listener so that replica nodes can
     * mirror the cache state.  Pass nullptr to unregister.
     *
     * Typical usage:
     * @code
     *   auto mgr = std::make_shared<cache::CacheReplicationManager>(repCfg);
     *   mgr->addReplica(myTransportListener, snapshotNdjson);
     *   cache.setReplicationListener(mgr);
     * @endcode
     *
     * @param listener Shared pointer to an ICacheReplicationListener
     *                 implementation; nullptr disables replication.
     */
    void setReplicationListener(
        std::shared_ptr<cache::ICacheReplicationListener> listener);

    // ========================================================================
    // Phase 5: AccessCoordinator Integration (BLOCK 2: Cache Integration)
    // ========================================================================

    /**
     * @brief Register an eviction listener for tier coordination.
     *
     * Once registered, every cache eviction from L1/L2 tiers notifies the
     * listener so that the AccessCoordinator can:
     * - Detect hot entries (high access_count) for warm-tier promotion
     * - Detect cold entries (low access_count) for warm-tier demotion
     * - Trigger L3 fallback before L1/L2 overflow
     * - Track access patterns for predictive promotion
     *
     * Pass nullptr to unregister (disables coordination).
     *
     * Typical usage:
     * @code
     *   auto coordinator = std::make_shared<access_model::AccessCoordinator>();
     *   cache.setEvictionListener(coordinator);
     * @endcode
     *
     * @param listener Pointer to an EvictionListener implementation;
     *                 nullptr disables eviction notifications.
     *
     * @see include/access_model/access_coordinator.h
     * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
     */
    void setEvictionListener(access_model::EvictionListener* listener) noexcept;

private:
    struct L1Entry {
        nlohmann::json result;                         // Read-only after insert
        std::atomic<int64_t> created_at_ms{0};         // Written at insert; reset by adaptive TTL
        std::atomic<int64_t> last_accessed_ms{0};      // Updated lock-free on every get() hit
        std::atomic<int64_t> access_count{0};          // Incremented lock-free
        std::atomic<int> ttl_seconds{0};               // Adaptive TTL writes
        std::atomic<int64_t> window_start_ms{0};       // Adaptive TTL window start
        std::atomic<uint32_t> window_count{0};         // Accesses in current window
        std::atomic<bool> expired_flag{false};         // CAS-based expiry marker for lazy cleanup

        L1Entry() = default;
        L1Entry(const L1Entry&)            = delete;
        L1Entry& operator=(const L1Entry&) = delete;
        L1Entry(L1Entry&&)                 = delete;
        L1Entry& operator=(L1Entry&&)      = delete;
    };
    
    struct L2Entry {
        std::vector<uint8_t> compressed_result;  // Zstd compressed
        int64_t created_at_ms;
        int64_t last_accessed_ms;
        int64_t access_count = 0;
        int ttl_seconds;
        // Adaptive TTL: sliding 5-minute access window
        int64_t window_start_ms = 0;
        uint32_t window_count = 0;
    };
    
    Config config_;
    mutable cache::CacheMetrics enhanced_metrics_;  // Enhanced metrics (Phase 1)
    mutable CacheStats stats_;  // Kept for backward compatibility
    
    // Circuit breaker for L3 (RocksDB) operations (Phase 1)
    std::unique_ptr<cache::CircuitBreaker> l3_circuit_breaker_;
    
    // Phase 2: Rate limiter
    std::unique_ptr<cache::RateLimiter> rate_limiter_;

    // Phase 4: Replication coordinator for HA multi-node deployments
    std::shared_ptr<cache::ICacheCoordinator> coordinator_;
    mutable std::mutex coordinator_mutex_;
    // C-4: Shared flag that the coordinator callbacks check before dereferencing
    // 'this'.  Set to false in the destructor before tearing down callbacks, so
    // any in-flight dispatch from the coordinator's background thread will find
    // the flag false and return immediately instead of calling into freed memory.
    std::shared_ptr<std::atomic<bool>> callback_alive_{ std::make_shared<std::atomic<bool>>(true) };

    // [C-4] Alive guard: prevents coordinator callbacks from accessing a destroyed
    // AdaptiveQueryCache. The guard is shared between the object and any captured
    // callbacks. The destructor marks it inactive (under the guard mutex) before
    // teardown; callbacks acquire the guard mutex and check the flag before use.
    struct AliveGuard {
        std::mutex mutex;
        bool alive = true;
    };
    std::shared_ptr<AliveGuard> alive_guard_{std::make_shared<AliveGuard>()};

    // Internal: apply a replicated entry received from a peer
    void applyReplicatedEntry(const cache::ReplicationMessage& msg);
    // Internal: apply a replicated invalidation received from a peer
    void applyReplicatedInvalidation(const cache::ReplicationMessage& msg);
    
    // Phase 3: Per-tenant cache statistics (hits, misses, evictions, bytes)
    struct TenantMetrics {
        uint64_t hits     = 0;      ///< cache hits attributed to this tenant
        uint64_t misses   = 0;      ///< cache misses attributed to this tenant
        uint64_t evictions = 0;     ///< entries evicted (via invalidateTenant)
        size_t   bytes_used = 0;    ///< estimated bytes currently consumed
    };

    // Phase 2/3: Tenant isolation – per-tenant metrics map
    std::unordered_map<std::string, TenantMetrics> tenant_metrics_;
    // Per-tenant quota overrides (0 = use global config_.per_tenant_max_bytes)
    std::unordered_map<std::string, size_t> tenant_quota_overrides_;
    mutable std::mutex tenant_mutex_;

    // Phase 5: BLOCK 2 Cache Integration — AccessCoordinator listener
    // Notified when L1/L2 entries are evicted for tier promotion/demotion
    access_model::EvictionListener* eviction_listener_{nullptr};
    mutable std::mutex eviction_listener_mutex_;

    // GDPR: PII reverse index (L1 / L2 in-memory tier)
    // Maps pii_uuid → set of cache keys that carry that UUID's data.
    // Protected by pii_index_mutex_. Entries are lazily cleaned; stale
    // references (to already-evicted keys) are harmless.
    std::unordered_map<std::string, std::unordered_set<std::string>> pii_key_index_;
    mutable std::mutex pii_index_mutex_;
    
    // L1: In-memory HashMap (lock-free read path)
    std::unordered_map<std::string, std::unique_ptr<L1Entry>> l1_cache_;
    mutable std::shared_mutex l1_mutex_;
    mutable std::mutex l1_eviction_mutex_;  // Protects l1_eviction_strategy_ calls
    
    // L2: Compressed in-memory
    std::unordered_map<std::string, L2Entry> l2_cache_;
    mutable std::mutex l2_mutex_;

    // Eviction strategy trackers (initialised from Config::l1/l2_eviction_policy)
    std::unique_ptr<core::concerns::IEvictionStrategy> l1_eviction_strategy_;
    std::unique_ptr<core::concerns::IEvictionStrategy> l2_eviction_strategy_;
    
    // L3: RocksDB persistent cache
    std::shared_ptr<RocksDBWrapper> l3_db_;
    mutable std::timed_mutex l3_mutex_;

    // Phase 4: Predictive pre-fetcher (Markov-chain query sequence model)
    std::unique_ptr<cache::PredictivePrefetcher> prefetcher_;
    // Phase 4: Cache replication listener for HA deployments
    std::shared_ptr<cache::ICacheReplicationListener> replication_listener_;
    mutable std::mutex replication_mutex_;
    
    // Internal helper methods
    int64_t getCurrentTimeMs() const;
    bool isExpired(int64_t created_at_ms, int ttl_seconds) const;
    int calculateAdaptiveTTL(int64_t access_count) const;
    CacheLevel selectCacheLevel(size_t result_size) const;
    void promoteEntry(const std::string& fingerprint, const CacheEntry& entry);
    void evictLRU(CacheLevel level);
    double calculateLRUScore(int64_t last_accessed_ms, int64_t access_count) const;
    
    // Phase 5: BLOCK 2 Cache Integration — Emit eviction events to coordinator
    void emitEvictionEvent(const std::string& key, TierLevel tier, 
                          std::size_t size_bytes, uint64_t access_count,
                          int64_t last_access_ms, std::string_view reason);
    
    // Phase 1: Size validation and security
    bool validateEntrySize(size_t size, CacheLevel level) const;
    bool isWithinSizeLimit(size_t size) const;
    
    // Phase 2: Tenant isolation helpers
    std::string makeTenantKey(const std::string& fingerprint, const std::string& tenant_id) const;
    bool checkTenantQuota(const std::string& tenant_id, size_t additional_bytes);
    // Returns the effective quota for a tenant (override if set, else global default)
    size_t getEffectiveTenantQuota(const std::string& tenant_id) const;

    // Phase 4: Write-through helper - persist a result to L3 without modifying L1/L2
    bool writeThroughToL3(const std::string& fingerprint,
                          const nlohmann::json& query_params,
                          const nlohmann::json& result,
                          int64_t now_ms,
                          int ttl_seconds);
};

} // namespace themis

