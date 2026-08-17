/**
 * @file service_mesh_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <memory>
#include <string>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief REST API handler for service mesh sidecar proxy management.
 *
 * Exposes the network::ServiceMeshIntegration state as a set of read-only
 * REST endpoints under /api/v1/service-mesh/.  When
 * THEMIS_ENABLE_SERVICE_MESH is not compiled in, or when no integration
 * instance is provided, all endpoints return a JSON body indicating the
 * feature is disabled.
 *
 * ## Endpoints
 *
 *   GET /api/v1/service-mesh/status      – running state, probe stats,
 *                                          Envoy/Istio presence, TLS offload
 *   GET /api/v1/service-mesh/config      – configuration snapshot (probe
 *                                          port, drain timeout, inbound and
 *                                          excluded port lists)
 *   GET /api/v1/service-mesh/annotations – Istio traffic annotation hints
 *                                          for Kubernetes pod spec
 *
 * ## Usage
 *
 * @code
 * #ifdef THEMIS_ENABLE_SERVICE_MESH
 *   auto smi = std::make_shared<network::ServiceMeshIntegration>(cfg);
 *   auto handler = ServiceMeshApiHandler(smi);
 * #else
 *   auto handler = ServiceMeshApiHandler();
 * #endif
 * @endcode
 *
 * Thread-safety: all public methods are stateless reads of an externally
 * owned ServiceMeshIntegration.  The integration object must outlive this
 * handler.
 *
 * @note The internal integration pointer is stored as std::shared_ptr<void>
 *       to avoid including network/service_mesh.h in this public header
 *       (the network header is only defined when THEMIS_ENABLE_SERVICE_MESH
 *       is set).  The .cpp file performs the type-safe cast.
 */
class ServiceMeshApiHandler {
public:
    /**
     * @brief Construct a handler with an optional live integration object.
     *
     * @param smi  Shared pointer to a network::ServiceMeshIntegration
     *             instance, or nullptr (default) when service mesh support
     *             is not compiled in or not started.
     */
    explicit ServiceMeshApiHandler(std::shared_ptr<void> smi = nullptr);

    ~ServiceMeshApiHandler() = default;

    // Non-copyable, movable
    ServiceMeshApiHandler(const ServiceMeshApiHandler&) = delete;
    ServiceMeshApiHandler& operator=(const ServiceMeshApiHandler&) = delete;
    ServiceMeshApiHandler(ServiceMeshApiHandler&&) noexcept = default;
    ServiceMeshApiHandler& operator=(ServiceMeshApiHandler&&) noexcept = default;

    /**
     * @brief Handle GET /api/v1/service-mesh/status.
     *
     * Returns a JSON object with:
     *  - "enabled"       : bool  – true when compiled with service mesh support
     *  - "running"       : bool  – true when probe server is active
     *  - "envoy_present" : bool  – Envoy/Istio sidecar detection result
     *  - "tls_offloaded" : bool  – whether TLS is delegated to the sidecar
     *  - "address"       : string – probe server bind address (host:port)
     *  - "stats"         : object – healthz/readyz request and OK counters
     *
     * @param req Incoming HTTP request.
     * @return HTTP 200 with JSON body.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req) const;

    /**
     * @brief Handle GET /api/v1/service-mesh/config.
     *
     * Returns the current ServiceMeshIntegration::Config as JSON:
     *  - "host"                      : string
     *  - "probe_port"                : uint16
     *  - "trust_sidecar_mtls"        : bool
     *  - "drain_timeout_ms"          : uint32
     *  - "inbound_ports"             : string (comma-separated)
     *  - "excluded_ports"            : string (comma-separated)
     *  - "propagate_tracing_headers" : bool
     *
     * @param req Incoming HTTP request.
     * @return HTTP 200 with JSON body.
     */
    http::response<http::string_body> handleConfig(
        const http::request<http::string_body>& req) const;

    /**
     * @brief Handle GET /api/v1/service-mesh/annotations.
     *
     * Returns the recommended Istio pod-annotation key/value pairs derived
     * from the current configuration.  The client can apply these to the
     * Kubernetes pod spec to ensure correct Envoy traffic interception:
     *
     * @code
     * {
     *   "traffic.sidecar.istio.io/includeInboundPorts": "8766,8080",
     *   "traffic.sidecar.istio.io/excludeInboundPorts": "8082,8769"
     * }
     * @endcode
     *
     * @param req Incoming HTTP request.
     * @return HTTP 200 with JSON body.
     */
    http::response<http::string_body> handleAnnotations(
        const http::request<http::string_body>& req) const;

private:
    // Opaque pointer to network::ServiceMeshIntegration (type-erased to
    // avoid including the conditionally-compiled network header here).
    std::shared_ptr<void> smi_;

    // Build a simple JSON HTTP response.
    http::response<http::string_body> makeJson(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req) const;

    // Build a "feature disabled" response.
    http::response<http::string_body> makeDisabled(
        const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis
