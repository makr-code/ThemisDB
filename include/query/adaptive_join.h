/**
 * @file adaptive_join.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

// ============================================================================
// Join Algorithm Enumeration
// ============================================================================

/**
 * @brief Supported join algorithms for adaptive selection.
 *
 * Selection is driven at runtime by the AdaptiveJoinExecutor based on
 * cardinality, sortedness, index availability, and available memory.
 */
enum class JoinAlgorithm {
    HASH_JOIN,         ///< Build hash table on smaller side; default for large equi-joins
    MERGE_JOIN,        ///< Sorted inputs, O(n+m) merge; chosen when both sides are sorted
    NESTED_LOOP_JOIN,  ///< Quadratic; chosen when left side < 1,000 rows
    INDEX_NESTED_LOOP, ///< Right side has index, left side < 10,000 rows
    BROADCAST_JOIN,    ///< Distributed: broadcast small table to all nodes
    SHUFFLE_JOIN,      ///< Distributed: repartition both sides on join key
    GRACE_HASH_JOIN    ///< Partitioned hash join (out-of-core); when memory budget exceeded
};

/**
 * @brief Return a human-readable name for a JoinAlgorithm value.
 */
const char* joinAlgorithmName(JoinAlgorithm algo) noexcept;

// ============================================================================
// Supporting Data Structures
// ============================================================================

/**
 * @brief A single row value – represented as a map of column name to string.
 *
 * Using string representation keeps the executor independent of the storage
 * layer's internal value types.
 */
using RowValue = std::unordered_map<std::string, std::string>;

/**
 * @brief A table abstraction used by AdaptiveJoinExecutor.
 *
 * Provides row-level access, sortedness metadata, and index availability.
 */
struct Table {
    std::string name;
    std::vector<RowValue> rows;

    /// true if rows are already sorted on the join key column.
    bool is_sorted = false;

    /// true if a secondary index exists on the join key column.
    bool has_index = false;

    /// Returns the number of rows in this table.
    [[nodiscard]] size_t rowCount() const noexcept { return rows.size(); }
};

/**
 * @brief Specification of a join operation.
 *
 * Describes both sides of the join and the equi-join predicate.
 */
struct JoinSpec {
    /// Column name on the left table used as the join key.
    std::string left_key;

    /// Column name on the right table used as the join key.
    std::string right_key;

    /**
     * @brief Optional additional filter applied to candidate pairs.
     *
     * Returns true when the two rows should be included in the output.
     * Defaults to accepting all pairs.
     */
    std::function<bool(const RowValue&, const RowValue&)> filter;
};

/**
 * @brief Runtime statistics that influence algorithm selection.
 */
struct RuntimeStats {
    /// Available memory budget for the join in bytes.
    size_t memory_budget_bytes = 256ULL * 1024ULL * 1024ULL;  // 256 MiB default

    /// Estimated bytes per row (used for memory estimation).
    size_t bytes_per_row = 256;

    /// Whether the query is executing in a distributed environment.
    bool is_distributed = false;

    /// Threshold at which GRACE_HASH_JOIN is preferred over HASH_JOIN
    /// (expressed as a fraction of memory_budget_bytes that the build side
    /// would occupy; default 0.9 means 90 % of budget).
    double grace_hash_threshold = 0.9;
};

/**
 * @brief Result of an AdaptiveJoinExecutor::executeJoin call.
 */
struct JoinResult {
    /// The output rows (cartesian subset matching the join predicate).
    std::vector<RowValue> rows;

    /// Which algorithm was selected for this execution.
    JoinAlgorithm algorithm_used{JoinAlgorithm::HASH_JOIN};

    /// Estimated cost (in abstract units) of the chosen algorithm.
    double estimated_cost{0.0};

    /// Returns the number of result rows.
    [[nodiscard]] size_t rowCount() const noexcept { return rows.size(); }
};

// ============================================================================
// Cost Model
// ============================================================================

/**
 * @brief Estimate the cost of executing a join with the given algorithm.
 *
 * The cost is an abstract unit proportional to the number of row-level
 * operations (comparisons, hash probes, etc.).
 *
 * @param algo           Algorithm to evaluate.
 * @param left_rows      Cardinality of the left (outer) table.
 * @param right_rows     Cardinality of the right (inner) table.
 * @param left_sorted    Whether the left side is already sorted on the join key.
 * @param right_sorted   Whether the right side is already sorted on the join key.
 * @return               Estimated cost, or numeric_limits<double>::max() if
 *                       the algorithm is inapplicable.
 */
double estimateJoinCost(JoinAlgorithm algo,
                        size_t left_rows,
                        size_t right_rows,
                        bool left_sorted = false,
                        bool right_sorted = false) noexcept;

// ============================================================================
// AdaptiveJoinConfig
// ============================================================================

/**
 * @brief Tuning knobs for the AdaptiveJoinExecutor.
 */
struct AdaptiveJoinConfig {
    /// Left-side cardinality threshold for NESTED_LOOP_JOIN selection.
    size_t nested_loop_threshold = 1'000;

    /// Left-side cardinality threshold for INDEX_NESTED_LOOP selection.
    size_t index_nested_loop_threshold = 10'000;

    /// Right-side broadcast threshold for BROADCAST_JOIN (rows).
    size_t broadcast_threshold = 10'000;
};

// ============================================================================
// AdaptiveJoinExecutor
// ============================================================================

/**
 * @brief Adaptive join executor that selects the optimal join algorithm at
 *        runtime based on data characteristics and resource constraints.
 *
 * ### Algorithm selection policy (in priority order):
 * 1. **NESTED_LOOP_JOIN** – left side < `config.nested_loop_threshold` rows
 * 2. **INDEX_NESTED_LOOP** – right has index AND left < `config.index_nested_loop_threshold`
 * 3. **MERGE_JOIN** – both inputs sorted on the join key
 * 4. **GRACE_HASH_JOIN** – build-side memory estimate exceeds
 *    `stats.grace_hash_threshold × stats.memory_budget_bytes`
 * 5. **BROADCAST_JOIN** – distributed mode AND smaller side ≤ `config.broadcast_threshold`
 * 6. **SHUFFLE_JOIN** – distributed mode (fallback)
 * 7. **HASH_JOIN** – default for all remaining equi-joins
 *
 * @note All execution is in-memory and single-threaded; the GRACE_HASH_JOIN
 *       path simulates partition-based processing without actual I/O.
 */
class AdaptiveJoinExecutor {
public:
    explicit AdaptiveJoinExecutor(AdaptiveJoinConfig config = {}) noexcept;

    /**
     * @brief Execute a join, choosing the best algorithm automatically.
     *
     * @param spec   Join specification (keys + optional filter).
     * @param left   Left (outer) table.
     * @param right  Right (inner) table.
     * @param stats  Runtime statistics that influence algorithm selection.
     * @return       JoinResult containing output rows and metadata.
     */
    [[nodiscard]] JoinResult executeJoin(const JoinSpec& spec,
                                         const Table& left,
                                         const Table& right,
                                         const RuntimeStats& stats) const;

    /**
     * @brief Determine which algorithm would be selected for the given inputs.
     *
     * Useful for testing and query-plan inspection without executing the join.
     * The memory budget is taken from @p stats.memory_budget_bytes.
     */
    [[nodiscard]] JoinAlgorithm selectAlgorithm(size_t left_rows,
                                                 size_t right_rows,
                                                 bool left_sorted,
                                                 bool right_sorted,
                                                 bool has_index,
                                                 const RuntimeStats& stats) const noexcept;

    /**
     * @brief Return the current configuration.
     */
    [[nodiscard]] const AdaptiveJoinConfig& config() const noexcept { return config_; }

    /**
     * @brief Update the configuration.
     */
    void setConfig(AdaptiveJoinConfig cfg) noexcept { config_ = std::move(cfg); }

private:
    // ---- Individual algorithm implementations ----

    JoinResult executeHashJoin(const JoinSpec& spec,
                               const Table& left,
                               const Table& right) const;

    JoinResult executeMergeJoin(const JoinSpec& spec,
                                const Table& left,
                                const Table& right) const;

    JoinResult executeNestedLoopJoin(const JoinSpec& spec,
                                     const Table& left,
                                     const Table& right) const;

    JoinResult executeIndexNestedLoopJoin(const JoinSpec& spec,
                                          const Table& left,
                                          const Table& right) const;

    JoinResult executeGraceHashJoin(const JoinSpec& spec,
                                    const Table& left,
                                    const Table& right) const;

    JoinResult executeBroadcastJoin(const JoinSpec& spec,
                                    const Table& left,
                                    const Table& right) const;

    JoinResult executeShuffleJoin(const JoinSpec& spec,
                                  const Table& left,
                                  const Table& right) const;

    // ---- Helper ----

    /// Merge two rows into a single output row (right side wins on key collision).
    static RowValue mergeRows(const RowValue& left_row, const RowValue& right_row);

    AdaptiveJoinConfig config_;
};

} // namespace themis
