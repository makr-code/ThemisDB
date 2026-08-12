/**
 * @file incremental_agg.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <string>
#include <vector>

namespace themis {
namespace query {

/**
 * @brief Supported incremental aggregate operations.
 */
enum class AggOp { SUM, COUNT, AVG, MIN, MAX };

/**
 * @brief Delta-based incremental aggregator.
 *
 * Maintains a running aggregate over the current window by processing
 * add(value) and remove(value) operations without re-scanning the synopsis.
 *
 * For MIN/MAX the implementation falls back to a full re-scan over the
 * reference deque when the evicted value equals the current extremum.
 *
 * Values are extracted from JSON payloads by a caller-supplied extractor
 * function; this class works over pre-extracted doubles for simplicity.
 */
class IncrementalAgg {
public:
    explicit IncrementalAgg(AggOp op) noexcept;

    /** @brief Add a value to the running aggregate. */
    void add(double value);

    /**
     * @brief Remove a value from the running aggregate.
     *
     * For MIN/MAX a re-scan hint is set when `value` equals the current
     * extremum; the caller must invoke rescan() with the current window values
     * before querying result().
     */
    void remove(double value);

    /**
     * @brief Perform a full re-scan to recompute MIN / MAX.
     *
     * Must be called after remove() when rescanNeeded() returns true.
     *
     * @param values  All values currently in the window.
     */
    void rescan(const std::vector<double>& values);

    /** @return true if rescan() must be called before result(). */
    [[nodiscard]] bool rescanNeeded() const noexcept { return rescan_needed_; }

    /**
     * @return The current aggregate value.
     *
     * Returns 0.0 when count == 0 (AVG, SUM, COUNT, MIN, MAX all yield 0
     * for an empty window).
     */
    [[nodiscard]] double result() const noexcept;

    /** @return Number of values currently tracked. */
    [[nodiscard]] int64_t count() const noexcept { return count_; }

    /** Reset state (used when a new window starts). */
    void reset() noexcept;

private:
    AggOp   op_;
    int64_t count_{0};
    double  sum_{0.0};
    double  min_{std::numeric_limits<double>::max()};
    double  max_{std::numeric_limits<double>::lowest()};
    bool    rescan_needed_{false};
};

}  // namespace query
}  // namespace themis
