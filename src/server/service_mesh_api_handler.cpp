/**
 * @file service_mesh_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Service Mesh REST API handler.
// See include/server/service_mesh_api_handler.h for design documentation.

#include "server/service_mesh_api_handler.h"

#include <nlohmann/json.hpp>

#include "utils/tracing.h"

#ifdef THEMIS_ENABLE_SERVICE_MESH
#include "network/service_mesh.h"
#endif

namespace themis {
namespace server {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

ServiceMeshApiHandler::ServiceMeshApiHandler(std::shared_ptr<void> smi)
    : smi_(std::move(smi))
{}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ServiceMeshApiHandler::makeJson(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ServiceMeshApiHandler::makeDisabled(
    const http::request<http::string_body>& req) const
{
    json body = {
        {"enabled", false},
        {"message", "Service mesh support is not compiled in this build "
                    "(recompile with THEMIS_ENABLE_SERVICE_MESH=ON)"}
    };
    return makeJson(http::status::ok, body.dump(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/service-mesh/status
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ServiceMeshApiHandler::handleStatus(
    const http::request<http::string_body>& req) const
{
    auto span = ::themis::Tracer::startSpan("handleStatus");
#ifdef THEMIS_ENABLE_SERVICE_MESH
    if (!smi_) {
        json body = {
            {"enabled", true},
            {"running", false},
            {"message", "ServiceMeshIntegration not started"}
        };
        return makeJson(http::status::ok, body.dump(), req);
    }

    auto* smi = static_cast<network::ServiceMeshIntegration*>(smi_.get());
    const auto stats = smi->getStats();

    json body = {
        {"enabled",       true},
        {"running",       smi->isRunning()},
        {"envoy_present", smi->isEnvoyPresent()},
        {"tls_offloaded", smi->isTLSOffloadedToSidecar()},
        {"address",       smi->getAddress()},
        {"stats", {
            {"healthz_requests", stats.healthz_requests},
            {"readyz_requests",  stats.readyz_requests},
            {"healthz_ok",       stats.healthz_ok},
            {"readyz_ok",        stats.readyz_ok}
        }}
    };
    return makeJson(http::status::ok, body.dump(), req);
#else
    return makeDisabled(req);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/service-mesh/config
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ServiceMeshApiHandler::handleConfig(
    const http::request<http::string_body>& req) const
{
    auto span = ::themis::Tracer::startSpan("handleConfig");
#ifdef THEMIS_ENABLE_SERVICE_MESH
    if (!smi_) {
        json body = {
            {"enabled", true},
            {"message", "ServiceMeshIntegration not started"}
        };
        return makeJson(http::status::ok, body.dump(), req);
    }

    auto* smi = static_cast<network::ServiceMeshIntegration*>(smi_.get());

    json body = {
        {"enabled",                    true},
        {"probe_port",                 smi->getPort()},
        {"inbound_ports",              smi->getInboundPorts()},
        {"excluded_ports",             smi->getExcludedPorts()},
        {"tls_offloaded_to_sidecar",   smi->isTLSOffloadedToSidecar()},
        {"address",                    smi->getAddress()}
    };
    return makeJson(http::status::ok, body.dump(), req);
#else
    return makeDisabled(req);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/service-mesh/annotations
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ServiceMeshApiHandler::handleAnnotations(
    const http::request<http::string_body>& req) const
{
    auto span = ::themis::Tracer::startSpan("handleAnnotations");
#ifdef THEMIS_ENABLE_SERVICE_MESH
    if (!smi_) {
        json body = {
            {"enabled", true},
            {"message", "ServiceMeshIntegration not started"}
        };
        return makeJson(http::status::ok, body.dump(), req);
    }

    auto* smi = static_cast<network::ServiceMeshIntegration*>(smi_.get());

    // Return the recommended Istio pod-annotation key/value pairs so that
    // operators can copy them directly into the Kubernetes pod spec.
    json body = {
        {"traffic.sidecar.istio.io/includeInboundPorts",
            smi->getInboundPorts()},
        {"traffic.sidecar.istio.io/excludeInboundPorts",
            smi->getExcludedPorts()}
    };
    return makeJson(http::status::ok, body.dump(), req);
#else
    return makeDisabled(req);
#endif
}

} // namespace server
} // namespace themis
