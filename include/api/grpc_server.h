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

struct GrpcServerConfig {
    std::string host         = "0.0.0.0";
    uint16_t    port         = 50051;        ///< Standard gRPC port
    bool        tls_enabled  = false;
    std::string tls_cert_path;               ///< PEM server certificate
    std::string tls_key_path;                ///< PEM private key
    std::string tls_ca_cert_path;            ///< CA cert for mTLS (optional)
    bool        require_client_cert = false; ///< Enable mutual TLS

    int max_message_size_bytes = 100 * 1024 * 1024;
};

class GrpcApiServer {
public:
    GrpcApiServer();
    ~GrpcApiServer();

    // Non-copyable, movable
    GrpcApiServer(const GrpcApiServer&)            = delete;
    GrpcApiServer& operator=(const GrpcApiServer&) = delete;
    GrpcApiServer(GrpcApiServer&&)                 noexcept = default;
    GrpcApiServer& operator=(GrpcApiServer&&)      noexcept = default;

    /**
     * @brief Initialize.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool initialize(const GrpcServerConfig& config);

    /**
     * @brief Register Service.
     * @param[in,out] service Input/output parameter.
     */
    void registerService(grpc::Service* service);

    /**
     * @brief Start.
     * @return True when the operation succeeds.
     */
    bool start();

    /**
     * @brief Stop.
     */
    void stop();

    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    bool isRunning() const;

    /**
     * @brief Get Address.
     * @return Return value.
     */
    std::string getAddress() const;

    /**
     * @brief Get Port.
     * @return Return value.
     */
    uint16_t getPort() const;

private:
    GrpcServerConfig                  config_;
    std::unique_ptr<grpc::Server>     server_;
    std::atomic<bool>                 running_{false};
    std::string                       server_address_;
    std::vector<grpc::Service*>       services_;
    mutable std::timed_mutex          mutex_;

    /**
     * @brief Load File.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string loadFile(const std::string& path);

    /**
     * @brief Build Credentials.
     * @return Return value.
     */
    std::shared_ptr<grpc::ServerCredentials> buildCredentials() const;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_GRPC
