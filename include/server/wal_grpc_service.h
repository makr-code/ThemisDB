/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_grpc_service.h                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
