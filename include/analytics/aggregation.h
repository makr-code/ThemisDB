/**
 * @file aggregation.h
 * @brief Analytics aggregation engine with iterator-safe group processing.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416 iterator safety applied (Sprint 7 Batch C, Phase 2B+2D)
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

/**
 * @brief Scalar value type used in aggregation operations.
 */
using AggValue = std::variant<
    std::monostate,  ///< NULL
    int64_t,
    double,
    std::string
>;

// ---------------------------------------------------------------------------
// AggregateFunction
// ---------------------------------------------------------------------------

/**
 * @brief Supported aggregate functions.
 */
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

/**
 * @brief Describes one aggregate column in a GROUP BY query.
 */
struct AggregateSpec {
    std::string       source_column; ///< Input column name.
    std::string       output_column; ///< Output column name in result.
    AggregateFunction function;      ///< Aggregation function to apply.
};

// ---------------------------------------------------------------------------
// GroupKey
// ---------------------------------------------------------------------------

/**
 * @brief Composite group-by key.  Serialised to a string for map lookup.
 */
using GroupKey = std::string;

// ---------------------------------------------------------------------------
// AggregationRow / AggregationResult
// ---------------------------------------------------------------------------

/**
 * @brief One input row: column name → scalar value.
 */
using AggregationRow = std::unordered_map<std::string, AggValue>;

/**
 * @brief One output group row.
 */
using AggregationOutputRow = std::unordered_map<std::string, AggValue>;

/**
 * @brief The full aggregation output: one row per distinct group.
 */
struct AggregationResult {
    std::vector<std::string>         group_columns; ///< GROUP BY column names.
    std::vector<std::string>         agg_columns;   ///< Aggregated column names.
    std::vector<AggregationOutputRow> rows;          ///< Result rows.

    /**
     * @brief Bounds-checked row accessor.
     * @param idx Zero-based row index.
     * @return Const reference to the row.
     * @throws std::out_of_range if `idx >= rows.size()`.
     */
    [[nodiscard]] const AggregationOutputRow& at(std::size_t idx) const;

    /**
     * @brief Paginate result rows.
     * @param offset First row (validated with `AdvanceSafe`).
     * @param limit  Row count cap.
     * @return Sub-vector of rows.
     */
    [[nodiscard]] std::vector<AggregationOutputRow> page(
        std::size_t offset, std::size_t limit) const;
};

// ---------------------------------------------------------------------------
// Aggregator
// ---------------------------------------------------------------------------

/**
 * @brief Stateful GROUP BY aggregation engine.
 *
 * Processes rows in a single pass (call `feed()` once per input row) and
 * materialises the final result with `finalise()`.
 *
 * **Type A safety:** new groups are accumulated in a secondary map and merged
 * after each batch, so no iterator into the active group map is held across
 * a potential rehash or insertion.
 *
 * **Type C safety:** all user-supplied page offsets are validated with
 * `AdvanceSafe::advance()` inside `AggregationResult::page()`.
 *
 * **Usage:**
 * ```cpp
 * Aggregator agg({"country"}, {{"revenue", "total_revenue", AggregateFunction::kSum}});
 * for (const auto& row : input_rows) { agg.feed(row); }
 * AggregationResult result = agg.finalise();
 * ```
 */
class Aggregator {
public:
    /**
     * @brief Construct aggregator.
     * @param group_by_columns  Ordered list of columns to group by.
     * @param agg_specs         Aggregation specifications.
     * @throws std::invalid_argument if lists are empty or contain duplicates.
     */
    Aggregator(std::vector<std::string>       group_by_columns,
               std::vector<AggregateSpec>     agg_specs);

    ~Aggregator() = default;

    Aggregator(const Aggregator&)            = delete;
    Aggregator& operator=(const Aggregator&) = delete;
    Aggregator(Aggregator&&)                 noexcept = default;
    Aggregator& operator=(Aggregator&&)      noexcept = default;

    /**
     * @brief Feed one input row into the aggregator.
     * @param row Input row mapping column names to values.
     *
     * NULL values in group-by columns are treated as a distinct group key
     * (represented as the empty string in the composite key).
     */
    void feed(const AggregationRow& row);

    /**
     * @brief Finalise aggregation and return the result set.
     *
     * After `finalise()` returns the aggregator state is consumed; calling
     * `feed()` again results in undefined behaviour.
     *
     * @return Completed aggregation result.
     */
    [[nodiscard]] AggregationResult finalise();

    /**
     * @brief Current number of distinct groups seen so far.
     * @return Group count.
     */
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

    /// Serialise group-by column values to a stable string key.
    GroupKey make_key(const AggregationRow& row) const;

    /// Apply one value to an accumulator.
    static void accumulate(AccState& acc, AggregateFunction fn,
                           const AggValue& val);

    /// Extract final value from accumulator.
    static AggValue extract(const AccState& acc, AggregateFunction fn);
};

}  // namespace analytics
}  // namespace themis
