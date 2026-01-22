#pragma once

#include "plugins/rpc_plugin_interface.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

/**
 * @file rpc_service_impl.h
 * @brief ThemisDB RPC Service Implementation
 * 
 * This file contains the service implementation that handles RPC requests
 * for ThemisDB operations (GET, PUT, DELETE, Query, etc.)
 */

namespace themis {
class RocksDBWrapper;  // Forward declaration
namespace index {
class SpatialIndexManager;  // Forward declaration
}
}

namespace themis {
namespace server {
namespace rpc {

using json = nlohmann::json;

/**
 * @brief RPC Method Handler for ThemisDB operations
 * 
 * Refactored to use RocksDBWrapper directly for database operations.
 */
class ThemisRPCService {
public:
    explicit ThemisRPCService(
        RocksDBWrapper* storage,
        themis::index::SpatialIndexManager* spatial_index = nullptr
    ) : storage_(storage), spatial_index_(spatial_index) {}
    
    /**
     * @brief Handle GET operation
     */
    json handleGet(const json& params);
    
    /**
     * @brief Handle PUT operation
     */
    json handlePut(const json& params);
    
    /**
     * @brief Handle DELETE operation
     */
    json handleDelete(const json& params);
    
    /**
     * @brief Handle batch GET operation
     */
    json handleBatchGet(const json& params);
    
    /**
     * @brief Handle batch PUT operation
     */
    json handleBatchPut(const json& params);
    
    /**
     * @brief Handle AQL query
     */
    json handleQuery(const json& params);
    
    /**
     * @brief Handle vector search
     */
    json handleVectorSearch(const json& params);
    
    /**
     * @brief Handle graph traversal
     */
    json handleGraphTraverse(const json& params);
    
    /**
     * @brief Handle geo query
     */
    json handleGeoQuery(const json& params);
    
    /**
     * @brief Handle time series query
     */
    json handleTimeSeriesQuery(const json& params);
    
    /**
     * @brief Handle transaction begin
     */
    json handleTransactionBegin(const json& params);
    
    /**
     * @brief Handle transaction commit
     */
    json handleTransactionCommit(const json& params);
    
    /**
     * @brief Handle transaction abort
     */
    json handleTransactionAbort(const json& params);
    
    /**
     * @brief Handle health check
     */
    json handleHealthCheck(const json& params);
    
    /**
     * @brief Authenticate user
     */
    json handleAuthenticate(const json& params);
    
    /**
     * @brief Handle search operation - search by collection and field filters
     */
    json handleSearch(const json& params);
    
    /**
     * @brief Handle statistics retrieval - get real database statistics
     */
    json handleStats(const json& params);
    
    /**
     * @brief Handle entity update - update entity with merge logic
     */
    json handleUpdateEntity(const json& params);
    
    /**
     * @brief Handle batch update - batch update operations
     */
    json handleBatchUpdate(const json& params);
    
    /**
     * @brief Handle paginated query - paginated query execution with cursor
     */
    json handlePaginatedQuery(const json& params);
    
    /**
     * @brief Handle index operations retrieval - get index management info
     */
    json handleGetIndexOperations(const json& params);
    
    /**
     * @brief Handle aggregation pipeline - execute aggregation pipeline
     */
    json handleAggregationPipeline(const json& params);
    
    /**
     * @brief Handle list collections - list all collections in database
     */
    json handleListCollections(const json& params);
    
    /**
     * @brief Handle create index - create index on collection
     */
    json handleCreateIndex(const json& params);
    
    /**
     * @brief Handle drop index - drop index from collection
     */
    json handleDropIndex(const json& params);
    
    /**
     * @brief Handle get collection metadata - retrieve collection metadata
     */
    json handleGetCollectionMetadata(const json& params);
    
    /**
     * @brief Dispatch method call
     */
    json dispatch(const std::string& method, const json& params, const themis::plugins::rpc::RPCRequestContext& context);
    
private:
    RocksDBWrapper* storage_;
    themis::index::SpatialIndexManager* spatial_index_;
    
    /**
     * @brief Verify authentication token from context
     */
    bool verifyAuth(const themis::plugins::rpc::RPCRequestContext& context, std::string& username);
    
    /**
     * @brief Create error response
     */
    json createError(themis::plugins::rpc::RPCErrorCode code, const std::string& message);
    
    /**
     * @brief Create success response
     */
    json createSuccess(const json& result);
};

} // namespace rpc
} // namespace server
} // namespace themis
