/**
 * @file ethics_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
namespace query { class QueryEngine; }
using QueryEngine = query::QueryEngine;

namespace server {

class EthicsApiHandler {
public:
    EthicsApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<QueryEngine> query_engine,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle.
     * @param[in] req Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string& target);

    /**
     * @brief Handle Debate Init.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDebateInit(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Make Decision.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMakeDecision(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Evaluation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEvaluation(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Arguments.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetArguments(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Search Arguments.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSearchArguments(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle List Philosophies.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListPhilosophies(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Philosophy.
     * @param[in] req Input parameter.
     * @param[in] school Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetPhilosophy(
        const http::request<http::string_body>& req,
        const std::string& school);

    /**
     * @brief Handle Build Context.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBuildContext(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetMetrics(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<QueryEngine> query_engine_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
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
        
    nlohmann::json executeAQL(
        const std::string& aql_query,
        const nlohmann::json& bind_vars = nlohmann::json::object());
        
    /**
     * @brief Extract Query Param.
     * @param[in] target Input parameter.
     * @param[in] param Input parameter.
     * @return Return value.
     */
    std::string extractQueryParam(
        const std::string& target,
        const std::string& param) const;
};

} // namespace server
} // namespace themis
