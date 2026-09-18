/**
 * @file grpc_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "plugins/rpc_plugin_interface.h"

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {

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

    /**
     * @brief ----------------------------------------------------------------------- v0.
     * @param[in] cert_path Path to the cert.
     * @param[in] key_path Path to the key.
     * @param[in] ca_path Path to the ca.
     * @return True when the operation succeeds.
     * @details 2.0 extensions -----------------------------------------------------------------------
     */

    bool reloadTls(const std::string& cert_path,
                   const std::string& key_path,
                   const std::string& ca_path);

    /**
     * @brief Get Admin Address.
     * @return Return value.
     */
    std::string getAdminAddress() const;

    /**
     * @brief ----------------------------------------------------------------------- v0.
     * @param[in] service_name Name of the service.
     * @param[in] serving Input parameter.
     * @details 3.0 — Health & Observability -----------------------------------------------------------------------
     */

    void setServiceHealth(const std::string& service_name, bool serving);

    /**
     * @brief Is Service Healthy.
     * @param[in] service_name Name of the service.
     * @return True when the operation succeeds.
     */
    bool isServiceHealthy(const std::string& service_name) const;

    /**
     * @brief Record RPC.
     * @param[in] method Input parameter.
     * @param[in] success Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordRPC(const std::string& method, bool success, uint64_t duration_ms);

    /**
     * @brief Get Metrics Text.
     * @return Return value.
     */
    std::string getMetricsText() const;

    void setAccessLogSink(std::function<void(const std::string&)> sink);

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
    std::unordered_map<std::string, std::unique_ptr<MethodMetrics>> method_metrics_;

    mutable std::mutex health_mutex_;
    std::map<std::string, bool> health_states_;

    mutable std::mutex log_sink_mutex_;
    std::function<void(const std::string&)> access_log_sink_;

    /**
     * @brief Load File.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::string loadFile(const std::string& path);

    /**
     * @brief Configure Credentials.
     * @return Return value.
     */
    std::shared_ptr<grpc::ServerCredentials> configureCredentials();

    /**
     * @brief Build Ssl Credentials.
     * @param[in] cert_pem Input parameter.
     * @param[in] key_pem Input parameter.
     * @param[in] ca_pem Input parameter.
     * @param[in] require_client_cert Input parameter.
     * @return Return value.
     */
    std::shared_ptr<grpc::ServerCredentials> buildSslCredentials(
        const std::string& cert_pem,
        const std::string& key_pem,
        const std::string& ca_pem,
        bool require_client_cert);

    /**
     * @brief Method Metrics Locked.
     * @param[in] method Input parameter.
     * @return Return value.
     */
    MethodMetrics& methodMetricsLocked(const std::string& method);
};

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

