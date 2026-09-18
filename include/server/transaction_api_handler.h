/**
 * @file transaction_api_handler.h
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
class TransactionManager;

namespace server {

class TransactionApiHandler {
public:
    TransactionApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Transaction.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTransaction(const http::request<http::string_body>& req);

    /**
     * @brief Handle Begin.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBegin(const http::request<http::string_body>& req);

    /**
     * @brief Handle Commit.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCommit(const http::request<http::string_body>& req);

    /**
     * @brief Handle Rollback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRollback(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Version.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetVersion(const http::request<http::string_body>& req);

    /**
     * @brief Handle Explain.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleExplain(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
