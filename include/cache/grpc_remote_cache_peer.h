/**
 * @file grpc_remote_cache_peer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

// GrpcRemoteCachePeer – gRPC-backed implementation of IRemoteCachePeer.
//
// Delivers cache invalidation messages to a remote peer over a gRPC channel
// using a simple JSON payload framing over the generic gRPC unary RPC API.
// No generated protobuf stubs are required; the payload is encoded as a
// UTF-8 JSON string inside a grpc::ByteBuffer, mirroring the transport-only
// design of GrpcTransport (src/network/grpc_transport.h).
//
// RPC method: /themis.cache.v1.CacheInvalidation/Invalidate
// Request payload (JSON):
//   { "type": "invalidate",         "key": "<key>",      "tenant_id": "<tid>" }
//   { "type": "invalidate_tenant",  "key": "",           "tenant_id": "<tid>" }
//
// This class is guarded by THEMIS_ENABLE_GRPC.

#pragma once

#include "cache/cache_replication_coordinator.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#ifdef THEMIS_ENABLE_GRPC

#include <grpcpp/create_channel.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/security/credentials.h>

namespace themis {
namespace cache {

class GrpcRemoteCachePeer final : public IRemoteCachePeer {
public:
    static constexpr const char* kInvalidateMethod =
        "/themis.cache.v1.CacheInvalidation/Invalidate";

    // ── Configuration ─────────────────────────────────────────────────────────

    struct Config {
        std::string address;

        int rpc_timeout_ms = 1000;

        bool        tls_enabled   = false;
        bool        allow_insecure = false;
        std::string tls_ca_cert;  ///< PEM-encoded CA certificate (in-memory)

        Config() = default;
        explicit Config(std::string addr) : address(std::move(addr)) {}
    };

    /**
     * @brief ── Lifecycle ─────────────────────────────────────────────────────────────
     * @param[in] config Input parameter.
     * @return Return value.
     */

    explicit GrpcRemoteCachePeer(Config config);

    /**
     * @brief Grpc Remote Cache Peer.
     * @param[in] addr Input parameter.
     * @return Return value.
     */
    explicit GrpcRemoteCachePeer(const std::string& addr);

    ~GrpcRemoteCachePeer() override = default;

    // ── IRemoteCachePeer ──────────────────────────────────────────────────────

    void invalidate(const std::string& key,
                    const std::string& tenant_id = "") override;

    void invalidateTenant(const std::string& tenant_id) override;

    std::string address() const override { return config_.address; }

    bool isHealthy() const override {
        // memory_order fix: acquire pairs with release stores in sendRpc()
        // to ensure the health state written after an RPC outcome is visible.
        return healthy_.load(std::memory_order_acquire);
    }

private:
    /**
     * @brief Build Credentials.
     * @return Return value.
     */
    std::shared_ptr<grpc::ChannelCredentials> buildCredentials() const;

    /**
     * @brief Send Rpc.
     * @param[in] type Input parameter.
     * @param[in] key Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void sendRpc(const std::string& type,
                 const std::string& key,
                 const std::string& tenant_id);

    Config                                    config_;
    std::shared_ptr<grpc::Channel>            channel_;
    std::unique_ptr<grpc::GenericStub>        stub_;
    std::atomic<bool>                         healthy_{true};
};

}  // namespace cache
}  // namespace themis

#else

namespace themis {
namespace cache {

class GrpcRemoteCachePeer final : public IRemoteCachePeer {
public:
    using BackendInvokeFn = std::function<bool(const std::string& address,
                                               const std::string& type,
                                               const std::string& key,
                                               const std::string& tenant_id)>;

    static constexpr const char* kInvalidateMethod =
        "/themis.cache.v1.CacheInvalidation/Invalidate";

    struct Config {
        std::string address = {};
        int rpc_timeout_ms = 1000;
        bool        tls_enabled   = false;
        bool        allow_insecure = false;
        std::string tls_ca_cert;

        Config() = default;
        explicit Config(std::string addr) : address(std::move(addr)) {}
    };

    /**
     * @brief Grpc Remote Cache Peer.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GrpcRemoteCachePeer(Config config)
        : config_(std::move(config)) {}

    /**
     * @brief Grpc Remote Cache Peer.
     * @param[in] addr Input parameter.
     * @return Return value.
     */
    explicit GrpcRemoteCachePeer(const std::string& addr)
        : GrpcRemoteCachePeer(Config(addr)) {}

    ~GrpcRemoteCachePeer() override = default;

    /**
     * @brief Set Backend Invoke Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), bridgeMutex(), backendInvokeFn(), std::move().
     */
    static void setBackendInvokeFn(BackendInvokeFn fn) {
        std::lock_guard<std::mutex> lk(bridgeMutex());
        backendInvokeFn() = std::move(fn);
    }

    void invalidate(const std::string& key,
                    const std::string& tenant_id = "") override {
        invoke("invalidate", key, tenant_id);
    }

    void invalidateTenant(const std::string& tenant_id) override {
        invoke("invalidate_tenant", "", tenant_id);
    }

    std::string address() const override { return config_.address; }

    bool isHealthy() const override {
        // memory_order fix: acquire pairs with release stores in invoke()
        // to ensure the health state is correctly visible across threads.
        return healthy_.load(std::memory_order_acquire);
    }

private:
    /**
     * @brief Bridge Mutex.
     * @return Return value.
     * @details Implements bridgeMutex without additional internal calls.
     */
    static std::mutex& bridgeMutex() {
        static std::mutex m;
        return m;
    }

    /**
     * @brief Backend Invoke Fn.
     * @return Return value.
     * @details Implements backendInvokeFn without additional internal calls.
     */
    static BackendInvokeFn& backendInvokeFn() {
        static BackendInvokeFn fn;
        return fn;
    }

    /**
     * @brief Invoke.
     * @param[in] type Input parameter.
     * @param[in] key Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: lk(), bridgeMutex(), backendInvokeFn(), store(), fn().
     */
    void invoke(const std::string& type,
                const std::string& key,
                const std::string& tenant_id) {
        BackendInvokeFn fn;
        {
            std::lock_guard<std::mutex> lk(bridgeMutex());
            fn = backendInvokeFn();
        }
        if (!fn) {
            // memory_order fix: release so the false state is visible to
            // any thread polling isHealthy() with acquire semantics.
            healthy_.store(false, std::memory_order_release);
            throw std::runtime_error("GrpcRemoteCachePeer stub: gRPC transport unavailable");
        }
        bool ok = false;
        try {
            ok = fn(config_.address, type, key, tenant_id);
        } catch (...) {
            healthy_.store(false, std::memory_order_release);
            throw;
        }
        healthy_.store(ok, std::memory_order_release);
        if (!ok) {
            throw std::runtime_error("GrpcRemoteCachePeer backend invocation failed");
        }
    }

    Config            config_;
    std::atomic<bool> healthy_{false};
};

}  // namespace cache
}  // namespace themis

#endif  // THEMIS_ENABLE_GRPC
