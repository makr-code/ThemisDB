/**
 * @file plan_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <string_view>

#include "query/query_optimizer.h"

namespace themis {
namespace query {

/**
 * @brief Query Plan Cache — v1.7.0 (Thread-Safe Enhanced)
 *
 * Caches optimized query plans so that repeated executions of the same
 * parameterized query can skip the parse + optimize phases.
 *
 * Features:
 *  - Plan fingerprinting  (SHA256 of query structure + statistics snapshot)
 *  - Parameterized plan reuse  (same plan, different bind values)
 *  - Plan invalidation on schema / statistics changes
 *  - Statistics-aware plan selection  (10× cardinality drift → invalidate)
 *  - Periodic refresh  (stale plans older than 24 h are evicted lazily)
 *
 * Thread Safety: all public methods are thread-safe.
 *
 * THREAD-SAFETY DETAILS:
 *  - Shared state (cache_, lru_list_, table_index_) protected by cache_mutex_
 *  - Statistics counters (GAP-4) use std::atomic<> for lock-free updates
 *  - No wait loops or unbounded operations while holding cache_mutex_
 *  - get() and put() operations respect deadline propagation via pre-lock fail-fast checks (GAP-5)
 *  - Lock ordering: cache_mutex_ is lowest level; never acquire other locks while holding it
 *
 * Invalidation Strategy:
 *  - Schema change   : invalidateTable(table) drops every plan that touches
 *                      the named table.
 *  - Statistics drift: get() compares current statistics with the snapshot
 *                      stored at cache time; if any table's row-count ratio
 *                      exceeds 10×, the plan is considered stale.
 *  - Periodic        : plans older than max_plan_age (default 24 h) are
 *                      removed on the next get() or via evictExpired().
 */
class PlanCache {
public:
    // =========================================================================
    // Supporting types
    // =========================================================================

    /**
     * @brief Lightweight statistics snapshot captured at plan-cache time.
     *
     * Stores per-table cardinality estimates that were used to produce the
     * cached plan.  On retrieval the current cardinality is compared; if the
     * ratio exceeds the drift threshold the entry is invalidated.
     */
    struct Statistics {
        /// table_name → estimated row count at plan-creation time
        std::unordered_map<std::string, size_t> table_cardinalities;

        Statistics() = default;

        explicit Statistics(
            std::unordered_map<std::string, size_t> cards)
            : table_cardinalities(std::move(cards)) {}
    };

    /**
     * @brief Metadata for a single bind parameter of a parameterized query.
     */
    struct ParameterInfo {
        std::string name;    ///< Bind-parameter name (e.g. "@age")
        std::string type;    ///< Type hint: "int", "string", "float", …
        std::string sample_value; ///< Representative value used at plan time
    };

    /**
     * @brief A fully cached, ready-to-execute query plan entry.
     */
    struct CachedPlan {
        std::string query_fingerprint;
        QueryOptimizer::Plan plan;
        std::vector<ParameterInfo> parameters;
        std::chrono::system_clock::time_point created_at;
        Statistics statistics_snapshot;
        std::string topology_fingerprint;
        size_t estimated_size_bytes = 0;
        size_t consecutive_execution_failures = 0;

        /// Tables referenced by this plan (drives table-level invalidation).
        std::vector<std::string> referenced_tables;

        bool isExpired(std::chrono::seconds max_age) const {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - created_at);
            return age >= max_age;
        }
    };

    /**
     * @brief Configuration for the plan cache.
     */
    struct Config {
        /// Maximum number of plans to hold simultaneously.
        size_t max_entries = 1000;

        /// Plans older than this are considered stale (default 24 h).
        std::chrono::seconds max_plan_age{86400};

        /// Statistics drift factor that triggers invalidation (default 10×).
        double statistics_drift_factor = 10.0;

        /// Optional approximate memory ceiling for cached plans (0 = disabled).
        size_t max_memory_bytes = 0;

        /// Start capacity evictions once memory use exceeds this fraction.
        double memory_eviction_threshold = 0.8;

        /// Evict an entry after this many consecutive execution failures.
        size_t max_consecutive_failures = 3;

        Config() = default;
    };

    /**
     * @brief Cache hit/miss statistics for monitoring.
     * 
     * THREAD-SAFETY (GAP-4): All fields use std::atomic<> for lock-free updates.
     * This avoids blocking during high-frequency counter updates.
     */
    struct CacheStats {
        std::atomic<uint64_t> hits          {0};
        std::atomic<uint64_t> misses        {0};
        std::atomic<uint64_t> invalidations {0};  ///< entries removed by table invalidation
        std::atomic<uint64_t> evictions     {0};  ///< entries removed by age or capacity
        std::atomic<uint64_t> stat_drifts   {0};  ///< entries rejected due to statistics drift
        std::atomic<size_t>   current_size  {0};
        std::atomic<size_t>   current_memory_bytes {0};

        CacheStats() = default;

        CacheStats(const CacheStats& other) noexcept {
            hits.store(other.hits.load(std::memory_order_acquire), std::memory_order_relaxed);
            misses.store(other.misses.load(std::memory_order_acquire), std::memory_order_relaxed);
            invalidations.store(other.invalidations.load(std::memory_order_acquire), std::memory_order_relaxed);
            evictions.store(other.evictions.load(std::memory_order_acquire), std::memory_order_relaxed);
            stat_drifts.store(other.stat_drifts.load(std::memory_order_acquire), std::memory_order_relaxed);
            current_size.store(other.current_size.load(std::memory_order_acquire), std::memory_order_relaxed);
            current_memory_bytes.store(other.current_memory_bytes.load(std::memory_order_acquire), std::memory_order_relaxed);
        }

        CacheStats& operator=(const CacheStats& other) noexcept {
            if (this == &other) {
                return *this;
            }
            hits.store(other.hits.load(std::memory_order_acquire), std::memory_order_relaxed);
            misses.store(other.misses.load(std::memory_order_acquire), std::memory_order_relaxed);
            invalidations.store(other.invalidations.load(std::memory_order_acquire), std::memory_order_relaxed);
            evictions.store(other.evictions.load(std::memory_order_acquire), std::memory_order_relaxed);
            stat_drifts.store(other.stat_drifts.load(std::memory_order_acquire), std::memory_order_relaxed);
            current_size.store(other.current_size.load(std::memory_order_acquire), std::memory_order_relaxed);
            current_memory_bytes.store(other.current_memory_bytes.load(std::memory_order_acquire), std::memory_order_relaxed);
            return *this;
        }

        CacheStats(CacheStats&& other) noexcept : CacheStats(other) {}
        CacheStats& operator=(CacheStats&& other) noexcept { return *this = other; }

        double hitRate() const {
            uint64_t h = hits.load(std::memory_order_acquire);
            uint64_t m = misses.load(std::memory_order_acquire);
            uint64_t total = h + m;
            return total > 0 ? static_cast<double>(h) / total : 0.0;
        }
    };

    // =========================================================================
    // Construction
    // =========================================================================

    explicit PlanCache(const Config& config);
    ~PlanCache();

    // Non-copyable
    PlanCache(const PlanCache&)            = delete;
    PlanCache& operator=(const PlanCache&) = delete;
    PlanCache(PlanCache&&)                 = default;
    PlanCache& operator=(PlanCache&&)      = default;

    // =========================================================================
    // Core API
    // =========================================================================

    /**
     * @brief Retrieve a cached plan for @p query, validating it against
     *        @p current_stats.
     *
     * Returns an empty optional when:
     *  - no entry exists for the fingerprint,
     *  - the entry has expired (age > max_plan_age),
     *  - statistics drift exceeds the configured threshold.
     *
     * @param query        Raw query string (used for fingerprinting).
     * @param current_stats Current table cardinalities for drift check.
     * @param topology_fingerprint Optional shard/topology discriminator for
     *        distributed plans. Empty means topology-agnostic.
     * @param deadline     Optional deadline (std::chrono::steady_clock).
     *        If set and exceeded before lock acquisition, returns nullopt immediately.
     *        Prevents cascading timeouts in federated query contexts (GAP-5).
     * @return Optional CachedPlan; nullopt on miss, staleness, or deadline exceeded.
     *
     * THREAD-SAFE: Acquires cache_mutex_. Deadline support is fail-fast via
     *              pre-lock deadline check (no timed mutex wait).
     */
    std::optional<CachedPlan> get(
        const std::string& query,
        const Statistics&  current_stats = Statistics{},
        const std::string& topology_fingerprint = {},
        std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

    /**
     * @brief Store an optimized plan in the cache.
     *
     * If the cache is at capacity, the least-recently-used entry is evicted.
     *
     * @param query      Raw query string.
     * @param plan       Optimized plan produced by QueryOptimizer.
     * @param stats      Statistics snapshot at plan-creation time.
     * @param params     Bind-parameter metadata (optional).
     * @param tables     Tables referenced by the plan (for schema invalidation).
     * @param topology_fingerprint Optional topology discriminator for
     *        distributed plans that should not be reused across layouts.
     * @param deadline   Optional deadline (std::chrono::steady_clock).
     *        If set and exceeded, returns immediately without caching (GAP-5).
     *
     * THREAD-SAFE: Acquires cache_mutex_. Deadline support is fail-fast via
     *              pre-lock deadline check (no timed mutex wait).
     */
    void put(const std::string&             query,
             const QueryOptimizer::Plan&    plan,
             const Statistics&              stats,
             const std::vector<ParameterInfo>& params = {},
             const std::vector<std::string>&   tables = {},
             const std::string& topology_fingerprint = {},
             std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

    /**
    * @brief Mark execution of the cached plan as failed.
    *
    * Repeated failures indicate that a once-valid plan is no longer safe to
    * reuse. Once the configured failure budget is exceeded the entry is evicted
    * and the method returns true.
    *
    * @param query Query text used for cache lookup.
    * @param topology_fingerprint Optional topology discriminator.
    * @return true if the entry was evicted after the failure was recorded.
    */
    bool recordExecutionFailure(const std::string& query,
                               const std::string& topology_fingerprint = {});

    /**
     * @brief Invalidate all cached plans that reference @p table.
     *
     * Call this whenever a schema change or bulk-write affects @p table.
     *
     * @param table  Table (collection) name.
     * @return Number of plans invalidated.
     */
    size_t invalidateTable(const std::string& table);

    /**
     * @brief Evict all plans whose age exceeds max_plan_age.
     *
     * This is called lazily during get() but can also be invoked explicitly.
     *
     * @return Number of plans evicted.
     */
    size_t evictExpired();

    /**
     * @brief Remove all entries from the cache.
     */
    void clear();

    // =========================================================================
    // Diagnostics
    // =========================================================================

    CacheStats getStats() const;

    /**
     * @brief Return approximate memory used by all cached entries.
     */
    size_t estimateCurrentMemoryBytes() const;

    /**
     * @brief Compute a deterministic fingerprint for @p query.
     *
     * The fingerprint is a 64-character hex SHA256 digest of the normalized
     * query template. Literal numbers and quoted string values are replaced with
     * placeholders so the same parameterized structure reuses one entry.
     */
    static std::string fingerprint(const std::string& query);

    /**
     * @brief Normalize a query to its reusable template form.
     *
     * Quoted string literals and numeric literals are replaced with `?`, while
     * parameter markers such as `@age` are preserved.
     */
    static std::string normalizeQueryTemplate(std::string_view query);

private:
    // -------------------------------------------------------------------------
    // Internal entry with LRU tracking
    // -------------------------------------------------------------------------
    struct Entry {
        CachedPlan plan;
        /// Iterator into lru_list_ for O(1) LRU reordering.
        std::list<std::string>::iterator lru_it;
    };

    // -------------------------------------------------------------------------
    // Helpers (caller must hold cache_mutex_)
    // -------------------------------------------------------------------------

    /// Evict the least-recently-used entry.
    void evictLRU_locked();

    /// Remove a single entry by fingerprint (updates all indexes).
    void removeEntry_locked(
        std::unordered_map<std::string, Entry>::iterator it);

    /// Return true if the cardinality ratio between snapshot and current
    /// exceeds statistics_drift_factor (in either direction).
    bool isDriftExceeded(const Statistics& snapshot,
                         const Statistics& current) const;

    std::string makeCacheKey(const std::string& query,
                             const std::string& topology_fingerprint) const;

    static size_t estimatePlanSizeBytes(const CachedPlan& plan);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    Config config_;

    mutable std::mutex cache_mutex_;

    /// fingerprint → Entry
    std::unordered_map<std::string, Entry> cache_;

    /// LRU tracking list (most-recent at front).
    std::list<std::string> lru_list_;

    /// table_name → set of fingerprints that reference it.
    std::unordered_map<std::string, std::vector<std::string>> table_index_;

    mutable CacheStats stats_;
};

// =============================================================================
// PlanReuseValidator — Roadmap item: "Add plan reuse validation" (Q3 2026)
// =============================================================================

/**
 * @brief Validates whether a cached plan is safe to reuse before execution.
 *
 * Separates the reuse-safety decision from the cache implementation so that
 * callers can apply project-specific policy (e.g. explain-plan diffing,
 * cardinality re-check, parameter compatibility) without modifying PlanCache.
 *
 * ## Usage
 * @code
 *   PlanReuseValidator validator;
 *   validator.setCardinalityDriftFactor(5.0);   // invalidate at 5× drift
 *
 *   auto entry = cache.get(query, current_stats);
 *   auto result = validator.validate(*entry, current_stats);
 *   if (entry && result.safe()) {
 *       // safe to execute the cached plan
 *   } else {
 *       // must re-plan (result.reason explains why)
 *   }
 * @endcode
 *
 * Thread safety: all methods are const/side-effect-free and safe to call
 * concurrently.
 */
class PlanReuseValidator {
public:
    /**
     * @brief Verdict returned by validate().
     */
    enum class Verdict : uint8_t {
        /// The cached plan is safe to reuse.
        SAFE = 0,
        /// The plan is stale due to statistics drift — must re-plan.
        STALE_STATISTICS = 1,
        /// The plan has exceeded its execution-failure budget — re-plan.
        FAILURE_BUDGET_EXCEEDED = 2,
        /// The plan has exceeded its maximum age — re-plan.
        PLAN_EXPIRED = 3,
    };

    /**
     * @brief Result of a validate() call with verdict and human-readable reason.
     */
    struct Result {
        Verdict     verdict{Verdict::SAFE};
        std::string reason;

        bool safe() const noexcept { return verdict == Verdict::SAFE; }
    };

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Set the cardinality-drift factor that triggers STALE_STATISTICS.
     *
     * Default: 10.0 (matches PlanCache internal threshold).
     * Callers that want earlier invalidation can use a smaller value (e.g. 5.0).
     *
     * @param factor  Positive multiplier; must be > 1.0.
     */
    void setCardinalityDriftFactor(double factor) noexcept {
        cardinality_drift_factor_ = (factor > 1.0) ? factor : 10.0;
    }

    /**
     * @brief Set the maximum allowed execution-failure count before FAILURE_BUDGET_EXCEEDED.
     *
     * Default: 3.
     */
    void setMaxFailureCount(size_t count) noexcept {
        max_failure_count_ = count;
    }

    /**
     * @brief Set the maximum plan age before PLAN_EXPIRED.
     *
     * Default: 24 h (matches PlanCache default).
     */
    void setMaxPlanAge(std::chrono::seconds age) noexcept {
        max_plan_age_ = age;
    }

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * @brief Determine whether a cached plan entry is safe to reuse.
     *
     * Checks (in order):
     * 1. Plan age vs. max_plan_age
     * 2. Execution-failure count vs. max_failure_count
     * 3. Per-table cardinality drift vs. cardinality_drift_factor
     *
     * @param entry           The PlanCache::CachedPlan candidate.
     * @param current_stats   Statistics collected immediately before this call.
     * @return                Result with verdict and human-readable reason.
     */
    Result validate(const PlanCache::CachedPlan& entry,
                    const PlanCache::Statistics& current_stats) const
    {
        using clock = std::chrono::system_clock;

        // 1. Age check
        const auto age = std::chrono::duration_cast<std::chrono::seconds>(
            clock::now() - entry.created_at);
        if (age > max_plan_age_) {
            return {Verdict::PLAN_EXPIRED,
                    "plan age " + std::to_string(age.count()) +
                    "s exceeds limit " + std::to_string(max_plan_age_.count()) + "s"};
        }

        // 2. Failure-budget check
        if (entry.consecutive_execution_failures >= max_failure_count_) {
            return {Verdict::FAILURE_BUDGET_EXCEEDED,
                    "failure_count=" + std::to_string(entry.consecutive_execution_failures) +
                    " >= max=" + std::to_string(max_failure_count_)};
        }

        // 3. Cardinality-drift check against the cached statistics snapshot
        for (const auto& [table, cached_card] : entry.statistics_snapshot.table_cardinalities) {
            if (cached_card == 0) continue;
            auto it = current_stats.table_cardinalities.find(table);
            if (it == current_stats.table_cardinalities.end()) continue;
            const double ratio = static_cast<double>(it->second) /
                                 static_cast<double>(cached_card);
            if (ratio > cardinality_drift_factor_ ||
                ratio < (1.0 / cardinality_drift_factor_))
            {
                return {Verdict::STALE_STATISTICS,
                        "table '" + table + "' cardinality drifted " +
                        std::to_string(ratio) + "x (threshold " +
                        std::to_string(cardinality_drift_factor_) + "x)"};
            }
        }

        return {Verdict::SAFE, "ok"};
    }

private:
    double              cardinality_drift_factor_{10.0};
    size_t              max_failure_count_{3};
    std::chrono::seconds max_plan_age_{86400};  // 24 h
};

} // namespace query
} // namespace themis
