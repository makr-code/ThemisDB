/**
 * @file wal_grpc_service.h
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

namespace themis {
namespace sharding {
class WALApplier;
}

namespace server {

// gRPC WAL Apply service wrapper with optional non-proto service injection.
class WalGrpcService {
public:
    using ServiceFn = std::function<void*()>;

    /**
     * @brief Wal Grpc Service.
     * @param[in] wal_applier Input parameter.
     * @return Return value.
     */
    explicit WalGrpcService(std::shared_ptr<sharding::WALApplier> wal_applier);
    ~WalGrpcService();

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

private:
    std::shared_ptr<sharding::WALApplier> wal_applier_;

    void* service_ptr_ = nullptr;

    // Impl is only instantiated when shard_rpc gRPC headers are available
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace server
} // namespace themis
