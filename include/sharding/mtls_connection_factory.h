/**
 * @file mtls_connection_factory.h
 * @brief mTLS Connection Factory Interface and Implementation
 * @version 2.0.0
 * @date 2026-07-19
 * 
 * Defines the factory pattern for creating and managing mTLS connections.
 * This interface decouples connection lifecycle management from the pool,
 * enabling dependency injection and testability.
 * 
 * @note This is part of the v2.0 refactor to move connection ownership into the pool.
 */

#pragma once

#include <string>
#include <memory>
#include <optional>
#include <functional>
#include <cstdint>

// Forward declarations for OpenSSL types — avoids pulling in all of <openssl/ssl.h>
// in headers that only need the pointer types.
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace themis::sharding {

/**
 * @brief Custom deleter for OpenSSL SSL connection objects.
 *
 * Used with `std::unique_ptr<SSL, SSLDeleter>` to ensure `SSL_free()` is
 * called when the pointer goes out of scope.  `SSL_free()` also releases the
 * underlying BIO (and thus the socket file descriptor / SOCKET handle) when
 * `BIO_CLOSE` was specified during BIO creation.
 */
struct SSLDeleter {
    void operator()(SSL* ptr) const;
};

/**
 * @brief Abstract interface for mTLS connection factory
 * 
 * Defines the contract for creating and establishing mTLS connections.
 * Implementations should handle:
 * - TLS context configuration
 * - TCP connection establishment
 * - TLS handshake
 * - Error handling and diagnostics
 * 
 * @note Thread-safe: implementations must be safe to call from multiple threads
 */
class IEndpointConnectionFactory {
public:
    virtual ~IEndpointConnectionFactory() = default;
    
    /**
     * @brief Create a new mTLS connection to the specified endpoint
     * 
     * Performs the full connection lifecycle:
     * 1. TCP connect to endpoint
     * 2. TLS handshake with mutual authentication
     * 3. Verify peer certificate
     * 
     * @param endpoint Target endpoint (e.g., "localhost:50051")
     * 
     * @return Fully established SSL connection, or nullopt on failure:
     *   - nullopt: Connection failed, timeout, or handshake error
     *   - unique_ptr<SSL>: Valid connection ready for use
     * 
     * @note Caller owns the returned connection and must release it back
     *       to the pool via EndpointConnectionPool::releaseConnection()
     * 
     * @note Implementation should not throw; return nullopt on errors
     */
    virtual std::optional<std::unique_ptr<SSL, SSLDeleter>> 
    createConnection(const std::string& endpoint) = 0;
};

/**
 * @brief Concrete factory for creating mTLS connections
 * 
 * Uses Boost.Asio and OpenSSL to establish mTLS connections with:
 * - Certificate-based authentication
 * - Configurable TLS versions
 * - Connection timeouts
 * - Peer certificate verification
 * 
 * **Ownership Model:**
 * - Factory must be constructed with a valid SSL context
 * - SSL context lifetime must exceed factory lifetime (and pool lifetime)
 * - Factory is typically created per MTLSClient instance
 * - Multiple pools can share the same factory if desired
 */
class MTLSConnectionFactory : public IEndpointConnectionFactory {
public:
    /**
     * @brief Configuration for connection factory
     */
    struct Config {
        // SSL/TLS context reference (must outlive factory)
        SSL_CTX* ssl_context = nullptr;
        
        // Connection timeouts
        uint32_t connect_timeout_ms = 5000;   // TCP connect timeout
        uint32_t tls_handshake_timeout_ms = 10000; // TLS handshake timeout
        
        // TLS options
        bool verify_peer = true;              // Verify peer certificate
        bool verify_hostname = true;          // Verify hostname match
        
        // Diagnostics
        bool enable_logging = true;           // Log connection attempts
    };
    
    /**
     * @brief Construct factory with SSL context and configuration
     * 
     * @param ssl_context OpenSSL SSL_CTX pointer (must outlive this factory)
     * @param config Factory configuration
     * 
     * @throws std::invalid_argument if ssl_context is null
     */
    MTLSConnectionFactory(SSL_CTX* ssl_context, const Config& config = Config{});
    
    ~MTLSConnectionFactory() override = default;
    
    // Delete copy/move to ensure single ownership model
    MTLSConnectionFactory(const MTLSConnectionFactory&) = delete;
    MTLSConnectionFactory& operator=(const MTLSConnectionFactory&) = delete;
    MTLSConnectionFactory(MTLSConnectionFactory&&) = delete;
    MTLSConnectionFactory& operator=(MTLSConnectionFactory&&) = delete;
    
    /**
     * @brief Create a new mTLS connection to the specified endpoint
     * 
     * Implements the full connection lifecycle:
     * 1. Create SSL object from context
     * 2. Establish TCP connection with timeout
     * 3. Perform TLS handshake
     * 4. Verify peer certificate (if enabled)
     * 5. Verify hostname (if enabled)
     * 
     * @param endpoint Target endpoint (e.g., "localhost:50051")
     * 
     * @return Fully established SSL connection, or nullopt on error:
     *   - TCP connection failed: logs error and returns nullopt
     *   - TLS handshake failed: logs error and returns nullopt
     *   - Certificate verification failed: logs error and returns nullopt
     *   - Success: returns unique_ptr<SSL> ready for use
     * 
     * @note Not const because it may update internal statistics
     */
    std::optional<std::unique_ptr<SSL, SSLDeleter>> 
    createConnection(const std::string& endpoint) override;
    
    /**
     * @brief Get factory configuration
     */
    const Config& getConfig() const { return config_; }
    
private:
    SSL_CTX* ssl_context_;
    Config config_;
    
    /**
     * @brief Parse endpoint into host and port
     * @return Pair of (host, port) or nullopt on invalid format
     */
    static std::optional<std::pair<std::string, std::string>> 
    parseEndpoint(const std::string& endpoint);
};

} // namespace themis::sharding
