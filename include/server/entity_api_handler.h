/**
 * @file entity_api_handler.h
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
#include <atomic>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class GraphIndexManager;
class TransactionManager;
class FieldEncryption;
class Changefeed;

namespace index {
class SpatialIndexManager;
}

class KeyProvider;

namespace sharding {
class WALManager;
class ReplicationCoordinator;
class MultiPrimaryCoordinator;
struct WriteConcernConfig;
class CollectionRedundancyManager;
class ConsistentHashRing;
class ShardTopology;
}

namespace utils {
class Tracer;
}

namespace server {

// Configuration for entity operations
struct EntityApiConfig {
    bool feature_cdc = false;  // Enable change data capture
    bool feature_geo = false;  // Enable geo/spatial index
    bool feature_replication = false;  // Enable replication/write concern
    bool feature_raid = false;  // Enable RAID-style redundancy via RedundancyStrategy
};

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
 * - Optional: Spatial/geo indexing
 * - Optional: Change data capture (CDC)
 * - Optional: Replication and write concern
 * 
 * Extracted from http_server.cpp (~880 lines) to improve maintainability.
 */
class EntityApiHandler {
public:
    /**
     * @brief Authentication context extracted from request
     */
    struct AuthContext {
        std::string user_id;
        std::vector<std::string> groups;
    };

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
     * @param config Feature configuration flags
     * @param spatial_index Optional: Spatial/geo index manager (raw pointer, not owned)
     * @param changefeed Optional: CDC changefeed manager
     * @param wal_manager Optional: WAL manager for replication
     * @param replication_coordinator Optional: Replication coordinator
     * @param multi_primary_coordinator Optional: Multi-primary coordinator
     * @param redundancy_manager Optional: RAID-style redundancy manager
     * @param hash_ring Optional: Consistent hash ring for shard routing
     * @param shard_topology Optional: Shard topology for RAID operations
     */
    EntityApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<FieldEncryption> field_encryption,
        std::shared_ptr<KeyProvider> key_provider,
        std::shared_ptr<themis::AuthMiddleware> auth,
        const EntityApiConfig& config = EntityApiConfig{},
        index::SpatialIndexManager* spatial_index = nullptr,
        std::shared_ptr<Changefeed> changefeed = nullptr,
        std::shared_ptr<sharding::WALManager> wal_manager = nullptr,
        std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator = nullptr,
        std::shared_ptr<sharding::MultiPrimaryCoordinator> multi_primary_coordinator = nullptr,
        std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager = nullptr,
        std::shared_ptr<sharding::ConsistentHashRing> hash_ring = nullptr,
        std::shared_ptr<sharding::ShardTopology> shard_topology = nullptr
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

    /**
     * @brief Bulk document insert from newline-delimited JSON (NDJSON).
     *
     * Endpoint: POST /v2/documents
     * Content-Type: application/x-ndjson
     *
     * Each line in the body is a JSON object representing a document to insert.
     * Accepts up to 10,000 documents per request.  Returns a summary of
     * successful inserts and any per-line errors.
     */
    http::response<http::string_body> handleBulkNdjson(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<KeyProvider> key_provider_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    // Configuration
    EntityApiConfig config_;
    
    // Optional features
    index::SpatialIndexManager* spatial_index_;  // Not owned
    std::shared_ptr<Changefeed> changefeed_;
    std::shared_ptr<sharding::WALManager> wal_manager_;
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator_;
    std::shared_ptr<sharding::MultiPrimaryCoordinator> multi_primary_coordinator_;
    
    // RAID redundancy (optional)
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager_;
    std::shared_ptr<sharding::ConsistentHashRing> hash_ring_;
    std::shared_ptr<sharding::ShardTopology> shard_topology_;

    // Helper methods
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helpers
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& scope,
        const std::string& action,
        const std::string& resource
    );
};

} // namespace server
} // namespace themis
