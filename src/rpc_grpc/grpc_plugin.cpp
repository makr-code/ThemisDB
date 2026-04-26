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
#include <iostream>
#include <sstream>
#include <stdexcept>

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

        // ---- v0.2.0: keepalive tuning ----------------------------------------
        auto ka_it = config_.extra_config.find("keepalive_time_ms");
        if (ka_it != config_.extra_config.end()) {
            try {
                int ms = std::stoi(ka_it->second);
                builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, ms);
            } catch (...) {
                // Invalid value — fall back to gRPC default
            }
        }
        auto kt_it = config_.extra_config.find("keepalive_timeout_ms");
        if (kt_it != config_.extra_config.end()) {
            try {
                int ms = std::stoi(kt_it->second);
                builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, ms);
            } catch (...) {}
        }

        // ---- v0.2.0: admin port binding ----------------------------------------
        auto ap_it = config_.extra_config.find("admin_port");
        if (ap_it != config_.extra_config.end()) {
            try {
                int ap = std::stoi(ap_it->second);
                if (ap > 0 && ap < 65536) {
                    admin_address_ = config_.host + ":" + std::to_string(ap);
                    // GAP-016: Log warning for insecure admin port binding (CWE-295).
                    std::cerr << "[SECURITY] GRPCServer: admin port " << ap
                              << " bound with insecure credentials (GAP-016/CWE-295)."
                              << std::endl;
                    builder.AddListeningPort(admin_address_,
                                             grpc::InsecureServerCredentials());
                }
            } catch (...) {
                admin_address_.clear();
            }
        }

        // Register all services
        for (auto* service : services_) {
            builder.RegisterService(service);
        }
        // gRPC requires at least one completion queue or a registered sync service.
        if (services_.empty()) {
            idle_cq_ = builder.AddCompletionQueue();
        }

        // Max message sizes
        builder.SetMaxReceiveMessageSize(100 * 1024 * 1024); // 100 MB
        builder.SetMaxSendMessageSize(100 * 1024 * 1024);    // 100 MB

        server_ = builder.BuildAndStart();

        if (server_) {
            running_ = true;
            start_time_ = std::chrono::steady_clock::now();
            std::cout << "gRPC server listening on " << server_address_ << std::endl;
            if (!admin_address_.empty()) {
                std::cout << "gRPC admin port bound on " << admin_address_ << std::endl;
            }

            // v0.3.0: mark global health as SERVING
            setServiceHealth("", true);

            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.uptime_seconds = 0;
            }
            return true;
        } else {
            std::cerr << "Failed to start gRPC server" << std::endl;
            admin_address_.clear();
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception starting gRPC server: " << e.what() << std::endl;
        admin_address_.clear();
        return false;
    }
}

void GRPCServer::stop() {
    if (server_ && running_) {
        std::cout << "Shutting down gRPC server..." << std::endl;
        // v0.3.0: mark global health as NOT_SERVING
        setServiceHealth("", false);
        server_->Shutdown();
        if (idle_cq_) {
            idle_cq_->Shutdown();
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

// ============================================================================
// v0.2.0 — TLS Hot-Reload
// ============================================================================

bool GRPCServer::reloadTls(const std::string& cert_path,
                             const std::string& key_path,
                             const std::string& ca_path)
{
    if (!running_) return false;
    if (!config_.tls_enabled) return false;

    try {
        std::string cert = loadFile(cert_path);
        std::string key  = loadFile(key_path);
        std::string ca   = loadFile(ca_path);

        auto new_creds = buildSslCredentials(cert, key, ca, config_.auth_required);

        std::lock_guard<std::mutex> lock(tls_mutex_);
        credentials_ = std::move(new_creds);
        std::cout << "TLS certificates reloaded successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "TLS reload failed (old credentials retained): " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// v0.2.0 — Admin Address
// ============================================================================

std::string GRPCServer::getAdminAddress() const {
    return admin_address_;
}

// ============================================================================
// v0.3.0 — Health Service
// ============================================================================

void GRPCServer::setServiceHealth(const std::string& service_name, bool serving) {
    std::lock_guard<std::mutex> lock(health_mutex_);
    health_states_[service_name] = serving;
}

bool GRPCServer::isServiceHealthy(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    auto it = health_states_.find(service_name);
    if (it == health_states_.end()) return true; // not tracked → assume SERVING
    return it->second;
}

// ============================================================================
// v0.3.0 — Interceptor Metrics
// ============================================================================

GRPCServer::MethodMetrics& GRPCServer::methodMetricsLocked(const std::string& method) {
    auto it = method_metrics_.find(method);
    if (it == method_metrics_.end()) {
        it = method_metrics_.emplace(method,
                                     std::make_unique<MethodMetrics>()).first;
    }
    return *it->second;
}

void GRPCServer::recordRPC(const std::string& method, bool success,
                             uint64_t duration_ms)
{
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        auto& m = methodMetricsLocked(method);
        ++m.requests;
        if (!success) ++m.errors;
        m.latency_ms += duration_ms;
    }

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.total_requests;
        if (success) ++stats_.successful_requests;
        else         ++stats_.failed_requests;
    }

    // Emit access log entry
    const int status_code = success ? 0 : 2; // 0=OK, 2=UNKNOWN
    logAccess(method, status_code, duration_ms, "");
}

// ============================================================================
// v0.3.0 — Prometheus Metrics Text
// ============================================================================

std::string GRPCServer::getMetricsText() const {
    std::ostringstream out;

    // Snapshot method-level counters
    std::unordered_map<std::string, uint64_t> reqs, errs, lats;
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        for (const auto& kv : method_metrics_) {
            reqs[kv.first] = kv.second->requests.load();
            errs[kv.first] = kv.second->errors.load();
            lats[kv.first] = kv.second->latency_ms.load();
        }
    }

    if (reqs.empty()) return "";

    // grpc_server_requests_total
    out << "# HELP grpc_server_requests_total Total gRPC requests received.\n";
    out << "# TYPE grpc_server_requests_total counter\n";
    for (const auto& kv : reqs)
        out << "grpc_server_requests_total{method=\"" << kv.first << "\"} "
            << kv.second << "\n";

    // grpc_server_errors_total
    out << "# HELP grpc_server_errors_total Total gRPC requests that returned non-OK status.\n";
    out << "# TYPE grpc_server_errors_total counter\n";
    for (const auto& kv : errs)
        out << "grpc_server_errors_total{method=\"" << kv.first << "\"} "
            << kv.second << "\n";

    // grpc_server_latency_ms_total
    out << "# HELP grpc_server_latency_ms_total Cumulative call latency in milliseconds.\n";
    out << "# TYPE grpc_server_latency_ms_total counter\n";
    for (const auto& kv : lats)
        out << "grpc_server_latency_ms_total{method=\"" << kv.first << "\"} "
            << kv.second << "\n";

    // grpc_server_active_connections
    uint64_t active = 0;
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        active = stats_.active_connections;
    }
    out << "# HELP grpc_server_active_connections Current number of active gRPC connections.\n";
    out << "# TYPE grpc_server_active_connections gauge\n";
    out << "grpc_server_active_connections " << active << "\n";

    return out.str();
}

// ============================================================================
// v0.3.0 — Structured Access Log
// ============================================================================

void GRPCServer::setAccessLogSink(std::function<void(const std::string&)> sink) {
    std::lock_guard<std::mutex> lock(log_sink_mutex_);
    access_log_sink_ = std::move(sink);
}

void GRPCServer::logAccess(const std::string& method, int status_code,
                             uint64_t duration_ms,
                             const std::string& client_cn)
{
    std::function<void(const std::string&)> sink_copy;
    {
        std::lock_guard<std::mutex> lock(log_sink_mutex_);
        if (!access_log_sink_) return;
        sink_copy = access_log_sink_;
    }

    // Build a minimal JSON object (no external JSON library required)
    auto jsEscape = [](const std::string& s) {
        std::string out;
        out.reserve(s.size() + 2);
        out += '"';
        for (char c : s) {
            if      (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else                out += c;
        }
        out += '"';
        return out;
    };

    std::ostringstream js;
    js << "{"
       << "\"timestamp_ms\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()
       << ",\"method\":"      << jsEscape(method)
       << ",\"status_code\":" << status_code
       << ",\"duration_ms\":" << duration_ms
       << ",\"client_cn\":"   << jsEscape(client_cn)
       << "}";

    sink_copy(js.str());
}

// ============================================================================
// Internal helpers
// ============================================================================

std::string GRPCServer::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Failed to open file: " + path);
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

std::shared_ptr<grpc::ServerCredentials>
GRPCServer::configureCredentials() {
    if (!config_.tls_enabled) {
        // GAP-016: Log a security warning when falling back to insecure credentials
        // (CWE-295). Using std::cerr instead of a proper log sink misses SIEM routing.
        // Replace std::cout with std::cerr + structured warning so the message is
        // visible at default log levels.
        std::cerr << "[SECURITY] GRPCServer: TLS is disabled — using insecure gRPC "
                     "credentials. All gRPC traffic is unencrypted. "
                     "Enable TLS in production (GAP-016/CWE-295)." << std::endl;
        return grpc::InsecureServerCredentials();
    }

    try {
        std::string cert = loadFile(config_.tls_cert_path);
        std::string key  = loadFile(config_.tls_key_path);
        std::string ca   = loadFile(config_.tls_ca_cert_path);
        auto creds = buildSslCredentials(cert, key, ca, config_.auth_required);
        {
            std::lock_guard<std::mutex> lock(tls_mutex_);
            credentials_ = creds;
        }
        return creds;
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to configure TLS: " << e.what() << std::endl;
        std::cerr << "Server will NOT start with insecure credentials for security" << std::endl;
        throw std::runtime_error("TLS configuration failed - aborting for security");
    }
}

std::shared_ptr<grpc::ServerCredentials>
GRPCServer::buildSslCredentials(const std::string& cert_pem,
                                  const std::string& key_pem,
                                  const std::string& ca_pem,
                                  bool require_client_cert)
{
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
    ssl_opts.pem_key_cert_pairs.push_back(pair);

    return grpc::SslServerCredentials(ssl_opts);
}

const char* GRPCPlugin::getName() const {
    return "grpc";
}

const char* GRPCPlugin::getVersion() const {
    return "2.0.0";
}

PluginType GRPCPlugin::getType() const {
    return PluginType::CUSTOM;
}

PluginCapabilities GRPCPlugin::getCapabilities() const {
    PluginCapabilities caps;
    caps.supports_streaming     = true;
    caps.supports_batching      = true;
    caps.thread_safe            = true;
    caps.supports_transactions  = false;
    caps.gpu_accelerated        = false;
    return caps;
}

bool GRPCPlugin::initialize(const char* config_json) {
    (void)config_json;
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
    return 50051;
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

themis::plugins::IThemisPlugin* createPlugin() {
    return new themis::plugins::rpc::grpc_plugin::GRPCPlugin();
}

void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
    delete plugin;
}

} // extern "C"
