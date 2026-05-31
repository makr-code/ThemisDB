/*
 * ThemisDB | File: grpc_server.h | Version: 0.0.15 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 147
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2730 [api] gRPC API surface alon... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    mutable std::mutex                mutex_;

    /// Load the contents of a PEM file.  Throws std::runtime_error on failure.
    static std::string loadFile(const std::string& path);

    /// Build TLS or insecure server credentials from config_.
    std::shared_ptr<grpc::ServerCredentials> buildCredentials() const;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_GRPC
