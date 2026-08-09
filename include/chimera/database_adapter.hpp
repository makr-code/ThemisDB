/**
 * @file database_adapter.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: database_adapter.hpp | Version: 0.0.43 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 142
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0
 * PR History (last 5): #4478 feat(chimera): Streaming re... (2026-04-11) | #4129 feat(chimera): Multi-Databa... (2026-03-12) | #4123 feat(chimera): AdapterConfi... (2026-03-12) | #4122 feat(chimera): async/promis... (2026-03-12) | #4098 feat(chimera): Batch Operat... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

// Prefer using the external CHIMERA headers when available. If the
// external submodule isn't present (CI or shallow checkout), provide a
// minimal local shim so builds don't fail at configure/compile time.
#if __has_include("../../external/chimera/include/chimera/database_adapter.hpp")
#include "../../external/chimera/include/chimera/database_adapter.hpp"

// Compatibility shim: older ThemisDB adapter code references capability
// tokens that are not present in some external CHIMERA snapshots.
//
// IMPORTANT: These aliases are compile-time compatibility mappings only.
// They preserve source compatibility for focused builds and tests where we
// compile against older external enum surfaces.
#ifndef GRAPH_OPERATIONS
    #define GRAPH_OPERATIONS GRAPH_TRAVERSAL
#endif

#ifndef STREAMING_RESULTS
    #define STREAMING_RESULTS STREAM_PROCESSING
#endif

#ifndef PREPARED_STATEMENTS
    // No dedicated enum entry in older snapshots; map to a currently unused
    // capability token to keep references well-formed in compile-time checks.
    #define PREPARED_STATEMENTS SHARDING
#endif

#ifndef CONNECTION_POOLING
    // No dedicated enum entry in older snapshots; map to a currently unused
    // capability token for compatibility with legacy capability checks.
    #define CONNECTION_POOLING REPLICATION
#endif
#else
// Minimal compatibility shim: enough types to satisfy ThemisDB compile-time
// dependencies. This is intentionally small — the real CHIMERA API provides
// a much richer surface. Do not rely on this shim for full functionality.

#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <future>

namespace chimera {

enum class ErrorCode {
    SUCCESS = 0,
    NOT_IMPLEMENTED = 1,
    INVALID_ARGUMENT = 2,
    NOT_FOUND = 3,
    ALREADY_EXISTS = 4,
    PERMISSION_DENIED = 5,
    CONNECTION_ERROR = 6,
    TIMEOUT = 7,
    RESOURCE_EXHAUSTED = 8,
    INTERNAL_ERROR = 9,
    UNSUPPORTED = 10,
    TRANSACTION_ABORTED = 11,
    CONSTRAINT_VIOLATION = 12,
    DEADLOCK = 13,
    /// Adapter dispatch failed: query/command could not be forwarded to the
    /// backend engine.  Non-retryable unless the underlying cause is transient.
    DISPATCH_FAILED = 14,
    /// Capability mismatch: the adapter does not support a feature required by
    /// the caller (e.g., vector search on a relational-only backend).
    /// Non-retryable; the caller must reconfigure or select a different adapter.
    CAPABILITY_MISMATCH = 15
};

// Minimal capability enum for shim mode is defined below when the external
// header is not available. When the external header *is* present we map the
// legacy STREAMING_RESULTS token to the newer STREAM_PROCESSING via macro
// above so existing code continues to compile.

template<typename T>
struct Result {
    std::optional<T> value;
    ErrorCode error_code = ErrorCode::SUCCESS;
    std::string error_message;

    bool is_ok() const { return error_code == ErrorCode::SUCCESS; }
    bool is_err() const { return error_code != ErrorCode::SUCCESS; }

    static Result<T> ok(T v) { return Result<T>{std::optional<T>(std::move(v)), ErrorCode::SUCCESS, ""}; }
    static Result<T> err(ErrorCode c, std::string m) { return Result<T>{std::nullopt, c, std::move(m)}; }
};

using Scalar = std::variant<
    std::monostate,
    bool,
    int64_t,
    double,
    std::string,
    std::vector<uint8_t>
>;

struct RelationalRow { std::vector<Scalar> columns; };
struct RelationalTable { std::vector<RelationalRow> rows; };
struct QueryStatistics { double duration_ms = 0.0; };

} // namespace chimera

#endif

// ---------------------------------------------------------------------------
// Extended interfaces: Streaming and Prepared Statements
// These were added after the external submodule was frozen at v0.0.37.
// They are defined here in the main-repo shim so that ThemisDBAdapter and
// the CHIMERA test suite can use them without requiring a submodule update.
// ---------------------------------------------------------------------------

#ifndef CHIMERA_STREAMING_PREPARED_INTERFACES_HPP
#define CHIMERA_STREAMING_PREPARED_INTERFACES_HPP

#include <optional>

namespace chimera {

/**
 * @struct StreamConfig
 * @brief Configuration hints for server-side cursor / streaming execution.
 */
struct StreamConfig {
    size_t default_batch_size = 1000; ///< Rows fetched per network round-trip
    size_t prefetch           = 2;    ///< Number of batches to prefetch
    uint32_t timeout_ms       = 30000;///< Per-batch fetch timeout (milliseconds)
};

/**
 * @class IResultStream
 * @brief Cursor interface for streaming large result sets row-by-row.
 */
class IResultStream {
public:
    virtual ~IResultStream() = default;

    /// Returns true while there are more rows to fetch.
    virtual bool has_more() const = 0;

    /// Fetch the next batch of rows (up to @p batch_size).
    virtual Result<std::vector<RelationalRow>> next_batch(
        size_t batch_size = 0
    ) = 0;

    /// Zero-based index of the next row that will be returned.
    virtual size_t position() const = 0;

    /// Total number of rows if known, nullopt otherwise.
    virtual std::optional<size_t> total_size() const = 0;

    /// Release server-side cursor resources.
    virtual Result<bool> close() = 0;
};

/**
 * @class IStreamingAdapter
 * @brief Mixin that adds streaming query execution to an adapter.
 */
class IStreamingAdapter {
public:
    virtual ~IStreamingAdapter() = default;

    /// Execute a query and return a streaming cursor.
    virtual Result<std::unique_ptr<IResultStream>> execute_query_stream(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;

    /// Update the stream configuration for subsequent stream queries.
    virtual Result<bool> set_stream_config(const StreamConfig& config) = 0;
};

/**
 * @class IPreparedStatement
 * @brief Handle for a server-side prepared (pre-parsed) statement.
 */
class IPreparedStatement {
public:
    virtual ~IPreparedStatement() = default;

    /// Opaque server-side statement identifier (UUID).
    virtual std::string get_id() const = 0;

    /// Original query text passed to prepare().
    virtual std::string get_query() const = 0;

    /// Bind a named parameter (e.g. @name tokens).
    virtual Result<bool> bind(const std::string& name, const Scalar& value) = 0;

    /// Bind a positional parameter (1-based index).
    virtual Result<bool> bind(size_t position, const Scalar& value) = 0;

    /// Bind all named parameters at once.
    virtual Result<bool> bind_all(
        const std::map<std::string, Scalar>& params
    ) = 0;

    /// Execute with currently bound parameters and return the result table.
    virtual Result<RelationalTable> execute() = 0;

    /// Asynchronous variant of execute().
    virtual std::future<Result<RelationalTable>> execute_async() = 0;

    /// Clear all bound parameters for re-use with different values.
    virtual Result<bool> reset() = 0;

    /// Accumulated execution statistics for this statement.
    virtual Result<QueryStatistics> get_statistics() const = 0;
};

/**
 * @class IPreparedStatementAdapter
 * @brief Mixin that adds prepared-statement support to an adapter.
 */
class IPreparedStatementAdapter {
public:
    virtual ~IPreparedStatementAdapter() = default;

    /// Parse and cache a query; return a statement handle.
    virtual Result<std::unique_ptr<IPreparedStatement>> prepare(
        const std::string& query
    ) = 0;

    /// Release a cached prepared statement by its ID.
    virtual Result<bool> unprepare(const std::string& statement_id) = 0;

    /// List all currently prepared statement IDs.
    virtual Result<std::vector<std::string>> list_prepared() = 0;
};

} // namespace chimera

#endif // CHIMERA_STREAMING_PREPARED_INTERFACES_HPP
