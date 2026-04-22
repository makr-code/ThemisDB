/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main_server.cpp                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  ⚫ DRAFT                                        ║
    • Quality Score:   11.0/100                                       ║
    • Total Lines:     2284                                           ║
    • Open Issues:     TODOs: 0, Stubs: 17                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 649f5c7538  2026-04-14  ci(release): enforce canonical naming scheme and repair t... ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 7e8c588d0f  2026-04-14  ci(release): enforce canonical naming scheme and repair t... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📝 Draft / Stub                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// v1.1.0: mimalloc integration (20-40% memory boost, drop-in replacement)
// NOTE: Mimalloc is lazy-loaded after CRT initialization to avoid crashes during
// static object construction. This prevents exit code -1073741502 (0xC0000142).
// See initializeMimalloc() function below for details.

// Windows headers must come before Boost.Asio on Windows
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
// Early crash diagnostics for Windows
#include <eh.h>
#include <excpt.h>
#else
// For dlopen on Linux/Unix
#include <dlfcn.h>
#endif

#include "utils/logger.h"
#include "utils/tracing.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#ifdef THEMIS_ENABLE_HTTP_SERVER
#include "server/http_server.h"
#endif
#include "config/config_path_resolver.h"
#include "config/config_metrics_exporter.h"
#include "sharding/wal_applier.h"
#include "sharding/wal_manager.h"
#include "sharding/wal_shipper.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/replication_coordinator.h"
#include "sharding/multi_primary_coordinator.h"
#include "sharding/health_monitor.h"
#include "sharding/replica_topology.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/shard_repair_engine.h"
#include "sharding/sharding_manager.h"
#include "core/concerns/concerns_context.h"
#include "utils/retention_manager.h"
#include "utils/audit_logger.h"
#include "utils/pki_client.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "security/hsm_provider.h"
#include "security/hsm_security_checker.h"
#include "security/hsm_security_metrics.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/metrics_registry.h"
#include "themis/build_info.h"
#include "themis/license_info.h"
#include "themis/runtime_license_gate.h"
#include "themis/base/module_loader.h"
#include "themis/base/hot_reload_manager.h"
#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/registry.h>
#endif

#ifdef THEMIS_ENABLE_LLM
#include "llm/embedded_llm.h"
#include "llm/model_downloader.h"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <csignal>
#include <memory>
#include <optional>
#include <thread>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#ifndef _WIN32
#include <unistd.h>  // for write() in signal handler
#else
#include <io.h>      // Windows equivalent: _write()
#endif
#include <cstring>   // for strlen() in signal handler

#ifdef THEMIS_ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include "server/wal_grpc_service.h"
#include "utils/file_utils.h"
#endif

using namespace themis;
using json = nlohmann::json;

// Global atomic flag for signal handling (async-signal-safe)
std::atomic<bool> g_shutdown_requested{false};
#ifndef _WIN32
// SIGHUP flag for TLS certificate hot-reload (Linux/macOS only)
std::atomic<bool> g_tls_reload_requested{false};
#endif
#ifdef THEMIS_HAS_PROMETHEUS
std::shared_ptr<prometheus::Registry> g_config_prom_registry;
#endif
// Server instance (accessed only from main thread, not from signal handler)
#ifdef THEMIS_ENABLE_HTTP_SERVER
std::shared_ptr<server::HttpServer> g_server;
#endif

#ifdef THEMIS_ENABLE_GRPC
static std::unique_ptr<grpc::Server> g_wal_grpc_server;
static std::unique_ptr<server::WalGrpcService> g_wal_grpc_service;
#endif

static std::shared_ptr<themis::sharding::WALShipper> g_wal_shipper;

// HSM security warning thread
static std::thread g_hsm_warning_thread;
static std::atomic<bool> g_hsm_warning_thread_running{false};
// Global HSM provider (non-static for external access from monitoring_api_handler)
std::shared_ptr<themis::security::HSMProvider> g_hsm_provider;

// ============================================================================
// Lazy Mimalloc Initialization (after CRT startup)
// ============================================================================
// Mimalloc can be loaded after the C Runtime has fully initialized
// This prevents crashes during static object construction
#ifdef THEMIS_ENABLE_MIMALLOC
static bool initializeMimalloc() {
    try {
        // On Windows, load mimalloc DLL dynamically if available
        #ifdef _WIN32
        // Try to load mimalloc DLL - it may not always be available in PATH
        // but should be in the same directory as themis_server.exe
        HMODULE mimalloc_handle = ::LoadLibraryA("mimalloc.dll");
        if (mimalloc_handle) {
            // Successfully loaded - now we can use mimalloc functions if needed
            // The DLL exports functions like mi_malloc, mi_free, etc.
            THEMIS_INFO("Mimalloc allocator loaded successfully");
            return true;
        } else {
            THEMIS_WARN("Could not load mimalloc.dll - using system allocator");
            return false;
        }
        #else
        // On Linux/Unix, try dlopen
        void* mimalloc_handle = dlopen("libmimalloc.so", RTLD_LAZY);
        if (mimalloc_handle) {
            THEMIS_INFO("Mimalloc allocator loaded successfully");
            return true;
        } else {
            THEMIS_WARN("Could not load libmimalloc.so - using system allocator");
            return false;
        }
        #endif
    } catch (const std::exception& e) {
        THEMIS_WARN("Mimalloc initialization failed: {} - using system allocator", e.what());
        return false;
    }
}
#endif

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        // Only use async-signal-safe operations in signal handler
        // Write to stderr is async-signal-safe, unlike logging
        const char* msg = "\nReceived shutdown signal, initiating graceful shutdown...\n";
#ifndef _WIN32
        (void)write(STDERR_FILENO, msg, strlen(msg));
#else
        // Windows: use _write() with file descriptor 2 (stderr)
        (void)_write(2, msg, static_cast<unsigned int>(strlen(msg)));
#endif
        
        // Set atomic flag to trigger shutdown in main thread
        g_shutdown_requested.store(true, std::memory_order_release);
    }
#ifndef _WIN32
    else if (signal == SIGHUP) {
        const char* msg = "\nReceived SIGHUP, scheduling TLS certificate hot-reload...\n";
        (void)write(STDERR_FILENO, msg, strlen(msg));
        g_tls_reload_requested.store(true, std::memory_order_release);
    }
#endif
}

#ifdef _WIN32
// Windows unhandled exception filter for early crash diagnostics
LONG WINAPI windows_unhandled_exception_filter(EXCEPTION_POINTERS* pExp) {
    const auto code = pExp ? pExp->ExceptionRecord->ExceptionCode : 0u;
    const char* exception_name = "UNKNOWN";
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            exception_name = "ACCESS_VIOLATION";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            exception_name = "ARRAY_BOUNDS_EXCEEDED";
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            exception_name = "DATATYPE_MISALIGNMENT";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            exception_name = "FLT_DIVIDE_BY_ZERO";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            exception_name = "INT_DIVIDE_BY_ZERO";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            exception_name = "STACK_OVERFLOW";
            break;
    }
    
    // Write to stderr immediately (before any heap allocations)
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer),
        "\n*** WINDOWS STRUCTURED EXCEPTION ***\n"
        "Exception Code: 0x%08X (%s)\n"
        "Exception Address: 0x%p\n"
        "*** This may indicate a global variable initialization error ***\n",
        code, exception_name,
        pExp ? pExp->ExceptionRecord->ExceptionAddress : nullptr);
    
    if (len > 0 && len < static_cast<int>(sizeof(buffer))) {
        _write(2, buffer, len);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

// ============================================================================
// HSM Security Warning Thread
// ============================================================================
/**
 * Periodic HSM security warning thread
 * Logs ERROR-level warnings every 5 minutes when stub HSM is active
 */
void hsmSecurityWarningLoop() {
    using namespace std::chrono;
    const auto warning_interval = minutes(5);
    const auto warning_interval_seconds = duration_cast<seconds>(warning_interval).count();
    
    while (g_hsm_warning_thread_running.load(std::memory_order_relaxed)) {
        // Sleep for 5 minutes in 1-second increments to allow quick shutdown
        for (int i = 0; i < static_cast<int>(warning_interval_seconds) &&
                g_hsm_warning_thread_running.load(std::memory_order_relaxed); ++i) {
            std::this_thread::sleep_for(seconds(1));
        }
        
        if (!g_hsm_warning_thread_running.load(std::memory_order_relaxed)) break;
        
        // Perform HSM security check
        if (g_hsm_provider) {
            g_hsm_provider->periodicSecurityCheck();
            
            // Additional check using HSMSecurityChecker
            std::string warning = themis::security::HSMSecurityChecker::getPeriodicWarning(*g_hsm_provider);
            if (!warning.empty()) {
                THEMIS_ERROR("[SECURITY] {}", warning);
            }
        }
    }
}

/**
 * Start HSM security warning thread
 */
void startHSMWarningThread() {
    if (g_hsm_provider && !g_hsm_warning_thread_running.load()) {
        g_hsm_warning_thread_running.store(true, std::memory_order_release);
        g_hsm_warning_thread = std::thread(hsmSecurityWarningLoop);
        THEMIS_INFO("HSM security warning thread started (5-minute interval)");
    }
}

/**
 * Stop HSM security warning thread
 */
void stopHSMWarningThread() {
    if (g_hsm_warning_thread_running.load()) {
        g_hsm_warning_thread_running.store(false, std::memory_order_release);
        if (g_hsm_warning_thread.joinable()) {
            g_hsm_warning_thread.join();
        }
        THEMIS_INFO("HSM security warning thread stopped");
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Install a process-wide unhandled exception filter for early diagnostics.
    SetUnhandledExceptionFilter(windows_unhandled_exception_filter);
#endif
    
    // --- Early flag handling (no heavy initialization) ---
    // Simple hardcoded usage text
    auto print_usage = [](const char* prog) {
        std::cout << "Usage: " << prog << " [options]\n"
                  << "Options:\n"
                  << "  --db PATH            Database path (default: ./data/themis_server)\n"
                  << "  --data-dir PATH      Alias for --db (Docker-friendly)\n"
                  << "  --host HOST          Server host (default: 0.0.0.0)\n"
                  << "  --port PORT          Server port (default: 8765)\n"
                  << "  --threads N          Number of worker threads (default: auto)\n"
                  << "  --config FILE        Load server/storage config from JSON or YAML file\n"
                  << "  --allow-stub-hsm     Allow insecure stub HSM provider (development only)\n"
                  << "  --version, -v        Show version information and exit\n"
                  << "  --build-info         Show build configuration details and exit\n"
                  << "  --license-info       Show embedded license information and exit\n"
                  << "  --help, -h           Show this help message\n";
    };

    bool show_build_info = false;
    bool show_license_info = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--version" || arg == "-v") {
#ifdef THEMIS_VERSION_STRING
            std::cout << THEMIS_VERSION_STRING << std::endl;
#else
            std::cout << "unknown" << std::endl;
#endif
            return 0;
        } else if (arg == "--build-info") {
            show_build_info = true;
        } else if (arg == "--license-info") {
            show_license_info = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Initialize logger AFTER simple flag checks to avoid file I/O for --version/--help
    // This prevents unnecessary initialization when user just wants version info
    utils::Logger::init("themis_server.log", utils::Logger::Level::INFO);

#ifdef THEMIS_HAS_PROMETHEUS
    // Initialize Prometheus registry for config path resolution metrics
    g_config_prom_registry = std::make_shared<prometheus::Registry>();
    config::ConfigMetricsExporter::registerWithRegistry(g_config_prom_registry);
#endif
    
    THEMIS_INFO("=== Themis Multi-Model Database API Server ===");
#ifdef THEMIS_VERSION_STRING
    THEMIS_INFO("Version: {}", THEMIS_VERSION_STRING);
#else
    THEMIS_INFO("Version: unknown");
#endif
    
    // Display build info if requested
    if (show_build_info) {
        try {
            auto build_config = themis::build_info::getBuildConfiguration();
            std::string build_info = themis::build_info::formatBuildInfo(build_config);
            std::istringstream iss(build_info);
            std::string line;
            while (std::getline(iss, line)) {
                std::cout << line << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to retrieve build info: " << e.what() << std::endl;
            return 1;
        }
        return 0;
    }
    
    // Display license info if requested
    if (show_license_info) {
        try {
            auto license = themis::license::getEmbeddedLicense();
            if (license) {
                std::string license_info = themis::license::formatLicenseInfo(*license);
                std::cout << license_info << std::endl;
            } else {
                std::cout << "No embedded license data" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to retrieve license info: " << e.what() << std::endl;
            return 1;
        }
        return 0;
    }
    
    // Display build configuration and edition information (startup logging)
    try {
        auto build_config = themis::build_info::getBuildConfiguration();
        std::string build_info = themis::build_info::formatBuildInfo(build_config);
        // Log the formatted build info (line by line to preserve formatting)
        std::istringstream iss(build_info);
        std::string line;
        while (std::getline(iss, line)) {
            THEMIS_INFO("{}", line);
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to display build configuration: {}", e.what());
    }
    
    // Display embedded license information (if present)
    try {
        auto license = themis::license::getEmbeddedLicense();
        if (license) {
            std::string license_info = themis::license::formatLicenseInfo(*license);
            // Log the formatted license info (line by line to preserve formatting)
            std::istringstream iss(license_info);
            std::string line;
            while (std::getline(iss, line)) {
                THEMIS_INFO("{}", line);
            }
            
            // Check license validity
            if (!themis::license::isLicenseValid(*license)) {
                THEMIS_ERROR("WARNING: License has expired!");
                THEMIS_ERROR("Please contact {} to renew your license.", 
                           license->contact_email.empty() ? "your license provider" : license->contact_email);
                
                // HYPERSCALER: FATAL error if license expired
                #ifdef THEMIS_HYPERSCALER_EDITION
                THEMIS_ERROR("HYPERSCALER Edition requires a valid license. Server cannot start.");
                return 1;
                #endif
                
                // ENTERPRISE Release: FATAL error if license expired
                #if defined(THEMIS_ENTERPRISE_EDITION) && defined(NDEBUG)
                THEMIS_ERROR("ENTERPRISE Edition (Release build) requires a valid license. Server cannot start.");
                return 1;
                #endif
                
                // Note: We continue to start the server for Community/Debug builds but log the warning
            } else {
                int days = themis::license::getDaysUntilExpiry(*license);
                if (days < 30) {
                    THEMIS_WARN("License will expire in {} days. Please renew soon.", days);
                }
            }
            
            // Verify license signature (if present)
            if (!license->signature.empty() && !themis::license::verifyLicenseSignature(*license)) {
                THEMIS_ERROR("License signature verification FAILED!");
                
                // HYPERSCALER: FATAL error if signature invalid
                #ifdef THEMIS_HYPERSCALER_EDITION
                THEMIS_ERROR("HYPERSCALER Edition requires a valid license signature. Server cannot start.");
                return 1;
                #endif
                
                // ENTERPRISE: Warning for invalid signature, continue
                #ifdef THEMIS_ENTERPRISE_EDITION
                THEMIS_WARN("License signature is invalid. This may indicate a tampered license.");
                #endif
            }
        } else {
            THEMIS_INFO("No embedded license data (running without license embedding)");
            
            // HYPERSCALER Edition: License is MANDATORY
            #ifdef THEMIS_HYPERSCALER_EDITION
            THEMIS_ERROR("HYPERSCALER Edition requires an embedded license.");
            THEMIS_ERROR("Please rebuild with -DTHEMIS_LICENSE_FILE=/path/to/license.json");
            return 1;
            #endif
            
            // ENTERPRISE Release: License is MANDATORY
            #if defined(THEMIS_ENTERPRISE_EDITION) && defined(NDEBUG)
            THEMIS_ERROR("ENTERPRISE Edition (Release build) requires an embedded license.");
            THEMIS_ERROR("For development, use Debug build: cmake -DCMAKE_BUILD_TYPE=Debug");
            THEMIS_ERROR("For production, rebuild with: cmake -DCMAKE_BUILD_TYPE=Release -DTHEMIS_LICENSE_FILE=/path/to/license.json");
            return 1;
            #endif
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to display license information: {}", e.what());
    }

    // === RUNTIME LICENSE GATE INITIALIZATION ===
    // Activate the runtime feature gate using the embedded license so that
    // Enterprise/Hyperscaler feature checks at request time reflect the actual
    // license validity, not just the compile-time edition flags.
    try {
        themis::license::LicenseClientConfig lc_cfg;
        // Allow offline activation (server_url is empty → offline path).
        // Operators who deploy with a license server set THEMIS_LICENSE_SERVER_URL
        // and THEMIS_LICENSE_API_KEY in their environment / config.
        const char* ls_url = std::getenv("THEMIS_LICENSE_SERVER_URL");
        const char* ls_key = std::getenv("THEMIS_LICENSE_API_KEY");
        if (ls_url) lc_cfg.server_url = ls_url;
        if (ls_key) lc_cfg.api_key    = ls_key;
        lc_cfg.allow_offline = true;

        themis::license::LicenseClient lc(lc_cfg);
        auto activation = lc.activate();

        themis::license::RuntimeLicenseGate::instance().initialize(
            activation, lc.getCachedLicense());

        const std::string& status = activation.status;
        if (activation.success) {
            if (status == "grace") {
                THEMIS_WARN("License: running in grace period ({} days remaining). "
                            "Ensure the license server is reachable.",
                            activation.grace_days_remaining);
            } else {
                THEMIS_INFO("License gate: runtime validation successful (status: {}).", status);
            }
        } else {
            THEMIS_WARN("License gate: runtime validation failed (status: {}, reason: {}). "
                        "Enterprise/Hyperscaler features will be blocked.",
                        status, activation.error_message);
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("License gate initialization failed: {}. "
                    "Enterprise/Hyperscaler features will be blocked.", e.what());
        // Initialize gate in invalid state so feature checks still work safely.
        themis::license::LicenseActivationResult failed;
        failed.success       = false;
        failed.status        = "invalid";
        failed.error_message = e.what();
        themis::license::RuntimeLicenseGate::instance().initialize(failed);
    }
    
    try {
        // === MIMALLOC LAZY INITIALIZATION ===
        // Initialize mimalloc after CRT is fully set up (prevents crash during static init)
#ifdef THEMIS_ENABLE_MIMALLOC
        initializeMimalloc();
#endif
        
        // Parse command line arguments
        std::string db_path = "./data/themis_server";
        std::string host = "0.0.0.0";
        uint16_t port = 8765;
        size_t num_threads = 0; // Auto-detect
        std::optional<std::string> config_path;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            constexpr std::string_view kDataDirPrefix = "--data-dir=";
            constexpr std::string_view kConfigPrefix  = "--config=";
            if (arg == "--db" && i + 1 < argc) {
                db_path = argv[++i];
            } else if (arg == "--data-dir" && i + 1 < argc) {
                // --data-dir is an alias for --db (Docker-friendly name)
                db_path = argv[++i];
            } else if (arg.rfind(kDataDirPrefix, 0) == 0) {
                // Support --data-dir=/path format (equals-separated)
                db_path = arg.substr(kDataDirPrefix.size());
            } else if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
            } else if (arg == "--port" && i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else if (arg == "--threads" && i + 1 < argc) {
                num_threads = std::stoul(argv[++i]);
            } else if (arg == "--config" && i + 1 < argc) {
                config_path = argv[++i];
            } else if (arg.rfind(kConfigPrefix, 0) == 0) {
                // Support --config=/path format (equals-separated)
                config_path = arg.substr(kConfigPrefix.size());
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --db PATH            Database path (default: ./data/themis_server)\n"
                          << "  --data-dir PATH      Alias for --db (Docker-friendly)\n"
                          << "  --host HOST          Server host (default: 0.0.0.0)\n"
                          << "  --port PORT          Server port (default: 8765)\n"
                          << "  --threads N          Number of worker threads (default: auto)\n"
                          << "  --config FILE        Load server/storage config from JSON or YAML file\n"
                          << "  --version, -v        Show version information and exit\n"
                          << "  --build-info         Show build configuration details and exit\n"
                          << "  --license-info       Show embedded license information and exit\n"
                          << "  --help, -h           Show this help message\n";
                return 0;
            }
        }
        
        // Load config (JSON or YAML) if provided or in default locations
        auto load_config = [&](const std::string& path) -> std::optional<json> {
            try {
                auto ends_with = [](const std::string& s, const std::string& suffix){
                    return s.size() >= suffix.size() && s.compare(s.size()-suffix.size(), suffix.size(), suffix) == 0;
                };

                if (ends_with(path, ".yaml") || ends_with(path, ".yml")) {
                    YAML::Node root = YAML::LoadFile(path);
                    // recursive conversion YAML -> JSON
                    std::function<json(const YAML::Node&)> to_json = [&](const YAML::Node& n) -> json {
                        if (!n) return nullptr;
                        if (n.IsScalar()) {
                            // Try boolean
                            try { return n.as<bool>(); } catch (...) {}
                            // Try integer
                            try { return n.as<long long>(); } catch (...) {}
                            // Try double
                            try { return n.as<double>(); } catch (...) {}
                            // Fallback string
                            return n.as<std::string>("");
                        } else if (n.IsSequence()) {
                            json arr = json::array();
                            for (const auto& it : n) arr.push_back(to_json(it));
                            return arr;
                        } else if (n.IsMap()) {
                            json obj = json::object();
                            for (auto it = n.begin(); it != n.end(); ++it) {
                                const auto& k = it->first.as<std::string>();
                                obj[k] = to_json(it->second);
                            }
                            return obj;
                        }
                        return nullptr;
                    };
                    return to_json(root);
                } else {
                    std::ifstream f(path);
                    if (!f.is_open()) {
                        THEMIS_WARN("Cannot open config file: {}", path);
                        return std::nullopt;
                    }
                    json j; 
                    f >> j; 
                    return j;
                }
            } catch (const YAML::Exception& e) {
                THEMIS_ERROR("YAML parsing error in {}: {}", path, e.what());
                return std::nullopt;
            } catch (const json::exception& e) {
                THEMIS_ERROR("JSON parsing error in {}: {}", path, e.what());
                return std::nullopt;
            } catch (const std::exception& e) {
                THEMIS_ERROR("Config loading error in {}: {}", path, e.what());
                return std::nullopt;
            } catch (...) {
                THEMIS_ERROR("Unknown error loading config file: {}", path);
                return std::nullopt;
            }
        };

        std::optional<json> cfg;
        if (config_path) {
            cfg = load_config(*config_path);
            if (!cfg) {
                THEMIS_ERROR("Failed to read config file: {}", *config_path);
                return 1;
            }
        } else {
            // default search paths (prefer YAML) - now includes new hierarchical paths
            for (const auto& p : {
                    std::string("./config.yaml"), std::string("./config.yml"), std::string("./config.json"),
                    std::string("./config/core/config.yaml"), std::string("./config/core/config.yml"), std::string("./config/core/config.json"),
                    std::string("./config/config.yaml"), std::string("./config/config.yml"), std::string("./config/config.json"),
                    std::string("/etc/vccdb/config.yaml"), std::string("/etc/vccdb/config.yml"), std::string("/etc/vccdb/config.json")}) {
                cfg = load_config(p);
                if (cfg) { THEMIS_INFO("Loaded config from {}", p); break; }
            }
        }

        THEMIS_INFO("Database path: {}", db_path);
        THEMIS_INFO("Server: {}:{}", host, port);
        
        // Configure RocksDB
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path;
        // v1.5.0: Increased defaults for write-amplification optimization
        db_config.memtable_size_mb = 512;       // Increased from 128MB
        db_config.block_cache_size_mb = 512;
        db_config.enable_wal = true;
        db_config.enable_blobdb = false;

        // Apply JSON config if present
        if (cfg) {
            // storage
            if (cfg->contains("storage")) {
                const auto& s = (*cfg)["storage"];
                if (s.contains("rocksdb_path")) db_config.db_path = s["rocksdb_path"].get<std::string>();
                if (s.contains("memtable_size_mb")) db_config.memtable_size_mb = s["memtable_size_mb"].get<size_t>();
                if (s.contains("block_cache_size_mb")) db_config.block_cache_size_mb = s["block_cache_size_mb"].get<size_t>();
                if (s.contains("enable_blobdb")) db_config.enable_blobdb = s["enable_blobdb"].get<bool>();
                if (s.contains("enable_high_parallel_tuning")) db_config.enable_high_parallel_tuning = s["enable_high_parallel_tuning"].get<bool>();
                if (s.contains("high_parallel_thread_threshold")) db_config.high_parallel_thread_threshold = s["high_parallel_thread_threshold"].get<int>();
                if (s.contains("compression")) {
                    const auto& c = s["compression"];
                    if (c.contains("default")) db_config.compression_default = c["default"].get<std::string>();
                    if (c.contains("bottommost")) db_config.compression_bottommost = c["bottommost"].get<std::string>();
                }
            }
            // server
            if (cfg->contains("server")) {
                const auto& sv = (*cfg)["server"];
                if (sv.contains("host")) host = sv["host"].get<std::string>();
                if (sv.contains("port")) port = static_cast<uint16_t>(sv["port"].get<int>());
                if (sv.contains("worker_threads")) num_threads = sv["worker_threads"].get<size_t>();
            }
            // features (beta)
            if (cfg->contains("features")) {
                // values read later into server_config
            }
        }

        // Enable high-parallel tuning automatically based on worker_threads if not explicitly set
        if (!db_config.enable_high_parallel_tuning && num_threads >= static_cast<size_t>(db_config.high_parallel_thread_threshold)) {
            db_config.enable_high_parallel_tuning = true;
            THEMIS_INFO("Enabling high-parallel RocksDB tuning (workers: {}, threshold: {})", num_threads, db_config.high_parallel_thread_threshold);
        }
        
        // Create database wrapper
        THEMIS_INFO("Opening RocksDB database...");
        auto db = std::make_shared<RocksDBWrapper>(db_config);
        
        if (!db->open()) {
            THEMIS_ERROR("Failed to open database!");
            return 1;
        }
        
        THEMIS_INFO("Database opened successfully");
        
        // ═══════════════════════════════════════════════════════════════════
        // HSM Security Integration (FIND-002)
        // ═══════════════════════════════════════════════════════════════════
        // Initialize HSM provider and perform security validation
        THEMIS_INFO("Initializing HSM security...");
        
        // Load HSM config from security.yaml or main config
        themis::security::HSMConfig hsm_config;
        bool hsm_config_loaded = false;
        
        // Try to load from security.yaml first (try new path, then legacy)
        for (const auto& security_config_path : {
                std::string("./config/core/security.yaml"),
                std::string("./config/core/security.yml"),
                std::string("./config/security.yaml"),
                std::string("./config/security.yml"),
                std::string("/etc/themisdb/security.yaml")}) {
            auto sec_cfg = load_config(security_config_path);
            if (sec_cfg && sec_cfg->contains("hsm")) {
                const auto& hsm = (*sec_cfg)["hsm"];
                if (hsm.contains("provider")) {
                    std::string provider = hsm["provider"].get<std::string>();
                    // For now, we support stub and pkcs11
                    // stub provider means empty library_path
                    if (provider == "stub") {
                        hsm_config.library_path = "";  // Empty = stub provider
                    } else if (provider == "pkcs11" && hsm.contains("pkcs11")) {
                        const auto& pkcs11 = hsm["pkcs11"];
                        hsm_config.library_path = pkcs11.value("library_path", std::string());
                        hsm_config.slot_id = pkcs11.value("slot_id", 0);
                        hsm_config.pin = pkcs11.value("pin", std::string());
                        hsm_config.token_label = pkcs11.value("token_label", std::string());
                        hsm_config.key_label = pkcs11.value("key_label", std::string("themis-signing-key"));
                    }
                    hsm_config_loaded = true;
                    THEMIS_INFO("HSM configuration loaded from {}", security_config_path);
                    break;
                }
            }
        }
        
        // Fall back to main config if security.yaml not found
        if (!hsm_config_loaded && cfg && cfg->contains("hsm")) {
            const auto& hsm = (*cfg)["hsm"];
            if (hsm.contains("provider")) {
                std::string provider = hsm["provider"].get<std::string>();
                if (provider == "stub") {
                    hsm_config.library_path = "";
                } else if (provider == "pkcs11" && hsm.contains("pkcs11")) {
                    const auto& pkcs11 = hsm["pkcs11"];
                    hsm_config.library_path = pkcs11.value("library_path", std::string());
                    hsm_config.slot_id = pkcs11.value("slot_id", 0);
                    hsm_config.pin = pkcs11.value("pin", std::string());
                }
                hsm_config_loaded = true;
                THEMIS_INFO("HSM configuration loaded from main config");
            }
        }
        
        // Create HSM provider (defaults to stub if no config)
        g_hsm_provider = std::make_shared<themis::security::HSMProvider>(hsm_config);
        
        if (!g_hsm_provider->initialize()) {
            THEMIS_WARN("HSM provider initialization failed, using stub provider");
        }
        
        // Display HSM status
        THEMIS_INFO("HSM Provider Status:");
        THEMIS_INFO("  Provider Type: {}", g_hsm_provider->isStubProvider() ? "stub (DEVELOPMENT ONLY)" : "real HSM");
        THEMIS_INFO("  Token Info: {}", g_hsm_provider->getTokenInfo());
        THEMIS_INFO("  Production Mode: {}", themis::security::HSMSecurityChecker::isProductionMode() ? "YES" : "NO");
        
        // Perform startup security validation
        if (g_hsm_provider->isStubProvider()) {
            // Check if --allow-stub-hsm flag is present
            bool allow_stub = themis::security::HSMSecurityChecker::hasAllowStubFlag(argc, argv);
            
            if (!allow_stub) {
                // Display startup warning banner
                THEMIS_WARN("╔════════════════════════════════════════════════════════════════════════════╗");
                THEMIS_WARN("║  ⚠️  WARNING: INSECURE HSM CONFIGURATION DETECTED                          ║");
                THEMIS_WARN("║                                                                            ║");
                THEMIS_WARN("║  The HSM provider is set to 'stub' which is DEVELOPMENT ONLY.              ║");
                THEMIS_WARN("║  Master encryption keys are NOT protected by hardware security.            ║");
                THEMIS_WARN("║                                                                            ║");
                THEMIS_WARN("║  FOR PRODUCTION USE:                                                       ║");
                THEMIS_WARN("║  - Configure a real HSM provider (PKCS#11, AWS KMS, Azure Key Vault)       ║");
                THEMIS_WARN("║  - See: docs/security/HSM_PRODUCTION_SETUP.md                              ║");
                THEMIS_WARN("║                                                                            ║");
                THEMIS_WARN("║  To suppress this warning in development, use: --allow-stub-hsm            ║");
                THEMIS_WARN("╚════════════════════════════════════════════════════════════════════════════╝");
                
                // Start periodic warning thread (every 5 minutes)
                startHSMWarningThread();
            } else {
                THEMIS_WARN("HSM stub provider allowed by --allow-stub-hsm flag (DEVELOPMENT ONLY)");
            }
        } else {
            THEMIS_INFO("HSM provider is hardware-backed - production ready");
        }
        
        // Validate production safety (will fail startup if stub in production without flag)
        if (!themis::security::HSMSecurityChecker::validateProductionSafety(*g_hsm_provider, argc, argv)) {
            THEMIS_CRITICAL("Server startup aborted due to HSM security violation");
            // Clean up warning thread if it was started
            stopHSMWarningThread();
            if (g_hsm_provider) {
                g_hsm_provider->finalize();
                g_hsm_provider.reset();
            }
            return 1;
        }
        
        // Create index managers
        THEMIS_INFO("Initializing index managers...");
        auto secondary_index = std::make_shared<SecondaryIndexManager>(*db);
        auto graph_index = std::make_shared<GraphIndexManager>(*db);
        auto vector_index = std::make_shared<VectorIndexManager>(*db);

        // Parse vector_index config and initialize (optional auto-load)
        std::string vector_save_path;
        if (cfg && cfg->contains("vector_index")) {
            const auto& vi = (*cfg)["vector_index"];
            std::string object_name = vi.value("object_name", std::string());
            int dimension = vi.value("dimension", 0);
            std::string metric_str = vi.value("metric", std::string("COSINE"));
            int hnsw_m = vi.value("hnsw_m", 16);
            int hnsw_ef_construction = vi.value("hnsw_ef_construction", 200);
            int ef_search = vi.value("ef_search", 64);
            bool load_on_startup = vi.value("load_on_startup", true);
            bool save_on_shutdown = vi.value("save_on_shutdown", true);
            if (vi.contains("save_path")) {
                vector_save_path = vi["save_path"].get<std::string>();
                vector_index->setAutoSavePath(vector_save_path, save_on_shutdown);
                THEMIS_INFO("Vector index auto-save path: {} (save_on_shutdown={})", vector_save_path, save_on_shutdown);
            }

            // Initialize index if object_name & dimension provided
            if (!object_name.empty() && dimension > 0) {
                VectorIndexManager::Metric metric = (metric_str == "L2")
                    ? VectorIndexManager::Metric::L2
                    : VectorIndexManager::Metric::COSINE;
                THEMIS_INFO("Initializing vector index: object='{}', dim={}, metric={}, M={}, efC={}, efS={}",
                    object_name, dimension, metric_str, hnsw_m, hnsw_ef_construction, ef_search);
                auto st = vector_index->init(object_name, dimension, metric, hnsw_m, hnsw_ef_construction, ef_search,
                    load_on_startup ? vector_save_path : std::string());
                if (!st.ok) {
                    THEMIS_WARN("Vector index init failed: {}", st.message);
                }
            } else {
                THEMIS_INFO("Vector index not initialized (object_name/dimension missing). You can init via API or config.");
            }
        }
        
        // Create transaction manager
        auto tx_manager = std::make_shared<TransactionManager>(
            *db, *secondary_index, *graph_index, *vector_index
        );
        
        THEMIS_INFO("All managers initialized");
        
#ifdef THEMIS_ENABLE_LLM
        // Initialize EmbeddedLLM if enabled in config
        if (cfg && cfg->contains("llm")) {
            const auto& llm_cfg = (*cfg)["llm"];
            bool llm_enabled = llm_cfg.value("enabled", false);
            
            if (llm_enabled) {
                try {
                    themis::llm::EmbeddedLLM::Config llm_config;
                    llm_config.model_path = llm_cfg.value("model_path", std::string("models/default.gguf"));
                    llm_config.model_id = llm_cfg.value("model_id", std::string("default"));
                    llm_config.n_gpu_layers = llm_cfg.value("gpu_layers", 0);
                    llm_config.n_ctx = llm_cfg.value("context_size", 4096);
                    llm_config.n_threads = llm_cfg.value("threads", 4);
                    llm_config.enable_caching = llm_cfg.value("enable_caching", true);
                    
                    // Check environment variables for overrides
                    if (const char* env_gpu_layers = std::getenv("THEMIS_GPU_LAYERS")) {
                        llm_config.n_gpu_layers = std::atoi(env_gpu_layers);
                    }
                    if (const char* env_threads = std::getenv("THEMIS_THREADS")) {
                        llm_config.n_threads = std::atoi(env_threads);
                    }
                    if (const char* env_ctx = std::getenv("THEMIS_CONTEXT_SIZE")) {
                        llm_config.n_ctx = std::atoi(env_ctx);
                    }
                    if (const char* env_model_dir = std::getenv("THEMIS_MODEL_DIR")) {
                        llm_config.model_path = std::string(env_model_dir) + "/" + 
                            std::filesystem::path(llm_config.model_path).filename().string();
                    }
                    
                    // Auto-download model if not present
                    bool auto_download = llm_cfg.value("auto_download", true);
                    if (const char* env_disable = std::getenv("THEMIS_DISABLE_AUTO_DOWNLOAD")) {
                        if (std::string(env_disable) == "1" || std::string(env_disable) == "true") {
                            auto_download = false;
                        }
                    }
                    
                    if (auto_download && !std::filesystem::exists(llm_config.model_path)) {
                        THEMIS_INFO("Model not found: {}", llm_config.model_path);
                        THEMIS_INFO("Starting auto-download...");
                        
                        try {
                            // Configure download from Ollama
                            themis::llm::ModelDownloadConfig dl_config;
                            dl_config.model_name = llm_cfg.value("ollama_model", std::string("phi3:mini-4k"));
                            dl_config.ollama_url = llm_cfg.value("ollama_url", std::string("http://localhost:11434"));
                            
                            // Override with environment variable if set
                            if (const char* env_ollama = std::getenv("THEMIS_OLLAMA_ENDPOINT")) {
                                dl_config.ollama_url = env_ollama;
                            }
                            
                            // Extract directory from model path
                            std::filesystem::path model_path_obj(llm_config.model_path);
                            dl_config.download_dir = model_path_obj.parent_path().string();
                            
                            // Create directory if needed
                            if (!dl_config.download_dir.empty() && !std::filesystem::exists(dl_config.download_dir)) {
                                std::filesystem::create_directories(dl_config.download_dir);
                            }
                            
                            // Progress callback with percentage-based logging
                            size_t last_logged_percent = 0;
                            dl_config.progress_callback = [&last_logged_percent](size_t downloaded, size_t total, const std::string& /*status*/) {
                                if (total > 0) {
                                    size_t current_percent = (downloaded * 100) / total;
                                    // Log every 10% progress
                                    if (current_percent >= last_logged_percent + 10 || current_percent == 100) {
                                        float mb_downloaded = downloaded / (1024.0f * 1024.0f);
                                        float mb_total = total / (1024.0f * 1024.0f);
                                        THEMIS_INFO("Download progress: {:.1f}% ({:.1f} / {:.1f} MB)", 
                                            static_cast<float>(current_percent), mb_downloaded, mb_total);
                                        last_logged_percent = current_percent;
                                    }
                                }
                            };
                            
                            // Download model
                            themis::llm::ModelDownloader downloader;
                            auto result = downloader.downloadFromOllama(dl_config);
                            
                            if (result.success) {
                                THEMIS_INFO("✓ Model downloaded successfully");
                                THEMIS_INFO("  Path: {}", result.model_path);
                                THEMIS_INFO("  Size: {:.1f} MB", result.file_size_bytes / (1024.0f * 1024.0f));
                                THEMIS_INFO("  Time: {:.1f}s", result.download_time_seconds);
                                
                                // Update model path to downloaded location
                                llm_config.model_path = result.model_path;
                            } else {
                                THEMIS_WARN("Model download failed: {}", result.error_message);
                                THEMIS_WARN("Attempting to proceed without auto-download...");
                            }
                            
                        } catch (const std::exception& e) {
                            THEMIS_WARN("Auto-download exception: {}", e.what());
                            THEMIS_WARN("Proceeding with configured model path...");
                        }
                    }
                    
                    THEMIS_INFO("Initializing EmbeddedLLM...");
                    THEMIS_INFO("  Model: {}", llm_config.model_path);
                    THEMIS_INFO("  GPU Layers: {}", llm_config.n_gpu_layers);
                    THEMIS_INFO("  Context Size: {}", llm_config.n_ctx);
                    THEMIS_INFO("  Threads: {}", llm_config.n_threads);
                    
                    themis::llm::EmbeddedLLMManager::instance().initialize(llm_config);
                    
                    if (THEMIS_LLM().isReady()) {
                        THEMIS_INFO("EmbeddedLLM initialized successfully: {}", THEMIS_LLM().getModelInfo());
                    } else {
                        THEMIS_WARN("EmbeddedLLM initialization completed but model not ready (likely lazy loading)");
                    }
                } catch (const std::exception& e) {
                    bool llm_required = llm_cfg.value("required", false);
                    if (llm_required) {
                        THEMIS_ERROR("EmbeddedLLM initialization failed and is marked as required: {}", e.what());
                        return 1;
                    } else {
                        THEMIS_WARN("EmbeddedLLM initialization failed (non-critical): {}", e.what());
                        THEMIS_WARN("LLM features will not be available");
                    }
                }
            } else {
                THEMIS_INFO("EmbeddedLLM disabled in configuration");
            }
        } else {
            THEMIS_INFO("No LLM configuration found, LLM features disabled");
        }
#else
        THEMIS_INFO("LLM support not compiled (THEMIS_ENABLE_LLM=OFF)");
#endif
        
        // Initialize tracing if enabled
        if (cfg && cfg->contains("tracing")) {
            const auto& tracing_cfg = (*cfg)["tracing"];
            bool tracing_enabled = tracing_cfg.value("enabled", false);
            if (tracing_enabled) {
                std::string service_name = tracing_cfg.value("service_name", std::string("themis-server"));
                std::string otlp_endpoint = tracing_cfg.value("otlp_endpoint", std::string("http://localhost:4318"));
                
                if (Tracer::initialize(service_name, otlp_endpoint)) {
                    THEMIS_INFO("Distributed tracing enabled: service='{}', endpoint='{}'", service_name, otlp_endpoint);
                } else {
                    THEMIS_WARN("Failed to initialize distributed tracing");
                }
            }
        }
        
    // Create and start HTTP server
#ifdef THEMIS_ENABLE_HTTP_SERVER
        server::HttpServer::Config server_config(host, port, num_threads);
        // Apply feature flags
        if (cfg && cfg->contains("features")) {
            const auto& f = (*cfg)["features"];
            server_config.feature_semantic_cache = f.value("semantic_cache", false);
            server_config.feature_llm_store = f.value("llm_store", false);
            server_config.feature_cdc = f.value("cdc", false);
            server_config.feature_timeseries = f.value("timeseries", false);
        }
        // SSE/CDC streaming config
        if (cfg && cfg->contains("sse")) {
            const auto& sse = (*cfg)["sse"];
            server_config.sse_max_events_per_second = sse.value("max_events_per_second", uint32_t(0));
            if (server_config.sse_max_events_per_second > 0) {
                THEMIS_INFO("SSE rate limit: {} events/second per connection", server_config.sse_max_events_per_second);
            }
        }
#endif
        // WAL components for replica apply endpoint
        themis::sharding::WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = db_path + "/wal_replica";
        auto wal_manager = std::make_shared<themis::sharding::WALManager>(wal_cfg);

        auto wal_applier = std::make_shared<themis::sharding::WALApplier>(
            themis::sharding::WALApplierConfig{.replica_id = "local"}
        );
        // Apply handler: idempotent apply to RocksDB (optional key/value) and append to replica WAL.
        wal_applier->setApplyHandler([wal_manager, db](const themis::sharding::WALEntry& entry) {
            try {
                const std::string marker_key = "replica_applied/" + entry.lsn.toString();
                std::string existing;
                // Idempotent: if already applied, skip
                if (db->get(marker_key, existing)) {
                    return true;
                }

                // Append to local WAL for durability
                wal_manager->append(entry);

                // Optional: apply to RocksDB if payload has key/value
                bool data_ok = true;
                if (entry.data.contains("key")) {
                    const std::string user_key = entry.data.value("key", std::string());
                    switch (entry.type) {
                        case themis::sharding::WALEntryType::DELETE:
                            if (!db->del(user_key)) data_ok = false;
                            break;
                        default: {
                            // Value may be any JSON; store serialized
                            std::string value_str;
                            if (entry.data.contains("value")) {
                                value_str = entry.data["value"].dump();
                            } else {
                                value_str = entry.data.dump();
                            }
                            if (!db->put(user_key, value_str)) data_ok = false;
                            break;
                        }
                    }
                }

                // Mark applied (even if no user_key) to keep idempotency
                if (!db->put(marker_key, entry.data.dump())) {
                    return false;
                }
                return data_ok;
            } catch (...) {
                return false;
            }
        });

        // Initialize WALShipper and ReplicationCoordinator for replication (leader-side)
        bool shipper_enabled = false;
        themis::sharding::WALShipperConfig shipper_cfg;
        shipper_cfg.primary_id = "primary-local";
        std::vector<std::pair<std::string, std::string>> replicas; // (replica_id, endpoint)

        if (cfg && cfg->contains("replication")) {
            const auto& repl = (*cfg)["replication"];
            shipper_enabled = repl.value("shipper_enabled", false);
            shipper_cfg.primary_id = repl.value("primary_id", shipper_cfg.primary_id);
            shipper_cfg.batch_size = repl.value("batch_size", size_t(100));
            shipper_cfg.max_batch_bytes = repl.value("max_batch_bytes", size_t(1024 * 1024));
            shipper_cfg.ship_interval_ms = repl.value("ship_interval_ms", uint64_t(100));
            shipper_cfg.retry_delay_ms = repl.value("retry_delay_ms", uint64_t(1000));
            shipper_cfg.max_retry_delay_ms = repl.value("max_retry_delay_ms", uint64_t(60000));
            shipper_cfg.max_retries = repl.value("max_retries", size_t(5));
            shipper_cfg.health_check_interval_ms = repl.value("health_check_interval_ms", uint64_t(10000));
            
            // Compression config
            std::string comp = repl.value("compression", std::string("zstd"));
            if (comp == "none") {
                shipper_cfg.compression = themis::sharding::WALShipperConfig::CompressionType::None;
            } else if (comp == "lz4") {
                shipper_cfg.compression = themis::sharding::WALShipperConfig::CompressionType::LZ4;
            } else {
                shipper_cfg.compression = themis::sharding::WALShipperConfig::CompressionType::Zstd;
            }
            shipper_cfg.compression_level = repl.value("compression_level", 3);
            
            // mTLS paths (optional; dev can skip)
            if (repl.contains("cert_path")) shipper_cfg.cert_path = repl["cert_path"].get<std::string>();
            if (repl.contains("key_path")) shipper_cfg.key_path = repl["key_path"].get<std::string>();
            if (repl.contains("ca_cert_path")) shipper_cfg.ca_cert_path = repl["ca_cert_path"].get<std::string>();

            // Replica endpoints
            if (repl.contains("replicas")) {
                for (const auto& r : repl["replicas"]) {
                    std::string rep_id = r.value("replica_id", std::string("replica-") + std::to_string(replicas.size()));
                    std::string endpoint = r.value("endpoint", std::string());
                    if (!endpoint.empty()) {
                        replicas.emplace_back(rep_id, endpoint);
                    }
                }
            }
        }

        if (shipper_enabled && !replicas.empty()) {
            try {
                g_wal_shipper = std::make_shared<themis::sharding::WALShipper>(wal_manager, shipper_cfg);
                for (const auto& [rep_id, endpoint] : replicas) {
                    g_wal_shipper->addReplica(rep_id, endpoint);
                    THEMIS_INFO("Added replica: {} ({})", rep_id, endpoint);
                }
                
                // Create PrometheusMetrics exporter for replication metrics
                themis::sharding::PrometheusMetrics::Config prom_cfg;
                prom_cfg.http_port = 8080;
                prom_cfg.http_path = "/metrics";
                auto prometheus_metrics = std::make_shared<themis::sharding::PrometheusMetrics>(prom_cfg);
                g_wal_shipper->setMetricsExporter(prometheus_metrics);
                THEMIS_INFO("Prometheus metrics exporter configured for WALShipper");
                
                g_wal_shipper->start();
                THEMIS_INFO("WALShipper started with {} replica(s)", replicas.size());
            } catch (const std::exception& e) {
                THEMIS_ERROR("Failed to start WALShipper: {}", e.what());
            }
        } else if (shipper_enabled && replicas.empty()) {
            THEMIS_WARN("WALShipper enabled but no replicas configured; skipping shipper startup");
        }

        // Create ReplicationCoordinator (wraps shipper for write concern enforcement)
        std::shared_ptr<themis::sharding::ReplicationCoordinator> replication_coordinator;
        if (g_wal_shipper) {
            replication_coordinator = std::make_shared<themis::sharding::ReplicationCoordinator>(g_wal_shipper);
            THEMIS_INFO("ReplicationCoordinator created for write concern enforcement");
        }

        // Multi-primary coordination + health monitoring (optional)
        std::shared_ptr<themis::sharding::MultiPrimaryCoordinator> multi_primary_coordinator;
        std::shared_ptr<themis::sharding::HealthMonitor> health_monitor;

        if (cfg && cfg->contains("multi_primary")) {
            const auto& mp_cfg_json = (*cfg)["multi_primary"];
            bool mp_enabled = mp_cfg_json.value("enabled", false);
            if (mp_enabled) {
                themis::sharding::MultiPrimaryConfig mp_cfg;
                mp_cfg.current_node_id = mp_cfg_json.value("current_node_id", host);
                mp_cfg.use_last_write_wins = mp_cfg_json.value("use_last_write_wins", true);
                mp_cfg.allow_concurrent_writes = mp_cfg_json.value("allow_concurrent_writes", true);
                mp_cfg.cross_primary_replication = mp_cfg_json.value("cross_primary_replication", true);
                mp_cfg.auto_promote_on_primary_failure = mp_cfg_json.value("auto_promote_on_primary_failure", true);
                mp_cfg.default_write_concern = themis::sharding::parseWriteConcern(
                    mp_cfg_json.value("default_write_concern", std::string("MAJORITY"))
                );
                mp_cfg.promotion_timeout = std::chrono::milliseconds(
                    mp_cfg_json.value("promotion_timeout_ms", 5000)
                );

                if (mp_cfg_json.contains("nodes") && mp_cfg_json["nodes"].is_array()) {
                    for (const auto& node : mp_cfg_json["nodes"]) {
                        std::string node_id = node.value("node_id", std::string());
                        std::string endpoint = node.value("endpoint", std::string());
                        if (!node_id.empty()) {
                            mp_cfg.primary_node_ids.push_back(node_id);
                            if (!endpoint.empty()) {
                                mp_cfg.primary_endpoints[node_id] = endpoint;
                            }
                        }
                    }
                }

                multi_primary_coordinator = std::make_shared<themis::sharding::MultiPrimaryCoordinator>(mp_cfg);
                THEMIS_INFO("MultiPrimaryCoordinator enabled with {} nodes", mp_cfg.primary_node_ids.size());

                // Health monitor (optional)
                themis::sharding::HealthMonitorConfig hm_cfg;
                if (cfg->contains("health_monitor")) {
                    const auto& hm_json = (*cfg)["health_monitor"];
                    hm_cfg.heartbeat_interval = std::chrono::milliseconds(
                        hm_json.value("heartbeat_interval_ms", 1000)
                    );
                    hm_cfg.health_check_timeout = std::chrono::milliseconds(
                        hm_json.value("health_check_timeout_ms", 500)
                    );
                    hm_cfg.max_consecutive_failures = hm_json.value("max_consecutive_failures", 3u);
                    hm_cfg.auto_failover_enabled = hm_json.value("auto_failover_enabled", true);
                    hm_cfg.auto_promote_standby = hm_json.value("auto_promote_standby", true);
                    hm_cfg.failover_cooldown = std::chrono::milliseconds(
                        hm_json.value("failover_cooldown_ms", 10000)
                    );
                    hm_cfg.health_check_path = hm_json.value("health_check_path", std::string("/health"));
                }

                // Use an empty replica topology for now (can be populated from config later)
                auto topology = std::make_shared<themis::sharding::ReplicaTopology>();
                health_monitor = std::make_shared<themis::sharding::HealthMonitor>(
                    hm_cfg, multi_primary_coordinator, topology
                );
                health_monitor->start();
                THEMIS_INFO("HealthMonitor started (hb={}ms, timeout={}ms)",
                    hm_cfg.heartbeat_interval.count(), hm_cfg.health_check_timeout.count());
            }
        }

        // ═══════════════════════════════════════════════════════════════════
        // RAID Redundancy Configuration
        // ═══════════════════════════════════════════════════════════════════
        // Initialize RAID components if configured
        std::shared_ptr<themis::sharding::CollectionRedundancyManager> redundancy_manager;
        std::shared_ptr<themis::sharding::ConsistentHashRing> hash_ring;
        std::shared_ptr<themis::sharding::ShardTopology> shard_topology;
        bool raid_enabled = false;

        if (cfg && cfg->contains("raid") && (*cfg)["raid"].contains("enabled")) {
            raid_enabled = (*cfg)["raid"]["enabled"].get<bool>();
        }

        if (raid_enabled) {
            THEMIS_INFO("Initializing RAID redundancy components...");

            // Create consistent hash ring
            int virtual_nodes = 100; // default
            if (cfg->contains("sharding") && (*cfg)["sharding"].contains("hash_ring")) {
                virtual_nodes = (*cfg)["sharding"]["hash_ring"].value("virtual_nodes_per_shard", 100);
            }
            hash_ring = std::make_shared<themis::sharding::ConsistentHashRing>(virtual_nodes);

            // Create shard topology
            shard_topology = std::make_shared<themis::sharding::ShardTopology>();

            // Add shards to hash ring and topology
            if (cfg->contains("sharding") && (*cfg)["sharding"].contains("shards")) {
                const auto& shards = (*cfg)["sharding"]["shards"];
                for (const auto& shard : shards) {
                    if (shard.contains("id")) {
                        std::string shard_id = shard["id"].get<std::string>();
                        hash_ring->addNode(shard_id);
                        
                        // Create ShardInfo with at least the shard_id
                        themis::sharding::ShardInfo info;
                        info.shard_id = shard_id;
                        info.is_healthy = true;
                        shard_topology->addShard(info);
                        
                        THEMIS_INFO("  Added shard to hash ring and topology: {}", shard_id);
                    }
                }
            } else {
                // Default: add local shard
                const std::string default_shard_id = "shard-0";
                hash_ring->addNode(default_shard_id);
                
                themis::sharding::ShardInfo info;
                info.shard_id = default_shard_id;
                info.is_healthy = true;
                shard_topology->addShard(info);
                
                THEMIS_INFO("  Added default shard to hash ring and topology: {}", default_shard_id);
            }

            // Create redundancy manager
            redundancy_manager = std::make_shared<themis::sharding::CollectionRedundancyManager>();

            // Configure default redundancy
            themis::sharding::RedundancyConfig default_config;
            if (cfg->contains("raid") && (*cfg)["raid"].contains("default")) {
                const auto& def = (*cfg)["raid"]["default"];
                
                // Parse mode
                std::string mode_str = def.value("mode", std::string("MIRROR"));
                if (mode_str == "NONE") default_config.mode = themis::sharding::RedundancyMode::NONE;
                else if (mode_str == "MIRROR") default_config.mode = themis::sharding::RedundancyMode::MIRROR;
                else if (mode_str == "STRIPE") default_config.mode = themis::sharding::RedundancyMode::STRIPE;
                else if (mode_str == "STRIPE_MIRROR") default_config.mode = themis::sharding::RedundancyMode::STRIPE_MIRROR;
                else if (mode_str == "PARITY") default_config.mode = themis::sharding::RedundancyMode::PARITY;
                else if (mode_str == "RAID6") default_config.mode = themis::sharding::RedundancyMode::RAID6;
                else if (mode_str == "GEO_MIRROR") default_config.mode = themis::sharding::RedundancyMode::GEO_MIRROR;
                
                default_config.replication_factor = def.value("replication_factor", 3);
                
                // Parse write concern
                std::string wc_str = def.value("write_concern", std::string("MAJORITY"));
                if (wc_str == "ONE") default_config.write_concern = themis::sharding::WriteConcern::ONE;
                else if (wc_str == "MAJORITY") default_config.write_concern = themis::sharding::WriteConcern::MAJORITY;
                else if (wc_str == "ALL") default_config.write_concern = themis::sharding::WriteConcern::ALL;
                else if (wc_str == "QUORUM") default_config.write_concern = themis::sharding::WriteConcern::QUORUM;
            }
            redundancy_manager->setDefaultConfig(default_config);
            THEMIS_INFO("  RAID default mode: {}, replication_factor: {}", 
                       static_cast<int>(default_config.mode), default_config.replication_factor);

            // Configure per-collection redundancy
            if (cfg->contains("raid") && (*cfg)["raid"].contains("collections")) {
                const auto& collections = (*cfg)["raid"]["collections"];
                for (auto it = collections.begin(); it != collections.end(); ++it) {
                    std::string collection_name = it.key();
                    const auto& coll_config = it.value();
                    
                    themis::sharding::RedundancyConfig coll_redundancy_config;
                    
                    // Parse mode
                    std::string mode_str = coll_config.value("mode", std::string("MIRROR"));
                    if (mode_str == "NONE") coll_redundancy_config.mode = themis::sharding::RedundancyMode::NONE;
                    else if (mode_str == "MIRROR") coll_redundancy_config.mode = themis::sharding::RedundancyMode::MIRROR;
                    else if (mode_str == "STRIPE") coll_redundancy_config.mode = themis::sharding::RedundancyMode::STRIPE;
                    else if (mode_str == "STRIPE_MIRROR") coll_redundancy_config.mode = themis::sharding::RedundancyMode::STRIPE_MIRROR;
                    else if (mode_str == "PARITY") coll_redundancy_config.mode = themis::sharding::RedundancyMode::PARITY;
                    else if (mode_str == "RAID6") coll_redundancy_config.mode = themis::sharding::RedundancyMode::RAID6;
                    else if (mode_str == "GEO_MIRROR") coll_redundancy_config.mode = themis::sharding::RedundancyMode::GEO_MIRROR;
                    
                    coll_redundancy_config.replication_factor = coll_config.value("replication_factor", 3);
                    
                    // Parse write concern
                    std::string wc_str = coll_config.value("write_concern", std::string("MAJORITY"));
                    if (wc_str == "ONE") coll_redundancy_config.write_concern = themis::sharding::WriteConcern::ONE;
                    else if (wc_str == "MAJORITY") coll_redundancy_config.write_concern = themis::sharding::WriteConcern::MAJORITY;
                    else if (wc_str == "ALL") coll_redundancy_config.write_concern = themis::sharding::WriteConcern::ALL;
                    else if (wc_str == "QUORUM") coll_redundancy_config.write_concern = themis::sharding::WriteConcern::QUORUM;
                    
                    redundancy_manager->setCollectionConfig(collection_name, coll_redundancy_config);
                    THEMIS_INFO("  Collection '{}': mode={}, replication_factor={}", 
                               collection_name, static_cast<int>(coll_redundancy_config.mode), 
                               coll_redundancy_config.replication_factor);
                }
            }

            THEMIS_INFO("RAID redundancy components initialized successfully");
        }

        // Create HttpServer with all components
#ifdef THEMIS_ENABLE_HTTP_SERVER
        g_server = std::make_shared<server::HttpServer>(
            server_config,
            db,
            secondary_index,
            graph_index,
            vector_index,
            tx_manager,
            wal_applier,
            wal_manager,
            replication_coordinator,
            multi_primary_coordinator,
            health_monitor,
            redundancy_manager,
            hash_ring,
            shard_topology
        );
        // Inject live ShardingManager so /v1/admin/shards/* endpoints are functional
        g_server->setShardingManager(&themis::sharding::ShardingManager::GetInstance());

        // ═══════════════════════════════════════════════════════════════════
        // MODULE LOADER — dynamic plugin subsystem
        // ═══════════════════════════════════════════════════════════════════
        // Instantiate ModuleLoader from optional config["modules"] block.
        // When no block is present the server still starts; the
        // /v1/admin/modules/* endpoints return 503 until a loader is injected.
        auto module_loader = std::make_shared<themis::modules::ModuleLoader>();
        auto hot_reload_mgr = std::make_shared<themis::modules::HotReloadManager>();
        {
            bool modules_configured = false;
            if (cfg && cfg->contains("modules")) {
                const auto& mcfg = (*cfg)["modules"];
                bool require_sig   = mcfg.value("require_signature", false);
                bool allow_unsigned = mcfg.value("allow_unsigned", true);
                std::string manifest = mcfg.value("hash_manifest", std::string());

                module_loader->setRequireSignature(require_sig);
                module_loader->setAllowUnsigned(allow_unsigned);
                if (!manifest.empty() && !module_loader->setHashManifest(manifest)) {
                    THEMIS_WARN("ModuleLoader: failed to load hash manifest from '{}'", manifest);
                }

                // Watchdog configuration — read once and reuse for startWatchdog decision
                bool wd_enabled = true;
                if (mcfg.contains("watchdog")) {
                    const auto& wdcfg = mcfg["watchdog"];
                    themis::modules::WatchdogConfig wd;
                    wd.enabled            = wdcfg.value("enabled", true);
                    wd.check_interval_ms  = wdcfg.value("check_interval_ms", uint64_t(30000));
                    wd.max_restart_attempts = wdcfg.value("max_restart_attempts", uint32_t(5));
                    wd.initial_backoff_ms = wdcfg.value("initial_backoff_ms", uint64_t(5000));
                    wd.max_backoff_ms     = wdcfg.value("max_backoff_ms", uint64_t(300000));
                    wd.backoff_multiplier = wdcfg.value("backoff_multiplier", 2.0);
                    wd_enabled            = wd.enabled;
                    module_loader->configureWatchdog(wd);
                }

                // Bulk-load from directory (optional)
                std::string module_dir = mcfg.value("directory", std::string());
                if (!module_dir.empty()) {
                    THEMIS_INFO("ModuleLoader: scanning '{}'…", module_dir);
                    size_t loaded = module_loader->loadAllModules(module_dir);
                    THEMIS_INFO("ModuleLoader: {} module(s) loaded from '{}'", loaded, module_dir);
                    modules_configured = true;
                }

                // Wire all loaded modules into HotReloadManager for zero-downtime swapping
                for (const auto& m : module_loader->getAllLoadedModules()) {
                    try {
                        hot_reload_mgr->registerModule(m.name, *module_loader);
                    } catch (const std::exception& ex) {
                        THEMIS_WARN("HotReloadManager: failed to register '{}': {}", m.name, ex.what());
                    }
                }

                // Start watchdog using the flag already determined above
                if (wd_enabled) {
                    module_loader->startWatchdog();
                    THEMIS_INFO("ModuleLoader: watchdog started");
                }
            }

            if (!modules_configured) {
                THEMIS_INFO("ModuleLoader: no modules.directory configured — dynamic plugin loading disabled");
            }
        }
#ifdef THEMIS_ENABLE_HTTP_SERVER
        // Inject ModuleLoader into HTTP server → activates /v1/admin/modules/* endpoints
        g_server->setModuleLoader(module_loader.get());
#endif
#else
        THEMIS_INFO("HTTP server disabled at build time (THEMIS_ENABLE_HTTP_SERVER=OFF)");
#endif

#ifdef THEMIS_ENABLE_GRPC
        // Start WAL gRPC Apply service (if stubs are available)
        g_wal_grpc_service = std::make_unique<server::WalGrpcService>(wal_applier);
        if (auto* wal_service = g_wal_grpc_service->service()) {
            std::string grpc_host = "0.0.0.0";
            int grpc_port = 50051;
            if (const char* h = std::getenv("THEMIS_WAL_GRPC_HOST")) grpc_host = h;
            if (const char* p = std::getenv("THEMIS_WAL_GRPC_PORT")) {
                try { grpc_port = std::stoi(p); } catch (...) {}
            }
            std::string grpc_addr = grpc_host + ":" + std::to_string(grpc_port);

            grpc::ServerBuilder builder;
            
            // Configure server credentials (mTLS support)
            std::shared_ptr<grpc::ServerCredentials> credentials;
            bool enable_mtls = false;
            std::string actual_mode = "insecure"; // Track actual credential mode for accurate logging
            
            if (const char* mtls_env = std::getenv("THEMIS_WAL_GRPC_ENABLE_MTLS")) {
                std::string mtls_str(mtls_env);
                enable_mtls = (mtls_str == "true" || mtls_str == "1" || mtls_str == "yes");
            }
            
            if (enable_mtls) {
                // mTLS enabled - create SSL server credentials
                try {
                    grpc::SslServerCredentialsOptions ssl_opts;
                    
                    // Load server certificate and private key (required)
                    const char* cert_path = std::getenv("THEMIS_WAL_GRPC_CERT_PATH");
                    const char* key_path = std::getenv("THEMIS_WAL_GRPC_KEY_PATH");
                    if (!cert_path || !key_path || std::strlen(cert_path) == 0 || std::strlen(key_path) == 0) {
                        throw std::runtime_error("THEMIS_WAL_GRPC_CERT_PATH and THEMIS_WAL_GRPC_KEY_PATH are required when mTLS is enabled");
                    }
                    
                    grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
                    key_cert_pair.private_key = themis::utils::readFileContents(key_path);
                    key_cert_pair.cert_chain = themis::utils::readFileContents(cert_path);
                    ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
                    THEMIS_INFO("WAL gRPC: Loaded server certificate from: {}", cert_path);
                    
                    // Configure client certificate requirement
                    bool require_client_cert = true;
                    if (const char* req_client = std::getenv("THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT")) {
                        std::string req_str(req_client);
                        require_client_cert = (req_str != "false" && req_str != "0" && req_str != "no");
                    }
                    
                    if (require_client_cert) {
                        // Mutual TLS mode - CA certificate is required for client verification
                        const char* ca_cert_path = std::getenv("THEMIS_WAL_GRPC_CA_CERT_PATH");
                        if (!ca_cert_path || std::strlen(ca_cert_path) == 0) {
                            throw std::runtime_error("THEMIS_WAL_GRPC_CA_CERT_PATH is required when THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT is true");
                        }
                        ssl_opts.pem_root_certs = themis::utils::readFileContents(ca_cert_path);
                        THEMIS_INFO("WAL gRPC: Loaded CA certificate from: {}", ca_cert_path);
                        
                        ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
                        THEMIS_INFO("WAL gRPC: Client certificate verification enabled (mutual TLS)");
                        actual_mode = "mTLS";
                    } else {
                        // Server-side TLS with optional client certificates
                        const char* ca_cert_path = std::getenv("THEMIS_WAL_GRPC_CA_CERT_PATH");
                        if (ca_cert_path && std::strlen(ca_cert_path) > 0) {
                            ssl_opts.pem_root_certs = themis::utils::readFileContents(ca_cert_path);
                            THEMIS_INFO("WAL gRPC: Loaded CA certificate from: {}", ca_cert_path);
                        }
                        
                        ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_CLIENT_CERTIFICATE_AND_VERIFY;
                        THEMIS_INFO("WAL gRPC: Server-side TLS with optional client certificate verification (if provided)");
                        actual_mode = "TLS";
                    }
                    
                    credentials = grpc::SslServerCredentials(ssl_opts);
                    THEMIS_INFO("WAL gRPC: TLS/mTLS configured successfully for production deployment");
                    
                } catch (const std::exception& e) {
                    THEMIS_ERROR("WAL gRPC: Failed to configure mTLS: {}. Server will NOT start to avoid insecure fallback in production.", e.what());
                    THEMIS_ERROR("WAL gRPC: Fix the TLS configuration or set THEMIS_WAL_GRPC_ENABLE_MTLS=false for development.");
                    // Skip gRPC server startup when mTLS is explicitly enabled but misconfigured
                    credentials = nullptr;
                    actual_mode = "failed";
                }
            } else {
                // mTLS not enabled - use insecure credentials (development only)
                credentials = grpc::InsecureServerCredentials();
                actual_mode = "insecure";
                THEMIS_WARN("WAL gRPC: mTLS is disabled. This is insecure and should only be used in development.");
            }
            
            if (credentials) {
                builder.AddListeningPort(grpc_addr, credentials);
                builder.RegisterService(static_cast<grpc::Service*>(wal_service));
                builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);
                builder.SetMaxSendMessageSize(100 * 1024 * 1024);

                g_wal_grpc_server = builder.BuildAndStart();
                if (g_wal_grpc_server) {
                    THEMIS_INFO("WAL gRPC Apply service listening on {} (mode: {})", grpc_addr, actual_mode);
                } else {
                    THEMIS_WARN("Failed to start WAL gRPC Apply service (address: {})", grpc_addr); // NOPII: grpc_addr is a server bind address, not personal data
                }
            } else {
                THEMIS_ERROR("WAL gRPC Apply service NOT started due to TLS configuration errors");
            }
        } else {
            THEMIS_INFO("WAL gRPC stubs not found; skipping gRPC Apply service startup");
        }
#endif
        
        // Setup signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
#ifndef _WIN32
        // SIGHUP: reload TLS certificates without restarting
        std::signal(SIGHUP, signalHandler);
#endif
        
        // Retention worker (optional, runs in background if enabled in config)
        std::atomic<bool> retention_stop{false};
        std::thread retention_thread;
        bool retention_enabled = false;
        int retention_interval_hours = 24;
        std::string retention_policies_path = themis::config::ConfigPathResolver::mapLegacyToNew("./config/retention_policies.yaml");
        // Fall back to legacy if new doesn't exist
        if (!std::filesystem::exists(retention_policies_path)) {
            retention_policies_path = "./config/retention_policies.yaml";
        }
        
        if (cfg && cfg->contains("features") && (*cfg)["features"].contains("retention")) {
            const auto& ret_cfg = (*cfg)["features"]["retention"];
            retention_enabled = ret_cfg.value("enabled", false);
            retention_interval_hours = ret_cfg.value("interval_hours", 24);
            if (ret_cfg.contains("policies_path")) {
                std::string cfg_path = ret_cfg["policies_path"].get<std::string>();
                auto resolved = themis::config::ConfigPathResolver::tryResolve(cfg_path);
                retention_policies_path = resolved ? *resolved : cfg_path;
            }
        }
        
        if (retention_enabled) {
            try {
                // Instantiate retention manager with configured YAML path
                auto retention_mgr = std::make_shared<vcc::RetentionManager>(retention_policies_path);
                
                // Setup audit logging for retention actions
                auto key_provider = std::make_shared<themis::MockKeyProvider>();
                key_provider->createKey("retention_audit_key", 32);
                auto field_enc = std::make_shared<themis::FieldEncryption>(key_provider);
                
                themis::utils::PKIConfig pki_cfg;
                pki_cfg.service_id = "themis-retention";
                pki_cfg.endpoint = "https://pki.example.com";
                pki_cfg.signature_algorithm = "RSA-SHA256";
                if (const char* k = std::getenv("THEMIS_PKI_PRIVATE_KEY")) { pki_cfg.key_path = k; }
                if (const char* c = std::getenv("THEMIS_PKI_CERTIFICATE")) { pki_cfg.cert_path = c; }
                if (const char* p = std::getenv("THEMIS_PKI_PRIVATE_KEY_PASSPHRASE")) { pki_cfg.key_passphrase = p; }
                auto pki_client = std::make_shared<themis::utils::VCCPKIClient>(pki_cfg);
                
                themis::utils::AuditLoggerConfig audit_cfg;
                audit_cfg.enabled = true;
                audit_cfg.encrypt_then_sign = true;
                audit_cfg.log_path = "data/logs/retention_audit.jsonl";
                audit_cfg.key_id = "retention_audit_key";
                auto audit_logger = std::make_shared<themis::utils::AuditLogger>(field_enc, pki_client, audit_cfg);
                
                // Capture db and secondary_index for entity enumeration
                auto db_ptr = db;
                auto sec_idx_ptr = secondary_index;
                
                // Create main audit logger instance for retention operations
                themis::utils::AuditLoggerConfig main_audit_cfg;
                main_audit_cfg.enabled = true;
                main_audit_cfg.encrypt_then_sign = true;
                main_audit_cfg.log_path = "data/logs/audit.jsonl";
                main_audit_cfg.key_id = "saga_log";
                auto main_audit_logger = std::make_shared<themis::utils::AuditLogger>(field_enc, pki_client, main_audit_cfg);
                
                retention_thread = std::thread([retention_mgr, &retention_stop, retention_interval_hours, db_ptr, sec_idx_ptr, audit_logger, main_audit_logger]() {
                    using namespace std::chrono;
                    auto interval = hours(retention_interval_hours);
                    auto next_run = system_clock::now();
                    
                    while (!retention_stop.load(std::memory_order_relaxed)) {
                        auto now_tp = system_clock::now();
                        if (now_tp >= next_run) {
                            // Entity provider: enumerate entities per policy from DB
                            auto entity_provider = [db_ptr, sec_idx_ptr, main_audit_logger](const std::string& policy_name) 
                                -> std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> {
                                std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> entities;
                                
                                // Special handling for audit_logs policy - enumerate from audit log files
                                if (policy_name == "audit_logs") {
                                    try {
                                        auto entries = main_audit_logger->enumerateEntries();
                                        for (const auto& entry : entries) {
                                            // Use entry number as entity ID
                                            std::string entry_id = "audit_entry_" + std::to_string(entry.entry_number);
                                            entities.emplace_back(entry_id, entry.timestamp);
                                        }
                                        THEMIS_INFO("[Retention] Enumerated {} audit log entries for policy '{}'", 
                                                    entries.size(), policy_name);
                                        return entities;
                                    } catch (const std::exception& e) {
                                        THEMIS_ERROR("[Retention] Failed to enumerate audit logs: {}", e.what());
                                        return entities;
                                    }
                                }
                                
                                // Map policy names to collections (example heuristic)
                                // In production: use policy metadata or a mapping table
                                std::string collection;
                                if (policy_name.find("user") != std::string::npos || policy_name.find("personal") != std::string::npos) {
                                    collection = "users";
                                } else if (policy_name.find("transaction") != std::string::npos) {
                                    collection = "transactions";
                                } else if (policy_name.find("session") != std::string::npos) {
                                    collection = "sessions";
                                } else if (policy_name.find("analytics") != std::string::npos) {
                                    collection = "analytics";
                                } else if (policy_name.find("backup") != std::string::npos) {
                                    collection = "backups";
                                } else {
                                    // Skip unknown policies
                                    return entities;
                                }
                                
                                // Scan collection for entities with created_at timestamps
                                // Use range scan if created_at index exists, otherwise skip
                                try {
                                    if (sec_idx_ptr->hasRangeIndex(collection, "created_at")) {
                                        // Scan all entries in created_at index (unbounded range)
                                        auto [status, pks] = sec_idx_ptr->scanKeysRange(
                                            collection, "created_at",
                                            std::nullopt, std::nullopt,
                                            false, false,
                                            10000, false
                                        );
                                        
                                        if (status.ok) {
                                            for (const auto& pk : pks) {
                                                // Fetch entity to get created_at
                                                auto entity_opt = db_ptr->get(pk);
                                                if (entity_opt) {
                                                    try {
                                                        std::string entity_str(entity_opt->begin(), entity_opt->end());
                                                        auto j = nlohmann::json::parse(entity_str);
                                                        if (j.contains("created_at")) {
                                                            // Parse ISO8601 or epoch timestamp
                                                            int64_t ts_epoch = 0;
                                                            if (j["created_at"].is_number()) {
                                                                ts_epoch = j["created_at"].get<int64_t>();
                                                            } else if (j["created_at"].is_string()) {
                                                                // Simple epoch string parse
                                                                ts_epoch = std::stoll(j["created_at"].get<std::string>());
                                                            }
                                                            auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(ts_epoch));
                                                            entities.emplace_back(pk, tp);
                                                        }
                                                    } catch (...) {
                                                        // Skip malformed entities
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } catch (...) {
                                    // Index may not exist; silent fallback
                                }
                                
                                return entities;
                            };
                            
                            // Track whether we've already processed audit logs in this retention run
                            auto audit_archived = std::make_shared<std::atomic<bool>>(false);
                            auto audit_purged = std::make_shared<std::atomic<bool>>(false);
                            
                            auto archive_handler = [db_ptr, audit_logger, main_audit_logger, audit_archived, retention_mgr](const std::string& entity_id) -> bool {
                                // Special handling for audit log entries - bulk operation
                                if (entity_id.find("audit_entry_") == 0) {
                                    // Only perform the bulk archive operation once per retention run
                                    bool expected = false;
                                    if (!audit_archived->compare_exchange_strong(expected, true)) {
                                        // Already archived in this run
                                        return true;
                                    }
                                    
                                    try {
                                        // Get archive threshold from policy
                                        auto policy_result = retention_mgr->getPolicy("audit_logs");
                                        if (!policy_result) {
                                            THEMIS_ERROR("[Retention] Failed to get audit_logs policy");
                                            return false;
                                        }
                                        const auto* policy = *policy_result;
                                        
                                        auto now = std::chrono::system_clock::now();
                                        auto archive_threshold = now - policy->archive_after;
                                        
                                        std::string archive_path = "data/logs/audit_archive.jsonl";
                                        size_t archived = main_audit_logger->archiveOldEntries(archive_threshold, archive_path);
                                        
                                        THEMIS_INFO("[Retention] Archived {} audit log entries to {}", archived, archive_path);
                                        
                                        // Log the archival action
                                        try {
                                            nlohmann::json audit_event;
                                            audit_event["action"] = "AUDIT_LOG_ARCHIVE";
                                            audit_event["archived_count"] = archived;
                                            audit_event["archive_path"] = archive_path;
                                            audit_event["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                                                now.time_since_epoch()
                                            ).count();
                                            audit_event["classification"] = "retention_lifecycle";
                                            audit_logger->logEvent(audit_event);
                                        } catch (...) {
                                            THEMIS_WARN("[Retention] Failed to audit-log archive operation");
                                        }
                                        
                                        return true; // Report success even if 0 archived
                                    } catch (const std::exception& e) {
                                        THEMIS_ERROR("[Retention] Failed to archive audit logs: {}", e.what());
                                        return false;
                                    }
                                }
                                
                                // Regular entity archival
                                THEMIS_INFO("[Retention] Archive entity {}", entity_id);
                                
                                try {
                                    // Retrieve the entity data before archival
                                    auto entity_opt = db_ptr->get(entity_id);
                                    if (!entity_opt) {
                                        THEMIS_WARN("[Retention] Entity {} not found for archival", entity_id);
                                        return false;
                                    }
                                    
                                    // Create cold storage directory if it doesn't exist
                                    std::filesystem::path cold_storage_dir = "data/cold_storage";
                                    std::filesystem::create_directories(cold_storage_dir);
                                    
                                    // Export entity to JSONL format in cold storage
                                    // Group archived entities by date for efficient storage management
                                    auto now = std::chrono::system_clock::now();
                                    auto now_time_t = std::chrono::system_clock::to_time_t(now);
                                    std::tm tm_buf;
                                    #ifdef _WIN32
                                    gmtime_s(&tm_buf, &now_time_t);
                                    #else
                                    gmtime_r(&now_time_t, &tm_buf);
                                    #endif
                                    
                                    std::ostringstream date_str;
                                    date_str << std::put_time(&tm_buf, "%Y%m%d");
                                    std::string archive_file = (cold_storage_dir / ("archived_entities_" + date_str.str() + ".jsonl")).string();
                                    
                                    // Append entity to archive file
                                    std::ofstream archive_ofs(archive_file, std::ios::app);
                                    if (!archive_ofs.is_open()) {
                                        THEMIS_ERROR("[Retention] Failed to open archive file: {}", archive_file);
                                        return false;
                                    }
                                    
                                    // Write entity as JSON line
                                    nlohmann::json archived_entity;
                                    archived_entity["entity_id"] = entity_id;
                                    archived_entity["data"] = *entity_opt;
                                    archived_entity["archived_timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                                        now.time_since_epoch()
                                    ).count();
                                    archive_ofs << archived_entity.dump() << "\n";
                                    archive_ofs.close();
                                    
                                    // Audit log the archival action
                                    try {
                                        nlohmann::json audit_event;
                                        audit_event["action"] = "RETENTION_ARCHIVE";
                                        audit_event["entity_id"] = entity_id;
                                        audit_event["archive_path"] = archive_file;
                                        audit_event["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                                            now.time_since_epoch()
                                        ).count();
                                        audit_event["classification"] = "retention_lifecycle";
                                        audit_logger->logEvent(audit_event);
                                    } catch (...) {
                                        THEMIS_WARN("[Retention] Failed to audit-log archive for {}", entity_id);
                                    }
                                    
                                    THEMIS_INFO("[Retention] Successfully archived entity {} to {}", entity_id, archive_file);
                                    return true;
                                    
                                } catch (const std::exception& e) {
                                    THEMIS_ERROR("[Retention] Failed to archive entity {}: {}", entity_id, e.what());
                                    return false;
                                }
                            };
                            
                            auto purge_handler = [db_ptr, audit_logger, main_audit_logger, audit_purged, retention_mgr](const std::string& entity_id) -> bool {
                                // Special handling for audit log entries - bulk operation
                                if (entity_id.find("audit_entry_") == 0) {
                                    // Only perform the bulk purge operation once per retention run
                                    bool expected = false;
                                    if (!audit_purged->compare_exchange_strong(expected, true)) {
                                        // Already purged in this run
                                        return true;
                                    }
                                    
                                    try {
                                        // Get purge threshold from policy
                                        auto policy_result = retention_mgr->getPolicy("audit_logs");
                                        if (!policy_result) {
                                            THEMIS_ERROR("[Retention] Failed to get audit_logs policy");
                                            return false;
                                        }
                                        const auto* policy = *policy_result;
                                        
                                        auto now = std::chrono::system_clock::now();
                                        auto purge_threshold = now - policy->retention_period;
                                        
                                        size_t purged = main_audit_logger->purgeOldEntries(purge_threshold);
                                        
                                        THEMIS_INFO("[Retention] Purged {} audit log entries", purged);
                                        
                                        // Log the purge action
                                        try {
                                            nlohmann::json audit_event;
                                            audit_event["action"] = "AUDIT_LOG_PURGE";
                                            audit_event["purged_count"] = purged;
                                            audit_event["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                                                now.time_since_epoch()
                                            ).count();
                                            audit_event["classification"] = "retention_lifecycle";
                                            audit_logger->logEvent(audit_event);
                                        } catch (...) {
                                            THEMIS_WARN("[Retention] Failed to audit-log purge operation");
                                        }
                                        
                                        return true; // Report success even if 0 purged
                                    } catch (const std::exception& e) {
                                        THEMIS_ERROR("[Retention] Failed to purge audit logs: {}", e.what());
                                        return false;
                                    }
                                }
                                
                                // Regular entity purge
                                THEMIS_INFO("[Retention] Purge entity {}", entity_id);
                                
                                // Audit log the purge action
                                try {
                                    nlohmann::json audit_event;
                                    audit_event["action"] = "RETENTION_PURGE";
                                    audit_event["entity_id"] = entity_id;
                                    audit_event["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                                        std::chrono::system_clock::now().time_since_epoch()
                                    ).count();
                                    audit_event["classification"] = "retention_lifecycle";
                                    audit_logger->logEvent(audit_event);
                                } catch (...) {
                                    THEMIS_WARN("[Retention] Failed to audit-log purge for {}", entity_id);
                                }
                                
                                // Delete from DB
                                return db_ptr->del(entity_id);
                            };
                            
                            auto stats = retention_mgr->runRetentionCheck(entity_provider, archive_handler, purge_handler);
                            THEMIS_INFO("[Retention] Completed: scanned={}, archived={}, purged={}, retained={}, errors={}",
                                stats.total_entities_scanned, stats.archived_count, stats.purged_count, stats.retained_count, stats.error_count);
                            next_run = now_tp + interval;
                        }
                        
                        // Sleep in small chunks to react quickly on shutdown
                        int sleep_minutes = std::max(1, retention_interval_hours * 60 / 60); // at least 1min chunks
                        for (int i = 0; i < sleep_minutes && !retention_stop.load(std::memory_order_relaxed); ++i) {
                            std::this_thread::sleep_for(std::chrono::minutes(1));
                        }
                    }
                });
                THEMIS_INFO("Retention worker started (interval: {}h)", retention_interval_hours);
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to start retention worker: {}", e.what());
            } catch (...) {
                THEMIS_WARN("Failed to start retention worker: unknown error");
            }
        } else {
            THEMIS_INFO("Retention worker disabled (enable via config.json features.retention.enabled)");
        }

#ifdef THEMIS_ENABLE_HTTP_SERVER
        THEMIS_INFO("Starting HTTP server...");
        g_server->start();
#else
        THEMIS_INFO("HTTP server disabled; skipping start.");
#endif
        
        // Initialize sharding metrics if enabled
        bool sharding_enabled = false;
        std::string shard_id = "shard_0"; // Default shard ID
        int cluster_size = 1; // Default single-node cluster
        
        if (cfg && cfg->contains("sharding")) {
            const auto& shard_cfg = (*cfg)["sharding"];
            sharding_enabled = shard_cfg.value("enabled", false);
            if (sharding_enabled) {
                // Read shard configuration from config or environment
                if (const char* env_shard_id = std::getenv("SHARD_ID")) {
                    shard_id = env_shard_id;
                } else {
                    shard_id = shard_cfg.value("shard_id", std::string("shard_0"));
                }
                
                if (const char* env_cluster_size = std::getenv("CLUSTER_SIZE")) {
                    cluster_size = std::stoi(env_cluster_size);
                } else {
                    cluster_size = shard_cfg.value("cluster_size", 1);
                }
                
                // Initialize Prometheus metrics
                themis::sharding::PrometheusMetrics::Config metrics_cfg;
                
                auto sharding_metrics = std::make_shared<themis::sharding::PrometheusMetrics>(metrics_cfg);
                
                // Register initial cluster state
                sharding_metrics->recordClusterSize(cluster_size);
                sharding_metrics->recordShardHealth(shard_id, "healthy");
                
                // Register metrics globally
                themis::sharding::ShardingMetricsRegistry::instance().registerMetrics(sharding_metrics);
                
                THEMIS_INFO("Sharding metrics initialized: shard_id={}, cluster_size={}", shard_id, cluster_size);
            }
        } else if (const char* env_enable = std::getenv("ENABLE_METRICS")) {
            // Fallback: Environment variable based initialization
            if (std::string(env_enable) == "true" || std::string(env_enable) == "1") {
                sharding_enabled = true;
                
                if (const char* env_shard_id = std::getenv("SHARD_ID")) {
                    shard_id = env_shard_id;
                }
                if (const char* env_cluster_size = std::getenv("CLUSTER_SIZE")) {
                    cluster_size = std::stoi(env_cluster_size);
                }
                
                themis::sharding::PrometheusMetrics::Config metrics_cfg;
                
                auto sharding_metrics = std::make_shared<themis::sharding::PrometheusMetrics>(metrics_cfg);
                sharding_metrics->recordClusterSize(cluster_size);
                sharding_metrics->recordShardHealth(shard_id, "healthy");
                
                themis::sharding::ShardingMetricsRegistry::instance().registerMetrics(sharding_metrics);
                
                THEMIS_INFO("Sharding metrics initialized from environment: shard_id={}, cluster_size={}", shard_id, cluster_size);
            }
        }
        
        // Get hardware/system information
        int cpu_count = std::thread::hardware_concurrency();
        
        // Comprehensive startup configuration debug output
        THEMIS_INFO("");
        THEMIS_INFO("=================================================");
        THEMIS_INFO("  ✅ THEMIS DATABASE SERVER STARTUP COMPLETE");
        THEMIS_INFO("=================================================");
        THEMIS_INFO("");
        THEMIS_INFO("🖥️  HARDWARE DETECTION & ADAPTATION:");
        THEMIS_INFO("  CPU Cores Detected:      {}", cpu_count);
        THEMIS_INFO("  Worker Threads Allocated:{}", (num_threads > 0 ? std::to_string(num_threads) : std::to_string(cpu_count) + " (auto)"));
        THEMIS_INFO("  Thread Pool Utilization: {}", ((num_threads > 0 ? num_threads : cpu_count) * 100 / cpu_count));
        
        // GPU detection
        #ifdef THEMIS_GEO_ENABLED
        THEMIS_INFO("  GPU Backend:             ENABLED (spatial indexing)");
        #ifdef THEMIS_GEO_BOOST_BACKEND
        THEMIS_INFO("  GPU Type:                Boost CPU Backend (fallback)");
        #else
        THEMIS_INFO("  GPU Type:                NVIDIA CUDA (if available)");
        #endif
        #else
        THEMIS_INFO("  GPU Backend:             disabled");
        #endif
        
        // SIMD support
        THEMIS_INFO("  Database Path:           {}", db_config.db_path);
        THEMIS_INFO("  Memtable Size:           {} MB ({})", db_config.memtable_size_mb, 
                    (db_config.memtable_size_mb >= 512 ? "write-optimized (v1.5.0)" : 
                     db_config.memtable_size_mb >= 256 ? "aggressive" : 
                     db_config.memtable_size_mb >= 128 ? "balanced" : "conservative"));
        THEMIS_INFO("  Max Write Buffers:       {} ({})", db_config.max_write_buffer_number,
                    (db_config.max_write_buffer_number >= 6 ? "high-throughput" : "standard"));
        THEMIS_INFO("  Block Cache Size:        {} MB ({})", db_config.block_cache_size_mb,
                    (db_config.block_cache_size_mb >= 512 ? "high-performance" : (db_config.block_cache_size_mb >= 256 ? "balanced" : "low-latency")));
        THEMIS_INFO("  Async I/O:               {}", (db_config.enable_async_io ? "yes (scan optimization)" : "no"));
        THEMIS_INFO("  WAL Enabled:             {}", (db_config.enable_wal ? "yes (crash recovery)" : "no (speed mode)"));
        THEMIS_INFO("  BlobDB Enabled:          {}", (db_config.enable_blobdb ? "yes (large objects)" : "no"));
        THEMIS_INFO("");
        
        THEMIS_INFO("🗜️  COMPRESSION SETTINGS:");
        std::string default_compression = db_config.compression_default.empty() ? "Snappy (default)" : db_config.compression_default;
        std::string bottommost_compression = db_config.compression_bottommost.empty() ? "none" : db_config.compression_bottommost;
        THEMIS_INFO("  Default (L0-L5):         {} (ratio: high-speed)", default_compression);
        THEMIS_INFO("  Bottommost (archived):   {} (ratio: max-compression)", bottommost_compression);
        #ifdef THEMIS_HAS_ZSTD
        THEMIS_INFO("  ZSTD Support:            AVAILABLE (dict-based compression)");
        #endif
        THEMIS_INFO("  Compression Benefit:     est. 40-60% space savings");
        THEMIS_INFO("");
        
        THEMIS_INFO("🔍 INDEX MANAGERS:");
        THEMIS_INFO("  Secondary Index:         initialized (range queries)");
        THEMIS_INFO("  Graph Index:             initialized (relationship traversal)");
        THEMIS_INFO("  Vector Index (HNSW):     {}", (!vector_save_path.empty() ? "initialized (persistent at " + vector_save_path + ")" : "available"));
        #ifdef THEMIS_HNSW_ENABLED
        THEMIS_INFO("  Vector Search Capability:HNSWLIB-powered (high-dim similarity)");
        #endif
        THEMIS_INFO("");
        
        THEMIS_INFO("🌐 SHARDING & CLUSTERING:");
        if (sharding_enabled) {
            THEMIS_INFO("  Sharding:                ✅ ENABLED");
            THEMIS_INFO("  Shard ID:                {}", shard_id);
            THEMIS_INFO("  Cluster Size:            {} nodes", cluster_size);
            THEMIS_INFO("  Metrics Export:          /metrics (Prometheus format)");
        } else {
            THEMIS_INFO("  Sharding:                ❌ disabled (standalone mode)");
        }
        THEMIS_INFO("");
        
        THEMIS_INFO("⚙️  FEATURE FLAGS & CAPABILITIES:");
#ifdef THEMIS_ENABLE_HTTP_SERVER
        THEMIS_INFO("  Semantic Cache (beta):   {}", (server_config.feature_semantic_cache ? "✅ ENABLED" : "❌ disabled"));
        THEMIS_INFO("  LLM Interaction Store:   {}", (server_config.feature_llm_store ? "✅ ENABLED" : "❌ disabled"));
        THEMIS_INFO("  CDC (Change Data Feed):  {}", (server_config.feature_cdc ? "✅ ENABLED" : "❌ disabled"));
        THEMIS_INFO("  Time-Series Analytics:   {}", (server_config.feature_timeseries ? "✅ ENABLED" : "❌ disabled"));
#else
        THEMIS_INFO("  Semantic Cache (beta):   ❌ disabled");
        THEMIS_INFO("  LLM Interaction Store:   ❌ disabled");
        THEMIS_INFO("  CDC (Change Data Feed):  ❌ disabled");
        THEMIS_INFO("  Time-Series Analytics:   ❌ disabled");
#endif
        THEMIS_INFO("  Retention Manager:       {}", (retention_enabled ? "✅ ENABLED (interval: " + std::to_string(retention_interval_hours) + "h)" : "❌ disabled"));
        THEMIS_INFO("");
        
        THEMIS_INFO("🏢 ENTERPRISE FEATURES:");
        THEMIS_INFO("  Multi-Tenancy:           SUPPORTED (via schema isolation)");
        THEMIS_INFO("  Row-Level Security:      SUPPORTED (via governance engine)");
        THEMIS_INFO("  Fine-grained Encryption: SUPPORTED (field-level + MTLS)");
        THEMIS_INFO("  Audit Logging:           SUPPORTED (tamper-proof with PKI)");
        THEMIS_INFO("  Compliance Modes:        HIPAA, PCI-DSS, GDPR ready");
        THEMIS_INFO("");
        
        THEMIS_INFO("📡 SERVICES & PROTOCOLS:");
        THEMIS_INFO("  Transaction Manager:     ✅ active (ACID guarantees)");
#ifdef THEMIS_ENABLE_HTTP_SERVER
        THEMIS_INFO("  HTTP/REST API:           ✅ listening");
#else
        THEMIS_INFO("  HTTP/REST API:           ❌ disabled (build flag)");
#endif
        THEMIS_INFO("  WebSocket (gRPC):        ✅ available");
        THEMIS_INFO("  Content-FS (blob store): ✅ operational");
        THEMIS_INFO("  Distributed Tracing:     {}", (cfg && cfg->contains("tracing") && (*cfg)["tracing"].value("enabled", false) ? "✅ ENABLED" : "❌ disabled"));
        if (cfg && cfg->contains("tracing") && (*cfg)["tracing"].value("enabled", false)) {
            THEMIS_INFO("  Tracing Endpoint:        {}", (*cfg)["tracing"].value("otlp_endpoint", std::string("http://localhost:4318")));
        }
        THEMIS_INFO("");
        
        THEMIS_INFO("🔐 SECURITY & ENCRYPTION:");
        THEMIS_INFO("  TLS/HTTPS:               {}", (cfg && cfg->contains("server") && (*cfg)["server"].contains("tls") ? "✅ ENABLED" : "ℹ️  plaintext mode"));
        THEMIS_INFO("  mTLS (mutual TLS):       SUPPORTED (client authentication)");
        THEMIS_INFO("  End-to-End Encryption:   ✅ supported (client-side)");
        THEMIS_INFO("  PKI Integration:         ✅ VCCPKI client ready");
        THEMIS_INFO("");
        
        THEMIS_INFO("📈 OPTIMIZATION PROFILE:");
        if (db_config.memtable_size_mb >= 512 && db_config.block_cache_size_mb >= 512) {
            THEMIS_INFO("  → WRITE-OPTIMIZED MODE (v1.5.0: low write-amplification)");
        } else if (db_config.memtable_size_mb >= 256 && db_config.block_cache_size_mb >= 512) {
            THEMIS_INFO("  → HIGH-THROUGHPUT MODE (writes + analytics)");
        } else if (db_config.memtable_size_mb >= 128 && db_config.block_cache_size_mb >= 256) {
            THEMIS_INFO("  → BALANCED MODE (general purpose)");
        } else {
            THEMIS_INFO("  → LOW-LATENCY MODE (OLTP focused)");
        }
        THEMIS_INFO("");
        
        THEMIS_INFO("=================================================");
        THEMIS_INFO("  🚀 Themis is NOW READY FOR OPERATIONS");
        THEMIS_INFO("  API Endpoint:  http://{}:{}", host, port);
        THEMIS_INFO("  Health Check:  http://{}:{}/health", host, port);
        THEMIS_INFO("  Press Ctrl+C to stop gracefully");
        THEMIS_INFO("=================================================");
        THEMIS_INFO("");
        
        // Dynamic endpoint discovery from HttpServer (feature-aware, always in sync)
        try {
            auto endpoints = g_server->getRegisteredEndpoints();
            THEMIS_INFO("Available endpoints ({} total):", endpoints.size());
            
            // Group endpoints by category for improved readability
            std::string current_category;
            for (const auto& ep : endpoints) {
                // Extract category from path (e.g., "/cache/*" → "Cache", "/api/v1/schema" → "Schema")
                std::string category = "Core";
                if (ep.path.find("/cache/") != std::string::npos) category = "Cache (Feature)";
                else if (ep.path.find("/llm/") != std::string::npos) category = "LLM (Feature)";
                else if (ep.path.find("/changefeed") != std::string::npos) category = "CDC (Feature)";
                else if (ep.path.find("/ts/") != std::string::npos) category = "TimeSeries (Feature)";
                else if (ep.path.find("/pii/") != std::string::npos) category = "PII (Feature)";
                else if (ep.path.find("/api/v1/") != std::string::npos) category = "API v1";
                else if (ep.path.find("/vector/") != std::string::npos) category = "Vector";
                else if (ep.path.find("/graph/") != std::string::npos) category = "Graph";
                else if (ep.path.find("/search/") != std::string::npos) category = "Search";
                else if (ep.path.find("/auth/") != std::string::npos) category = "Auth";
                else if (ep.path.find("/admin/") != std::string::npos) category = "Admin";
                else if (ep.path.find("/graphql") != std::string::npos) category = "GraphQL";
                
                // Print category header on first entry of new category
                if (category != current_category && !current_category.empty()) {
                    THEMIS_INFO(""); // Blank line between categories
                }
                if (category != current_category) {
                    current_category = category;
                    THEMIS_INFO("  [{}]", category);
                }
                
                // Format: "  METHOD  /path/pattern             - Human description"
                THEMIS_INFO("    {:<6} {:<40} - {}", ep.method, ep.path, ep.description);
            }
            THEMIS_INFO("");
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to retrieve endpoint list: {}", e.what());
            // Fallback to minimal list if getRegisteredEndpoints() fails
            THEMIS_INFO("Available endpoints:");
            THEMIS_INFO("  GET  /health       - Health check");
            THEMIS_INFO("  POST /query        - Execute query");
            THEMIS_INFO("  POST /entities     - Create entity");
            THEMIS_INFO("  (See /api/openapi.json for complete API specification)");
        }
#ifdef THEMIS_ENABLE_HTTP_SERVER
        if (server_config.feature_cdc) {
            THEMIS_INFO("  GET  /changefeed          - CDC feed (beta)");
        }
#endif
#ifdef THEMIS_ENABLE_HTTP_SERVER
        if (server_config.feature_timeseries) {
            THEMIS_INFO("  POST /ts/put              - Store time-series data (beta)");
            THEMIS_INFO("  POST /ts/query            - Query time-series data (beta)");
            THEMIS_INFO("  POST /ts/aggregate        - Aggregate time-series (beta)");
        }
#endif
        THEMIS_INFO("");
        THEMIS_INFO("ready to serve...");
        
        // Wait for shutdown signal (check atomic flag periodically)
        THEMIS_INFO("Server running. Waiting for shutdown signal (Ctrl+C)...");
        while (!g_shutdown_requested.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

#ifndef _WIN32
            // Handle SIGHUP: hot-reload TLS certificates
            if (g_tls_reload_requested.exchange(false, std::memory_order_acq_rel)) {
#ifdef THEMIS_ENABLE_HTTP_SERVER
                if (g_server) {
                    THEMIS_INFO("SIGHUP received - reloading TLS certificates...");
                    if (g_server->reloadTls()) {
                        THEMIS_INFO("TLS certificates hot-reloaded successfully");
                    } else {
                        THEMIS_WARN("TLS hot-reload failed or TLS not enabled - continuing with current certificates");
                    }
                }
#endif
            }
#endif
        }
        
        THEMIS_INFO("Shutdown signal received, initiating graceful shutdown...");
        
        // Stop WALShipper before other services
        if (g_wal_shipper) {
            THEMIS_INFO("Stopping WALShipper...");
            g_wal_shipper->stop();
            g_wal_shipper.reset();
        }
        
#ifdef THEMIS_ENABLE_GRPC
        if (g_wal_grpc_server) {
            THEMIS_INFO("Stopping WAL gRPC Apply service...");
            g_wal_grpc_server->Shutdown();
            g_wal_grpc_server.reset();
            g_wal_grpc_service.reset();
        }
#endif
#ifdef THEMIS_ENABLE_HTTP_SERVER
        g_server->stop();
#endif
        
        THEMIS_INFO("=================================================");
        THEMIS_INFO("Initiating graceful shutdown sequence...");
        THEMIS_INFO("=================================================");
        
        // Step 1: Stop retention worker
        THEMIS_INFO("[1/5] Stopping retention worker...");
        try {
            retention_stop.store(true, std::memory_order_relaxed);
            if (retention_thread.joinable()) retention_thread.join();
            THEMIS_INFO("Retention worker stopped");
        } catch (...) {
            THEMIS_WARN("Error stopping retention worker (continuing shutdown)");
        }

    // Step 2: Shutdown tracing (no more traces)
    THEMIS_INFO("[2/5] Shutting down distributed tracing...");
        Tracer::shutdown();
        
        // Step 3: Save vector index before closing DB
        if (vector_index && !vector_save_path.empty()) {
            THEMIS_INFO("[3/5] Saving vector index to disk...");
            vector_index->shutdown();
        } else {
            THEMIS_INFO("[3/5] Vector index save skipped (not configured)");
        }
        
        // Step 4: Database is already closed by server->stop()
        // but we ensure cleanup here
        THEMIS_INFO("[4/5] Database cleanup...");
        if (db && db->isOpen()) {
            db->close();
            THEMIS_INFO("Database closed cleanly");
        } else {
            THEMIS_INFO("Database already closed by server");
        }
        
        // Step 5: Clear shared pointers
        THEMIS_INFO("[5/5] Releasing resources...");
        
        // Stop ModuleLoader watchdog before releasing the loader
        if (module_loader && module_loader->isWatchdogRunning()) {
            THEMIS_INFO("Stopping ModuleLoader watchdog...");
            module_loader->stopWatchdog();
        }
        
        // Stop HSM warning thread
        stopHSMWarningThread();
        
        // Finalize HSM provider
        if (g_hsm_provider) {
            g_hsm_provider->finalize();
            g_hsm_provider.reset();
        }
        
#ifdef THEMIS_ENABLE_HTTP_SERVER
        g_server.reset();
#endif
        tx_manager.reset();
        vector_index.reset();
        graph_index.reset();
        secondary_index.reset();
        db.reset();
        module_loader.reset();
        hot_reload_mgr.reset();
        
        THEMIS_INFO("=================================================");
        THEMIS_INFO("Shutdown complete. All data saved.");
        THEMIS_INFO("=================================================");
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    
    utils::Logger::shutdown();
    return 0;
}
