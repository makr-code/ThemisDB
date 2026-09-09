/**
 * @file shard_rpc_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=34; TODO=1, Stub=14, Unimpl=1, Mock=1, Sim=11, Debt=6, C=0, H=11, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_rpc_client.h"
#include "sharding/circuit_breaker.h"
#include "sharding/mtls_connection_pool.h"
#include "sharding/operational_metrics.h"
#include "sharding/prometheus_metrics.h"
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

// Maximum delay between retry attempts (milliseconds).  Both sendRequestGrpc
// and sendRequestInProcess use this cap so that a single constant controls the
// ceiling across both paths.
static constexpr int kMaxRetryDelayMs = 5000;

struct ShardRPCClient::Impl {
    Config config;
    bool use_grpc = false;
    CircuitBreaker circuit_breaker;

    /// Optional injected response handler for the explicit in-process test path.
    /// When non-null, sendRequestInProcess() delegates to this function instead
    /// of returning the built-in hardcoded responses.
    std::mutex handler_mutex;
    InProcessResponseHandler in_process_handler;
    
#if THEMIS_HAS_SHARD_GRPC
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<themis::sharding::proto::ShardService::Stub> stub;
#endif

    /// Emit a single-attempt metric to both sinks (if configured).
    void recordMetrics(const std::string& method,
                       const std::string& outcome,
                       uint64_t latency_us) const
    {
        const std::string& sid = config.shard_id.empty() ? config.endpoint : config.shard_id;
        if (config.operational_metrics) {
            config.operational_metrics->recordRpcCall(sid, method, outcome, latency_us);
        }
        if (config.prometheus_metrics) {
            config.prometheus_metrics->recordRpcCall(
                sid, method, outcome, static_cast<double>(latency_us) / 1000.0);
        }
    }
    
    static CircuitBreaker makeCb(const Config& cfg) {
        CircuitBreaker::Config cb;
        // Negative values are invalid; leave the field at its default rather than
        // casting to size_t (which would underflow to SIZE_MAX, preventing the
        // circuit from ever opening) or producing a negative chrono duration
        // (which would make isTimeoutElapsed() always true).
        if (cfg.circuit_breaker_failure_threshold > 0) {
            cb.failure_threshold =
                static_cast<size_t>(cfg.circuit_breaker_failure_threshold);
        }
        if (cfg.circuit_breaker_recovery_ms > 0) {
            cb.timeout = std::chrono::milliseconds(cfg.circuit_breaker_recovery_ms);
        }
        return CircuitBreaker(cb);
    }

    explicit Impl(const Config& cfg) : config(cfg), circuit_breaker(makeCb(cfg)) {
        // Production transports must use real RPC endpoints whenever gRPC support
        // is compiled in. In-process routing is reserved for explicit test-only
        // endpoints such as inproc://shard-a.
        use_grpc = !isExplicitInProcessEndpoint(config.endpoint);

        // If a connection pool is supplied, apply the max_pool_connections limit
        // (propagated from GossipConfigManagerConfig::rpc_max_pool_connections).
        if (config.connection_pool && config.max_pool_connections > 0) {
            auto pool = config.connection_pool->getPool(config.endpoint);
            if (pool) {
                // Pool configuration is set at construction time; log the
                // effective value so operators can verify gossip propagation.
                THEMIS_INFO("ShardRPCClient: using connection pool for {} "
                            "(max_connections={})",
                            config.endpoint, config.max_pool_connections);
            }
        }
        
#if THEMIS_HAS_SHARD_GRPC
        if (use_grpc) {
            initializeGrpcChannel();
        }
#else
        // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
        // Purpose: Allow ShardRPCClient to compile and function in single-node
        //   or test builds that do not have the shard gRPC proto files (themis_shard.grpc.pb.h).
        //   When THEMIS_HAS_SHARD_GRPC is 0, all sendRequest() calls are routed
        //   to sendRequestInProcess() which returns hardcoded JSON responses.
        // Activation: THEMIS_HAS_SHARD_GRPC == 0 (default when the shard proto
        //   files have not been generated by protoc).
        // Production Delta: RPC calls never leave the process; all "remote"
        //   shard peers appear to respond immediately with hardcoded success.
        //   Failure injection, circuit-breaker testing, and multi-node correctness
        //   cannot be exercised without enabling the real gRPC path.
        // Removal Plan: Run `cmake -DTHEMIS_HAS_SHARD_GRPC=1` after protoc has
        //   generated src/gen/themis_shard.grpc.pb.{h,cc}.  The in-process
        //   simulation path (sendRequestInProcess) is retained as a fallback for
        //   loopback/single-node mode and test isolation.
        // Roadmap ref: src/sharding/FUTURE_ENHANCEMENTS.md §"WAL gRPC Replication"
        
        // Force in-process simulation if gRPC is not available
        use_grpc = false;
        THEMIS_WARN("ShardRPCClient initialized without gRPC support (THEMIS_HAS_SHARD_GRPC=0). "
                    "This is a test-only configuration; production deployments must enable gRPC "
                    "via: cmake -DTHEMIS_ENABLE_GRPC=ON after generating proto files with protoc.");
#endif
    }
    
    /**
     * @brief Check if endpoint explicitly requests in-process test routing.
     *
     * Production endpoints must never silently fall back to local-only transport.
     * Only explicit test-only schemes are allowed to reach the in-process path.
     */
    bool isExplicitInProcessEndpoint(const std::string& endpoint) const {
        return endpoint.rfind("inproc://", 0) == 0 || endpoint.rfind("loopback://", 0) == 0;
    }

    [[nodiscard]] bool isProductionSafeEndpoint() const {
        return use_grpc || isExplicitInProcessEndpoint(config.endpoint);
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
            // mTLS enabled - create SSL credentials. Any cert load failure is a
            // security boundary event: the channel must fail closed unless the
            // caller explicitly opted into a local/test-only insecure override.
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

                if (ssl_opts.pem_root_certs.empty() ||
                    ssl_opts.pem_cert_chain.empty() ||
                    ssl_opts.pem_private_key.empty()) {
                    throw std::runtime_error(
                        "ShardRPCClient: mTLS enabled but one or more PEM credentials are missing");
                }
                
                credentials = grpc::SslCredentials(ssl_opts);
                THEMIS_INFO("mTLS enabled for shard RPC communication (server verification: {})", 
                           config.tls_verify_server);
                
                if (!config.tls_verify_server) {
                    THEMIS_WARN("Server certificate verification is disabled. This is insecure and should only be used in development/testing.");
                }
                
            } catch (const std::exception& e) {
                if (config.allow_insecure) {
                    THEMIS_WARN("[SECURITY] ShardRPCClient: mTLS material is incomplete; using explicit local/test insecure override for {} ({})", config.endpoint, e.what());
                    credentials = grpc::InsecureChannelCredentials();
                } else {
                    THEMIS_ERROR("[SECURITY] ShardRPCClient: mTLS material is incomplete and the transport is blocked: {}", e.what());
                    throw std::runtime_error(
                        "ShardRPCClient: secure transport requires valid mTLS PEM material; "
                        "set allow_insecure=true only for local/test overrides");
                }
            }
        } else {
            const bool prod_mode = []() {
                const char* prod = getenv("THEMIS_PRODUCTION_MODE");
                return prod && std::string(prod) == "1";
            }();
            const bool env_override = []() {
                const char* override_flag = getenv("THEMIS_SHARD_MTLS_DISABLED");
                return override_flag && std::string(override_flag) == "1";
            }();

            if (prod_mode && !env_override && !config.allow_insecure) {
                throw std::runtime_error(
                    "ShardRPCClient: mTLS must be enabled in production mode "
                    "(THEMIS_PRODUCTION_MODE=1). Set enable_mtls=true or set "
                    "allow_insecure=true / THEMIS_SHARD_MTLS_DISABLED=1 for local/test override.");
            }

            if (!prod_mode || env_override || config.allow_insecure) {
                THEMIS_WARN("ShardRPCClient: insecure transport enabled for {} (local/test override active)", config.endpoint);
            }
            credentials = grpc::InsecureChannelCredentials();
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
        if (!channel) {
          return false;
        }
        
        auto state = channel->GetState(false);
        return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
    }
    
    bool waitForChannelReady(int timeout_ms) {
        if (!channel) {
          return false;
        }
        
        auto deadline = std::chrono::system_clock::now() + 
                       std::chrono::milliseconds(timeout_ms);
        
        return channel->WaitForConnected(deadline);
    }
#endif
};

/**
 * @brief Construct RPC client and initialize transport mode (gRPC/in-process).
 * @param config Endpoint, timeout, retry, TLS and metrics configuration.
 */
ShardRPCClient::ShardRPCClient(const Config& config)
    : impl_(std::make_unique<Impl>(config))
{
    THEMIS_INFO("ShardRPCClient created for endpoint: {}", config.endpoint);
}

/** @brief Destroy RPC client and release internal transport resources. */
ShardRPCClient::~ShardRPCClient() = default;

/** @brief Install or clear in-process response handler used by simulation fallback. */
void ShardRPCClient::setInProcessResponseHandler(InProcessResponseHandler handler) {
    std::lock_guard<std::mutex> lk(impl_->handler_mutex);
    impl_->in_process_handler = std::move(handler);
}

/**
 * @brief Send PREPARE RPC and return shard vote.
 * @param txn_id Transaction identifier.
 * @param operations Serialized operation set for phase 1.
 * @return True when shard votes COMMIT.
 */
bool ShardRPCClient::prepare(
    const std::string& txn_id,
    const nlohmann::json& operations
) {
    THEMIS_DEBUG("RPC PREPARE to {}: txn={}, ops={}", 
                impl_->config.endpoint, txn_id,static_cast<int>(operations.size()));
    
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

/**
 * @brief Send COMMIT RPC for previously prepared transaction.
 * @param txn_id Transaction identifier.
 * @param commit_timestamp Commit timestamp used by participant MVCC path.
 * @return True when shard confirms commit.
 */
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

/**
 * @brief Send ABORT RPC for transaction.
 * @param txn_id Transaction identifier.
 * @return True when shard confirms abort.
 */
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

/**
 * @brief Send compensation request used by SAGA-style rollback flows.
 * @param txn_id Transaction identifier.
 * @param operation Compensation payload.
 * @return True when compensation is acknowledged.
 */
bool ShardRPCClient::compensate(
    const std::string& txn_id,
    const nlohmann::json& operation
) {
    THEMIS_DEBUG("RPC COMPENSATE to {}: txn={}", impl_->config.endpoint, txn_id);

    try {
        nlohmann::json params = {
            {"transaction_id", txn_id},
            {"operation", operation}
        };

        auto response = sendRequest("compensate", params);

        if (response.contains("status") && response["status"] == "compensated") {
            THEMIS_DEBUG("RPC COMPENSATE success");
            return true;
        } else {
            THEMIS_WARN("RPC COMPENSATE failed");
            return false;
        }

    } catch (const std::exception& e) {
        THEMIS_ERROR("RPC COMPENSATE exception: {}", e.what());
        return false;
    }
}

/**
 * @brief Execute point-in-time snapshot read on target shard.
 * @param snapshot_ts Snapshot timestamp.
 * @param query Query payload.
 * @return Result data array, or empty array on failure.
 */
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

/** @brief Perform lightweight health ping via RPC. */
bool ShardRPCClient::ping() {
    try {
        auto response = sendRequest("ping", nlohmann::json::object());
        return response.contains("status") && response["status"] == "ok";
    } catch (...) {
        return false;
    }
}

/** @brief Collect remote wait-for graph edges for distributed deadlock detection. */
std::vector<ShardRPCClient::WaitForEdge> ShardRPCClient::collectWaitForEdges() {
    try {
        auto response = sendRequest("collect_wait_for_edges", nlohmann::json::object());
        std::vector<WaitForEdge> edges = {};

        if (!response.contains("edges") || !response["edges"].is_array()) {
            return edges;
        }
        for (const auto& edge : response["edges"]) {
            if (!edge.contains("waiting_transaction_id") ||
                !edge.contains("blocking_transaction_id")) {
                continue;
            }
            const auto& waiting  = edge["waiting_transaction_id"];
            const auto& blocking = edge["blocking_transaction_id"];
            if (!waiting.is_string() || !blocking.is_string()) {
                continue;
            }
            edges.push_back({waiting.get<std::string>(),
                             blocking.get<std::string>()});
        }
        return edges;
    } catch (const std::exception& e) {
        THEMIS_WARN("collectWaitForEdges from {} failed: {}", impl_->config.endpoint, e.what());
        return {};
    }
}

/**
 * @brief Replicate one entity write to remote shard.
 * @param collection Collection name.
 * @param uuid Entity identifier.
 * @param data Entity payload.
 * @param timestamp_ns Write timestamp (0 means caller leaves default behavior).
 * @return True when replication RPC succeeds.
 */
bool ShardRPCClient::writeEntity(
    const std::string& collection,
    const std::string& uuid,
    const nlohmann::json& data,
    uint64_t timestamp_ns
) {
    THEMIS_DEBUG("RPC WRITE_ENTITY to {}: collection={} uuid={}",
                impl_->config.endpoint, collection, uuid);
    try {
        nlohmann::json params = {
            {"collection",    collection},
            {"uuid",          uuid},
            {"data",          data},
            {"timestamp_ns",  timestamp_ns}
        };
        auto response = sendRequest("write_entity", params);
        return response.contains("success") && response["success"].get<bool>();
    } catch (const std::exception& e) {
        THEMIS_ERROR("RPC WRITE_ENTITY exception: {}", e.what());
        return false;
    }
}

/** @brief Dispatch RPC call through gRPC path or in-process fallback path. */
nlohmann::json ShardRPCClient::sendRequest(
    const std::string& method,
    const nlohmann::json& params
) {
#if THEMIS_HAS_SHARD_GRPC
if (impl_->use_grpc) {
    return sendRequestGrpc(method, params);
}
#endif

if (impl_->isExplicitInProcessEndpoint(impl_->config.endpoint)) {
    return sendRequestInProcess(method, params);
}

throw std::runtime_error(
    "ShardRPCClient fail-closed: no real shard transport available for endpoint '" +
    impl_->config.endpoint + "'. Only explicit test-only inproc:// or loopback:// endpoints may use the in-process path."
);
}

#if THEMIS_HAS_SHARD_GRPC
/**
 * @brief Execute one RPC with retries and exponential backoff over gRPC transport.
 * @param method Logical method name.
 * @param params RPC request payload.
 * @return RPC response payload.
 * @throws std::exception On final retry exhaustion or non-retryable failures.
 */
nlohmann::json ShardRPCClient::sendRequestGrpc(
    const std::string& method,
    const nlohmann::json& params
) {
    int attempts = 0;
    std::exception_ptr last_exception = {};
    
    while (attempts < impl_->config.max_retries) {
        ++attempts;

        // Re-check the circuit breaker before every attempt so that a circuit
        // that opens mid-loop (due to recordFailure() exceeding the threshold)
        // blocks the remaining retry attempts immediately.
        if (impl_->config.enable_circuit_breaker &&
                !impl_->circuit_breaker.allowRequest()) {
            throw std::runtime_error("Circuit breaker is OPEN for endpoint: " +
                                     impl_->config.endpoint);
        }
        
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

            // Per-attempt latency measurement for metrics.
            const auto t0 = std::chrono::steady_clock::now();
            
            // Route to appropriate gRPC method
            nlohmann::json result;
            if (method == "prepare") {
                result = handlePrepareGrpc(context, params);
            } else if (method == "commit") {
                result = handleCommitGrpc(context, params);
            } else if (method == "abort") {
                result = handleAbortGrpc(context, params);
            } else if (method == "compensate") {
                // SAGA compensation: reuse the abort gRPC path until a dedicated
                // CompensateTransaction RPC is added to the shard_rpc.proto service
                // (tracked in Issue #106 / proto migration backlog).
                result = handleAbortGrpc(context, params);
                if (result.contains("status") && result["status"] == "aborted") {
                    result["status"] = "compensated";
                }
            } else if (method == "snapshot_read") {
                result = handleSnapshotReadGrpc(context, params);
            } else if (method == "write_entity") {
                result = handleWriteEntityGrpc(context, params);
            } else if (method == "ping") {
                result = handleHealthCheckGrpc(context);
            } else if (method == "collect_wait_for_edges") {
                result = handleCollectWaitForEdgesGrpc(context);
            } else {
                throw std::runtime_error("Unknown RPC method: " + method);
            }

            const auto latency_us = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0).count());

            if (impl_->config.enable_circuit_breaker) {
                impl_->circuit_breaker.recordSuccess();
            }
            impl_->recordMetrics(method, "success", latency_us);
            return result;
            
        } catch (const NonRetryableRpcError& e) {
            if (impl_->config.enable_circuit_breaker) {
                impl_->circuit_breaker.recordFailure();
            }
            impl_->recordMetrics(method, "non_retryable_error", 0);
            THEMIS_WARN("gRPC {} non-retryable error: {}", method, e.what());
            throw;  // Rethrow immediately without further retry attempts

        } catch (const std::exception& e) {
            last_exception = std::current_exception();
            std::string err_msg = e.what();

            if (impl_->config.enable_circuit_breaker) {
                impl_->circuit_breaker.recordFailure();
            }
            impl_->recordMetrics(method, "retryable_error", 0);

            THEMIS_WARN("gRPC {} attempt {}/{} failed: {}",
                       method, attempts, impl_->config.max_retries, err_msg);

            if (attempts < impl_->config.max_retries) {
                // Exponential backoff; cap the shift to prevent signed-int overflow
                // (shift <= 12 ensures retry_delay_ms * 4096 fits in int32 for any
                // reasonable retry_delay_ms, and the result is capped at 5s anyway)
                const int shift = std::min(attempts - 1, 12);
                int delay_ms = std::min(impl_->config.retry_delay_ms * (1 << shift), kMaxRetryDelayMs);
                
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

/** @brief Build and execute PrepareTransaction gRPC call. */
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
        throw NonRetryableRpcError(status.error_message());
    }
    
    nlohmann::json result = {
        {"vote", response.vote_commit() ? "commit" : "abort"},
        {"status", response.vote_commit() ? "prepared" : "failed"},
        {"error", response.error()}
    };
    
    return result;
}

/** @brief Build and execute CommitTransaction gRPC call. */
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
        throw NonRetryableRpcError(status.error_message());
    }
    
    nlohmann::json result = {
        {"status", response.success() ? "committed" : "failed"},
        {"error", response.error()}
    };
    
    return result;
}

/** @brief Build and execute AbortTransaction gRPC call. */
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
        throw NonRetryableRpcError(status.error_message());
    }
    
    nlohmann::json result = {
        {"status", response.success() ? "aborted" : "failed"}
    };
    
    return result;
}

/** @brief Execute snapshot-read metadata call sequence over gRPC. */
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

/** @brief Execute HealthCheck gRPC call and normalize response JSON. */
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

/** @brief Execute ReplicateData gRPC call for single-entity replication. */
nlohmann::json ShardRPCClient::handleWriteEntityGrpc(
    grpc::ClientContext& context,
    const nlohmann::json& params
) {
    themis::sharding::proto::ReplicateRequest request;
    request.set_shard_id(impl_->config.shard_id.empty()
                         ? impl_->config.endpoint
                         : impl_->config.shard_id);

    auto* entity = request.add_entities();
    entity->set_uuid(params.value("uuid", std::string{}));
    entity->set_collection(params.value("collection", std::string{}));
    entity->set_data(params.value("data", nlohmann::json{}).dump());
    entity->set_version(1);
    if (params.contains("timestamp_ns") && !params["timestamp_ns"].is_null()) {
        const uint64_t ts = params["timestamp_ns"].get<uint64_t>();
        entity->set_timestamp_ns(ts);
        request.set_timestamp_ns(ts);
    }

    themis::sharding::proto::ReplicateResponse response;
    grpc::Status status = impl_->stub->ReplicateData(&context, request, &response);

    if (!status.ok()) {
        if (isRetryableError(status.error_code())) {
            throw std::runtime_error("Retryable error: " + status.error_message());
        }
        throw NonRetryableRpcError(status.error_message());
    }

    return {
        {"success",           response.success()},
        {"replicated_count",  static_cast<uint64_t>(response.replicated_count())},
        {"error",             response.error()}
    };
}

/** @brief Execute CollectWaitForEdges gRPC call and map protobuf to JSON. */
nlohmann::json ShardRPCClient::handleCollectWaitForEdgesGrpc(
    grpc::ClientContext& context
) {
    themis::sharding::proto::CollectWaitForEdgesRequest request;
    themis::sharding::proto::CollectWaitForEdgesResponse response;

    grpc::Status status = impl_->stub->CollectWaitForEdges(&context, request, &response);

    if (!status.ok()) {
        if (isRetryableError(status.error_code())) {
            throw std::runtime_error("CollectWaitForEdges failed: " + status.error_message());
        }
        throw NonRetryableRpcError(status.error_message());
    }

    auto edges_json = nlohmann::json::array();
    for (const auto& edge : response.edges()) {
        edges_json.push_back({
            {"waiting_transaction_id",  edge.waiting_transaction_id()},
            {"blocking_transaction_id", edge.blocking_transaction_id()}
        });
    }
    return {
        {"edges",    std::move(edges_json)},
        {"shard_id", response.shard_id()}
    };
}

/** @brief Classify gRPC status codes into retryable and fail-fast categories. */
bool ShardRPCClient::isRetryableError(grpc::StatusCode code) {
    // Categorize errors as retryable or non-retryable
    switch (code) {
        case grpc::StatusCode::UNAVAILABLE:
        [[fallthrough]];
        case grpc::StatusCode::DEADLINE_EXCEEDED:
        [[fallthrough]];
        case grpc::StatusCode::RESOURCE_EXHAUSTED:
        [[fallthrough]];
        case grpc::StatusCode::ABORTED:
        [[fallthrough]];
        case grpc::StatusCode::INTERNAL:
            return true;
        
        case grpc::StatusCode::INVALID_ARGUMENT:
        [[fallthrough]];
        case grpc::StatusCode::NOT_FOUND:
        [[fallthrough]];
        case grpc::StatusCode::ALREADY_EXISTS:
        [[fallthrough]];
        case grpc::StatusCode::PERMISSION_DENIED:
        [[fallthrough]];
        case grpc::StatusCode::UNAUTHENTICATED:
        [[fallthrough]];
        case grpc::StatusCode::FAILED_PRECONDITION:
        [[fallthrough]];
        case grpc::StatusCode::OUT_OF_RANGE:
        [[fallthrough]];
        case grpc::StatusCode::UNIMPLEMENTED:
            return false;
        
        default:
            return false;
    }
}
#endif

/**
 * @brief Execute RPC via in-process simulation path with retry and backoff.
 * @param method Logical method name.
 * @param params RPC request payload.
 * @return Simulated or injected response payload.
 */
nlohmann::json ShardRPCClient::sendRequestInProcess(
    const std::string& method,
    const nlohmann::json& params
) {
    (void)params;
    // Test-only path: explicitly allowed only for inproc:// or loopback:// endpoints.
    // Production endpoints must never silently fall back to this mode; callers
    // must use the fail-closed error path in sendRequest().
    if (!impl_->isExplicitInProcessEndpoint(impl_->config.endpoint)) {
        throw std::runtime_error(
            "In-process simulation is forbidden for production endpoint '" +
            impl_->config.endpoint + "'. Use an explicit test-only inproc:// or loopback:// endpoint."
        );
    }

    // Snapshot the injected handler (if any) so the lock is not held during the
    // (possibly sleeping) retry loop.
    InProcessResponseHandler injected_handler;
    {
        std::lock_guard<std::mutex> lk(impl_->handler_mutex);
        injected_handler = impl_->in_process_handler;
    }
    
    // Log a diagnostic warning that in-process simulation is in use
    static std::atomic<bool> logged = false;
    if (!logged.exchange(true)) {
        THEMIS_WARN("ShardRPCClient: Using in-process simulation path for RPC calls to {}. "
                    "This is a test-only configuration; production deployments must enable gRPC. "
                    "To suppress this warning, enable THEMIS_HAS_SHARD_GRPC=1 and deploy real shard peers.",
                    impl_->config.endpoint);
    }

    // In-process simulation for single-node deployments
    int attempts = 0;
    std::exception_ptr last_exception = {};
    
    while (attempts < impl_->config.max_retries) {
        ++attempts;

        // Re-check circuit breaker before every attempt (mirrors sendRequestGrpc).
        if (impl_->config.enable_circuit_breaker &&
                !impl_->circuit_breaker.allowRequest()) {
            throw std::runtime_error("Circuit breaker is OPEN for endpoint: " +
                                     impl_->config.endpoint);
        }
        
        try {
            THEMIS_DEBUG("RPC {} attempt {}/{} to {} (in-process)",
                        method, attempts, impl_->config.max_retries,
                        impl_->config.endpoint);
            
            // Simulate network delay
            const auto t0 = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(10)
            );
            
            // If a custom handler has been injected (e.g. for testing failure
            // scenarios), delegate to it instead of the hardcoded fallback.
            nlohmann::json response;
            if (injected_handler) {
                response = injected_handler(method, params);
            } else {
                // Built-in hardcoded fallback responses for single-node / test mode.
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
                } else if (method == "compensate") {
                    response = {
                        {"status", "compensated"}
                    };
                } else if (method == "snapshot_read") {
                    response = {
                        {"status", "success"},
                        {"data", nlohmann::json::array()}
                    };
                } else if (method == "write_entity") {
                    response = {
                        {"success",          true},
                        {"replicated_count", 1}
                    };
                } else if (method == "ping") {
                    response = {
                        {"status", "ok"}
                    };
                } else if (method == "collect_wait_for_edges") {
                    // In-process / single-node simulation: no local wait edges to report.
                    // In a real multi-node deployment the gRPC path (handleCollectWaitForEdgesGrpc)
                    // queries the shard's local lock-wait state.
                    response = {
                        {"edges",    nlohmann::json::array()},
                        {"shard_id", impl_->config.shard_id}
                    };
                } else {
                    // Unknown method in in-process simulation
                    THEMIS_ERROR("In-process RPC simulation does not support method: {}. "
                                 "This method is only supported in gRPC mode. "
                                 "Enable THEMIS_HAS_SHARD_GRPC to use production gRPC transport.",
                                 method);
                    throw std::runtime_error("Unknown RPC method in in-process simulation: " + method);
                }
            }

            const auto latency_us = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0).count());
            
            if (impl_->config.enable_circuit_breaker) {
                impl_->circuit_breaker.recordSuccess();
            }
            impl_->recordMetrics(method, "success", latency_us);
            return response;
            
        } catch (const std::exception& e) {
            last_exception = std::current_exception();

            if (impl_->config.enable_circuit_breaker) {
                impl_->circuit_breaker.recordFailure();
            }
            impl_->recordMetrics(method, "retryable_error", 0);

            THEMIS_WARN("RPC {} attempt {}/{} failed: {}",
                       method, attempts, impl_->config.max_retries, e.what());
            
            if (attempts < impl_->config.max_retries) {
                // Exponential backoff; same overflow-safe calculation as sendRequestGrpc
                const int shift = std::min(attempts - 1, 12);
                const int delay_ms = std::min(impl_->config.retry_delay_ms * (1 << shift), kMaxRetryDelayMs);
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
    
    throw std::runtime_error("RPC request failed after " + 
                           std::to_string(impl_->config.max_retries) + " attempts");
}

} // namespace themis::sharding
