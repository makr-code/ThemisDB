/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_plugin.h                                      ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:43:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d2e3e6ec74  2026-04-15  feat(rpc_grpc): v0.2.0 — BidiStreamAdapter, keepalive tun... ║
    • 6897bb74a5  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/rpc_plugin_interface.h"
#include <grpcpp/grpcpp.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/**
 * @file grpc_plugin.h
 * @brief gRPC Plugin Implementation for ThemisDB
 * 
 * This plugin provides gRPC server functionality for ThemisDB,
 * enabling high-performance RPC communication with:
 * - HTTP/2 multiplexing
 * - Protocol Buffers serialization
 * - Native mTLS support
 * - Bidirectional streaming
 * 
 * Part of ThemisDB v1.3.0
 */

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {

/**
 * @brief gRPC Server Implementation
 */
class GRPCServer : public IRPCServer {
public:
    GRPCServer();
    ~GRPCServer() override;
    
    // IRPCServer interface
    RPCProtocol getProtocol() const override;
    bool initialize(const RPCServerConfig& config) override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    RPCServerStats getStats() const override;
    void registerService(void* service_impl) override;
    std::string getAddress() const override;
    void resetStats() override;

    // -----------------------------------------------------------------------
    // v0.2.0 extensions
    // -----------------------------------------------------------------------

    /**
     * @brief Reload TLS certificates without restarting the server.
     *
     * Validates the new certificate files using the same fail-closed logic as
     * `start()`.  On success, the updated `SslServerCredentials` is stored and
     * will be used for all subsequent new connections.  Existing TLS sessions
     * continue with their negotiated parameters.
     *
     * If the new certificate files are invalid the old credentials remain
     * active (fail-safe) and `false` is returned.
     *
     * Only valid while the server is running with TLS enabled.
     *
     * @param cert_path  Path to the new server certificate PEM file.
     * @param key_path   Path to the new server private-key PEM file.
     * @param ca_path    Path to the new CA certificate PEM file.
     * @return `true` if the reload succeeded; `false` otherwise.
     */
    bool reloadTls(const std::string& cert_path,
                   const std::string& key_path,
                   const std::string& ca_path);

    /**
     * @brief Return the admin server address if multi-port binding is active.
     *
     * If `extra_config["admin_port"]` was set before `start()`, returns
     * `"<host>:<admin_port>"`.  Returns an empty string when no admin port
     * is bound.
     */
    std::string getAdminAddress() const;

private:
    RPCServerConfig config_;
    std::unique_ptr<grpc::Server> server_;
    std::atomic<bool> running_{false};
    mutable std::mutex stats_mutex_;
    RPCServerStats stats_;
    std::string server_address_;
    std::string admin_address_;   ///< Non-empty when admin port is bound.
    std::chrono::steady_clock::time_point start_time_;

    // Current TLS credentials (updated by reloadTls()).
    mutable std::mutex tls_mutex_;
    std::shared_ptr<grpc::ServerCredentials> credentials_;

    // Service implementations registered with this server
    std::vector<grpc::Service*> services_;

    /**
     * @brief Load file contents (for certificates)
     */
    std::string loadFile(const std::string& path);

    /**
     * @brief Configure SSL/TLS credentials
     */
    std::shared_ptr<grpc::ServerCredentials> configureCredentials();

    /**
     * @brief Build SSL credentials from explicit PEM strings.
     *
     * Factored out so it can be called by both `configureCredentials()` and
     * `reloadTls()`.
     */
    std::shared_ptr<grpc::ServerCredentials> buildSslCredentials(
        const std::string& cert_pem,
        const std::string& key_pem,
        const std::string& ca_pem,
        bool require_client_cert);
};

/**
 * @brief gRPC Plugin
 */
class GRPCPlugin : public IRPCPlugin {
public:
    GRPCPlugin() = default;
    ~GRPCPlugin() override = default;
    
    // IThemisPlugin interface
    const char* getName() const override;
    const char* getVersion() const override;
    PluginType getType() const override;
    PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override;
    
    // IRPCPlugin interface
    std::unique_ptr<IRPCServer> createServer() override;
    RPCProtocol getProtocol() const override;
    uint16_t getDefaultPort() const override;
    const char* getProtocolDescription() const override;
    
private:
    bool initialized_{false};
};

} // namespace grpc_plugin
} // namespace rpc
} // namespace plugins
} // namespace themis
