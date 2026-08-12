/**
 * @file grpc_server.h
 * @brief gRPC API server for ThemisDB.
 *
 * @details Provides HTTP/2 + Protocol Buffer transport for database operations,
 * running in parallel with the existing Beast HTTP/REST server.
 *
 * Core components:
 *  - `GrpcServerConfig`: Configuration for host, port, TLS, max message size
 *  - `GrpcApiServer`: Lifecycle management (initialize, start, stop)
 *
 * Architecture:
 *  - Reuses existing `ThemisCoreServiceImpl` — no business-logic duplication
 *  - TLS credentials sourced from same cert/key pair as REST server
 *  - gRPC reflection exposed in debug builds only (prevents schema leakage in production)
 *  - Independent of HTTP server lifecycle (can start/stop separately)
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Fail-closed guards (QW-42): all config parameters validated before initialization
 *  - Port must be in [1, 65535]; max_message_size_bytes clamped to 100 MB
 *  - Host field must be non-empty and <= 256 chars (prevents resource exhaustion)
 *  - TLS: if enabled, cert_path and key_path must be non-empty
 *
 * ### Lifecycle
 * ```cpp
 * GrpcApiServer srv;
 * GrpcServerConfig cfg;
 * cfg.port = 50051;
 * cfg.tls_enabled = true;
 * cfg.tls_cert_path = "/etc/themis/server.pem";
 * cfg.tls_key_path = "/etc/themis/server-key.pem";
 *
 * if (!srv.initialize(cfg)) {
 *     // Configuration validation failed (fail-closed)
 *     return error;
 * }
 * srv.registerService(&my_core_service);
 * if (!srv.start()) {
 *     // Failed to bind or start listening
 *     return error;
 * }
 * // ... serve ...
 * srv.stop();  // Graceful shutdown
 * ```
 *
 * ### Thread safety
 * - `initialize()`, `registerService()`, `start()`, `stop()` should only be called
 *   from a single initialization thread (not concurrent safe)
 * - Service implementation callbacks may be invoked concurrently from the gRPC thread pool
 *
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 */


#pragma once

#ifdef THEMIS_ENABLE_GRPC

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <vector>

// Forward declarations
namespace themis {
namespace core {
class ThemisCoreServiceImpl;
} // namespace core
} // namespace themis

namespace themis {
namespace api {

/**
 * @brief Configuration for the gRPC API server.
 *
 * TLS settings mirror those of the Beast HTTP server so both transports
 * share the same certificate/key pair in production.
 */
struct GrpcServerConfig {
    std::string host         = "0.0.0.0";
    uint16_t    port         = 50051;        ///< Standard gRPC port
    bool        tls_enabled  = false;
    std::string tls_cert_path;               ///< PEM server certificate
    std::string tls_key_path;                ///< PEM private key
    std::string tls_ca_cert_path;            ///< CA cert for mTLS (optional)
    bool        require_client_cert = false; ///< Enable mutual TLS

    /// Maximum inbound/outbound message size in bytes (default 100 MB)
    int max_message_size_bytes = 100 * 1024 * 1024;
};

/**
 * @brief gRPC API server for ThemisDB.
 *
 * Runs alongside the existing Beast HTTP/REST server and exposes the same
 * database operations over gRPC (HTTP/2 + Protocol Buffers).
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Reuses the existing ThemisCoreServiceImpl; no business-logic duplication.
 *  - TLS credentials are configured from the same cert/key pair used by REST.
 *  - gRPC reflection is exposed in debug builds only to prevent schema
 *    leakage in production.
 *  - Does not affect existing REST endpoints.
 *
 * Lifecycle:
 * @code
 *   GrpcApiServer srv;
 *   GrpcServerConfig cfg;
 *   cfg.port = 50051;
 *   srv.initialize(cfg);
 *   srv.registerService(&my_core_service_impl);
 *   srv.start();
 *   // ... serve ...
 *   srv.stop();
 * @endcode
 */
class GrpcApiServer {
public:
    GrpcApiServer();
    ~GrpcApiServer();

    // Non-copyable, movable
    GrpcApiServer(const GrpcApiServer&)            = delete;
    GrpcApiServer& operator=(const GrpcApiServer&) = delete;
    GrpcApiServer(GrpcApiServer&&)                 = default;
    GrpcApiServer& operator=(GrpcApiServer&&)      = default;

    /**
     * @brief Configure the server.  Must be called before start().
     * @param config  Server configuration.
     * @return true on success, false on invalid configuration.
     * 
     * @note Fail-closed guards (QW-42): Validates all configuration parameters:
     *   - port: must be in range [1, 65535] (fail-closed: rejects 0 and > 65535)
     *   - host: must be non-empty and <= 256 characters (prevents resource exhaustion)
     *   - TLS: if enabled, cert_path and key_path must be non-empty
     *   - max_message_size_bytes: must be in range (0, 1 GB], clamped to 100 MB default
     * Returns false (fail-closed) if any guard fails; server remains uninitialized.
     */
    bool initialize(const GrpcServerConfig& config);

    /**
     * @brief Register a gRPC service implementation.
     *
     * Must be called after initialize() and before start().
     * @param service  Non-owning pointer to a grpc::Service implementation.
     *                 The caller is responsible for the lifetime of the object.
     */
    void registerService(grpc::Service* service);

    /**
     * @brief Start listening and serving requests.
     *
     * In NDEBUG builds the gRPC reflection service is NOT registered so that
     * the proto schema is not exposed to unauthenticated callers.  In debug
     * builds reflection is registered automatically to aid development.
     *
     * @return true if the server started successfully.
     */
    bool start();

    /**
     * @brief Gracefully shut down the server.
     *
     * Blocks until all in-flight RPCs complete or the deadline expires.
     */
    void stop();

    /// @return true while the server is accepting connections.
    bool isRunning() const;

    /// @return The listening address ("host:port").
    std::string getAddress() const;

    /// @return The configured port number.
    uint16_t getPort() const;

private:
    GrpcServerConfig                  config_;
    std::unique_ptr<grpc::Server>     server_;
    std::atomic<bool>                 running_{false};
    std::string                       server_address_;
    std::vector<grpc::Service*>       services_;
    mutable std::timed_mutex          mutex_;

    /// Load the contents of a PEM file.  Throws std::runtime_error on failure.
    static std::string loadFile(const std::string& path);

    /// Build TLS or insecure server credentials from config_.
    std::shared_ptr<grpc::ServerCredentials> buildCredentials() const;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_GRPC
