/**
 * @file wal_grpc_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: wal_grpc_service.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <functional>
#include <memory>

namespace themis {
namespace sharding {
class WALApplier;
}

namespace server {

// gRPC WAL Apply service wrapper with strict non-proto fail-closed behavior.
class WalGrpcService {
public:
    /// Callback type that provides an opaque grpc::Service* to the wrapper
    /// when the generated proto stubs are absent from the build.
    using ServiceFn = std::function<void*()>;

    explicit WalGrpcService(std::shared_ptr<sharding::WALApplier> wal_applier);
    ~WalGrpcService();

    /**
     * @brief Return the underlying grpc::Service pointer for registration.
     *
     * Returns the concrete service implementation when shard gRPC headers are
     * available.
     *
     * In non-proto builds, construction requires a non-empty callback registered
     * via setServiceFn() that returns a non-null service pointer. If that
     * requirement is not met, the constructor throws and no instance is created.
     */
    void* service();

    /**
     * @brief Configure a process-wide callback that provides a grpc::Service*.
     *
     * Used in non-proto builds (THEMIS_HAS_SHARD_GRPC == 0) to wire a service
     * instance obtained from another module (e.g. dynamically loaded generated
     * stubs). The callback is invoked once during construction.
     *
     * A missing callback, thrown callback exception, or nullptr callback result
     * causes constructor failure via std::runtime_error (fail-closed).
     *
     * Pass an empty function to remove a previously registered callback.
     */
    static void setServiceFn(ServiceFn fn);

private:
    std::shared_ptr<sharding::WALApplier> wal_applier_;

    void* service_ptr_ = nullptr;

    // Impl is only instantiated when shard_rpc gRPC headers are available
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace server
} // namespace themis
