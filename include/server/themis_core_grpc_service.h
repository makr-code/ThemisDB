/**
 * @file themis_core_grpc_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <memory>
#include <string>

// Forward declarations of the internal components used by the service.
namespace themis {
    class RocksDBWrapper;
    class TransactionManager;
    class AQLEngine;
}

namespace themis {
namespace core {

/**
 * @brief ThemisDB Core gRPC Service wrapper (ThemisCoreService, themis_core.proto)
 *
 * Wraps the ThemisCoreService implementation defined in proto/themis_core.proto.
 * Follows the same conditional-compilation pattern as WalGrpcService: the actual
 * grpc::Service-derived implementation is compiled in only when the generated
 * `themis_core.grpc.pb.h` header is on the include path. In non-proto builds,
 * construction requires an injected non-null service instance callback.
 *
 * Part of ThemisDB v1.3.0 – Feature #8: gRPC Protocol
 */
class ThemisCoreServiceImpl {
public:
    /// Callback type that provides an opaque grpc::Service* to the wrapper
    /// when the generated proto stubs are absent from the build.
    using ServiceInstanceFn = std::function<void*()>;

    /**
     * @brief Construct service with database components.
     * @param db         RocksDB wrapper for storage operations.
     * @param txn_mgr    Transaction manager for ACID operations.
     * @param aql_engine AQL query engine.
     */
    ThemisCoreServiceImpl(
        std::shared_ptr<RocksDBWrapper>    db,
        std::shared_ptr<TransactionManager> txn_mgr,
        std::shared_ptr<AQLEngine>          aql_engine
    );

    ~ThemisCoreServiceImpl();

    /**
     * @brief Return the underlying grpc::Service pointer for registration.
     *
     * Returns the concrete service implementation when `themis_core.grpc.pb.h`
     * is available (i.e. protoc has been run).
     *
     * If a service-instance callback was registered via setServiceInstanceFn(),
     * the result of that callback is returned for non-proto builds.
     *
     * A missing callback, thrown callback exception, or nullptr callback result
     * causes constructor failure via std::runtime_error (fail-closed).
     */
    void* getServiceInstance();

    /**
     * @brief Configure a process-wide callback that provides a grpc::Service*.
     *
     * Used in non-proto builds to wire a service instance obtained from another
     * module (e.g. a dynamically loaded plugin or generated stubs). The callback
     * is invoked once during construction.
     *
     * Pass an empty function to remove a previously registered callback.
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
