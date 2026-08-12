/**
 * @file hot_reload_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include "updates/hot_reload_engine.h"
#include "updates/manifest_database.h"

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief HTTP API Handler for Hot-Reload endpoints
 * 
 * Provides REST endpoints for:
 * - GET  /api/updates/manifests/:version   - Get manifest for version
 * - POST /api/updates/download/:version    - Download release
 * - POST /api/updates/apply/:version       - Apply hot-reload (requires admin)
 * - POST /api/updates/rollback/:id         - Rollback to previous version (requires admin)
 * - GET  /api/updates/rollback             - List rollback points
 */
class HotReloadApiHandler {
public:
    /**
     * @brief Construct handler
     */
    HotReloadApiHandler(
        std::shared_ptr<updates::ManifestDatabase> manifest_db,
        std::shared_ptr<updates::HotReloadEngine> reload_engine
    );
    
    /**
     * @brief Handle hot-reload HTTP requests
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<updates::ManifestDatabase> manifest_db_;
    std::shared_ptr<updates::HotReloadEngine> reload_engine_;
    
    /**
     * @brief GET /api/updates/manifests/:version
     */
    http::response<http::string_body> handleGetManifest(
        const http::request<http::string_body>& req,
        const std::string& version
    );
    
    /**
     * @brief POST /api/updates/download/:version
     */
    http::response<http::string_body> handleDownload(
        const http::request<http::string_body>& req,
        const std::string& version
    );
    
    /**
     * @brief POST /api/updates/apply/:version
     */
    http::response<http::string_body> handleApply(
        const http::request<http::string_body>& req,
        const std::string& version
    );
    
    /**
     * @brief POST /api/updates/rollback/:id
     */
    http::response<http::string_body> handleRollback(
        const http::request<http::string_body>& req,
        const std::string& rollback_id
    );
    
    /**
     * @brief GET /api/updates/rollback
     */
    http::response<http::string_body> handleListRollbacks(
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
    
    /**
     * @brief Extract path parameter (e.g., version from /api/updates/apply/:version)
     */
    std::string extractPathParam(const std::string& path, const std::string& prefix);
};

} // namespace server
} // namespace themis
