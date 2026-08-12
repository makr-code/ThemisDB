/**
 * @file spatial_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace index {
class SpatialIndexManager;
}

namespace server {

/**
 * @brief Handler for Spatial Index Operations
 * 
 * This handler manages all spatial index-related endpoints:
 * - POST /spatial/index/create - Create a spatial index
 * - POST /spatial/index/rebuild - Rebuild a spatial index
 * - GET /spatial/index/stats - Get spatial index statistics
 * - GET /spatial/metrics - Get spatial query metrics
 * 
 * Features:
 * - R-tree based spatial indexing
 * - Geospatial query support
 * - Index statistics and monitoring
 * - Performance metrics
 * 
 * Extracted from http_server.cpp (~200 lines) to improve maintainability.
 */
class SpatialApiHandler {
public:
    /**
     * @brief Construct a new Spatial API Handler
     * 
     * @param storage Storage backend
     * @param spatial_index Spatial index manager
     * @param auth Authentication/authorization middleware
     */
    SpatialApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<index::SpatialIndexManager> spatial_index,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /spatial/index/create request
     * @param req HTTP request with spatial index specification
     * @return HTTP response with creation status
     */
    http::response<http::string_body> handleIndexCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /spatial/index/rebuild request
     * @param req HTTP request with index name
     * @return HTTP response with rebuild status
     */
    http::response<http::string_body> handleIndexRebuild(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /spatial/index/stats request
     * @param req HTTP request
     * @return HTTP response with spatial index statistics
     */
    http::response<http::string_body> handleIndexStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /spatial/metrics request
     * @param req HTTP request
     * @return HTTP response with spatial query metrics
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<index::SpatialIndexManager> spatial_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Query parameter parsing
    std::unordered_map<std::string, std::string> parseQuery(const std::string& target);
    std::string urlDecode(const std::string& str);
};

} // namespace server
} // namespace themis
