/**
 * @file hot_reload_api_handler.h
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
#include "updates/hot_reload_engine.h"
#include "updates/manifest_database.h"

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class HotReloadApiHandler {
public:
    HotReloadApiHandler(
        std::shared_ptr<updates::ManifestDatabase> manifest_db,
        std::shared_ptr<updates::HotReloadEngine> reload_engine
    );
    
    /**
     * @brief Handle Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<updates::ManifestDatabase> manifest_db_;
    std::shared_ptr<updates::HotReloadEngine> reload_engine_;
    
    /**
     * @brief Handle Get Manifest.
     * @param[in] req Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetManifest(
        const http::request<http::string_body>& req,
        const std::string& version
    );
    
    /**
     * @brief Handle Download.
     * @param[in] req Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDownload(
        const http::request<http::string_body>& req,
        const std::string& version
    );
    
    /**
     * @brief Handle Apply.
     * @param[in] req Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleApply(
        const http::request<http::string_body>& req,
        const std::string& version
    );
    
    /**
     * @brief Handle Rollback.
     * @param[in] req Input parameter.
     * @param[in] rollback_id Identifier of the rollback.
     * @return Return value.
     */
    http::response<http::string_body> handleRollback(
        const http::request<http::string_body>& req,
        const std::string& rollback_id
    );
    
    /**
     * @brief Handle List Rollbacks.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListRollbacks(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create Json Response.
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
     * @brief Create Error Response.
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
    
    /**
     * @brief Extract Path Param.
     * @param[in] path Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractPathParam(const std::string& path, const std::string& prefix);
};

} // namespace server
} // namespace themis
