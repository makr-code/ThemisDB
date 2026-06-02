/*
 * ThemisDB | File: themisdb_grpc_service_factory.h | Version: 0.0.12 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 97
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4409 Add training materials from... (2026-03-25)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Fluent factory for building a fully-wired ThemisDBGrpcService.
 *
 * Usage:
 * @code
 *   auto svc = ThemisDBGrpcServiceFactory{}
 *                  .withDb(my_db)
 *                  .withTxnMgr(my_txn_mgr)
 *                  .withQueryEngine(my_aql_engine)
 *                  .withVectorIndex(my_vector_index)
 *                  .build();
 *   grpc_server.registerService(svc->service());
 * @endcode
 *
 * All components are optional.  When a component is not provided the
 * corresponding RPC stubs return grpc::StatusCode::UNIMPLEMENTED.
 *
 * This factory is the recommended way to assemble a ThemisDBGrpcService
 * because it makes the dependency set explicit and allows incremental wiring
 * as components become available.
 */
class ThemisDBGrpcServiceFactory {
public:
    ThemisDBGrpcServiceFactory() = default;

    /// Set the storage backend.
    ThemisDBGrpcServiceFactory& withDb(
        std::shared_ptr<RocksDBWrapper> db) {
        db_ = std::move(db);
        return *this;
    }

    /// Set the transaction manager.
    ThemisDBGrpcServiceFactory& withTxnMgr(
        std::shared_ptr<TransactionManager> txn_mgr) {
        txn_mgr_ = std::move(txn_mgr);
        return *this;
    }

    /// Set the AQL engine (enables ExecuteAQL, StreamAQL, HybridSearch, FullTextSearch).
    ThemisDBGrpcServiceFactory& withQueryEngine(
        std::shared_ptr<themis::IQueryEngine> engine) {
        aql_engine_ = std::move(engine);
        return *this;
    }

    /// Set the vector index (enables VectorSearch and FilteredVectorSearch).
    ThemisDBGrpcServiceFactory& withVectorIndex(
        std::shared_ptr<themis::IVectorIndex> index) {
        vector_index_ = std::move(index);
        return *this;
    }

    /// Build and return the configured ThemisDBGrpcService.
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
