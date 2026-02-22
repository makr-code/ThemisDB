/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_aggregator.cpp                            ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     334                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Aggregator Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_aggregator.h"
#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

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

    // Collect all rows that have sys_start in [from, to)
    // We scan all history (not just current) so that short-lived rows
    // falling fully within the window are captured.
    // Note: only keys with at least one current row are enumerated here;
    // deleted keys are not included (same limitation as RetentionManager).
    std::vector<VersionedDocument> rows;
    {
        auto current_rows = table.scan(kMaxTimestamp);
        for (const auto& cr : current_rows) {
            auto hist = table.getHistoryInRange(cr.key, {from, to});
            for (auto& h : hist) {
                rows.push_back(std::move(h));
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
            if (r.sys_time.start < data_min) data_min = r.sys_time.start;
            if (r.sys_time.start > data_max) data_max = r.sys_time.start;
        }
        if (from == kMinTimestamp) from = data_min;
        if (to   == kMaxTimestamp) to   = data_max + 1; // exclusive end
    }

    // Sort by sys_start ascending (required by session window logic)
    std::sort(rows.begin(), rows.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
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
            if (values.empty()) return 0.0;
            return std::accumulate(values.begin(), values.end(), 0.0) /
                   static_cast<double>(values.size());
        case AggregateFunc::MIN:
            if (values.empty()) return 0.0;
            return *std::min_element(values.begin(), values.end());
        case AggregateFunc::MAX:
            if (values.empty()) return 0.0;
            return *std::max_element(values.begin(), values.end());
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
        size_t count = 0;

        for (const auto& row : rows) {
            Timestamp event_ts = row.sys_time.start;
            if (event_ts >= win_start && event_ts < win_end) {
                ++count;
                auto val = extractMeasure(row.data, spec.measure_field);
                if (val.has_value() &&
                    spec.func != AggregateFunc::COUNT) {
                    values.push_back(*val);
                }
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
        size_t count = 0;

        for (const auto& row : rows) {
            Timestamp event_ts = row.sys_time.start;
            if (event_ts >= win_start && event_ts < win_end) {
                ++count;
                auto val = extractMeasure(row.data, spec.measure_field);
                if (val.has_value() && spec.func != AggregateFunc::COUNT) {
                    values.push_back(*val);
                }
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

    Timestamp session_start = kMinTimestamp - 1; // kInvalidTimestamp sentinel
    Timestamp session_end   = kMinTimestamp - 1;
    Timestamp last_event    = kMinTimestamp - 1;

    auto isSet = [](Timestamp t) { return t != kMinTimestamp - 1; };

    std::vector<double> cur_values;
    size_t              cur_count = 0;

    auto flushSession = [&]() {
        if (cur_count == 0 && spec.func != AggregateFunc::COUNT) {
            return;
        }
        if (!isSet(session_start)) {
            return;
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
            cur_values.push_back(*val);
        }
    }

    flushSession();
    return results;
}

} // namespace temporal
} // namespace themisdb
