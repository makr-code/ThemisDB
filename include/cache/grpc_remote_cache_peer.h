/**
 * @file grpc_remote_cache_peer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief gRPC-backed IRemoteCachePeer for cross-node cache invalidation.
 *
 * Each instance holds a gRPC channel to one peer node.  Calls to
 * invalidate() / invalidateTenant() serialize the request as JSON, send it
 * as a blocking unary RPC, and update the internal health state.
 *
 * Intended to be constructed by the PeerFactory injected into
 * CacheReplicationCoordinator; the factory is called from refreshPeers()
 * on the coordinator's construction thread, not on the fanout worker thread.
 * All IRemoteCachePeer methods are then called from the fanout worker thread.
 *
 * Thread-safety: invalidate() and invalidateTenant() are thread-safe if
 * called from a single fanout thread; concurrent calls from multiple threads
 * are NOT supported (each peer is owned by a single coordinator).
 */
class GrpcRemoteCachePeer final : public IRemoteCachePeer {
public:
    /// gRPC method identifier used for cache invalidation RPCs.
    static constexpr const char* kInvalidateMethod =
        "/themis.cache.v1.CacheInvalidation/Invalidate";

    // ── Configuration ─────────────────────────────────────────────────────────

    struct Config {
        /// Peer address in "host:port" format.
        std::string address;

        /// RPC deadline in milliseconds (default: 1 000 ms).
        int rpc_timeout_ms = 1000;

        /// TLS: set to true and provide CA cert path to use SSL credentials.
        /// When false, insecure channel credentials are used (not recommended
        /// for production).
        bool        tls_enabled   = false;
        std::string tls_ca_cert;  ///< PEM-encoded CA certificate (in-memory)

        Config() = default;
        explicit Config(std::string addr) : address(std::move(addr)) {}
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Construct a peer using a configuration struct.
     *
     * The gRPC channel is created eagerly; the first RPC will establish the
     * connection.
     */
    explicit GrpcRemoteCachePeer(Config config);

    /**
     * @brief Convenience constructor: insecure peer at the given address.
     *
     * Equivalent to GrpcRemoteCachePeer(Config(addr)).
     */
    explicit GrpcRemoteCachePeer(const std::string& addr);

    ~GrpcRemoteCachePeer() override = default;

    // ── IRemoteCachePeer ──────────────────────────────────────────────────────

    /**
     * @brief Send a key-pattern invalidation RPC to the remote peer.
     *
     * @param key       Cache key or glob pattern to invalidate.
     * @param tenant_id Tenant scope; empty = global.
     * @throws std::runtime_error on RPC failure (caller should catch and retry).
     */
    void invalidate(const std::string& key,
                    const std::string& tenant_id = "") override;

    /**
     * @brief Send a tenant-wide invalidation RPC to the remote peer.
     *
     * @param tenant_id Tenant identifier; must not be empty.
     * @throws std::runtime_error on RPC failure (caller should catch and retry).
     */
    void invalidateTenant(const std::string& tenant_id) override;

    std::string address() const override { return config_.address; }

    bool isHealthy() const override {
        // memory_order fix: acquire pairs with release stores in sendRpc()
        // to ensure the health state written after an RPC outcome is visible.
        return healthy_.load(std::memory_order_acquire);
    }

private:
    std::shared_ptr<grpc::ChannelCredentials> buildCredentials() const;

    /**
     * @brief Execute a blocking unary RPC with the given JSON payload.
     *
     * @throws std::runtime_error when the RPC fails or times out.
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

/** @brief Grpc remote cache peer. */
class GrpcRemoteCachePeer final : public IRemoteCachePeer {
public:
    using BackendInvokeFn = std::function<bool(const std::string& address,
                                               const std::string& type,
                                               const std::string& key,
                                               const std::string& tenant_id)>;

    static constexpr const char* kInvalidateMethod =
        "/themis.cache.v1.CacheInvalidation/Invalidate";

    struct Config {
        std::string address;
        int rpc_timeout_ms = 1000;
        bool        tls_enabled   = false;
        std::string tls_ca_cert;

        Config() = default;
        explicit Config(std::string addr) : address(std::move(addr)) {}
    };

    explicit GrpcRemoteCachePeer(Config config)
        : config_(std::move(config)) {}

    explicit GrpcRemoteCachePeer(const std::string& addr)
        : GrpcRemoteCachePeer(Config(addr)) {}

    ~GrpcRemoteCachePeer() override = default;

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
    static std::mutex& bridgeMutex() {
        static std::mutex m;
        return m;
    }

    static BackendInvokeFn& backendInvokeFn() {
        static BackendInvokeFn fn;
        return fn;
    }

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
