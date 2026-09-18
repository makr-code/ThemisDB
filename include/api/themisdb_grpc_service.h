/**
 * @file themisdb_grpc_service.h
 * @brief gRPC service implementation for ThemisDB database operations.
 *
 * @details Implements the generated gRPC service stub, translating protobuf
 * messages to/from internal data structures and delegating to core service logic.
 *
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include <functional>
#include <memory>

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/registry.h>
#endif

// Forward declarations of the internal components required by the service.
namespace themis {
class RocksDBWrapper;
class TransactionManager;
} // namespace themis

// Interface forward declarations (always available without proto stubs)
namespace themis {
class IQueryEngine;
class IVectorIndex;
using IQueryEnginePtr = std::shared_ptr<IQueryEngine>;
} // namespace themis

namespace themis {
namespace api {

class ThemisDBGrpcService {
public:
    using ServiceFn = std::function<void*()>;

    ThemisDBGrpcService(
        std::shared_ptr<RocksDBWrapper>     db,
        std::shared_ptr<TransactionManager> txn_mgr
    );

    ThemisDBGrpcService(
        std::shared_ptr<RocksDBWrapper>          db,
        std::shared_ptr<TransactionManager>      txn_mgr,
        std::shared_ptr<themis::IQueryEngine>    aql_engine,
        std::shared_ptr<themis::IVectorIndex>    vector_index
    );

    ~ThemisDBGrpcService();

    /**
     * @brief Service.
     * @return Pointer to the result.
     */
    void* service();

    /**
     * @brief Set Service Fn.
     * @param[in] fn Input parameter.
     */
    static void setServiceFn(ServiceFn fn);

#ifdef THEMIS_HAS_PROMETHEUS
    /**
     * @brief Set Prometheus Registry.
     * @param[in] registry Input parameter.
     */
    void setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry);
#endif

private:
    std::shared_ptr<RocksDBWrapper>             db_;
    std::shared_ptr<TransactionManager>         txn_mgr_;
    std::shared_ptr<themis::IQueryEngine>       aql_engine_;
    std::shared_ptr<themis::IVectorIndex>       vector_index_;

    void* service_ptr_ = nullptr;

    class Impl;
    std::unique_ptr<Impl> impl_;

    /**
     * @brief Build Impl.
     */
    void buildImpl();
};

} // namespace api
} // namespace themis
