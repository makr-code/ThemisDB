/**
 * @file stats.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Analytics – Statistics Utilities
 *
 * Lightweight, header-only statistical helpers shared across the analytics
 * subsystem.  All functions operate on const references to avoid O(N) heap
 * copies on every call-site.
 *
 * Key functions:
 *   - computePercentile(vals, p)   — percentile via linear interpolation
 *     (does NOT sort in-place; copies into a scratch vector internally)
 *
 * Thread-safety: all functions are stateless and thread-safe.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace themis::analytics::detail {

/**
 * @brief Compute Percentile.
 * @param[in] vals Input parameter.
 * @param[in] p Input parameter.
 * @return Return value.
 * @details Calls: empty(), scratch(), begin(), end(), std::sort(), front(), back(), size().
 */
inline double computePercentile(const std::vector<double>& vals, double p) {
    if (vals.empty()) {
      return 0.0;
    }
    // Local scratch copy – the only allocation; avoids modifying caller's data.
    std::vector<double> scratch(vals.begin(), vals.end());
    std::sort(scratch.begin(), scratch.end());
    if (p <= 0.0) {
      return scratch.front();
    }
    if (p >= 100.0) {
      return scratch.back();
    }
    const double idx = (p / 100.0) * static_cast<double>(scratch.size() - 1);
    const size_t lo  = static_cast<size_t>(idx);
    const size_t hi  = lo + 1;
    if (hi >= scratch.size()) {
      return scratch.back();
    }
    const double frac = idx - static_cast<double>(lo);
    return scratch[lo] + frac * (scratch[hi] - scratch[lo]);
}

/**
 * @brief Compute Percentile.
 * @param[in] vals Input parameter.
 * @param[in] p Input parameter.
 * @return Return value.
 * @details Calls: empty(), scratch(), begin(), end(), std::sort(), front(), back(), size().
 */
inline double computePercentile(std::span<const double> vals, double p) {
    if (vals.empty()) {
      return 0.0;
    }
    std::vector<double> scratch(vals.begin(), vals.end());
    std::sort(scratch.begin(), scratch.end());
    if (p <= 0.0) {
      return scratch.front();
    }
    if (p >= 100.0) {
      return scratch.back();
    }
    const double idx = (p / 100.0) * static_cast<double>(scratch.size() - 1);
    const size_t lo  = static_cast<size_t>(idx);
    const size_t hi  = lo + 1;
    if (hi >= scratch.size()) {
      return scratch.back();
    }
    const double frac = idx - static_cast<double>(lo);
    return scratch[lo] + frac * (scratch[hi] - scratch[lo]);
}

} // namespace themis::analytics::detail
