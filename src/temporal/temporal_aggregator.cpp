/**
 * @file temporal_aggregator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Aggregator Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_aggregator.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace themisdb {
namespace temporal {

// ============================================================================
// Public API
// ============================================================================

std::vector<AggregateResult> TemporalAggregator::aggregate(
    const SystemVersionedTable& table,
    const AggregationSpec& spec,
    Timestamp from,
    Timestamp to) const {

    if (from >= to) {
        return {};
    }

    // Collect all rows whose sys_start falls into the requested range.
    // Use getAllKeys()+getHistory so deleted/non-current keys are included,
    // and filter explicitly by sys_start to match event-time window semantics.
    std::vector<VersionedDocument> rows;
    {
        const auto keys = table.getAllKeys();
        for (const auto& key : keys) {
            auto hist = table.getHistory(key);
            for (auto& h : hist) {
                const bool after_from = (from == kMinTimestamp) || (h.sys_time.start >= from);
                const bool before_to  = (to == kMaxTimestamp) || (h.sys_time.start < to);
                if (after_from && before_to) {
                    rows.push_back(std::move(h));
                }
            }
        }
    }

    if (rows.empty()) {
        // For TUMBLING/SLIDING with sentinel range we would generate infinite
        // windows; return empty directly.
        if (from == kMinTimestamp || to == kMaxTimestamp) {
            return {};
        }
    }

    // Narrow sentinel bounds to the actual data range to avoid infinite
    // window loops in TUMBLING / SLIDING.
    if (from == kMinTimestamp || to == kMaxTimestamp) {
        Timestamp data_min = kMaxTimestamp;
        Timestamp data_max = kMinTimestamp;
        for (const auto& r : rows) {
            if (r.sys_time.start < data_min) {
              data_min = r.sys_time.start;
            }
            if (r.sys_time.start > data_max) {
              data_max = r.sys_time.start;
            }
        }
        if (from == kMinTimestamp) {
          from = data_min;
        }
        if (to   == kMaxTimestamp) to   = data_max + 1; // exclusive end
    }

    // Sort deterministically by event-time. For equal sys_start timestamps,
    // tie-break by sys_end and key so FIRST/LAST are stable across platforms.
    std::sort(rows.begin(), rows.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  if (a.sys_time.start != b.sys_time.start) {
                      return a.sys_time.start < b.sys_time.start;
                  }
                  if (a.sys_time.end != b.sys_time.end) {
                      return a.sys_time.end < b.sys_time.end;
                  }
                  return a.key < b.key;
              });

    switch (spec.window_type) {
        case WindowType::TUMBLING:
            return computeTumbling(rows, spec, from, to);
        case WindowType::SLIDING:
            return computeSliding(rows, spec, from, to);
        case WindowType::SESSION:
            return computeSession(rows, spec, from, to);
    }
    return {};
}

std::vector<AggregateResult> TemporalAggregator::aggregate(
    const SystemVersionedTable& table,
    const AggregationSpec& spec) const {
    return aggregate(table, spec, kMinTimestamp, kMaxTimestamp);
}

// ============================================================================
// aggregateByGroup – temporal GROUP BY
// ============================================================================

std::map<std::string, std::vector<AggregateResult>>
TemporalAggregator::aggregateByGroup(
    const SystemVersionedTable& table,
    const AggregationSpec& spec,
    Timestamp from,
    Timestamp to) const {

    if (from >= to) {
        return {};
    }

    if (spec.group_by_fields.empty()) {
        // No grouping fields: return a single unnamed group using the standard path.
        std::map<std::string, std::vector<AggregateResult>> result;
        result[""] = aggregate(table, spec, from, to);
        return result;
    }

    // Collect and sort all in-range rows.
    // Use getAllKeys() so that deleted/expired keys (not current at kMaxTimestamp)
    // are also included; their history may still overlap [from, to).
    std::vector<VersionedDocument> all_rows;
    {
        const auto keys = table.getAllKeys();
        for (const auto& k : keys) {
            auto hist = table.getHistoryInRange(k, {from, to});
            for (auto& h : hist) {
                all_rows.push_back(std::move(h));
            }
        }
    }

    // Narrow sentinel bounds.
    Timestamp eff_from = from;
    Timestamp eff_to   = to;
    if (!all_rows.empty()) {
        if (eff_from == kMinTimestamp || eff_to == kMaxTimestamp) {
            Timestamp data_min = kMaxTimestamp;
            Timestamp data_max = kMinTimestamp;
            for (const auto& r : all_rows) {
                if (r.sys_time.start < data_min) {
                  data_min = r.sys_time.start;
                }
                if (r.sys_time.start > data_max) {
                  data_max = r.sys_time.start;
                }
            }
            if (eff_from == kMinTimestamp) {
              eff_from = data_min;
            }
            if (eff_to   == kMaxTimestamp) {
              eff_to   = data_max + 1;
            }
        }
    }

    std::sort(all_rows.begin(), all_rows.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
              });

    // Partition rows by group key.
    std::map<std::string, std::vector<VersionedDocument>> groups;
    std::map<std::string, std::map<std::string, std::string>> group_kv_map;

    for (const auto& row : all_rows) {
        auto [key, kv] = buildGroupKey(row.data, spec.group_by_fields);
        groups[key].push_back(row);
        if (group_kv_map.find(key) == group_kv_map.end()) {
            group_kv_map[key] = std::move(kv);
        }
    }

    // For each group, run the window computation and tag results.
    std::map<std::string, std::vector<AggregateResult>> output;
    for (auto& [gkey, grows] : groups) {
        std::vector<AggregateResult> windows;
        switch (spec.window_type) {
            case WindowType::TUMBLING:
                windows = computeTumbling(grows, spec, eff_from, eff_to);
                break;
            case WindowType::SLIDING:
                windows = computeSliding(grows, spec, eff_from, eff_to);
                break;
            case WindowType::SESSION:
                windows = computeSession(grows, spec, eff_from, eff_to);
                break;
        }
        const auto& kv = group_kv_map[gkey];
        for (auto& w : windows) {
            w.group_values = kv;
        }
        output[gkey] = std::move(windows);
    }
    return output;
}

std::map<std::string, std::vector<AggregateResult>>
TemporalAggregator::aggregateByGroup(
    const SystemVersionedTable& table,
    const AggregationSpec& spec) const {
    return aggregateByGroup(table, spec, kMinTimestamp, kMaxTimestamp);
}

// ============================================================================
// aggregateSnapshots – state-based snapshot aggregation
// ============================================================================

std::vector<AggregateResult> TemporalAggregator::aggregateSnapshots(
    const SystemVersionedTable& table,
    const AggregationSpec& spec,
    Timestamp from,
    Timestamp to) const {

    if (from >= to || spec.window_size_ms <= 0) {
        return {};
    }

    // Collect every version (current + history) for all keys so we can
    // reconstruct table state at arbitrary points in time.
    std::vector<VersionedDocument> all_versions;
    {
        const auto keys = table.getAllKeys();
        for (const auto& k : keys) {
            auto hist = table.getHistory(k);
            for (auto& v : hist) {
                all_versions.push_back(std::move(v));
            }
        }
    }

    // Short-circuit: nothing to aggregate when the table is empty.
    if (all_versions.empty()) {
        return {};
    }

    // Pre-sort versions by sys_start so we can maintain an incremental active set
    // across snapshot ticks instead of re-scanning all versions at every tick.
    // This reduces complexity from O(ticks × all_versions) to O(ticks × max_active).
    std::sort(all_versions.begin(), all_versions.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
              });

    std::vector<AggregateResult> results;
    std::vector<const VersionedDocument*> active; // versions active at current tick
    size_t next_to_activate = 0;

    for (Timestamp snap = from; snap < to; snap += spec.window_size_ms) {
        // Activate versions whose sys_start ≤ snap (they have arrived by this tick).
        while (next_to_activate < all_versions.size() &&
               all_versions[next_to_activate].sys_time.start <= snap) {
            active.push_back(&all_versions[next_to_activate++]);
        }

        // Purge versions that ended before or at snap (sys_end ≤ snap means not
        // visible at snap because visibility requires sys_start ≤ snap < sys_end).
        active.erase(
            std::remove_if(active.begin(), active.end(),
                           [snap](const VersionedDocument* v) {
                               return v->sys_time.end <= snap;
                           }),
            active.end());

        // Aggregate over the active set.
        std::vector<double> values;
        std::vector<std::tuple<Timestamp, Timestamp, double>> snap_ordered;
        size_t count = 0;
        const bool snap_need_order = (spec.func == AggregateFunc::FIRST_VALUE ||
                                       spec.func == AggregateFunc::LAST_VALUE);

        for (const VersionedDocument* ver : active) {
            ++count;
            auto val = extractMeasure(ver->data, spec.measure_field);
            if (val.has_value() && spec.func != AggregateFunc::COUNT) {
                if (snap_need_order) {
                    snap_ordered.emplace_back(ver->sys_time.start,
                                              ver->sys_time.end,
                                              *val);
                } else {
                    values.push_back(*val);
                }
            }
        }

        if (snap_need_order && !snap_ordered.empty()) {
            std::sort(snap_ordered.begin(), snap_ordered.end());
            for (auto& v : snap_ordered) {
              values.push_back(std::get<2>(v));
            }
        }

        if (spec.func == AggregateFunc::COUNT || count > 0) {
            AggregateResult res;
            res.window_start = snap;
            res.window_end   = snap + spec.window_size_ms;
            if (res.window_end > to) {
              res.window_end = to;
            }
            res.record_count = count;
            res.value        = applyFunc(spec.func, values, count);
            results.push_back(res);
        }
    }

    return results;
}

// ============================================================================
// analyzeTrend – linear trend analysis
// ============================================================================

TrendResult TemporalAggregator::analyzeTrend(
    const SystemVersionedTable& table,
    const std::string& measure_field,
    Timestamp from,
    Timestamp to,
    int64_t window_size_ms) const {

    TrendResult trend;
    trend.period_start = from;
    trend.period_end   = to;

    if (from >= to) {
        return trend;
    }

    // Auto-derive window size: split range into ~10 buckets (minimum 1 ms).
    int64_t effective_window = window_size_ms;
    if (effective_window <= 0) {
        effective_window = std::max<int64_t>(1LL, (to - from) / 10);
    }

    AggregationSpec spec;
    spec.window_type   = WindowType::TUMBLING;
    spec.window_size_ms = effective_window;
    spec.func          = AggregateFunc::SUM;
    spec.measure_field = measure_field;

    auto windows = aggregate(table, spec, from, to);
    if (windows.empty()) {
        return trend;
    }

    // Build (x, y) vectors: x = window centre relative to period_start (ms),
    // y = aggregated value.
    std::vector<double> xs;
    std::vector<double> ys = {};

    xs.reserve(windows.size());
    ys.reserve(windows.size());

    for (const auto& w : windows) {
        if (w.record_count == 0) {
            continue;
        }
        double centre = static_cast<double>(w.window_start + w.window_end) / 2.0
                        - static_cast<double>(from);
        xs.push_back(centre);
        ys.push_back(w.value);
    }

    trend.sample_count = xs.size();
    if (xs.size() < 2) {
        if (!ys.empty()) {
            trend.intercept = ys[0];
        }
        return trend;
    }

    auto [slope, intercept, r2] = computeLinearRegression(xs, ys);
    trend.slope     = slope;
    trend.intercept = intercept;
    trend.r_squared = r2;

    return trend;
}

// ============================================================================
// Private helpers
// ============================================================================

std::optional<double> TemporalAggregator::extractMeasure(
    const Document& doc, const std::string& field) {

    if (field.empty()) {
        return std::nullopt;
    }
    auto it = doc.find(field);
    if (it == doc.end()) {
        return std::nullopt;
    }
    if (it->is_number()) {
        return it->get<double>();
    }
    return std::nullopt;
}

double TemporalAggregator::applyFunc(AggregateFunc func,
                                      const std::vector<double>& values,
                                      size_t count) {
    if (count == 0) {
        return 0.0;
    }
    switch (func) {
        case AggregateFunc::COUNT:
            return static_cast<double>(count);
        case AggregateFunc::SUM:
            return std::accumulate(values.begin(), values.end(), 0.0);
        case AggregateFunc::AVG:
            if (values.empty()) {
              return 0.0;
            }
            return std::accumulate(values.begin(), values.end(), 0.0) /
                   static_cast<double>(values.size());
        case AggregateFunc::MIN:
            if (values.empty()) {
              return 0.0;
            }
            return *std::min_element(values.begin(), values.end());
        case AggregateFunc::MAX:
            if (values.empty()) {
              return 0.0;
            }
            return *std::max_element(values.begin(), values.end());
        // FIRST_VALUE / LAST_VALUE: caller must supply values sorted ascending
        // by sys_start.  applyFunc sees them as a sorted sequence.
        case AggregateFunc::FIRST_VALUE:
            return values.empty() ? 0.0 : values.front();
        case AggregateFunc::LAST_VALUE:
            return values.empty() ? 0.0 : values.back();
    }
    return 0.0;
}

// ── TUMBLING ─────────────────────────────────────────────────────────────────

std::vector<AggregateResult> TemporalAggregator::computeTumbling(
    const std::vector<VersionedDocument>& rows,
    const AggregationSpec& spec,
    Timestamp from,
    Timestamp to) {

    if (spec.window_size_ms <= 0) {
        return {};
    }

    std::vector<AggregateResult> results;

    for (Timestamp win_start = from; win_start < to;
         win_start += spec.window_size_ms) {

        Timestamp win_end = win_start + spec.window_size_ms;
        if (win_end > to) {
            win_end = to;
        }

        std::vector<double> values;
        std::vector<std::tuple<Timestamp, Timestamp, double>> ordered_vals; // for FIRST/LAST_VALUE
        size_t count = 0;
        const bool need_order = (spec.func == AggregateFunc::FIRST_VALUE ||
                                  spec.func == AggregateFunc::LAST_VALUE);

        for (const auto& row : rows) {
            Timestamp event_ts = row.sys_time.start;
            if (event_ts >= win_start && event_ts < win_end) {
                ++count;
                auto val = extractMeasure(row.data, spec.measure_field);
                if (val.has_value() &&
                    spec.func != AggregateFunc::COUNT) {
                    if (need_order) {
                        ordered_vals.emplace_back(event_ts, row.sys_time.end, *val);
                    } else {
                        values.push_back(*val);
                    }
                }
            }
        }

        if (need_order && !ordered_vals.empty()) {
            std::sort(ordered_vals.begin(), ordered_vals.end());
            for (auto& v : ordered_vals) {
              values.push_back(std::get<2>(v));
            }
        }

        if (spec.func == AggregateFunc::COUNT || count > 0) {
            AggregateResult res;
            res.window_start  = win_start;
            res.window_end    = win_end;
            res.record_count  = count;
            res.value         = applyFunc(spec.func, values, count);
            results.push_back(res);
        }
    }
    return results;
}

// ── SLIDING ───────────────────────────────────────────────────────────────────

std::vector<AggregateResult> TemporalAggregator::computeSliding(
    const std::vector<VersionedDocument>& rows,
    const AggregationSpec& spec,
    Timestamp from,
    Timestamp to) {

    if (spec.window_size_ms <= 0) {
        return {};
    }

    int64_t slide = (spec.slide_interval_ms > 0)
                        ? spec.slide_interval_ms
                        : spec.window_size_ms;

    std::vector<AggregateResult> results;

    for (Timestamp win_start = from; win_start < to; win_start += slide) {

        Timestamp win_end = win_start + spec.window_size_ms;
        // Do not clamp to 'to' for SLIDING so that windows fully extend
        // beyond the right boundary (consistent with standard semantics).
        // Rows beyond 'to' are simply not in the input anyway.

        std::vector<double> values;
        std::vector<std::tuple<Timestamp, Timestamp, double>> ordered_vals;
        size_t count = 0;
        const bool need_order_s = (spec.func == AggregateFunc::FIRST_VALUE ||
                                    spec.func == AggregateFunc::LAST_VALUE);

        for (const auto& row : rows) {
            Timestamp event_ts = row.sys_time.start;
            if (event_ts >= win_start && event_ts < win_end) {
                ++count;
                auto val = extractMeasure(row.data, spec.measure_field);
                if (val.has_value() && spec.func != AggregateFunc::COUNT) {
                    if (need_order_s) {
                        ordered_vals.emplace_back(event_ts, row.sys_time.end, *val);
                    } else {
                        values.push_back(*val);
                    }
                }
            }
        }

        if (need_order_s && !ordered_vals.empty()) {
            std::sort(ordered_vals.begin(), ordered_vals.end());
            for (auto& v : ordered_vals) {
              values.push_back(std::get<2>(v));
            }
        }

        if (spec.func == AggregateFunc::COUNT || count > 0) {
            AggregateResult res;
            res.window_start  = win_start;
            res.window_end    = win_end;
            res.record_count  = count;
            res.value         = applyFunc(spec.func, values, count);
            results.push_back(res);
        }
    }
    return results;
}

// ── SESSION ───────────────────────────────────────────────────────────────────

std::vector<AggregateResult> TemporalAggregator::computeSession(
    const std::vector<VersionedDocument>& rows,
    const AggregationSpec& spec,
    Timestamp from,
    Timestamp to) {

    if (rows.empty()) {
        return {};
    }

    std::vector<AggregateResult> results;
    const bool need_order_sess = (spec.func == AggregateFunc::FIRST_VALUE ||
                                   spec.func == AggregateFunc::LAST_VALUE);

    Timestamp session_start = kMinTimestamp - 1; // kInvalidTimestamp sentinel
    Timestamp session_end   = kMinTimestamp - 1;
    Timestamp last_event    = kMinTimestamp - 1;

    auto isSet = [](Timestamp t) { return t != kMinTimestamp - 1; };

    std::vector<double> cur_values;
    std::vector<std::tuple<Timestamp, Timestamp, double>> cur_ordered;
    size_t              cur_count = 0;

    auto flushSession = [&]() {
        if (cur_count == 0 && spec.func != AggregateFunc::COUNT) {
            return;
        }
        if (!isSet(session_start)) {
            return;
        }
        if (need_order_sess && !cur_ordered.empty()) {
            std::sort(cur_ordered.begin(), cur_ordered.end());
            cur_values.clear();
            for (auto& v : cur_ordered) {
              cur_values.push_back(std::get<2>(v));
            }
            cur_ordered.clear();
        }
        AggregateResult res;
        res.window_start  = session_start;
        res.window_end    = session_end + 1; // exclusive end
        res.record_count  = cur_count;
        res.value         = applyFunc(spec.func, cur_values, cur_count);
        results.push_back(res);
        cur_values.clear();
        cur_count     = 0;
        session_start = kMinTimestamp - 1;
        session_end   = kMinTimestamp - 1;
        last_event    = kMinTimestamp - 1;
    };

    for (const auto& row : rows) {
        Timestamp event_ts = row.sys_time.start;
        if (event_ts < from || event_ts >= to) {
            continue;
        }

        if (isSet(last_event) &&
            (event_ts - last_event) >= spec.gap_duration_ms) {
            flushSession();
        }

        if (!isSet(session_start)) {
            session_start = event_ts;
        }
        session_end = event_ts;
        last_event  = event_ts;
        ++cur_count;

        auto val = extractMeasure(row.data, spec.measure_field);
        if (val.has_value() && spec.func != AggregateFunc::COUNT) {
            if (need_order_sess) {
                cur_ordered.emplace_back(event_ts, row.sys_time.end, *val);
            } else {
                cur_values.push_back(*val);
            }
        }
    }

    flushSession();
    return results;
}

// ── buildGroupKey ─────────────────────────────────────────────────────────────

std::pair<std::string, std::map<std::string, std::string>>
TemporalAggregator::buildGroupKey(const Document& doc,
                                   const std::vector<std::string>& fields) {
    if (fields.empty()) {
        return {"", {}};
    }

    // Use a JSON object as the canonical group key.  nlohmann::json::dump()
    // with default settings applies UTF-8 escaping, so field names or values
    // containing '|', '=', '"' or any other special character are
    // unambiguous and can never collide with a different set of group values.
    // Keys in the JSON object are sorted by nlohmann's map ordering so the
    // output is deterministic regardless of iteration order.
    Document json_key = Document::object();
    std::map<std::string, std::string> kv;

    for (const auto& field : fields) {
        auto it = doc.find(field);
        if (it != doc.end()) {
            json_key[field] = *it;
            kv[field] = it->is_string() ? it->get<std::string>() : it->dump();
        } else {
            json_key[field] = nullptr;
            kv[field] = "";
        }
    }

    return {json_key.dump(), std::move(kv)};
}

// ── computeLinearRegression ───────────────────────────────────────────────────

std::tuple<double, double, double>
TemporalAggregator::computeLinearRegression(const std::vector<double>& x,
                                             const std::vector<double>& y) {
    const size_t n = x.size();
    if (n < 2) {
        return {0.0, (n == 1 ? y[0] : 0.0), 0.0};
    }

    double sum_x  = 0.0;
    double sum_y  = 0.0;
    double sum_xx = 0.0;
    double sum_xy = 0.0;

    for (size_t i = 0; i < n; ++i) {
        sum_x  += x[i];
        sum_y  += y[i];
        sum_xx += x[i] * x[i];
        sum_xy += x[i] * y[i];
    }

    const double dn = static_cast<double>(n);
    const double sum_x2_over_n = (sum_x * sum_x) / dn;
    const double denom = sum_xx - sum_x2_over_n;

    double slope     = 0.0;
    double intercept = sum_y / dn;

    // Use a relative epsilon to guard against near-singular cases where
    // denom is non-zero but so small that the computed slope would be
    // numerically meaningless (e.g. when all x values are nearly equal).
    const double eps = std::numeric_limits<double>::epsilon() *
                       std::max({sum_xx, std::abs(sum_x2_over_n), 1.0});
    if (std::abs(denom) > eps) {
        slope     = (sum_xy - (sum_x * sum_y) / dn) / denom;
        intercept = (sum_y - slope * sum_x) / dn;
    }

    // Compute r²
    const double y_mean = sum_y / dn;
    double ss_tot = 0.0;
    double ss_res = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double fitted = slope * x[i] + intercept;
        ss_res += (y[i] - fitted) * (y[i] - fitted);
        ss_tot += (y[i] - y_mean) * (y[i] - y_mean);
    }

    const double r2 = (ss_tot > 0.0) ? (1.0 - ss_res / ss_tot) : 1.0;

    return {slope, intercept, r2};
}

} // namespace temporal
} // namespace themisdb
