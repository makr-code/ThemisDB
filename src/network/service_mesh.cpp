/**
 * @file service_mesh.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Istio/Envoy service mesh integration for the network module.
// See include/network/service_mesh.h for design documentation.

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include "network/service_mesh.h"
#include "utils/logger.h"

#include <chrono>
#include <cstdlib>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>

namespace themis::network {

namespace {

constexpr int kShutdownJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
static void timedJoin(std::thread& t,
                      int timeout_ms = kShutdownJoinTimeoutMs) noexcept {
    if (!t.joinable()) {
      return;
    }
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable()) {
          inner.join();
        }
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) !=
            std::future_status::ready) {
        // thread_join_no_timeout: detach on deadline to avoid indefinite block
        THEMIS_WARN("Thread did not finish within {} ms during shutdown; detaching.",
                    timeout_ms);
    }
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

ServiceMeshIntegration::ServiceMeshIntegration(const Config& config)
    : config_(config)
{}

ServiceMeshIntegration::~ServiceMeshIntegration() noexcept {
    try {
        stop();
    } catch (...) {
        // Suppress exceptions in destructor; stop() failure is non-critical
        THEMIS_WARN("ServiceMeshIntegration::stop() threw exception during destruction");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Port validation
// ─────────────────────────────────────────────────────────────────────────────

/* static */
bool ServiceMeshIntegration::isValidPort([[maybe_unused]] uint16_t port) {
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
            // no_timeout: SO_SNDTIMEO is POSIX-standard (Linux, macOS, FreeBSD).
            // Windows uses a DWORD millisecond value instead of timeval.
#if defined(_WIN32)
            {
                DWORD timeout_ms = 5000;
                ::setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO,
                             reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
            }
#else
            {
                struct timeval tv{5, 0};  // 5 s send deadline
                ::setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO,
                             reinterpret_cast<const char*>(&tv), sizeof(tv));
            }
#endif
            boost::asio::write(socket, net::buffer(resp), ec);
            return;
        }

        const std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n" +
            body;

        // no_timeout: SO_SNDTIMEO is POSIX-standard (Linux, macOS, FreeBSD).
        // Windows uses a DWORD millisecond value instead of timeval.
#if defined(_WIN32)
        {
            DWORD timeout_ms = 5000;
            ::setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO,
                         reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
        }
#else
        {
            struct timeval tv{5, 0};  // 5 s send deadline
            ::setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO,
                         reinterpret_cast<const char*>(&tv), sizeof(tv));
        }
#endif
        boost::asio::write(socket, net::buffer(resp), ec);

    } catch (const std::exception& ex) {
        THEMIS_WARN("[ServiceMesh] probe handler exception: {}", ex.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Accept loop
// ─────────────────────────────────────────────────────────────────────────────

void ServiceMeshIntegration::acceptLoop() {
    // R10: Add timeout enforcement to accept loop using Boost.Asio deadline timer.
    // The mesh health probe server should not block indefinitely on accept().
    // Use 30-second deadline as this is for occasional Kubernetes health probes.
    const int timeout_ms = 30000;
    const auto poll_interval = std::chrono::milliseconds(10);

    boost::system::error_code ec;
    acceptor_->non_blocking(true, ec);
    if (ec) {
        THEMIS_WARN("[ServiceMesh] Failed to enable non-blocking accept: {}",
                    ec.message());
        return;
    }

    while (running_.load(std::memory_order_acquire)) {
        tcp::socket socket(*io_ctx_);

        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(timeout_ms);
        bool accepted = false;

        while (running_.load(std::memory_order_acquire)) {
            ec.clear();
            acceptor_->accept(socket, ec);
            if (!ec) {
                accepted = true;
                break;
            }

            if (ec == boost::asio::error::operation_aborted) {
                break;
            }

            if (ec == boost::asio::error::would_block ||
                ec == boost::asio::error::try_again) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    THEMIS_DEBUG("[ServiceMesh] Accept timeout after {} ms",
                                 timeout_ms);
                    break;
                }
                std::this_thread::sleep_for(poll_interval);
                continue;
            }

            THEMIS_WARN("[ServiceMesh] Accept error: {}", ec.message());
            break;
        }

        if (!running_.load(std::memory_order_acquire) ||
            ec == boost::asio::error::operation_aborted) {
            break;
        }

        if (!accepted) {
            continue;
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
        timedJoin(t);
    }
    threads_.clear();

    acceptor_.reset();
    io_ctx_.reset();

    THEMIS_INFO("[ServiceMesh] probe server stopped");
}

}  // namespace themis::network

#endif  // THEMIS_ENABLE_SERVICE_MESH
