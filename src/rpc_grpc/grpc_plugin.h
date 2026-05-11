/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_plugin.h                                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:50:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     179                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d2e3e6ec74  2026-04-15  feat(rpc_grpc): v0.2.0 — BidiStreamAdapter, keepalive tun... ║
    • 6897bb74a5  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/rpc_plugin_interface.h"
#include <grpcpp/grpcpp.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
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

    // -----------------------------------------------------------------------
    // v0.3.0 — Health & Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Set the health state for a named gRPC service.
     *
     * The string `""` denotes the overall server health (default service).
     * Called automatically: `SERVING` on `start()`, `NOT_SERVING` on `stop()`.
     *
     * @param service_name  Service name as registered, or `""` for global.
     * @param serving       `true` = SERVING, `false` = NOT_SERVING.
     */
    void setServiceHealth(const std::string& service_name, bool serving);

    /**
     * @brief Return the current health state for a named service.
     *
     * Returns `true` (SERVING) if the service is healthy or not yet tracked.
     * Returns `false` (NOT_SERVING) when explicitly set via `setServiceHealth`.
     */
    bool isServiceHealthy(const std::string& service_name) const;

    /**
     * @brief Record a completed RPC call (interceptor hook).
     *
     * Thread-safe.  Increments per-method counters used by `getMetricsText()`.
     *
     * @param method       Full RPC method name, e.g. `"/helloworld.Greeter/SayHello"`.
     * @param success      `true` = OK status; `false` = any non-OK status.
     * @param duration_ms  Wall-clock duration of the call.
     */
    void recordRPC(const std::string& method, bool success, uint64_t duration_ms);

    /**
     * @brief Export all collected metrics in Prometheus text format (v0.0.4).
     *
     * Emitted metric families:
     *   - `grpc_server_requests_total{method}` counter
     *   - `grpc_server_errors_total{method}` counter
     *   - `grpc_server_latency_ms_total{method}` counter (use for avg: latency_ms/requests)
     *   - `grpc_server_active_connections` gauge (from `stats_.active_connections`)
     *
     * Returns an empty string if no metrics have been recorded yet.
     */
    std::string getMetricsText() const;

    /**
     * @brief Register a sink for structured JSON access log entries.
     *
     * Each entry is a single JSON object with fields:
     *   `timestamp_ms`, `method`, `status_code`, `duration_ms`, `client_cn`.
     *
     * Call `logAccess()` to emit one entry.  This is called automatically
     * within `recordRPC()` when a sink is registered.
     *
     * @param sink  Callable that receives one JSON log line per RPC.
     *              Pass an empty function to disable logging.
     */
    void setAccessLogSink(std::function<void(const std::string&)> sink);

    /**
     * @brief Emit one structured JSON access-log entry.
     *
     * Uses the registered sink (no-op if none is set).
     *
     * @param method       Full RPC method name.
     * @param status_code  gRPC status code integer (0 = OK).
     * @param duration_ms  Call duration in milliseconds.
     * @param client_cn    Client certificate CN (empty string if not applicable).
     */
    void logAccess(const std::string& method, int status_code,
                   uint64_t duration_ms,
                   const std::string& client_cn = "");

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
    // Idle completion queue – created when no services are registered so that
    // BuildAndStart() can succeed (drained on stop)
    std::unique_ptr<grpc::ServerCompletionQueue> idle_cq_;

    // -----------------------------------------------------------------------
    // v0.3.0 — health state, interceptor metrics, access log
    // -----------------------------------------------------------------------

    /// Per-method request/error/latency accumulators.
    struct MethodMetrics {
        std::atomic<uint64_t> requests{0};
        std::atomic<uint64_t> errors{0};
        std::atomic<uint64_t> latency_ms{0};

        MethodMetrics() = default;
        // Atomics are not copyable; provide explicit move-only ctors needed
        // by std::unordered_map emplace.
        MethodMetrics(const MethodMetrics&) = delete;
        MethodMetrics& operator=(const MethodMetrics&) = delete;
    };

    mutable std::mutex metrics_mutex_;
    /// Key = method name (e.g. "/pkg.Svc/Method").
    std::unordered_map<std::string, std::unique_ptr<MethodMetrics>> method_metrics_;

    mutable std::mutex health_mutex_;
    /// Key = service name ("" = global).  Value = true → SERVING.
    std::map<std::string, bool> health_states_;

    mutable std::mutex log_sink_mutex_;
    std::function<void(const std::string&)> access_log_sink_;

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

    /// Return or create the MethodMetrics for @p method (must hold metrics_mutex_).
    MethodMetrics& methodMetricsLocked(const std::string& method);
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

