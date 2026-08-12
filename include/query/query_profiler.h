/**
 * @file query_profiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>

namespace themis {
namespace query {

/**
 * @brief Timing and resource statistics for a single query-plan operator.
 */
struct OperatorProfile {
    /// Operator name, e.g. "HashJoin", "SeqScan", "Filter".
    std::string operator_name;

    /// Wall-clock time spent in this operator (nanoseconds).
    int64_t duration_ns = 0;

    /// Number of rows fed into this operator.
    size_t rows_in = 0;

    /// Number of rows emitted by this operator.
    size_t rows_out = 0;

    /// Peak heap memory consumed by this operator (bytes).
    size_t memory_bytes = 0;

    /// Number of storage (disk/RocksDB) read I/O operations.
    size_t io_reads = 0;
};

/**
 * @brief Aggregated profiling data for a complete query execution.
 */
struct QueryProfile {
    /// Query text that was profiled.
    std::string query_text;

    /// Total wall-clock time from plan start to last result row (nanoseconds).
    int64_t total_duration_ns = 0;

    /// Peak heap memory across all operators (bytes).
    size_t peak_memory_bytes = 0;

    /// Per-operator breakdown, in execution order.
    std::vector<OperatorProfile> operators;

    /// Number of result rows returned to the caller.
    size_t result_rows = 0;

    /// True if the query was served entirely from cache.
    bool cache_hit = false;

    /// Returns the operator profile with the highest `duration_ns`, or nullptr
    /// if no operators have been recorded.
    const OperatorProfile* slowestOperator() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for query-execution profiling.
 *
 * Implementations record timing and resource information for each query
 * operator, allowing operators to be instrumented without coupling to a
 * specific profiling backend.
 *
 * Thread safety: implementations must be safe for concurrent use by
 * multiple operator threads within the same query execution.
 */
class IQueryProfiler {
public:
    virtual ~IQueryProfiler() = default;

    /**
     * @brief Signal the start of a query.
     * @param query_text  AQL or SQL text being executed (for reporting).
     */
    virtual void beginQuery(const std::string& query_text) = 0;

    /**
     * @brief Signal the end of a query.
     * @param result_rows Number of rows returned to the caller.
     * @param cache_hit   True when the result was served from cache.
     */
    virtual void endQuery(size_t result_rows, bool cache_hit = false) = 0;

    /**
     * @brief Signal the start of an individual plan operator.
     * @param operator_name Human-readable operator identifier.
     */
    virtual void beginOperator(const std::string& operator_name) = 0;

    /**
     * @brief Signal the end of an operator.
     * @param rows_in     Rows consumed by the operator.
     * @param rows_out    Rows emitted by the operator.
     * @param memory_bytes Peak memory used by the operator.
     * @param io_reads    Storage read operations performed.
     */
    virtual void endOperator(size_t rows_in, size_t rows_out,
                             size_t memory_bytes = 0,
                             size_t io_reads = 0) = 0;

    /// Retrieve the accumulated profile for the most recent query.
    [[nodiscard]] virtual QueryProfile getProfile() const = 0;

    /// Reset all accumulated state, ready for the next query.
    virtual void reset() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Concrete implementations
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Low-overhead query profiler suitable for production use.
 *
 * Records wall-clock time for each operator using
 * `std::chrono::steady_clock`. Memory and I/O statistics are provided
 * by the caller via `endOperator()`.
 *
 * Not thread-safe between `beginQuery`/`endQuery` pairs; each query
 * must use its own `QueryProfiler` instance (or protect with a mutex).
 */
class QueryProfiler : public IQueryProfiler {
public:
    ~QueryProfiler() override = default;
    QueryProfiler() = default;

    void beginQuery(const std::string& query_text) override;
    void endQuery(size_t result_rows, bool cache_hit = false) override;
    void beginOperator(const std::string& operator_name) override;
    void endOperator(size_t rows_in, size_t rows_out,
                     size_t memory_bytes = 0,
                     size_t io_reads = 0) override;
    QueryProfile getProfile() const override;
    void reset() override;

private:
    QueryProfile profile_;
    std::chrono::steady_clock::time_point query_start_;
    std::chrono::steady_clock::time_point op_start_;
    std::string current_operator_;
};

/**
 * @brief No-op profiler that discards all profiling information.
 *
 * Use when profiling is disabled to avoid any runtime overhead.
 * All methods are empty and `getProfile()` returns a default-constructed
 * `QueryProfile`.
 */
class NullQueryProfiler : public IQueryProfiler {
public:
    ~NullQueryProfiler() override = default;
    void beginQuery(const std::string&) override {}
    void endQuery(size_t, bool) override {}
    void beginOperator(const std::string&) override {}
    void endOperator(size_t, size_t, size_t, size_t) override {}
    QueryProfile getProfile() const override { return {}; }
    void reset() override {}
};

} // namespace query
} // namespace themis
