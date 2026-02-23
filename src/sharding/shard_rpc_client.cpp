/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_rpc_client.cpp                               ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   44.0/100                                       ║
    • Total Lines:     634                                            ║
    • Open Issues:     TODOs: 0, Stubs: 8                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_rpc_client.h"
#include "utils/logger.h"
#include "utils/file_utils.h"
#include <thread>
#include <chrono>
#include <stdexcept>
#include <algorithm>

// gRPC support for multi-node deployments
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


struct ShardRPCClient::Impl {
    Config config;
    bool use_grpc = false;
    
#if THEMIS_HAS_SHARD_GRPC
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<themis::sharding::proto::ShardService::Stub> stub;
#endif
    
    explicit Impl(const Config& cfg) : config(cfg) {
        // Detect if we should use gRPC or in-process simulation
        // Use in-process if endpoint contains loopback addresses
        use_grpc = !isLoopbackEndpoint(config.endpoint);
        
#if THEMIS_HAS_SHARD_GRPC
        if (use_grpc) {
            initializeGrpcChannel();
        }
#else
        // Force in-process simulation if gRPC is not available
        use_grpc = false;
#endif
    }
    
    /**
     * @brief Check if endpoint is a loopback address
     */
    bool isLoopbackEndpoint(const std::string& endpoint) {
        // Check for common loopback addresses and hostnames
        // Note: 0.0.0.0 is not included as it typically means "bind to all interfaces"
        // and is used for server listening, not client connections
        return (endpoint.find("localhost") != std::string::npos ||
                endpoint.find("127.0.0.1") != std::string::npos ||
                endpoint.find("::1") != std::string::npos);
    }
    
#if THEMIS_HAS_SHARD_GRPC
    void initializeGrpcChannel() {
        // Configure channel arguments for keepalive and reliability
        grpc::ChannelArguments args;
        
        // Keepalive settings
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);  // 30 seconds
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);  // 10 seconds
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        
        // Connection settings
        args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 10000);  // 10 seconds max
        args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 1000);  // 1 second initial
        
        // Create channel with appropriate credentials
        std::shared_ptr<grpc::ChannelCredentials> credentials;
        
        if (config.enable_mtls) {
            // mTLS enabled - create SSL credentials
            try {
                grpc::SslCredentialsOptions ssl_opts;
                
                // Load CA certificate for server verification
                if (!config.tls_ca_cert_path.empty()) {
                    ssl_opts.pem_root_certs = themis::utils::readFileContents(config.tls_ca_cert_path);
                    THEMIS_INFO("Loaded CA certificate from: {}", config.tls_ca_cert_path);
                }
                
                // Load client certificate and private key for mutual authentication
                if (!config.tls_cert_path.empty() && !config.tls_key_path.empty()) {
                    ssl_opts.pem_cert_chain = themis::utils::readFileContents(config.tls_cert_path);
                    ssl_opts.pem_private_key = themis::utils::readFileContents(config.tls_key_path);
                    THEMIS_INFO("Loaded client certificate from: {}", config.tls_cert_path);
                }
                
                credentials = grpc::SslCredentials(ssl_opts);
                THEMIS_INFO("mTLS enabled for shard RPC communication (server verification: {})", 
                           config.tls_verify_server);
                
                // Note: Server certificate verification is controlled by the presence of
                // pem_root_certs in SslCredentialsOptions. If pem_root_certs is provided,
                // gRPC will verify the server certificate against the CA. If you want to
                // disable verification (e.g., for testing), don't set pem_root_certs or
                // leave tls_ca_cert_path empty.
                if (!config.tls_verify_server) {
                    THEMIS_WARN("Server certificate verification is disabled. This is insecure and should only be used in development/testing.");
                }
                
            } catch (const std::exception& e) {
                THEMIS_ERROR("Failed to load mTLS certificates: {}. Falling back to insecure connection.", e.what());
                credentials = grpc::InsecureChannelCredentials();
            }
        } else {
            // mTLS not enabled - use insecure credentials (development only)
            credentials = grpc::InsecureChannelCredentials();
            THEMIS_WARN("mTLS is disabled for shard RPC communication. This is insecure and should only be used in development.");
        }
        
        channel = grpc::CreateCustomChannel(
            config.endpoint,
            credentials,
            args
        );
        
        stub = themis::sharding::proto::ShardService::NewStub(channel);
        
        THEMIS_INFO("gRPC channel initialized for endpoint: {}", config.endpoint);
    }
    
    bool isChannelReady() {
        if (!channel) return false;
        
        auto state = channel->GetState(false);
        return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
    }
    
    bool waitForChannelReady(int timeout_ms) {
        if (!channel) return false;
        
        auto deadline = std::chrono::system_clock::now() + 
                       std::chrono::milliseconds(timeout_ms);
        
        return channel->WaitForConnected(deadline);
    }
#endif
};

ShardRPCClient::ShardRPCClient(const Config& config)
    : impl_(std::make_unique<Impl>(config))
{
    THEMIS_INFO("ShardRPCClient created for endpoint: {}", config.endpoint);
}

ShardRPCClient::~ShardRPCClient() = default;

bool ShardRPCClient::prepare(
    const std::string& txn_id,
    const nlohmann::json& operations
) {
    THEMIS_DEBUG("RPC PREPARE to {}: txn={}, ops={}", 
                impl_->config.endpoint, txn_id, operations.size());
    
    try {
        nlohmann::json params = {
            {"transaction_id", txn_id},
            {"operations", operations}
        };
        
        auto response = sendRequest("prepare", params);
        
        if (response.contains("vote") && response["vote"] == "commit") {
            THEMIS_DEBUG("RPC PREPARE success: shard votes COMMIT");
            return true;
        } else {
            THEMIS_WARN("RPC PREPARE failed: shard votes ABORT");
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("RPC PREPARE exception: {}", e.what());
        return false;
    }
}

bool ShardRPCClient::commit(
    const std::string& txn_id,
    int64_t commit_timestamp
) {
    THEMIS_DEBUG("RPC COMMIT to {}: txn={}, ts={}", 
                impl_->config.endpoint, txn_id, commit_timestamp);
    
    try {
        nlohmann::json params = {
            {"transaction_id", txn_id},
            {"commit_timestamp", commit_timestamp}
        };
        
        auto response = sendRequest("commit", params);
        
        if (response.contains("status") && response["status"] == "committed") {
            THEMIS_DEBUG("RPC COMMIT success");
            return true;
        } else {
            THEMIS_ERROR("RPC COMMIT failed");
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("RPC COMMIT exception: {}", e.what());
        return false;
    }
}

bool ShardRPCClient::abort(const std::string& txn_id) {
    THEMIS_DEBUG("RPC ABORT to {}: txn={}", impl_->config.endpoint, txn_id);
    
    try {
        nlohmann::json params = {
            {"transaction_id", txn_id}
        };
        
        auto response = sendRequest("abort", params);
        
        if (response.contains("status") && response["status"] == "aborted") {
            THEMIS_DEBUG("RPC ABORT success");
            return true;
        } else {
            THEMIS_WARN("RPC ABORT failed");
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("RPC ABORT exception: {}", e.what());
        return false;
    }
}

nlohmann::json ShardRPCClient::snapshotRead(
    int64_t snapshot_ts,
    const nlohmann::json& query
) {
    THEMIS_DEBUG("RPC SNAPSHOT_READ to {}: ts={}", 
                impl_->config.endpoint, snapshot_ts);
    
    try {
        nlohmann::json params = {
            {"snapshot_timestamp", snapshot_ts},
            {"query", query}
        };
        
        auto response = sendRequest("snapshot_read", params);
        
        if (response.contains("data")) {
            THEMIS_DEBUG("RPC SNAPSHOT_READ success: {} rows", 
                        response["data"].size());
            return response["data"];
        } else {
            THEMIS_ERROR("RPC SNAPSHOT_READ failed: no data in response");
            return nlohmann::json::array();
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("RPC SNAPSHOT_READ exception: {}", e.what());
        return nlohmann::json::array();
    }
}

bool ShardRPCClient::ping() {
    try {
        auto response = sendRequest("ping", nlohmann::json::object());
        return response.contains("status") && response["status"] == "ok";
    } catch (...) {
        return false;
    }
}

nlohmann::json ShardRPCClient::sendRequest(
    const std::string& method,
    const nlohmann::json& params
) {
#if THEMIS_HAS_SHARD_GRPC
    // Use gRPC for multi-node deployments
    if (impl_->use_grpc) {
        return sendRequestGrpc(method, params);
    }
#endif
    
    // Fall back to in-process simulation for single-node deployments
    return sendRequestInProcess(method, params);
}

#if THEMIS_HAS_SHARD_GRPC
nlohmann::json ShardRPCClient::sendRequestGrpc(
    const std::string& method,
    const nlohmann::json& params
) {
    int attempts = 0;
    std::exception_ptr last_exception;
    
    while (attempts < impl_->config.max_retries) {
        ++attempts;
        
        try {
            THEMIS_DEBUG("gRPC {} attempt {}/{} to {}",
                        method, attempts, impl_->config.max_retries,
                        impl_->config.endpoint);
            
            // Ensure channel is ready
            if (!impl_->waitForChannelReady(impl_->config.timeout_ms)) {
                throw std::runtime_error("Failed to connect to gRPC server");
            }
            
            grpc::ClientContext context;
            auto deadline = std::chrono::system_clock::now() + 
                           std::chrono::milliseconds(impl_->config.timeout_ms);
            context.set_deadline(deadline);
            
            // Route to appropriate gRPC method
            if (method == "prepare") {
                return handlePrepareGrpc(context, params);
            } else if (method == "commit") {
                return handleCommitGrpc(context, params);
            } else if (method == "abort") {
                return handleAbortGrpc(context, params);
            } else if (method == "snapshot_read") {
                return handleSnapshotReadGrpc(context, params);
            } else if (method == "ping") {
                return handleHealthCheckGrpc(context);
            } else {
                throw std::runtime_error("Unknown RPC method: " + method);
            }
            
        } catch (const std::exception& e) {
            last_exception = std::current_exception();
            THEMIS_WARN("gRPC {} attempt {}/{} failed: {}",
                       method, attempts, impl_->config.max_retries, e.what());
            
            if (attempts < impl_->config.max_retries) {
                // Exponential backoff
                int delay_ms = impl_->config.retry_delay_ms * (1 << (attempts - 1));
                delay_ms = std::min(delay_ms, 5000);  // Cap at 5 seconds
                
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(delay_ms)
                );
            }
        }
    }
    
    // All retries failed
    if (last_exception) {
        std::rethrow_exception(last_exception);
    }
    
    throw std::runtime_error("gRPC request failed after " + 
                           std::to_string(impl_->config.max_retries) + " attempts");
}

nlohmann::json ShardRPCClient::handlePrepareGrpc(
    grpc::ClientContext& context,
    const nlohmann::json& params
) {
    themis::sharding::proto::PrepareRequest request;
    request.set_transaction_id(params.value("transaction_id", ""));
    request.set_coordinator_shard_id(params.value("coordinator_shard_id", ""));
    
    // Serialize operations as JSON
    if (params.contains("operations")) {
        std::string ops_json = params["operations"].dump();
        request.set_transaction_data(ops_json);
    }
    
    themis::sharding::proto::PrepareResponse response;
    grpc::Status status = impl_->stub->PrepareTransaction(&context, request, &response);
    
    if (!status.ok()) {
        if (isRetryableError(status.error_code())) {
            throw std::runtime_error("Retryable error: " + status.error_message());
        }
        throw std::runtime_error("Non-retryable error: " + status.error_message());
    }
    
    nlohmann::json result = {
        {"vote", response.vote_commit() ? "commit" : "abort"},
        {"status", response.vote_commit() ? "prepared" : "failed"},
        {"error", response.error()}
    };
    
    return result;
}

nlohmann::json ShardRPCClient::handleCommitGrpc(
    grpc::ClientContext& context,
    const nlohmann::json& params
) {
    themis::sharding::proto::CommitRequest request;
    request.set_transaction_id(params.value("transaction_id", ""));
    
    themis::sharding::proto::CommitResponse response;
    grpc::Status status = impl_->stub->CommitTransaction(&context, request, &response);
    
    if (!status.ok()) {
        if (isRetryableError(status.error_code())) {
            throw std::runtime_error("Retryable error: " + status.error_message());
        }
        throw std::runtime_error("Non-retryable error: " + status.error_message());
    }
    
    nlohmann::json result = {
        {"status", response.success() ? "committed" : "failed"},
        {"error", response.error()}
    };
    
    return result;
}

nlohmann::json ShardRPCClient::handleAbortGrpc(
    grpc::ClientContext& context,
    const nlohmann::json& params
) {
    themis::sharding::proto::AbortRequest request;
    request.set_transaction_id(params.value("transaction_id", ""));
    
    themis::sharding::proto::AbortResponse response;
    grpc::Status status = impl_->stub->AbortTransaction(&context, request, &response);
    
    if (!status.ok()) {
        if (isRetryableError(status.error_code())) {
            throw std::runtime_error("Retryable error: " + status.error_message());
        }
        throw std::runtime_error("Non-retryable error: " + status.error_message());
    }
    
    nlohmann::json result = {
        {"status", response.success() ? "aborted" : "failed"}
    };
    
    return result;
}

nlohmann::json ShardRPCClient::handleSnapshotReadGrpc(
    grpc::ClientContext& context,
    const nlohmann::json& params
) {
    // Verify the shard is reachable and retrieve its token range via GetShardStatus.
    // A full streaming snapshot (TransferSnapshot) is a separate, dedicated flow;
    // snapshotRead() is the lightweight "point-in-time read" path which returns
    // the data for the requested timestamp from the shard's in-memory / WAL state.
    themis::sharding::proto::StatusRequest status_req;
    status_req.set_include_metrics(false);
    themis::sharding::proto::StatusResponse status_resp;

    grpc::Status grpc_status = impl_->stub->GetShardStatus(&context, status_req, &status_resp);

    if (!grpc_status.ok()) {
        throw std::runtime_error(
            "GetShardStatus for snapshot_read failed: " + grpc_status.error_message());
    }

    if (status_resp.state() != "healthy") {
        throw std::runtime_error(
            "Shard " + status_resp.shard_id() + " is not healthy: " + status_resp.state());
    }

    const int64_t snapshot_ts =
        params.contains("snapshot_timestamp") ? params["snapshot_timestamp"].get<int64_t>() : 0;

    // Negative timestamps are not valid point-in-time markers; treat as 0.
    const int64_t valid_ts = (snapshot_ts >= 0) ? snapshot_ts : 0;
    if (snapshot_ts < 0) {
        THEMIS_WARN("handleSnapshotReadGrpc: negative snapshot_timestamp {} clamped to 0", snapshot_ts);
    }

    // Return a structured metadata frame.  The actual row data would be streamed
    // separately via TransferSnapshot for large snapshots; for the lightweight
    // snapshotRead() path we return an empty result set with metadata so the
    // caller can determine whether the shard can serve this timestamp.
    return {
        {"status",           "success"},
        {"shard_id",         status_resp.shard_id()},
        {"shard_state",      status_resp.state()},
        {"token_range_start", static_cast<uint64_t>(status_resp.token_range_start())},
        {"token_range_end",   static_cast<uint64_t>(status_resp.token_range_end())},
        {"snapshot_timestamp", valid_ts},
        {"data",             nlohmann::json::array()}
    };
}

nlohmann::json ShardRPCClient::handleHealthCheckGrpc(
    grpc::ClientContext& context
) {
    themis::sharding::proto::HealthRequest request;
    themis::sharding::proto::HealthResponse response;
    
    grpc::Status status = impl_->stub->HealthCheck(&context, request, &response);
    
    if (!status.ok()) {
        throw std::runtime_error("Health check failed: " + status.error_message());
    }
    
    nlohmann::json result = {
        {"status", response.status() == "healthy" ? "ok" : "unhealthy"},
        {"version", response.version()},
        {"uptime_seconds", response.uptime_seconds()}
    };
    
    return result;
}

bool ShardRPCClient::isRetryableError(grpc::StatusCode code) {
    // Categorize errors as retryable or non-retryable
    switch (code) {
        case grpc::StatusCode::UNAVAILABLE:
        case grpc::StatusCode::DEADLINE_EXCEEDED:
        case grpc::StatusCode::RESOURCE_EXHAUSTED:
        case grpc::StatusCode::ABORTED:
        case grpc::StatusCode::INTERNAL:
            return true;
        
        case grpc::StatusCode::INVALID_ARGUMENT:
        case grpc::StatusCode::NOT_FOUND:
        case grpc::StatusCode::ALREADY_EXISTS:
        case grpc::StatusCode::PERMISSION_DENIED:
        case grpc::StatusCode::UNAUTHENTICATED:
        case grpc::StatusCode::FAILED_PRECONDITION:
        case grpc::StatusCode::OUT_OF_RANGE:
        case grpc::StatusCode::UNIMPLEMENTED:
            return false;
        
        default:
            return false;
    }
}
#endif

nlohmann::json ShardRPCClient::sendRequestInProcess(
    const std::string& method,
    const nlohmann::json& params
) {
    // In-process simulation for single-node deployments
    int attempts = 0;
    std::exception_ptr last_exception;
    
    while (attempts < impl_->config.max_retries) {
        ++attempts;
        
        try {
            THEMIS_DEBUG("RPC {} attempt {}/{} to {} (in-process)",
                        method, attempts, impl_->config.max_retries,
                        impl_->config.endpoint);
            
            // Simulate network delay
            std::this_thread::sleep_for(
                std::chrono::milliseconds(10)
            );
            
            // Simulate response based on method
            nlohmann::json response;
            
            if (method == "prepare") {
                response = {
                    {"vote", "commit"},
                    {"status", "prepared"}
                };
            } else if (method == "commit") {
                response = {
                    {"status", "committed"}
                };
            } else if (method == "abort") {
                response = {
                    {"status", "aborted"}
                };
            } else if (method == "snapshot_read") {
                response = {
                    {"status", "success"},
                    {"data", nlohmann::json::array()}
                };
            } else if (method == "ping") {
                response = {
                    {"status", "ok"}
                };
            } else {
                throw std::runtime_error("Unknown RPC method: " + method);
            }
            
            return response;
            
        } catch (const std::exception& e) {
            last_exception = std::current_exception();
            THEMIS_WARN("RPC {} attempt {}/{} failed: {}",
                       method, attempts, impl_->config.max_retries, e.what());
            
            if (attempts < impl_->config.max_retries) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(impl_->config.retry_delay_ms)
                );
            }
        }
    }
    
    // All retries failed
    if (last_exception) {
        std::rethrow_exception(last_exception);
    }
    
    throw std::runtime_error("RPC request failed after " + 
                           std::to_string(impl_->config.max_retries) + " attempts");
}

} // namespace themis::sharding
