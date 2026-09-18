/**
 * @file update_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include "utils/update_checker.h"

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief HTTP API Handler for Update Checker endpoints
 * 
 * Provides REST endpoints for:
 * - GET  /api/updates        - Get current update status
 * - POST /api/updates/check  - Trigger manual update check
 * - GET  /api/updates/config - Get update checker configuration
 * - PUT  /api/updates/config - Update configuration
 * 
 * Future:
 * - POST /api/updates/apply  - Apply update (hot-reload)
 */
class UpdateApiHandler {
public:
    /**
     * @brief Construct handler with update checker instance
     * @param[in] checker Input parameter.
     * @return Return value.
     */
    explicit UpdateApiHandler(std::shared_ptr<utils::UpdateChecker> checker);
    
    /**
     * @brief Handle update-related HTTP requests
     * @param req HTTP request
     * @return HTTP response
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<utils::UpdateChecker> checker_;
    
    /**
     * @brief GET /api/updates - Get update status
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStatus(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief POST /api/updates/check - Trigger manual check
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCheckNow(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief GET /api/updates/config - Get configuration
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetConfig(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief PUT /api/updates/config - Update configuration
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateConfig(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create JSON response
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> createJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create error response
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> createErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );
};

} // namespace server
} // namespace themis
