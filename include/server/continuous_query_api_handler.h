/**
 * @file continuous_query_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/continuous_query_engine.h"
#include "query/continuous_query_registry.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <string>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief REST and SSE handler for standing continuous-query endpoints.
 *
 * Endpoints:
 *   POST   /v1/queries/continuous            – register a new query
 *   DELETE /v1/queries/continuous/:name      – drop a registered query
 *   GET    /v1/queries/continuous            – list all queries (JSON array)
 *   GET    /v1/queries/continuous/:name/results – SSE stream of CQResult deltas
 *
 * All methods are thread-safe: the handler itself is stateless and delegates
 * state management to the injected ContinuousQueryEngine.
 */
class ContinuousQueryApiHandler {
public:
    /**
     * @brief Construct a handler backed by the given engine.
     *
     * @param engine  Shared ContinuousQueryEngine instance; must not be null.
     */
    explicit ContinuousQueryApiHandler(
        std::shared_ptr<themis::query::ContinuousQueryEngine> engine);

    ~ContinuousQueryApiHandler() = default;

    // Non-copyable, movable
    ContinuousQueryApiHandler(const ContinuousQueryApiHandler&) = delete;
    ContinuousQueryApiHandler& operator=(const ContinuousQueryApiHandler&) = delete;
    ContinuousQueryApiHandler(ContinuousQueryApiHandler&&) = default;
    ContinuousQueryApiHandler& operator=(ContinuousQueryApiHandler&&) = default;

    /**
     * @brief POST /v1/queries/continuous
     *
     * Request body (JSON):
     * @code
     * {
     *   "name":               "my_query",
     *   "source_collection":  "events",
     *   "window": {
     *     "type":     "TIME_SLIDING" | "COUNT_SLIDING" | "TUMBLING",
     *     "range_ms": 60000,
     *     "slide_ms": 1000,
     *     "rows":     1000,
     *     "slide_rows": 100,
     *     "partition_by": ""
     *   },
     *   "aql_body":           "FOR e IN events RETURN e",
     *   "result_mode":        "DELTA" | "SNAPSHOT" | "CHANGES",
     *   "allowed_lateness_ms": 500
     * }
     * @endcode
     *
     * @return 201 Created with `{"name": "<name>"}`, or 400/409 on error.
     */
    [[nodiscard]] http::response<http::string_body> handleRegister(
        const http::request<http::string_body>& req);

    /**
     * @brief DELETE /v1/queries/continuous/:name
     *
     * Drops the named query, cancels its evaluation loop, and drains all
     * subscriber queues.
     *
     * @param name  Query name extracted from the URL path.
     * @return 200 OK on success, 404 if not found.
     */
    [[nodiscard]] http::response<http::string_body> handleDrop(
        const http::request<http::string_body>& req,
        const std::string& name);

    /**
     * @brief GET /v1/queries/continuous
     *
     * Returns a JSON array of ContinuousQueryInfo objects describing all
     * registered queries with their runtime statistics.
     *
     * @return 200 OK with JSON array body.
     */
    [[nodiscard]] http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /**
     * @brief GET /v1/queries/continuous/:name/results  (SSE stream)
     *
     * Subscribes to the named query and streams results as Server-Sent Events
     * (Content-Type: text/event-stream).  Each event carries one CQResult:
     *
     * @code
     * data: {"payload":"...","is_retract":false}
     *
     * @endcode
     *
     * The stream continues until:
     *   - the subscription's hasMore() returns false (query dropped),
     *   - the per-poll timeout of 60 s elapses with no results (emits a
     *     heartbeat comment `": heartbeat"` to keep the connection alive), or
     *   - an unrecoverable internal error occurs.
     *
     * @param name   Query name extracted from the URL path.
     * @return HTTP response with Content-Type: text/event-stream.
     */
    [[nodiscard]] http::response<http::string_body> handleStreamSse(
        const http::request<http::string_body>& req,
        const std::string& name);

private:
    std::shared_ptr<themis::query::ContinuousQueryEngine> engine_;

    /// Build a simple JSON error response.
    [[nodiscard]] static http::response<http::string_body> makeError(
        http::status status, const std::string& message,
        const http::request<http::string_body>& req);

    /// Build a simple JSON success response.
    [[nodiscard]] static http::response<http::string_body> makeJson(
        http::status status, const std::string& body,
        const http::request<http::string_body>& req);

    /// Deserialise a WindowSpec from a JSON object; returns default on error.
    [[nodiscard]] static themis::query::WindowSpec windowFromJson(
        const nlohmann::json& j);

    /// Serialise a ContinuousQueryInfo to JSON.
    [[nodiscard]] static nlohmann::json infoToJson(
        const themis::query::ContinuousQueryInfo& info);
};

}  // namespace server
}  // namespace themis

