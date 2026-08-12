/**
 * @file vector_api_handler.h
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
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class VectorIndexManager;
class FieldEncryption;
class KeyProvider;
class AuthMiddleware;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

struct AuthContext {
    std::string user_id;
    std::string tenant_id;
    std::vector<std::string> roles;
    std::map<std::string, std::string> attributes;
};

/**
 * @brief Handler for Vector Operations
 * 
 * This handler manages all vector-related endpoints:
 * - POST /vector/search - Perform vector similarity search
 * - POST /vector/batch_insert - Batch insert vectors
 * - DELETE /vector/by-filter - Delete vectors by filter
 * - POST /vector/index/save - Save vector index to disk
 * - POST /vector/index/load - Load vector index from disk
 * - GET /vector/index/config - Get vector index configuration
 * - PUT /vector/index/config - Update vector index configuration
 * - GET /vector/index/stats - Get vector index statistics
 * - POST /vector/index/incremental-reindex - Incremental HNSW re-index without full rebuild
 * 
 * Features:
 * - HNSW-based vector similarity search
 * - GPU acceleration support (optional)
 * - Index persistence and loading
 * - Configuration management
 * - Performance statistics
 * 
 * Extracted from http_server.cpp (~450 lines) to improve maintainability.
 */
class VectorApiHandler {
public:
    /**
     * @brief Construct a new Vector API Handler
     * 
     * @param storage Storage backend
     * @param vector_index Vector index manager (HNSW/Faiss)
     * @param auth Authentication/authorization middleware
     * @param field_encryption Field-level encryption (optional)
     * @param key_provider Key provider for encryption (optional)
     */
    VectorApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<::themis::AuthMiddleware> auth,
        std::shared_ptr<FieldEncryption> field_encryption = nullptr,
        std::shared_ptr<KeyProvider> key_provider = nullptr
    );

    /**
     * @brief Handle POST /vector/search request
     * @param req HTTP request with query vector and search parameters
     * @return HTTP response with nearest neighbors
     */
    http::response<http::string_body> handleSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /vector/batch_insert request
     * @param req HTTP request with vectors to insert
     * @return HTTP response with insertion status
     */
    http::response<http::string_body> handleBatchInsert(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /vector/by-filter request
     * @param req HTTP request with filter criteria
     * @return HTTP response with deletion status
     */
    http::response<http::string_body> handleDeleteByFilter(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /vector/index/save request
     * @param req HTTP request with save path
     * @return HTTP response with save status
     */
    http::response<http::string_body> handleIndexSave(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /vector/index/load request
     * @param req HTTP request with load path
     * @return HTTP response with load status
     */
    http::response<http::string_body> handleIndexLoad(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /vector/index/config request
     * @param req HTTP request
     * @return HTTP response with current configuration
     */
    http::response<http::string_body> handleIndexConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /vector/index/config request
     * @param req HTTP request with new configuration
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleIndexConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /vector/index/stats request
     * @param req HTTP request
     * @return HTTP response with index statistics
     */
    http::response<http::string_body> handleIndexStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /vector/index/incremental-reindex request
     * 
     * Syncs the in-memory HNSW index with current storage without a full rebuild.
     * Accepts optional JSON body:
     *   { "rebuild_threshold": 0.20, "vector_field": "embedding" }
     * 
     * @param req HTTP request (body optional)
     * @return HTTP response with IncrementalReindexStats as JSON
     */
    http::response<http::string_body> handleIncrementalReindex(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<::themis::AuthMiddleware> auth_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<KeyProvider> key_provider_;

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
    
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis
