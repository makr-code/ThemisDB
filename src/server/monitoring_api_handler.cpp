/*
 * ThemisDB | File: monitoring_api_handler.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 1767
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=310 | delta=307 | status=divergent
 * External Severity (v3): C=5, H=213, M=92
 * PR: #1107 Add HSM security warning system (FIND-002) (2026-03-11T17:52:26Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/monitoring_api_handler.h"
#include <stdexcept>
#include "server/openapi_route_registry.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "server/auth_middleware.h"
#include "server/api_version.h"
#include "server/sharding_metrics_handler.h"
#include "metadata/schema_manager.h"
#include "plugins/plugin_manager.h"
#include "security/hsm_provider.h"
#include "security/hsm_security_metrics.h"
#include "themis/build_info.h"
#include "themis/license_info.h"
#include "themis/runtime_license_gate.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "observability/metrics_collector.h"
#include "config/config_metrics_exporter.h"
#include <ctime>
#include <sstream>
#ifndef _WIN32
#include <sys/resource.h>
#endif

// External reference to global HSM provider (defined in main_server.cpp)
extern std::shared_ptr<themis::security::HSMProvider> g_hsm_provider;

namespace themis {
namespace server {

using json = nlohmann::json;

MonitoringApiHandler::MonitoringApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<::themis::AuthMiddleware> auth,
    std::atomic<uint64_t>* request_count,
    std::atomic<uint64_t>* error_count,
    const std::chrono::steady_clock::time_point* start_time,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    ::themis::SchemaManager* schema_manager,
    std::shared_ptr<ShardingMetricsHandler> sharding_metrics,
    const std::atomic<bool>* is_running,
    const std::atomic<uint64_t>* active_requests,
    const std::atomic<uint64_t>* active_connections,
    std::shared_ptr<core::concerns::ConcernsContext> concerns
)
    : storage_(std::move(storage))
    , auth_(std::move(auth))
    , request_count_(request_count)
    , error_count_(error_count)
    , start_time_(start_time)
    , secondary_index_(std::move(secondary_index))
    , schema_manager_(schema_manager)
    , sharding_metrics_(std::move(sharding_metrics))
    , is_running_(is_running)
    , active_requests_(active_requests)
    , active_connections_(active_connections)
    , concerns_(std::move(concerns))
{
}

http::response<http::string_body> MonitoringApiHandler::handleHealthCheck(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleHealthCheck");
    // Implementation moved from http_server.cpp handleHealthCheck()
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - *start_time_
    ).count();
    
    json response = {
        {"status", "healthy"},
        {"version", "0.1.0"},
        {"database", "themis"},
        {"uptime_seconds", uptime_seconds}
    };
    
    // Add license information if available
    auto license = themis::license::getEmbeddedLicense();
    if (license) {
        // Mask license key for security (show only first 8 chars)
        std::string masked_key = license->license_key;
        if (masked_key.length() > 8) {
            masked_key = masked_key.substr(0, 8) + "...";
        }
        
        response["license"] = {
            {"organization", license->organization_name},
            {"edition", license->edition},
            {"license_key", masked_key},  // Masked for security
            {"valid", themis::license::isLicenseValid(*license)},
            {"days_until_expiry", themis::license::getDaysUntilExpiry(*license)}
        };
    }
    
    return makeResponse(http::status::ok, response.dump(), req);
}

http::response<http::string_body> MonitoringApiHandler::handleLiveness(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleLiveness");
    // Liveness probe: server process is running and not deadlocked
    bool alive = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);

    json checks = {
        {"server_running", alive}
    };

    // --- Core concerns health (logger, tracer, metrics, cache) ---
    if (concerns_) {
        auto health = concerns_->healthCheck();
        checks["concerns"] = buildConcernsJson(health, alive);
    }

    json response = {
        {"status", alive ? "alive" : "dead"},
        {"checks", checks}
    };

    auto status = alive ? http::status::ok : http::status::service_unavailable;
    return makeResponse(status, response.dump(), req);
}

http::response<http::string_body> MonitoringApiHandler::handleReadiness(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleReadiness");
    // Readiness probe: server is ready to accept traffic.
    // Reports per-layer health: server state, storage, connections, memory.
    bool server_running = (is_running_ == nullptr) || is_running_->load(std::memory_order_relaxed);

    // --- Layer 1: Storage ---
    bool storage_ok = false;
    std::string storage_error;
    if (storage_) {
        try {
            storage_ok = (storage_->getRawDB() != nullptr);
        } catch (const std::exception& e) {
            storage_error = e.what();
            storage_ok = false;
        }
    } else {
        // No storage configured - not a readiness failure for lightweight deployments
        storage_ok = true;
    }

    // --- Layer 2: Active connections ---
    uint64_t conn_count = active_connections_
        ? active_connections_->load(std::memory_order_relaxed) : 0;
    uint64_t req_count = active_requests_
        ? active_requests_->load(std::memory_order_relaxed) : 0;

    // --- Layer 3: Memory (RSS) via getrusage (Linux/macOS only) ---
    int64_t rss_bytes = -1;
#ifndef _WIN32
    struct rusage ru{};
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
#ifdef __APPLE__
        // macOS: ru_maxrss is bytes
        rss_bytes = static_cast<int64_t>(ru.ru_maxrss);
#else
        // Linux: ru_maxrss is kilobytes
        rss_bytes = static_cast<int64_t>(ru.ru_maxrss) * 1024;
#endif
    }
#endif

    bool ready = server_running && storage_ok;

    json checks = {
        {"server_running", server_running},
        {"storage_available", storage_ok},
        {"active_connections", conn_count},
        {"active_requests", req_count}
    };

    if (!storage_error.empty()) {
        checks["storage_error"] = storage_error;
    }

    if (rss_bytes >= 0) {
        checks["memory_rss_bytes"] = rss_bytes;
    }

    // --- Layer 4: Core concerns readiness (logger, tracer, metrics, cache) ---
    if (concerns_) {
        auto readiness = concerns_->readinessCheck();
        checks["concerns"] = buildConcernsJson(readiness, ready);
    }

    json response = {
        {"status", ready ? "ready" : "not_ready"},
        {"checks", checks}
    };

    auto status = ready ? http::status::ok : http::status::service_unavailable;
    return makeResponse(status, response.dump(), req);
}

http::response<http::string_body> MonitoringApiHandler::handleVersion(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleVersion");
    // Implementation moved from http_server.cpp handleVersion()
    try {
        auto build_config = themis::build_info::getBuildConfiguration();
        
        json response = {
            {"edition", {
                {"name", build_config.edition_name},
                {"type", build_config.edition_type},
                {"gpu_max_vram_gb", build_config.gpu_max_vram_gb},
                {"sharding_max_nodes", build_config.sharding_max_nodes}
            }},
            {"build", {
                {"compiler", build_config.compiler},
                {"compiler_version", build_config.compiler_version},
                {"build_type", build_config.build_type},
                {"timestamp", build_config.build_timestamp}
            }}
        };
        
#ifdef THEMIS_VERSION_STRING
        response["version"] = THEMIS_VERSION_STRING;
#else
        response["version"] = "unknown";
#endif

        // Add embedded license information if available
        auto license = themis::license::getEmbeddedLicense();
        if (license) {
            // Mask license key for security (show only first 8 chars)
            std::string masked_key = license->license_key;
            if (masked_key.length() > 8) {
                masked_key = masked_key.substr(0, 8) + "...";
            }
            
            response["license"] = {
                {"organization_name", license->organization_name},
                {"organization_id", license->organization_id},
                {"contact_email", license->contact_email},
                {"license_key", masked_key},  // Masked for security
                {"edition", license->edition},
                {"issued_date", license->issued_date},
                {"expiry_date", license->expiry_date},
                {"valid", themis::license::isLicenseValid(*license)},
                {"days_until_expiry", themis::license::getDaysUntilExpiry(*license)},
                {"limits", {
                    {"max_nodes", license->max_nodes},
                    {"max_cores", license->max_cores},
                    {"max_storage_tb", license->max_storage_tb}
                }},
                {"build_id", license->build_id},
                {"build_timestamp", license->build_timestamp}
            };
            
            // Add signature status if present
            if (!license->signature.empty()) {
                response["license"]["signature_valid"] = themis::license::verifyLicenseSignature(*license);
            }
        }
        
        // Add module information
        json modules_compiled = json::array();
        json modules_disabled = json::array();
        
        for (const auto& mod : build_config.modules) {
            json module_info = {
                {"name", mod.name},
                {"description", mod.description}
            };
            
            if (mod.compiled_in) {
                modules_compiled.push_back(module_info);
            } else {
                modules_disabled.push_back(module_info);
            }
        }
        
        response["modules"] = {
            {"compiled_in", modules_compiled},
            {"not_compiled", modules_disabled},
            {"total", build_config.modules.size()},
            {"compiled_count", modules_compiled.size()},
            {"disabled_count", modules_disabled.size()}
        };

        // Add API versioning information (supported versions, deprecation policy).
        // Use a static instance to avoid re-constructing (and re-logging) on every request.
        {
            static const APIVersionManager version_mgr;
            auto current = version_mgr.getCurrentVersion();
            auto minimum = version_mgr.getMinimumVersion();

            response["api_version"] = {
                {"major", current.major},
                {"minor", current.minor},
                {"patch", current.patch},
                {"string", current.toString()}
            };

            response["api_minimum_version"] = {
                {"major", minimum.major},
                {"minor", minimum.minor},
                {"patch", minimum.patch},
                {"string", minimum.toString()}
            };

            json supported = json::array();
            for (const auto& v : version_mgr.getSupportedVersions()) {
                supported.push_back({
                    {"major", v.major},
                    {"minor", v.minor},
                    {"patch", v.patch}
                });
            }
            response["supported_api_versions"] = supported;
        }
        
        return makeResponse(http::status::ok, response.dump(2), req);
    } catch (const std::exception& e) {
        json error_response = {
            {"error", "Failed to retrieve version information"},
            {"message", e.what()}
        };
        return makeResponse(http::status::internal_server_error, error_response.dump(), req);
    }
}

#ifdef _MSC_VER
#pragma optimize("", off)
#endif
http::response<http::string_body> MonitoringApiHandler::handleOpenApi(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleOpenApi");
    // Assemble an OpenAPI 3.1.0 document from all routes registered via
    // RouteRegistry.  registerRoutes() is called here so that monitoring-handler
    // routes are always present even if the caller did not invoke it explicitly
    // at startup.
    try {
        std::string api_version;
#ifdef THEMIS_VERSION_STRING
        api_version = THEMIS_VERSION_STRING;
#else
        api_version = "0.1.0";
#endif

        // Lazy-register monitoring routes (idempotent: last-write-wins).
        registerRoutes();

        json spec = RouteRegistry::instance().buildOpenApiSpec(api_version);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = spec.dump(2);
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

http::response<http::string_body> MonitoringApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleStats");
    // Implementation moved from http_server.cpp handleStats()
    try {
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - *start_time_
        ).count();
        
        uint64_t total_requests = request_count_->load(std::memory_order_relaxed);
        uint64_t total_errors = error_count_->load(std::memory_order_relaxed);
        double qps = uptime_seconds > 0 ? static_cast<double>(total_requests) / uptime_seconds : 0.0;
        
        // Get RocksDB stats (returns JSON string)
        std::string rocksdb_stats = storage_->getStats();
        
        // Parse RocksDB JSON
        json rocksdb_json;
        try {
            rocksdb_json = json::parse(rocksdb_stats);
        } catch (const std::exception&) {
            rocksdb_json = {{"error", "Failed to parse RocksDB stats"}};
        }
        
        // Build complete stats response
        json response = {
            {"server", {
                {"uptime_seconds", uptime_seconds},
                {"total_requests", total_requests},
                {"total_errors", total_errors},
                {"queries_per_second", qps}
            }},
            {"storage", rocksdb_json}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req); // Pretty print with indent 2
    } catch (const std::exception& e) {
        error_count_->fetch_add(1, std::memory_order_relaxed);
        return makeErrorResponse(http::status::internal_server_error, 
                                 std::string("Failed to get stats: ") + e.what(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::handleCapabilities(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleCapabilities");
    // Implementation moved from http_server.cpp handleCapabilities()
    // No auth required for capabilities (read-only, non-sensitive)
    json caps;
    
    // Add edition and build information
    try {
        auto build_config = themis::build_info::getBuildConfiguration();
        caps["edition"] = {
            {"name", build_config.edition_name},
            {"type", build_config.edition_type},
            {"gpu_max_vram_gb", build_config.gpu_max_vram_gb},
            {"sharding_max_nodes", build_config.sharding_max_nodes}
        };
        
        caps["build"] = {
            {"compiler", build_config.compiler},
            {"version", build_config.compiler_version},
            {"type", build_config.build_type}
        };
    } catch (const std::exception&) {
        // If build info fails, continue with basic capabilities
    }

    // Build flags
#ifdef THEMIS_GEO_ENABLED
    const bool geo_enabled = true;
#else
    const bool geo_enabled = false;
#endif
#ifdef THEMIS_GEO_SIMD_ENABLED
    const bool geo_simd = true;
#else
    const bool geo_simd = false;
#endif
#ifdef THEMIS_GEO_GPU_ENABLED
    const bool geo_gpu = true;
#else
    const bool geo_gpu = false;
#endif
#ifdef THEMIS_GEO_H3_ENABLED
    const bool geo_h3 = true;
#else
    const bool geo_h3 = false;
#endif
#ifdef THEMIS_GEO_GEOS_PLUGIN_ENABLED
    const bool geo_geos = true;
#else
    const bool geo_geos = false;
#endif
#ifdef THEMIS_ENTERPRISE_ENABLED
    const bool enterprise = true;
#else
    const bool enterprise = false;
#endif

#ifdef THEMIS_GPU_ENABLED
    const bool vector_gpu = true;
#else
    const bool vector_gpu = false;
#endif

    caps["geo"] = {
        {"enabled", geo_enabled},
        {"enterprise_compiled", enterprise},
        {"accel", {
            {"simd_compiled", geo_simd},
            {"gpu_compiled", geo_gpu}
        }},
        {"plugins_compiled", {
            {"geos", geo_geos},
            {"h3", geo_h3}
        }}
    };

    caps["vector"] = {
        {"gpu_compiled", vector_gpu}
    };

    // Server basics
    caps["server"] = {
        {"version", "1.0.0"}
    };

    // Schema awareness (if available)
    if (schema_manager_) {
        try {
            auto schema_caps = schema_manager_->getCapabilitiesJSON();
            if (schema_caps.contains("capabilities")) {
                caps["schema_awareness"] = {
                    {"enabled", true},
                    {"capabilities", schema_caps["capabilities"]}
                };
            }
        } catch (const std::exception&) {
            // If schema manager fails, continue without schema capabilities
            caps["schema_awareness"] = {
                {"enabled", false}
            };
        }
    } else {
        caps["schema_awareness"] = {
            {"enabled", false}
        };
    }

    return makeResponse(http::status::ok, caps.dump(), req);
}

http::response<http::string_body> MonitoringApiHandler::handleMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleMetrics");
    // Basic Prometheus metrics implementation
    // Includes core server metrics, auth metrics, RocksDB stats, and index rebuild metrics
    // Note: Some advanced metrics (latency histograms, vector index, SSE, rate limiter, sharding)
    // require additional dependencies not included in this basic handler
    
    try {
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - *start_time_
        ).count();

        uint64_t total_requests = request_count_->load(std::memory_order_relaxed);
        uint64_t total_errors = error_count_->load(std::memory_order_relaxed);
        double qps = uptime_seconds > 0 ? static_cast<double>(total_requests) / uptime_seconds : 0.0;

        // Parse RocksDB stats JSON
        json rdb;
        try {
            rdb = json::parse(storage_->getStats());
        } catch (const std::exception&) {
            rdb = json::object();
        }
        json r = rdb.contains("rocksdb") ? rdb["rocksdb"] : json::object();

        auto get_u64 = [&](const char* k) -> uint64_t {
            if (r.contains(k) && r[k].is_number_unsigned()) return r[k].get<uint64_t>();
            if (r.contains(k) && r[k].is_number_integer()) return static_cast<uint64_t>(r[k].get<int64_t>());
            return 0ull;
        };
        uint64_t block_cache_usage = get_u64("block_cache_usage_bytes");
        uint64_t block_cache_capacity = get_u64("block_cache_capacity_bytes");
        uint64_t estimate_keys = get_u64("estimate_num_keys");
        uint64_t pending_compaction = get_u64("estimate_pending_compaction_bytes");
        uint64_t memtable_bytes = get_u64("memtable_size_bytes");

        std::string out;
        out.reserve(4096);

        // themis_build_info – static info metric with version/build labels
        {
            std::string version;
#ifdef THEMIS_VERSION_STRING
            version = THEMIS_VERSION_STRING;
#else
            version = "unknown";
#endif
            auto build_cfg = themis::build_info::getBuildConfiguration();
            // Sanitize label values: replace '"' and '\n' with '_'
            auto sanitize = [](std::string s) -> std::string {
                for (auto& c : s) { if (c == '"' || c == '\n') c = '_'; }
                return s;
            };
            out += "# HELP themis_build_info ThemisDB build information\n";
            out += "# TYPE themis_build_info gauge\n";
            out += "themis_build_info{";
            out += "version=\""     + sanitize(version)                  + "\",";
            out += "build_type=\""  + sanitize(build_cfg.build_type)     + "\",";
            out += "compiler=\""    + sanitize(build_cfg.compiler)       + "\",";
            out += "edition=\""     + sanitize(build_cfg.edition_name)   + "\"";
            out += "} 1\n\n";
        }

        out += "# HELP process_uptime_seconds Process uptime in seconds\n";
        out += "# TYPE process_uptime_seconds gauge\n";
        out += "process_uptime_seconds " + std::to_string(uptime_seconds) + "\n";

        out += "# HELP vccdb_requests_total Total HTTP requests handled\n";
        out += "# TYPE vccdb_requests_total counter\n";
        out += "vccdb_requests_total " + std::to_string(total_requests) + "\n";

        out += "# HELP vccdb_errors_total Total HTTP errors returned\n";
        out += "# TYPE vccdb_errors_total counter\n";
        out += "vccdb_errors_total " + std::to_string(total_errors) + "\n";

        out += "# HELP vccdb_qps Queries per second (approx)\n";
        out += "# TYPE vccdb_qps gauge\n";
        out += "vccdb_qps " + std::to_string(qps) + "\n";

        // Auth metrics
        if (auth_) {
            const auto& m = auth_->getMetrics();
            out += "# HELP themis_authz_success_total Successful authorizations\n";
            out += "# TYPE themis_authz_success_total counter\n";
            out += "themis_authz_success_total " + std::to_string(m.authz_success_total.load()) + "\n";
            out += "# HELP themis_authz_denied_total Denied authorizations (forbidden)\n";
            out += "# TYPE themis_authz_denied_total counter\n";
            out += "themis_authz_denied_total " + std::to_string(m.authz_denied_total.load()) + "\n";
            out += "# HELP themis_authz_invalid_token_total Invalid or missing tokens\n";
            out += "# TYPE themis_authz_invalid_token_total counter\n";
            out += "themis_authz_invalid_token_total " + std::to_string(m.authz_invalid_token_total.load()) + "\n";
        }

        out += "# HELP rocksdb_block_cache_usage_bytes RocksDB block cache usage in bytes\n";
        out += "# TYPE rocksdb_block_cache_usage_bytes gauge\n";
        out += "rocksdb_block_cache_usage_bytes " + std::to_string(block_cache_usage) + "\n";

        out += "# HELP rocksdb_block_cache_capacity_bytes RocksDB block cache capacity in bytes\n";
        out += "# TYPE rocksdb_block_cache_capacity_bytes gauge\n";
        out += "rocksdb_block_cache_capacity_bytes " + std::to_string(block_cache_capacity) + "\n";

        out += "# HELP rocksdb_estimate_num_keys Estimated number of keys in DB\n";
        out += "# TYPE rocksdb_estimate_num_keys gauge\n";
        out += "rocksdb_estimate_num_keys " + std::to_string(estimate_keys) + "\n";

        out += "# HELP rocksdb_pending_compaction_bytes Estimated pending compaction bytes\n";
        out += "# TYPE rocksdb_pending_compaction_bytes gauge\n";
        out += "rocksdb_pending_compaction_bytes " + std::to_string(pending_compaction) + "\n";

        out += "# HELP rocksdb_memtable_size_bytes Current memtable size in bytes\n";
        out += "# TYPE rocksdb_memtable_size_bytes gauge\n";
        out += "rocksdb_memtable_size_bytes " + std::to_string(memtable_bytes) + "\n";

        if (r.contains("files_per_level") && r["files_per_level"].is_object()) {
            for (auto it = r["files_per_level"].begin(); it != r["files_per_level"].end(); ++it) {
                std::string level = it.key();
                uint64_t val = 0;
                if (it.value().is_number_integer()) val = static_cast<uint64_t>(it.value().get<int64_t>());
                else if (it.value().is_number_unsigned()) val = it.value().get<uint64_t>();
                out += "rocksdb_files_level{level=\"" + level + "\"} " + std::to_string(val) + "\n";
            }
        }

        // Index rebuild metrics
        if (secondary_index_) {
            auto& rebuild_metrics = secondary_index_->getRebuildMetrics();
            uint64_t rebuild_count = rebuild_metrics.rebuild_count.load(std::memory_order_relaxed);
            uint64_t rebuild_duration_ms = rebuild_metrics.rebuild_duration_ms.load(std::memory_order_relaxed);
            uint64_t rebuild_entities = rebuild_metrics.rebuild_entities_processed.load(std::memory_order_relaxed);
            
            out += "# HELP themis_index_rebuilds_total Total number of index rebuilds performed\n";
            out += "# TYPE themis_index_rebuilds_total counter\n";
            out += "themis_index_rebuilds_total " + std::to_string(rebuild_count) + "\n";
            
            out += "# HELP themis_index_rebuild_duration_milliseconds_total Total duration of all index rebuilds in milliseconds\n";
            out += "# TYPE themis_index_rebuild_duration_milliseconds_total counter\n";
            out += "themis_index_rebuild_duration_milliseconds_total " + std::to_string(rebuild_duration_ms) + "\n";
            
            out += "# HELP themis_index_rebuild_entities_total Total number of entities processed during index rebuilds\n";
            out += "# TYPE themis_index_rebuild_entities_total counter\n";
            out += "themis_index_rebuild_entities_total " + std::to_string(rebuild_entities) + "\n";

            // Query metrics from SecondaryIndexManager
            auto& qmetrics = secondary_index_->getQueryMetrics();
            uint64_t cursor_anchor_hits = qmetrics.cursor_anchor_hits_total.load(std::memory_order_relaxed);
            uint64_t range_scan_steps = qmetrics.range_scan_steps_total.load(std::memory_order_relaxed);
            out += "# HELP themis_cursor_anchor_hits_total Total number of cursor anchor usages in ORDER BY pagination\n";
            out += "# TYPE themis_cursor_anchor_hits_total counter\n";
            out += "themis_cursor_anchor_hits_total " + std::to_string(cursor_anchor_hits) + "\n";
            out += "# HELP themis_range_scan_steps_total Total index scan steps performed during range scans\n";
            out += "# TYPE themis_range_scan_steps_total counter\n";
            out += "themis_range_scan_steps_total " + std::to_string(range_scan_steps) + "\n";
        }
        
        // Plugin metrics
        try {
            auto& plugin_manager = themis::plugins::PluginManager::instance();
            auto all_stats = plugin_manager.getMetrics().getAllStats();
            
            if (!all_stats.empty()) {
                out += "\n# HELP themis_plugin_loads_total Total number of plugin loads\n";
                out += "# TYPE themis_plugin_loads_total counter\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    out += "themis_plugin_loads_total{plugin=\"" + plugin_name + "\"} 1\n";
                }
                
                out += "\n# HELP themis_plugin_reloads_total Total number of plugin reloads\n";
                out += "# TYPE themis_plugin_reloads_total counter\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    out += "themis_plugin_reloads_total{plugin=\"" + plugin_name + "\"} " 
                         + std::to_string(stats.reload_count) + "\n";
                }
                
                out += "\n# HELP themis_plugin_errors_total Total number of plugin errors\n";
                out += "# TYPE themis_plugin_errors_total counter\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    out += "themis_plugin_errors_total{plugin=\"" + plugin_name + "\"} " 
                         + std::to_string(stats.errors) + "\n";
                }
                
                out += "\n# HELP themis_plugin_function_calls_total Total number of plugin function calls\n";
                out += "# TYPE themis_plugin_function_calls_total counter\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    out += "themis_plugin_function_calls_total{plugin=\"" + plugin_name + "\"} " 
                         + std::to_string(stats.function_calls) + "\n";
                }
                
                out += "\n# HELP themis_plugin_load_duration_seconds Plugin load duration\n";
                out += "# TYPE themis_plugin_load_duration_seconds histogram\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    double load_seconds = stats.load_time.count() / 1000.0;
                    out += "themis_plugin_load_duration_seconds_sum{plugin=\"" + plugin_name + "\"} " 
                         + std::to_string(load_seconds) + "\n";
                    out += "themis_plugin_load_duration_seconds_count{plugin=\"" + plugin_name + "\"} 1\n";
                }
                
                out += "\n# HELP themis_plugin_memory_bytes Plugin memory usage in bytes\n";
                out += "# TYPE themis_plugin_memory_bytes gauge\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    out += "themis_plugin_memory_bytes{plugin=\"" + plugin_name + "\"} " 
                         + std::to_string(stats.memory_bytes) + "\n";
                }
                
                out += "\n# HELP themis_plugin_call_latency_milliseconds Plugin call latency metrics\n";
                out += "# TYPE themis_plugin_call_latency_milliseconds summary\n";
                for (const auto& [plugin_name, stats] : all_stats) {
                    if (stats.function_calls > 0) {
                        out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name 
                             + "\",quantile=\"0.95\"} " + std::to_string(stats.p95_call_latency_ms) + "\n";
                        out += "themis_plugin_call_latency_milliseconds{plugin=\"" + plugin_name 
                             + "\",quantile=\"0.99\"} " + std::to_string(stats.p99_call_latency_ms) + "\n";
                        out += "themis_plugin_call_latency_milliseconds_sum{plugin=\"" + plugin_name 
                             + "\"} " + std::to_string(stats.sum_call_latency_ms) + "\n";
                        out += "themis_plugin_call_latency_milliseconds_count{plugin=\"" + plugin_name 
                             + "\"} " + std::to_string(stats.function_calls) + "\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            // If plugin metrics fail, log and continue without them
            THEMIS_WARN("Failed to collect plugin metrics: {}", e.what());
        } catch (const std::exception&) {
            // Catch any other exceptions to prevent metrics collection from breaking /metrics endpoint
            THEMIS_WARN("Unknown error while collecting plugin metrics");
        }
        
        // Add distributed tracing metrics
        try {
            out += "\n# HELP themis_trace_spans_total Total number of trace spans created\n";
            out += "# TYPE themis_trace_spans_total counter\n";
            out += "themis_trace_spans_total " + std::to_string(Tracer::getTotalSpans()) + "\n";
            
            out += "# HELP themis_trace_active_spans Number of currently active spans\n";
            out += "# TYPE themis_trace_active_spans gauge\n";
            out += "themis_trace_active_spans " + std::to_string(Tracer::getActiveSpans()) + "\n";
        } catch (const std::exception&) {
            // If tracing metrics fail, log and continue
            THEMIS_WARN("Failed to collect tracing metrics");
        }
        
        // HSM Security Metrics (FIND-002)
        try {
            if (g_hsm_provider) {
                std::string hsm_metrics = themis::security::HSMSecurityMetrics::exportMetrics(*g_hsm_provider);
                out += "\n# === HSM Security Metrics ===\n";
                out += hsm_metrics;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to collect HSM security metrics: {}", e.what());
        } catch (const std::exception&) {
            THEMIS_WARN("Unknown error while collecting HSM security metrics");
        }

        // Config path resolution metrics (hit rate, miss rate, legacy fallback rate)
        try {
            std::string config_metrics = themis::config::ConfigMetricsExporter::collect();
            if (!config_metrics.empty()) {
                out += "\n# === Config Path Resolution Metrics ===\n";
                out += config_metrics;
            }
            // Also sync into MetricsCollector so Grafana dashboard gauges stay current
            themis::config::ConfigMetricsExporter::updateMetricsCollector();
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to collect config path resolution metrics: {}", e.what());
        } catch (const std::exception&) {
            THEMIS_WARN("Unknown error while collecting config path resolution metrics");
        }

        // Append metrics from the central MetricsCollector (query latency, cache,
        // TSStore writes, sharding, security, tracing spans, etc.)
        try {
            std::string collector_metrics =
                observability::MetricsCollector::getInstance().getPrometheusMetrics();
            if (!collector_metrics.empty()) {
                out += "\n# === ThemisDB Subsystem Metrics (MetricsCollector) ===\n";
                out += collector_metrics;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to collect subsystem metrics: {}", e.what());
        } catch (const std::exception&) {
            THEMIS_WARN("Unknown error while collecting subsystem metrics");
        }

        // Return Prometheus format response
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "text/plain; version=0.0.4");
        res.keep_alive(req.keep_alive());
        res.body() = out;
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Failed to generate metrics: ") + e.what(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::handlePluginMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handlePluginMetrics");
    // GET /api/plugins/metrics - Return plugin metrics in JSON format
    try {
        auto& plugin_manager = themis::plugins::PluginManager::instance();
        auto all_stats = plugin_manager.getMetrics().getAllStats();
        
        json response;
        
        for (const auto& [plugin_name, stats] : all_stats) {
            // Convert loaded_at to ISO 8601 string
            auto time_t_val = std::chrono::system_clock::to_time_t(stats.loaded_at);
            std::tm tm_val;
            #ifdef _WIN32
                gmtime_s(&tm_val, &time_t_val);
            #else
                gmtime_r(&time_t_val, &tm_val);
            #endif
            char time_buf[32];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);
            
            response[plugin_name] = {
                {"load_time_ms", stats.load_time.count()},
                {"last_reload_ms", stats.last_reload_time.count()},
                {"loaded_at", time_buf},
                {"reload_count", stats.reload_count},
                {"function_calls", stats.function_calls},
                {"errors", stats.errors},
                {"memory_bytes", stats.memory_bytes},
                {"avg_latency_ms", stats.avg_call_latency_ms},
                {"p95_latency_ms", stats.p95_call_latency_ms},
                {"p99_latency_ms", stats.p99_call_latency_ms}
            };
        }
        
        return makeResponse(http::status::ok, response.dump(2), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, 
                                 std::string("Failed to get plugin metrics: ") + e.what(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> MonitoringApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

// ==================== Phase 1.5: Sharding and SLO Endpoints ====================

http::response<http::string_body> MonitoringApiHandler::handleShardingMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleShardingMetrics");
    if (!sharding_metrics_) {
        return makeErrorResponse(http::status::service_unavailable, 
                                 "Sharding metrics not configured", req);
    }
    
    try {
        std::string metrics = sharding_metrics_->getMetrics();
        
        // Also include SLO metrics if available
        std::string slo_metrics = sharding_metrics_->getSLOMetrics();
        if (!slo_metrics.empty()) {
            metrics += "\n" + slo_metrics;
        }
        
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8");
        res.keep_alive(req.keep_alive());
        res.body() = metrics;
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, 
                                 std::string("Failed to get sharding metrics: ") + e.what(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::handleSLOStatus(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleSLOStatus");
    if (!sharding_metrics_) {
        return makeErrorResponse(http::status::service_unavailable, 
                                 "SLO monitoring not configured", req);
    }
    
    try {
        std::string slo_status = sharding_metrics_->getSLOStatus();
        return makeResponse(http::status::ok, slo_status, req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, 
                                 std::string("Failed to get SLO status: ") + e.what(), req);
    }
}

// static
json MonitoringApiHandler::buildConcernsJson(
    const core::concerns::HealthStatus& status, bool& ok)
{
    // Build a per-concern JSON object and update the caller's `ok` flag.
    json result = {
        {"logger",          {{"ok", status.logger.ok},          {"message", status.logger.message}}},
        {"tracer",          {{"ok", status.tracer.ok},          {"message", status.tracer.message}}},
        {"metrics",         {{"ok", status.metrics.ok},         {"message", status.metrics.message}}},
        {"cache",           {{"ok", status.cache.ok},           {"message", status.cache.message}}},
        {"secrets",         {{"ok", status.secrets.ok},         {"message", status.secrets.message}}},
        {"circuit_breaker", {{"ok", status.circuit_breaker.ok}, {"message", status.circuit_breaker.message}}},
        {"feature_flags",   {{"ok", status.featureFlags.ok},    {"message", status.featureFlags.message}}},
        {"audit_log",       {{"ok", status.auditLog.ok},        {"message", status.auditLog.message}}}
    };
    if (!status.isHealthy()) {
        ok = false;
    }
    return result;
}

// ============================================================================
// Operator Observability REST API  (Q1)
// ============================================================================

http::response<http::string_body> MonitoringApiHandler::handleObservabilityAlerts(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleObservabilityAlerts");
    try {
        auto severityToString = [](observability::AlertSeverity severity) -> const char* {
            switch (severity) {
                case observability::AlertSeverity::INFO: return "info";
                case observability::AlertSeverity::WARNING: return "warning";
                case observability::AlertSeverity::ERROR: return "error";
                case observability::AlertSeverity::CRITICAL: return "critical";
                default: return "unknown";
            }
        };
        auto statusToString = [](observability::AlertStatus status) -> const char* {
            switch (status) {
                case observability::AlertStatus::FIRING: return "firing";
                case observability::AlertStatus::RESOLVED: return "resolved";
                case observability::AlertStatus::SILENCED: return "silenced";
                default: return "unknown";
            }
        };

        json arr = json::array();
        if (alertmanager_) {
            for (const auto& alert : alertmanager_->getActiveAlerts()) {
                json a;
                a["alert_id"]   = alert.alert_id;
                a["alert_name"] = alert.alert_name;
                a["severity"]   = severityToString(alert.severity);
                a["status"]     = statusToString(alert.status);
                a["message"]    = alert.message;
                // ISO-8601 fired_at
                std::time_t t = std::chrono::system_clock::to_time_t(alert.fired_at);
                std::tm tm_buf{};
#if defined(_WIN32)
                gmtime_s(&tm_buf, &t);
#else
                gmtime_r(&t, &tm_buf);
#endif
                char ts[32];
                std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
                a["fired_at"] = ts;
                json labels = json::object();
                for (const auto& [k, v] : alert.labels) { labels[k] = v; }
                a["labels"] = labels;
                arr.push_back(a);
            }
        }
        json body;
        body["alerts"] = arr;
        body["count"]  = arr.size();
        body["alertmanager_enabled"] = (alertmanager_ != nullptr) &&
                                       alertmanager_->getConfig().enabled;
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Failed to list alerts: ") + e.what(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::handleObservabilityAlertSilence(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleObservabilityAlertSilence");
    try {
        // Extract alert ID from path: /api/v1/observability/alerts/{id}/silence
        const std::string target = std::string(req.target());
        // Find the segment between /alerts/ and /silence
        const std::string prefix = "/api/v1/observability/alerts/";
        const std::string suffix = "/silence";
        if (target.rfind(prefix, 0) != 0) {
            return makeErrorResponse(http::status::bad_request, "Invalid path", req);
        }
        std::string rest = target.substr(prefix.size());
        auto spos = rest.rfind(suffix);
        if (spos == std::string::npos) {
            return makeErrorResponse(http::status::bad_request, "Path must end with /silence", req);
        }
        std::string alert_id = rest.substr(0, spos);
        if (alert_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing alert ID in path", req);
        }

        // Parse optional duration from request body
        int duration_minutes = 60;
        if (!req.body().empty()) {
            try {
                auto j = json::parse(req.body());
                if (j.contains("duration_minutes") && j["duration_minutes"].is_number_integer()) {
                    duration_minutes = j["duration_minutes"].get<int>();
                }
            } catch (const std::exception&) {
                // ignore JSON parse errors; use default duration
            }
        }
        if (duration_minutes <= 0) {
            return makeErrorResponse(http::status::bad_request,
                                     "duration_minutes must be a positive integer", req);
        }

        if (!alertmanager_) {
            return makeErrorResponse(http::status::service_unavailable,
                                     "Alertmanager not configured", req);
        }

        auto result = alertmanager_->silenceAlert(alert_id, duration_minutes);
        if (!result) {
            return makeErrorResponse(http::status::bad_gateway,
                                     "Silence request failed: " + result.error().message(), req);
        }

        json body;
        body["alert_id"]        = alert_id;
        body["silenced"]        = true;
        body["duration_minutes"] = duration_minutes;
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Failed to silence alert: ") + e.what(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::handleObservabilityHealth(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleObservabilityHealth");
    try {
        json body;
        body["status"] = "ok";

        // Alertmanager health
        {
            json am;
            if (alertmanager_) {
                const auto& cfg = alertmanager_->getConfig();
                am["enabled"]      = cfg.enabled;
                am["endpoint_url"] = cfg.endpoint_url;
                am["active_alerts"] = static_cast<int>(
                    alertmanager_->getActiveAlerts().size());
                if (cfg.enabled) {
                    auto conn = alertmanager_->testConnection();
                    am["reachable"] = conn.has_value();
                    if (!conn) {
                        am["error"]  = conn.error().message();
                        body["status"] = "degraded";
                    }
                } else {
                    am["reachable"] = false;
                }
            } else {
                am["enabled"]   = false;
                am["reachable"] = false;
            }
            body["alertmanager"] = am;
        }

        // Tracing health
        {
            json tracing;
            tracing["total_spans"]  = Tracer::getTotalSpans();
            tracing["active_spans"] = Tracer::getActiveSpans();
            body["tracing"] = tracing;
        }

        // MetricsCollector health
        {
            auto& mc = observability::MetricsCollector::getInstance();
            json mc_obj;
            mc_obj["dropped_series"]     = mc.getDroppedSeriesCount();
            mc_obj["cardinality_limit"]  = static_cast<int64_t>(mc.getCardinalityLimit());
            body["metrics_collector"] = mc_obj;
        }

        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Failed to get observability health: ") + e.what(),
                                 req);
    }
}

// ---------------------------------------------------------------------------
// /metrics/html  – lightweight human-readable metrics dashboard
// ---------------------------------------------------------------------------

http::response<http::string_body> MonitoringApiHandler::handleMetricsHtml(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleMetricsHtml");
    try {
        // Collect raw Prometheus text
        std::string version;
#ifdef THEMIS_VERSION_STRING
        version = THEMIS_VERSION_STRING;
#else
        version = "unknown";
#endif
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - *start_time_
        ).count();

        // Build a minimal metrics snapshot for the HTML table
        // We reuse the text metrics output from handleMetrics() but wrap it in HTML.
        // Gather a fresh copy via MonitoringApiHandler::handleMetrics() internals is complex,
        // so we call through the existing implementation.
        // Build an artificial GET /metrics request to reuse the implementation.
        http::request<http::string_body> metrics_req{http::verb::get, "/metrics", 11};
        auto prom_resp = handleMetrics(metrics_req);
        const std::string prom_text = prom_resp.body();

        // Parse the prometheus text into (name, value) pairs for the table
        std::vector<std::pair<std::string, std::string>> rows;
        std::istringstream iss(prom_text);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto sp = line.rfind(' ');
            if (sp == std::string::npos) continue;
            rows.emplace_back(line.substr(0, sp), line.substr(sp + 1));
        }

        std::string html;
        html.reserve(8192);
        html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
        html += "<meta charset=\"UTF-8\">\n";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
        html += "<title>ThemisDB Metrics</title>\n";
        html += "<style>\n";
        html += "body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;margin:0;padding:16px}\n";
        html += "h1{color:#00d4ff;margin-bottom:4px;}\n";
        html += "p.sub{color:#888;margin-top:0;font-size:0.85em}\n";
        html += "table{border-collapse:collapse;width:100%;margin-top:12px}\n";
        html += "th{background:#16213e;color:#00d4ff;padding:6px 12px;text-align:left;";
        html += "border-bottom:2px solid #0f3460}\n";
        html += "td{padding:4px 12px;border-bottom:1px solid #0f3460}\n";
        html += "tr:hover td{background:#16213e}\n";
        html += ".val{text-align:right;color:#00ff9f}\n";
        html += "a{color:#00d4ff;text-decoration:none}a:hover{text-decoration:underline}\n";
        html += "</style>\n</head>\n<body>\n";
        html += "<h1>ThemisDB Metrics Dashboard</h1>\n";
        html += "<p class=\"sub\">Version: <b>" + version + "</b> &nbsp;|&nbsp; ";
        html += "Uptime: <b>" + std::to_string(uptime_seconds) + "s</b> &nbsp;|&nbsp; ";
        html += "<a href=\"/metrics\">Raw Prometheus</a></p>\n";
        html += "<table>\n<tr><th>Metric</th><th class=\"val\">Value</th></tr>\n";
        for (const auto& [name, val] : rows) {
            html += "<tr><td>" + name + "</td><td class=\"val\">" + val + "</td></tr>\n";
        }
        html += "</table>\n</body>\n</html>\n";

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "text/html; charset=utf-8");
        res.keep_alive(req.keep_alive());
        res.body() = html;
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Failed to generate metrics HTML: ") + e.what(), req);
    }
}

// =============================================================================
// GET /api/v1/license/status
// =============================================================================

http::response<http::string_body> MonitoringApiHandler::handleLicenseStatus(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleLicenseStatus");
    using namespace themis::license;

    const RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();

    json body;
    body["initialized"]          = gate.isInitialized();
    body["status"]               = gate.licenseStatus();
    body["grace_days_remaining"] = gate.graceDaysRemaining();

    auto lic = gate.currentLicense();
    if (!lic) {
        // Fall back to the compile-time embedded license if the gate hasn't
        // been initialised yet (e.g. very early health probe during startup).
        lic = getEmbeddedLicense();
    }

    if (lic) {
        // Mask the license key: show only the first 8 characters.
        std::string masked_key = lic->license_key;
        if (masked_key.size() > 8) {
            masked_key = masked_key.substr(0, 8) + "...";
        }
        body["license_key"]      = masked_key;
        body["organization"]     = lic->organization_name;
        body["edition"]          = lic->edition;
        body["expiry_date"]      = lic->expiry_date;
        body["days_until_expiry"] = getDaysUntilExpiry(*lic);
        body["valid"]            = isLicenseValid(*lic);
    } else {
        body["license_key"]      = nullptr;
        body["organization"]     = nullptr;
        body["edition"]          = edition::EDITION_STRING;
        body["expiry_date"]      = nullptr;
        body["days_until_expiry"] = nullptr;
        body["valid"]            = false;
    }

    return makeResponse(http::status::ok, body.dump(), req);
}

// =============================================================================
// registerRoutes() – populate the global RouteRegistry with all monitoring
// handler endpoint annotations so that handleOpenApi() can auto-generate the
// OpenAPI 3.1.0 spec without duplicating path information.
// =============================================================================

void MonitoringApiHandler::registerRoutes() {
    RouteRegistry& reg = RouteRegistry::instance();

    // --- Monitoring / Health ---
    reg.registerRoute({"/health", "get", {
        "Basic health check", "", "getHealth", {"monitoring"}, {}, {},
        {{"200", {{"description","Server is healthy"},
                  {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}},
         {"503", {{"description","Server is unhealthy"}}}}
    }});
    reg.registerRoute({"/health/live", "get", {
        "Liveness probe",
        "Returns 200 if the server process is running, 503 if it has stopped",
        "getLiveness", {"monitoring"}, {}, {},
        {{"200", {{"description","Server process is alive"}}},
         {"503", {{"description","Server process is dead"}}}}
    }});
    reg.registerRoute({"/health/ready", "get", {
        "Readiness probe",
        "Returns 200 when all subsystems (storage, connections) are ready. "
        "Returns per-layer health details.",
        "getReadiness", {"monitoring"}, {}, {},
        {{"200", {{"description","All layers ready"}}},
         {"503", {{"description","One or more layers not ready"}}}}
    }});

    // --- Version / Stats / Capabilities ---
    reg.registerRoute({"/version", "get", {
        "Get server version", "", "getVersion", {"monitoring"}, {}, {},
        {{"200", {{"description","Version and build information"}}}}
    }});
    reg.registerRoute({"/stats", "get", {
        "Runtime statistics", "", "getStats", {"monitoring"}, {}, {},
        {{"200", {{"description","Runtime statistics in JSON"}}}}
    }});
    reg.registerRoute({"/api/capabilities", "get", {
        "Server capabilities", "", "getCapabilities", {"monitoring"}, {}, {},
        {{"200", {{"description","Server feature capability map"}}}}
    }});

    // --- Metrics ---
    reg.registerRoute({"/metrics", "get", {
        "Prometheus metrics",
        "Returns server metrics in Prometheus text exposition format",
        "getMetrics", {"monitoring"}, {}, {},
        {{"200", {{"description","Prometheus text format metrics"},
                  {"content",{{"text/plain",{{"schema",{{"type","string"}}}}}}}}}}
    }});
    reg.registerRoute({"/metrics/html", "get", {
        "HTML metrics dashboard",
        "Renders current Prometheus metrics as a human-readable HTML table "
        "with dark-mode styling",
        "getMetricsHtml", {"monitoring"}, {}, {},
        {{"200", {{"description","HTML metrics dashboard"},
                  {"content",{{"text/html",{{"schema",{{"type","string"}}}}}}}}}}
    }});

    // --- Observability operator API ---
    reg.registerRoute({"/api/v1/observability/alerts", "get", {
        "List active alerts",
        "Returns the list of currently active (firing or silenced) alerts as a JSON array",
        "getObservabilityAlerts", {"observability"}, {}, {},
        {{"200", {{"description","Active alert list"},
                  {"content",{{"application/json",{{"schema",{
                      {"type","object"},
                      {"properties",{
                          {"alerts",{{"type","array"}}},
                          {"count",{{"type","integer"}}},
                          {"alertmanager_enabled",{{"type","boolean"}}}
                      }}
                  }}}}}}}}}
    }});
    reg.registerRoute({"/api/v1/observability/alerts/{id}/silence", "post", {
        "Silence an alert",
        "Silences the named alert for a configurable duration. "
        "Body: {\"duration_minutes\": <int>} (default 60)",
        "silenceObservabilityAlert", {"observability"},
        {RouteParam{"id", "path", true, "Alert identifier", {{"type","string"}}}},
        {{"required",false},
         {"content",{{"application/json",{{"schema",{
             {"type","object"},
             {"properties",{{"duration_minutes",{{"type","integer"},{"default",60}}}}}
         }}}}}}},
        {{"200",{{"description","Alert silenced"}}},
         {"400",{{"description","Bad request"}}},
         {"404",{{"description","Alert not found"}}},
         {"503",{{"description","Alertmanager not configured"}}}}
    }});
    reg.registerRoute({"/api/v1/observability/health", "get", {
        "Observability subsystem health",
        "Returns aggregate health of the observability stack: Alertmanager "
        "connectivity, tracing span counters, and MetricsCollector cardinality",
        "getObservabilityHealth", {"observability"}, {}, {},
        {{"200",{{"description","Observability health status"},
                 {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}}  }
    }});

    // --- License ---
    reg.registerRoute({"/api/v1/license/status", "get", {
        "Runtime license status",
        "Returns the current runtime license state: initialized flag, status string "
        "(active/grace/expired/invalid), grace days remaining, masked license key, "
        "organization, edition, expiry date, days until expiry, and validity flag.",
        "getLicenseStatus", {"license"}, {}, {},
        {{"200",{{"description","License status document"},
                 {"content",{{"application/json",{{"schema",{
                     {"type","object"},
                     {"properties",{
                         {"initialized",         {{"type","boolean"}}},
                         {"status",              {{"type","string"},{"example","active"}}},
                         {"grace_days_remaining",{{"type","integer"}}},
                         {"license_key",         {{"type","string"},{"example","THEMIS-EN..."}}},
                         {"organization",        {{"type","string"}}},
                         {"edition",             {{"type","string"},{"example","ENTERPRISE"}}},
                         {"expiry_date",         {{"type","string"},{"format","date"}}},
                         {"days_until_expiry",   {{"type","integer"}}},
                         {"valid",               {{"type","boolean"}}}
                     }}
                 }}}}}}}}}
    }});

    // --- OpenAPI self-reference ---
    reg.registerRoute({"/api/openapi.json", "get", {
        "OpenAPI specification",
        "Returns this OpenAPI 3.1 specification document",
        "getOpenApiSpec", {"monitoring"}, {}, {},
        {{"200",{{"description","OpenAPI 3.1 specification"},
                 {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}}  }
    }});

    // --- Entities CRUD ---
    reg.registerRoute({"/entities", "get", {
        "List entities", "", "listEntities", {"entities"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {},
        {{"200",{{"description","Entity list"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}},
                              {"Deprecation",{{"$ref","#/components/headers/Deprecation"}}},
                              {"Sunset",     {{"$ref","#/components/headers/Sunset"}}},
                              {"Link",       {{"$ref","#/components/headers/Link"}}}}}}},
         {"401",{{"description","Unauthorized"}}}}
    }});
    reg.registerRoute({"/entities", "post", {
        "Create entity", "", "createEntity", {"entities"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Entity created"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"400",{{"description","Bad request"}}},
         {"413",{{"description","Payload too large"}}}}
    }});
    reg.registerRoute({"/entities/{key}", "get", {
        "Get entity by key", "", "getEntity", {"entities"},
        {RouteParam{"key","path",true,"Entity key",{{"type","string"}}},
         RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {},
        {{"200",{{"description","Entity found"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}},
                              {"Deprecation",{{"$ref","#/components/headers/Deprecation"}}},
                              {"Sunset",     {{"$ref","#/components/headers/Sunset"}}},
                              {"Link",       {{"$ref","#/components/headers/Link"}}}}}}},
         {"404",{{"description","Not found"}}}}
    }});
    reg.registerRoute({"/entities/{key}", "put", {
        "Upsert entity by key", "", "upsertEntity", {"entities"},
        {RouteParam{"key","path",true,"Entity key",{{"type","string"}}},
         RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Entity updated"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"201",{{"description","Entity created"}}}}
    }});
    reg.registerRoute({"/entities/{key}", "delete", {
        "Delete entity by key", "", "deleteEntity", {"entities"},
        {RouteParam{"key","path",true,"Entity key",{{"type","string"}}},
         RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {},
        {{"200",{{"description","Entity deleted"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"404",{{"description","Not found"}}}}
    }});

    // --- Query ---
    reg.registerRoute({"/query", "post", {
        "Execute a query", "", "postQuery", {"query"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Query results"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"400",{{"description","Bad query"}}}}
    }});
    reg.registerRoute({"/query/aql", "post", {
        "Execute an AQL (ThemisDB Query Language) query", "", "postAqlQuery", {"query"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","AQL query results"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"400",{{"description","AQL syntax error"}}}}
    }});

    // --- UDF Registration ---
    reg.registerRoute({"/api/v1/query/udfs", "post", {
        "Register or replace an AQL user-defined function (UDF)", "", "registerUdf", {"udf"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","UDF registered"}}},
         {"400",{{"description","Invalid UDF definition"}}}}
    }});
    reg.registerRoute({"/api/v1/query/udfs", "get", {
        "List all registered AQL user-defined functions", "", "listUdfs", {"udf"},
        {},
        {},
        {{"200",{{"description","UDF list"}}}}
    }});
    reg.registerRoute({"/api/v1/query/udfs/{name}", "get", {
        "Get a single AQL user-defined function definition", "", "getUdf", {"udf"},
        {RouteParam{"name","path",true,"UDF name",{{"type","string"}}}},
        {},
        {{"200",{{"description","UDF definition"}}},
         {"404",{{"description","UDF not found"}}}}
    }});
    reg.registerRoute({"/api/v1/query/udfs/{name}", "delete", {
        "Unregister an AQL user-defined function", "", "deleteUdf", {"udf"},
        {RouteParam{"name","path",true,"UDF name",{{"type","string"}}}},
        {},
        {{"204",{{"description","UDF deleted"}}},
         {"404",{{"description","UDF not found"}}}}
    }});

    // --- Changefeed Compact ---
    reg.registerRoute({"/changefeed/compact", "post", {
        "Compact changefeed log",
        "Runs a compaction pass retaining only the latest event per key. "
        "Requires cdc:admin scope.",
        "compactChangefeed", {"changefeed"}, {}, {},
        {{"200",{{"description","Compaction completed"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Feature cdc disabled"}}},
         {"503",{{"description","Service unavailable"}}},
         {"500",{{"description","Internal error"}}}}
    }});

    // --- Incremental Graph Query ---
    reg.registerRoute({"/graph/query/incremental", "post", {
        "Register incremental BFS graph query",
        "Registers a live BFS query that is automatically re-executed on graph changes.",
        "registerIncrementalGraphQuery", {"graph"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Incremental query registered"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"503",{{"description","Service unavailable"}}},
         {"500",{{"description","Internal error"}}}}
    }});
    reg.registerRoute({"/graph/query/incremental/{handle}", "delete", {
        "Unregister incremental BFS graph query",
        "Stops re-execution of the incremental query identified by handle.",
        "unregisterIncrementalGraphQuery", {"graph"},
        {RouteParam{"handle","path",true,"Registration handle",{{"type","integer"},{"format","int64"}}}},
        {},
        {{"200",{{"description","Query unregistered"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Handle not found"}}},
         {"503",{{"description","Service unavailable"}}}}
    }});

    // --- Graph Changes Notification ---
    reg.registerRoute({"/graph/changes", "post", {
        "Notify graph of topology changes",
        "Pushes a batch of vertex/edge mutations; triggers re-execution of affected incremental queries.",
        "notifyGraphChanges", {"graph"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Changes applied"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"503",{{"description","Service unavailable"}}},
         {"500",{{"description","Internal error"}}}}
    }});

    // --- SSE Streaming Query ---
    reg.registerRoute({"/v2/query/stream", "get", {
        "Stream AQL query results via Server-Sent Events",
        "Executes an AQL query and streams each result document as an SSE 'data:' event. "
        "Requires query parameter 'q' (URL-encoded AQL string).",
        "queryStreamSse", {"query"},
        {RouteParam{"q","query",true,"URL-encoded AQL query",{{"type","string"}}},
         RouteParam{"max_seconds","query",false,"Max stream duration (1-60)",{{"type","integer"},{"default",30}}},
         RouteParam{"heartbeat_ms","query",false,"Heartbeat interval ms",{{"type","integer"},{"default",15000}}},
         RouteParam{"retry_ms","query",false,"SSE reconnect interval ms",{{"type","integer"},{"default",3000}}}},
        {},
        {{"200",{{"description","SSE stream"},
                 {"content",{{"text/event-stream",{{"schema",{{"type","string"}}}}}}}  }},
         {"400",{{"description","Missing or invalid query parameter"}}},
         {"401",{{"description","Unauthorized"}}},
         {"500",{{"description","Internal error"}}}}
    }});

    // --- Serverless Functions ---
    reg.registerRoute({"/api/v1/functions", "post", {
        "Register a serverless function",
        "Registers a new in-process function defined by a JSON DSL code block.",
        "registerServerlessFunction", {"serverless"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Function registered"}}},
         {"400",{{"description","Invalid function definition"}}},
         {"401",{{"description","Unauthorized"}}}}
    }});
    reg.registerRoute({"/api/v1/functions", "get", {
        "List registered serverless functions", "", "listServerlessFunctions", {"serverless"},
        {RouteParam{"tenant_id","query",false,"Filter by tenant",{{"type","string"}}}},
        {},
        {{"200",{{"description","List of functions"}}},
         {"401",{{"description","Unauthorized"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}", "get", {
        "Get serverless function definition", "", "getServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Function definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}", "put", {
        "Update serverless function definition", "", "updateServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Function updated"}}},
         {"400",{{"description","Invalid definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}", "delete", {
        "Delete a serverless function", "", "deleteServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Function deleted"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}/invoke", "post", {
        "Invoke a serverless function",
        "Executes the function pipeline with the provided JSON payload.",
        "invokeServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Function result"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}},
         {"504",{{"description","Execution timed out"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}/versions", "get", {
        "List version history of a serverless function", "", "listServerlessFunctionVersions", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Version history"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});

    // --- Task Scheduler ---
    reg.registerRoute({"/api/tasks", "post", {
        "Register a new scheduled task",
        "Creates a new AQL-query or function task with interval, cron, or manual trigger.",
        "registerTask", {"tasks"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Task created"}}},
         {"400",{{"description","Invalid task definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
    reg.registerRoute({"/api/tasks", "get", {
        "List all registered scheduled tasks", "", "listTasks", {"tasks"},
        {},
        {},
        {{"200",{{"description","Task list"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
    reg.registerRoute({"/api/tasks/stats", "get", {
        "Get task scheduler statistics", "", "getTaskStats", {"tasks"},
        {},
        {},
        {{"200",{{"description","Scheduler statistics"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}", "get", {
        "Get a scheduled task by ID", "", "getTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}", "put", {
        "Update an existing scheduled task", "", "updateTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Task updated"}}},
         {"400",{{"description","Invalid task definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}", "delete", {
        "Unregister a scheduled task", "", "deleteTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task deleted"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}/enable", "post", {
        "Enable (resume) a disabled task", "", "enableTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task enabled"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}/disable", "post", {
        "Disable (pause) a scheduled task", "", "disableTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task disabled"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}/execute", "post", {
        "Execute a task immediately (out of schedule)",
        "Triggers immediate execution of a task regardless of its scheduled trigger. "
        "Requires tasks:admin scope.",
        "executeTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task executed"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden – requires tasks:admin scope"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/ui/tasks", "get", {
        "Task Scheduler Web UI",
        "Self-contained single-page application for managing scheduled tasks.",
        "tasksWebUi", {"tasks"},
        {},
        {},
        {{"200",{{"description","HTML dashboard"},
                 {"content",{{"text/html",{{"schema",{{"type","string"}}}}}}}  }},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
}

} // namespace server
} // namespace themis
