/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_server.cpp                                    ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:15:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     336                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ed2d46e8d1  2026-04-13  fix(api): complete gRPC stub wiring — bounds checks, stop... ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 97cd900111  2026-03-25  feat(api): gRPC Phase 4 – mutex fix, deadline, RPC stubs,... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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

// Optional YAML config support for loading grpc.max_message_size_mb from
// config/networking/grpc.yaml at initialize() time.
#if __has_include(<yaml-cpp/yaml.h>)
#  include <yaml-cpp/yaml.h>
#  define THEMIS_GRPC_HAS_YAML 1
#else
#  define THEMIS_GRPC_HAS_YAML 0
#endif

namespace {
/// Well-known path to the gRPC networking configuration file, relative to the
/// process working directory.  Exposed as a named constant so that
/// integration tests and deployment tooling can predict the location.
constexpr const char* kGrpcNetworkingConfigPath = "config/networking/grpc.yaml";

/// Maximum allowed value for grpc.max_message_size_mb in the config file.
/// Values above this are rejected to prevent integer overflow when multiplying
/// by 1024 * 1024 on a 32-bit signed int.
/// 2047 MB * 1024 * 1024 = 2,146,435,072 < INT_MAX (2,147,483,647).
/// 2048 MB * 1024 * 1024 = 2,147,483,648 > INT_MAX (overflow).
constexpr int kMaxMessageSizeMbLimit = 2047;
} // namespace

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

    // Try to load grpc.max_message_size_mb from config/networking/grpc.yaml so
    // that operators can tune the limit without recompiling.  The caller-
    // supplied struct value is used as a fallback when the file is absent or
    // the key is not present.
#if THEMIS_GRPC_HAS_YAML
    {
        try {
            YAML::Node node = YAML::LoadFile(kGrpcNetworkingConfigPath);
            if (node["grpc"] && node["grpc"]["max_message_size_mb"]) {
                const int mb = node["grpc"]["max_message_size_mb"].as<int>();
                if (mb > 0 && mb <= kMaxMessageSizeMbLimit) {
                    config_.max_message_size_bytes = mb * 1024 * 1024;
                    THEMIS_INFO("GrpcApiServer: max_message_size_bytes overridden to " +
                                std::to_string(config_.max_message_size_bytes) +
                                " bytes from " + kGrpcNetworkingConfigPath);
                } else if (mb > kMaxMessageSizeMbLimit) {
                    THEMIS_WARN("GrpcApiServer: grpc.max_message_size_mb=" +
                                std::to_string(mb) + " exceeds limit of " +
                                std::to_string(kMaxMessageSizeMbLimit) + " MB; ignoring");
                }
            }
        } catch (const std::exception& ex) {
            // Config file absent or unreadable – warn so that operators are
            // notified their configuration file is being ignored, then fall
            // back to the caller-supplied struct value.
            THEMIS_WARN(std::string("GrpcApiServer: could not load ") +
                        kGrpcNetworkingConfigPath + " (" + ex.what() +
                        "); using default max_message_size_bytes=" +
                        std::to_string(config_.max_message_size_bytes));
        }
    }
#endif

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
    std::unique_lock<std::mutex> lock(mutex_);

    if (!running_ || !server_) {
        return;
    }

    THEMIS_INFO("GrpcApiServer: shutting down " + server_address_);

    // Mark as stopped and extract the server handle while the lock is held so
    // that concurrent calls to isRunning() immediately see the stopped state.
    running_ = false;
    std::unique_ptr<grpc::Server> local_server = std::move(server_);

    // Release the mutex before calling Shutdown() so that other threads
    // invoking isRunning() or stop() are not deadlocked for the full
    // 30-second drain window (ROADMAP Phase 4 requirement).
    lock.unlock();

    // Apply a 30-second hard deadline so stop() never blocks indefinitely
    // when a misbehaving RPC handler stalls.
    const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(30);
    local_server->Shutdown(deadline);
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
