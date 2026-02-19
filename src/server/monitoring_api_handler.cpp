#include "server/monitoring_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "server/auth_middleware.h"
#include "server/sharding_metrics_handler.h"
#include "metadata/schema_manager.h"
#include "plugins/plugin_manager.h"
#include "security/hsm_provider.h"
#include "security/hsm_security_metrics.h"
#include "themis/build_info.h"
#include "themis/license_info.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <ctime>

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
    std::shared_ptr<ShardingMetricsHandler> sharding_metrics
)
    : storage_(std::move(storage))
    , auth_(std::move(auth))
    , request_count_(request_count)
    , error_count_(error_count)
    , start_time_(start_time)
    , secondary_index_(std::move(secondary_index))
    , schema_manager_(schema_manager)
    , sharding_metrics_(std::move(sharding_metrics))
{
}

http::response<http::string_body> MonitoringApiHandler::handleHealthCheck(
    const http::request<http::string_body>& req
) {
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

http::response<http::string_body> MonitoringApiHandler::handleVersion(
    const http::request<http::string_body>& req
) {
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
        
        return makeResponse(http::status::ok, response.dump(2), req);
    } catch (const std::exception& e) {
        json error_response = {
            {"error", "Failed to retrieve version information"},
            {"message", e.what()}
        };
        return makeResponse(http::status::internal_server_error, error_response.dump(), req);
    }
}

http::response<http::string_body> MonitoringApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
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
        } catch (...) {
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
    } catch (...) {
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
        } catch (...) {
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
        } catch (...) {
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
        out.reserve(2048);
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

        // Auth metrics (if available)
        // TODO: Re-enable when AuthMiddleware provides metrics interface
        // if (auth_) {
        //     const auto& m = auth_->getMetrics();
        //     out += "# HELP themis_authz_success_total Successful authorizations\n";
        //     out += "# TYPE themis_authz_success_total counter\n";
        //     out += "themis_authz_success_total " + std::to_string(m.authz_success_total.load()) + "\n";
        //     out += "# HELP themis_authz_denied_total Denied authorizations (forbidden)\n";
        //     out += "# TYPE themis_authz_denied_total counter\n";
        //     out += "themis_authz_denied_total " + std::to_string(m.authz_denied_total.load()) + "\n";
        //     out += "# HELP themis_authz_invalid_token_total Invalid or missing tokens\n";
        //     out += "# TYPE themis_authz_invalid_token_total counter\n";
        //     out += "themis_authz_invalid_token_total " + std::to_string(m.authz_invalid_token_total.load()) + "\n";
        // }

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
        } catch (...) {
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
        } catch (...) {
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
        } catch (...) {
            THEMIS_WARN("Unknown error while collecting HSM security metrics");
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

} // namespace server
} // namespace themis
