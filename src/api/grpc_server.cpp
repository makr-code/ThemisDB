/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_server.cpp                                    ║
  Module:          api                                                ║
  Description:     gRPC API server alongside REST                     ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifdef THEMIS_ENABLE_GRPC

#include "api/grpc_server.h"
#include "utils/logger.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// gRPC reflection is only compiled in debug builds to prevent schema leakage
// in production (FUTURE_ENHANCEMENTS.md – Security / Reliability).
#ifndef NDEBUG
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
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        THEMIS_WARN("GrpcApiServer::start – server is already running on " + server_address_);
        return false;
    }

    if (server_address_.empty()) {
        THEMIS_ERROR("GrpcApiServer::start – call initialize() first");
        return false;
    }

    try {
        grpc::ServerBuilder builder;

        // Listening address and credentials (TLS or insecure)
        auto credentials = buildCredentials();
        builder.AddListeningPort(server_address_, credentials);

        // Message size limits
        builder.SetMaxReceiveMessageSize(config_.max_message_size_bytes);
        builder.SetMaxSendMessageSize(config_.max_message_size_bytes);

        // Register application services
        for (auto* svc : services_) {
            builder.RegisterService(svc);
        }

        // Register gRPC reflection in debug builds only.
        // Reflection exposes the full proto schema to any gRPC client, which
        // is useful during development but must not be available in production.
#ifndef NDEBUG
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        THEMIS_INFO("GrpcApiServer: gRPC reflection enabled (debug build)");
#endif

        server_ = builder.BuildAndStart();

        if (!server_) {
            THEMIS_ERROR("GrpcApiServer::start – BuildAndStart() returned nullptr for " + server_address_);
            return false;
        }

        running_ = true;
        THEMIS_INFO("GrpcApiServer listening on " + server_address_);
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
    server_->Shutdown();
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
    return config_.port;
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
