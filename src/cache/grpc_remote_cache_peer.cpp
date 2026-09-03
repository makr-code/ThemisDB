/**
 * @file grpc_remote_cache_peer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.12
 * @date 2026-06-02 11:49:05
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 78/100
 * @note Lines: 150
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note PR History (last 5): #4330 feat(cache): network-backed... (2026-03-19)
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

// PERMANENT FALLBACK NOTE:
// Purpose: Allow the cache module to be built without gRPC.  When
//   `THEMIS_ENABLE_GRPC` is not defined, this entire translation unit is
//   excluded from compilation.  Any class or function defined here
//   (GrpcRemoteCachePeer) is expected to be provided as an inline no-op fallback
//   in include/cache/grpc_remote_cache_peer.h.  Callers that attempt to use
//   the remote cache peer in a non-gRPC build will get a fallback that reports
//   itself as unhealthy and returns error for every operation.
// Activation: `THEMIS_ENABLE_GRPC` not defined at compile time.
// Production Delta: Cross-node cache replication/invalidation via gRPC is
//   disabled.  Each ThemisDB instance operates as a fully isolated cache
//   island; distributed invalidation events are silently dropped.
// This fallback is PERMANENT for no-gRPC builds.  Install gRPC C++ libraries
//   and set `-DTHEMIS_ENABLE_GRPC=1` to activate the real gRPC client below.
// Roadmap ref: src/cache/FUTURE_ENHANCEMENTS.md (gRPC peer activation — planned)

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
    if (!config_.tls_enabled && !config_.allow_insecure) {
        throw std::runtime_error(
            "GrpcRemoteCachePeer: insecure transport is disabled by default; "
            "set tls_enabled=true or allow_insecure=true for an explicit local/test override");
    }

    if (!config_.tls_enabled) {
        THEMIS_WARN("GrpcRemoteCachePeer: explicit insecure override enabled for {} — "
                    "this is a local/test-only exception and must not be used in production",
                    config_.address);
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
        // memory_order fix: use release so the health state is visible to any
        // thread that subsequently reads healthy_ with acquire semantics.
        healthy_.store(false, std::memory_order_release);
        const std::string msg =
            "[GrpcRemoteCachePeer] completion queue error for '" + type +
            "' to " + config_.address +
            " (got_event=" + (got_event ? "true" : "false") +
            ", ok=" + (ok ? "true" : "false") + ")";
        THEMIS_WARN("{}", msg);
        throw std::runtime_error(msg);
    }

    if (status.ok()) {
        healthy_.store(true, std::memory_order_release);
        THEMIS_DEBUG("[GrpcRemoteCachePeer] {} ok → peer={}", type, config_.address);
    } else {
        healthy_.store(false, std::memory_order_release);
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

