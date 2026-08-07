/**
 * @file query_optimizer.h
 * @brief Phase 2 hardening: Deterministic range-query optimizer with downsampling consistency.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2 — Core Implementation Complete
 * 
 * ## Overview
 * 
 * TSQueryOptimizer implements deterministic range-query semantics and query plan optimization
 * with explicit compliance to the frozen timeseries_api_contract.h.
 * 
 * ## Key Features
 * 
 * - **Deterministic Range Queries**: Inclusive bounds [start, end] guarantee exact matches
 * - **Downsampling Consistency**: Identical inputs produce identical bucket counts and aggregates
 * - **Query Plan Caching**: Performance optimization with explicit cache validation
 * - **Retention Awareness**: Queries respect retention boundaries without error raising
 * - **Predicate Pushdown**: Tag-based filtering reduces data scanned
 * - **Aggregate Detection**: Automatic use of pre-computed aggregates when beneficial
 * 
 * ## Thread Safety
 * 
 * - Query plan cache protected by internal std::mutex
 * - Safe for concurrent optimizeAggregateQuery() calls from multiple threads
 * - Cache is thread-local safe via lock_guard
 * 
 * ## Performance Expectations
 * 
 * - Range-query p99: ≤ 500µs (GATE-TSRG-02)
 * - Downsampling p99: ≤ 1ms (GATE-TSRG-04)
 * - Series lookup p99: ≤ 50µs (GATE-TSRG-06)
 * 
 * @see include/timeseries/timeseries_api_contract.h § 2 (Range-query) and § 3 (Downsampling)
 * @see src/timeseries/ROADMAP.md — Phase 2 items
 * @see src/timeseries/PERFORMANCE_EXPECTATIONS.md
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace themis {

class TSStore;
struct AggConfig;
class TierSelector;

/**
 * Query optimizer for time-series queries.
 * 
 * Automatically rewrites queries to use pre-computed continuous aggregates
 * when available, improving query performance by orders of magnitude.
 * 
 * Features:
 * - Automatic aggregate detection (checks for {metric}__agg_{window}ms)
 * - Cost-based optimization (prefer aggregates for large time ranges)
 * - Multi-level aggregate selection (1m → 1h → 1d)
 * - Fallback to raw data when aggregates unavailable
 * - Query rewrite explanations
 * - Predicate filter support for tag-based pre-filtering
 * - Query result plan caching for repeated identical queries
 * 
 * Example:
 *   Query: SELECT avg(cpu_usage) FROM server01 WHERE time >= now() - 7d
 *   
 *   Without optimizer: Scans 604,800 raw data points (1 per 10s * 7 days)
 *   With optimizer:    Scans 168 hourly aggregates (24 * 7 days)
 *   Speedup:          ~3600x
 */
class TSQueryOptimizer {
public:
    /**
     * @brief Predicate filter for tag-based pre-filtering
     *
     * Specifies tag key/value constraints that the optimizer can push down
     * to reduce the amount of data scanned.
     */
    struct PredicateFilter {
        std::string tag_key;           ///< Tag key to filter on (e.g., "region")
        std::string tag_value;         ///< Required value (e.g., "us-east")
        bool required = true;          ///< If true, only return points matching this filter

        // Convenience factory
        static PredicateFilter eq(const std::string& key, const std::string& value) {
            return {key, value, true};
        }
    };

    struct OptimizationHint {
        bool use_aggregates = true;  // Try to use pre-aggregates
        int64_t min_window_for_agg_ms = 3600000;  // Min time range to use aggregates (1 hour)
        size_t max_raw_points = 10000;  // Max raw points before forcing aggregates
        bool explain = false;  // Return optimization plan
        std::vector<PredicateFilter> predicates;  ///< Optional tag predicates to push down
        bool use_cache = true;  ///< Whether to use/populate the query plan cache

        /// Decode-width hint for the Gorilla SIMD decoder.
        /// Tells the decoder which value width to use for width-specific vectorisation paths,
        /// avoiding unnecessary bit-width checks in tight decode loops.
        enum class DecodeWidth { Auto, Float32, Float64 };
        DecodeWidth decode_width = DecodeWidth::Auto;
    };
    
    struct QueryPlan {
        bool uses_aggregate = false;
        std::string source_metric;  // Original or aggregate metric name
        int64_t from_timestamp_ms;
        int64_t to_timestamp_ms;
        size_t estimated_points;
        double estimated_speedup = 1.0;  // Speedup factor vs raw query
        std::string explanation;
        std::vector<PredicateFilter> active_predicates;  ///< Predicates included in this plan
        /// Decode-width resolved from the hint, propagated to the SIMD decoder.
        OptimizationHint::DecodeWidth decode_width = OptimizationHint::DecodeWidth::Auto;
    };
    
    explicit TSQueryOptimizer(TSStore* store);
    
    /**
     * Optimize an aggregate query (min/max/avg/sum/count).
     * 
     * Returns optimized query plan that may use pre-computed aggregates.
     * If no suitable aggregate found, returns plan for raw data query.
     */
    QueryPlan optimizeAggregateQuery(
        const std::string& metric,
        const std::optional<std::string>& entity,
        int64_t from_timestamp_ms,
        int64_t to_timestamp_ms,
        const OptimizationHint& hint
    );

    // Overload without hint (uses defaults)
    QueryPlan optimizeAggregateQuery(
        const std::string& metric,
        const std::optional<std::string>& entity,
        int64_t from_timestamp_ms,
        int64_t to_timestamp_ms
    );
    
    /**
     * Find best available aggregate for a given metric and time range.
     * 
     * Searches for aggregates with window sizes: 1m, 5m, 15m, 1h, 6h, 1d
     * Returns aggregate with largest window that fits the query time range.
     */
    std::optional<std::string> findBestAggregate(
        const std::string& metric,
        int64_t time_range_ms
    );
    
    /**
     * Check if an aggregate exists for a metric with specific window.
     */
    bool aggregateExists(
        const std::string& metric,
        std::chrono::milliseconds window
    );
    
    /**
     * Register available aggregates (for caching).
     * 
     * Optimizer will check these before querying TSStore.
     */
    void registerAvailableAggregate(
        const std::string& metric,
        std::chrono::milliseconds window
    );

    // ========== Query Plan Cache ==========

    /**
     * Clear the internal query plan cache.
     */
    void clearCache();

    /**
     * Returns the number of plans currently in the cache.
     */
    size_t cacheSize() const;

    /**
     * Returns total cache hits since creation or last reset.
     */
    uint64_t cacheHits() const { return cache_hits_.load(); }

    /**
     * Returns total cache misses since creation or last reset.
     */
    uint64_t cacheMisses() const { return cache_misses_.load(); }

    // ========== Index-Aware Query Planning ==========

    /**
     * @brief Known index type hints for a metric.
     *
     * When the caller knows that a particular index is available for a metric,
     * it can register the hint so the optimizer can factor index access cost
     * into the query plan.
     */
    enum class IndexType {
        None,        ///< No secondary index
        TimeRange,   ///< RocksDB column-family per time-chunk (Hypertable)
        Bloom,       ///< Bloom filter on entity/tag fields
        Inverted     ///< Full inverted index on tags
    };

    struct IndexHint {
        std::string metric;
        IndexType   type = IndexType::None;
        double      selectivity = 1.0; ///< Estimated fraction of rows selected (0.0 – 1.0)
    };

    /**
     * Register an index hint for a metric.
     * When the optimizer builds a plan for this metric, it considers the
     * registered index to estimate the effective scan cost.
     */
    void registerIndexHint(IndexHint hint);

    /**
     * Retrieve the registered index hint for a metric (if any).
     */
    std::optional<IndexHint> getIndexHint(const std::string& metric) const;

    // ========== Downsampling Tier Integration ==========

    /**
     * @brief Attach a TierSelector so the optimizer can route queries to
     *        downsampling tiers when a suitable resolution is available.
     *
     * The selector is NOT owned by the optimizer.  Pass nullptr to disable
     * tier-based routing.
     */
    void setTierSelector(const TierSelector* selector);

    /**
     * @brief Optimise a query taking downsampling tiers into account.
     *
     * Like optimizeAggregateQuery() but additionally consults the registered
     * TierSelector to find the coarsest tier whose resolution is ≤
     * `requested_resolution_ms`.  If a matching tier exists it is preferred
     * over the standard continuous-aggregate lookup.
     *
     * @param requested_resolution_ms  The finest granularity the caller needs
     *                                 (0 = full resolution, skip tier routing).
     */
    QueryPlan optimizeWithTiers(
        const std::string& metric,
        const std::optional<std::string>& entity,
        int64_t from_timestamp_ms,
        int64_t to_timestamp_ms,
        std::chrono::milliseconds requested_resolution_ms,
        const OptimizationHint& hint);

private:
    TSStore* store_;
    
    // Common aggregate window sizes (in increasing order)
    static const std::vector<std::chrono::milliseconds> COMMON_WINDOWS;
    
    // Cache hit/miss counters — atomic for thread safety
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, QueryPlan> plan_cache_;
    mutable std::atomic<uint64_t> cache_hits_{0};
    mutable std::atomic<uint64_t> cache_misses_{0};

    // Index hints registry
    mutable std::mutex index_mutex_;
    std::unordered_map<std::string, IndexHint> index_hints_;

    // Optional downsampling tier selector (not owned)
    const TierSelector* tier_selector_{nullptr};

    // Helpers
    std::string buildCacheKey(const std::string& metric,
                               const std::optional<std::string>& entity,
                               int64_t from_ms, int64_t to_ms,
                               const OptimizationHint& hint) const;
    size_t estimateRawPointCount(int64_t time_range_ms) const;
    size_t estimateAggregatePointCount(int64_t time_range_ms, std::chrono::milliseconds window) const;
    bool shouldUseAggregate(size_t raw_points, size_t agg_points, const OptimizationHint& hint) const;
    std::string buildExplanation(const QueryPlan& plan, bool used_agg, size_t raw_points, size_t agg_points) const;
};

} // namespace themis
