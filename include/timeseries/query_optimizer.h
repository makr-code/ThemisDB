/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_optimizer.h                                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace themis {

class TSStore;
struct AggConfig;

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
    struct OptimizationHint {
        bool use_aggregates = true;  // Try to use pre-aggregates
        int64_t min_window_for_agg_ms = 3600000;  // Min time range to use aggregates (1 hour)
        size_t max_raw_points = 10000;  // Max raw points before forcing aggregates
        bool explain = false;  // Return optimization plan
    };
    
    struct QueryPlan {
        bool uses_aggregate = false;
        std::string source_metric;  // Original or aggregate metric name
        int64_t from_timestamp_ms;
        int64_t to_timestamp_ms;
        size_t estimated_points;
        double estimated_speedup = 1.0;  // Speedup factor vs raw query
        std::string explanation;
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

private:
    TSStore* store_;
    
    // Common aggregate window sizes (in increasing order)
    static const std::vector<std::chrono::milliseconds> COMMON_WINDOWS;
    
    // Helpers
    size_t estimateRawPointCount(int64_t time_range_ms) const;
    size_t estimateAggregatePointCount(int64_t time_range_ms, std::chrono::milliseconds window) const;
    bool shouldUseAggregate(size_t raw_points, size_t agg_points, const OptimizationHint& hint) const;
    std::string buildExplanation(const QueryPlan& plan, bool used_agg, size_t raw_points, size_t agg_points) const;
};

} // namespace themis
