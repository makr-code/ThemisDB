/**
 * @file rpc_service_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGet(const json& params);
    /**
     * @brief TBD: Describe handleGetInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGetInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle PUT operation (upsert with optional transaction support)
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handlePut(const json& params);
    /**
     * @brief TBD: Describe handlePutInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handlePutInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle INSERT operation (strict insert - fails if entity already exists)
     * Supports optional transaction_id for transactional inserts.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleInsert(const json& params);
    /**
     * @brief TBD: Describe handleInsertInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleInsertInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle DELETE operation
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleDelete(const json& params);
    /**
     * @brief TBD: Describe handleDeleteInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleDeleteInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle batch GET operation
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchGet(const json& params);
    /**
     * @brief TBD: Describe handleBatchGetInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchGetInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle batch PUT operation
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchPut(const json& params);
    /**
     * @brief TBD: Describe handleBatchPutInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchPutInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle batch DELETE operation
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchDelete(const json& params);
    /**
     * @brief TBD: Describe handleBatchDeleteInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchDeleteInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle AQL query
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleQuery(const json& params);
    
    /**
     * @brief Handle vector search
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleVectorSearch(const json& params);
    /**
     * @brief TBD: Describe handleVectorSearchInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleVectorSearchInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle graph traversal
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGraphTraverse(const json& params);
    /**
     * @brief TBD: Describe handleGraphTraverseInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGraphTraverseInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle geo query
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGeoQuery(const json& params);
    /**
     * @brief TBD: Describe handleGeoQueryInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGeoQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle time series query
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTimeSeriesQuery(const json& params);
    
    /**
     * @brief Handle transaction begin
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTransactionBegin(const json& params);
    /**
     * @brief TBD: Describe handleTransactionBeginInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTransactionBeginInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle transaction commit
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTransactionCommit(const json& params);
    /**
     * @brief TBD: Describe handleTransactionCommitInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTransactionCommitInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle transaction abort
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTransactionAbort(const json& params);
    /**
     * @brief TBD: Describe handleTransactionAbortInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTransactionAbortInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle health check
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleHealthCheck(const json& params);
    
    /**
     * @brief Authenticate user
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleAuthenticate(const json& params);
    
    /**
     * @brief Handle search operation - search by collection and field filters
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleSearch(const json& params);
    
    /**
     * @brief Handle statistics retrieval - get real database statistics
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleStats(const json& params);
    /**
     * @brief TBD: Describe handleStatsInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleStatsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle entity update - update entity with merge logic
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleUpdateEntity(const json& params);
    /**
     * @brief TBD: Describe handleUpdateEntityInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleUpdateEntityInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle batch update - batch update operations
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchUpdate(const json& params);
    
    /**
     * @brief Handle paginated query - paginated query execution with cursor
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handlePaginatedQuery(const json& params);
    
    /**
     * @brief Handle index operations retrieval - get index management info
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGetIndexOperations(const json& params);
    
    /**
     * @brief Handle aggregation pipeline - execute aggregation pipeline
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleAggregationPipeline(const json& params);
    
    /**
     * @brief Handle list collections - list all collections in database
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleListCollections(const json& params);
    
    /**
     * @brief Handle create index - create index on collection
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleCreateIndex(const json& params);
    /**
     * @brief TBD: Describe handleCreateIndexInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleCreateIndexInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle drop index - drop index from collection
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleDropIndex(const json& params);
    /**
     * @brief TBD: Describe handleDropIndexInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleDropIndexInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle get collection metadata - retrieve collection metadata
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGetCollectionMetadata(const json& params);
    
    /**
     * @brief Dispatch method call
     * @param[in] method Input parameter.
     * @param[in] params Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
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
     * @param[in] code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    json createError(themis::plugins::rpc::RPCErrorCode code, const std::string& message);
    
    /**
     * @brief Create success response
     * @param[in] result Input parameter.
     * @return Return value.
     */
    json createSuccess(const json& result);

    /**
     * @brief TBD: Describe handleAggregationPipelineInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleAggregationPipelineInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleListCollectionsInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleListCollectionsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleGetCollectionMetadataInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGetCollectionMetadataInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleQueryInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleSearchInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleSearchInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handlePaginatedQueryInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handlePaginatedQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleTimeSeriesQueryInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTimeSeriesQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleGetIndexOperationsInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGetIndexOperationsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief TBD: Describe handleBatchUpdateInternal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchUpdateInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
};

} // namespace rpc
} // namespace server
} // namespace themis
