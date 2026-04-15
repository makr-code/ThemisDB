/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            parallel_executor.h                                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:04:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     300                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 39ac8c3efe  2026-03-20  Split default-arg constructors into overloads ║
    • c195c25f52  2026-03-14  fix(query): address PR review comments on ParallelExecutor ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file parallel_executor.h
 * @brief Intra-query parallel execution engine (v1.7.0).
 *
 * Provides morsel-driven parallelism for the three most common query
 * operators: table scan with predicate, partitioned hash join, and
 * two-phase aggregation.  All operations use Intel TBB task_group
 * internally and respect the single @c ParallelConfig that controls
 * thread count and morsel granularity.
 *
 * ### Performance targets (from roadmap:92)
 * - Linear scaling up to 8 cores
 * - 70–80 % efficiency at 16 cores
 * - Scans: 4× speedup on 4 cores
 * - Joins: 3× speedup on 4 cores
 */

#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "storage/base_entity.h"
#include "utils/expected.h"

namespace themis {

// ============================================================================
// ParallelExecutor
// ============================================================================

/**
 * @brief Intra-query parallel execution engine.
 *
 * Stateless executor: all configuration is embedded in @c ParallelConfig.
 * Create one instance per query context or share it across queries (the
 * class itself holds no mutable state after construction).
 *
 * @code
 * ParallelExecutor exec;
 *
 * // Parallel scan
 * auto result = exec.parallelScan(rows, [](const BaseEntity& e) {
 *     auto v = e.getFieldAsInt("age");
 *     return v && *v > 30;
 * });
 *
 * // Parallel hash join
 * ParallelExecutor::JoinSpec spec{"dept_id", "id"};
 * auto joined = exec.parallelHashJoin(employees, departments, spec);
 *
 * // Parallel aggregation
 * ParallelExecutor::AggregateSpec agg{"salary",
 *     ParallelExecutor::AggregateFunction::Sum, {"dept"}};
 * auto totals = exec.parallelAggregate(employees, agg);
 * @endcode
 */
class ParallelExecutor {
public:
    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Runtime tuning knobs for the parallel executor.
     *
     * All three @c enable_* flags default to @c true.  Setting one to
     * @c false forces the corresponding operation into the single-threaded
     * sequential path regardless of the requested thread count.
     */
    struct ParallelConfig {
        /// Maximum number of worker threads. Defaults to all hardware threads.
        size_t max_threads = std::thread::hardware_concurrency();

        /// Rows per morsel (TBB task granularity).
        size_t morsel_size = 1'024;

        /// Enable parallel path for @c parallelScan.
        bool enable_parallel_scan = true;

        /// Enable parallel path for @c parallelHashJoin.
        bool enable_parallel_join = true;

        /// Enable parallel path for @c parallelAggregate.
        bool enable_parallel_aggregate = true;
    };

    // ========================================================================
    // Data types
    // ========================================================================

    /// A collection of entities passed to parallel operators.
    using Table = std::vector<BaseEntity>;

    /// Predicate callable used by @c parallelScan.
    using FilterFn = std::function<bool(const BaseEntity&)>;

    /// One row in the result of @c parallelHashJoin.
    struct JoinTuple {
        BaseEntity left;   ///< Row from the left (probe) side.
        BaseEntity right;  ///< Row from the right (build) side.
    };

    /**
     * @brief Equi-join specification for @c parallelHashJoin.
     *
     * The join is performed as:
     * @code
     *   left.getFieldAsString(left_key) == right.getFieldAsString(right_key)
     * @endcode
     */
    struct JoinSpec {
        std::string left_key;   ///< Field name on the left (probe) side.
        std::string right_key;  ///< Field name on the right (build) side.
    };

    /// Aggregation function applied by @c parallelAggregate.
    enum class AggregateFunction {
        Count,          ///< COUNT(*) – counts all input rows (field is ignored).
        Sum,            ///< SUM(field)
        Avg,            ///< AVG(field)
        Min,            ///< MIN(field)
        Max,            ///< MAX(field)
    };

    /**
     * @brief Aggregation specification for @c parallelAggregate.
     *
     * When @c group_by is empty the result contains a single entry keyed by
     * the empty string.  When @c group_by is non-empty each distinct
     * combination of group-field values produces one entry.  The group key
     * uses a length-prefixed encoding (@c "len:value|len:value|...") so
     * field values that contain the @c '|' character never cause collisions.
     */
    struct AggregateSpec {
        std::string field;               ///< Field to aggregate (ignored for Count).
        AggregateFunction function;      ///< Aggregation function.
        std::vector<std::string> group_by; ///< Optional GROUP BY fields.
    };

    /**
     * @brief Result of @c parallelAggregate.
     *
     * Maps a group key (or @c "" for un-grouped queries) to the computed
     * aggregate value.
     */
    using AggregateResult = std::unordered_map<std::string, double>;

    // ========================================================================
    // Construction
    // ========================================================================

    ParallelExecutor();
    explicit ParallelExecutor(ParallelConfig config);

    const ParallelConfig& getConfig() const noexcept { return config_; }
    void setConfig(const ParallelConfig& cfg) {
        config_ = cfg;
        validateConfig(config_);
    }

    // ========================================================================
    // Public API
    // ========================================================================

    /**
     * @brief Parallel table scan with filter predicate.
     *
     * Splits @p input into morsels and evaluates @p filter concurrently.
     * Falls back to a sequential loop when:
     *   - @c enable_parallel_scan is @c false, or
     *   - the effective thread count resolves to 1, or
     *   - @p input is smaller than one morsel.
     *
     * @param input       Rows to scan (read-only).
     * @param filter      Predicate; rows for which it returns @c true are kept.
     * @param num_threads Desired parallelism (0 → use @c config.max_threads).
     * @return Filtered rows in the same relative order as in @p input
     *         (morsel-stable: rows from earlier morsels precede rows from
     *         later morsels, and within each morsel the input order is kept).
     */
    Result<Table> parallelScan(
        const Table&    input,
        const FilterFn& filter,
        size_t          num_threads = 0) const;

    /**
     * @brief Partitioned parallel hash join.
     *
     * Algorithm:
     *   1. Hash-partition both sides by @c JoinSpec::right_key modulo the
     *      partition count (= effective thread count).
     *   2. Each worker builds a hash table over its right-side partition.
     *   3. Each worker probes with its left-side partition rows.
     *   4. Results are merged into the returned vector.
     *
     * Falls back to a single-partition (sequential) hash join when
     * @c enable_parallel_join is @c false or effective threads = 1.
     *
     * @param left        Probe-side rows.
     * @param right       Build-side rows.
     * @param spec        Equi-join key specification.
     * @param num_threads Desired parallelism (0 → use @c config.max_threads).
     * @return All matching (left, right) tuples.
     */
    Result<std::vector<JoinTuple>> parallelHashJoin(
        const Table&    left,
        const Table&    right,
        const JoinSpec& spec,
        size_t          num_threads = 0) const;

    /**
     * @brief Two-phase parallel aggregation.
     *
     * Phase 1: Each worker computes partial aggregates over its morsel.
     * Phase 2: Partial aggregates are merged into the final result.
     *
     * Falls back to single-threaded aggregation when
     * @c enable_parallel_aggregate is @c false or effective threads = 1.
     *
     * @param input       Rows to aggregate (read-only).
     * @param spec        Aggregation specification.
     * @param num_threads Desired parallelism (0 → use @c config.max_threads).
     * @return Aggregated values keyed by group key (or @c "" if no GROUP BY).
     */
    Result<AggregateResult> parallelAggregate(
        const Table&         input,
        const AggregateSpec& spec,
        size_t               num_threads = 0) const;

private:
    ParallelConfig config_;

    /// Resolve effective thread count: clamp to [1, max_threads].
    size_t resolveThreads(size_t requested) const noexcept;

    /// Clamp zero values in a config to their minimum of 1.
    static void validateConfig(ParallelConfig& cfg) noexcept;

    // ── Internal sequential helpers ─────────────────────────────────────────

    static Table sequentialScan(
        const Table& input, const FilterFn& filter);

    static std::vector<JoinTuple> sequentialHashJoin(
        const Table& left, const Table& right, const JoinSpec& spec);

    static AggregateResult sequentialAggregate(
        const Table& input, const AggregateSpec& spec);

    // ── Partial aggregate bookkeeping ───────────────────────────────────────

    /// Per-group accumulator used during the parallel phase.
    struct PartialAgg {
        double sum   = 0.0;
        double count = 0.0;
        double min   = std::numeric_limits<double>::max();
        double max   = std::numeric_limits<double>::lowest();
    };

    using PartialMap = std::unordered_map<std::string, PartialAgg>;

    static void mergePartial(PartialMap& dst, const PartialMap& src);
    static double finalise(const PartialAgg& p, AggregateFunction fn);

    /// Compute the group key string from an entity for the given spec.
    static std::string groupKey(
        const BaseEntity& e, const std::vector<std::string>& group_by);
};

} // namespace themis
