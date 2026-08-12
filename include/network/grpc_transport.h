/**
 * @file grpc_transport.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – gRPC native transport for the binary wire protocol (network module).
//
// Provides a gRPC/HTTP2-based alternative transport for the binary wire protocol,
// complementing the TCP wire protocol server (port 8766), the UDP fast-path
// (port 8769), and the QUIC transport (port 8770).
//
// Key properties:
//   - Port 8771 (dedicated; does not conflict with TCP, UDP fast-path, QUIC,
//     or the gRPC API server at 50051)
//   - Carries binary wire protocol frames as raw byte payloads over gRPC
//     bidirectional streaming (generic service; no generated protobuf stubs
//     required in this module)
//   - TLS / mTLS via gRPC credentials – same cert/key files as the TCP server
//   - Per-connection stats for Prometheus integration
//   - Configurable message size limit, connection limit, and thread count
//   - Guarded by THEMIS_ENABLE_GRPC
//
// Design constraints (from ROADMAP.md):
//   "gRPC server is handled by the server module; this module provides only
//    the binary wire protocol."
// This class is the transport layer only.  It accepts gRPC streaming calls
// and forwards raw binary frames to/from clients; it does NOT implement any
// gRPC service (ThemisDBService, WalGrpcService, etc.).  Those live in
// src/server/ and src/api/.

#pragma once

#ifdef THEMIS_ENABLE_GRPC

#include <grpcpp/grpcpp.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis {

class RocksDBWrapper;

namespace network {

/// Default port for the gRPC native wire-protocol transport.
/// Intentionally different from the gRPC API server (50051),
/// TCP wire protocol (8766), UDP fast-path (8769), and QUIC (8770).
constexpr uint16_t kGrpcTransportDefaultPort = 8771;

// ─────────────────────────────────────────────────────────────────────────────
// GrpcTransport
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief gRPC native transport for ThemisDB binary wire protocol (network module).
 *
 * Listens for incoming gRPC connections and multiplexes binary wire protocol
 * frames over gRPC bidirectional streaming RPCs.  Uses gRPC's
 * `AsyncGenericService` so that no proto-generated stubs are required in the
 * network module itself; the framing contract lives in
 * `src/network/themis_wire_v1.proto`.
 *
 * This class is intentionally transport-only.  Business logic (authentication,
 * query routing, storage access) is handled by the server module once frames
 * are dispatched by the frame handler callback.
 *
 * Lifecycle:
 * @code
 *   GrpcTransport::Config cfg;
 *   cfg.port = kGrpcTransportDefaultPort;
 *   GrpcTransport transport(cfg);
 *   transport.start();
 *   // ... serve ...
 *   transport.stop();
 * @endcode
 *
 * Thread safety: start()/stop() must not be called concurrently.
 */
class GrpcTransport {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        std::string host = "0.0.0.0";
        uint16_t    port = kGrpcTransportDefaultPort;

        /// Number of gRPC completion-queue polling threads.
        std::size_t num_threads = 2;

        /// TLS: set tls_enabled=true and provide cert/key PEM paths.
        /// Mirrors GrpcServerConfig in include/api/grpc_server.h.
        bool        tls_enabled       = false;
        std::string tls_cert_path;    ///< PEM server certificate chain
        std::string tls_key_path;     ///< PEM private key
        std::string tls_ca_cert_path; ///< CA cert for mTLS (empty = server-TLS only)
        bool        require_client_cert = false; ///< Enable mutual TLS

        /// Maximum inbound/outbound message size in bytes (default 4 MB).
        int max_message_size_bytes = 4 * 1024 * 1024;

        /// Maximum total active connections (0 = unlimited).
        uint32_t max_connections = 0;

        /// gRPC keepalive: time between keepalive pings in milliseconds.
        int keepalive_time_ms = 30000;

        /// gRPC keepalive: time to wait for a keepalive ping ack in milliseconds.
        int keepalive_timeout_ms = 10000;

        Config() = default;
    };

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t connections_accepted  = 0;
        uint64_t connections_active    = 0;
        uint64_t connections_closed    = 0;
        uint64_t frames_received       = 0;
        uint64_t frames_sent           = 0;
        uint64_t bytes_received        = 0;
        uint64_t bytes_sent            = 0;
        uint64_t parse_errors          = 0;
        uint64_t handshakes_completed  = 0;
        uint64_t connection_limit_drops = 0;
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @param config   Transport configuration.
     * @param storage  Optional storage handle forwarded to frame handlers.
     */
    explicit GrpcTransport(const Config&                   config,
                           std::shared_ptr<RocksDBWrapper> storage = nullptr);

    ~GrpcTransport();

    /// Build server credentials, start listening, and launch I/O threads.
    /// @return true on success, false if the server could not be started
    ///         (invalid config, port in use, TLS load failure, etc.).
    bool start();

    /// Gracefully shut down the gRPC server and join all threads.
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    Stats getStats() const;

    // ── Helpers (public for unit-test access) ────────────────────────────────

    /**
     * @brief Return true if @p port is a valid wire-protocol transport port.
     *
     * Validates that the port is non-zero, not a well-known HTTP/HTTPS port,
     * and does not conflict with any other ThemisDB transport port.
     */
    static bool isValidPort(uint16_t port);

    /// @return The configured listening address ("host:port").
    std::string getAddress() const;

    /// @return The configured port.
    uint16_t getPort() const { return config_.port; }

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    /// Build gRPC server credentials from config_ (TLS or insecure).
    std::shared_ptr<grpc::ServerCredentials> buildCredentials() const;

    /// Load a PEM file; throws std::runtime_error on failure.
    static std::string loadFile(const std::string& path);

    /// Completion-queue drain loop (one per thread).
    void drainCompletionQueue(grpc::ServerCompletionQueue* cq);

    /// Enforce max_connections limit; increments connection_limit_drops on
    /// failure.  Returns false when the limit is exceeded.
    bool checkConnectionLimit();

    // ── Members ──────────────────────────────────────────────────────────────

    Config                          config_;
    std::shared_ptr<RocksDBWrapper> storage_;

    std::unique_ptr<grpc::Server>                        server_;
    std::unique_ptr<grpc::AsyncGenericService>           generic_service_;
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_;
    std::vector<std::thread>                             threads_;

    std::atomic<bool>  running_{false};
    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

}  // namespace network
}  // namespace themis

#endif  // THEMIS_ENABLE_GRPC
