/**
 * @file temporal_aggregator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <map>
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
    MAX,
    FIRST_VALUE, ///< Value of measure_field from the earliest row (by sys_start) in the window
    LAST_VALUE   ///< Value of measure_field from the latest  row (by sys_start) in the window
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
    std::vector<std::string> group_by_fields; ///< JSON field names to group results by
                                              ///< (empty = no grouping)
};

/**
 * A single aggregated result for one window.
 */
struct AggregateResult {
    Timestamp window_start{0};
    Timestamp window_end{0};
    double    value{0.0};
    size_t    record_count{0};
    std::map<std::string, std::string> group_values; ///< Populated when using aggregateByGroup()

    nlohmann::json toJson() const {
        nlohmann::json j = {{"window_start", window_start},
                            {"window_end",   window_end},
                            {"value",        value},
                            {"record_count", record_count}};
        if (!group_values.empty()) {
            j["group_values"] = group_values;
        }
        return j;
    }
};

/**
 * Result of a linear trend analysis over time-windowed aggregates.
 */
struct TrendResult {
    double    slope{0.0};        ///< Rate of change of the measure per millisecond
    double    intercept{0.0};    ///< Fitted value at t = period_start
    double    r_squared{0.0};    ///< Coefficient of determination [0, 1]
    size_t    sample_count{0};   ///< Number of windows used in the regression
    Timestamp period_start{0};   ///< Start of the analysed range
    Timestamp period_end{0};     ///< End of the analysed range

    nlohmann::json toJson() const {
        return {{"slope",        slope},
                {"intercept",    intercept},
                {"r_squared",    r_squared},
                {"sample_count", sample_count},
                {"period_start", period_start},
                {"period_end",   period_end}};
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

    /**
     * Grouped windowed aggregation (temporal GROUP BY).
     *
     * Partitions all in-range rows by the combination of values specified in
     * spec.group_by_fields, then runs the requested window aggregation
     * independently for each group.  The returned map is keyed by a canonical
     * group-key string (field1=value1|field2=value2).  Each AggregateResult
     * in the vectors has its group_values map populated.
     *
     * When spec.group_by_fields is empty the call falls back to the standard
     * aggregate() behaviour and returns a single entry keyed by the empty
     * string (""), with group_values left empty.
     *
     * @param table  Source table.
     * @param spec   Window and aggregation specification, including group_by_fields.
     * @param from   Query range start (inclusive, ms epoch).
     * @param to     Query range end   (exclusive,  ms epoch).
     * @return       Map from group key → ordered window results.
     */
    std::map<std::string, std::vector<AggregateResult>> aggregateByGroup(
        const SystemVersionedTable& table,
        const AggregationSpec& spec,
        Timestamp from,
        Timestamp to) const;

    /** Convenience overload using the full available time range. */
    std::map<std::string, std::vector<AggregateResult>> aggregateByGroup(
        const SystemVersionedTable& table,
        const AggregationSpec& spec) const;

    /**
     * Snapshot aggregation at regular intervals.
     *
     * At each tick `t` in [from, to) advancing by spec.window_size_ms, this
     * method computes the aggregate of all rows that were CURRENT at that
     * exact instant (sys_start ≤ t < sys_end).  Each result window spans
     * [t, t + window_size_ms) and represents the state of the table at the
     * snapshot tick.
     *
     * This is distinct from the standard aggregate() call, which buckets rows
     * by when they were WRITTEN (sys_start), not by when they were VISIBLE.
     *
     * @param table  Source table.
     * @param spec   Specification (window_size_ms used as snapshot interval;
     *               func and measure_field used for the aggregation).
     * @param from   Range start (inclusive, ms epoch).
     * @param to     Range end   (exclusive,  ms epoch).
     * @return       One AggregateResult per snapshot tick, ordered by window_start.
     */
    std::vector<AggregateResult> aggregateSnapshots(
        const SystemVersionedTable& table,
        const AggregationSpec& spec,
        Timestamp from,
        Timestamp to) const;

    /**
     * Linear trend analysis over time-windowed aggregates.
     *
     * The method first partitions [from, to) into tumbling windows of
     * window_size_ms (auto-derived as range/10 when window_size_ms is 0),
     * computes the requested aggregate per window, and then performs an
     * ordinary-least-squares regression of value vs. window centre time.
     *
     * @param table          Source table.
     * @param measure_field  JSON field to aggregate (uses SUM by default).
     * @param from           Range start (inclusive, ms epoch).
     * @param to             Range end   (exclusive,  ms epoch).
     * @param window_size_ms Tumbling window size for bucketing; 0 = auto.
     * @return               TrendResult with slope, intercept, and r².
     */
    TrendResult analyzeTrend(const SystemVersionedTable& table,
                             const std::string& measure_field,
                             Timestamp from,
                             Timestamp to,
                             int64_t window_size_ms = 0) const;

private:
    // Collect the numeric value of measure_field from a document.
    // Returns std::nullopt when the field is absent or not numeric.
    static std::optional<double> extractMeasure(const Document& doc,
                                                const std::string& field);

    // Build a canonical group-key string and a key→value map for a row.
    // Returns {"", {}} when group_by_fields is empty.
    static std::pair<std::string, std::map<std::string, std::string>>
    buildGroupKey(const Document& doc,
                  const std::vector<std::string>& fields);

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

    // Perform ordinary-least-squares regression on (x, y) pairs.
    // Returns {slope, intercept, r_squared}.
    static std::tuple<double, double, double>
    computeLinearRegression(const std::vector<double>& x,
                            const std::vector<double>& y);
};

} // namespace temporal
} // namespace themisdb
