/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_plugin.h                                      ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:51:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6897bb74a5  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
    
private:
    RPCServerConfig config_;
    std::unique_ptr<grpc::Server> server_;
    std::atomic<bool> running_{false};
    mutable std::mutex stats_mutex_;
    RPCServerStats stats_;
    std::string server_address_;
    std::chrono::steady_clock::time_point start_time_;
    
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
