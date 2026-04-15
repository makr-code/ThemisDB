/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_remote_cache_peer.h                           ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 18:02:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     168                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • f7f2be3028  2026-03-18  feat(cache): implement network-backed peer discovery for ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

#ifdef THEMIS_ENABLE_GRPC

#include "cache/cache_replication_coordinator.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/security/credentials.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

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
        return healthy_.load(std::memory_order_relaxed);
    }

private:
    /// Build gRPC channel credentials from config_.
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

#endif  // THEMIS_ENABLE_GRPC
