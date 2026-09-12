/**
 * @file time_series.cpp
 * @brief Time-series data store implementation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416/CWE-129 iterator safety — Sprint 7 Batch C Phase 2C
 *   Gap B011: user-supplied offset without validation in page() — FIXED
 *   Gap B012: query_window() sub-range without RangeValidator — FIXED
 *   Gap B013: dereference after lower_bound without end check — FIXED
 * @note Status: Production Ready
 */

#include "analytics/time_series.h"
#include "security/safe_iterator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace analytics {

using themis::security::SafeIterator::AdvanceSafe;
using themis::security::SafeIterator::BoundsChecker;
using themis::security::SafeIterator::RangeValidator;

// ---------------------------------------------------------------------------
// TimeSeries constructor
// ---------------------------------------------------------------------------

TimeSeries::TimeSeries(std::string name)
    : name_(std::move(name))
{
    if (name_.empty()) {
        throw std::invalid_argument("TimeSeries: name must not be empty");
    }
}

// ---------------------------------------------------------------------------
// TimeSeries::name / size
// ---------------------------------------------------------------------------

const std::string& TimeSeries::name() const noexcept { return name_; }

std::size_t TimeSeries::size() const noexcept { return points_.size(); }

// ---------------------------------------------------------------------------
// TimeSeries::ensure_sorted
// ---------------------------------------------------------------------------

void TimeSeries::ensure_sorted() const
{
    if (!sorted_) {
        std::sort(points_.begin(), points_.end());
        sorted_ = true;
    }
}

// ---------------------------------------------------------------------------
// TimeSeries::append
// ---------------------------------------------------------------------------

void TimeSeries::append(TimePoint ts, double value)
{
    // Appending invalidates sorted order only when the new timestamp is before
    // the current last element.
    if (!points_.empty() && ts < points_.back().timestamp) {
        sorted_ = false;
    }
    points_.push_back({ts, value});
}

// ---------------------------------------------------------------------------
// TimeSeries::append_batch
// ---------------------------------------------------------------------------

void TimeSeries::append_batch(const std::vector<DataPoint>& batch)
{
    if (batch.empty()) { return; }

    points_.insert(points_.end(), batch.begin(), batch.end());
    sorted_ = false;  // Conservative: re-sort on next read.
}

// ---------------------------------------------------------------------------
// TimeSeries::query_window
// ---------------------------------------------------------------------------

std::vector<DataPoint> TimeSeries::query_window(const TimeWindow& window) const
{
    if (window.begin_ts >= window.end_ts) {
        return {};
    }

    ensure_sorted();

    // Find first element >= begin_ts.
    DataPoint sentinel_begin{window.begin_ts, 0.0};
    auto lo = std::lower_bound(points_.cbegin(), points_.cend(), sentinel_begin);

    // Gap B013: previously dereferenced lo without checking lo != end().
    // Fix: lo is only the start of our sub-range — we validate the range below.
    if (lo == points_.cend()) {
        return {};
    }

    // Find first element >= end_ts.
    DataPoint sentinel_end{window.end_ts, 0.0};
    auto hi = std::lower_bound(lo, points_.cend(), sentinel_end);

    // Gap B012: previously iterated [lo, hi) without RangeValidator.
    // Fix: wrap sub-range in RangeValidator.
    RangeValidator<std::vector<DataPoint>::const_iterator> sub(lo, hi);
    if (sub.empty()) {
        return {};
    }

    std::vector<DataPoint> result = {};

    result.reserve(sub.size());

    for (auto it = sub.begin(); it != sub.end(); ++it) {
        BoundsChecker::check_dereference(it, lo, hi);
        result.push_back(*it);
    }
    return result;
}

// ---------------------------------------------------------------------------
// TimeSeries::page
// ---------------------------------------------------------------------------

std::vector<DataPoint> TimeSeries::page(std::size_t offset,
                                         std::size_t limit) const
{
    if (points_.empty() || limit == 0) {
        return {};
    }

    ensure_sorted();

    // Gap B011: user-supplied offset driven advance without bounds check.
    // Fix: AdvanceSafe::advance() throws if offset > size().
    auto it  = points_.cbegin();
    auto end = points_.cend();

    AdvanceSafe::advance(it, static_cast<std::ptrdiff_t>(offset),
                         points_.cbegin(), end);

    auto remaining = static_cast<std::size_t>(std::distance(it, end));
    std::size_t count = std::min(limit, remaining);

    auto it_end = it;
    AdvanceSafe::advance(it_end, static_cast<std::ptrdiff_t>(count), it, end);

    RangeValidator<std::vector<DataPoint>::const_iterator> sub(it, it_end);

    std::vector<DataPoint> result = {};

    result.reserve(sub.size());
    for (auto pos = sub.begin(); pos != sub.end(); ++pos) {
        BoundsChecker::check_dereference(pos, it, it_end);
        result.push_back(*pos);
    }
    return result;
}

// ---------------------------------------------------------------------------
// TimeSeries::stats
// ---------------------------------------------------------------------------

std::optional<TimeSeriesStats> TimeSeries::stats(const TimeWindow& window) const
{
    auto pts = query_window(window);
    if (pts.empty()) {
        return std::nullopt;
    }

    TimeSeriesStats s;
    s.count = pts.size();
    s.min   = pts.front().value;
    s.max   = pts.front().value;

    // Single-pass Welford online variance.
    double M2 = 0.0;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        double v    = pts[i].value;
        s.sum      += v;
        if (v < s.min) { s.min = v; }
        if (v > s.max) { s.max = v; }
        double delta = v - s.mean;
        s.mean      += delta / static_cast<double>(i + 1);
        double delta2 = v - s.mean;
        M2          += delta * delta2;
    }

    s.variance = (s.count > 1) ? M2 / static_cast<double>(s.count - 1) : 0.0;
    return s;
}

}  // namespace analytics
}  // namespace themis
