/**
 * @file profiling_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/profiling_api_handler.h"
#include <sstream>
#include <algorithm>
#include <charconv>
#include "utils/tracing.h"
#include <stdexcept>
#include "utils/logger.h"

namespace themis {
namespace server {

using json = nlohmann::json;

ProfilingApiHandler::ProfilingApiHandler(
    std::shared_ptr<observability::QueryProfiler> query_profiler,
    std::shared_ptr<observability::StorageProfiler> storage_profiler,
    std::shared_ptr<observability::PerformanceAnalyzer> analyzer)
    : query_profiler_(query_profiler),
      storage_profiler_(storage_profiler),
      analyzer_(analyzer) {}

http::response<http::string_body> ProfilingApiHandler::handle_request(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handle_request");
    
    std::string target(req.target());
    auto method = req.method();
    
    // Route to appropriate handler
    if (target == "/api/profiling/enable" && method == http::verb::post) {
        return handle_enable(req);
    }
    else if (target == "/api/profiling/disable" && method == http::verb::post) {
        return handle_disable(req);
    }
    else if (target.find("/api/profiling/queries") == 0 && method == http::verb::get) {
        return handle_get_queries(req);
    }
    else if (target.find("/api/profiling/slow-queries") == 0 && method == http::verb::get) {
        return handle_get_slow_queries(req);
    }
    else if (target == "/api/profiling/storage" && method == http::verb::get) {
        return handle_get_storage(req);
    }
    else if (target == "/api/profiling/analyze" && method == http::verb::post) {
        return handle_analyze(req);
    }
    else if (target == "/api/profiling/export" && method == http::verb::get) {
        return handle_export(req);
    }
    else if (target == "/api/profiling/clear" && method == http::verb::post) {
        return handle_clear(req);
    }
    else if (target == "/api/profiling/config" && method == http::verb::get) {
        return handle_get_config(req);
    }
    else if (target == "/api/profiling/config" && method == http::verb::post) {
        return handle_set_config(req);
    }
    
    return make_error_response(http::status::not_found, "Endpoint not found");
}

http::response<http::string_body> ProfilingApiHandler::handle_enable(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_enable");
    
    query_profiler_->enable();
    storage_profiler_->enable();
    
    json response = {
        {"success", true},
        {"message", "Profiling enabled"}
    };
    
    return make_response(http::status::ok, response);
}

http::response<http::string_body> ProfilingApiHandler::handle_disable(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_disable");
    
    query_profiler_->disable();
    storage_profiler_->disable();
    
    json response = {
        {"success", true},
        {"message", "Profiling disabled"}
    };
    
    return make_response(http::status::ok, response);
}

http::response<http::string_body> ProfilingApiHandler::handle_get_queries(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handle_get_queries");
    
    int limit = 100;
    if (!get_query_param_int(std::string(req.target()), "limit", 100, limit)) {
        return make_error_response(http::status::bad_request, "invalid limit");
    }
    if (limit < 0) {
        return make_error_response(http::status::bad_request, "invalid limit");
    }
    
    auto profiles = query_profiler_->get_all_profiles();
    
    // Sort by duration (descending) and limit
    std::sort(profiles.begin(), profiles.end(),
             [](const auto& a, const auto& b) {
                 return a->total_duration > b->total_duration;
             });
    
    if (static_cast<int>(profiles.size()) > static_cast<size_t>(limit)) {
        profiles.resize(limit);
    }
    
    json result = json::array();
    for (const auto& profile : profiles) {
        result.push_back(profile->toJSON());
    }
    
    return make_response(http::status::ok, result);
}

http::response<http::string_body> ProfilingApiHandler::handle_get_slow_queries(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handle_get_slow_queries");
    
    int threshold_ms = 1000;
    if (!get_query_param_int(std::string(req.target()), "threshold_ms", 1000, threshold_ms)) {
        return make_error_response(http::status::bad_request, "invalid threshold_ms");
    }
    if (threshold_ms < 0) {
        return make_error_response(http::status::bad_request, "invalid threshold_ms");
    }
    
    auto slow_queries = query_profiler_->get_slow_queries(
        std::chrono::milliseconds(threshold_ms));
    
    json result = json::array();
    for (const auto& profile : slow_queries) {
        result.push_back(profile->toJSON());
    }
    
    return make_response(http::status::ok, result);
}

http::response<http::string_body> ProfilingApiHandler::handle_get_storage(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_get_storage");
    
    json result = {
        {"operation_summary", storage_profiler_->get_operation_summary()},
        {"cache_metrics", storage_profiler_->get_cache_metrics()},
        {"amplification_metrics", storage_profiler_->get_amplification_metrics()}
    };
    
    auto latest_stats = storage_profiler_->get_latest_rocksdb_stats();
    if (latest_stats.has_value()) {
        result["rocksdb_stats"] = latest_stats->toJSON();
    }
    
    return make_response(http::status::ok, result);
}

http::response<http::string_body> ProfilingApiHandler::handle_analyze(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_analyze");
    
    auto analysis = analyzer_->analyze(*query_profiler_, *storage_profiler_);
    
    return make_response(http::status::ok, analysis.toJSON());
}

http::response<http::string_body> ProfilingApiHandler::handle_export(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_export");
    
    auto query_profiles = query_profiler_->get_all_profiles();
    auto storage_stats = storage_profiler_->get_rocksdb_stats_history();
    
    json query_json = json::array();
    for (const auto& profile : query_profiles) {
        query_json.push_back(profile->toJSON());
    }
    
    json storage_json = json::array();
    for (const auto& stats : storage_stats) {
        storage_json.push_back(stats.toJSON());
    }
    
    json result = {
        {"query_profiles", query_json},
        {"storage_stats", storage_json},
        {"query_statistics", query_profiler_->get_statistics()},
        {"storage_summary", storage_profiler_->get_operation_summary()}
    };
    
    return make_response(http::status::ok, result);
}

http::response<http::string_body> ProfilingApiHandler::handle_clear(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_clear");
    
    query_profiler_->clear();
    storage_profiler_->clear();
    
    json response = {
        {"success", true},
        {"message", "Profiles cleared"}
    };
    
    return make_response(http::status::ok, response);
}

http::response<http::string_body> ProfilingApiHandler::handle_get_config(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handle_get_config");
    
    auto query_config = query_profiler_->get_config();
    auto storage_config = storage_profiler_->get_config();
    auto analyzer_config = analyzer_->get_config();
    
    json result = {
        {"query_profiler", {
            {"enabled", query_config.enabled},
            {"profile_all_queries", query_config.profile_all_queries},
            {"collect_operator_stats", query_config.collect_operator_stats},
            {"max_profiles_retained", query_config.max_profiles_retained},
            {"slow_query_threshold_ms", query_config.slow_query_threshold.count()}
        }},
        {"storage_profiler", {
            {"enabled", storage_config.enabled},
            {"collect_op_stats", storage_config.collect_op_stats},
            {"max_ops_retained", storage_config.max_ops_retained},
            {"slow_op_threshold_ms", storage_config.slow_op_threshold.count()}
        }},
        {"analyzer", {
            {"slow_query_threshold_ms", analyzer_config.slow_query_threshold.count()},
            {"cache_hit_rate_threshold", analyzer_config.cache_hit_rate_threshold},
            {"write_amplification_threshold", analyzer_config.write_amplification_threshold}
        }}
    };
    
    return make_response(http::status::ok, result);
}

http::response<http::string_body> ProfilingApiHandler::handle_set_config(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handle_set_config");
    
    try {
        json body = json::parse(req.body());
        
        // Update query profiler config
        if (body.contains("query_profiler")) {
            auto config = query_profiler_->get_config();
            auto qp = body["query_profiler"];
            
            if (qp.contains("enabled")) {
              config.enabled = qp["enabled"];
            }
            if (qp.contains("profile_all_queries")) 
                config.profile_all_queries = qp["profile_all_queries"];
            if (qp.contains("slow_query_threshold_ms")) {
                const auto threshold_ms = qp["slow_query_threshold_ms"].get<int>();
                if (threshold_ms < 0) {
                    throw std::invalid_argument("query_profiler.slow_query_threshold_ms must be >= 0");
                }
                config.slow_query_threshold = 
                    std::chrono::milliseconds(threshold_ms);
            }
            
            query_profiler_->set_config(config);
        }
        
        // Update storage profiler config
        if (body.contains("storage_profiler")) {
            auto config = storage_profiler_->get_config();
            auto sp = body["storage_profiler"];
            
            if (sp.contains("enabled")) {
              config.enabled = sp["enabled"];
            }
            if (sp.contains("slow_op_threshold_ms")) {
                const auto threshold_ms = sp["slow_op_threshold_ms"].get<int>();
                if (threshold_ms < 0) {
                    throw std::invalid_argument("storage_profiler.slow_op_threshold_ms must be >= 0");
                }
                config.slow_op_threshold = 
                    std::chrono::milliseconds(threshold_ms);
            }
            
            storage_profiler_->set_config(config);
        }
        
        // Update analyzer config
        if (body.contains("analyzer")) {
            auto config = analyzer_->get_config();
            auto an = body["analyzer"];
            
            if (an.contains("slow_query_threshold_ms")) {
                const auto threshold_ms = an["slow_query_threshold_ms"].get<int>();
                if (threshold_ms < 0) {
                    throw std::invalid_argument("analyzer.slow_query_threshold_ms must be >= 0");
                }
                config.slow_query_threshold = 
                    std::chrono::milliseconds(threshold_ms);
            }
            if (an.contains("cache_hit_rate_threshold")) {
                const auto cache_hit_rate = an["cache_hit_rate_threshold"].get<double>();
                if (cache_hit_rate < 0.0 || cache_hit_rate > 1.0) {
                    throw std::invalid_argument("analyzer.cache_hit_rate_threshold must be between 0.0 and 1.0");
                }
                config.cache_hit_rate_threshold = cache_hit_rate;
            }
            
            analyzer_->set_config(config);
        }
        
        json response = {
            {"success", true},
            {"message", "Configuration updated"}
        };
        
        return make_response(http::status::ok, response);
        
    } catch (const std::exception& e) {
        return make_error_response(http::status::bad_request, 
                                  std::string("Invalid configuration: ") + e.what());
    }
}

http::response<http::string_body> ProfilingApiHandler::make_response(
    http::status status, const json& body) {
    auto span = Tracer::startSpan("make_response");
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ProfilingApiHandler::make_error_response(
    http::status status, const std::string& message) {
    auto span = Tracer::startSpan("make_error_response");
    
    json error = {
        {"error", message}
    };
    
    return make_response(status, error);
}

bool ProfilingApiHandler::get_query_param_int(const std::string& target,
                                              const std::string& param_name,
                                              int default_value,
                                              int& value) {
    value = default_value;

    const auto query_pos = target.find('?');
    if (query_pos == std::string::npos || query_pos + 1 >= target.size()) {
        return true;
    }

    const std::string query = target.substr(query_pos + 1);
    size_t pos = 0;
    bool found = false;
    std::string value_str = {};

    while (pos <= query.size()) {
        const size_t amp = query.find('&', pos);
        const size_t token_end = (amp == std::string::npos) ? query.size() : amp;
        if (token_end > pos) {
            const std::string token = query.substr(pos, token_end - pos);
            const size_t eq = token.find('=');
            const std::string key = (eq == std::string::npos) ? token : token.substr(0, eq);
            if (key == param_name) {
                value_str = (eq == std::string::npos) ? "" : token.substr(eq + 1);
                found = true;
                break;
            }
        }
        if (amp == std::string::npos) {
            break;
        }
        pos = amp + 1;
    }

    if (!found) {
        return true;
    }

    try {
        value = std::stoi(value_str);
        return true;
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "profiling_api_handler: unhandled exception caught");
        value = default_value;
        return false;
    }
}

} // namespace server
} // namespace themis


