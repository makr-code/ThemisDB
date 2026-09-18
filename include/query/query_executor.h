/**
 * @file query_executor.h
 * @brief Query execution engine with iterator-safe result-set traversal.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
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

using Row = std::vector<ColumnValue>;

struct ResultSet {
    std::vector<std::string> column_names; ///< Column names in projection order.
    std::vector<Row>         rows;         ///< Materialised rows.

    [[nodiscard]] std::size_t row_count() const noexcept { return rows.size(); }

    [[nodiscard]] const Row& at(std::size_t index) const;

    [[nodiscard]] std::vector<Row> page(std::size_t offset, std::size_t limit) const;
};

// ---------------------------------------------------------------------------
// Execution context
// ---------------------------------------------------------------------------

struct ExecutionContext {
    std::size_t max_materialise_rows = 1024;
    std::size_t row_limit = 100'000;
    uint32_t timeout_ms = 0;
};

// ---------------------------------------------------------------------------
// QueryPlan (forward opaque type)
// ---------------------------------------------------------------------------

struct QueryPlan {
    std::string                                  fingerprint; ///< SHA-256 digest.
    std::vector<std::string>                     column_names;
    std::vector<std::unordered_map<std::string, ColumnValue>> source_rows;
};

// ---------------------------------------------------------------------------
// RowCallback
// ---------------------------------------------------------------------------

using RowCallback = std::function<bool(const Row& row)>;

// ---------------------------------------------------------------------------
// QueryExecutor
// ---------------------------------------------------------------------------

class QueryExecutor {
public:
    QueryExecutor(const QueryPlan& plan, const ExecutionContext& context);

    ~QueryExecutor() = default;

    // Non-copyable, movable
    QueryExecutor(const QueryExecutor&)            = delete;
    QueryExecutor& operator=(const QueryExecutor&) = delete;
    QueryExecutor(QueryExecutor&&)                 noexcept = default;
    QueryExecutor& operator=(QueryExecutor&&)      noexcept = default;

    [[nodiscard]] ResultSet execute();

    /**
     * @brief Execute streaming.
     * @param[in] cb Input parameter.
     * @return Return value.
     */
    std::size_t execute_streaming(RowCallback cb);

    /**
     * @brief Abort.
     * @note Exception safety: noexcept.
     */
    void abort() noexcept;

private:
    const QueryPlan*     plan_;
    const ExecutionContext* context_;
    std::atomic<bool>    aborted_{false};
    std::chrono::steady_clock::time_point execution_start_;

    Row build_row(const std::unordered_map<std::string, ColumnValue>& src) const;

    [[nodiscard]] bool isExecutionTimeoutExceeded() const noexcept;
};

}  // namespace query
}  // namespace themis
