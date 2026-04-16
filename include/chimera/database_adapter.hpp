/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            database_adapter.hpp                               ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 18:44:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     29                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 29ac1cf537  2026-04-14  fix                                     ║
    • fd7d928faf  2026-04-10  refactor(chimera): extract generic chimera assets into su... ║
    • 04a46f63a9  2026-03-12  fix(chimera): address PR review comments on multi-databas... ║
    • 3bd2167e65  2026-03-12  feat(chimera): implement multi-database adapter registrat... ║
    • 16eb8c2a4c  2026-03-12  fix(chimera): address async API review comments (RAII cle... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "../../external/chimera/include/chimera/database_adapter.hpp"

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
