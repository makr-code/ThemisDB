/**
 * @file plan_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PlanCache {
public:
    // =========================================================================
    // Supporting types
    // =========================================================================

    struct Statistics {
        std::unordered_map<std::string, size_t> table_cardinalities;

        Statistics() = default;

        explicit Statistics(
            std::unordered_map<std::string, size_t> cards)
            : table_cardinalities(std::move(cards)) {}
    };

    struct ParameterInfo {
        std::string name;    ///< Bind-parameter name (e.g. "@age")
        std::string type;    ///< Type hint: "int", "string", "float", …
        std::string sample_value; ///< Representative value used at plan time
    };

    struct CachedPlan {
        std::string query_fingerprint;
        QueryOptimizer::Plan plan;
        std::vector<ParameterInfo> parameters;
        std::chrono::system_clock::time_point created_at;
        Statistics statistics_snapshot;
        std::string topology_fingerprint;
        size_t estimated_size_bytes = 0;
        size_t consecutive_execution_failures = 0;

        std::vector<std::string> referenced_tables;

        bool isExpired(std::chrono::seconds max_age) const {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - created_at);
            return age >= max_age;
        }
    };

    struct Config {
        size_t max_entries = 1000;

        std::chrono::seconds max_plan_age{86400};

        double statistics_drift_factor = 10.0;

        size_t max_memory_bytes = 0;

        double memory_eviction_threshold = 0.8;

        size_t max_consecutive_failures = 3;

        Config() = default;
    };

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

    /**
     * @brief Plan Cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PlanCache(const Config& config);
    ~PlanCache();

    // Non-copyable
    PlanCache(const PlanCache&)            = delete;
    PlanCache& operator=(const PlanCache&) = delete;
    PlanCache(PlanCache&&)                 noexcept = default;
    PlanCache& operator=(PlanCache&&)      noexcept = default;

    // =========================================================================
    // Core API
    // =========================================================================

    std::optional<CachedPlan> get(
        const std::string& query,
        const Statistics&  current_stats = Statistics{},
        const std::string& topology_fingerprint = {},
        std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

    void put(const std::string&             query,
             const QueryOptimizer::Plan&    plan,
             const Statistics&              stats,
             const std::vector<ParameterInfo>& params = {},
             const std::vector<std::string>&   tables = {},
             const std::string& topology_fingerprint = {},
             std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

    bool recordExecutionFailure(const std::string& query,
                               const std::string& topology_fingerprint = {});

    /**
     * @brief Invalidate Table.
     * @param[in] table Input parameter.
     * @return Return value.
     */
    size_t invalidateTable(const std::string& table);

    /**
     * @brief Evict Expired.
     * @return Return value.
     */
    size_t evictExpired();

    /**
     * @brief Clear.
     */
    void clear();

    // =========================================================================
    // Diagnostics
    // =========================================================================

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    CacheStats getStats() const;

    /**
     * @brief Estimate Current Memory Bytes.
     * @return Return value.
     */
    size_t estimateCurrentMemoryBytes() const;

    /**
     * @brief Fingerprint.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    static std::string fingerprint(const std::string& query);

    /**
     * @brief Normalize Query Template.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    static std::string normalizeQueryTemplate(std::string_view query);

private:
    // -------------------------------------------------------------------------
    // Internal entry with LRU tracking
    // -------------------------------------------------------------------------
    struct Entry {
        CachedPlan plan;
        std::list<std::string>::iterator lru_it;
    };

    /**
     * @brief ------------------------------------------------------------------------- Helpers (caller must hold cache_mutex_) -------------------------------------------------------------------------
     */

    void evictLRU_locked();

    void removeEntry_locked(
        std::unordered_map<std::string, Entry>::iterator it);

    /**
     * @brief Is Drift Exceeded.
     * @param[in] snapshot Input parameter.
     * @param[in] current Input parameter.
     * @return True when the operation succeeds.
     */
    bool isDriftExceeded(const Statistics& snapshot,
                         const Statistics& current) const;

    /**
     * @brief Make Cache Key.
     * @param[in] query Input parameter.
     * @param[in] topology_fingerprint Input parameter.
     * @return Return value.
     */
    std::string makeCacheKey(const std::string& query,
                             const std::string& topology_fingerprint) const;

    /**
     * @brief Estimate Plan Size Bytes.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    static size_t estimatePlanSizeBytes(const CachedPlan& plan);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    Config config_;

    mutable std::mutex cache_mutex_;

    std::unordered_map<std::string, Entry> cache_;

    std::list<std::string> lru_list_;

    std::unordered_map<std::string, std::vector<std::string>> table_index_;

    mutable CacheStats stats_;
};

// =============================================================================
// PlanReuseValidator — Roadmap item: "Add plan reuse validation" (Q3 2026)
// =============================================================================

class PlanReuseValidator {
public:
    enum class Verdict : uint8_t {
        SAFE = 0,
        STALE_STATISTICS = 1,
        FAILURE_BUDGET_EXCEEDED = 2,
        PLAN_EXPIRED = 3,
    };

    struct Result {
        Verdict     verdict{Verdict::SAFE};
        std::string reason;

        bool safe() const noexcept { return verdict == Verdict::SAFE; }
    };

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    void setCardinalityDriftFactor(double factor) noexcept {
        cardinality_drift_factor_ = (factor > 1.0) ? factor : 10.0;
    }

    void setMaxFailureCount(size_t count) noexcept {
        max_failure_count_ = count;
    }

    void setMaxPlanAge(std::chrono::seconds age) noexcept {
        max_plan_age_ = age;
    }

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

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
            if (cached_card == 0) {
              continue;
            }
            auto it = current_stats.table_cardinalities.find(table);
            if (it == current_stats.table_cardinalities.end()) {
              continue;
            }
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
