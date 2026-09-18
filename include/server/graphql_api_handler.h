/**
 * @file graphql_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class GraphQLApiHandler {
public:
    GraphQLApiHandler() = default;

    explicit GraphQLApiHandler(QueryEngine* engine) : engine_(engine) {}

    /**
     * @brief Set Query Engine.
     * @param[in,out] engine Input/output parameter.
     * @details Implements setQueryEngine without additional internal calls.
     */
    void setQueryEngine(QueryEngine* engine) { engine_ = engine; }

    /**
     * @brief Handle Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePost(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Schema Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaGet(
        const http::request<http::string_body>& req);

private:
    QueryEngine* engine_ = nullptr;

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req);

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);

    /**
     * @brief Serialize Value.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    nlohmann::json serializeValue(
        const std::shared_ptr<graphql::Value>& val) const;
};

} // namespace server
} // namespace themis
