/*
 * ThemisDB | File: plan_cache.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>

#include "query/query_optimizer.h"

namespace themis {
namespace query {

/**
 * @brief Query Plan Cache — v1.7.0
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

        Config() = default;
    };

    /**
     * @brief Cache hit/miss statistics for monitoring.
     */
    struct CacheStats {
        uint64_t hits          = 0;
        uint64_t misses        = 0;
        uint64_t invalidations = 0;  ///< entries removed by table invalidation
        uint64_t evictions     = 0;  ///< entries removed by age or capacity
        uint64_t stat_drifts   = 0;  ///< entries rejected due to statistics drift
        size_t   current_size  = 0;

        double hitRate() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
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
     * @return Optional CachedPlan; nullopt on miss or staleness.
     */
    std::optional<CachedPlan> get(
        const std::string& query,
        const Statistics&  current_stats = Statistics{});

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
     */
    void put(const std::string&             query,
             const QueryOptimizer::Plan&    plan,
             const Statistics&              stats,
             const std::vector<ParameterInfo>& params = {},
             const std::vector<std::string>&   tables = {});

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
     * @brief Compute a deterministic fingerprint for @p query.
     *
     * The fingerprint is a 64-character hex SHA256 digest of the query text.
     * It is stable across process restarts and architectures.
     */
    static std::string fingerprint(const std::string& query);

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

} // namespace query
} // namespace themis
