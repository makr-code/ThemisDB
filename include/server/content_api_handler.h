/**
 * @file content_api_handler.h
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
class VectorIndexManager;

namespace content {
class ContentManager;
class ContentProcessor;
}

namespace server {

/**
 * @brief Handler for Content Operations
 * 
 * This handler manages all content-related endpoints:
 * - POST /content/import - Import and process content (PDF, images, etc.)
 * - GET /content/:id - Retrieve content metadata
 * - GET /content/:id/blob - Retrieve raw content blob
 * - GET /content/:id/chunks - Retrieve content chunks
 * - POST /search/hybrid - Hybrid search (text + vector)
 * - POST /search/fusion - Fusion search with RRF
 * - POST /search/fulltext - Fulltext search
 * - GET /content/config - Get content processing configuration
 * - PUT /content/config - Update content processing configuration
 * - GET /config/content-filters - Get content filter schema
 * - PUT /config/content-filters - Update content filter schema
 * - GET /config/edge-weights - Get edge weight configuration
 * - PUT /config/edge-weights - Update edge weight configuration
 * - GET /config/encryption-schema - Get encryption schema
 * - PUT /config/encryption-schema - Update encryption schema
 * 
 * Features:
 * - Multi-format content ingestion (PDF, DOCX, images, etc.)
 * - Content chunking and embedding
 * - Hybrid and fusion search capabilities
 * - Content filtering and classification
 * - Configuration management
 * 
 * Extracted from http_server.cpp (~900 lines) to improve maintainability.
 */
class ContentApiHandler {
public:
    /**
     * @brief Construct a new Content API Handler
     * 
     * @param storage Storage backend
     * @param content_manager Content management system
     * @param content_processor Content processing engine
     * @param auth Authentication/authorization middleware
     * @param secondary_index Secondary index manager for fulltext search
     * @param vector_index Vector index manager for vector search
     */
    ContentApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<content::ContentManager> content_manager,
        std::shared_ptr<content::ContentProcessor> content_processor,
        std::shared_ptr<themis::AuthMiddleware> auth,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<VectorIndexManager> vector_index
    );

    /**
     * @brief Handle POST /content/import request
     * @param req HTTP request with content data or URL
     * @return HTTP response with import status and content ID
     */
    http::response<http::string_body> handleImport(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /content/:id request
     * @param req HTTP request
     * @return HTTP response with content metadata
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /content/:id/blob request
     * @param req HTTP request
     * @return HTTP response with raw content blob
     */
    http::response<http::string_body> handleGetBlob(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /content/:id/chunks request
     * @param req HTTP request
     * @return HTTP response with content chunks
     */
    http::response<http::string_body> handleGetChunks(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /search/hybrid request
     * @param req HTTP request with search query
     * @return HTTP response with hybrid search results
     */
    http::response<http::string_body> handleHybridSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /search/fusion request
     * @param req HTTP request with search query
     * @return HTTP response with fusion search results
     */
    http::response<http::string_body> handleFusionSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /search/fulltext request
     * @param req HTTP request with search query
     * @return HTTP response with fulltext search results
     */
    http::response<http::string_body> handleFulltextSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /content/config request
     * @param req HTTP request
     * @return HTTP response with current configuration
     */
    http::response<http::string_body> handleConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /content/config request
     * @param req HTTP request with new configuration
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /config/content-filters request
     * @param req HTTP request
     * @return HTTP response with filter schema
     */
    http::response<http::string_body> handleContentFilterSchemaGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /config/content-filters request
     * @param req HTTP request with new filter schema
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleContentFilterSchemaPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /config/edge-weights request
     * @param req HTTP request
     * @return HTTP response with edge weight configuration
     */
    http::response<http::string_body> handleEdgeWeightConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /config/edge-weights request
     * @param req HTTP request with new edge weight configuration
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleEdgeWeightConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /config/encryption-schema request
     * @param req HTTP request
     * @return HTTP response with encryption schema
     */
    http::response<http::string_body> handleEncryptionSchemaGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /config/encryption-schema request
     * @param req HTTP request with new encryption schema
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleEncryptionSchemaPut(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<content::ContentManager> content_manager_;
    std::shared_ptr<content::ContentProcessor> content_processor_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
