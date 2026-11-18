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
#include <string>

namespace themis {
namespace sharding {
class WALApplier;
}

namespace server {

// gRPC WAL Apply service wrapper with optional non-proto service injection.
class WalGrpcService {
public:
    using ServiceFn = std::function<void*()>;
    using EntityWriteFn = std::function<bool(
        const std::string& collection,
        const std::string& uuid,
        const std::string& data_json,
        std::string& error)>;

    /**
     * @brief Creates the registered shard gRPC service.
     * @param wal_applier Applies WAL batches; may be null when only entity
     *        replication is configured.
     * @param entity_write_fn Persists validated replicated entities through
     *        the node's normal index/storage path; an empty callback disables
     *        ReplicateData and causes it to return FAILED_PRECONDITION.
     * @param local_shard_id This server's configured shard ID. When non-empty,
     *        ReplicateData rejects requests addressed to another shard.
     * @throws std::runtime_error if the non-proto service bridge fails.
     */
    explicit WalGrpcService(
        std::shared_ptr<sharding::WALApplier> wal_applier,
        EntityWriteFn entity_write_fn = {},
        std::string local_shard_id = {});
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
    EntityWriteFn entity_write_fn_;
    std::string local_shard_id_;

    void* service_ptr_ = nullptr;

    // Impl is only instantiated when shard_rpc gRPC headers are available
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace server
} // namespace themis
