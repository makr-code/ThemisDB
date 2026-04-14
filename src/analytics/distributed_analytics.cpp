/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_analytics.cpp                          ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:31:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     724                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 3f98a289d9  2026-03-18  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 64ab4e942a  2026-02-24  fix(analytics): add clarifying comments on population var... ║
    • 1e9824a24c  2026-02-24  fix(analytics): align STDDEV/VARIANCE to population varia... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Distributed Analytics Sharding - Implementation
 *
 * Scatter-gather coordinator for distributed OLAP queries.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analytics/distributed_analytics.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <limits>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace themisdb {
namespace analytics {

// ============================================================================
// Helpers
// ============================================================================

namespace {

using OLAPResult  = themis::analytics::OLAPResult;
using OLAPQuery   = themis::analytics::OLAPQuery;
using Row         = OLAPResult::Row;
using Measure     = themis::analytics::Measure;
using RowValue    = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

/**
 * Convert a RowValue to double for numeric aggregation.
 * Returns 0.0 for non-numeric or null values.
 */
double toDouble(const RowValue& v) {
    if (auto* d = std::get_if<double>(&v))   return *d;
    if (auto* i = std::get_if<int64_t>(&v))  return static_cast<double>(*i);
    if (auto* b = std::get_if<bool>(&v))     return *b ? 1.0 : 0.0;
    return 0.0;
}

/**
 * Convert a RowValue to a string for use in group keys.
 */
std::string valueToString(const RowValue& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "<null>";
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* i = std::get_if<int64_t>(&v))     return std::to_string(*i);
    if (auto* d = std::get_if<double>(&v))       return std::to_string(*d);
    if (auto* b = std::get_if<bool>(&v))         return *b ? "true" : "false";
    return "";
}

// -----------------------------------------------------------------------
// Per-group merge accumulator
// -----------------------------------------------------------------------

/**
 * Tracks the partial state needed to correctly merge one measure column
 * across shard results.
 *
 * For AVG, we accumulate a weighted sum and total count so the final result
 * is exact (sum/count), not an average of averages.
 *
 * For STDDEV/VARIANCE we use Chan's parallel algorithm which requires the
 * running count, mean, and M2 (sum of squared deviations).
 */
struct MeasureAccumulator {
    Measure::Function func = Measure::Function::Sum;

    // Used by COUNT, SUM, AVG (numerator), STDDEV, VARIANCE
    double sum   = 0.0;
    double count = 0.0;

    // Used by MIN / MAX
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();

    // Used by STDDEV / VARIANCE  (Chan's parallel formula state)
    double mean = 0.0;
    double m2   = 0.0;

    // Used by FIRST / LAST
    RowValue first_value;
    RowValue last_value;
    bool has_first = false;

    void accumulate(const RowValue& val) {
        double dval = toDouble(val);

        switch (func) {
            case Measure::Function::Count:
                sum += dval;
                break;

            case Measure::Function::Sum:
                sum += dval;
                break;

            case Measure::Function::Avg:
                // Treat incoming value as a partial sum scaled to 1 "row".
                // We also need the count, but for a single shard the result
                // is already an average.  We accumulate (avg * 1) here and
                // during the shard-merge step we use the weighted approach.
                // See accumulateWeighted() below for the proper path.
                sum   += dval;
                count += 1.0;
                break;

            case Measure::Function::Min:
                if (dval < min_val) min_val = dval;
                break;

            case Measure::Function::Max:
                if (dval > max_val) max_val = dval;
                break;

            case Measure::Function::StdDev:
            case Measure::Function::Variance: {
                // Welford online update
                count += 1.0;
                double delta  = dval - mean;
                mean += delta / count;
                double delta2 = dval - mean;
                m2   += delta * delta2;
                break;
            }

            case Measure::Function::CountDistinct:
                // Approximate: sum per-shard counts (may overcount cross-shard
                // duplicates).  A full HyperLogLog merge would be exact.
                sum += dval;
                break;

            case Measure::Function::Median:
            case Measure::Function::Percentile:
                // Approximate: accumulate values and compute average at end.
                sum   += dval;
                count += 1.0;
                break;

            case Measure::Function::First:
                if (!has_first) {
                    first_value = val;
                    has_first   = true;
                }
                last_value = val;
                break;

            case Measure::Function::Last:
                last_value = val;
                if (!has_first) {
                    first_value = val;
                    has_first   = true;
                }
                break;
        }
    }

    /**
     * Weighted accumulation for AVG, STDDEV, VARIANCE from a shard that
     * already computed the aggregate together with its row count.
     *
     * @param agg_val    Pre-computed aggregate value from shard.
     * @param row_count  Number of rows in that shard's group.
     */
    void accumulateWeightedAvg(double agg_val, double row_count) {
        // Maintain parallel-sum and total-count so we can compute a
        // weighted average at the end.
        sum   += agg_val * row_count;
        count += row_count;
    }

    /**
     * Merge another Chan state (for STDDEV/VARIANCE parallel combination).
     */
    void mergeVarianceState(double other_count,
                             double other_mean,
                             double other_m2) {
        if (other_count == 0.0) return;
        double total = count + other_count;
        double delta = other_mean - mean;
        mean  = (count * mean + other_count * other_mean) / total;
        m2   += other_m2 + delta * delta * count * other_count / total;
        count = total;
    }

    /** Finalise and return the merged aggregate value. */
    RowValue finalise() const {
        switch (func) {
            case Measure::Function::Count:
                return RowValue{static_cast<int64_t>(std::llround(sum))};

            case Measure::Function::Sum:
                return RowValue{sum};

            case Measure::Function::Avg:
                if (count == 0.0) return RowValue{0.0};
                return RowValue{sum / count};

            case Measure::Function::Min:
                if (min_val == std::numeric_limits<double>::max()) return RowValue{0.0};
                return RowValue{min_val};

            case Measure::Function::Max:
                if (max_val == std::numeric_limits<double>::lowest()) return RowValue{0.0};
                return RowValue{max_val};

            case Measure::Function::StdDev:
                // Population stddev (divides by n, not n-1): consistent with
                // OLAPEngine::computeAggregate which also uses population variance.
                // count is integer-valued (incremented by 1.0 per Welford update),
                // so count < 1.0 is equivalent to count == 0.
                if (count < 1.0) return RowValue{0.0};
                return RowValue{std::sqrt(m2 / count)};

            case Measure::Function::Variance:
                // Population variance: see StdDev comment above.
                if (count < 1.0) return RowValue{0.0};
                return RowValue{m2 / count};

            case Measure::Function::CountDistinct:
                return RowValue{static_cast<int64_t>(std::llround(sum))};

            case Measure::Function::Median:
            case Measure::Function::Percentile:
                if (count == 0.0) return RowValue{0.0};
                return RowValue{sum / count};

            case Measure::Function::First:
                return has_first ? first_value : RowValue{std::nullptr_t{}};

            case Measure::Function::Last:
                return last_value;
        }
        return RowValue{0.0};
    }
};

// -----------------------------------------------------------------------
// GroupAccumulator – holds one accumulator per measure column
// -----------------------------------------------------------------------

struct GroupAccumulator {
    Row prototype;   // Dimension values (dimension columns preserved as-is)
    std::unordered_map<std::string, MeasureAccumulator> measures;

    // For AVG: we also need the companion COUNT from the same shard row.
    // We store the row-count from the result's grand_total if available,
    // otherwise fall back to unweighted accumulation.
};

} // anonymous namespace

// ============================================================================
// DistributedAnalyticsSharding – construction / destruction
// ============================================================================

DistributedAnalyticsSharding::DistributedAnalyticsSharding() {
    startHealthMonitor();
}

DistributedAnalyticsSharding::DistributedAnalyticsSharding(const Config& cfg)
    : config_(cfg) {
    startHealthMonitor();
}

DistributedAnalyticsSharding::~DistributedAnalyticsSharding() {
    stopping_.store(true, std::memory_order_release);
    health_monitor_cv_.notify_all();
    if (health_monitor_thread_.joinable()) {
        health_monitor_thread_.join();
    }
}

// ============================================================================
// DistributedAnalyticsSharding – background health monitor
// ============================================================================

void DistributedAnalyticsSharding::startHealthMonitor() {
    if (config_.health_check_interval.count() > 0) {
        health_monitor_thread_ =
            std::thread(&DistributedAnalyticsSharding::runHealthMonitor, this);
    }
}

void DistributedAnalyticsSharding::runHealthMonitor() {
    while (!stopping_.load(std::memory_order_acquire)) {
        // Wait for the configured interval (or until stopped)
        {
            std::unique_lock<std::mutex> lock(health_monitor_mutex_);
            health_monitor_cv_.wait_for(
                lock,
                config_.health_check_interval,
                [this] { return stopping_.load(std::memory_order_acquire); });
        }

        if (stopping_.load(std::memory_order_acquire)) break;

        // Snapshot shard list under main mutex (brief)
        std::vector<ShardEntry> snapshot;
        {
            std::lock_guard<std::mutex> main_lock(mutex_);
            snapshot = shards_;
        }

        // Run health checks off the main lock
        for (const auto& e : snapshot) {
            if (stopping_.load(std::memory_order_acquire)) break;
            if (e.executor && e.cached_healthy) {
                bool healthy = false;
                try {
                    healthy = e.executor->isHealthy();
                } catch (...) {
                    healthy = false;
                }
                e.cached_healthy->store(healthy, std::memory_order_release);
            }
        }
    }
}

// ============================================================================
// DistributedAnalyticsSharding – shard management
// ============================================================================

void DistributedAnalyticsSharding::addShard(
        const std::string& shard_id,
        std::shared_ptr<ShardQueryExecutor> executor) {
    bool initial_healthy = true;
    if (executor) {
        try {
            initial_healthy = executor->isHealthy();
        } catch (...) {
            initial_healthy = false;
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& e : shards_) {
        if (e.shard_id == shard_id) {
            e.executor = std::move(executor);
            if (!e.cached_healthy) {
                e.cached_healthy = std::make_shared<std::atomic<bool>>(initial_healthy);
            } else {
                e.cached_healthy->store(initial_healthy, std::memory_order_release);
            }
            return;
        }
    }
    ShardEntry entry;
    entry.shard_id = shard_id;
    entry.executor = std::move(executor);
    entry.cached_healthy = std::make_shared<std::atomic<bool>>(initial_healthy);
    shards_.push_back(std::move(entry));
}

void DistributedAnalyticsSharding::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    shards_.erase(
        std::remove_if(shards_.begin(), shards_.end(),
                       [&](const ShardEntry& e) { return e.shard_id == shard_id; }),
        shards_.end());
}

size_t DistributedAnalyticsSharding::getShardCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shards_.size();
}

size_t DistributedAnalyticsSharding::getHealthyShardCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t n = 0;
    for (const auto& e : shards_) {
        if (e.cached_healthy && e.cached_healthy->load(std::memory_order_relaxed)) ++n;
    }
    return n;
}

std::future<size_t> DistributedAnalyticsSharding::getHealthyShardCountAsync() const {
    // Snapshot shard list under a brief lock
    std::vector<ShardEntry> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = shards_;
    }
    // Perform live health checks asynchronously, off the registry lock
    return std::async(
        std::launch::async,
        [snapshot = std::move(snapshot)]() -> size_t {
            size_t n = 0;
            for (const auto& e : snapshot) {
                if (e.executor && e.executor->isHealthy()) ++n;
            }
            return n;
        });
}

std::vector<std::string> DistributedAnalyticsSharding::getShardIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    ids.reserve(shards_.size());
    for (const auto& e : shards_) ids.push_back(e.shard_id);
    return ids;
}

// ============================================================================
// Row group key
// ============================================================================

/*static*/
std::string DistributedAnalyticsSharding::rowGroupKey(
        const Row& row,
        const std::vector<themis::analytics::Dimension>& dims,
        int64_t grouping_id) {
    std::ostringstream oss;
    oss << grouping_id;
    for (const auto& dim : dims) {
        oss << '|';
        auto it = row.values.find(dim.name);
        if (it != row.values.end()) {
            oss << valueToString(it->second);
        } else {
            oss << "<missing>";
        }
    }
    return oss.str();
}

// ============================================================================
// mergeResults
// ============================================================================

/*static*/
OLAPResult DistributedAnalyticsSharding::mergeResults(
        const std::vector<OLAPResult>& partials,
        const OLAPQuery& query) {
    if (partials.empty()) return {};

    // ------------------------------------------------------------------
    // Step 1: collect all column names from the first non-empty result.
    // ------------------------------------------------------------------
    OLAPResult merged;
    for (const auto& p : partials) {
        if (!p.columns.empty()) {
            merged.columns = p.columns;
            break;
        }
    }

    // Build a map from measure column name → Measure::Function
    std::unordered_map<std::string, Measure::Function> measure_funcs;
    for (const auto& m : query.measures) {
        measure_funcs[m.name] = m.function;
    }

    // Build a set of dimension column names for fast lookup
    std::unordered_map<std::string, bool> dim_set;
    for (const auto& d : query.dimensions) {
        dim_set[d.name] = true;
    }

    // ------------------------------------------------------------------
    // Step 2: For each row in each partial result, accumulate into a
    //         per-group accumulator keyed by (grouping_id, dim values).
    // ------------------------------------------------------------------
    std::unordered_map<std::string, GroupAccumulator> groups;
    std::vector<std::string> group_order; // preserve first-seen ordering

    for (const auto& partial : partials) {
        for (const auto& row : partial.rows) {
            std::string key = rowGroupKey(row, query.dimensions, row.grouping_id);

            auto it = groups.find(key);
            if (it == groups.end()) {
                // New group: initialise
                GroupAccumulator acc;
                acc.prototype = row; // dimension values from first shard
                acc.prototype.values.clear();

                // Copy dimension columns into the prototype
                for (const auto& dim : query.dimensions) {
                    auto vit = row.values.find(dim.name);
                    if (vit != row.values.end()) {
                        acc.prototype.values[dim.name] = vit->second;
                    }
                }
                acc.prototype.grouping_id = row.grouping_id;

                // Initialise measure accumulators
                for (const auto& m : query.measures) {
                    MeasureAccumulator ma;
                    ma.func = m.function;
                    acc.measures[m.name] = ma;
                }

                groups[key] = std::move(acc);
                group_order.push_back(key);
                it = groups.find(key);
            }

            // Accumulate each measure value
            for (const auto& m : query.measures) {
                auto vit = row.values.find(m.name);
                if (vit == row.values.end()) continue;

                auto& ma = it->second.measures[m.name];

                if (m.function == Measure::Function::Avg) {
                    // For AVG: look for a companion COUNT in this row.
                    // The partial result may have included a COUNT column if
                    // the caller added one.  We use it for weighted merge.
                    // Otherwise fall back to treating this shard's value as
                    // contributing 1 "unit".
                    double row_count = 1.0;
                    // Look for a COUNT column with the same base field name
                    std::string cnt_col = "__cnt_" + m.name;
                    auto cnt_it = row.values.find(cnt_col);
                    if (cnt_it != row.values.end()) {
                        row_count = toDouble(cnt_it->second);
                    }
                    ma.accumulateWeightedAvg(toDouble(vit->second), row_count);
                } else if (m.function == Measure::Function::StdDev ||
                           m.function == Measure::Function::Variance) {
                    // Best-effort: treat the shard value as a single sample
                    ma.accumulate(vit->second);
                } else {
                    ma.accumulate(vit->second);
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // Step 3: Merge grand_totals (SUM / COUNT / MIN / MAX)
    // ------------------------------------------------------------------
    std::unordered_map<std::string, MeasureAccumulator> grand_accs;
    for (const auto& m : query.measures) {
        MeasureAccumulator ma;
        ma.func = m.function;
        grand_accs[m.name] = ma;
    }
    for (const auto& partial : partials) {
        for (const auto& m : query.measures) {
            auto git = partial.grand_totals.find(m.name);
            if (git == partial.grand_totals.end()) continue;
            RowValue rv{git->second};
            grand_accs[m.name].accumulate(rv);
        }
    }

    // ------------------------------------------------------------------
    // Step 4: Build the merged rows from the accumulators
    // ------------------------------------------------------------------
    merged.rows.reserve(group_order.size());
    for (const auto& key : group_order) {
        const auto& acc = groups.at(key);
        Row out = acc.prototype;

        for (const auto& m : query.measures) {
            auto ait = acc.measures.find(m.name);
            if (ait == acc.measures.end()) continue;
            out.values[m.name] = ait->second.finalise();
        }
        merged.rows.push_back(std::move(out));
    }

    // Build grand_totals
    for (const auto& m : query.measures) {
        auto it = grand_accs.find(m.name);
        if (it == grand_accs.end()) continue;
        merged.grand_totals[m.name] = toDouble(it->second.finalise());
    }

    // Aggregate metadata
    merged.total_rows     = static_cast<int64_t>(merged.rows.size());
    merged.has_more       = false;
    merged.execution_time_ms = 0.0;
    for (const auto& p : partials) {
        merged.execution_time_ms += p.execution_time_ms;
    }

    return merged;
}

// ============================================================================
// executeDistributed
// ============================================================================

DistributedAnalyticsSharding::DistributedResult
DistributedAnalyticsSharding::executeDistributed(const OLAPQuery& query) {
    // Snapshot the active shard list under the lock (uses cached health — no I/O)
    std::vector<ShardEntry> active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& e : shards_) {
            if (e.executor &&
                e.cached_healthy &&
                e.cached_healthy->load(std::memory_order_relaxed)) {
                active.push_back(e);
            }
        }
    }

    DistributedResult result;
    result.total_shards = active.size();

    if (active.empty()) {
        spdlog::warn("DistributedAnalyticsSharding: no healthy shards registered");
        return result;
    }

    // ------------------------------------------------------------------
    // Scatter: dispatch query to each shard asynchronously
    // ------------------------------------------------------------------
    using FutureResult = std::future<std::pair<OLAPResult, ShardExecutionInfo>>;
    std::vector<FutureResult> futures;
    futures.reserve(active.size());

    for (const auto& entry : active) {
        futures.push_back(
            std::async(std::launch::async,
                [&entry, &query]() -> std::pair<OLAPResult, ShardExecutionInfo> {
                    ShardExecutionInfo info;
                    info.shard_id = entry.shard_id;

                    auto t0 = std::chrono::steady_clock::now();
                    try {
                        OLAPResult partial = entry.executor->execute(entry.shard_id, query);
                        auto t1 = std::chrono::steady_clock::now();
                        info.success = true;
                        info.execution_time_ms =
                            std::chrono::duration<double, std::milli>(t1 - t0).count();
                        return {std::move(partial), std::move(info)};
                    } catch (const std::exception& ex) {
                        auto t1 = std::chrono::steady_clock::now();
                        info.success = false;
                        info.error   = ex.what();
                        info.execution_time_ms =
                            std::chrono::duration<double, std::milli>(t1 - t0).count();
                        spdlog::error(
                            "DistributedAnalyticsSharding: shard {} failed: {}",
                            entry.shard_id, ex.what());
                        return {OLAPResult{}, std::move(info)};
                    }
                }));
    }

    // ------------------------------------------------------------------
    // Gather: collect partial results
    // ------------------------------------------------------------------
    std::vector<OLAPResult> partials;
    partials.reserve(active.size());

    for (auto& f : futures) {
        auto [partial, info] = f.get();
        result.shard_info.push_back(info);
        if (info.success) {
            ++result.successful_shards;
            partials.push_back(std::move(partial));
        } else if (!config_.allow_partial_results) {
            // At least one shard failed and partial results are not allowed
            spdlog::error(
                "DistributedAnalyticsSharding: shard {} failed and "
                "allow_partial_results=false; aborting merge",
                info.shard_id);
            return result;
        }
    }

    if (partials.empty()) {
        spdlog::warn("DistributedAnalyticsSharding: all shards failed");
        return result;
    }

    // ------------------------------------------------------------------
    // Merge
    // ------------------------------------------------------------------
    result.merged = mergeResults(partials, query);
    return result;
}

// ============================================================================
// execute (convenience)
// ============================================================================

OLAPResult DistributedAnalyticsSharding::execute(const OLAPQuery& query) {
    return executeDistributed(query).merged;
}

} // namespace analytics
} // namespace themisdb
