/**
 * @file themis_core_grpc_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <memory>
#include <string>

// Bring in the concrete query-engine interface so AQLEngine can be used
// as a complete, callable type inside the gRPC service implementation.
#include "themis/base/interfaces/query_interface.h"

// Forward declarations of the internal components used by the service.
namespace themis {
    class RocksDBWrapper;
    class TransactionManager;
    using AQLEngine = IQueryEngine;
}

namespace themis {
namespace core {

class ThemisCoreServiceImpl {
public:
    using ServiceInstanceFn = std::function<void*()>;

    ThemisCoreServiceImpl(
        std::shared_ptr<RocksDBWrapper>    db,
        std::shared_ptr<TransactionManager> txn_mgr,
        std::shared_ptr<AQLEngine>          aql_engine
    );

    ~ThemisCoreServiceImpl();

    /**
     * @brief Get Service Instance.
     * @return Pointer to the result.
     */
    void* getServiceInstance();

    /**
     * @brief Set Service Instance Fn.
     * @param[in] fn Input parameter.
     */
    static void setServiceInstanceFn(ServiceInstanceFn fn);

private:
    std::shared_ptr<RocksDBWrapper>     db_;
    std::shared_ptr<TransactionManager> txn_mgr_;
    std::shared_ptr<AQLEngine>          aql_engine_;

    void* service_ptr_ = nullptr;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace core
} // namespace themis
