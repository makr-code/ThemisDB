/**
 * @file gap_fill.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/gap_fill.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace themis {

// ============================================================================
// Helpers
// ============================================================================

namespace {

// Find the last data point whose timestamp_ms <= target_ts.
// Returns points.end() if no such point exists.
std::vector<TSStore::DataPoint>::const_iterator
findPrecedingOrEqual(const std::vector<TSStore::DataPoint>& points,
                     int64_t target_ts) {
    // Use upper_bound to find first point > target_ts, then step back.
    auto it = std::upper_bound(
        points.begin(), points.end(), target_ts,
        [](int64_t ts, const TSStore::DataPoint& dp) {
            return ts < dp.timestamp_ms;
        });
    if (it == points.begin()) return points.end(); // nothing before target
    --it;
    return it;
}

// Find the first data point whose timestamp_ms >= target_ts.
// Returns points.end() if no such point exists.
std::vector<TSStore::DataPoint>::const_iterator
findFollowingOrEqual(const std::vector<TSStore::DataPoint>& points,
                     int64_t target_ts) {
    return std::lower_bound(
        points.begin(), points.end(), target_ts,
        [](const TSStore::DataPoint& dp, int64_t ts) {
            return dp.timestamp_ms < ts;
        });
}

// Build a synthetic point by copying context fields from a donor.
TSStore::DataPoint makeSynthetic(const TSStore::DataPoint& donor,
                                  int64_t timestamp_ms,
                                  double  value) {
    TSStore::DataPoint out;
    out.metric       = donor.metric;
    out.entity       = donor.entity;
    out.timestamp_ms = timestamp_ms;
    out.value        = value;
    out.tags         = donor.tags;
    // metadata is intentionally left empty for synthetic points
    return out;
}

// Build a placeholder point using a constant value when no donor is available.
TSStore::DataPoint makePlaceholder(int64_t timestamp_ms, double fill_value) {
    TSStore::DataPoint out;
    out.timestamp_ms = timestamp_ms;
    out.value        = fill_value;
    return out;
}

} // anonymous namespace

// ============================================================================
// ForwardFillGapFiller
// ============================================================================

std::vector<TSStore::DataPoint> ForwardFillGapFiller::fill(
    const std::vector<TSStore::DataPoint>& points,
    const std::vector<int64_t>&            timestamps_to_fill,
    const GapFillConfig&                   cfg) const {

    std::vector<TSStore::DataPoint> result;
    result.reserve(timestamps_to_fill.size());

    for (int64_t target_ts : timestamps_to_fill) {
        // Exact match?
        auto exact = findFollowingOrEqual(points, target_ts);
        if (exact != points.end() && exact->timestamp_ms == target_ts) {
            result.push_back(*exact);
            continue;
        }

        // Preceding observation
        auto prev = findPrecedingOrEqual(points, target_ts);
        if (prev == points.end()) {
            // No preceding point: bootstrap from the first available point
            if (points.empty()) {
                result.push_back(makePlaceholder(target_ts, cfg.null_fill_value));
            } else {
                // gap from start-of-series
                int64_t gap = target_ts - points.front().timestamp_ms;
                if (cfg.max_gap_ms > 0 && gap > cfg.max_gap_ms) {
                    result.push_back(
                        makeSynthetic(points.front(), target_ts, cfg.null_fill_value));
                } else {
                    result.push_back(
                        makeSynthetic(points.front(), target_ts, points.front().value));
                }
            }
            continue;
        }

        // Check gap constraint
        int64_t gap = target_ts - prev->timestamp_ms;
        if (cfg.max_gap_ms > 0 && gap > cfg.max_gap_ms) {
            result.push_back(makeSynthetic(*prev, target_ts, cfg.null_fill_value));
        } else {
            result.push_back(makeSynthetic(*prev, target_ts, prev->value));
        }
    }

    return result;
}

// ============================================================================
// LinearInterpolationGapFiller
// ============================================================================

std::vector<TSStore::DataPoint> LinearInterpolationGapFiller::fill(
    const std::vector<TSStore::DataPoint>& points,
    const std::vector<int64_t>&            timestamps_to_fill,
    const GapFillConfig&                   cfg) const {

    std::vector<TSStore::DataPoint> result;
    result.reserve(timestamps_to_fill.size());

    for (int64_t target_ts : timestamps_to_fill) {
        // Exact match?
        auto exact = findFollowingOrEqual(points, target_ts);
        if (exact != points.end() && exact->timestamp_ms == target_ts) {
            result.push_back(*exact);
            continue;
        }

        if (points.empty()) {
            result.push_back(makePlaceholder(target_ts, cfg.null_fill_value));
            continue;
        }

        auto next_it = findFollowingOrEqual(points, target_ts);
        auto prev_it = (next_it == points.begin())
                           ? points.end()
                           : std::prev(next_it);

        if (prev_it == points.end()) {
            // Before first point — carry first value forward
            int64_t gap = target_ts - points.front().timestamp_ms;
            double val = (cfg.max_gap_ms > 0 && gap > cfg.max_gap_ms)
                             ? cfg.null_fill_value
                             : points.front().value;
            result.push_back(makeSynthetic(points.front(), target_ts, val));
            continue;
        }

        if (next_it == points.end()) {
            // After last point — carry last value forward
            int64_t gap = target_ts - points.back().timestamp_ms;
            double val = (cfg.max_gap_ms > 0 && gap > cfg.max_gap_ms)
                             ? cfg.null_fill_value
                             : points.back().value;
            result.push_back(makeSynthetic(points.back(), target_ts, val));
            continue;
        }

        // Interpolate between prev and next
        int64_t t0 = prev_it->timestamp_ms;
        int64_t t1 = next_it->timestamp_ms;
        double  v0 = prev_it->value;
        double  v1 = next_it->value;

        // Gap check uses the gap to the preceding observation
        double interp_val;
        if (cfg.max_gap_ms > 0 && (t1 - t0) > cfg.max_gap_ms) {
            interp_val = cfg.null_fill_value;
        } else if (t1 == t0) {
            interp_val = v0;
        } else {
            double frac = static_cast<double>(target_ts - t0) /
                          static_cast<double>(t1 - t0);
            interp_val = v0 + frac * (v1 - v0);
        }

        result.push_back(makeSynthetic(*prev_it, target_ts, interp_val));
    }

    return result;
}

// ============================================================================
// BackwardFillGapFiller
// ============================================================================

std::vector<TSStore::DataPoint> BackwardFillGapFiller::fill(
    const std::vector<TSStore::DataPoint>& points,
    const std::vector<int64_t>&            timestamps_to_fill,
    const GapFillConfig&                   cfg) const {

    std::vector<TSStore::DataPoint> result;
    result.reserve(timestamps_to_fill.size());

    for (int64_t target_ts : timestamps_to_fill) {
        // Exact match?
        auto exact = findFollowingOrEqual(points, target_ts);
        if (exact != points.end() && exact->timestamp_ms == target_ts) {
            result.push_back(*exact);
            continue;
        }

        auto next_it = findFollowingOrEqual(points, target_ts);

        if (next_it == points.end()) {
            // No following point: fall back to last available
            if (points.empty()) {
                result.push_back(makePlaceholder(target_ts, cfg.null_fill_value));
            } else {
                int64_t gap = target_ts - points.back().timestamp_ms;
                double val = (cfg.max_gap_ms > 0 && gap > cfg.max_gap_ms)
                                 ? cfg.null_fill_value
                                 : points.back().value;
                result.push_back(makeSynthetic(points.back(), target_ts, val));
            }
            continue;
        }

        int64_t gap = next_it->timestamp_ms - target_ts;
        if (cfg.max_gap_ms > 0 && gap > cfg.max_gap_ms) {
            result.push_back(makeSynthetic(*next_it, target_ts, cfg.null_fill_value));
        } else {
            result.push_back(makeSynthetic(*next_it, target_ts, next_it->value));
        }
    }

    return result;
}

// ============================================================================
// GapFiller
// ============================================================================

std::unique_ptr<IGapFiller> GapFiller::makeImpl(GapFillMethod method) {
    switch (method) {
        case GapFillMethod::ForwardFill:
            return std::make_unique<ForwardFillGapFiller>();
        case GapFillMethod::LinearInterpolation:
            return std::make_unique<LinearInterpolationGapFiller>();
        case GapFillMethod::BackwardFill:
            return std::make_unique<BackwardFillGapFiller>();
        case GapFillMethod::NullFill:
            // NullFill is implemented as ForwardFill with max_gap_ms = 1
            // (every gap exceeds 1 ms, so null_fill_value is always used).
            return std::make_unique<ForwardFillGapFiller>();
    }
    return std::make_unique<ForwardFillGapFiller>();
}

GapFiller::GapFiller(GapFillConfig cfg)
    : config_(std::move(cfg))
    , impl_(makeImpl(config_.method)) {}

void GapFiller::setConfig(const GapFillConfig& cfg) {
    config_ = cfg;
    impl_   = makeImpl(config_.method);
}

std::vector<TSStore::DataPoint> GapFiller::fill(
    const std::vector<TSStore::DataPoint>& points,
    const std::vector<int64_t>&            timestamps_to_fill) const {

    // NullFill: force max_gap_ms = 1 so every missing timestamp gets null
    if (config_.method == GapFillMethod::NullFill) {
        GapFillConfig null_cfg = config_;
        null_cfg.max_gap_ms = 1;
        return impl_->fill(points, timestamps_to_fill, null_cfg);
    }

    return impl_->fill(points, timestamps_to_fill, config_);
}

std::vector<int64_t> GapFiller::regularTimestamps(int64_t from_ms,
                                                    int64_t to_ms,
                                                    int64_t interval_ms) {
    if (interval_ms <= 0 || from_ms > to_ms) return {};

    std::vector<int64_t> ts;
    ts.reserve(static_cast<size_t>((to_ms - from_ms) / interval_ms) + 2);
    for (int64_t t = from_ms; t <= to_ms; t += interval_ms) {
        ts.push_back(t);
    }
    return ts;
}

} // namespace themis
