/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_optimizer.cpp                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "timeseries/query_optimizer.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <sstream>

namespace themis {

// Common aggregate window sizes (1 minute to 1 day)
const std::vector<std::chrono::milliseconds> TSQueryOptimizer::COMMON_WINDOWS = {
    std::chrono::minutes(1),
    std::chrono::minutes(5),
    std::chrono::minutes(15),
    std::chrono::hours(1),
    std::chrono::hours(6),
    std::chrono::hours(24)
};

TSQueryOptimizer::TSQueryOptimizer(TSStore* store)
    : store_(store) {
    
    if (!store_) {
        throw std::invalid_argument("TSQueryOptimizer: TSStore cannot be null");
    }
}

TSQueryOptimizer::QueryPlan TSQueryOptimizer::optimizeAggregateQuery(
    const std::string& metric,
    const std::optional<std::string>& entity,
    int64_t from_timestamp_ms,
    int64_t to_timestamp_ms,
    const OptimizationHint& hint) {
    
    auto span = Tracer::startSpan("TSQueryOptimizer.optimizeAggregateQuery");

    span.setAttribute("metric", metric);
    if (entity.has_value()) {
        span.setAttribute("entity", *entity);
    }
    span.setAttribute("time_range_ms", to_timestamp_ms - from_timestamp_ms);
    
    QueryPlan plan;
    plan.source_metric = metric;
    plan.from_timestamp_ms = from_timestamp_ms;
    plan.to_timestamp_ms = to_timestamp_ms;
    
    int64_t time_range_ms = to_timestamp_ms - from_timestamp_ms;
    size_t raw_points = estimateRawPointCount(time_range_ms);
    plan.estimated_points = raw_points;
    
    // Check if we should try to use aggregates
    if (!hint.use_aggregates) {
        plan.explanation = "Aggregates disabled by hint";
        span.setAttribute("uses_aggregate", false);
        return plan;
    }
    
    if (time_range_ms < hint.min_window_for_agg_ms) {
        plan.explanation = "Time range too small for aggregates (< " + 
                          std::to_string(hint.min_window_for_agg_ms / 1000) + "s)";
        span.setAttribute("uses_aggregate", false);
        return plan;
    }
    
    // Find best aggregate
    auto agg_metric_opt = findBestAggregate(metric, time_range_ms);
    
    if (!agg_metric_opt.has_value()) {
        plan.explanation = "No suitable aggregate found for metric: " + metric;
        span.setAttribute("uses_aggregate", false);
        span.recordError("No aggregate available");
        return plan;
    }
    
    // Extract window size from aggregate name
    std::string agg_metric = *agg_metric_opt;
    size_t pos = agg_metric.rfind("__agg_");
    if (pos == std::string::npos) {
        plan.explanation = "Invalid aggregate metric name: " + agg_metric;
        return plan;
    }
    
    std::string window_str = agg_metric.substr(pos + 6);  // Skip "__agg_"
    window_str = window_str.substr(0, window_str.length() - 2);  // Remove "ms"
    int64_t window_ms = std::stoll(window_str);
    
    size_t agg_points = estimateAggregatePointCount(time_range_ms, std::chrono::milliseconds(window_ms));
    
    // Decide whether to use aggregate based on cost
    if (!shouldUseAggregate(raw_points, agg_points, hint)) {
        plan.explanation = "Aggregate not cost-effective (raw: " + std::to_string(raw_points) + 
                          " points, agg: " + std::to_string(agg_points) + " points)";
        span.setAttribute("uses_aggregate", false);
        return plan;
    }
    
    // Use aggregate
    plan.uses_aggregate = true;
    plan.source_metric = agg_metric;
    plan.estimated_points = agg_points;
    plan.estimated_speedup = static_cast<double>(raw_points) / std::max(agg_points, size_t(1));
    plan.explanation = buildExplanation(plan, true, raw_points, agg_points);
    
    span.setAttribute("uses_aggregate", true);
    span.setAttribute("aggregate_metric", agg_metric);
    span.setAttribute("estimated_speedup", plan.estimated_speedup);
    
    THEMIS_DEBUG("Query optimized: {} → {} (speedup: {:.2f}x, raw: {}, agg: {})",
                 metric, agg_metric, plan.estimated_speedup, raw_points, agg_points);
    
    return plan;
}

TSQueryOptimizer::QueryPlan TSQueryOptimizer::optimizeAggregateQuery(
    const std::string& metric,
    const std::optional<std::string>& entity,
    int64_t from_timestamp_ms,
    int64_t to_timestamp_ms) {
    OptimizationHint hint{};
    return optimizeAggregateQuery(metric, entity, from_timestamp_ms, to_timestamp_ms, hint);
}

std::optional<std::string> TSQueryOptimizer::findBestAggregate(
    const std::string& metric,
    int64_t time_range_ms) {
    
    // Try window sizes from largest to smallest
    // Larger windows = fewer points = faster queries
    for (auto it = COMMON_WINDOWS.rbegin(); it != COMMON_WINDOWS.rend(); ++it) {
        auto window = *it;
        
        // Window should be significantly smaller than time range
        // Rule of thumb: At least 10 windows in the range
        if (window.count() * 10 > time_range_ms) {
            continue;
        }
        
        std::string agg_metric = ContinuousAggregateManager::derivedMetricName(metric, window);
        
        // Check if aggregate exists
        if (aggregateExists(metric, window)) {
            THEMIS_DEBUG("Found aggregate: {} (window: {}ms)", agg_metric, window.count());
            return agg_metric;
        }
    }
    
    return std::nullopt;
}

bool TSQueryOptimizer::aggregateExists(
    const std::string& metric,
    std::chrono::milliseconds window) {
    
    std::string agg_metric = ContinuousAggregateManager::derivedMetricName(metric, window);
    
    // Query TSStore to check if any data points exist for this aggregate
    TSStore::QueryOptions opts;
    opts.metric = agg_metric;
    opts.limit = 1;  // Just check existence
    
    auto result = store_->query(opts);
    
    return result.has_value() && !result.value().empty();
}

void TSQueryOptimizer::registerAvailableAggregate(
    const std::string& metric,
    std::chrono::milliseconds window) {
    
    // For future optimization: Could cache available aggregates
    // to avoid repeated TSStore queries
    THEMIS_DEBUG("Registered aggregate: {} (window: {}ms)", metric, window.count());
}

// ===== Helpers =====

size_t TSQueryOptimizer::estimateRawPointCount(int64_t time_range_ms) const {
    // Assume typical time-series ingestion rate: 1 point per 10 seconds
    // This is configurable and could be metric-specific
    constexpr int64_t DEFAULT_INTERVAL_MS = 10000;  // 10 seconds
    
    return static_cast<size_t>(time_range_ms / DEFAULT_INTERVAL_MS);
}

size_t TSQueryOptimizer::estimateAggregatePointCount(
    int64_t time_range_ms,
    std::chrono::milliseconds window) const {
    
    return static_cast<size_t>(time_range_ms / window.count());
}

bool TSQueryOptimizer::shouldUseAggregate(
    size_t raw_points,
    size_t agg_points,
    const OptimizationHint& hint) const {
    
    // Always use aggregate if raw points exceed max
    if (raw_points > hint.max_raw_points) {
        return true;
    }
    
    // Use aggregate if it reduces scan by at least 5x
    constexpr double MIN_SPEEDUP = 5.0;
    double speedup = static_cast<double>(raw_points) / std::max(agg_points, size_t(1));
    
    return speedup >= MIN_SPEEDUP;
}

std::string TSQueryOptimizer::buildExplanation(
    const QueryPlan& plan,
    bool used_agg,
    size_t raw_points,
    size_t agg_points) const {
    
    std::ostringstream oss;
    
    if (used_agg) {
        oss << "Using pre-computed aggregate: " << plan.source_metric << " ";
        oss << "(scans " << agg_points << " points vs " << raw_points << " raw points, ";
        oss << std::fixed << std::setprecision(1) << plan.estimated_speedup << "x speedup)";
    } else {
        oss << "Using raw data: " << plan.source_metric << " ";
        oss << "(scans " << raw_points << " points)";
    }
    
    return oss.str();
}

} // namespace themis
