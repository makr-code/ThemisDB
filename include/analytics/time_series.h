/**
 * @file time_series.h
 * @brief Time-series data access with iterator-safe user-offset navigation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416 iterator safety applied (Sprint 7 Batch C, Phase 2C)
 * @note Status: Production Ready
 *
 * Provides sorted, append-only time-series storage with windowed read access.
 * All user-supplied time offsets and window sizes are validated through
 * `themis::security::SafeIterator::AdvanceSafe` before any iterator is moved,
 * addressing gap IDs B011–B015 from the Sprint 7 scan.
 *
 * **CWE Remediations:**
 * - CWE-129 (array index): `AdvanceSafe::advance()` replaces all raw
 *   iterator arithmetic involving user-supplied offsets.
 * - CWE-416 (use-after-free): `BoundsChecker::check_dereference()` guards
 *   every data point read.
 * - `RangeValidator` wraps every sub-range before inner loops.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "security/safe_iterator.h"

namespace themis {
namespace analytics {

// ---------------------------------------------------------------------------
// TimePoint / DataPoint
// ---------------------------------------------------------------------------

/// Nanoseconds since Unix epoch.
using TimePoint = std::int64_t;

/**
 * @brief One time-series observation.
 */
struct DataPoint {
    TimePoint timestamp; ///< Nanoseconds since epoch.
    double    value;     ///< Observed value.

    /**
     * @brief Ordering by timestamp (ascending).
     */
    bool operator<(const DataPoint& rhs) const noexcept {
        return timestamp < rhs.timestamp;
    }
};

// ---------------------------------------------------------------------------
// TimeWindow
// ---------------------------------------------------------------------------

/**
 * @brief A contiguous [begin_ts, end_ts) time window.
 *
 * Both endpoints are in nanoseconds since epoch.  `end_ts` is exclusive.
 */
struct TimeWindow {
    TimePoint begin_ts; ///< Window start (inclusive).
    TimePoint end_ts;   ///< Window end (exclusive).

    /**
     * @brief Check whether a timestamp falls inside the window.
     * @param ts Timestamp to test.
     * @return `true` if `begin_ts <= ts < end_ts`.
     */
    [[nodiscard]] bool contains(TimePoint ts) const noexcept {
        return ts >= begin_ts && ts < end_ts;
    }
};

// ---------------------------------------------------------------------------
// TimeSeriesStats
// ---------------------------------------------------------------------------

/**
 * @brief Descriptive statistics over a set of observations.
 */
struct TimeSeriesStats {
    std::size_t count{0};
    double      sum{0.0};
    double      min{0.0};
    double      max{0.0};
    double      mean{0.0};
    double      variance{0.0};
};

// ---------------------------------------------------------------------------
// TimeSeries
// ---------------------------------------------------------------------------

/**
 * @brief Append-only, sorted time-series store with iterator-safe read access.
 *
 * Internally maintains a sorted vector of `DataPoint` values.  All read
 * operations that accept user-supplied offsets or window boundaries validate
 * those values through `SafeIterator` before any iterator arithmetic.
 *
 * **Thread safety:** not thread-safe; external synchronisation required for
 * concurrent access.
 *
 * **Usage:**
 * ```cpp
 * TimeSeries ts("sensor.temperature");
 * ts.append(now_ns(), 22.5);
 * ts.append(now_ns() + 1000, 23.1);
 *
 * auto window = ts.query_window({start_ns, end_ns});
 * auto page   = ts.page(user_offset, page_size);
 * ```
 */
class TimeSeries {
public:
    /**
     * @brief Construct a named time series.
     * @param name Series identifier (non-empty).
     * @throws std::invalid_argument if `name` is empty.
     */
    explicit TimeSeries(std::string name);

    ~TimeSeries() = default;

    // Non-copyable (potentially large data); movable.
    TimeSeries(const TimeSeries&)            = delete;
    TimeSeries& operator=(const TimeSeries&) = delete;
    TimeSeries(TimeSeries&&)                 = default;
    TimeSeries& operator=(TimeSeries&&)      = default;

    // -----------------------------------------------------------------------
    // Write
    // -----------------------------------------------------------------------

    /**
     * @brief Append a new data point.
     *
     * Points need not be inserted in order; the backing store is re-sorted
     * lazily before the next read access.
     *
     * @param ts    Timestamp in nanoseconds since epoch.
     * @param value Observed value.
     */
    void append(TimePoint ts, double value);

    /**
     * @brief Append multiple data points from a range.
     * @param points Vector of data points to insert.
     */
    void append_batch(const std::vector<DataPoint>& points);

    // -----------------------------------------------------------------------
    // Read
    // -----------------------------------------------------------------------

    /**
     * @brief Return all data points within a time window.
     *
     * Uses `std::lower_bound`/`std::upper_bound` on the sorted store; then
     * copies the resulting sub-range — validated by `RangeValidator` — into
     * the output vector.
     *
     * @param window Time window [begin_ts, end_ts).
     * @return Data points whose timestamps fall within the window, in order.
     */
    [[nodiscard]] std::vector<DataPoint> query_window(const TimeWindow& window) const;

    /**
     * @brief Paginate the series with a user-supplied offset.
     *
     * @param offset First point to return (0-based; validated with `AdvanceSafe`).
     * @param limit  Maximum points to return.
     * @return Sub-vector of data points.
     * @throws std::out_of_range if `offset > size()`.
     *
     * **Iterator safety:** `offset` is validated through
     * `AdvanceSafe::advance()` before any iterator movement.
     */
    [[nodiscard]] std::vector<DataPoint> page(std::size_t offset,
                                              std::size_t limit) const;

    /**
     * @brief Compute statistics over a time window.
     *
     * @param window Time window to aggregate.
     * @return `std::nullopt` if the window contains no data points.
     */
    [[nodiscard]] std::optional<TimeSeriesStats> stats(
        const TimeWindow& window) const;

    /**
     * @brief Number of data points currently stored.
     * @return Point count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Series name as supplied at construction.
     * @return Series identifier.
     */
    [[nodiscard]] const std::string& name() const noexcept;

private:
    std::string             name_;
    mutable std::vector<DataPoint>  points_;
    mutable bool            sorted_{true};

    /// Ensure `points_` is sorted by timestamp (lazy sort).
    void ensure_sorted() const;
};

}  // namespace analytics
}  // namespace themis
