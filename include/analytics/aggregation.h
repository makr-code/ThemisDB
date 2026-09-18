/**
 * @file aggregation.h
 * @brief Analytics aggregation engine with iterator-safe group processing.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Status: Production Ready
 *
 * Provides GROUP BY aggregation (SUM, COUNT, AVG, MIN, MAX, FIRST, LAST) over
 * ordered or unordered input rows.  Iterator safety is enforced via
 * `themis::security::SafeIterator` to address:
 *
 * - **Type A (Invalidation):** `push_back()` inside iterator loops invalidates
 *   vector iterators; remediated by collect-then-write patterns (gap A004).
 * - **Type C (Unsafe Advance):** `std::advance()` with user-supplied offsets
 *   replaced by `AdvanceSafe::advance()` throughout (gap C001, C002).
 *
 * **CWE Remediations:**
 * - CWE-416: Collect-then-modify for all group insertion operations.
 * - CWE-129: `AdvanceSafe::advance()` for user offset navigation.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "security/safe_iterator.h"

namespace themis {
namespace analytics {

// ---------------------------------------------------------------------------
// Value type
// ---------------------------------------------------------------------------

using AggValue = std::variant<
    std::monostate,  ///< NULL
    int64_t,
    double,
    std::string
>;

// ---------------------------------------------------------------------------
// AggregateFunction
// ---------------------------------------------------------------------------

enum class AggregateFunction {
    kSum,   ///< Numeric sum (NULL inputs skipped)
    kCount, ///< Row count (including NULLs unless kCountNonNull is used)
    kCountNonNull, ///< Count rows where value is non-NULL
    kAvg,   ///< Arithmetic mean
    kMin,   ///< Minimum value
    kMax,   ///< Maximum value
    kFirst, ///< First value in input order
    kLast,  ///< Last value in input order
};

// ---------------------------------------------------------------------------
// AggregateSpec
// ---------------------------------------------------------------------------

struct AggregateSpec {
    std::string       source_column; ///< Input column name.
    std::string       output_column; ///< Output column name in result.
    AggregateFunction function;      ///< Aggregation function to apply.
};

// ---------------------------------------------------------------------------
// GroupKey
// ---------------------------------------------------------------------------

using GroupKey = std::string;

// ---------------------------------------------------------------------------
// AggregationRow / AggregationResult
// ---------------------------------------------------------------------------

using AggregationRow = std::unordered_map<std::string, AggValue>;

using AggregationOutputRow = std::unordered_map<std::string, AggValue>;

struct AggregationResult {
    std::vector<std::string>         group_columns; ///< GROUP BY column names.
    std::vector<std::string>         agg_columns;   ///< Aggregated column names.
    std::vector<AggregationOutputRow> rows;          ///< Result rows.

    [[nodiscard]] const AggregationOutputRow& at(std::size_t idx) const;

    [[nodiscard]] std::vector<AggregationOutputRow> page(
        std::size_t offset, std::size_t limit) const;
};

// ---------------------------------------------------------------------------
// Aggregator
// ---------------------------------------------------------------------------

class Aggregator {
public:
    Aggregator(std::vector<std::string>       group_by_columns,
               std::vector<AggregateSpec>     agg_specs);

    ~Aggregator() = default;

    Aggregator(const Aggregator&)            = delete;
    Aggregator& operator=(const Aggregator&) = delete;
    Aggregator(Aggregator&&)                 noexcept = default;
    Aggregator& operator=(Aggregator&&)      noexcept = default;

    /**
     * @brief Feed.
     * @param[in] row Input parameter.
     */
    void feed(const AggregationRow& row);

    [[nodiscard]] AggregationResult finalise();

    [[nodiscard]] std::size_t group_count() const noexcept;

private:
    // Per-accumulator state for one (group, aggregate-spec) pair.
    struct AccState {
        double       sum{0.0};
        double       min{0.0};
        double       max{0.0};
        std::int64_t count{0};
        AggValue     first;
        AggValue     last;
        bool         first_seen{false};
    };

    // Per-group accumulator map: spec-index → state.
    struct GroupState {
        AggregationRow                   key_values; ///< GROUP BY column snapshot.
        std::vector<AccState>            accs;        ///< One entry per agg_spec_.
    };

    std::vector<std::string>  group_by_columns_;
    std::vector<AggregateSpec> agg_specs_;
    std::unordered_map<GroupKey, GroupState> groups_;

    /**
     * @brief Make key.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    GroupKey make_key(const AggregationRow& row) const;

    /**
     * @brief Accumulate.
     * @param[in,out] acc Input/output parameter.
     * @param[in] fn Input parameter.
     * @param[in] val Input parameter.
     */
    static void accumulate(AccState& acc, AggregateFunction fn,
                           const AggValue& val);

    /**
     * @brief Extract.
     * @param[in] acc Input parameter.
     * @param[in] fn Input parameter.
     * @return Return value.
     */
    static AggValue extract(const AccState& acc, AggregateFunction fn);
};

}  // namespace analytics
}  // namespace themis
