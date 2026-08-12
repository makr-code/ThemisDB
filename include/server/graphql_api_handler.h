/**
 * @file graphql_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "api/graphql.h"
#include "api/graphql_aql_resolver.h"
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {
namespace query { class QueryEngine; }
using QueryEngine = query::QueryEngine;
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
 * When constructed with a QueryEngine pointer the handler injects AQL
 * resolvers into every ExecutionContext before execution, enabling:
 *   query  { aql(query: "FOR d IN docs RETURN d") }
 *   mutation { aqlMutation(query: "INSERT {...} INTO docs") }
 *   query  { apiVersion schemaVersion }
 *
 * The injected resolvers apply the GraphQL complexity → AQL cost model
 * bridge (GraphQLComplexityEstimator + QueryResourceLimits) so that the
 * same resource constraints that govern plain AQL queries also govern AQL
 * sub-queries embedded inside GraphQL operations.
 *
 * @note Thread-safe – all public methods are stateless and may be called
 *       concurrently from different I/O threads.  The QueryEngine pointer
 *       must remain valid for the lifetime of this object.
 */
class GraphQLApiHandler {
public:
    /**
     * @brief Default constructor – no AQL engine wired.
     *
     * The `aql`, `aqlMutation`, `apiVersion`, and `schemaVersion` resolvers
     * are still registered but `aql` / `aqlMutation` return null when no
     * engine is available.  Useful for pure schema inspection.
     */
    GraphQLApiHandler() = default;

    /**
     * @brief Construct with a QueryEngine for full AQL resolver support.
     *
     * @param engine  Non-owning pointer to the AQL query engine.
     *                The engine must outlive this handler.
     */
    explicit GraphQLApiHandler(QueryEngine* engine) : engine_(engine) {}

    /**
     * @brief Inject or replace the QueryEngine after construction.
     *
     * Thread-safe only if called before the handler starts serving requests.
     */
    void setQueryEngine(QueryEngine* engine) { engine_ = engine; }

    /**
     * @brief Handle a GraphQL request body.
     *
     * Expects a JSON body with at least a @c "query" field:
     * @code
     * {
     *   "query":         "{ apiVersion }",
     *   "variables":     {},               // optional
     *   "operationName": "GetVersion"      // optional
     * }
     * @endcode
     *
     * Returns a standard GraphQL response envelope:
     * @code
     * { "data": { ... }, "errors": [ ... ] }
     * @endcode
     *
     * The complexity of the GraphQL document is scored before execution.
     * Queries exceeding the complexity budget are rejected with HTTP 400.
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
    /// Non-owning pointer to the AQL engine; nullptr = no AQL resolver.
    QueryEngine* engine_ = nullptr;

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
