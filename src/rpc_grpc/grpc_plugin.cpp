/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_plugin.cpp                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-30 04:19:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     286                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c508  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250db  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
    RPCServerStats result = stats_;
    if (running_) {
        auto elapsed = std::chrono::steady_clock::now() - start_time_;
        result.uptime_seconds = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
    }
    return result;
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
        // Load certificates
        std::string server_cert = loadFile(config_.tls_cert_path);
        std::string server_key = loadFile(config_.tls_key_path);
        std::string ca_cert = loadFile(config_.tls_ca_cert_path);
        
        // Configure SSL options
        grpc::SslServerCredentialsOptions ssl_opts;
        
        // For mutual TLS, require client certificates
        if (config_.auth_required) {
            ssl_opts.client_certificate_request = 
                GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
            std::cout << "gRPC server configured for mutual TLS (mTLS)" << std::endl;
        } else {
            ssl_opts.client_certificate_request = 
                GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
            std::cout << "gRPC server configured for server-side TLS only" << std::endl;
        }
        
        // Set root CA for verifying client certificates
        ssl_opts.pem_root_certs = ca_cert;
        
        // Set server certificate and private key
        grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
        key_cert_pair.private_key = server_key;
        key_cert_pair.cert_chain = server_cert;
        ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
        
        return grpc::SslServerCredentials(ssl_opts);
        
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to configure TLS: " << e.what() << std::endl;
        std::cerr << "Server will NOT start with insecure credentials for security" << std::endl;
        // SECURITY: Fail-closed instead of falling back to insecure mode
        throw std::runtime_error("TLS configuration failed - aborting for security");
    }
}

// ============================================================================
// GRPCPlugin Implementation
// ============================================================================

const char* GRPCPlugin::getName() const {
    return "grpc";
}

const char* GRPCPlugin::getVersion() const {
    return "1.0.0";
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

bool GRPCPlugin::initialize(const char* config_json) {
    (void)config_json; // unused
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
