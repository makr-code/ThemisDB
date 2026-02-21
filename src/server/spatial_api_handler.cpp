/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_api_handler.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:42:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     334                                            ║
    • Open Issues:     TODOs: 1, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3055833f6  2026-02-20  GPU geospatial backend: replace stub with real intersecti... ║
    • 86839235d  2026-01-13  Add documentation archival system and new issue templates ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/spatial_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/spatial_index.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "geo/spatial_backend.h"
#include <nlohmann/json.hpp>
#include <sstream>

namespace themis {
namespace server {

using json = nlohmann::json;

SpatialApiHandler::SpatialApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<index::SpatialIndexManager> spatial_index,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , spatial_index_(std::move(spatial_index))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> SpatialApiHandler::handleIndexCreate(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("http.spatial_index_create");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    
    try {
        auto j = json::parse(req.body());
        
        if (!j.contains("table") || !j["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'table' field", req);
        }
        
        std::string table = j["table"];
        std::string geometry_column = j.value("geometry_column", "geometry");
        
        if (!spatial_index_) {
            return makeErrorResponse(http::status::internal_server_error, "Spatial index manager not available", req);
        }
        
        // Parse optional config
        index::RTreeConfig config;
        if (j.contains("config") && j["config"].is_object()) {
            auto cfg = j["config"];
            if (cfg.contains("total_bounds") && cfg["total_bounds"].is_object()) {
                auto bounds = cfg["total_bounds"];
                config.total_bounds = themis::geo::MBR(
                    bounds.value("minx", -180.0),
                    bounds.value("miny", -90.0),
                    bounds.value("maxx", 180.0),
                    bounds.value("maxy", 90.0)
                );
            }
        }
        
        auto status = spatial_index_->createSpatialIndex(table, geometry_column, config);
        
        if (!status) {
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::bad_request, status.message, req);
        }
        
        json response;
        response["success"] = true;
        response["table"] = table;
        response["geometry_column"] = geometry_column;
        response["message"] = "Spatial index created successfully";
        
        res.body() = response.dump();
        span.setStatus(true);
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
    
    res.prepare_payload();
    return res;
}

http::response<http::string_body> SpatialApiHandler::handleIndexRebuild(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("http.spatial_index_rebuild");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    
    try {
        auto j = json::parse(req.body());
        
        if (!j.contains("table") || !j["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'table' field", req);
        }
        
        std::string table = j["table"];
        
        if (!spatial_index_) {
            return makeErrorResponse(http::status::internal_server_error, "Spatial index manager not available", req);
        }
        
        // TODO: Implement rebuild by scanning all entities in table and re-indexing
        // For now, return a not-implemented response
        json response;
        response["success"] = false;
        response["table"] = table;
        response["message"] = "Spatial index rebuild not yet implemented. Use drop + create for now.";
        response["status_code"] = 501;
        
        res.result(http::status::not_implemented);
        res.body() = response.dump();
        span.setStatus(false, "not_implemented");
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
    
    res.prepare_payload();
    return res;
}

http::response<http::string_body> SpatialApiHandler::handleIndexStats(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("http.spatial_index_stats");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    
    try {
        auto params = parseQuery(std::string(req.target()));
        std::string table = params["table"];
        
        if (table.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' query parameter", req);
        }
        
        if (!spatial_index_) {
            return makeErrorResponse(http::status::internal_server_error, "Spatial index manager not available", req);
        }
        
        auto stats = spatial_index_->getStats(table);
        json response;
        response["success"] = true;
        response["table"] = table;
        response["entry_count"] = stats.entry_count;
        response["total_bounds"] = {
            {"minx", stats.total_bounds.minx},
            {"miny", stats.total_bounds.miny},
            {"maxx", stats.total_bounds.maxx},
            {"maxy", stats.total_bounds.maxy}
        };
        response["avg_area"] = stats.avg_area;
        response["morton_buckets"] = stats.morton_buckets;
        res.body() = response.dump();
        span.setStatus(true);
        
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
    
    res.prepare_payload();
    return res;
}

http::response<http::string_body> SpatialApiHandler::handleMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("http.spatial_metrics");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    
    try {
        if (!spatial_index_) {
            return makeErrorResponse(http::status::internal_server_error, "Spatial index manager not available", req);
        }
        
        const auto& metrics = spatial_index_->getMetrics();
        
        json response;
        response["query_count"] = metrics.query_count.load();
        response["mbr_candidate_count"] = metrics.mbr_candidate_count.load();
        response["exact_check_count"] = metrics.exact_check_count.load();
        response["exact_check_passed"] = metrics.exact_check_passed.load();
        response["exact_check_failed"] = metrics.exact_check_failed.load();
        response["insert_count"] = metrics.insert_count.load();
        response["remove_count"] = metrics.remove_count.load();
        response["update_count"] = metrics.update_count.load();
        
        // Compute derived metrics
        uint64_t total_exact = metrics.exact_check_count.load();
        if (total_exact > 0) {
            double precision = static_cast<double>(metrics.exact_check_passed.load()) / total_exact;
            response["exact_check_precision"] = precision;
            response["false_positive_rate"] = 1.0 - precision;
        } else {
            response["exact_check_precision"] = nullptr;
            response["false_positive_rate"] = nullptr;
        }
        
        uint64_t total_queries = metrics.query_count.load();
        if (total_queries > 0) {
            response["avg_candidates_per_query"] = static_cast<double>(metrics.mbr_candidate_count.load()) / total_queries;
        } else {
            response["avg_candidates_per_query"] = nullptr;
        }

        // GPU spatial backend stats (always present; gpu_present=false when no device)
        try {
            auto gpu_json_str = geo::getGpuSpatialBackendStatsJson();
            auto gpu_stats = json::parse(gpu_json_str);
            response["gpu_backend"] = gpu_stats;
        } catch (const std::exception& e) {
            response["gpu_backend"] = nullptr;
        }
        
        res.body() = response.dump();
        span.setStatus(true);
        
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
    
    res.prepare_payload();
    return res;
}

// Helper to decode URL-encoded strings
std::string SpatialApiHandler::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        // Check if we have at least 2 more characters after '%' for hex decoding
        // i + 2 < str.size() ensures str[i+1] and str[i+2] are valid
        if (str[i] == '%' && i + 2 < str.size()) {
            int value = 0;
            std::istringstream is(str.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

// Helper to parse query parameters from URL
std::unordered_map<std::string, std::string> SpatialApiHandler::parseQuery(const std::string& target) {
    std::unordered_map<std::string, std::string> out;
    auto qpos = target.find('?');
    if (qpos == std::string::npos) return out;
    auto qs = target.substr(qpos + 1);
    std::istringstream iss(qs);
    std::string kv;
    while (std::getline(iss, kv, '&')) {
        auto eq = kv.find('=');
        std::string k = (eq == std::string::npos) ? kv : kv.substr(0, eq);
        std::string v = (eq == std::string::npos) ? std::string() : kv.substr(eq + 1);
        out[urlDecode(k)] = urlDecode(v);
    }
    return out;
}

http::response<http::string_body> SpatialApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> SpatialApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis
