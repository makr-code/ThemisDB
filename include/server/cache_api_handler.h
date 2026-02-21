/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_api_handler.h                                ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:37:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     106                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2d90acb6a  2026-01-15  Add build scripts and documentation generation tools ║
    • 612b5bf41  2026-01-12  Refactor http_server.cpp: Complete foundation with struct... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {

// Forward declarations
class SemanticCache;

namespace server {

/**
 * @brief Handler for Cache Operations
 * 
 * This handler manages all cache-related endpoints:
 * - POST /cache/query - Query semantic cache
 * - POST /cache/put - Store query result in cache
 * - GET /cache/stats - Get cache statistics
 * 
 * Features:
 * - Semantic query caching
 * - Vector similarity-based cache lookup
 * - Cache hit/miss statistics
 * - TTL management
 * 
 * Extracted from http_server.cpp (~200 lines) to improve maintainability.
 */
class CacheApiHandler {
public:
    /**
     * @brief Construct a new Cache API Handler
     * 
     * @param semantic_cache Semantic cache instance
     * @param auth Authentication/authorization middleware
     */
    CacheApiHandler(
        std::shared_ptr<SemanticCache> semantic_cache,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /cache/query request
     * @param req HTTP request with query to lookup in cache
     * @return HTTP response with cached result or miss indicator
     */
    http::response<http::string_body> handleQuery(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /cache/put request
     * @param req HTTP request with query and result to cache
     * @return HTTP response with cache status
     */
    http::response<http::string_body> handlePut(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /cache/stats request
     * @param req HTTP request
     * @return HTTP response with cache statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

private:
    std::shared_ptr<SemanticCache> semantic_cache_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
