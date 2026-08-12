/**
 * @file compression_selector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "timeseries/tsstore.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

// ============================================================================
// Compression strategy enumeration
// ============================================================================

/**
 * @brief Compression strategy that can be applied to a time-series chunk.
 *
 * - Gorilla : XOR delta-of-delta float encoding; best for floating-point
 *             metrics with continuous variation.
 * - DeltaOfDelta : Integer-only DoD encoding; best for monotonically
 *                  increasing integer counters (e.g. event sequences).
 * - RLE    : Run-Length Encoding; best for step-function or constant-value
 *             series (e.g. binary state sensors).
 * - None   : No compression; raw JSON storage (debugging / tiny series).
 */
enum class CompressionStrategy { Gorilla, DeltaOfDelta, RLE, None };

// ============================================================================
// SeriesProfile
// ============================================================================

/**
 * @brief Statistical profile of a time-series sample used to drive
 *        compression strategy selection.
 */
struct SeriesProfile {
    size_t  sample_count{0};

    /// Sample variance of the value sequence.  High variance favours Gorilla.
    double  value_variance{0.0};

    /// Fraction of adjacent pairs where both timestamps differ by the same
    /// delta (0 = fully irregular, 1 = perfectly regular).
    double  timestamp_regularity{0.0};

    /// Fraction of adjacent value pairs that are identical (runs). Values
    /// close to 1 indicate RLE will compress well.
    double  run_length_ratio{0.0};

    /// Mean absolute delta-of-delta for timestamps (ms).  Small values
    /// indicate a near-constant inter-sample interval → DeltaOfDelta wins.
    double  dod_mean_abs{0.0};
};

/**
 * @brief Compute a SeriesProfile from a vector of DataPoints.
 *
 * @param points  Input data points (need not be sorted, but duplicate
 *                timestamps are counted as identical value runs).
 * @return        Computed profile.  Returns a zero-initialised profile when
 *                @p points is empty or contains a single element.
 */
SeriesProfile profileSeries(const std::vector<TSStore::DataPoint>& points);

// ============================================================================
// ICompressionSelector
// ============================================================================

/**
 * @brief Strategy interface for adaptive compression selection.
 *
 * Implementations receive a SeriesProfile (or raw DataPoints) and return
 * the CompressionStrategy that is expected to yield the best trade-off
 * between compression ratio and CPU cost for that series.
 */
class ICompressionSelector {
public:
    virtual ~ICompressionSelector() = default;

    /**
     * @brief Select a compression strategy from a pre-computed profile.
     */
    virtual CompressionStrategy select(const SeriesProfile& profile) const = 0;

    /**
     * @brief Convenience overload: profile the points internally and select.
     */
    virtual CompressionStrategy selectForPoints(
        const std::vector<TSStore::DataPoint>& points) const = 0;
};

// ============================================================================
// HeuristicCompressionSelector
// ============================================================================

/**
 * @brief Rule-based heuristic compression selector.
 *
 * Decision logic (evaluated in order):
 *  1. sample_count < min_samples  → None (too little data to compress)
 *  2. run_length_ratio > rle_run_ratio_threshold  → RLE
 *  3. dod_mean_abs ≤ dod_mean_abs_threshold AND timestamp_regularity ≥
 *     regularity_threshold  → DeltaOfDelta
 *  4. default  → Gorilla
 */
class HeuristicCompressionSelector : public ICompressionSelector {
public:
    struct Config {
        /// Minimum number of samples required to enable compression.
        size_t min_samples{4};

        /// Fraction of identical adjacent values above which RLE is preferred.
        double rle_run_ratio_threshold{0.70};

        /// Mean absolute DoD (ms) below which DeltaOfDelta is preferred.
        double dod_mean_abs_threshold{1.0};

        /// Timestamp regularity score (0–1) required to prefer DeltaOfDelta.
        double regularity_threshold{0.90};
    };

    HeuristicCompressionSelector();
    explicit HeuristicCompressionSelector(Config cfg);

    CompressionStrategy select(const SeriesProfile& profile) const override;

    CompressionStrategy selectForPoints(
        const std::vector<TSStore::DataPoint>& points) const override;

    const Config& config() const { return config_; }

private:
    Config config_;
};

// ============================================================================
// PerSeriesCompressionRegistry
// ============================================================================

/**
 * @brief Per-series compression strategy registry.
 *
 * Maintains a two-level lookup:
 *  - Pinned entries: manually-forced strategies that override selection.
 *  - Cached entries: strategy returned by the selector on the last call to
 *    strategyFor(); cached values are evicted when clearCache() is called.
 *
 * The registry owns an ICompressionSelector that can be replaced at runtime
 * via setSelector().  The default selector is HeuristicCompressionSelector
 * with default configuration.
 *
 * Thread safety: NOT thread-safe.  External synchronisation is required when
 * called from multiple threads.
 */
class PerSeriesCompressionRegistry {
public:
    using SeriesKey = std::string;  ///< "metric:entity"

    PerSeriesCompressionRegistry();
    explicit PerSeriesCompressionRegistry(
        std::unique_ptr<ICompressionSelector> selector);

    /**
     * @brief Replace the internal selector.
     *
     * Clears the strategy cache so that subsequent strategyFor() calls use
     * the new selector.  Pinned entries are not affected.
     */
    void setSelector(std::unique_ptr<ICompressionSelector> selector);

    /**
     * @brief Return the compression strategy for a series.
     *
     * Priority: pinned > cached > freshly-selected (result is then cached).
     *
     * @param metric  Metric name.
     * @param entity  Entity identifier.
     * @param sample  Representative data points used to profile the series
     *                when no pinned or cached entry exists.  May be empty;
     *                in that case the selector receives an empty sample and
     *                will typically return None.
     */
    CompressionStrategy strategyFor(const std::string& metric,
                                    const std::string& entity,
                                    const std::vector<TSStore::DataPoint>& sample);

    /**
     * @brief Force a specific strategy for a series, overriding selection.
     */
    void pinStrategy(const std::string& metric,
                     const std::string& entity,
                     CompressionStrategy strategy);

    /**
     * @brief Remove a previously pinned strategy.
     *
     * After removal the next strategyFor() call will re-run the selector.
     */
    void clearPin(const std::string& metric, const std::string& entity);

    /**
     * @brief Discard all cached (non-pinned) selections.
     */
    void clearCache();

    /**
     * @brief Total number of entries (pinned + cached).
     */
    size_t registrySize() const;

    /**
     * @brief Remove all pinned and cached entries.
     */
    void clear();

private:
    std::unique_ptr<ICompressionSelector> selector_;
    std::unordered_map<SeriesKey, CompressionStrategy> pinned_;
    std::unordered_map<SeriesKey, CompressionStrategy> cached_;

    static SeriesKey makeKey(const std::string& metric, const std::string& entity);
};

} // namespace themis
