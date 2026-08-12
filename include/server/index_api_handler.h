/**
 * @file index_api_handler.h
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
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class AdaptiveIndexManager;

namespace server {

/**
 * @brief Handler for Index Management Operations
 * 
 * This handler manages all index-related endpoints:
 * - POST /index/create - Create a secondary index
 * - POST /index/drop - Drop an existing index
 * - GET /index/stats - Get index statistics
 * - POST /index/rebuild - Rebuild an index
 * - POST /index/reindex - Reindex existing data
 * - GET /index/suggestions - Get adaptive index suggestions
 * - GET /index/patterns - Get query patterns for optimization
 * - POST /index/record-pattern - Record a query pattern
 * - DELETE /index/patterns - Clear recorded patterns
 * 
 * Features:
 * - Secondary index management
 * - Index statistics and monitoring
 * - Adaptive indexing suggestions
 * - Query pattern analysis
 * - Index rebuilding and reindexing
 * 
 * Extracted from http_server.cpp (~400 lines) to improve maintainability.
 */
class IndexApiHandler {
public:
    /**
     * @brief Construct a new Index API Handler
     * 
     * @param storage Storage backend
     * @param secondary_index Secondary index manager
     * @param adaptive_index Adaptive index manager for suggestions
     * @param auth Authentication/authorization middleware
     */
    IndexApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<AdaptiveIndexManager> adaptive_index,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /index/create request
     * @param req HTTP request with index specification
     * @return HTTP response with creation status
     */
    http::response<http::string_body> handleCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /index/drop request
     * @param req HTTP request with index name
     * @return HTTP response with deletion status
     */
    http::response<http::string_body> handleDrop(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /index/stats request
     * @param req HTTP request
     * @return HTTP response with index statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /index/rebuild request
     * @param req HTTP request with index name
     * @return HTTP response with rebuild status
     */
    http::response<http::string_body> handleRebuild(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /index/reindex request
     * @param req HTTP request with reindex specification
     * @return HTTP response with reindex status
     */
    http::response<http::string_body> handleReindex(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /index/suggestions request
     * @param req HTTP request
     * @return HTTP response with adaptive index suggestions
     */
    http::response<http::string_body> handleSuggestions(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /index/patterns request
     * @param req HTTP request
     * @return HTTP response with recorded query patterns
     */
    http::response<http::string_body> handlePatterns(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /index/record-pattern request
     * @param req HTTP request with pattern to record
     * @return HTTP response with recording status
     */
    http::response<http::string_body> handleRecordPattern(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /index/patterns request
     * @param req HTTP request
     * @return HTTP response with clear status
     */
    http::response<http::string_body> handleClearPatterns(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<AdaptiveIndexManager> adaptive_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
