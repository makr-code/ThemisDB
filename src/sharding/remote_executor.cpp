/**
 * @file remote_executor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=5, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/remote_executor.h"
#include "sharding/circuit_breaker.h"
#include "utils/tracing.h"
#include <chrono>

namespace themis::sharding {

RemoteExecutor::RemoteExecutor(const Config& config)
    : config_(config),
      circuit_breaker_manager_(std::make_shared<CircuitBreakerManager>()) {
    
    // Initialize mTLS client
    MTLSClient::Config mtls_config{
        .cert_path = config.cert_path,
        .key_path = config.key_path,
        .key_passphrase = config.key_passphrase,
        .ca_cert_path = config.ca_cert_path,
        .crl_path = config.crl_path,
        .tls_version = "TLSv1.3",
        .verify_peer = true,
        .verify_hostname = true,
        .connect_timeout_ms = config.connect_timeout_ms,
        .request_timeout_ms = config.request_timeout_ms,
        .max_retries = config.max_retries
    };
    
    mtls_client_ = std::make_unique<MTLSClient>(mtls_config);
    
    // Initialize request signer if signing is enabled
    if (config.enable_signing) {
        SignedRequestSigner::Config signer_config{
            .shard_id = config.local_shard_id,
            .cert_path = config.cert_path,
            .key_path = config.key_path,
            .key_passphrase = config.key_passphrase
        };
        
        request_signer_ = std::make_unique<SignedRequestSigner>(signer_config);
    }
}

RemoteExecutor::Result RemoteExecutor::get(const ShardInfo& shard_info,
                                          const std::string& path) {
    return executeRequest("GET", shard_info, path);
}

RemoteExecutor::Result RemoteExecutor::post(const ShardInfo& shard_info,
                                           const std::string& path,
                                           const nlohmann::json& body) {
    return executeRequest("POST", shard_info, path, std::optional<nlohmann::json>(body));
}

RemoteExecutor::Result RemoteExecutor::put(const ShardInfo& shard_info,
                                          const std::string& path,
                                          const nlohmann::json& body) {
    return executeRequest("PUT", shard_info, path, std::optional<nlohmann::json>(body));
}

RemoteExecutor::Result RemoteExecutor::del(const ShardInfo& shard_info,
                                          const std::string& path) {
    return executeRequest("DELETE", shard_info, path);
}

RemoteExecutor::Result RemoteExecutor::executeQuery(const ShardInfo& shard_info,
                                                    const std::string& query) {
    // Execute query via POST to /api/v1/query endpoint
    nlohmann::json body = {
        {"query", query}
    };
    
    return post(shard_info, "/api/v1/query", body);
}

RemoteExecutor::Result RemoteExecutor::postBinary(const ShardInfo& shard_info,
                                                   const std::string& path,
                                                   const uint8_t* data,
                                                   std::size_t size) {
    // Base64-encode the binary payload so it can be embedded in a JSON body
    // and sent via the existing MTLSClient::post() interface.
    //
    // Trade-off: encoding adds ~33 % overhead; a future optimisation could
    // add a raw-octet-stream path in MTLSClient, but that requires protocol
    // changes outside the current scope.
    static constexpr char kBase64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded = {};
    encoded.reserve(((size + 2u) / 3u) * 4u);

    for (std::size_t i = 0; i < size; i += 3) {
        const auto b0 = data[i];
        const auto b1 = (i + 1 < size) ? data[i + 1] : std::uint8_t{0};
        const auto b2 = (i + 2 < size) ? data[i + 2] : std::uint8_t{0};

        encoded += kBase64Chars[(b0 >> 2) & 0x3F];
        encoded += kBase64Chars[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)];
        encoded += (i + 1 < size) ? kBase64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
        encoded += (i + 2 < size) ? kBase64Chars[b2 & 0x3F] : '=';
    }

    nlohmann::json body = {
        {"kv_state_b64", std::move(encoded)},
        {"size",         static_cast<std::uint64_t>(size)}
    };

    return post(shard_info, path, body);
}

bool RemoteExecutor::isReady() const {
    return mtls_client_ && mtls_client_->isReady();
}

std::string RemoteExecutor::getEndpointURL(const ShardInfo& shard_info) const {
    // Construct endpoint URL
    // If primary_endpoint already contains protocol, use it as-is
    if (shard_info.primary_endpoint.find("://") != std::string::npos) {
        return shard_info.primary_endpoint;
    }
    
    // Otherwise, prepend https://
    return "https://" + shard_info.primary_endpoint;
}

RemoteExecutor::Result RemoteExecutor::executeRequest(
    const std::string& method,
    const ShardInfo& shard_info,
    const std::string& path,
    const std::optional<nlohmann::json>& body) {
    
    auto span = Tracer::startSpan("RemoteExecutor.executeRequest");
    span.setAttribute("method", method);
    span.setAttribute("shard_id", shard_info.shard_id);
    span.setAttribute("path", path);
    
    auto start = std::chrono::steady_clock::now();
    
    // Check circuit breaker if enabled
    if (config_.enable_circuit_breaker) {
        auto& circuit_breaker = circuit_breaker_manager_->getCircuitBreaker(
            shard_info.shard_id,
            config_.circuit_breaker_config
        );
        
        if (!circuit_breaker.allowRequest()) {
            // Circuit is OPEN, reject request immediately
            Result result;
            result.shard_id = shard_info.shard_id;
            result.success = false;
            result.error = "Circuit breaker is OPEN for shard: " + shard_info.shard_id;
            result.http_status = 503; // Service Unavailable
            
            auto end = std::chrono::steady_clock::now();
            result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start
            ).count();
            
            return result;
        }
    }
    
    // Get endpoint URL
    std::string endpoint = getEndpointURL(shard_info);
    span.setAttribute("endpoint", endpoint);
    
    // Prepare request body (with signing if enabled)
    nlohmann::json request_body;
    if (body) {
        request_body = *body;
    }
    
    // Add signed request envelope if signing is enabled
    if (config_.enable_signing && request_signer_) {
        SignedRequest signed_req = request_signer_->createSignedRequest(
            method, path, request_body
        );
        
        // Wrap in signed envelope
        request_body = signed_req.toJSON();
    }
    
    // Execute request via mTLS client
    MTLSClient::Response response;
    
    if (method == "GET") {
        response = mtls_client_->get(endpoint, path);
    } else if (method == "POST") {
        response = mtls_client_->post(endpoint, path, request_body);
    } else if (method == "PUT") {
        response = mtls_client_->put(endpoint, path, request_body);
    } else if (method == "DELETE") {
        response = mtls_client_->del(endpoint, path);
    }
    
    auto end = std::chrono::steady_clock::now();
    uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // Record result with circuit breaker if enabled
    if (config_.enable_circuit_breaker) {
        auto& circuit_breaker = circuit_breaker_manager_->getCircuitBreaker(
            shard_info.shard_id,
            config_.circuit_breaker_config
        );
        
        if (response.success && response.status_code >= 200 && response.status_code < 300) {
            circuit_breaker.recordSuccess();
        } else {
            circuit_breaker.recordFailure();
        }
    }
    
    return convertResponse(response, shard_info.shard_id, elapsed_ms);
}

RemoteExecutor::Result RemoteExecutor::convertResponse(
    const MTLSClient::Response& response,
    const std::string& shard_id,
    uint64_t elapsed_ms) {
    
    Result result;
    result.shard_id = shard_id;
    result.success = response.success;
    result.error = response.error;
    result.execution_time_ms = elapsed_ms;
    result.http_status = response.status_code;
    
    if (response.success) {
        result.data = response.body;
    } else {
        // Include error details in data
        result.data = nlohmann::json{
            {"error", response.error},
            {"status_code", response.status_code},
            {"status_message", response.status_message}
        };
    }
    
    return result;
}

} // namespace themis::sharding
