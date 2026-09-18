/**
 * @file themisdb_grpc_service_factory.h
 * @brief Factory for creating and configuring gRPC service instances.
 *
 * @details Provides factory methods to construct gRPC service implementations
 * with proper configuration, dependency injection, and lifecycle management.
 *
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include "api/themisdb_grpc_service.h"
#include <memory>

// Forward declarations
namespace themis {
class RocksDBWrapper;
class TransactionManager;
class IQueryEngine;
class IVectorIndex;
} // namespace themis

namespace themis {
namespace api {

class ThemisDBGrpcServiceFactory {
public:
    ThemisDBGrpcServiceFactory() = default;

    /**
     * @brief With Db.
     * @param[in] db Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    ThemisDBGrpcServiceFactory& withDb(
        std::shared_ptr<RocksDBWrapper> db) {
        db_ = std::move(db);
        return *this;
    }

    /**
     * @brief With Txn Mgr.
     * @param[in] txn_mgr Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    ThemisDBGrpcServiceFactory& withTxnMgr(
        std::shared_ptr<TransactionManager> txn_mgr) {
        txn_mgr_ = std::move(txn_mgr);
        return *this;
    }

    /**
     * @brief With Query Engine.
     * @param[in] engine Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    ThemisDBGrpcServiceFactory& withQueryEngine(
        std::shared_ptr<themis::IQueryEngine> engine) {
        aql_engine_ = std::move(engine);
        return *this;
    }

    /**
     * @brief With Vector Index.
     * @param[in] index Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    ThemisDBGrpcServiceFactory& withVectorIndex(
        std::shared_ptr<themis::IVectorIndex> index) {
        vector_index_ = std::move(index);
        return *this;
    }

    std::unique_ptr<ThemisDBGrpcService> build() const {
        return std::make_unique<ThemisDBGrpcService>(
            db_,
            txn_mgr_,
            aql_engine_,
            vector_index_
        );
    }

private:
    std::shared_ptr<RocksDBWrapper>         db_;
    std::shared_ptr<TransactionManager>     txn_mgr_;
    std::shared_ptr<themis::IQueryEngine>   aql_engine_;
    std::shared_ptr<themis::IVectorIndex>   vector_index_;
};

} // namespace api
} // namespace themis
