/**
 * @file streaming_join.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "analytics/columnar_execution.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace analytics {

// ============================================================================
// JoinType
// ============================================================================

/**
 * @brief Supported streaming join semantics.
 */
enum class JoinType {
    Inner,     ///< Only matched rows are emitted.
    LeftOuter, ///< All probe rows are emitted; unmatched fields are null.
};

// ============================================================================
// IStreamingJoin — base interface
// ============================================================================

/**
 * @brief Abstract base for streaming join operators.
 *
 * Both `HashJoin` and `IntervalJoin` satisfy this interface, enabling
 * polymorphic use in pipeline stages.
 */
class IStreamingJoin {
public:
    virtual ~IStreamingJoin() = default;

    /**
     * @brief Process a probe-side batch and return the joined result.
     *
     * @param probe  A `ColumnBatch` from the probe stream.
     * @return       A new `ColumnBatch` containing the joined rows.
     *               Column order: probe columns first, then build columns
     *               (excluding join-key duplicates).
     */
    [[nodiscard]] virtual ColumnBatch probe(const ColumnBatch& probe) = 0;

    /**
     * @brief Reset the build-side state (hash table / event buffer).
     *
     * After `reset()` the join can be rebuilt for a new time window or
     * a new micro-batch.
     */
    virtual void reset() = 0;

    /**
     * @brief Number of rows currently stored on the build side.
     */
    [[nodiscard]] virtual size_t buildSideSize() const noexcept = 0;
};

// ============================================================================
// HashJoin
// ============================================================================

/**
 * @brief Columnar equi-join using an in-memory hash table.
 *
 * The "build" side is loaded once via `build()` or accumulated with
 * successive `addBuildBatch()` calls.  Every `probe()` call performs an
 * O(1) hash lookup per row.
 *
 * ### Performance targets
 * - Build throughput  ≥ 10 M rows/s on a single core (int64 key)
 * - Probe throughput  ≥ 10 M rows/s on a single core (cache-warm)
 * - Memory footprint  O(build_rows) with no over-allocation
 */
class HashJoin : public IStreamingJoin {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Column name(s) used as the join key.  The same names must exist
        /// on both the build and probe sides.
        std::vector<std::string> join_keys;

        /// Join semantics.
        JoinType join_type = JoinType::Inner;

        /// Columns to project from the build side into the result.
        /// Empty means "all build columns".
        std::vector<std::string> build_select;

        /// Columns to project from the probe side into the result.
        /// Empty means "all probe columns".
        std::vector<std::string> probe_select;

        /// Maximum number of rows stored on the build side before
        /// `addBuildBatch()` returns an error flag.  0 = unlimited.
        size_t max_build_rows = 0;
    };

    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /// A composite join key serialized to a string for use as a hash-map key.
    using CompositeKey = std::string;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    explicit HashJoin(Config config);
    ~HashJoin() override = default;

    // Non-copyable (hash table may be large)
    HashJoin(const HashJoin&)            = delete;
    HashJoin& operator=(const HashJoin&) = delete;
    HashJoin(HashJoin&&)                 noexcept noexcept = default;

    // -----------------------------------------------------------------------
    // Build phase
    // -----------------------------------------------------------------------

    /**
     * @brief Accumulate one batch into the build-side hash table.
     *
     * @param batch  A ColumnBatch from the build stream.
     * @return       `true` on success, `false` if `max_build_rows` was exceeded.
     */
    bool addBuildBatch(const ColumnBatch& batch);

    /**
     * @brief Convenience: add all batches from an iterator range.
     *
     * @tparam It  Iterator over `const ColumnBatch&`.
     */
    template<typename It>
    bool build(It begin, It end) {
        for (auto it = begin; it != end; ++it) {
            if (!addBuildBatch(*it)) return false;
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Probe phase — IStreamingJoin
    // -----------------------------------------------------------------------

    ColumnBatch probe(const ColumnBatch& probe) override;
    void        reset() override;
    size_t      buildSideSize() const noexcept override { return build_row_count_; }

private:
    Config cfg_;

    /// Hash table: composite key → row indices in build_columns_.
    std::unordered_map<CompositeKey, std::vector<size_t>> hash_table_;

    /// Build-side column data (all rows accumulated).
    std::vector<std::shared_ptr<Column>> build_columns_;

    /// Column names in the same order as build_columns_.
    std::vector<std::string> build_column_names_;

    size_t build_row_count_{0};

    // Helpers
    CompositeKey makeKey(const std::vector<std::shared_ptr<Column>>& cols,
                         const std::vector<size_t>& key_col_indices,
                         size_t row) const;

    ColumnValue getVal(const Column& col, size_t row) const;

    void appendNullRow(std::vector<std::shared_ptr<Column>>& cols,
                       const std::vector<std::string>&       names) const;
};

// ============================================================================
// IntervalJoin
// ============================================================================

/**
 * @brief Time-interval join that correlates events from two streams whose
 *        event-time columns fall within a configurable window.
 *
 * For each probe row with timestamp `t`, every build row with timestamp `b`
 * satisfying:
 * @code
 *   t - before_ms <= b <= t + after_ms
 * @endcode
 * is emitted as a joined pair.
 *
 * ### Ordering requirement
 * Build events must be added in **non-decreasing** event-time order.
 * Probe batches must also arrive in non-decreasing event-time order.
 * Out-of-order events trigger an optional callback and are skipped.
 *
 * ### Memory management
 * The build-side buffer is automatically pruned: events older than
 * `t_probe - before_ms - slack_ms` are discarded after each `probe()` call.
 * `slack_ms` (default: 0) adds extra retention for late-arriving probes.
 *
 * ### Performance target
 * - Probe throughput ≥ 1 M matched pairs/s for a 10-second window at 10 kHz
 */
class IntervalJoin : public IStreamingJoin {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Equi-join key columns (in addition to the time predicate).
        std::vector<std::string> join_keys;

        /// Column that holds the event timestamp (int64, milliseconds since epoch).
        std::string time_column;

        /// Match radius on the left (build side earlier than probe).
        int64_t before_ms = 0;

        /// Match radius on the right (build side later than probe).
        int64_t after_ms  = 0;

        /// Join semantics.
        JoinType join_type = JoinType::Inner;

        /// Extra retention margin for the build buffer beyond `before_ms`.
        int64_t slack_ms = 0;

        /// Columns to project from the build side (empty = all).
        std::vector<std::string> build_select;

        /// Columns to project from the probe side (empty = all).
        std::vector<std::string> probe_select;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    explicit IntervalJoin(Config config);
    ~IntervalJoin() override = default;

    IntervalJoin(const IntervalJoin&)            = delete;
    IntervalJoin& operator=(const IntervalJoin&) = delete;

    // -----------------------------------------------------------------------
    // Build phase
    // -----------------------------------------------------------------------

    /**
     * @brief Add one batch of build-side events to the internal buffer.
     *
     * Events are stored in arrival order; sorting is performed on demand.
     */
    void addBuildBatch(const ColumnBatch& batch);

    // -----------------------------------------------------------------------
    // Probe phase — IStreamingJoin
    // -----------------------------------------------------------------------

    ColumnBatch probe(const ColumnBatch& probe) override;
    void        reset() override;
    size_t      buildSideSize() const noexcept override;

private:
    Config cfg_;

    /// One stored build-side row.
    struct BuildRow {
        int64_t    timestamp_ms;
        std::vector<ColumnValue> values;   ///< All build columns in order.
    };

    std::vector<BuildRow>        build_buffer_;
    std::vector<std::string>     build_col_names_;
    bool                         build_sorted_ = false;

    // Helpers
    void sortBuildBuffer();
    void pruneBuildBuffer(int64_t min_keep_ms);

    ColumnValue getVal(const Column& col, size_t row) const;
    std::string makeKey(const BuildRow& row,
                        const std::vector<size_t>& key_col_indices) const;
    std::string makeProbeKey(const std::vector<std::shared_ptr<Column>>& probe_cols,
                              const std::vector<size_t>& key_col_indices,
                              size_t row) const;
};

} // namespace analytics
} // namespace themisdb
