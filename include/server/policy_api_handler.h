/**
 * @file policy_api_handler.h
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
class PolicyEngine;

namespace server {
class RangerClient;

class PolicyApiHandler {
public:
    PolicyApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        RangerClient* ranger_client,
        PolicyEngine* policy_engine,
        std::shared_ptr<themis::AuthMiddleware> auth,
        const std::string& service_name = "themisdb"
    );

    /**
     * @brief Handle Import Ranger.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleImportRanger(const http::request<http::string_body>& req);

    /**
     * @brief Handle Export Ranger.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleExportRanger(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    RangerClient* ranger_client_;
    PolicyEngine* policy_engine_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    std::string service_name_;

    // Helper methods
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
