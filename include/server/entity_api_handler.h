#pragma once

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class GraphIndexManager;
class TransactionManager;
class FieldEncryption;

namespace security {
class KeyProvider;
}

namespace utils {
class Tracer;
}

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class AuthMiddleware;

/**
 * @brief Handler for Entity CRUD Operations
 * 
 * This handler manages all entity-related endpoints:
 * - GET /entities/:key - Retrieve an entity by key
 * - PUT /entities/:key - Create or update an entity
 * - POST /entities - Create an entity
 * - DELETE /entities/:key - Delete an entity
 * - POST /entities/batch - Batch operations on multiple entities
 * 
 * Features:
 * - Field-level encryption/decryption support
 * - Secondary index management
 * - Graph edge management
 * - Authorization checks
 * - Audit logging
 * 
 * Extracted from http_server.cpp (~880 lines) to improve maintainability.
 */
class EntityApiHandler {
public:
    /**
     * @brief Construct a new Entity API Handler
     * 
     * @param storage Storage backend for entity persistence
     * @param secondary_index Secondary index manager for entity indexing
     * @param graph_index Graph index manager for relationship management
     * @param tx_manager Transaction manager for ACID operations
     * @param field_encryption Field-level encryption handler
     * @param key_provider Cryptographic key provider
     * @param auth Authentication/authorization middleware
     */
    EntityApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<FieldEncryption> field_encryption,
        std::shared_ptr<security::KeyProvider> key_provider,
        std::shared_ptr<AuthMiddleware> auth
    );

    /**
     * @brief Handle GET /entities/:key request
     * 
     * Retrieves an entity by its key. Supports optional decryption via ?decrypt=true query parameter.
     * 
     * @param req HTTP request
     * @return HTTP response with entity data or error
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /entities/:key request
     * 
     * Creates or updates an entity. Supports field-level encryption based on schema configuration.
     * 
     * @param req HTTP request with entity data in body
     * @return HTTP response with operation status
     */
    http::response<http::string_body> handlePut(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /entities/:key request
     * 
     * Deletes an entity and associated indexes/edges.
     * 
     * @param req HTTP request
     * @return HTTP response with deletion status
     */
    http::response<http::string_body> handleDelete(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /entities/batch request
     * 
     * Performs batch operations (create, update, delete) on multiple entities in a single transaction.
     * 
     * @param req HTTP request with batch operations in body
     * @return HTTP response with batch operation results
     */
    http::response<http::string_body> handleBatch(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<security::KeyProvider> key_provider_;
    std::shared_ptr<AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
