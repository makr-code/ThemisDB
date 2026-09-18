/**
 * @file adaptive_query_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include "access_model/access_tier_interface.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;

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
    
    /**
     * @brief Adaptive Query Cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AdaptiveQueryCache(const Config& config);
    ~AdaptiveQueryCache();
    
    // Non-copyable, moveable
    AdaptiveQueryCache(const AdaptiveQueryCache&) = delete;
    AdaptiveQueryCache& operator=(const AdaptiveQueryCache&) = delete;
    AdaptiveQueryCache(AdaptiveQueryCache&&) noexcept = default;
    AdaptiveQueryCache& operator=(AdaptiveQueryCache&&) noexcept = default;
    
    std::string generateFingerprint(const std::string& query, 
                                    const nlohmann::json& params = {},
                                    const std::string& tenant_id = "") const;
    
    /**
     * @brief Get.
     * @param[in] fingerprint Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::optional<CacheEntry> get(const std::string& fingerprint,
                                   const std::string& tenant_id);
    
    bool put(const std::string& fingerprint,
             const nlohmann::json& query_params,
             const nlohmann::json& result,
             const std::string& tenant_id = "",
             const std::vector<std::string>& pii_uuids = {});
    
    /**
     * @brief Invalidate.
     * @param[in] pattern Input parameter.
     * @return Return value.
     */
    size_t invalidate(const std::string& pattern);
    
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

    std::optional<nlohmann::json> get(const std::string& fingerprint) override;

    void put(const std::string& fingerprint, nlohmann::json result,
             uint32_t ttl_seconds = 0) override;

    bool remove(const std::string& fingerprint) override;

    bool contains(const std::string& fingerprint) const override;

    std::size_t size() const override;
    
    /**
     * @brief Clear Expired.
     * @return Return value.
     */
    uint64_t clearExpired();
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    CacheStats getStats() const;
    
    const cache::CacheMetrics& getEnhancedMetrics() const {
        return enhanced_metrics_;
    }
    
    /**
     * @brief Get Detailed Info.
     * @return Return value.
     */
    nlohmann::json getDetailedInfo() const;
    
    /**
     * @brief ======================================================================== Phase 3: Admin API & Operational Tooling ========================================================================
     * @return Return value.
     */
    
    nlohmann::json getStatsByTier() const;
    
    /**
     * @brief Get Health Status.
     * @return Return value.
     */
    nlohmann::json getHealthStatus() const;
    
    std::vector<std::string> exportKeys(size_t max_keys = 100) const;
    
    /**
     * @brief Get Tenant Stats.
     * @return Return value.
     */
    nlohmann::json getTenantStats() const;

    /**
     * @brief Get Tenant Stats For Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    nlohmann::json getTenantStatsForTenant(const std::string& tenant_id) const;
    
    size_t bulkPut(const std::vector<std::tuple<std::string, nlohmann::json, nlohmann::json, std::string>>& entries);
    
    /**
     * @brief Invalidate Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    size_t invalidateTenant(const std::string& tenant_id);

    /**
     * @brief Invalidate PII.
     * @param[in] pii_uuid Input parameter.
     * @return Return value.
     */
    size_t invalidatePII(const std::string& pii_uuid);

    /**
     * @brief Update Tenant Quota.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] quota_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateTenantQuota(const std::string& tenant_id, size_t quota_bytes);

    /**
     * @brief Get Circuit Breaker Status.
     * @return Return value.
     */
    nlohmann::json getCircuitBreakerStatus() const;

    /**
     * @brief Reset Circuit Breaker.
     */
    void resetCircuitBreaker();

    /**
     * @brief ======================================================================== Phase 4: Cache Replication for High-Availability Multi-Node Deployments ========================================================================
     * @param[in] coordinator Input parameter.
     */

    void setCoordinator(std::shared_ptr<cache::ICacheCoordinator> coordinator);

    /**
     * @brief Get Replication Stats.
     * @return Return value.
     */
    nlohmann::json getReplicationStats() const;

    // ========================================================================
    // Phase 3: Cache Warmup and Snapshot
    // ========================================================================

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

    WarmupResult warmupFromLog(const std::string& log_path, size_t max_entries = 0);

    /**
     * @brief Export Snapshot.
     * @param[in] out_path Path to the out.
     * @return Return value.
     */
    WarmupResult exportSnapshot(const std::string& out_path) const;

    // ========================================================================
    // Phase 4: Predictive Pre-Fetching
    // ========================================================================

    void recordQueryAccess(const std::string& fingerprint,
                           const std::string& tenant_id = "");

    std::vector<std::string> getPrefetchCandidates(
        const std::string& fingerprint,
        const std::string& tenant_id = "") const;

    /**
     * @brief Get Prefetch Stats.
     * @return Return value.
     */
    nlohmann::json getPrefetchStats() const;

    /**
     * @brief Record Prefetch Overhead Bytes.
     * @param[in] bytes Input parameter.
     */
    void recordPrefetchOverheadBytes(uint64_t bytes);

    /**
     * @brief Save Prefetch Model.
     */
    void savePrefetchModel();

    /**
     * @brief Load Prefetch Model.
     */
    void loadPrefetchModel();
    /**
     * @brief Phase 4: Cache Replication for High-Availability ========================================================================
     * @param[in] listener Input parameter.
     */

    void setReplicationListener(
        std::shared_ptr<cache::ICacheReplicationListener> listener);

    /**
     * @brief ======================================================================== Phase 5: AccessCoordinator Integration (BLOCK 2: Cache Integration) ========================================================================
     * @param[in,out] listener Input/output parameter.
     * @note Exception safety: noexcept.
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

    /**
     * @brief Internal: apply a replicated entry received from a peer
     * @param[in] msg Input parameter.
     */
    void applyReplicatedEntry(const cache::ReplicationMessage& msg);
    /**
     * @brief Internal: apply a replicated invalidation received from a peer
     * @param[in] msg Input parameter.
     */
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
    /**
     * @brief Get Current Time Ms.
     * @return Return value.
     */
    int64_t getCurrentTimeMs() const;
    /**
     * @brief Is Expired.
     * @param[in] created_at_ms Input parameter.
     * @param[in] ttl_seconds Input parameter.
     * @return True when the operation succeeds.
     */
    bool isExpired(int64_t created_at_ms, int ttl_seconds) const;
    /**
     * @brief Calculate Adaptive TTL.
     * @param[in] access_count Input parameter.
     * @return Return value.
     */
    int calculateAdaptiveTTL(int64_t access_count) const;
    /**
     * @brief Select Cache Level.
     * @param[in] result_size Input parameter.
     * @return Return value.
     */
    CacheLevel selectCacheLevel(size_t result_size) const;
    /**
     * @brief Promote Entry.
     * @param[in] fingerprint Input parameter.
     * @param[in] entry Input parameter.
     */
    void promoteEntry(const std::string& fingerprint, const CacheEntry& entry);
    /**
     * @brief Evict LRU.
     * @param[in] level Input parameter.
     */
    void evictLRU(CacheLevel level);
    /**
     * @brief Calculate LRUScore.
     * @param[in] last_accessed_ms Input parameter.
     * @param[in] access_count Input parameter.
     * @return Return value.
     */
    double calculateLRUScore(int64_t last_accessed_ms, int64_t access_count) const;
    
    /**
     * @brief Phase 5: BLOCK 2 Cache Integration — Emit eviction events to coordinator
     * @param[in] key Input parameter.
     * @param[in] tier Input parameter.
     * @param[in] size_bytes Input parameter.
     * @param[in] access_count Input parameter.
     * @param[in] last_access_ms Input parameter.
     * @param[in] reason Input parameter.
     */
    void emitEvictionEvent(const std::string& key, access_model::TierLevel tier, 
                          std::size_t size_bytes, uint64_t access_count,
                          int64_t last_access_ms, std::string_view reason);
    
    /**
     * @brief Phase 1: Size validation and security
     * @param[in] size Input parameter.
     * @param[in] level Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateEntrySize(size_t size, CacheLevel level) const;
    /**
     * @brief Is Within Size Limit.
     * @param[in] size Input parameter.
     * @return True when the operation succeeds.
     */
    bool isWithinSizeLimit(size_t size) const;
    
    /**
     * @brief Phase 2: Tenant isolation helpers
     * @param[in] fingerprint Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::string makeTenantKey(const std::string& fingerprint, const std::string& tenant_id) const;
    /**
     * @brief Check Tenant Quota.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] additional_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkTenantQuota(const std::string& tenant_id, size_t additional_bytes);
    /**
     * @brief Returns the effective quota for a tenant (override if set, else global default)
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    size_t getEffectiveTenantQuota(const std::string& tenant_id) const;

    /**
     * @brief Phase 4: Write-through helper - persist a result to L3 without modifying L1/L2
     * @param[in] fingerprint Input parameter.
     * @param[in] query_params Input parameter.
     * @param[in] result Input parameter.
     * @param[in] now_ms Input parameter.
     * @param[in] ttl_seconds Input parameter.
     * @return True when the operation succeeds.
     */
    bool writeThroughToL3(const std::string& fingerprint,
                          const nlohmann::json& query_params,
                          const nlohmann::json& result,
                          int64_t now_ms,
                          int ttl_seconds);
};

} // namespace themis

