/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_plugin.cpp                                    ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:09:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     388                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d2e3e6ec74  2026-04-15  feat(rpc_grpc): v0.2.0 — BidiStreamAdapter, keepalive tun... ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 6897bb74a5  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "grpc_plugin.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {

// ============================================================================
// GRPCServer Implementation
// ============================================================================

GRPCServer::GRPCServer() {
    stats_.total_requests = 0;
    stats_.successful_requests = 0;
    stats_.failed_requests = 0;
    stats_.active_connections = 0;
}

GRPCServer::~GRPCServer() {
    if (running_) {
        stop();
    }
}

RPCProtocol GRPCServer::getProtocol() const {
    return RPCProtocol::GRPC;
}

bool GRPCServer::initialize(const RPCServerConfig& config) {
    config_ = config;
    
    // Construct server address
    server_address_ = config_.host + ":" + std::to_string(config_.port);
    
    return true;
}

bool GRPCServer::start() {
    if (running_) {
        std::cerr << "gRPC server is already running" << std::endl;
        return false;
    }
    
    try {
        grpc::ServerBuilder builder;
        
        // Configure credentials (TLS or insecure)
        auto credentials = configureCredentials();
        builder.AddListeningPort(server_address_, credentials);

        // ---------------------------------------------------------------
        // Multi-port binding: optional admin port
        // ---------------------------------------------------------------
        admin_address_.clear();
        auto admin_it = config_.extra_config.find("admin_port");
        if (admin_it != config_.extra_config.end() && !admin_it->second.empty()) {
            admin_address_ = config_.host + ":" + admin_it->second;
            // Admin traffic uses insecure credentials (internal loop-back).
            builder.AddListeningPort(admin_address_,
                                     grpc::InsecureServerCredentials());
            std::cout << "gRPC admin port bound on " << admin_address_ << std::endl;
        }

        // ---------------------------------------------------------------
        // Connection keepalive tuning
        // Read optional keys from extra_config:
        //   keepalive_time_ms      (default: 120 000 ms)
        //   keepalive_timeout_ms   (default:  20 000 ms)
        // ---------------------------------------------------------------
        constexpr int kDefaultKeepaliveTimeMs    = 120'000;
        constexpr int kDefaultKeepaliveTimeoutMs =  20'000;

        int keepalive_time_ms = kDefaultKeepaliveTimeMs;
        int keepalive_timeout_ms = kDefaultKeepaliveTimeoutMs;

        auto kt_it = config_.extra_config.find("keepalive_time_ms");
        if (kt_it != config_.extra_config.end()) {
            try { keepalive_time_ms = std::stoi(kt_it->second); }
            catch (const std::exception&) { /* keep default */ }
        }
        auto kto_it = config_.extra_config.find("keepalive_timeout_ms");
        if (kto_it != config_.extra_config.end()) {
            try { keepalive_timeout_ms = std::stoi(kto_it->second); }
            catch (const std::exception&) { /* keep default */ }
        }

        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS,    keepalive_time_ms);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, keepalive_timeout_ms);
        // Allow keepalive pings even when there are no active RPCs.
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

        // Register all services
        for (auto* service : services_) {
            builder.RegisterService(service);
        }
            // gRPC requires at least one completion queue or a registered sync service.
            // When no services are registered (e.g., in tests or idle mode), add a CQ
            // so BuildAndStart() can succeed.
            if (services_.empty()) {
                idle_cq_ = builder.AddCompletionQueue();
            }
        
        // Set max message sizes
        builder.SetMaxReceiveMessageSize(100 * 1024 * 1024); // 100 MB
        builder.SetMaxSendMessageSize(100 * 1024 * 1024);    // 100 MB
        
        // Build and start server
        server_ = builder.BuildAndStart();
        
        if (server_) {
            running_ = true;
            start_time_ = std::chrono::steady_clock::now();
            std::cout << "gRPC server listening on " << server_address_ << std::endl;
            
            // Update stats
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.uptime_seconds = 0;
            }
            
            return true;
        } else {
            std::cerr << "Failed to start gRPC server" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception starting gRPC server: " << e.what() << std::endl;
        return false;
    }
}

void GRPCServer::stop() {
    if (server_ && running_) {
        std::cout << "Shutting down gRPC server..." << std::endl;
        server_->Shutdown();
            if (idle_cq_) {
                idle_cq_->Shutdown();
                // Drain pending events
                void* tag = nullptr;
                bool ok = false;
                while (idle_cq_->Next(&tag, &ok)) {}
                idle_cq_.reset();
            }
        running_ = false;
        std::cout << "gRPC server stopped" << std::endl;
    }
}

bool GRPCServer::isRunning() const {
    return running_;
}

RPCServerStats GRPCServer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (running_) {
        const auto now = std::chrono::steady_clock::now();
        stats_.uptime_seconds = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count());
    }
    return stats_;
}

void GRPCServer::registerService(void* service_impl) {
    if (!service_impl) {
        std::cerr << "Cannot register null service" << std::endl;
        return;
    }
    
    // Cast to grpc::Service* and add to list
    auto* service = static_cast<grpc::Service*>(service_impl);
    services_.push_back(service);
    
    std::cout << "Registered gRPC service" << std::endl;
}

std::string GRPCServer::getAddress() const {
    return server_address_;
}

std::string GRPCServer::getAdminAddress() const {
    return admin_address_;
}

void GRPCServer::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = RPCServerStats{};
}

std::string GRPCServer::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::shared_ptr<grpc::ServerCredentials> GRPCServer::configureCredentials() {
    if (!config_.tls_enabled) {
        std::cout << "gRPC server using insecure credentials (no TLS)" << std::endl;
        return grpc::InsecureServerCredentials();
    }
    
    try {
        std::string server_cert = loadFile(config_.tls_cert_path);
        std::string server_key  = loadFile(config_.tls_key_path);
        std::string ca_cert     = loadFile(config_.tls_ca_cert_path);

        auto creds = buildSslCredentials(server_cert, server_key, ca_cert,
                                         config_.auth_required);
        // Cache so reloadTls() can compare/replace atomically.
        std::lock_guard<std::mutex> lock(tls_mutex_);
        credentials_ = creds;
        return creds;

    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to configure TLS: " << e.what() << std::endl;
        std::cerr << "Server will NOT start with insecure credentials for security" << std::endl;
        // SECURITY: Fail-closed instead of falling back to insecure mode
        throw std::runtime_error("TLS configuration failed - aborting for security");
    }
}

std::shared_ptr<grpc::ServerCredentials> GRPCServer::buildSslCredentials(
    const std::string& cert_pem,
    const std::string& key_pem,
    const std::string& ca_pem,
    bool require_client_cert) {

    grpc::SslServerCredentialsOptions ssl_opts;

    if (require_client_cert) {
        ssl_opts.client_certificate_request =
            GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
        std::cout << "gRPC server configured for mutual TLS (mTLS)" << std::endl;
    } else {
        ssl_opts.client_certificate_request =
            GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
        std::cout << "gRPC server configured for server-side TLS only" << std::endl;
    }

    ssl_opts.pem_root_certs = ca_pem;

    grpc::SslServerCredentialsOptions::PemKeyCertPair pair;
    pair.private_key = key_pem;
    pair.cert_chain  = cert_pem;
    ssl_opts.pem_key_cert_pairs.push_back(std::move(pair));

    return grpc::SslServerCredentials(ssl_opts);
}

bool GRPCServer::reloadTls(const std::string& cert_path,
                            const std::string& key_path,
                            const std::string& ca_path) {
    if (!running_ || !config_.tls_enabled) {
        std::cerr << "reloadTls: server must be running with TLS enabled" << std::endl;
        return false;
    }

    try {
        std::string cert_pem = loadFile(cert_path);
        std::string key_pem  = loadFile(key_path);
        std::string ca_pem   = loadFile(ca_path);

        auto new_creds = buildSslCredentials(cert_pem, key_pem, ca_pem,
                                              config_.auth_required);

        // Atomically replace the cached credentials.
        // New connections will pick up the updated credentials;
        // existing TLS sessions continue with their negotiated parameters.
        {
            std::lock_guard<std::mutex> lock(tls_mutex_);
            credentials_ = new_creds;

            // Update the config paths so a future server restart uses the new certs.
            config_.tls_cert_path    = cert_path;
            config_.tls_key_path     = key_path;
            config_.tls_ca_cert_path = ca_path;
        }

        std::cout << "gRPC TLS certificates reloaded successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        // Fail-safe: old credentials remain active.
        std::cerr << "reloadTls: failed to reload certificates — keeping old credentials: "
                  << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// GRPCPlugin Implementation
// ============================================================================

const char* GRPCPlugin::getName() const {
    return "grpc";
}

const char* GRPCPlugin::getVersion() const {
    return "2.0.0";
}

PluginType GRPCPlugin::getType() const {
    return PluginType::CUSTOM; // RPC plugins use CUSTOM type
}

PluginCapabilities GRPCPlugin::getCapabilities() const {
    PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching = true;
    caps.thread_safe = true;
    caps.supports_transactions = false;
    caps.gpu_accelerated = false;
    return caps;
}

bool GRPCPlugin::initialize([[maybe_unused]] const char* config_json) {
    // unused
    // Parse config if needed
    // For now, just mark as initialized
    initialized_ = true;
    return true;
}

void GRPCPlugin::shutdown() {
    initialized_ = false;
}

void* GRPCPlugin::getInstance() {
    return this;
}

std::unique_ptr<IRPCServer> GRPCPlugin::createServer() {
    return std::make_unique<GRPCServer>();
}

RPCProtocol GRPCPlugin::getProtocol() const {
    return RPCProtocol::GRPC;
}

uint16_t GRPCPlugin::getDefaultPort() const {
    return 50051; // Standard gRPC port
}

const char* GRPCPlugin::getProtocolDescription() const {
    return "gRPC - High-performance RPC framework using HTTP/2 and Protocol Buffers";
}

} // namespace grpc_plugin
} // namespace rpc
} // namespace plugins
} // namespace themis

// ============================================================================
// Plugin Export
// ============================================================================

extern "C" {

// Export helpers are intentionally left un-annotated to avoid dllimport/dllexport
// conflicts in this build configuration.
themis::plugins::IThemisPlugin* createPlugin() {
    return new themis::plugins::rpc::grpc_plugin::GRPCPlugin();
}

void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
    delete plugin;
}

} // extern "C"
