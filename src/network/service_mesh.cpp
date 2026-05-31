/*
 * ThemisDB | File: service_mesh.cpp | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 254
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=7, L=0
 * PR History (last 5): #3632 fix(build): register 40+ mi... (2026-03-12) | #3395 Add service mesh sidecar pr... (2026-03-12) | #3337 feat(network): Implement Is... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// ThemisDB – Istio/Envoy service mesh integration for the network module.
// See include/network/service_mesh.h for design documentation.

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include "network/service_mesh.h"
#include "utils/logger.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>

namespace themis::network {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

ServiceMeshIntegration::ServiceMeshIntegration(const Config& config)
    : config_(config)
{}

ServiceMeshIntegration::~ServiceMeshIntegration() {
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Port validation
// ─────────────────────────────────────────────────────────────────────────────

/* static */
bool ServiceMeshIntegration::isValidPort(uint16_t port) {
    // Reject port 0 and well-known HTTP/HTTPS ports.
    if (port == 0 || port == 80 || port == 443) {
        return false;
    }
    // Reject other ThemisDB transport ports:
    //   8080 – HTTP API server
    //   8081 – general health endpoint
    //   8766 – TCP binary wire protocol
    //   8769 – UDP fast-path
    //   8770 – QUIC transport
    //   8771 – gRPC native transport
    //   50051 – gRPC API server (server/api module)
    if (port == 8080 || port == 8081 ||
        port == 8766 || port == 8769 || port == 8770 || port == 8771 ||
        port == 50051) {
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string ServiceMeshIntegration::getAddress() const {
    return config_.host + ":" + std::to_string(config_.probe_port);
}

bool ServiceMeshIntegration::isEnvoyPresent() const {
    // KUBERNETES_SERVICE_HOST is set in every Kubernetes pod.
    if (std::getenv("KUBERNETES_SERVICE_HOST") != nullptr) {
        return true;
    }
    // Istio agent sets ISTIO_META_MESH_ID or PILOT_CERT_PROVIDER.
    if (std::getenv("ISTIO_META_MESH_ID") != nullptr ||
        std::getenv("PILOT_CERT_PROVIDER") != nullptr) {
        return true;
    }
    return false;
}

ServiceMeshIntegration::Stats ServiceMeshIntegration::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Probe HTTP/1.1 connection handler
// ─────────────────────────────────────────────────────────────────────────────

void ServiceMeshIntegration::serveProbe(tcp::socket socket) {
    try {
        // Read up to 4 KB – enough for a health check GET request with tracing
        // headers (x-b3-traceid, x-request-id, x-forwarded-for, etc.).
        std::array<char, 4096> buf{};
        boost::system::error_code ec;
        const std::size_t n = socket.read_some(net::buffer(buf), ec);
        if (ec && ec != boost::asio::error::eof) {
            return;
        }

        const std::string req(buf.data(), n);

        // Determine the requested path.  Match "GET /healthz " or
        // "GET /healthz\r" to avoid false-positives on longer paths such as
        // "GET /healthz_details" or "GET /api/healthz".
        const bool is_healthz =
            (req.find("GET /healthz ")  != std::string::npos ||
             req.find("GET /healthz\r") != std::string::npos);
        const bool is_readyz  =
            (req.find("GET /readyz ")   != std::string::npos ||
             req.find("GET /readyz\r")  != std::string::npos);

        std::string body;
        if (is_healthz) {
            body = "OK";
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.healthz_requests;
            ++stats_.healthz_ok;
        } else if (is_readyz) {
            body = "OK";
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.readyz_requests;
            ++stats_.readyz_ok;
        } else {
            // Unknown path – return 404.
            const std::string resp =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 9\r\n"
                "Connection: close\r\n\r\n"
                "Not Found";
            boost::asio::write(socket, net::buffer(resp), ec);
            return;
        }

        const std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n" +
            body;

        boost::asio::write(socket, net::buffer(resp), ec);

    } catch (const std::exception& ex) {
        THEMIS_WARN("[ServiceMesh] probe handler exception: {}", ex.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Accept loop
// ─────────────────────────────────────────────────────────────────────────────

void ServiceMeshIntegration::acceptLoop() {
    while (running_.load(std::memory_order_acquire)) {
        boost::system::error_code ec;
        tcp::socket socket(*io_ctx_);
        acceptor_->accept(socket, ec);
        if (ec) {
            // boost::asio::error::operation_aborted is expected on stop().
            break;
        }
        serveProbe(std::move(socket));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// start / stop
// ─────────────────────────────────────────────────────────────────────────────

bool ServiceMeshIntegration::start() {
    if (running_.load(std::memory_order_acquire)) {
        THEMIS_WARN("[ServiceMesh] start() called while already running");
        return false;
    }

    const std::string address = getAddress();

    try {
        io_ctx_ = std::make_unique<net::io_context>(1);

        const net::ip::address bind_addr =
            net::ip::make_address(config_.host);
        const tcp::endpoint ep(bind_addr, config_.probe_port);

        acceptor_ = std::make_unique<tcp::acceptor>(*io_ctx_);
        acceptor_->open(ep.protocol());
        acceptor_->set_option(tcp::acceptor::reuse_address(true));
        acceptor_->bind(ep);
        acceptor_->listen(net::socket_base::max_listen_connections);

        running_.store(true, std::memory_order_release);

        THEMIS_INFO(
            "[ServiceMesh] probe server listening on {} "
            "(Istio inbound: {}, excluded: {})",
            address, config_.inbound_ports, config_.excluded_ports);

        if (config_.trust_sidecar_mtls) {
            THEMIS_INFO(
                "[ServiceMesh] TLS offloaded to Envoy sidecar "
                "(trust_sidecar_mtls=true)");
        }

        threads_.emplace_back([this] { acceptLoop(); });

    } catch (const std::exception& ex) {
        THEMIS_ERROR("[ServiceMesh] start() failed for {}: {}", address, ex.what());
        acceptor_.reset();
        io_ctx_.reset();
        return false;
    }

    return true;
}

void ServiceMeshIntegration::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    THEMIS_INFO("[ServiceMesh] shutting down probe server {}", getAddress());

    // Honour the configured drain timeout so that the Envoy sidecar has time
    // to stop routing new connections before we close the probe server.
    if (config_.drain_timeout_ms > 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.drain_timeout_ms));
    }

    if (acceptor_ && acceptor_->is_open()) {
        boost::system::error_code ec;
        acceptor_->close(ec);
    }

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();

    acceptor_.reset();
    io_ctx_.reset();

    THEMIS_INFO("[ServiceMesh] probe server stopped");
}

}  // namespace themis::network

#endif  // THEMIS_ENABLE_SERVICE_MESH
