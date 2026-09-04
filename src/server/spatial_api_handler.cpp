/**
 * @file spatial_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/spatial_api_handler.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/spatial_index.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
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
        auto& spatial_index = *spatial_index_;
        
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
        
        auto status = spatial_index.createSpatialIndex(table, geometry_column, config);
        
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
        auto& spatial_index = *spatial_index_;
        
        // Rebuild: drop existing index, re-create it, then re-index all entities
        // Step 1: get geometry column (default "geometry")
        std::string geometry_column = j.value("geometry_column", "geometry");

        // Step 2: drop and re-create the spatial index
        auto drop_status = spatial_index.dropSpatialIndex(table);
        if (!drop_status.ok) {
            // Treat "not found" as non-fatal (index may not exist yet)
            THEMIS_WARN("SpatialRebuild: drop returned: {}", drop_status.message);
        }
        auto create_status = spatial_index.createSpatialIndex(table, geometry_column);
        if (!create_status.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to re-create spatial index: " + create_status.message, req);
        }

        // Step 3: scan all entities stored under prefix "{table}:" and re-insert geometry
        size_t indexed = 0;
        size_t skipped = 0;
        const std::string scan_prefix = table + ":";
        if (storage_) {
            storage_->scanPrefix(scan_prefix, [&](std::string_view key, std::string_view value) -> bool {
                try {
                    // Deserialize entity and look up geometry field
                    auto entity = BaseEntity::fromJson(key, value);
                    auto doc = nlohmann::json::parse(entity.toJson());
                    if (!doc.contains(geometry_column)) { ++skipped; return true; }

                    // Parse geometry from the field value (GeoJSON object/string or WKT string)
                    geo::GeometryInfo geom_info;
                    const auto& geo_val = doc[geometry_column];
                    bool parse_ok = false;
                    if (geo_val.is_object()) {
                        // GeoJSON object: must have "type" and "coordinates" keys
                        if (geo_val.contains("type") && geo_val.contains("coordinates")) {
                            try {
                                geom_info = geo::EWKBParser::parseGeoJSON(geo_val.dump());
                                parse_ok = true;
                            } catch (...) {}
                        }
                    } else if (geo_val.is_string()) {
                        const std::string& geo_str = geo_val.get<std::string>();
                        // Try GeoJSON string: must start with '{' and contain "type" key
                        if (!geo_str.empty() && geo_str.front() == '{' &&
                            geo_str.find("\"type\"") != std::string::npos) {
                            try {
                                geom_info = geo::EWKBParser::parseGeoJSON(geo_str);
                                parse_ok = true;
                            } catch (...) {}
                        }
                        if (!parse_ok) {
                            try {
                                geom_info = geo::EWKBParser::parseWKT(geo_str);
                                parse_ok = true;
                            } catch (...) {}
                        }
                    }
                    if (!parse_ok) {
                        THEMIS_WARN("SpatialRebuild: skipping key={} – geometry parse failed", key);
                        ++skipped; return true;
                    }

                    // Build sidecar and insert
                    auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
                    // Extract the primary key (strip table prefix from RocksDB key)
                    std::string_view pk = key;
                    if (static_cast<int>(pk.size()) > static_cast<int>(scan_prefix.size())) {
                        pk = pk.substr(scan_prefix.size());
                    }
                    auto ins = spatial_index.insert(table, pk, sidecar);
                    if (ins.ok) { ++indexed; } else { ++skipped; }
                } catch (const std::exception& ex) {
                    THEMIS_WARN("SpatialRebuild: skipping key={} – {}", key, ex.what());
                    ++skipped;
                }
                return true; // continue scan
            });
        }

        THEMIS_INFO("SpatialIndexRebuild: table={}, indexed={}, skipped={}", table, indexed, skipped);
        json response = {
            {"success", true},
            {"table",   table},
            {"indexed", indexed},
            {"skipped", skipped},
            {"message", "Spatial index rebuilt successfully"}
        };
        res.result(http::status::ok);
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
        auto& spatial_index = *spatial_index_;
        
        auto stats = spatial_index.getStats(table);
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
        auto& spatial_index = *spatial_index_;
        
        const auto& metrics = spatial_index.getMetrics();
        
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
        } catch (...) {
            THEMIS_DEBUG([[maybe_unused]] "spatial_api_handler: unhandled exception caught");
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
std::string SpatialApiHandler::urlDecode([[maybe_unused]] const std::string& str) {
    std::string result = {};
    result.reserve(str.size());
    for (size_t i = 0; i <static_cast<int>(str.size()); ++i) {
        // Check if we have at least 2 more characters after '%' for hex decoding
        // i + 2 <static_cast<int>(str.size()) ensures str[i+1] and str[i+2] are valid
        if (str[i] == '%' && i + 2 <static_cast<int>(str.size())) {
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
std::unordered_map<std::string, std::string> SpatialApiHandler::parseQuery([[maybe_unused]] const std::string& target) {
    std::unordered_map<std::string, std::string> out;
    auto qpos = target.find('?');
    if (qpos == std::string::npos) {
      return out;
    }
    auto qs = target.substr(qpos + 1);
    std::istringstream iss(qs);
    std::string kv = {};
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

