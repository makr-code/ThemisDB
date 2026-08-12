/**
 * @file udf_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief HTTP API Handler for AQL User-Defined Function (UDF) registration.
 *
 * Exposes a RESTful interface for managing user-defined functions that are
 * callable from AQL queries via the global FunctionRegistry:
 *
 *   POST   /api/v1/query/udfs        – register or replace a UDF
 *   GET    /api/v1/query/udfs        – list all registered UDFs
 *   GET    /api/v1/query/udfs/{name} – get a single UDF definition
 *   DELETE /api/v1/query/udfs/{name} – unregister a UDF
 *
 * ### UDF request body (POST)
 * ```json
 * {
 *   "name":             "MY_FUNC",
 *   "description":      "optional description",
 *   "arguments": [
 *     {"name": "input", "type": "STRING", "required": true}
 *   ],
 *   "return_type":      "STRING",
 *   "is_deterministic": true,
 *   "body": {
 *     "type": "call",
 *     "function": "UPPER",
 *     "args": [{"type": "arg", "index": 0}]
 *   }
 * }
 * ```
 *
 * See `include/query/functions/udf_registry.h` for the full body expression DSL.
 *
 * @note After registration the function is immediately available in AQL:
 *       `RETURN MY_FUNC("hello")`
 */
class UdfApiHandler {
public:
    UdfApiHandler() = default;

    /** POST /api/v1/query/udfs – register (or replace) a UDF. */
    http::response<http::string_body> handleRegister(
        const http::request<http::string_body>& req);

    /** GET /api/v1/query/udfs – list all registered UDFs. */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /** GET /api/v1/query/udfs/{name} – get a single UDF definition. */
    http::response<http::string_body> handleGet(
        const http::request<http::string_body>& req,
        const std::string& name);

    /** DELETE /api/v1/query/udfs/{name} – unregister a UDF. */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string& name);

private:
    http::response<http::string_body> makeJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req) const;

    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis
