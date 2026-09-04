/**
 * @file shard_rpc_server.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_rpc_server.h"
#include "utils/logger.h"
#include "utils/file_utils.h"
#include <stdexcept>

#ifdef THEMIS_ENABLE_GRPC
#if __has_include("shard_rpc.grpc.pb.h")
#include <grpcpp/grpcpp.h>
#include "shard_rpc.grpc.pb.h"
#include "shard_rpc.pb.h"
#define THEMIS_HAS_SHARD_GRPC 1
#else
#define THEMIS_HAS_SHARD_GRPC 0
#endif
#else
#define THEMIS_HAS_SHARD_GRPC 0
#endif

namespace themis::sharding {


#if THEMIS_HAS_SHARD_GRPC

/**
 * @brief Implementation of the gRPC ShardService
 * 
 * Handles incoming RPC requests from other shards for:
 * - Distributed transactions (2PC)
 * - Data replication
 * - Health checks
 */
class ShardServiceImpl final : public themis::sharding::proto::ShardService::Service {
public:
    explicit ShardServiceImpl(ShardRPCServer::RequestHandler* handler)
        : handler_([[maybe_unused]] handler) {}

    // Called once Impl is constructed so we can serve shard identity.
    void setImplRef(const std::string& address) {
        listen_address_ = address;
    }
    
    grpc::Status PrepareTransaction(
        [[maybe_unused]] grpc::ServerContext* context,
        const themis::sharding::proto::PrepareRequest* request,
        themis::sharding::proto::PrepareResponse* response
    ) override {
        if ([[maybe_unused]] !handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC PrepareTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool vote_commit = handler_->onPrepare(
                request->transaction_id(),
                request->coordinator_shard_id(),
                request->transaction_data()
            );
            
            response->set_vote_commit(vote_commit);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("PrepareTransaction failed: {}", e.what());
            response->set_vote_commit(false);
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status CommitTransaction(
        [[maybe_unused]] grpc::ServerContext* context,
        const themis::sharding::proto::CommitRequest* request,
        themis::sharding::proto::CommitResponse* response
    ) override {
        if ([[maybe_unused]] !handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC CommitTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool success = handler_->onCommit([[maybe_unused]] request->transaction_id());
            response->set_success(success);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("CommitTransaction failed: {}", e.what());
            response->set_success(false);
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status AbortTransaction(
        [[maybe_unused]] grpc::ServerContext* context,
        const themis::sharding::proto::AbortRequest* request,
        themis::sharding::proto::AbortResponse* response
    ) override {
        if ([[maybe_unused]] !handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC AbortTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool success = handler_->onAbort([[maybe_unused]] request->transaction_id());
            response->set_success(success);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("AbortTransaction failed: {}", e.what());
            response->set_success(false);
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status HealthCheck(
        [[maybe_unused]] grpc::ServerContext* context,
        [[maybe_unused]] const themis::sharding::proto::HealthRequest* request,
        themis::sharding::proto::HealthResponse* response
    ) override {
        THEMIS_DEBUG("gRPC HealthCheck");
        
        if ([[maybe_unused]] handler_) {
            auto health_info = handler_->onHealthCheck();
            response->set_status(health_info.is_healthy ? "healthy" : "unhealthy");
            response->set_version(health_info.version);
            response->set_uptime_seconds(health_info.uptime_seconds);
        } else {
            response->set_status("healthy");
            // Use compile-time version string
#ifdef THEMIS_VERSION_STRING
            response->set_version(THEMIS_VERSION_STRING);
#else
            response->set_version("unknown");
#endif
            response->set_uptime_seconds(0);
        }
        
        return grpc::Status::OK;
    }

    grpc::Status CollectWaitForEdges(
        [[maybe_unused]] grpc::ServerContext* context,
        [[maybe_unused]] const themis::sharding::proto::CollectWaitForEdgesRequest* request,
        themis::sharding::proto::CollectWaitForEdgesResponse* response
    ) override {
        THEMIS_DEBUG("gRPC CollectWaitForEdges");

        response->set_shard_id(listen_address_);

        if ([[maybe_unused]] handler_) {
            try {
                const auto edges = handler_->onCollectWaitForEdges();
                for (const auto& edge : edges) {
                    auto* proto_edge = response->add_edges();
                    proto_edge->set_waiting_transaction_id(edge.waiting_transaction_id);
                    proto_edge->set_blocking_transaction_id(edge.blocking_transaction_id);
                }
            } catch (const std::exception& e) {
                THEMIS_ERROR("CollectWaitForEdges: onCollectWaitForEdges threw: {}", e.what());
                return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
            }
        }
        // No handler: return an empty edge list (fail-open — no deadlock assumed).

        return grpc::Status::OK;
    }
    
    grpc::Status GetShardStatus(
        [[maybe_unused]] grpc::ServerContext* context,
        const themis::sharding::proto::StatusRequest* request,
        themis::sharding::proto::StatusResponse* response
    ) override {
        THEMIS_DEBUG("gRPC GetShardStatus (include_metrics={})", request->include_metrics());

        // shard_id: use the server's listen address as the stable shard identity
        if (listen_address_.empty()) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                "Shard identity not yet initialised (setImplRef not called)");
        }
        response->set_shard_id(listen_address_);

        // state: derived from health check when handler is available
        if ([[maybe_unused]] handler_) {
            try {
                auto health = handler_->onHealthCheck();
                response->set_state(health.is_healthy ? "healthy" : "unhealthy");
            } catch (const std::exception& e) {
                THEMIS_ERROR("GetShardStatus: health check threw: {}", e.what());
                response->set_state("unknown");
            }
        } else {
            response->set_state("healthy");
        }

        // Token range: advertise the full uint64 range as a sentinel.
        // A full ring-aware implementation would query the ConsistentHashRing here.
        response->set_token_range_start(0);
        response->set_token_range_end(UINT64_MAX);

        return grpc::Status::OK;
    }
    
private:
    ShardRPCServer::RequestHandler* handler_;
    std::string listen_address_;
};

#endif // THEMIS_HAS_SHARD_GRPC

struct ShardRPCServer::Impl {
    std::string listen_address = {};
    RequestHandler* handler = nullptr;
    ShardRPCServer::Config config;  // Store full configuration
    
#if THEMIS_HAS_SHARD_GRPC
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<ShardServiceImpl> service;
#endif
    
    explicit Impl(const std::string& addr) : listen_address(addr) {
        config.listen_address = addr;
    }
    
    explicit Impl(const ShardRPCServer::Config& cfg) : listen_address(cfg.listen_address), config(cfg) {}
};

/**
 * @brief Construct shard RPC server with listen address convenience API.
 * @param listen_address Server bind address.
 */
ShardRPCServer::ShardRPCServer(const std::string& listen_address)
    : impl_(std::make_unique<Impl>(listen_address))
{
    THEMIS_INFO("ShardRPCServer created on: {}", listen_address);
}

/**
 * @brief Construct shard RPC server with explicit TLS/runtime configuration.
 * @param config Server configuration.
 */
ShardRPCServer::ShardRPCServer(const Config& config)
    : impl_(std::make_unique<Impl>(config))
{
    THEMIS_INFO("ShardRPCServer created on: {} (mTLS: {})", config.listen_address, config.enable_mtls);
}

/** @brief Destroy server instance and ensure shutdown of active gRPC server. */
ShardRPCServer::~ShardRPCServer() {
    stop();
}

/** @brief Install application request handler used by incoming RPC methods. */
void ShardRPCServer::setRequestHandler([[maybe_unused]] RequestHandler* handler) {
    impl_->handler = handler;
}

/**
 * @brief Start gRPC server and bind to configured endpoint.
 * @return True when server starts successfully; false otherwise.
 */
bool ShardRPCServer::start() {
#if THEMIS_HAS_SHARD_GRPC
    try {
        impl_->service = std::make_unique<ShardServiceImpl>([[maybe_unused]] impl_->handler);
        impl_->service->setImplRef(impl_->listen_address);
        
        grpc::ServerBuilder builder;
        
        // Configure server credentials
        std::shared_ptr<grpc::ServerCredentials> credentials;
        
        if (impl_->config.enable_mtls) {
            // mTLS enabled - create SSL server credentials
            try {
                grpc::SslServerCredentialsOptions ssl_opts;
                
                // Load CA certificate for client verification
                if (!impl_->config.tls_ca_cert_path.empty()) {
                    ssl_opts.pem_root_certs = themis::utils::readFileContents(impl_->config.tls_ca_cert_path);
                    THEMIS_INFO("Loaded CA certificate from: {}", impl_->config.tls_ca_cert_path);
                }
                
                // Load server certificate and private key
                if (!impl_->config.tls_cert_path.empty() && !impl_->config.tls_key_path.empty()) {
                    grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
                    key_cert_pair.private_key = themis::utils::readFileContents(impl_->config.tls_key_path);
                    key_cert_pair.cert_chain = themis::utils::readFileContents(impl_->config.tls_cert_path);
                    ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
                    THEMIS_INFO("Loaded server certificate from: {}", impl_->config.tls_cert_path);
                }
                
                // Configure client certificate requirement
                if (impl_->config.tls_require_client_cert) {
                    ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
                    THEMIS_INFO("Client certificate verification enabled (mutual TLS)");
                } else {
                    ssl_opts.client_certificate_request = GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
                }
                
                credentials = grpc::SslServerCredentials(ssl_opts);
                THEMIS_INFO("mTLS enabled for shard RPC server");
                
            } catch (const std::exception& e) {
                // GAP-016: mTLS cert load failure silently fell back to insecure.
                // Log at ERROR so operators see the degradation (CWE-295).
                THEMIS_ERROR("[SECURITY] ShardRPCServer: Failed to load mTLS certificates: {}. "
                             "Falling back to INSECURE connection (GAP-016/CWE-295).", e.what());
                credentials = grpc::InsecureServerCredentials();
            }
        } else {
            // mTLS not enabled - check production mode enforcement
            if (const char* prod = getenv("THEMIS_PRODUCTION_MODE"); prod && std::string(prod) == "1") {
                if (const char* override_flag = getenv("THEMIS_SHARD_MTLS_DISABLED");
                    override_flag && std::string(override_flag) == "1") {
                    THEMIS_WARN("ShardRPCServer: mTLS disabled in production mode via "
                                "THEMIS_SHARD_MTLS_DISABLED=1 — this is INSECURE and for dev/test only.");
                } else {
                    throw std::runtime_error(
                        "ShardRPCServer: mTLS must be enabled in production mode "
                        "(THEMIS_PRODUCTION_MODE=1). Set enable_mtls=true or set "
                        "THEMIS_SHARD_MTLS_DISABLED=1 to explicitly override (insecure, dev only).");
                }
            }
            // mTLS not enabled - use insecure credentials (development only)
            credentials = grpc::InsecureServerCredentials();
            THEMIS_WARN("mTLS is disabled for shard RPC server. This is insecure and should only be used in development.");
        }
        
        builder.AddListeningPort(impl_->listen_address, credentials);
        builder.RegisterService(impl_->service.get());
        
        // Keepalive settings (match client settings)
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        
        // Connection settings
        builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS, 300000);  // 5 minutes
        builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_AGE_MS, 3600000);  // 1 hour
        
        impl_->server = builder.BuildAndStart();
        
        if (impl_->server) {
            THEMIS_INFO("gRPC ShardService listening on: {}", impl_->listen_address);
            return true;
        } else {
            THEMIS_ERROR("Failed to start gRPC ShardService");
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to start gRPC server: {}", e.what());
        return false;
    }
#else
    THEMIS_WARN("gRPC support not available, ShardRPCServer not started");
    return false;
#endif
}

/** @brief Stop gRPC server and release registered service instance. */
void ShardRPCServer::stop() {
#if THEMIS_HAS_SHARD_GRPC
    if (impl_->server) {
        THEMIS_INFO("Stopping gRPC ShardService");
        impl_->server->Shutdown();
        impl_->server.reset();
    }
    impl_->service.reset();
#endif
}

/** @brief Block until server shutdown completes. */
void ShardRPCServer::wait() {
#if THEMIS_HAS_SHARD_GRPC
    if (impl_->server) {
        impl_->server->Wait();
    }
#endif
}

} // namespace themis::sharding
