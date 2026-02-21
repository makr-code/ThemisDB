/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            timeseries_api_handler.cpp                         ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     505                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/timeseries_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "timeseries/timeseries_metrics.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <chrono>

namespace themis {
namespace server {

TimeSeriesApiHandler::TimeSeriesApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<TSStore> ts_store,
    std::shared_ptr<ContinuousAggregateManager> agg_manager,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , ts_store_(std::move(ts_store))
    , agg_manager_(std::move(agg_manager))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> TimeSeriesApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesPut");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        if (!body.contains("metric") || !body.contains("entity") || !body.contains("value")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required fields: metric, entity, value", req);
        }
        
        std::string metric = body["metric"];
        std::string entity = body["entity"];
        
        // Build TSStore DataPoint directly from request
        TSStore::DataPoint ts_point;
        ts_point.metric = metric;
        ts_point.entity = entity;
        ts_point.value = body["value"].get<double>();
        ts_point.timestamp_ms = body.value("timestamp_ms", 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count());
        ts_point.tags = body.value("tags", nlohmann::json::object());
        ts_point.metadata = body.value("metadata", nlohmann::json::object());
        
        auto result = ts_store_->putDataPoint(ts_point);
        
        if (!result) {
            span.setStatus(false, "put_failed");
            return makeErrorResponse(http::status::internal_server_error, 
                result.error().message(), req);
        }
        
        nlohmann::json response = {
            {"success", true},
            {"metric", metric},
            {"entity", entity},
            {"timestamp_ms", ts_point.timestamp_ms}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::created, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleQuery(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesQuery");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Only 'metric' is required; 'entity' is optional per tests
        if (!body.contains("metric")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: metric", req);
        }
        
        std::string metric = body["metric"];
        
        TSStore::QueryOptions query_opts;
        query_opts.metric = metric;
        // Optional entity filter
        if (body.contains("entity") && !body["entity"].is_null()) {
            query_opts.entity = body["entity"].get<std::string>();
        }
        query_opts.from_timestamp_ms = body.value("from_ms", int64_t(0));
        query_opts.to_timestamp_ms = body.value("to_ms", INT64_MAX);
        query_opts.limit = body.value("limit", size_t(1000));
        if (body.contains("tags")) {
            query_opts.tag_filter = body["tags"];
        }
        
        auto result = ts_store_->query(query_opts);
        
        if (!result) {
            span.setStatus(false, "query_failed");
            return makeErrorResponse(http::status::internal_server_error, 
                result.error().message(), req);
        }
        
        auto& points = *result;
        
        nlohmann::json response = {
            {"metric", metric},
            {"count", points.size()},
            {"data", nlohmann::json::array()}
        };
        
        for (const auto& p : points) {
            nlohmann::json point_json = {
                {"entity", p.entity},
                {"timestamp_ms", p.timestamp_ms},
                {"value", p.value},
                {"tags", p.tags}
            };
            response["data"].push_back(point_json);
        }
        
        span.setAttribute("points_count", static_cast<int64_t>(points.size()));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleAggregate(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesAggregate");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Only 'metric' is required; 'entity' optional
        if (!body.contains("metric")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: metric", req);
        }
        
        std::string metric = body["metric"];
        
        TSStore::QueryOptions query_opts;
        query_opts.metric = metric;
        if (body.contains("entity") && !body["entity"].is_null()) {
            query_opts.entity = body["entity"].get<std::string>();
        }
        query_opts.from_timestamp_ms = body.value("from_ms", int64_t(0));
        query_opts.to_timestamp_ms = body.value("to_ms", INT64_MAX);
        query_opts.limit = body.value("limit", size_t(1000000)); // No limit for aggregation
        if (body.contains("tags")) {
            query_opts.tag_filter = body["tags"];
        }
        
        auto result = ts_store_->aggregate(query_opts);
        
        if (!result) {
            span.setStatus(false, "aggregate_failed");
            return makeErrorResponse(http::status::internal_server_error, 
                result.error().message(), req);
        }
        
        auto& agg = *result;
        
        nlohmann::json response = {
            {"metric", metric},
            {"aggregation", {
                {"min", agg.min},
                {"max", agg.max},
                {"avg", agg.avg},
                {"sum", agg.sum},
                {"count", agg.count},
                {"first_timestamp_ms", agg.first_timestamp_ms},
                {"last_timestamp_ms", agg.last_timestamp_ms}
            }}
        };
        
        span.setAttribute("agg_count", static_cast<int64_t>(agg.count));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleConfigGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesConfigGet");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    
    try {
        // Prefer persisted config if present so settings survive restarts
        auto stored = storage_->get("config:timeseries");
        nlohmann::json response;
        if (stored) {
            std::string s(stored->begin(), stored->end());
            response = nlohmann::json::parse(s);
        } else {
            const auto& config = ts_store_->getConfig();
            response = {
                {"compression", config.compression == TSStore::CompressionType::Gorilla ? "gorilla" : "none"},
                {"chunk_size_hours", config.chunk_size_hours}
            };
        }

        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleConfigPut(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesConfigPut");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        // Load current persisted config if present, otherwise use in-memory defaults
        nlohmann::json persisted;
        auto v = storage_->get("config:timeseries");
        if (v) {
            std::string s(v->begin(), v->end());
            persisted = nlohmann::json::parse(s);
        } else {
            const auto& cur = ts_store_->getConfig();
            persisted = {
                {"compression", cur.compression == TSStore::CompressionType::Gorilla ? "gorilla" : "none"},
                {"chunk_size_hours", cur.chunk_size_hours}
            };
        }

        // Apply updates from request body into persisted JSON
        if (body.contains("compression")) {
            if (!body["compression"].is_string()) {
                span.setStatus(false, "invalid_compression_type");
                return makeErrorResponse(http::status::bad_request, "compression must be a string", req);
            }
            std::string compression_str = body["compression"];
            if (compression_str != "gorilla" && compression_str != "none") {
                span.setStatus(false, "invalid_compression");
                return makeErrorResponse(http::status::bad_request, 
                    "Invalid compression type. Must be 'gorilla' or 'none'", req);
            }
            persisted["compression"] = compression_str;
        }

        if (body.contains("chunk_size_hours")) {
            if (!body["chunk_size_hours"].is_number_integer()) {
                span.setStatus(false, "invalid_chunk_size_type");
                return makeErrorResponse(http::status::bad_request, "chunk_size_hours must be an integer", req);
            }
            int chunk_size = body["chunk_size_hours"];
            if (chunk_size <= 0 || chunk_size > 168) { // Max 1 week
                span.setStatus(false, "invalid_chunk_size");
                return makeErrorResponse(http::status::bad_request, 
                    "chunk_size_hours must be between 1 and 168 (1 week)", req);
            }
            persisted["chunk_size_hours"] = chunk_size;
        }

        // Persist to storage
        std::string config_str = persisted.dump();
        std::vector<uint8_t> bytes(config_str.begin(), config_str.end());
        bool ok = storage_->put("config:timeseries", bytes);
        if (!ok) {
            span.setStatus(false, "storage_error");
            return makeErrorResponse(http::status::internal_server_error, "Failed to store timeseries config", req);
        }

        // Apply to in-memory TSStore (affects new data points only)
        TSStore::Config new_config = ts_store_->getConfig();
        if (persisted.contains("compression")) {
            std::string compression_str = persisted["compression"];
            new_config.compression = (compression_str == "gorilla") ? TSStore::CompressionType::Gorilla : TSStore::CompressionType::None;
        }
        if (persisted.contains("chunk_size_hours")) {
            new_config.chunk_size_hours = persisted["chunk_size_hours"];
        }
        ts_store_->setConfig(new_config);

        nlohmann::json response = persisted;
        response["status"] = "ok";
        response["note"] = "Configuration updated. Changes apply to new data points only.";

        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleAggregatesGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesAggregatesGet");
    try {
        // Minimal placeholder: list supported aggregate functions
        nlohmann::json response = {
            {"aggregates", nlohmann::json::array({"min","max","avg","sum","count"})}
        };
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleRetentionGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesRetentionGet");
    try {
        // Minimal placeholder: empty list of retention policies
        nlohmann::json response = {
            {"policies", nlohmann::json::array()}
        };
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::handleMetricsGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesMetricsGet");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    
    try {
        // Check format parameter from query string
        std::string format = "json"; // default
        std::string target = std::string(req.target());
        size_t query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = target.substr(query_pos + 1);
            // Check for format parameter with proper boundary checking
            if (query_string == "format=prometheus" || 
                query_string.find("format=prometheus&") == 0 ||
                query_string.find("&format=prometheus&") != std::string::npos ||
                query_string.find("&format=prometheus") == query_string.length() - 18) {
                format = "prometheus";
            }
        }
        
        auto metrics = ts_store_->getMetrics();
        if (!metrics) {
            nlohmann::json response = {
                {"error", false},
                {"message", "Metrics collection is not enabled for time series"},
                {"enabled", false}
            };
            span.setStatus(true);
            return makeResponse(http::status::ok, response.dump(), req);
        }
        
        // Also update stats from TSStore before exporting metrics
        auto stats = ts_store_->getStats();
        metrics->updateStorageStats(stats.total_data_points, stats.total_metrics, stats.total_size_bytes);
        
        if (format == "prometheus") {
            // Return Prometheus text format
            std::string prom_text = metrics->exportPrometheus();
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "THEMIS/0.1.0");
            res.set(http::field::content_type, "text/plain; version=0.0.4");
            res.keep_alive(req.keep_alive());
            res.body() = prom_text;
            res.prepare_payload();
            span.setStatus(true);
            return res;
        } else {
            // Return JSON format
            std::string json_text = metrics->exportJson();
            span.setStatus(true);
            return makeResponse(http::status::ok, json_text, req);
        }
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> TimeSeriesApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> TimeSeriesApiHandler::makeResponse(
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
