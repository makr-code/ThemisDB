/**
 * @file udf_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

     * @brief TBD: Describe handleRegister.
     * @param[in] req Input parameter.
     * @return Return value.
    /** POST /api/v1/query/udfs – register (or replace) a UDF. */
    http::response<http::string_body> handleRegister(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleList.
     * @param[in] req Input parameter.
     * @return Return value.
    /** GET /api/v1/query/udfs – list all registered UDFs. */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleGet.
     * @param[in] req Input parameter.
     * @param[in] name Input parameter.
     * @return Return value.
    /** GET /api/v1/query/udfs/{name} – get a single UDF definition. */
    http::response<http::string_body> handleGet(
        const http::request<http::string_body>& req,
        const std::string& name);

     * @brief TBD: Describe handleDelete.
     * @param[in] req Input parameter.
     * @param[in] name Input parameter.
     * @return Return value.
    /** DELETE /api/v1/query/udfs/{name} – unregister a UDF. */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string& name);

private:
    /**
     * @brief TBD: Describe makeJsonResponse.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req) const;

    /**
     * @brief TBD: Describe makeErrorResponse.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis
