// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_rpc_server.h"
#include "utils/logger.h"
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
        : handler_(handler) {}
    
    grpc::Status PrepareTransaction(
        grpc::ServerContext* context,
        const themis::sharding::proto::PrepareRequest* request,
        themis::sharding::proto::PrepareResponse* response
    ) override {
        if (!handler_) {
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
        grpc::ServerContext* context,
        const themis::sharding::proto::CommitRequest* request,
        themis::sharding::proto::CommitResponse* response
    ) override {
        if (!handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC CommitTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool success = handler_->onCommit(request->transaction_id());
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
        grpc::ServerContext* context,
        const themis::sharding::proto::AbortRequest* request,
        themis::sharding::proto::AbortResponse* response
    ) override {
        if (!handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC AbortTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool success = handler_->onAbort(request->transaction_id());
            response->set_success(success);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("AbortTransaction failed: {}", e.what());
            response->set_success(false);
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status HealthCheck(
        grpc::ServerContext* context,
        const themis::sharding::proto::HealthRequest* request,
        themis::sharding::proto::HealthResponse* response
    ) override {
        THEMIS_DEBUG("gRPC HealthCheck");
        
        if (handler_) {
            auto health_info = handler_->onHealthCheck();
            response->set_status(health_info.is_healthy ? "healthy" : "unhealthy");
            response->set_version(health_info.version);
            response->set_uptime_seconds(health_info.uptime_seconds);
        } else {
            response->set_status("healthy");
            response->set_version("1.3.4");
            response->set_uptime_seconds(0);
        }
        
        return grpc::Status::OK;
    }
    
    // Additional RPCs (not yet implemented)
    grpc::Status GetShardStatus(
        grpc::ServerContext* context,
        const themis::sharding::proto::StatusRequest* request,
        themis::sharding::proto::StatusResponse* response
    ) override {
        return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "GetShardStatus not yet implemented");
    }
    
private:
    ShardRPCServer::RequestHandler* handler_;
};

#endif // THEMIS_HAS_SHARD_GRPC

struct ShardRPCServer::Impl {
    std::string listen_address;
    RequestHandler* handler = nullptr;
    
#if THEMIS_HAS_SHARD_GRPC
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<ShardServiceImpl> service;
#endif
    
    explicit Impl(const std::string& addr) : listen_address(addr) {}
};

ShardRPCServer::ShardRPCServer(const std::string& listen_address)
    : impl_(std::make_unique<Impl>(listen_address))
{
    THEMIS_INFO("ShardRPCServer created on: {}", listen_address);
}

ShardRPCServer::~ShardRPCServer() {
    stop();
}

void ShardRPCServer::setRequestHandler(RequestHandler* handler) {
    impl_->handler = handler;
}

bool ShardRPCServer::start() {
#if THEMIS_HAS_SHARD_GRPC
    try {
        impl_->service = std::make_unique<ShardServiceImpl>(impl_->handler);
        
        grpc::ServerBuilder builder;
        
        // Configure server with keepalive settings
        builder.AddListeningPort(impl_->listen_address, grpc::InsecureServerCredentials());
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

void ShardRPCServer::wait() {
#if THEMIS_HAS_SHARD_GRPC
    if (impl_->server) {
        impl_->server->Wait();
    }
#endif
}

} // namespace themis::sharding
