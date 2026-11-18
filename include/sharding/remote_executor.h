/**
 * @file remote_executor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/mtls_client.h"
#include "sharding/signed_request.h"
#include "sharding/shard_topology.h"
#include "sharding/circuit_breaker.h"
#include <string>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * Remote Executor - Execute requests on remote shards
 * 
 * Handles shard-to-shard communication using mTLS and signed requests.
 * Provides high-level interface for executing operations on remote shards
 * with automatic retry, error handling, and connection management.
 */
class RemoteExecutor {
public:
    /**
     * Configuration for Remote Executor
     */
    struct Config {
        // mTLS configuration
        std::string cert_path;          // This shard's certificate
        std::string key_path;           // This shard's private key
        std::string key_passphrase;     // Optional: key passphrase
        std::string ca_cert_path;       // Root CA certificate
        std::string crl_path;           // Optional: CRL file
        
        // This shard's identity
        std::string local_shard_id;     // This shard's ID
        
        // Request signing
        bool enable_signing = true;     // Enable signed requests
        
        // Timeouts and retries (inherited by mTLS client)
        uint32_t connect_timeout_ms = 5000;
        uint32_t request_timeout_ms = 30000;
        uint32_t max_retries = 3;
        
        // Circuit breaker configuration
        bool enable_circuit_breaker = true;  // Enable circuit breaker pattern
        CircuitBreaker::Config circuit_breaker_config;
    };
    
    /**
     * Result from remote execution
     */
    struct Result {
        std::string shard_id;           // Target shard ID
        nlohmann::json data;            // Response data
        bool success;                   // true if request succeeded
        /// @brief Error details when remote execution failed; empty on success.
        std::string error;
        uint64_t execution_time_ms;     // Execution time
        int http_status;                // HTTP status code
        /// True when the request was aborted by client cancellation (RequestContext overloads only).
        bool cancelled = false;
        /// True when the supplied RequestContext deadline expired (RequestContext overloads only).
        bool timed_out = false;
        /// True when a transport connect/request stage timeout fired while the context
        /// was still live (RequestContext overloads only). Exclusive with `timed_out`.
        bool stage_timed_out = false;
    };
    
    /**
     * Construct Remote Executor with configuration
     */
    explicit RemoteExecutor(const Config& config);
    
    /**
     * @brief Execute a GET request on a remote shard.
     * @param shard_info Target shard information
     * @param path Request path (e.g., "/api/v1/data/urn:themis:...")
     * @return Result with response data or error
     */
    Result get(const ShardInfo& shard_info, const std::string& path);

    /**
     * @brief GET with request deadline and cancellation, effective during the in-flight transport call.
     * @param[in] shard_info Target shard.
     * @param[in] path Request path.
     * @param[in] ctx Deadline + cancellation token; checked before the circuit breaker
     *            and propagated into MTLSClient's asynchronous transport.
     * @return Result with `cancelled` / `timed_out` set when aborted. A client
     *         cancellation is not recorded as a circuit-breaker failure; a timeout is.
     */
    Result get(const ShardInfo& shard_info,
               const std::string& path,
               const query::RequestContext& ctx);
    
    /**
     * @brief Executes a POST request on a remote shard without request cancellation.
     * @param shard_info Target shard information.
     * @param path Request path.
     * @param body JSON request body.
     * @return Result with response data, or an error when the request fails.
     */
    Result post(const ShardInfo& shard_info,
                const std::string& path,
                const nlohmann::json& body);

    /**
     * @brief POST with request deadline and cancellation, effective during the in-flight transport call.
     * @param[in] shard_info Target shard.
     * @param[in] path Request path.
     * @param[in] body JSON body (signed when signing is enabled).
     * @param[in] ctx Deadline + cancellation token.
     * @return Result with `cancelled` / `timed_out` set when aborted; see get(const ShardInfo&, const std::string&, const query::RequestContext&).
     */
    Result post(const ShardInfo& shard_info,
                const std::string& path,
                const nlohmann::json& body,
                const query::RequestContext& ctx);
    
    /**
     * @brief Executes a PUT request on a remote shard without request cancellation.
     * @param shard_info Target shard information.
     * @param path Request path.
     * @param body JSON request body.
     * @return Result with response data, or an error when the request fails.
     */
    Result put(const ShardInfo& shard_info,
               const std::string& path,
               const nlohmann::json& body);

    /**
     * @brief PUT with request deadline and cancellation during transport.
     * @param[in] shard_info Target shard.
     * @param[in] path Request path.
     * @param[in] body JSON body (signed when signing is enabled).
     * @param[in] ctx Deadline + cancellation token propagated to mTLS transport.
     * @return Result with `cancelled` / `timed_out` set when aborted.
     */
    Result put(const ShardInfo& shard_info,
               const std::string& path,
               const nlohmann::json& body,
               const query::RequestContext& ctx);
    
    /**
     * @brief Executes a DELETE request on a remote shard without request cancellation.
     * @param shard_info Target shard information.
     * @param path Request path.
     * @return Result with response data, or an error when the request fails.
     */
    Result del(const ShardInfo& shard_info, const std::string& path);

    /**
     * @brief DELETE with request deadline and cancellation during transport.
     * @param[in] shard_info Target shard.
     * @param[in] path Request path.
     * @param[in] ctx Deadline + cancellation token propagated to mTLS transport.
     * @return Result with `cancelled` / `timed_out` set when aborted.
     */
    Result del(const ShardInfo& shard_info,
               const std::string& path,
               const query::RequestContext& ctx);
    
    /**
     * @brief Executes an AQL query on a remote shard without request cancellation.
     * @param shard_info Target shard information.
     * @param query Query string to submit to the remote query endpoint.
     * @return Result with query data, or an error when the request fails.
     */
    Result executeQuery(const ShardInfo& shard_info, const std::string& query);

    /**
     * @brief Execute a remote query with request cancellation and deadline.
     * @param[in] shard_info Target shard.
     * @param[in] query AQL query string.
     * @param[in] ctx Deadline + cancellation token propagated to transport.
     * @return Query result or an outcome with timeout/cancellation flags.
     */
    Result executeQuery(const ShardInfo& shard_info,
                        const std::string& query,
                        const query::RequestContext& ctx);

    /**
     * @brief Transfers a raw binary payload to a remote shard via HTTP POST.
     *
     * Intended for KV-state transfer (Phase 5 KV-Prefix Cross-Shard sharing):
     * the serialised KV cache for a shared system-prompt prefix is POSTed to
     * the target shard so that it can warm its local KV cache and reduce
     * time-to-first-token (TTFT) for requests that share the same prefix.
     *
     * Content-Type is set to "application/octet-stream".  The response body
     * is returned in @c Result::response (base64-encoded by the mTLS layer if
     * non-printable characters are present).
     *
     * @param shard_info   Target shard.
     * @param path         Request path (e.g., "/api/v1/kv-prefix/ingest").
     * @param data         Pointer to binary payload bytes.
     * @param size         Payload size in bytes.
     * @return Result with success flag, shard_id, and optional error message.
     */
    Result postBinary(const ShardInfo& shard_info,
                      const std::string& path,
                      const uint8_t* data,
                      std::size_t size);

    /**
     * @brief Transfer binary data with request cancellation and deadline.
     * @param[in] shard_info Target shard.
     * @param[in] path Request path.
     * @param[in] data Binary payload; may be null only when @p size is zero.
     * @param[in] size Number of payload bytes.
     * @param[in] ctx Deadline + cancellation token propagated to mTLS transport.
     * @return Transfer result or an outcome with timeout/cancellation flags.
     */
    Result postBinary(const ShardInfo& shard_info,
                      const std::string& path,
                      const uint8_t* data,
                      std::size_t size,
                      const query::RequestContext& ctx);
    
    /**
     * Check if remote executor is ready
     * @return true if configured and ready to execute requests
     */
    bool isReady() const;
    
    /**
     * Get local shard ID
     * @return This shard's ID
     */
    const std::string& getLocalShardId() const {
        return config_.local_shard_id;
    }

private:
    Config config_;
    std::unique_ptr<MTLSClient> mtls_client_;
    std::unique_ptr<SignedRequestSigner> request_signer_;
    std::shared_ptr<CircuitBreakerManager> circuit_breaker_manager_;
    
    /**
     * Convert ShardInfo to endpoint URL
     * @param shard_info Shard information
     * @return Endpoint URL (e.g., "https://shard-001.dc1:8080")
     */
    std::string getEndpointURL(const ShardInfo& shard_info) const;
    
    /**
     * Execute request with optional signing
     * @param method HTTP method
     * @param shard_info Target shard
     * @param path Request path
     * @param body Optional request body
     * @return Result
     */
    Result executeRequest(const std::string& method,
                         const ShardInfo& shard_info,
                         const std::string& path,
                         const std::optional<nlohmann::json>& body = std::nullopt,
                         const query::RequestContext* ctx = nullptr);
    
    /**
     * Convert MTLSClient::Response to RemoteExecutor::Result
     */
    Result convertResponse(const MTLSClient::Response& response,
                          const std::string& shard_id,
                          uint64_t start_time_ms);
};

} // namespace themis::sharding
