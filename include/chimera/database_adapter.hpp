/**
 * @file database_adapter.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    DISPATCH_FAILED = 14,
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

    /**
     * @brief Ok.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static Result<T> ok(T v) { return Result<T>{std::optional<T>(std::move(v)), ErrorCode::SUCCESS, ""}; }
    /**
     * @brief Err.
     * @param[in] c Input parameter.
     * @param[in] m Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
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

struct StreamConfig {
    size_t default_batch_size = 1000; ///< Rows fetched per network round-trip
    size_t prefetch           = 2;    ///< Number of batches to prefetch
    uint32_t timeout_ms       = 30000;///< Per-batch fetch timeout (milliseconds)
};

class IResultStream {
public:
    /**
     * @brief IResult Stream.
     * @return Return value.
     */
    virtual ~IResultStream() = default;

    /**
     * @brief Has more.
     * @return True when the operation succeeds.
     */
    virtual bool has_more() const = 0;

    virtual Result<std::vector<RelationalRow>> next_batch(
        size_t batch_size = 0
    ) = 0;

    /**
     * @brief Position.
     * @return Return value.
     */
    virtual size_t position() const = 0;

    /**
     * @brief Total size.
     * @return Return value.
     */
    virtual std::optional<size_t> total_size() const = 0;

    /**
     * @brief Close.
     * @return Return value.
     */
    virtual Result<bool> close() = 0;
};

class IStreamingAdapter {
public:
    /**
     * @brief IStreaming Adapter.
     * @return Return value.
     */
    virtual ~IStreamingAdapter() = default;

    virtual Result<std::unique_ptr<IResultStream>> execute_query_stream(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;

    /**
     * @brief Set stream config.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual Result<bool> set_stream_config(const StreamConfig& config) = 0;
};

class IPreparedStatement {
public:
    /**
     * @brief IPrepared Statement.
     * @return Return value.
     */
    virtual ~IPreparedStatement() = default;

    /**
     * @brief Get id.
     * @return Return value.
     */
    virtual std::string get_id() const = 0;

    /**
     * @brief Get query.
     * @return Return value.
     */
    virtual std::string get_query() const = 0;

    /**
     * @brief Bind.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    virtual Result<bool> bind(const std::string& name, const Scalar& value) = 0;

    /**
     * @brief Bind.
     * @param[in] position Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    virtual Result<bool> bind(size_t position, const Scalar& value) = 0;

    virtual Result<bool> bind_all(
        const std::map<std::string, Scalar>& params
    ) = 0;

    /**
     * @brief Execute.
     * @return Return value.
     */
    virtual Result<RelationalTable> execute() = 0;

    /**
     * @brief Execute async.
     * @return Return value.
     */
    virtual std::future<Result<RelationalTable>> execute_async() = 0;

    /**
     * @brief Reset the modification detection flag.
     * @return None.
     */
    virtual Result<bool> reset() = 0;

    /**
     * @brief Get statistics.
     * @return Return value.
     */
    virtual Result<QueryStatistics> get_statistics() const = 0;
};

class IPreparedStatementAdapter {
public:
    /**
     * @brief IPrepared Statement Adapter.
     * @return Return value.
     */
    virtual ~IPreparedStatementAdapter() = default;

    /**
     * @brief Prepare.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    virtual Result<std::unique_ptr<IPreparedStatement>> prepare(
        const std::string& query
    ) = 0;

    /**
     * @brief Unprepare.
     * @param[in] statement_id Identifier of the statement.
     * @return Return value.
     */
    virtual Result<bool> unprepare(const std::string& statement_id) = 0;

    /**
     * @brief List prepared.
     * @return Return value.
     */
    virtual Result<std::vector<std::string>> list_prepared() = 0;
};

} // namespace chimera

#endif // CHIMERA_STREAMING_PREPARED_INTERFACES_HPP
