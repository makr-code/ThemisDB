/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_aggregator.h                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Aggregator
 *
 * Window-based aggregation over SystemVersionedTable data:
 *   - TUMBLING  : non-overlapping fixed-size windows
 *   - SLIDING   : overlapping windows advancing by a slide interval
 *   - SESSION   : gap-based windows (closed when data is silent ≥ gap_duration)
 *
 * Supported aggregation functions: SUM, AVG, COUNT, MIN, MAX
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include <functional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * Supported window types.
 */
enum class WindowType {
    TUMBLING, ///< Non-overlapping fixed-size windows
    SLIDING,  ///< Overlapping windows with a configurable slide interval
    SESSION   ///< Gap-based windows: a new window opens when the gap between
              ///< consecutive events exceeds gap_duration_ms
};

/**
 * Supported aggregation functions.
 */
enum class AggregateFunc {
    SUM,
    AVG,
    COUNT,
    MIN,
    MAX
};

/**
 * Specification for a windowed aggregation.
 */
struct AggregationSpec {
    WindowType   window_type{WindowType::TUMBLING};
    int64_t      window_size_ms{60'000};   ///< Window width in milliseconds
    int64_t      slide_interval_ms{0};     ///< For SLIDING: advance step (0 → same as window_size)
    int64_t      gap_duration_ms{30'000};  ///< For SESSION: inactivity gap
    AggregateFunc func{AggregateFunc::COUNT};
    std::string  measure_field;            ///< JSON field name for numeric measures
                                           ///< (ignored for COUNT)
};

/**
 * A single aggregated result for one window.
 */
struct AggregateResult {
    Timestamp window_start{0};
    Timestamp window_end{0};
    double    value{0.0};
    size_t    record_count{0};

    nlohmann::json toJson() const {
        return {{"window_start", window_start},
                {"window_end",   window_end},
                {"value",        value},
                {"record_count", record_count}};
    }
};

/**
 * TemporalAggregator
 *
 * Computes windowed aggregations over the system-time (sys_start) of rows
 * in a SystemVersionedTable.  Only rows that were current at any point
 * within the query range are included.
 *
 * Thread-safety: const methods; the caller is responsible for not mutating
 *               the table concurrently during an aggregation call.
 */
class TemporalAggregator {
public:
    TemporalAggregator() = default;

    /**
     * Run a windowed aggregation.
     *
     * @param table  Source table (all current + historical rows are scanned).
     * @param spec   Window and aggregation specification.
     * @param from   Query range start (inclusive, ms epoch).
     * @param to     Query range end   (exclusive,  ms epoch).
     * @return       One AggregateResult per window, ordered by window_start.
     */
    std::vector<AggregateResult> aggregate(const SystemVersionedTable& table,
                                           const AggregationSpec& spec,
                                           Timestamp from,
                                           Timestamp to) const;

    /**
     * Convenience overload that uses the full available time range.
     */
    std::vector<AggregateResult> aggregate(const SystemVersionedTable& table,
                                           const AggregationSpec& spec) const;

private:
    // Collect the numeric value of measure_field from a document.
    // Returns std::nullopt when the field is absent or not numeric.
    static std::optional<double> extractMeasure(const Document& doc,
                                                const std::string& field);

    // Assign each row to its containing windows, then compute the aggregate.
    static std::vector<AggregateResult> computeTumbling(
        const std::vector<VersionedDocument>& rows,
        const AggregationSpec& spec,
        Timestamp from, Timestamp to);

    static std::vector<AggregateResult> computeSliding(
        const std::vector<VersionedDocument>& rows,
        const AggregationSpec& spec,
        Timestamp from, Timestamp to);

    static std::vector<AggregateResult> computeSession(
        const std::vector<VersionedDocument>& rows,
        const AggregationSpec& spec,
        Timestamp from, Timestamp to);

    // Compute the aggregate value from a set of (value, present) pairs.
    static double applyFunc(AggregateFunc func,
                            const std::vector<double>& values,
                            size_t count);
};

} // namespace temporal
} // namespace themisdb
