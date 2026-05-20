/*
 * ThemisDB | File: rope_api_handler.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class VectorIndexManager;
class AuthMiddleware;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Handler for Rotary Position Embeddings (RoPE) Operations
 * 
 * This handler manages all RoPE-related endpoints:
 * - POST /api/v1/vector-index/{index_name}/rope/config - Configure RoPE
 * - GET /api/v1/vector-index/{index_name}/rope/config - Get RoPE configuration
 * - DELETE /api/v1/vector-index/{index_name}/rope/config - Disable RoPE
 * - POST /api/v1/vector-index/{index_name}/rope/add - Add entity with rotation
 * - POST /api/v1/vector-index/{index_name}/rope/add-relational - Add with relational rotation
 * - POST /api/v1/vector-index/{index_name}/rope/search - Search with rotation
 * - POST /api/v1/vector-index/{index_name}/rope/batch-add - Batch add with rotation
 * - GET /api/v1/vector-index/{index_name}/rope/stats - Get RoPE statistics
 * 
 * Features:
 * - Rotary position embeddings for sequential data
 * - Relational embeddings for knowledge graphs
 * - Batch operations for efficiency
 * - Configuration management
 * - Statistics and monitoring
 */
class RopeApiHandler {
public:
    /**
     * @brief Construct a new RoPE API Handler
     * 
     * @param storage Storage backend
     * @param vector_index Vector index manager with RoPE support
     * @param auth Authentication/authorization middleware
     */
    RopeApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<::themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /api/v1/vector-index/{index_name}/rope/config
     * @param req HTTP request with RoPE configuration
     * @return HTTP response with configuration status
     */
    http::response<http::string_body> handleConfigPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/vector-index/{index_name}/rope/config
     * @param req HTTP request
     * @return HTTP response with current RoPE configuration
     */
    http::response<http::string_body> handleConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /api/v1/vector-index/{index_name}/rope/config
     * @param req HTTP request
     * @return HTTP response with disable status
     */
    http::response<http::string_body> handleConfigDelete(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/vector-index/{index_name}/rope/add
     * @param req HTTP request with entity and position
     * @return HTTP response with add status
     */
    http::response<http::string_body> handleAddPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/vector-index/{index_name}/rope/add-relational
     * @param req HTTP request with entity and relation type
     * @return HTTP response with add status
     */
    http::response<http::string_body> handleAddRelationalPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/vector-index/{index_name}/rope/search
     * @param req HTTP request with query vector and position
     * @return HTTP response with search results
     */
    http::response<http::string_body> handleSearchPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/vector-index/{index_name}/rope/batch-add
     * @param req HTTP request with batch of entities
     * @return HTTP response with batch add status
     */
    http::response<http::string_body> handleBatchAddPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/vector-index/{index_name}/rope/stats
     * @param req HTTP request
     * @return HTTP response with RoPE statistics
     */
    http::response<http::string_body> handleStatsGet(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<::themis::AuthMiddleware> auth_;

    // Helper methods
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& permission,
        const std::string& resource,
        const std::string& path);
    
    // Extract index_name from path like /api/v1/vector-index/{index_name}/rope/...
    std::optional<std::string> extractIndexName(const std::string& path);
};

} // namespace server
} // namespace themis
