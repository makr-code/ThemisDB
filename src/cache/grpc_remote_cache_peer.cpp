/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_remote_cache_peer.cpp                         ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 04:16:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     149                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 0a465da9ae  2026-03-19  fix(cache): address all code review issues for CacheRepli... ║
    • f7f2be3028  2026-03-18  feat(cache): implement network-backed peer discovery for ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifdef THEMIS_ENABLE_GRPC

#include "cache/grpc_remote_cache_peer.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <stdexcept>

namespace themis {
namespace cache {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

GrpcRemoteCachePeer::GrpcRemoteCachePeer(Config config)
    : config_(std::move(config))
{
    auto creds = buildCredentials();
    channel_ = grpc::CreateChannel(config_.address, creds);
    stub_    = std::make_unique<grpc::GenericStub>(channel_);
    THEMIS_DEBUG("[GrpcRemoteCachePeer] created peer for {}", config_.address);
}

GrpcRemoteCachePeer::GrpcRemoteCachePeer(const std::string& addr)
    : GrpcRemoteCachePeer(Config(addr))
{}

// ─────────────────────────────────────────────────────────────────────────────
// IRemoteCachePeer
// ─────────────────────────────────────────────────────────────────────────────

void GrpcRemoteCachePeer::invalidate(const std::string& key,
                                      const std::string& tenant_id) {
    sendRpc("invalidate", key, tenant_id);
}

void GrpcRemoteCachePeer::invalidateTenant(const std::string& tenant_id) {
    sendRpc("invalidate_tenant", "", tenant_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<grpc::ChannelCredentials>
GrpcRemoteCachePeer::buildCredentials() const {
    if (!config_.tls_enabled) {
        return grpc::InsecureChannelCredentials();
    }
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = config_.tls_ca_cert;
    return grpc::SslCredentials(ssl_opts);
}

void GrpcRemoteCachePeer::sendRpc(const std::string& type,
                                   const std::string& key,
                                   const std::string& tenant_id) {
    // Serialize the request as a JSON payload.
    const std::string json_str = nlohmann::json{
        {"type",      type},
        {"key",       key},
        {"tenant_id", tenant_id}
    }.dump();

    // Wrap the JSON string in a grpc::ByteBuffer.
    const grpc::Slice slice(json_str.data(), json_str.size());
    grpc::ByteBuffer request_buf(&slice, 1);

    // Set up the RPC deadline.
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::milliseconds(config_.rpc_timeout_ms));

    // Execute a blocking unary call via the GenericStub.
    grpc::CompletionQueue cq;
    grpc::ByteBuffer      response_buf;
    grpc::Status          status;

    auto rpc = stub_->PrepareUnaryCall(&ctx, kInvalidateMethod, request_buf, &cq);
    rpc->StartCall();
    rpc->Finish(&response_buf, &status, reinterpret_cast<void*>(1));

    void* tag = nullptr;
    bool  ok  = false;
    const bool got_event = cq.Next(&tag, &ok);

    if (!got_event || !ok || tag != reinterpret_cast<void*>(1)) {
        healthy_.store(false, std::memory_order_relaxed);
        const std::string msg =
            "[GrpcRemoteCachePeer] completion queue error for '" + type +
            "' to " + config_.address +
            " (got_event=" + (got_event ? "true" : "false") +
            ", ok=" + (ok ? "true" : "false") + ")";
        THEMIS_WARN("{}", msg);
        throw std::runtime_error(msg);
    }

    if (status.ok()) {
        healthy_.store(true, std::memory_order_relaxed);
        THEMIS_DEBUG("[GrpcRemoteCachePeer] {} ok → peer={}", type, config_.address);
    } else {
        healthy_.store(false, std::memory_order_relaxed);
        const std::string msg =
            "[GrpcRemoteCachePeer] RPC '" + type + "' to " + config_.address +
            " failed: " + status.error_message() +
            " (code=" + std::to_string(static_cast<int>(status.error_code())) + ")";
        THEMIS_WARN("{}", msg);
        throw std::runtime_error(msg);
    }
}

}  // namespace cache
}  // namespace themis

#endif  // THEMIS_ENABLE_GRPC
