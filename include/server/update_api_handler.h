/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            update_api_handler.h                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
     */
    http::response<http::string_body> handleGetStatus(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief POST /api/updates/check - Trigger manual check
     */
    http::response<http::string_body> handleCheckNow(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief GET /api/updates/config - Get configuration
     */
    http::response<http::string_body> handleGetConfig(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief PUT /api/updates/config - Update configuration
     */
    http::response<http::string_body> handleUpdateConfig(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create JSON response
     */
    http::response<http::string_body> createJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create error response
     */
    http::response<http::string_body> createErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );
};

} // namespace server
} // namespace themis
