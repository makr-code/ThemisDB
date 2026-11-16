/**
 * @file mtls_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <optional>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

#include "query/request_context.h"

// Forward declarations to avoid pulling in Boost headers
namespace boost {
namespace asio {
    class io_context;
    namespace ssl {
        class context;
    }
}
namespace beast {
    namespace http {
        enum class verb;
    }
}
}

namespace themis::sharding {

// Forward declarations for connection pool
class MTLSConnectionPoolManager;
class EndpointConnectionPool;

/**
 * mTLS Client for Secure Shard-to-Shard Communication
 * 
 * Implements mutual TLS (mTLS) for authenticated and encrypted
 * communication between shards. Both client and server present
 * certificates during the TLS handshake.
 * 
 * Features:
 * - Mutual TLS authentication
 * - Certificate verification against Root CA
 * - CRL (Certificate Revocation List) checking
 * - Connection pooling and reuse
 * - Automatic retry with exponential backoff
 */
class MTLSClient {
public:
    /**
     * Configuration for mTLS Client
     */
    struct Config {
        std::string cert_path;          // Path to client certificate (PEM)
        std::string key_path;           // Path to private key (PEM)
        std::string key_passphrase;     // Optional: key passphrase
        std::string ca_cert_path;       // Path to Root CA certificate (PEM)
        std::string crl_path;           // Optional: Path to CRL file (PEM)
        
        // TLS configuration
        std::string tls_version = "TLSv1.3"; // TLS version (TLSv1.2, TLSv1.3)
        bool verify_peer = true;        // Verify peer certificate
        bool verify_hostname = true;    // Verify hostname against certificate
        
        // Connection settings
        uint32_t connect_timeout_ms = 5000;   // Connection timeout
        uint32_t request_timeout_ms = 30000;  // Request timeout
        uint32_t max_retries = 3;             // Maximum retry attempts
        uint32_t retry_delay_ms = 1000;       // Initial retry delay (exponential backoff)
        
        // Connection pooling (legacy - kept for backward compatibility)
        bool enable_pooling = true;     // Enable connection pooling
        uint32_t max_connections = 10;  // Max connections per endpoint (legacy)
        uint32_t idle_timeout_ms = 60000; // Idle connection timeout (legacy)
        
        // Dynamic connection pool configuration (new)
        bool use_connection_pool = true;        // Use new dynamic connection pool
        size_t pool_min_connections = 2;        // Minimum connections per endpoint
        size_t pool_max_connections = 50;       // Maximum connections per endpoint
        uint32_t pool_connection_ttl_s = 300;   // Connection TTL in seconds
        uint32_t pool_idle_timeout_s = 60;      // Idle timeout in seconds
    };
    
    /**
     * HTTP request result
     */
    struct Response {
        int status_code = 0;                // HTTP status code (200, 404, 500, etc.)
        std::string status_message;     // HTTP status message
        nlohmann::json body;            // Response body (JSON)
        std::string raw_body;           // Raw response body
        bool success;                   // true if status 2xx
        std::string error;              // Error message if failed
        /// True when the request was aborted because the caller's RequestContext
        /// was cancelled (only set by the RequestContext overloads).
        bool cancelled = false;
        /// True when the request was aborted because the deadline of the supplied
        /// RequestContext expired (only set by the RequestContext overloads). Note the
        /// supplied context may itself be a per-shard child budget.
        bool timed_out = false;
        /// True when a transport stage timeout fired (`connect_timeout_ms` for
        /// resolve/connect/handshake, `request_timeout_ms` for write/read) while the
        /// context was still live. Mutually exclusive with `timed_out`; never retried.
        bool stage_timed_out = false;
    };
    
    /**
     * Construct mTLS client with configuration
     * @param config Client configuration
     */
    explicit MTLSClient(const Config& config);
    
    /**
     * Destructor - cleanup SSL context and connections
     */
    ~MTLSClient();
    
    /**
     * Perform HTTP GET request with mTLS
     * @param endpoint Server endpoint (e.g., "https://shard-001.dc1:8080")
     * @param path Request path (e.g., "/api/v1/status")
     * @return Response with JSON body, or error
     * 
     * @deprecated v2.0: Direct connection creation via MTLSClient is being phased out
     * in favor of EndpointConnectionPool with injected MTLSConnectionFactory.
     * Use the connection pool API instead: MTLSConnectionPoolManager::getConnection().
     * This method remains for backward compatibility but will be removed in v3.0.
     * Transition guide: See include/sharding/mtls_connection_factory.h
     */
    Response get(const std::string& endpoint, const std::string& path);
    
    /**
     * Perform HTTP POST request with mTLS
     * @param endpoint Server endpoint
     * @param path Request path
     * @param body Request body (JSON)
     * @return Response with JSON body, or error
     * 
     * @deprecated v2.0: See get() for migration details.
     * Transition to EndpointConnectionPool with MTLSConnectionFactory.
     */
    Response post(const std::string& endpoint, 
                  const std::string& path,
                  const nlohmann::json& body);

    /**
     * Perform HTTP POST request with mTLS and custom Authorization header
     * @param endpoint Server endpoint
     * @param path Request path
     * @param body Request body (JSON)
     * @param authorization_header Value for the Authorization header (e.g. "Bearer <token>")
     * @return Response with JSON body, or error
     */
    Response post(const std::string& endpoint,
                  const std::string& path,
                  const nlohmann::json& body,
                  const std::string& authorization_header);
    
    /**
     * Perform HTTP PUT request with mTLS
     * @param endpoint Server endpoint
     * @param path Request path
     * @param body Request body (JSON)
     * @return Response with JSON body, or error
     */
    Response put(const std::string& endpoint,
                 const std::string& path,
                 const nlohmann::json& body);
    
    /**
     * Perform HTTP DELETE request with mTLS
     * @param endpoint Server endpoint
     * @param path Request path
     * @return Response with JSON body, or error
     */
    Response del(const std::string& endpoint, const std::string& path);

    /**
     * @brief HTTP GET that can be cancelled or deadline-bounded while in flight.
     *
     * Unlike the legacy overloads (blocking Beast calls where tcp_stream
     * timeouts do not apply), this runs resolve/connect/TLS-handshake/write/read
     * as asynchronous Boost.Asio operations on a private, per-call
     * `io_context` driven by the calling thread. A 10 ms poll timer on that
     * same executor evaluates @p ctx, the per-stage timeout and aborts the
     * pending operation (resolver cancel + socket close), so all handlers are
     * invoked and no thread, socket or timer outlives the call.
     *
     * Limits: token cancellation is observed by polling, so abort latency is up
     * to ~10 ms plus handler dispatch. The retry back-off honours @p ctx.
     * Stage timeouts (connect_timeout_ms for resolve/connect/handshake,
     * request_timeout_ms for write/read) are not retried.
     *
     * @param[in] endpoint Server endpoint (e.g. "https://host:8080").
     * @param[in] path Request path.
     * @param[in] ctx Deadline + cancellation token for this request.
     * @return Response; `cancelled` / `timed_out` distinguish an aborted request
     *         from a normal transport failure (`error` is set in all failure cases).
     */
    Response get(const std::string& endpoint,
                 const std::string& path,
                 const query::RequestContext& ctx);

    /**
     * @brief HTTP POST (JSON body) that can be cancelled or deadline-bounded in flight.
     * @param[in] endpoint Server endpoint.
     * @param[in] path Request path.
     * @param[in] body JSON request body.
     * @param[in] ctx Deadline + cancellation token for this request.
     * @return Response; see get(const std::string&, const std::string&, const query::RequestContext&).
     */
    Response post(const std::string& endpoint,
                  const std::string& path,
                  const nlohmann::json& body,
                  const query::RequestContext& ctx);
    
    /**
     * Check if client is configured and ready
     * @return true if client is ready to make requests
     */
    bool isReady() const;
    
    /**
     * Get current configuration
     * @return Client configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * Close all connections and reset connection pool
     */
    void reset();
    
    /**
     * Get connection pool statistics (JSON format for monitoring)
     * @return JSON object with pool statistics
     */
    nlohmann::json getPoolStatistics() const;
    
    /**
     * Parse endpoint into host and port
     * Supports both IPv4 and IPv6 addresses
     * 
     * Examples:
     *   - IPv4: "192.168.1.1:8080" -> ("192.168.1.1", "8080")
     *   - IPv6 with port: "[2001:db8::1]:8080" -> ("2001:db8::1", "8080")
     *   - IPv6 without port: "2001:db8::1" -> ("2001:db8::1", "8080")
     *   - Hostname: "example.com:9090" -> ("example.com", "9090")
     *   - With protocol: "https://[::1]:8080" -> ("::1", "8080")
     * 
     * @param endpoint Endpoint string to parse
     * @return Pair of (host, port)
     */
    static std::pair<std::string, std::string> parseEndpoint(const std::string& endpoint);

private:
    Config config_;
    
    // Boost.Asio and SSL context (PIMPL to hide Boost headers)
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Connection pool manager (new)
    std::shared_ptr<MTLSConnectionPoolManager> pool_manager_;
    
    /**
     * @brief Asynchronous, cancellable variant of request().
     * @param[in] method HTTP method ("GET" or "POST").
     * @param[in] endpoint Server endpoint.
     * @param[in] path Request path.
     * @param[in] body Optional JSON body.
     * @param[in] authorization_header Optional Authorization header value.
     * @param[in] ctx Deadline + cancellation token.
     * @return Response with cancelled/timed_out flags populated on abort.
     */
    Response requestWithContext(const std::string& method,
                                const std::string& endpoint,
                                const std::string& path,
                                const std::optional<nlohmann::json>& body,
                                const std::string& authorization_header,
                                const query::RequestContext& ctx);

    /**
     * Perform HTTP request with retry logic
     */
    Response request(const std::string& method,
                    const std::string& endpoint,
                    const std::string& path,
                    const std::optional<nlohmann::json>& body = std::nullopt,
                    const std::string& authorization_header = {});
    
    /**
     * Initialize SSL context with certificates
     */
    bool initSSLContext();
    
    /**
     * Verify peer certificate (called during TLS handshake)
     */
    bool verifyPeerCertificate(bool preverified, void* ctx);
};

} // namespace themis::sharding

