/**
 * @file gap_fill.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "timeseries/tsstore.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {

// ============================================================================
// GapFillMethod / GapFillConfig
// ============================================================================

/**
 * @brief Strategy for synthesising values at timestamps that have no
 *        observed data point.
 *
 * - ForwardFill   : LOCF — carry the last known value forward.
 * - LinearInterp  : Linear interpolation between the nearest preceding and
 *                   following known values.
 * - BackwardFill  : Carry the next known value backward (NOCB).
 * - NullFill      : Insert a point with a configurable constant (default 0).
 */
enum class GapFillMethod {
    ForwardFill,
    LinearInterpolation,
    BackwardFill,
    NullFill
};

struct GapFillConfig {
    GapFillMethod method{GapFillMethod::ForwardFill};

    /// Maximum gap length (ms) that will be filled.
    /// A value of 0 means no limit — all gaps are filled regardless of size.
    int64_t max_gap_ms{0};

    /// Constant value inserted for NullFill and for gaps that exceed
    /// max_gap_ms when the caller still wants a placeholder.
    double null_fill_value{0.0};
};

// ============================================================================
// IGapFiller
// ============================================================================

/**
 * @brief Interface for gap-filling implementations.
 *
 * Implementations receive a sorted set of known DataPoints and a sorted list
 * of target timestamps, and must return one DataPoint per target timestamp —
 * either the original observation (if one exists at that time) or a
 * synthesised value.
 *
 * The metric, entity, and tags fields of synthesised points are copied from
 * the nearest neighbour used for the fill value.
 */
class IGapFiller {
public:
    virtual ~IGapFiller() = default;

    /**
     * @brief Fill gaps for the requested timestamps.
     *
     * @param points             Known data points (must be sorted ascending
     *                           by timestamp_ms).
     * @param timestamps_to_fill Timestamps for which a value is required
     *                           (must be sorted ascending).
     * @param cfg                Gap-fill configuration.
     * @return                   One DataPoint per requested timestamp,
     *                           in ascending timestamp order.  Timestamps
     *                           present in @p points are returned verbatim;
     *                           missing timestamps receive a synthesised value.
     */
    virtual std::vector<TSStore::DataPoint> fill(
        const std::vector<TSStore::DataPoint>& points,
        const std::vector<int64_t>&            timestamps_to_fill,
        const GapFillConfig&                   cfg) const = 0;
};

// ============================================================================
// ForwardFillGapFiller
// ============================================================================

/**
 * @brief Gap-filler that carries the last known observation forward (LOCF).
 *
 * If no preceding observation exists for a requested timestamp, the earliest
 * available observation is used instead (backward-bootstrapping).  When
 * @p points is empty, synthesised points receive the null_fill_value.
 */
class ForwardFillGapFiller : public IGapFiller {
public:
    std::vector<TSStore::DataPoint> fill(
        const std::vector<TSStore::DataPoint>& points,
        const std::vector<int64_t>&            timestamps_to_fill,
        const GapFillConfig&                   cfg) const override;
};

// ============================================================================
// LinearInterpolationGapFiller
// ============================================================================

/**
 * @brief Gap-filler using piecewise linear interpolation.
 *
 * For a requested timestamp t located between two known observations at
 * t0 and t1:
 *   value(t) = v0 + (v1 − v0) × (t − t0) / (t1 − t0)
 *
 * Edge cases:
 *   - If t < first known timestamp → forward-fill from the first point.
 *   - If t > last known timestamp  → backward-fill from the last point.
 *   - When t0 == t1, v0 is returned directly.
 */
class LinearInterpolationGapFiller : public IGapFiller {
public:
    std::vector<TSStore::DataPoint> fill(
        const std::vector<TSStore::DataPoint>& points,
        const std::vector<int64_t>&            timestamps_to_fill,
        const GapFillConfig&                   cfg) const override;
};

// ============================================================================
// BackwardFillGapFiller
// ============================================================================

/**
 * @brief Gap-filler that carries the next known observation backward (NOCB).
 *
 * If no following observation exists, the last available observation is used.
 * When @p points is empty, synthesised points receive the null_fill_value.
 */
class BackwardFillGapFiller : public IGapFiller {
public:
    std::vector<TSStore::DataPoint> fill(
        const std::vector<TSStore::DataPoint>& points,
        const std::vector<int64_t>&            timestamps_to_fill,
        const GapFillConfig&                   cfg) const override;
};

// ============================================================================
// GapFiller  (façade / main entry-point)
// ============================================================================

/**
 * @brief Configurable façade over the gap-fill strategy implementations.
 *
 * Constructs the appropriate IGapFiller from GapFillConfig::method and
 * exposes a single fill() entry-point.
 *
 * ### Typical usage
 * @code
 *   GapFillConfig cfg;
 *   cfg.method     = GapFillMethod::LinearInterpolation;
 *   cfg.max_gap_ms = 60'000;   // do not fill gaps wider than 1 minute
 *
 *   GapFiller filler(cfg);
 *
 *   auto timestamps = GapFiller::regularTimestamps(from_ms, to_ms, 1000);
 *   auto filled     = filler.fill(known_points, timestamps);
 * @endcode
 */
class GapFiller {
public:
    explicit GapFiller(GapFillConfig cfg = {});

    /**
     * @brief Fill gaps using the stored configuration.
     *
     * @param points             Known data points (sorted ascending).
     * @param timestamps_to_fill Target timestamps (sorted ascending).
     * @return                   One DataPoint per requested timestamp.
     */
    std::vector<TSStore::DataPoint> fill(
        const std::vector<TSStore::DataPoint>& points,
        const std::vector<int64_t>&            timestamps_to_fill) const;

    /**
     * @brief Generate a regular sequence of timestamps within [from_ms, to_ms].
     *
     * @param from_ms     Start timestamp (inclusive).
     * @param to_ms       End timestamp (inclusive).
     * @param interval_ms Step size between consecutive timestamps.
     * @return            Sorted vector of timestamps.
     *                    Returns an empty vector when interval_ms ≤ 0 or
     *                    from_ms > to_ms.
     */
    static std::vector<int64_t> regularTimestamps(int64_t from_ms,
                                                   int64_t to_ms,
                                                   int64_t interval_ms);

    void                setConfig(const GapFillConfig& cfg);
    const GapFillConfig& config() const { return config_; }

private:
    GapFillConfig                    config_;
    std::unique_ptr<IGapFiller>      impl_;

    static std::unique_ptr<IGapFiller> makeImpl(GapFillMethod method);
};

} // namespace themis
