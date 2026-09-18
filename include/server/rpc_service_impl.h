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

class ThemisRPCService {
public:
    explicit ThemisRPCService(
        RocksDBWrapper* storage,
        themis::index::SpatialIndexManager* spatial_index = nullptr,
        std::shared_ptr<AuthMiddleware> auth = nullptr,
        const std::chrono::steady_clock::time_point* start_time = nullptr
    ) : storage_(storage), spatial_index_(spatial_index), auth_(auth), start_time_(start_time) {}
    
    /**
     * @brief Handle Get.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGet(const json& params);
    /**
     * @brief Handle Get Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGetInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Put.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handlePut(const json& params);
    /**
     * @brief Handle Put Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handlePutInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Insert.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleInsert(const json& params);
    /**
     * @brief Handle Insert Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleInsertInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Delete.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleDelete(const json& params);
    /**
     * @brief Handle Delete Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleDeleteInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Batch Get.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchGet(const json& params);
    /**
     * @brief Handle Batch Get Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchGetInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Batch Put.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchPut(const json& params);
    /**
     * @brief Handle Batch Put Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchPutInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Batch Delete.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchDelete(const json& params);
    /**
     * @brief Handle Batch Delete Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleBatchDeleteInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Query.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleQuery(const json& params);
    
    /**
     * @brief Handle Vector Search.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleVectorSearch(const json& params);
    /**
     * @brief Handle Vector Search Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleVectorSearchInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Graph Traverse.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGraphTraverse(const json& params);
    /**
     * @brief Handle Graph Traverse Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGraphTraverseInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Geo Query.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGeoQuery(const json& params);
    /**
     * @brief Handle Geo Query Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGeoQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Time Series Query.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTimeSeriesQuery(const json& params);
    
    /**
     * @brief Handle Transaction Begin.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTransactionBegin(const json& params);
    /**
     * @brief Handle Transaction Begin Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTransactionBeginInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Transaction Commit.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTransactionCommit(const json& params);
    /**
     * @brief Handle Transaction Commit Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTransactionCommitInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Transaction Abort.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleTransactionAbort(const json& params);
    /**
     * @brief Handle Transaction Abort Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTransactionAbortInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Health Check.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleHealthCheck(const json& params);
    
    /**
     * @brief Handle Authenticate.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleAuthenticate(const json& params);
    
    /**
     * @brief Handle Search.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleSearch(const json& params);
    
    /**
     * @brief Handle Stats.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleStats(const json& params);
    /**
     * @brief Handle Stats Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleStatsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Update Entity.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleUpdateEntity(const json& params);
    /**
     * @brief Handle Update Entity Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleUpdateEntityInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Batch Update.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleBatchUpdate(const json& params);
    
    /**
     * @brief Handle Paginated Query.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handlePaginatedQuery(const json& params);
    
    /**
     * @brief Handle Get Index Operations.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGetIndexOperations(const json& params);
    
    /**
     * @brief Handle Aggregation Pipeline.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleAggregationPipeline(const json& params);
    
    /**
     * @brief Handle List Collections.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleListCollections(const json& params);
    
    /**
     * @brief Handle Create Index.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleCreateIndex(const json& params);
    /**
     * @brief Handle Create Index Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleCreateIndexInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );

    /**
     * @brief Handle Drop Index.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleDropIndex(const json& params);
    /**
     * @brief Handle Drop Index Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleDropIndexInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    
    /**
     * @brief Handle Get Collection Metadata.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleGetCollectionMetadata(const json& params);
    
    /**
     * @brief Dispatch.
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
     * @brief Verify Auth.
     * @param[in] context Input parameter.
     * @param[in,out] username Input/output parameter.
     * @param[in] required_scope Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyAuth(const themis::plugins::rpc::RPCRequestContext& context, std::string& username, const std::string& required_scope);
    
    /**
     * @brief Create Error.
     * @param[in] code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    json createError(themis::plugins::rpc::RPCErrorCode code, const std::string& message);
    
    /**
     * @brief Create Success.
     * @param[in] result Input parameter.
     * @return Return value.
     */
    json createSuccess(const json& result);

    /**
     * @brief Handle Aggregation Pipeline Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleAggregationPipelineInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle List Collections Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleListCollectionsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Get Collection Metadata Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGetCollectionMetadataInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Query Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Search Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleSearchInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Paginated Query Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handlePaginatedQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Time Series Query Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleTimeSeriesQueryInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Get Index Operations Internal.
     * @param[in] params Input parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    json handleGetIndexOperationsInternal(
        const json& params,
        const std::optional<std::chrono::steady_clock::time_point>& deadline
    );
    /**
     * @brief Handle Batch Update Internal.
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
