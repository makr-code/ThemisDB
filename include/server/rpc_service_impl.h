/**
 * @file rpc_service_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Abstract Interface Header**: Defines RPC service method handler contracts.
 *       Implementation is delegated to the RPC plugin infrastructure.
 *       See plugins/rpc_plugin_interface.h and implementations in src/server/rpc*.cpp.
 */

#pragma once

#include "plugins/rpc_plugin_interface.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace themis {
class RocksDBWrapper;  // Forward declaration
class AuthMiddleware;  // Forward declaration
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
        themis::index::SpatialIndexManager* spatial_index = nullptr,
        std::shared_ptr<AuthMiddleware> auth = nullptr,
        const std::chrono::steady_clock::time_point* start_time = nullptr
    ) : storage_(storage), spatial_index_(spatial_index), auth_(auth), start_time_(start_time) {}
    
    /**
     * @brief Handle GET operation
     */
    json handleGet(const json& params);
    json handleGetInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle PUT operation (upsert with optional transaction support)
     */
    json handlePut(const json& params);
    json handlePutInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle INSERT operation (strict insert - fails if entity already exists)
     * Supports optional transaction_id for transactional inserts.
     */
    json handleInsert(const json& params);
    json handleInsertInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle DELETE operation
     */
    json handleDelete(const json& params);
    json handleDeleteInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle batch GET operation
     */
    json handleBatchGet(const json& params);
    json handleBatchGetInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle batch PUT operation
     */
    json handleBatchPut(const json& params);
    json handleBatchPutInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle batch DELETE operation
     */
    json handleBatchDelete(const json& params);
    json handleBatchDeleteInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle AQL query
     */
    json handleQuery(const json& params);
    
    /**
     * @brief Handle vector search
     */
    json handleVectorSearch(const json& params);
    json handleVectorSearchInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle graph traversal
     */
    json handleGraphTraverse(const json& params);
    json handleGraphTraverseInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle geo query
     */
    json handleGeoQuery(const json& params);
    json handleGeoQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle time series query
     */
    json handleTimeSeriesQuery(const json& params);
    
    /**
     * @brief Handle transaction begin
     */
    json handleTransactionBegin(const json& params);
    json handleTransactionBeginInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle transaction commit
     */
    json handleTransactionCommit(const json& params);
    json handleTransactionCommitInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle transaction abort
     */
    json handleTransactionAbort(const json& params);
    json handleTransactionAbortInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
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
    json handleStatsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle entity update - update entity with merge logic
     */
    json handleUpdateEntity(const json& params);
    json handleUpdateEntityInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
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
    json handleCreateIndexInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle drop index - drop index from collection
     */
    json handleDropIndex(const json& params);
    json handleDropIndexInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
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
    std::shared_ptr<AuthMiddleware> auth_;
    const std::chrono::steady_clock::time_point* start_time_;
    
    /**
     * @brief Verify authentication token from context and check required scope
     * @param context RPC request context containing metadata with auth token
     * @param username Output parameter for authenticated username
     * @param required_scope Required authorization scope (e.g., "rpc:read", "rpc:write", "rpc:admin")
     * @return true if authentication and authorization succeed, false otherwise
     */
    bool verifyAuth(const themis::plugins::rpc::RPCRequestContext& context, std::string& username, const std::string& required_scope);
    
    /**
     * @brief Create error response
     */
    json createError(themis::plugins::rpc::RPCErrorCode code, const std::string& message);
    
    /**
     * @brief Create success response
     */
    json createSuccess(const json& result);

    json handleAggregationPipelineInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleListCollectionsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleGetCollectionMetadataInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleSearchInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handlePaginatedQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleTimeSeriesQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleGetIndexOperationsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    json handleBatchUpdateInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
};

} // namespace rpc
} // namespace server
} // namespace themis
