/**
 * @file service_mesh.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Istio/Envoy service mesh integration for the network module.
//
// Provides Istio/Envoy sidecar compatibility for ThemisDB deployments in
// Kubernetes service mesh environments.
//
// Key capabilities:
//   - Dedicated probe HTTP server (port 8082) serving /healthz and /readyz
//     over plain HTTP/1.1 (exempt from Envoy mTLS interception by design)
//   - TLS offload detection: when Envoy handles mTLS, ThemisDB internal TLS
//     can be disabled by setting trust_sidecar_mtls=true
//   - Graceful shutdown with configurable sidecar drain timeout
//   - Envoy/Istio environment detection via well-known environment variables
//   - Port annotations helpers for Istio traffic.sidecar.istio.io annotations
//
// Design constraints:
//   - No dependency on Istio/Envoy SDKs; compatibility is achieved through
//     HTTP/1.1 health endpoints and conventional header/env-var detection
//   - Transport-layer only; does not implement service mesh control-plane logic
//   - Probe server is intentionally minimal (plain HTTP, no TLS) so that
//     Kubernetes kubelet health checks succeed even when Envoy is degraded
//   - Guarded by THEMIS_ENABLE_SERVICE_MESH
//
// Port allocation (does not conflict with other ThemisDB transports):
//   8080 – HTTP API server
//   8081 – general health endpoint (configured via themisdb.healthPort)
//   8082 – service mesh probe server (this module)
//   8766 – TCP binary wire protocol
//   8769 – UDP fast-path
//   8770 – QUIC transport
//   8771 – gRPC native transport

#pragma once

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include <boost/asio.hpp>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace network {

namespace net = boost::asio;
using tcp = net::ip::tcp;

/// Default port for the service mesh probe HTTP server.
/// Plain HTTP (not intercepted by Envoy) serving /healthz and /readyz.
constexpr uint16_t kServiceMeshProbeDefaultPort = 8082;

// ─────────────────────────────────────────────────────────────────────────────
// ServiceMeshIntegration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Istio/Envoy service mesh integration for ThemisDB (network module).
 *
 * Starts a minimal HTTP/1.1 probe server on a dedicated port to serve
 * Kubernetes liveness (/healthz) and readiness (/readyz) probes.  The probe
 * port is excluded from Envoy traffic interception so that health checks
 * continue to work even when the Envoy sidecar is unavailable or mTLS is
 * active on the main wire protocol port.
 *
 * When @c trust_sidecar_mtls is true, the caller should disable ThemisDB-level
 * TLS on the wire protocol server (@c WireProtocolServer::Config::enable_tls)
 * and let the Istio sidecar enforce mutual TLS transparently.
 *
 * Lifecycle:
 * @code
 *   ServiceMeshIntegration::Config cfg;
 *   cfg.trust_sidecar_mtls = true;  // Envoy handles mTLS
 *   ServiceMeshIntegration smi(cfg);
 *   smi.start();
 *   // ... serve ...
 *   smi.stop();
 * @endcode
 *
 * Thread safety: start()/stop() must not be called concurrently.
 */
class ServiceMeshIntegration {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        std::string host       = "0.0.0.0";
        uint16_t    probe_port = kServiceMeshProbeDefaultPort;

        /// When true, ThemisDB-level TLS is delegated to the Envoy sidecar.
        /// Callers should set WireProtocolServer::Config::enable_tls = false.
        bool trust_sidecar_mtls = false;

        /// Graceful shutdown: wait up to this many milliseconds for the Envoy
        /// sidecar to drain in-flight connections before the process exits.
        uint32_t drain_timeout_ms = 5000;

        /// Comma-separated ports included in Istio inbound traffic interception.
        /// Maps to traffic.sidecar.istio.io/includeInboundPorts annotation.
        std::string inbound_ports = "8766,8080";

        /// Comma-separated ports excluded from Istio inbound interception.
        /// Maps to traffic.sidecar.istio.io/excludeInboundPorts annotation.
        /// Typically includes the probe port and UDP fast-path to avoid
        /// intercepting health checks and UDP traffic.
        std::string excluded_ports = "8082,8769";

        /// Propagate Envoy/B3/W3C distributed tracing headers across
        /// service boundaries when forwarding requests.
        bool propagate_tracing_headers = true;

        Config() = default;
    };

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t healthz_requests = 0; ///< Total /healthz probe requests
        uint64_t readyz_requests  = 0; ///< Total /readyz  probe requests
        uint64_t healthz_ok       = 0; ///< Successful liveness responses
        uint64_t readyz_ok        = 0; ///< Successful readiness responses
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    explicit ServiceMeshIntegration(const Config& config = Config{});

    virtual ~ServiceMeshIntegration() noexcept;

    /// Bind the probe TCP socket and start the accept loop thread.
    /// @return true on success, false if the port could not be bound.
    bool start();

    /// Signal the accept loop to stop, wait for the drain timeout, then
    /// join the accept thread.  Safe to call on a never-started instance.
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    // ── Service mesh detection ────────────────────────────────────────────────

    /**
     * @brief Return true when an Envoy/Istio sidecar appears to be present.
     *
     * Detection heuristic: the @c KUBERNETES_SERVICE_HOST environment variable
     * is set in every Kubernetes pod.  Additionally, @c ISTIO_META_MESH_ID or
     * @c PILOT_CERT_PROVIDER signals that the Istio agent is active.
     *
     * This is a best-effort, read-only check that never blocks.
     */
    bool isEnvoyPresent() const;

    /// Return true when TLS is delegated to the Envoy sidecar.
    bool isTLSOffloadedToSidecar() const { return config_.trust_sidecar_mtls; }

    Stats getStats() const;

    // ── Helpers (public for unit-test access) ────────────────────────────────

    /**
     * @brief Return true if @p port is a valid service mesh probe port.
     *
     * Validates that the port is non-zero, not a well-known HTTP/HTTPS port,
     * and does not conflict with other ThemisDB transport ports.
     */
    static bool isValidPort(uint16_t port);

    /// @return The probe server listening address ("host:probe_port").
    std::string getAddress() const;

    /// @return The configured probe port.
    uint16_t getPort() const { return config_.probe_port; }

    /// @return The configured inbound ports hint for Istio annotation.
    const std::string& getInboundPorts()  const { return config_.inbound_ports;  }

    /// @return The configured excluded ports hint for Istio annotation.
    const std::string& getExcludedPorts() const { return config_.excluded_ports; }

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    /// Accept-loop running on accept_thread_.
    void acceptLoop();

    /// Serve a single probe HTTP/1.1 connection.
    void serveProbe(tcp::socket socket);

    // ── Members ──────────────────────────────────────────────────────────────

    Config config_;

    std::unique_ptr<net::io_context> io_ctx_;
    std::unique_ptr<tcp::acceptor>   acceptor_;
    std::vector<std::thread>         threads_;

    std::atomic<bool> running_{false};

    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

}  // namespace network
}  // namespace themis

#endif  // THEMIS_ENABLE_SERVICE_MESH
