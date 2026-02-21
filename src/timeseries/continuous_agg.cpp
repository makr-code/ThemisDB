/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_agg.cpp                                 ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     228                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5f378814d  2026-02-21  TimeSeries Module – Production Readiness Roadmap (All 7 P... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "timeseries/continuous_agg.h"
#include "timeseries/tsstore.h"
#include <sstream>
#include <algorithm>

namespace themis {

// ============================================================
// Multi-Shard helpers
// ============================================================

AggShardResult mergeShardResults(const std::vector<AggShardResult>& shards) {
    AggShardResult merged;
    if (shards.empty()) return merged;

    merged.min   = std::numeric_limits<double>::max();
    merged.max   = std::numeric_limits<double>::lowest();
    merged.valid = false;

    bool fields_initialized = false;
    for (const auto& s : shards) {
        if (!s.valid) continue;
        if (!fields_initialized) {
            // Initialize metadata fields from first valid shard
            merged.metric  = s.metric;
            merged.entity  = s.entity;
            merged.from_ms = s.from_ms;
            merged.to_ms   = s.to_ms;
            fields_initialized = true;
        }
        merged.valid  = true;
        merged.sum   += s.sum;
        merged.count += s.count;
        if (s.min < merged.min) merged.min = s.min;
        if (s.max > merged.max) merged.max = s.max;
        if (s.from_ms < merged.from_ms) merged.from_ms = s.from_ms;
        if (s.to_ms   > merged.to_ms)   merged.to_ms   = s.to_ms;
    }
    if (!merged.valid) {
        merged.min = 0.0;
        merged.max = 0.0;
    }
    return merged;
}

// ============================================================
// DistributedAggregateCoordinator
// ============================================================

DistributedAggregateCoordinator::DistributedAggregateCoordinator(
    TSStore* local_store,
    int shard_count,
    ShardQueryFn shard_query)
    : local_store_(local_store)
    , shard_count_(shard_count > 0 ? shard_count : 1)
    , shard_query_(std::move(shard_query)) {}

AggShardResult DistributedAggregateCoordinator::refreshAggregate(
    const AggConfig& cfg,
    int64_t from_ms,
    int64_t to_ms) {

    if (!shard_query_ || shard_count_ <= 1) {
        // Single-node: compute locally
        ContinuousAggregateManager mgr(local_store_);
        mgr.refresh(cfg, from_ms, to_ms);

        // Gather the result from store
        TSStore::QueryOptions qopt;
        qopt.metric          = ContinuousAggregateManager::derivedMetricName(cfg.metric, cfg.window.size);
        qopt.entity          = cfg.entity.value_or("");
        qopt.from_timestamp_ms = from_ms;
        qopt.to_timestamp_ms   = to_ms;
        qopt.limit           = 1000000;

        AggShardResult result;
        result.metric  = cfg.metric;
        result.entity  = cfg.entity.value_or("");
        result.from_ms = from_ms;
        result.to_ms   = to_ms;

        if (local_store_) {
            auto pts = local_store_->query(qopt);
            if (pts.has_value() && !pts->empty()) {
                result.valid = true;
                result.min = std::numeric_limits<double>::max();
                result.max = std::numeric_limits<double>::lowest();
                for (const auto& p : *pts) {
                    result.sum   += p.value;
                    result.count += 1;
                    if (p.value < result.min) result.min = p.value;
                    if (p.value > result.max) result.max = p.value;
                }
            }
        }
        return result;
    }

    // Multi-shard: fan-out
    std::vector<AggShardResult> partial_results;
    partial_results.reserve(shard_count_);
    for (int s = 0; s < shard_count_; ++s) {
        partial_results.push_back(shard_query_(s, cfg, from_ms, to_ms));
    }
    return mergeShardResults(partial_results);
}

// ============================================================
// ContinuousAggregateManager
// ============================================================

std::string ContinuousAggregateManager::derivedMetricName(const std::string& base, std::chrono::milliseconds win) {
    std::ostringstream oss;
    oss << base << "__agg_" << win.count() << "ms";
    return oss.str();
}

void ContinuousAggregateManager::refresh(const AggConfig& cfg, int64_t from_ms, int64_t to_ms) {
    if (!store_) return;
    const auto win_ms = cfg.window.size.count();
    const std::string out_metric = derivedMetricName(cfg.metric, cfg.window.size);

    // For MVP: if entity is provided, aggregate for that entity; otherwise, do nothing
    if (!cfg.entity.has_value()) return;
    const std::string entity = *cfg.entity;

    // Iterate windows
    for (int64_t wstart = from_ms; wstart <= to_ms; wstart += win_ms) {
        int64_t wend = std::min(wstart + win_ms - 1, to_ms);

        TSStore::QueryOptions qopt;
        qopt.metric = cfg.metric;
        qopt.entity = entity;
        qopt.from_timestamp_ms = wstart;
        qopt.to_timestamp_ms = wend;
        qopt.limit = 1000000; // big window cap

        auto result = store_->query(qopt);
        if (!result.has_value() || result->empty()) {
            continue;
        }
        const auto& points = *result;

        // Compute aggregates
        double minv = points[0].value;
        double maxv = points[0].value;
        double sum = 0.0;
        for (const auto& p : points) {
            if (p.value < minv) minv = p.value;
            if (p.value > maxv) maxv = p.value;
            sum += p.value;
        }
        size_t count = points.size();
        double avg = sum / static_cast<double>(count);

        // Store one data point per window at window end with avg as value and metadata
        TSStore::DataPoint out;
        out.metric = out_metric;
        out.entity = entity;
        out.timestamp_ms = wend;
        out.value = avg;
        out.metadata = {
            {"min", minv}, {"max", maxv}, {"sum", sum}, {"count", count},
            {"from_ms", wstart}, {"to_ms", wend}
        };
        store_->putDataPoint(out);
    }
}

RollupHierarchy RollupHierarchy::defaultHierarchy(const std::string& metric,
                                                   const std::optional<std::string>& entity) {
    return RollupHierarchy{
        metric,
        entity,
        {
            std::chrono::minutes(1),
            std::chrono::minutes(5),
            std::chrono::hours(1),
            std::chrono::hours(24)
        }
    };
}

void ContinuousAggregateManager::refreshHierarchy(const RollupHierarchy& hierarchy,
                                                    int64_t from_ms, int64_t to_ms) {
    if (!store_ || hierarchy.levels.empty()) return;

    // Process each level in order (smallest → largest)
    // First level reads from raw metric; subsequent levels read from previous level's output
    std::string source_metric = hierarchy.metric;
    for (const auto& level_window : hierarchy.levels) {
        AggConfig cfg;
        cfg.metric = source_metric;
        cfg.entity = hierarchy.entity;
        cfg.window.size = level_window;
        refresh(cfg, from_ms, to_ms);
        // Next level reads from this level's output
        source_metric = derivedMetricName(source_metric, level_window);
    }
}

} // namespace themis
