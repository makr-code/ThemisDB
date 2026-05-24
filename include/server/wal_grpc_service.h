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

// gRPC WAL Apply service wrapper; returns nullptr if gRPC stubs are unavailable
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
     * available.  Returns nullptr otherwise, and the caller must skip
     * registration without crashing.
     *
     * If a callback was registered via setServiceFn() its return value is used
     * for non-proto builds (provided the production-mode check already passed).
     */
    void* service();

    /**
     * @brief Configure a process-wide callback that provides a grpc::Service*.
     *
     * Used in non-proto builds (THEMIS_HAS_SHARD_GRPC == 0) to wire a service
     * instance obtained from another module (e.g. a test double or a
     * dynamically loaded plugin).  The callback is invoked once during
     * construction after the production-mode safety check.  Exceptions in the
     * callback are caught and the service pointer is clamped to nullptr
     * (fail-closed).
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
