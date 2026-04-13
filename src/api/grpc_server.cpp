/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_server.cpp                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:23:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     272                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 97cd900111  2026-03-25  feat(api): gRPC Phase 4 – mutex fix, deadline, RPC stubs,... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8f4f0c9ea2  2026-02-23  Implement gRPC API server alongside REST (src/api/grpc_se... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifdef THEMIS_ENABLE_GRPC

#include "api/grpc_server.h"
#include "utils/logger.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

// gRPC reflection is only compiled in debug builds to prevent schema leakage
// in production (FUTURE_ENHANCEMENTS.md – Security / Reliability).
#if !defined(NDEBUG) && !defined(THEMIS_TEST_BUILD)
#  include <grpcpp/ext/proto_server_reflection_plugin.h>
#endif

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

GrpcApiServer::GrpcApiServer() = default;

GrpcApiServer::~GrpcApiServer() {
    if (running_) {
        stop();
    }
}

// ---------------------------------------------------------------------------
// initialize()
// ---------------------------------------------------------------------------

bool GrpcApiServer::initialize(const GrpcServerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        THEMIS_WARN("GrpcApiServer::initialize called while server is already running");
        return false;
    }

    if (config.port == 0) {
        THEMIS_ERROR("GrpcApiServer::initialize – invalid port 0");
        return false;
    }

    config_         = config;
    server_address_ = config_.host + ":" + std::to_string(config_.port);

    THEMIS_INFO("GrpcApiServer initialized, will listen on " + server_address_);
    return true;
}

// ---------------------------------------------------------------------------
// registerService()
// ---------------------------------------------------------------------------

void GrpcApiServer::registerService(grpc::Service* service) {
    if (!service) {
        THEMIS_WARN("GrpcApiServer::registerService – null service pointer ignored");
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    services_.push_back(service);
    THEMIS_INFO("GrpcApiServer: registered gRPC service");
}

// ---------------------------------------------------------------------------
// start()
// ---------------------------------------------------------------------------

bool GrpcApiServer::start() {
    // Validate state under the lock, then release before the blocking
    // BuildAndStart() call (which binds a network socket).  Holding the mutex
    // across BuildAndStart() would prevent other threads from calling stop()
    // or any accessor while the socket is being opened.
    std::unique_lock<std::mutex> lock(mutex_);

    if (running_) {
        THEMIS_WARN("GrpcApiServer::start – server is already running on " + server_address_);
        return false;
    }

    if (server_address_.empty()) {
        THEMIS_ERROR("GrpcApiServer::start – call initialize() first");
        return false;
    }

    if (services_.empty()) {
        THEMIS_ERROR("GrpcApiServer::start – no gRPC services registered");
        return false;
    }

    // Capture config values while still locked.
    const std::string address    = server_address_;
    const auto        services   = services_;       // copy the registered service list
    const auto        msg_size   = config_.max_message_size_bytes;

    // Build credentials while still locked (reads config_, no network I/O).
    std::shared_ptr<grpc::ServerCredentials> credentials;
    try {
        credentials = buildCredentials();
    } catch (const std::exception& ex) {
        THEMIS_ERROR(std::string("GrpcApiServer::start – credential build failed: ") + ex.what());
        return false;
    }

    // ── Release the lock before the blocking socket bind ──────────────────
    lock.unlock();

    try {
        grpc::ServerBuilder builder;

        builder.AddListeningPort(address, credentials);
        builder.SetMaxReceiveMessageSize(msg_size);
        builder.SetMaxSendMessageSize(msg_size);

        for (auto* svc : services) {
            builder.RegisterService(svc);
        }

        // Register gRPC reflection in debug builds only.
        // Reflection exposes the full proto schema to any gRPC client, which
        // is useful during development but must not be available in production.
#if !defined(NDEBUG) && !defined(THEMIS_TEST_BUILD)
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        THEMIS_INFO("GrpcApiServer: gRPC reflection enabled (debug build)");
#endif

        auto server = builder.BuildAndStart();
        if (!server) {
            THEMIS_ERROR("GrpcApiServer::start – BuildAndStart() returned nullptr for " + address);
            return false;
        }

        // Re-acquire lock to update shared state.
        lock.lock();
        server_  = std::move(server);
        running_ = true;
        lock.unlock();

        THEMIS_INFO("GrpcApiServer listening on " + address);
        return true;

    } catch (const std::exception& ex) {
        THEMIS_ERROR(std::string("GrpcApiServer::start exception: ") + ex.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// stop()
// ---------------------------------------------------------------------------

void GrpcApiServer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_ || !server_) {
        return;
    }

    THEMIS_INFO("GrpcApiServer: shutting down " + server_address_);

    // Apply a 30-second hard deadline so stop() never blocks indefinitely
    // when a misbehaving RPC handler stalls (ROADMAP Phase 4).
    const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(30);
    server_->Shutdown(deadline);
    server_.reset();
    running_ = false;
    THEMIS_INFO("GrpcApiServer stopped");
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

bool GrpcApiServer::isRunning() const {
    return running_;
}

std::string GrpcApiServer::getAddress() const {
    return server_address_;
}

uint16_t GrpcApiServer::getPort() const {
    return server_address_.empty() ? 0 : config_.port;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string GrpcApiServer::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("GrpcApiServer: cannot open file: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::shared_ptr<grpc::ServerCredentials> GrpcApiServer::buildCredentials() const {
    if (!config_.tls_enabled) {
        THEMIS_INFO("GrpcApiServer: using insecure credentials (TLS disabled)");
        return grpc::InsecureServerCredentials();
    }

    try {
        const std::string cert = loadFile(config_.tls_cert_path);
        const std::string key  = loadFile(config_.tls_key_path);

        grpc::SslServerCredentialsOptions ssl_opts;

        if (config_.require_client_cert && !config_.tls_ca_cert_path.empty()) {
            ssl_opts.pem_root_certs = loadFile(config_.tls_ca_cert_path);
            ssl_opts.client_certificate_request =
                GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
            THEMIS_INFO("GrpcApiServer: mutual TLS (mTLS) enabled");
        } else {
            ssl_opts.client_certificate_request =
                GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
            THEMIS_INFO("GrpcApiServer: server-side TLS enabled");
        }

        grpc::SslServerCredentialsOptions::PemKeyCertPair kp;
        kp.private_key = key;
        kp.cert_chain  = cert;
        ssl_opts.pem_key_cert_pairs.push_back(std::move(kp));

        return grpc::SslServerCredentials(ssl_opts);

    } catch (const std::exception& ex) {
        // Fail-closed: do not fall back to insecure mode when TLS is
        // explicitly requested (FUTURE_ENHANCEMENTS.md – Design Constraints).
        THEMIS_ERROR(std::string("GrpcApiServer: TLS configuration failed – ") + ex.what());
        throw std::runtime_error(
            std::string("GrpcApiServer: TLS configuration failed: ") + ex.what());
    }
}

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_GRPC
