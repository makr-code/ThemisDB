/**
 * @file grpc_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "grpc_plugin.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {

namespace {

bool tryParseConfigInt(const std::string& value, int& parsed_value) noexcept {
    try {
        parsed_value = std::stoi(value);
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

} // namespace

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
    // Phase 3: Hardened lifecycle and fail-safe semantics
    if (running_) {
        std::cerr << "[RPC-E8300] Server start rejected: already running" << std::endl;
        return false;
    }

    try {
        grpc::ServerBuilder builder;

        // Configure credentials (TLS or insecure) — fail-closed on errors
        std::shared_ptr<grpc::ServerCredentials> credentials;
        try {
            credentials = configureCredentials();
        } catch (const std::exception& e) {
            std::cerr << "[RPC-E8302] Server start failed: credentials configuration error — "
                      << e.what() << std::endl;
            admin_address_.clear();
            return false;
        }

        builder.AddListeningPort(server_address_, credentials);

        // ---- v0.2.0: keepalive tuning ----------------------------------------
        auto ka_it = config_.extra_config.find("keepalive_time_ms");
        if (ka_it != config_.extra_config.end()) {
            int ms = 0;
            if (tryParseConfigInt(ka_it->second, ms)) {
                builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, ms);
            }
        }
        auto kt_it = config_.extra_config.find("keepalive_timeout_ms");
        if (kt_it != config_.extra_config.end()) {
            int ms = 0;
            if (tryParseConfigInt(kt_it->second, ms)) {
                builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, ms);
            }
        }

        // ---- v0.2.0: admin port binding ----------------------------------------
        auto ap_it = config_.extra_config.find("admin_port");
        if (ap_it != config_.extra_config.end()) {
            int admin_port = 0;
            if (tryParseConfigInt(ap_it->second, admin_port)
                && admin_port > 0 && admin_port < 65536) {
                admin_address_ = config_.host + ":" + std::to_string(admin_port);
                // GAP-016: Log warning for insecure admin port binding (CWE-295).
                std::cerr << "[RPC-W/GAP-016] GRPCServer: admin port " << admin_port
                          << " bound with insecure credentials (CWE-295)."
                          << std::endl;
                builder.AddListeningPort(admin_address_,
                                         grpc::InsecureServerCredentials());
            } else {
                admin_address_.clear();
            }
        }

        // Register all services
        if (services_.empty()) {
            std::cerr << "[RPC-W] Server start with no registered services" << std::endl;
        }
        for (auto* service : services_) {
            if (!service) {
                std::cerr << "[RPC-E8301] Server start rejected: null service pointer" << std::endl;
                admin_address_.clear();
                return false;
            }
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
            std::cerr << "[RPC-E8306] Server start failed: BuildAndStart() returned nullptr" << std::endl;
            admin_address_.clear();
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[RPC-E8306] Server start failed: unexpected exception — "
                  << e.what() << std::endl;
        admin_address_.clear();
        return false;
    }
}

void GRPCServer::stop() {
    // Phase 3: Deterministic shutdown with explicit diagnostics
    if (!server_ || !running_) {
        std::cerr << "[RPC-W] Server stop called but not running" << std::endl;
        return;
    }

    try {
        std::cout << "[RPC-I] Shutting down gRPC server..." << std::endl;
        
        // Mark all services as NOT_SERVING (graceful degradation)
        {
            std::lock_guard<std::mutex> lock(health_mutex_);
            for (auto& kv : health_states_) {
                kv.second = false;
            }
        }

        // Shutdown server (blocks until all in-flight RPCs complete or timeout)
        server_->Shutdown();

        // Drain idle completion queue if present
        if (idle_cq_) {
            idle_cq_->Shutdown();
            void* tag = nullptr;
            bool ok = false;
            while (idle_cq_->Next(&tag, &ok)) {
                // Drain all remaining messages
            }
            idle_cq_.reset();
        }

        running_ = false;
        std::cout << "[RPC-I] gRPC server stopped successfully" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[RPC-E8306] Server stop failed: " << e.what() << std::endl;
        running_ = false;
    }
}

void GRPCServer::registerService(void* service_impl) {
    // Phase 3: Bounded registration with explicit error handling
    if (!service_impl) {
        std::cerr << "[RPC-E8301] Service registration failed: null service pointer" << std::endl;
        return;
    }

    if (running_) {
        std::cerr << "[RPC-E8300] Service registration rejected: server already running "
                  << "(register services before start())" << std::endl;
        return;
    }

    // Idempotent check: verify same service not registered twice
    auto* service = static_cast<grpc::Service*>(service_impl);
    for (auto* existing : services_) {
        if (existing == service) {
            std::cerr << "[RPC-W] Service registration: service already registered (idempotent)"
                      << std::endl;
            return;
        }
    }

    services_.push_back(service);
    std::cout << "[RPC-I] Service registered (count=" << services_.size() << ")" << std::endl;
}

std::string GRPCServer::getAddress() const {
    return server_address_;
}

bool GRPCServer::isRunning() const {
    return running_.load(std::memory_order_acquire);
}

RPCServerStats GRPCServer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void GRPCServer::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = RPCServerStats{};
}

// ============================================================================
// v0.2.0 — TLS Hot-Reload (Phase 3: Fail-Safe, Deterministic Hardening)
// ============================================================================

bool GRPCServer::reloadTls(const std::string& cert_path,
                             const std::string& key_path,
                             const std::string& ca_path)
{
    // Phase 3: Standardized fail-safe behavior
    // ---- Precondition checks (fail-closed) ----
    if (!running_) {
        std::cerr << "[RPC-E8300] TLS reload rejected: server not running" << std::endl;
        return false;
    }
    if (!config_.tls_enabled) {
        std::cerr << "[RPC-E8302] TLS reload rejected: TLS not enabled in config" << std::endl;
        return false;
    }
    if (cert_path.empty() || key_path.empty() || ca_path.empty()) {
        std::cerr << "[RPC-E8302] TLS reload rejected: empty certificate paths" << std::endl;
        return false;
    }

    // ---- Atomic validation (old credentials remain active if any step fails) ----
    try {
        // Step 1: Load all files (fail-closed: no partial state)
        std::string cert, key, ca;
        try {
            cert = loadFile(cert_path);
            key  = loadFile(key_path);
            ca   = loadFile(ca_path);
        } catch (const std::exception& e) {
            std::cerr << "[RPC-E8302] TLS reload failed: file load error — "
                      << e.what() << " (old credentials retained)" << std::endl;
            return false;
        }

        // Step 2: Validate certificate contents (fail-closed)
        if (cert.empty() || key.empty()) {
            std::cerr << "[RPC-E8302] TLS reload failed: certificate or key is empty "
                      << "(old credentials retained)" << std::endl;
            return false;
        }

        // Step 3: Build new credentials (isolated operation)
        std::shared_ptr<grpc::ServerCredentials> new_creds;
        try {
            new_creds = buildSslCredentials(cert, key, ca, config_.auth_required);
        } catch (const std::exception& e) {
            std::cerr << "[RPC-E8302] TLS reload failed: credential build error — "
                      << e.what() << " (old credentials retained)" << std::endl;
            return false;
        }

        // Step 4: Atomic swap (only on success, old credentials remain if lock fails)
        {
            std::lock_guard<std::mutex> lock(tls_mutex_);
            credentials_ = std::move(new_creds);
        }

        std::cerr << "[RPC-I] TLS certificates reloaded successfully (cert_path="
                  << cert_path << ")" << std::endl;
        return true;

    } catch (const std::exception& e) {
        // Catch-all: unexpected exception (fail-closed)
        std::cerr << "[RPC-E8306] TLS reload failed: unexpected error — "
                  << e.what() << " (old credentials retained)" << std::endl;
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
        if (!success) {
          ++m.errors;
        }
        m.latency_ms += duration_ms;
    }

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.total_requests;
        if (success) {
          ++stats_.successful_requests;
        }
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
    std::ostringstream out = {};

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

    if (reqs.empty()) {
      return "";
    }

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
        if (!access_log_sink_) {
          return;
        }
        sink_copy = access_log_sink_;
    }

    // Build a minimal JSON object (no external JSON library required)
    auto jsEscape = [](const std::string& s) {
        std::string out = {};
        out.reserve(static_cast<int>(s.size()) + 2);
        out += '"';
        for (char c : s) {
            if      (c == '"') {
              out += "\\\"";
            }
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else                out += c;
        }
        out += '"';
        return out;
    };

    std::ostringstream js = {};
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
    if (!file) {
      throw std::runtime_error("Failed to open file: " + path);
    }
    std::ostringstream buf = {};
    buf << file.rdbuf();
    return buf.str();
}

std::shared_ptr<grpc::ServerCredentials>
GRPCServer::configureCredentials() {
    // Phase 3: Fail-closed credential configuration with explicit validation
    if (!config_.tls_enabled) {
        std::cerr << "[RPC-W/GAP-016] GRPCServer: TLS is disabled — using insecure gRPC "
                     "credentials. All gRPC traffic is unencrypted. "
                     "Enable TLS in production (CWE-295)." << std::endl;
        return grpc::InsecureServerCredentials();
    }

    try {
        // Validate paths exist and are readable (fail-closed)
        if (config_.tls_cert_path.empty() || config_.tls_key_path.empty() || 
            config_.tls_ca_cert_path.empty()) {
            throw std::runtime_error("TLS configuration incomplete: empty certificate paths");
        }

        std::string cert, key, ca;
        try {
            cert = loadFile(config_.tls_cert_path);
            key  = loadFile(config_.tls_key_path);
            ca   = loadFile(config_.tls_ca_cert_path);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to load TLS files: ") + e.what());
        }

        if (cert.empty() || key.empty()) {
            throw std::runtime_error("TLS certificate or key is empty");
        }

        auto creds = buildSslCredentials(cert, key, ca, config_.auth_required);
        {
            std::lock_guard<std::mutex> lock(tls_mutex_);
            credentials_ = creds;
        }
        return creds;

    } catch (const std::exception& e) {
        std::cerr << "[RPC-E8302] CRITICAL: Failed to configure TLS: " << e.what() 
                  << std::endl;
        std::cerr << "[RPC-E8302] Server will NOT start with insecure credentials for security"
                  << std::endl;
        throw std::runtime_error("TLS configuration failed - aborting for security");
    }
}

std::shared_ptr<grpc::ServerCredentials>
GRPCServer::buildSslCredentials(const std::string& cert_pem,
                                  const std::string& key_pem,
                                  const std::string& ca_pem,
                                  bool require_client_cert)
{
    // Phase 3: Explicit validation before building credentials
    if (cert_pem.empty() || key_pem.empty()) {
        throw std::runtime_error("Certificate or key PEM is empty");
    }

    try {
        grpc::SslServerCredentialsOptions ssl_opts;

        if (require_client_cert) {
            ssl_opts.client_certificate_request =
                GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
            std::cout << "[RPC-I] gRPC server configured for mutual TLS (mTLS)" << std::endl;
        } else {
            ssl_opts.client_certificate_request =
                GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
            std::cout << "[RPC-I] gRPC server configured for server-side TLS only" << std::endl;
        }

        ssl_opts.pem_root_certs = ca_pem;

        grpc::SslServerCredentialsOptions::PemKeyCertPair pair;
        pair.private_key = key_pem;
        pair.cert_chain  = cert_pem;
        ssl_opts.pem_key_cert_pairs.push_back(pair);

        return grpc::SslServerCredentials(ssl_opts);

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to build SSL credentials: ") + e.what());
    }
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
    auto plugin = std::make_unique<themis::plugins::rpc::grpc_plugin::GRPCPlugin>();
    return plugin.release();
}

void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
    delete plugin;
}

} // extern "C"

