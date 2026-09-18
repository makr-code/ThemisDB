/**
 * @file incremental_agg.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class AggOp { SUM, COUNT, AVG, MIN, MAX };

class IncrementalAgg {
public:
    /**
     * @brief Incremental Agg.
     * @param[in] op Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    explicit IncrementalAgg(AggOp op) noexcept;

    /**
     * @brief Add.
     * @param[in] value Input parameter.
     */
    void add(double value);

    /**
     * @brief Remove.
     * @param[in] value Input parameter.
     */
    void remove(double value);

    /**
     * @brief Rescan.
     * @param[in] values Input parameter.
     */
    void rescan(const std::vector<double>& values);

    [[nodiscard]] bool rescanNeeded() const noexcept { return rescan_needed_; }

    [[nodiscard]] double result() const noexcept;

    [[nodiscard]] int64_t count() const noexcept { return count_; }

    /**
     * @brief Reset the modification detection flag.
     * @note Exception safety: noexcept.
     */
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
