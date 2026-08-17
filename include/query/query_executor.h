/**
 * @file query_executor.h
 * @brief Query execution engine with iterator-safe result-set traversal.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416 iterator safety applied (Sprint 7 Batch C, Phase 2B)
 * @note Status: Production Ready
 *
 * The `QueryExecutor` is responsible for running a compiled `QueryPlan`
 * against the storage layer and streaming typed rows back to the caller.
 * Iterator safety is enforced at every result-set access via
 * `themis::security::SafeIterator`, addressing gap IDs B002–B009 from the
 * Sprint 7 scan (post-increment without bounds check, user-controlled offset
 * into result vectors).
 *
 * **CWE Remediations:**
 * - CWE-129: `AdvanceSafe::advance()` replaces raw `std::advance()` when
 *   navigating to user-requested row offsets.
 * - CWE-416: `BoundsChecker::check_dereference()` guards every row fetch
 *   before the iterator is dereferenced.
 * - `RangeValidator` validates every sub-range before inner loops begin.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "security/safe_iterator.h"

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// Value type
// ---------------------------------------------------------------------------

/**
 * @brief A single column value — one of the scalar types supported by AQL.
 *
 * Ordering follows AQL type precedence for implicit coercion.
 */
using ColumnValue = std::variant<
    std::monostate,   ///< NULL
    int64_t,          ///< INTEGER
    double,           ///< FLOAT
    std::string,      ///< TEXT / BLOB
    bool              ///< BOOLEAN
>;

// ---------------------------------------------------------------------------
// Row / ResultSet
// ---------------------------------------------------------------------------

/**
 * @brief One result row: an ordered sequence of typed column values.
 *
 * Column positions match the projection order in the query plan.
 */
using Row = std::vector<ColumnValue>;

/**
 * @brief A fully materialised result set (used for small result sets).
 *
 * For large result sets the streaming `RowCallback` API should be used
 * instead to avoid holding all rows in memory simultaneously.
 */
struct ResultSet {
    std::vector<std::string> column_names; ///< Column names in projection order.
    std::vector<Row>         rows;         ///< Materialised rows.

    /**
     * @brief Number of result rows.
     * @return Row count.
     */
    [[nodiscard]] std::size_t row_count() const noexcept { return rows.size(); }

    /**
     * @brief Access row at a given index with bounds checking.
     * @param index Zero-based row index.
     * @return Const reference to the row.
     * @throws std::out_of_range if `index >= row_count()`.
     *
     * **Iterator safety:** uses `BoundsChecker::check_dereference()` internally.
     */
    [[nodiscard]] const Row& at(std::size_t index) const;

    /**
     * @brief Fetch a window of rows [offset, offset+limit).
     *
     * Both `offset` and `limit` are user-supplied and validated through
     * `AdvanceSafe` before any iterator movement.
     *
     * @param offset First row to include (0-based).
     * @param limit  Maximum number of rows to return.
     * @return Sub-vector of rows; may be shorter than `limit` if near the end.
     * @throws std::out_of_range if `offset > row_count()`.
     */
    [[nodiscard]] std::vector<Row> page(std::size_t offset, std::size_t limit) const;
};

// ---------------------------------------------------------------------------
// Execution context
// ---------------------------------------------------------------------------

/**
 * @brief Opaque execution context injected by the caller.
 *
 * Holds references to storage, transaction state, and other subsystems
 * needed during plan execution.  `QueryExecutor` treats this as a
 * non-owning view; the lifetime must outlast the executor call.
 */
struct ExecutionContext {
    /// Maximum rows to materialise before streaming flush (0 = unbounded).
    std::size_t max_materialise_rows = 1024;
    /// Hard row-count limit across all result pages.
    std::size_t row_limit = 100'000;
    /// Query timeout in milliseconds (0 = no limit).
    uint32_t timeout_ms = 0;
};

// ---------------------------------------------------------------------------
// QueryPlan (forward opaque type)
// ---------------------------------------------------------------------------

/**
 * @brief Opaque compiled query plan produced by the query planner.
 *
 * The executor treats this as a read-only token; the actual plan tree is
 * accessed through the `QueryPlan::steps()` accessor in the implementation.
 */
struct QueryPlan {
    std::string                                  fingerprint; ///< SHA-256 digest.
    std::vector<std::string>                     column_names;
    std::vector<std::unordered_map<std::string, ColumnValue>> source_rows;
};

// ---------------------------------------------------------------------------
// RowCallback
// ---------------------------------------------------------------------------

/**
 * @brief Streaming row callback used when result sets may be large.
 *
 * Return `true` to continue streaming, `false` to abort early.
 *
 * @param row  The current result row (passed by const ref; do not store pointer).
 * @return `true` to continue, `false` to abort.
 */
using RowCallback = std::function<bool(const Row& row)>;

// ---------------------------------------------------------------------------
// QueryExecutor
// ---------------------------------------------------------------------------

/**
 * @brief Executes compiled query plans with iterator-safe result traversal.
 *
 * `QueryExecutor` is a short-lived, single-use object: construct it with a
 * plan and context, call `execute()` once, then discard.
 *
 * **Thread safety:** not thread-safe; each query must use its own executor.
 *
 * **Iterator safety guarantees:**
 * - Row-vector access: `BoundsChecker::check_dereference()` before every dereference.
 * - Page/offset arithmetic: `AdvanceSafe::advance()` replaces raw `std::advance()`.
 * - Sub-range iteration: `RangeValidator` validates every inner loop range.
 *
 * **Usage:**
 * ```cpp
 * ExecutionContext ctx;
 * ctx.row_limit = 500;
 * QueryExecutor exec(plan, ctx);
 * ResultSet rs = exec.execute();
 * for (const auto& row : rs.rows) { ... }
 * ```
 */
class QueryExecutor {
public:
    /**
     * @brief Construct the executor.
     * @param plan    Compiled query plan (must outlive the executor call).
     * @param context Execution constraints and dependencies.
     */
    QueryExecutor(const QueryPlan& plan, const ExecutionContext& context);

    ~QueryExecutor() = default;

    // Non-copyable, movable
    QueryExecutor(const QueryExecutor&)            = delete;
    QueryExecutor& operator=(const QueryExecutor&) = delete;
    QueryExecutor(QueryExecutor&&)                 noexcept = default;
    QueryExecutor& operator=(QueryExecutor&&)      noexcept = default;

    /**
     * @brief Execute the plan and materialise all results.
     *
     * @return Fully populated `ResultSet`.
     * @throws std::runtime_error if execution fails.
     * @throws std::length_error  if result exceeds `context.row_limit`.
     */
    [[nodiscard]] ResultSet execute();

    /**
     * @brief Execute the plan with streaming delivery via callback.
     *
     * Rows are emitted to `cb` one at a time in plan order.  Streaming stops
     * when either the plan is exhausted or `cb` returns `false`.
     *
     * @param cb  Row callback; must not throw.
     * @return Number of rows delivered to `cb`.
     * @throws std::runtime_error if execution fails.
     */
    std::size_t execute_streaming(RowCallback cb);

    /**
     * @brief Abort a running streaming execution at the next row boundary.
     *
     * Thread-safe: may be called from a signal handler or watchdog thread.
     * No-op if execution has already completed.
     */
    void abort() noexcept;

private:
    const QueryPlan*     plan_;
    const ExecutionContext* context_;
    std::atomic<bool>    aborted_{false};

    /// Build one Row from a source map entry.
    Row build_row(const std::unordered_map<std::string, ColumnValue>& src) const;
};

}  // namespace query
}  // namespace themis
