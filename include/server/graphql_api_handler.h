#pragma once

#include "api/graphql.h"
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief Handler for the GraphQL endpoint.
 *
 * Exposes a schema-driven query interface over HTTP:
 *   - POST /graphql             – execute a GraphQL query or mutation
 *   - POST /api/v1/graphql      – versioned alias
 *   - GET  /graphql/schema      – retrieve the SDL schema
 *   - GET  /api/v1/graphql/schema – versioned alias
 *
 * The handler is intentionally dependency-free: the GraphQL parser,
 * executor, and schema builder are all self-contained in the api/graphql
 * library and do not require access to the storage layer at this stage.
 * Resolvers that need database access can be registered on the
 * @c graphql::ExecutionContext before execution.
 *
 * @note Thread-safe – all public methods are stateless and may be called
 *       concurrently from different I/O threads.
 */
class GraphQLApiHandler {
public:
    GraphQLApiHandler() = default;

    /**
     * @brief Handle a GraphQL request body.
     *
     * Expects a JSON body with at least a @c "query" field:
     * @code
     * {
     *   "query":         "{ user { id name } }",
     *   "variables":     { "id": "123" },       // optional
     *   "operationName": "GetUser"               // optional
     * }
     * @endcode
     *
     * Returns a standard GraphQL response envelope:
     * @code
     * { "data": { ... }, "errors": [ ... ] }
     * @endcode
     *
     * @param req Incoming HTTP request.
     * @return HTTP 200 with GraphQL JSON response, or 4xx on invalid input.
     */
    http::response<http::string_body> handlePost(
        const http::request<http::string_body>& req);

    /**
     * @brief Return the GraphQL Schema Definition Language (SDL) document.
     *
     * @param req Incoming HTTP request.
     * @return HTTP 200 with SDL text body (Content-Type: text/plain).
     */
    http::response<http::string_body> handleSchemaGet(
        const http::request<http::string_body>& req);

private:
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req);

    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);

    /** Serialize a graphql::Value tree to a nlohmann::json node. */
    nlohmann::json serializeValue(
        const std::shared_ptr<graphql::Value>& val) const;
};

} // namespace server
} // namespace themis
