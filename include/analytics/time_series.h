/**
 * @file time_series.h
 * @brief Time-series data access with iterator-safe user-offset navigation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
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

using TimePoint = std::int64_t;

struct DataPoint {
    TimePoint timestamp; ///< Nanoseconds since epoch.
    double    value;     ///< Observed value.

    bool operator<(const DataPoint& rhs) const noexcept {
        return timestamp < rhs.timestamp;
    }
};

// ---------------------------------------------------------------------------
// TimeWindow
// ---------------------------------------------------------------------------

struct TimeWindow {
    TimePoint begin_ts; ///< Window start (inclusive).
    TimePoint end_ts;   ///< Window end (exclusive).

    [[nodiscard]] bool contains(TimePoint ts) const noexcept {
        return ts >= begin_ts && ts < end_ts;
    }
};

// ---------------------------------------------------------------------------
// TimeSeriesStats
// ---------------------------------------------------------------------------

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

class TimeSeries {
public:
    /**
     * @brief Time Series.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    explicit TimeSeries(std::string name);

    ~TimeSeries() = default;

    // Non-copyable (potentially large data); movable.
    TimeSeries(const TimeSeries&)            = delete;
    TimeSeries& operator=(const TimeSeries&) = delete;
    TimeSeries(TimeSeries&&)                 noexcept = default;
    TimeSeries& operator=(TimeSeries&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Write
    // -----------------------------------------------------------------------

    /**
     * @brief Append.
     * @param[in] ts Input parameter.
     * @param[in] value Input parameter.
     */
    void append(TimePoint ts, double value);

    /**
     * @brief Append batch.
     * @param[in] points Input parameter.
     */
    void append_batch(const std::vector<DataPoint>& points);

    // -----------------------------------------------------------------------
    // Read
    // -----------------------------------------------------------------------

    [[nodiscard]] std::vector<DataPoint> query_window(const TimeWindow& window) const;

    [[nodiscard]] std::vector<DataPoint> page(std::size_t offset,
                                              std::size_t limit) const;

    [[nodiscard]] std::optional<TimeSeriesStats> stats(
        const TimeWindow& window) const;

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] const std::string& name() const noexcept;

private:
    std::string             name_;
    mutable std::vector<DataPoint>  points_;
    mutable bool            sorted_{true};

    /**
     * @brief Ensure sorted.
     */
    void ensure_sorted() const;
};

}  // namespace analytics
}  // namespace themis
