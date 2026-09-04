/**
 * @file mtls_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/mtls_client.h"
#include "sharding/mtls_connection_pool.h"
#include "sharding/pki_shard_certificate.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace themis::sharding {

// PIMPL implementation to hide Boost details
struct MTLSClient::Impl {
    net::io_context ioc;
    ssl::context ssl_ctx{ssl::context::tlsv13_client};
    
    Impl() = default;
};

MTLSClient::MTLSClient(const Config& config)
    : config_(config), impl_(std::make_unique<Impl>()) {
    initSSLContext();
    
    // Initialize connection pool manager if enabled
    if (config_.use_connection_pool) {
        MTLSConnectionPoolManager::Config pool_config;
        pool_config.endpoint_config.min_connections = config_.pool_min_connections;
        pool_config.endpoint_config.max_connections = config_.pool_max_connections;
        pool_config.endpoint_config.connection_ttl = std::chrono::seconds(config_.pool_connection_ttl_s);
        pool_config.endpoint_config.idle_timeout = std::chrono::seconds(config_.pool_idle_timeout_s);
        
        pool_manager_ = std::make_shared<MTLSConnectionPoolManager>(pool_config);
    }
}

MTLSClient::~MTLSClient() = default;

bool MTLSClient::initSSLContext() {
    try {
        // Set TLS version
        if (config_.tls_version == "TLSv1.2") {
            impl_->ssl_ctx.set_options(
                ssl::context::default_workarounds |
                ssl::context::no_sslv2 |
                ssl::context::no_sslv3 |
                ssl::context::no_tlsv1 |
                ssl::context::no_tlsv1_1
            );
        } else {
            // TLS 1.3 (default)
            impl_->ssl_ctx.set_options(
                ssl::context::default_workarounds |
                ssl::context::no_sslv2 |
                ssl::context::no_sslv3 |
                ssl::context::no_tlsv1 |
                ssl::context::no_tlsv1_1 |
                ssl::context::no_tlsv1_2
            );
        }
        
        // Load client certificate
        impl_->ssl_ctx.use_certificate_chain_file(config_.cert_path);
        
        // Load private key
        if (!config_.key_passphrase.empty()) {
            impl_->ssl_ctx.set_password_callback(
                [this](std::size_t, ssl::context::password_purpose) {
                    return config_.key_passphrase;
                }
            );
        }
        impl_->ssl_ctx.use_private_key_file(config_.key_path, ssl::context::pem);
        
        // Load Root CA certificate for verification
        impl_->ssl_ctx.load_verify_file(config_.ca_cert_path);
        
        // Enable peer verification
        if (config_.verify_peer) {
            impl_->ssl_ctx.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MTLSClient: Failed to initialize SSL context: " << e.what() << std::endl;
        return false;
    }
}

MTLSClient::Response MTLSClient::get(const std::string& endpoint, const std::string& path) {
    return request("GET", endpoint, path);
}

MTLSClient::Response MTLSClient::post(const std::string& endpoint,
                                      const std::string& path,
                                      const nlohmann::json& body) {
    return request("POST", endpoint, path, std::optional<nlohmann::json>(body));
}

MTLSClient::Response MTLSClient::post(const std::string& endpoint,
                                      const std::string& path,
                                      const nlohmann::json& body,
                                      const std::string& authorization_header) {
    return request("POST", endpoint, path, std::optional<nlohmann::json>(body),
                   authorization_header);
}

MTLSClient::Response MTLSClient::put(const std::string& endpoint,
                                     const std::string& path,
                                     const nlohmann::json& body) {
    return request("PUT", endpoint, path, std::optional<nlohmann::json>(body));
}

MTLSClient::Response MTLSClient::del(const std::string& endpoint, const std::string& path) {
    return request("DELETE", endpoint, path);
}

MTLSClient::Response MTLSClient::request(const std::string& method,
                                        const std::string& endpoint,
                                        const std::string& path,
                                        const std::optional<nlohmann::json>& body,
                                        const std::string& authorization_header) {
    Response response;
    response.success = false;
    
    uint32_t retry_count = 0;
    uint32_t retry_delay = config_.retry_delay_ms;
    
    while (retry_count <= config_.max_retries) {
        try {
            // Parse endpoint (e.g., "https://shard-001.dc1:8080" -> "shard-001.dc1", "8080")
            auto [host, port] = parseEndpoint(endpoint);
            
            // Resolve hostname
            tcp::resolver resolver(impl_->ioc);
            auto const results = resolver.resolve(host, port);
            
            // Create SSL stream
            beast::ssl_stream<beast::tcp_stream> stream(impl_->ioc, impl_->ssl_ctx);
            
            // Set SNI (Server Name Indication)
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
                throw beast::system_error(
                    beast::error_code(static_cast<int>(::ERR_get_error()),
                                     net::error::get_ssl_category()),
                    "Failed to set SNI"
                );
            }
            
            // Connect with timeout
            beast::get_lowest_layer(stream).expires_after(
                std::chrono::milliseconds(config_.connect_timeout_ms)
            );
            beast::get_lowest_layer(stream).connect(results);
            
            // Perform SSL handshake
            stream.handshake(ssl::stream_base::client);
            
            // Verify peer certificate
            // Note: In production, we would extract and validate shard certificate here
            // using PKIShardCertificate::parseCertificatePEM and verify against expected shard_id
            
            // Build HTTP request
            http::request<http::string_body> req;
            if (method == "GET") {
                req.method(http::verb::get);
            } else if (method == "POST") {
                req.method(http::verb::post);
            } else if (method == "PUT") {
                req.method(http::verb::put);
            } else if (method == "DELETE") {
                req.method(http::verb::delete_);
            }
            
            req.target(path);
            req.version(11); // HTTP/1.1
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "ThemisDB-MTLSClient/1.0");
            req.set(http::field::accept, "application/json");
            
            // Add Authorization header when provided (e.g. for service-to-service JWT)
            if (!authorization_header.empty()) {
                req.set(http::field::authorization, authorization_header);
            }
            
            // Add body for POST/PUT
            if ((body && (method == "POST" || method == "PUT")) {
                std::string body_str = body->dump();
                req.body() = body_str;
                req.set(http::field::content_type, "application/json");
                req.set(http::field::content_length, std::to_string(body_str.size()));
            }
            req.prepare_payload();
            
            // Send request with timeout
            beast::get_lowest_layer(stream).expires_after(
                std::chrono::milliseconds(config_.request_timeout_ms)
            );
            http::write(stream, req);
            
            // Receive response
            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            
            // Populate response
            response.status_code = res.result_int();
            response.status_message = std::string(res.reason());
            response.raw_body = res.body();
            response.success = (response.status_code >= 200 && response.status_code < 300);
            
            // Parse JSON body if content-type is application/json
            if (res[http::field::content_type] == "application/json" && !response.raw_body.empty()) {
                try {
                    response.body = nlohmann::json::parse(response.raw_body);
                } catch (const nlohmann::json::exception& e) {
                    response.error = std::string("JSON parse error: ") + e.what();
                }
            }
            
            // Graceful shutdown
            beast::error_code ec;
            stream.shutdown(ec);
            // Ignore "stream truncated" error on shutdown
            if (ec && ec != beast::errc::not_connected) {
                // Log but don't fail the request
            }
            
            return response;
            
        } catch (const beast::system_error& e) {
            response.error = std::string("Network error: ") + e.what();
            
            // Retry logic
            if (retry_count < config_.max_retries) {
                retry_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay));
                retry_delay *= 2; // Exponential backoff
                continue;
            }
            
            response.status_code = 0;
            return response;
            
        } catch (const std::exception& e) {
            response.error = std::string("Exception: ") + e.what();
            response.status_code = 0;
            return response;
        }
    }
    
    response.error = "Max retries exceeded";
    response.status_code = 0;
    return response;
}

bool MTLSClient::isReady() const {
    // Check if SSL context was initialized successfully
    // In a real implementation, we'd track initialization state
    return !config_.cert_path.empty() && 
           !config_.key_path.empty() && 
           !config_.ca_cert_path.empty();
}

void MTLSClient::reset() {
    // Recreate IO context and SSL context
    impl_ = std::make_unique<Impl>();
    initSSLContext();
    
    // Reset connection pool if enabled
    if (pool_manager_) {
        pool_manager_->shutdown();
        
        // Reinitialize pool manager
        MTLSConnectionPoolManager::Config pool_config;
        pool_config.endpoint_config.min_connections = config_.pool_min_connections;
        pool_config.endpoint_config.max_connections = config_.pool_max_connections;
        pool_config.endpoint_config.connection_ttl = std::chrono::seconds(config_.pool_connection_ttl_s);
        pool_config.endpoint_config.idle_timeout = std::chrono::seconds(config_.pool_idle_timeout_s);
        
        pool_manager_ = std::make_shared<MTLSConnectionPoolManager>(pool_config);
    }
}

bool MTLSClient::verifyPeerCertificate(bool preverified, [[maybe_unused]] void* ctx) {
    // Future: extract certificate for detailed validation
    // In production, this would:
    // 1. Extract peer certificate from context
    // 2. Parse shard certificate info using PKIShardCertificate
    // 3. Verify shard_id matches expected value
    // 4. Check capabilities
    // 5. Verify against CRL if configured
    
    // For Phase 2, we rely on OpenSSL's built-in verification
    return preverified;
}

std::pair<std::string, std::string> MTLSClient::parseEndpoint(const std::string& endpoint) {
    // Parse endpoint: "https://host:port" or "host:port"
    // Supports both IPv4 and IPv6 addresses
    // IPv6 format: [2001:db8::1]:8080 or 2001:db8::1 (without port)
    // IPv4 format: 192.168.1.1:8080 or example.com:8080
    
    std::string host = {};
    std::string port = "8080"; // Default port
    
    std::string url = endpoint;
    
    // Remove protocol if present
    size_t proto_pos = url.find("://");
    if (proto_pos != std::string::npos) {
        url = url.substr(proto_pos + 3);
    }
    
    // Check for IPv6 address (enclosed in brackets)
    if (!url.empty() && url[0] == '[') {
        // IPv6 format: [host]:port or [host]
        size_t bracket_close = url.find(']');
        if (bracket_close != std::string::npos) {
            // Extract host (without brackets)
            host = url.substr(1, bracket_close - 1);
            
            // Check if port is specified after the bracket
            if (bracket_close + 1 <static_cast<int>(url.size()) && url[bracket_close + 1] == ':') {
                port = url.substr(bracket_close + 2);
            }
        } else {
            // Malformed IPv6 address, treat entire string as host
            host = url;
        }
    } else {
        // IPv4 or hostname: find the last colon for port separation
        size_t colon_pos = url.find_last_of(':');
        if (colon_pos != std::string::npos) {
            // Check if this might be an IPv6 address without brackets
            // (contains multiple colons but no brackets)
            size_t first_colon = url.find(':');
            if (first_colon != colon_pos) {
                // Multiple colons found - likely IPv6 without port
                host = url;
            } else {
                // Single colon - IPv4/hostname with port
                host = url.substr(0, colon_pos);
                port = url.substr(colon_pos + 1);
            }
        } else {
            // No colon - just a host
            host = url;
        }
    }
    
    return {host, port};
}

nlohmann::json MTLSClient::getPoolStatistics() const {
    nlohmann::json result = nlohmann::json::object();
    
    if (!pool_manager_) {
        result["enabled"] = false;
        result["message"] = "Connection pool not enabled";
        return result;
    }
    
    auto stats = pool_manager_->getStatistics();
    
    result["enabled"] = true;
    result["total_active"] = stats.total_active_connections;
    result["total_idle"] = stats.total_idle_connections;
    result["endpoints_cached"] = stats.cached_endpoint_pools;
    
    // Add per-endpoint statistics
    nlohmann::json endpoints = nlohmann::json::object();
    for (const auto& [endpoint, endpoint_stats] : stats.per_endpoint_stats) {
        nlohmann::json ep_stats;
        ep_stats["active"] = endpoint_stats.active_connections;
        ep_stats["idle"] = endpoint_stats.idle_connections;
        ep_stats["total_created"] = endpoint_stats.total_created;
        ep_stats["failed"] = endpoint_stats.connections_failed;
        ep_stats["utilization_percent"] = endpoint_stats.utilization_percent;
        
        endpoints[endpoint] = ep_stats;
    }
    result["per_endpoint"] = endpoints;
    
    return result;
}

} // namespace themis::sharding

