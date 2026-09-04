/**
 * @file timeseries_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/timeseries_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "timeseries/retention.h"
#include "timeseries/timeseries_metrics.h"
#include "timeseries/prometheus_remote_write.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <chrono>
#include <set>

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
    // TODO(W9-5): Wire setAggregatesProvider() after construction so that
    // handleAggregatesGet() returns live aggregate names instead of the
    // built-in fallback {min,max,avg,sum,count}.  The DI root that calls
    // this constructor should call:
    //   handler->setAggregatesProvider([[maybe_unused]] [engine]{ return engine->listAggregates(); });
    // where `engine` is an injected ContinuousAggMaterializationEngine.
    // See handleAggregatesGet() and AggregatesFn in timeseries_api_handler.h.
}

void TimeSeriesApiHandler::setRetentionPoliciesProviderFn([[maybe_unused]] RetentionPoliciesProviderFn fn) {
    std::lock_guard<std::mutex> lock(retentionPoliciesMutex_);
    retentionPoliciesFn_ = std::move(fn);
}

http::response<http::string_body> TimeSeriesApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTimeSeriesPut");
    
    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented, "Time-series feature not enabled", req);
    }
    auto& ts_store = *ts_store_;
    
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
        
        auto result = ts_store.putDataPoint(ts_point);
        
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
    auto& ts_store = *ts_store_;
    
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
        
        auto result = ts_store.query(query_opts);
        
        if (!result) {
            span.setStatus(false, "query_failed");
            return makeErrorResponse(http::status::internal_server_error, 
                result.error().message(), req);
        }
        
        auto& points = *result;
        
        nlohmann::json response = {
            {"metric", metric},
            {"count",static_cast<int>(points.size())},
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
    auto& ts_store = *ts_store_;
    
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
        
        auto result = ts_store.aggregate(query_opts);
        
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
    auto& ts_store = *ts_store_;
    
    try {
        // Prefer persisted config if present so settings survive restarts
        auto stored = storage_->get("config:timeseries");
        nlohmann::json response;
        if (stored) {
            std::string s(stored->begin(), stored->end());
            response = nlohmann::json::parse(s);
        } else {
            const auto& config = ts_store.getConfig();
            response = {
                {"compression", config.compression == TSStore::CompressionType::Gorilla ? "gorilla" : "none"},
                {"chunk_size_hours", config.chunk_size_hours},
                {"late_arrival_window_ms", config.late_arrival_window_ms}
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
    auto& ts_store = *ts_store_;
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        // Load current persisted config if present, otherwise use in-memory defaults
        nlohmann::json persisted;
        auto v = storage_->get("config:timeseries");
        if (v) {
            std::string s(v->begin(), v->end());
            persisted = nlohmann::json::parse(s);
        } else {
            const auto& cur = ts_store.getConfig();
            persisted = {
                {"compression", cur.compression == TSStore::CompressionType::Gorilla ? "gorilla" : "none"},
                {"chunk_size_hours", cur.chunk_size_hours},
                {"late_arrival_window_ms", cur.late_arrival_window_ms}
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

        if (body.contains("late_arrival_window_ms")) {
            if (!body["late_arrival_window_ms"].is_number_integer()) {
                span.setStatus(false, "invalid_late_arrival_window_type");
                return makeErrorResponse(http::status::bad_request, "late_arrival_window_ms must be an integer", req);
            }
            int64_t window = body["late_arrival_window_ms"].get<int64_t>();
            if (window < 0) {
                span.setStatus(false, "invalid_late_arrival_window");
                return makeErrorResponse(http::status::bad_request, 
                    "late_arrival_window_ms must be >= 0 (0 = disabled)", req);
            }
            persisted["late_arrival_window_ms"] = window;
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
        TSStore::Config new_config = ts_store.getConfig();
        if (persisted.contains("compression")) {
            std::string compression_str = persisted["compression"];
            new_config.compression = (compression_str == "gorilla") ? TSStore::CompressionType::Gorilla : TSStore::CompressionType::None;
        }
        if (persisted.contains("chunk_size_hours")) {
            new_config.chunk_size_hours = persisted["chunk_size_hours"];
        }
        if (persisted.contains("late_arrival_window_ms")) {
            new_config.late_arrival_window_ms = persisted["late_arrival_window_ms"].get<int64_t>();
        }
        ts_store.setConfig(new_config);

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
        std::set<std::string> aggregate_names;
        std::string aggregate_source = "builtin";
        bool degraded_mode = false;
 
        // STUB #301 REMEDIATION: Use real aggregates provider if available
        if (aggregates_fn_) {
            auto real_aggregates = aggregates_fn_();
            aggregate_names.insert(real_aggregates.begin(), real_aggregates.end());
            aggregate_source = "provider";
            span.setAttribute("aggregates.source", "real_provider");
        } else if (agg_engine_) {
            // Fall back to ContinuousAggMaterializationEngine for real registered aggregates
            auto real_aggregates = agg_engine_->listAggregates();
            aggregate_names.insert(real_aggregates.begin(), real_aggregates.end());
            aggregate_source = "agg_engine";
            span.setAttribute("aggregates.source", "agg_engine");
        } else {
            // STUB/SIMULATION NOTE:
            // Purpose: No aggregates provider (aggregates_fn_) or
            //          ContinuousAggMaterializationEngine (agg_engine_) has
            //          been injected at construction time. A fixed set of
            //          built-in aggregate function names is returned so the
            //          endpoint remains functional.
            // Activation: Both aggregates_fn_ and agg_engine_ are null,
            //             which is the default for unit tests and lightweight
            //             deployments that do not configure the aggregation
            //             subsystem.
            // Production Delta: Only the five built-in names are advertised;
            //                   custom or materialized aggregates are not
            //                   listed until a real provider is wired.
            // Removal Plan: Ensure aggregates_fn_ or agg_engine_ is always
            //               injected via TimeSeriesApiHandler::setAggregatesFn()
            //               or the relevant DI path, targeting v1.7.0 / Q4 2026.
            aggregate_names = {"min", "max", "avg", "sum", "count"};
            degraded_mode = true;
            span.setAttribute("aggregates.source", "builtin");
        }

        nlohmann::json materialized = nlohmann::json::array();
        if (storage_) {
            storage_->scanPrefix("wm:cagg:", [&materialized](std::string_view key, std::string_view value) {
                const std::string key_str(key);
                constexpr std::string_view kPrefix = "wm:cagg:";
                if (key_str.rfind(kPrefix, 0) == 0 && static_cast<int>(key_str.size()) > static_cast<int>(kPrefix.size())) {
                    materialized.push_back({
                        {"aggregate_id", key_str.substr(kPrefix.size())},
                        {"watermark_ms", std::string(value)}
                    });
                }
                return true;
            });
        }

        if (!materialized.empty()) {
            aggregate_names.insert("materialized");
        }

        nlohmann::json functions = nlohmann::json::array();
        for (const auto& name : aggregate_names) {
            functions.push_back(name);
        }

        nlohmann::json response = {
            {"aggregates", functions},
            {"materialized_aggregates", materialized},
            {"materialized_count",static_cast<int>(materialized.size())},
            {"source", aggregate_source},
            {"degraded_mode", degraded_mode}
        };
        if (degraded_mode) {
            response["degraded_reason"] =
                "No aggregate provider is wired; returning the builtin aggregate list only";
        }
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
        nlohmann::json policies = nlohmann::json::array();
        std::string policy_source = "storage_config";
        bool degraded_mode = false;
        
        // STUB #301 REMEDIATION: Use real retention policies provider if available
        if (retentions_fn_) {
            auto policy_map = retentions_fn_();
            for (const auto& [metric, retain_seconds] : policy_map) {
                policies.push_back({
                    {"metric", metric},
                    {"retain_seconds", retain_seconds},
                    {"source", "retention_provider"}
                });
            }
            policy_source = "provider";
            span.setAttribute("policies.source", "retention_provider");
        } else if (retentionPoliciesFn_) {
            // Also check legacy provider
            auto legacy_policies = retentionPoliciesFn_();
            policies = nlohmann::json(legacy_policies);
            policy_source = "legacy_provider";
            span.setAttribute("policies.source", "legacy_provider");
        } else {
            // Fall back to storage-based config
            degraded_mode = true;
            if (storage_) {
                auto stored = storage_->get("config:timeseries");
                if (stored) {
                    std::string serialized(stored->begin(), stored->end());
                    nlohmann::json cfg = nlohmann::json::parse(serialized, nullptr, false);
                    if (!cfg.is_discarded()) {
                        if (cfg.contains("retention_policies") && cfg["retention_policies"].is_array()) {
                            policies = cfg["retention_policies"];
                        } else if (cfg.contains("retention_policy") && cfg["retention_policy"].is_object()) {
                            policies.push_back(cfg["retention_policy"]);
                        }
                    }
                }
            }
            span.setAttribute("policies.source", "storage_config");
        }

        // Always add TSStore's late-arrival window if configured
        if (ts_store_) {
            const auto& config = ts_store_->getConfig();
            if (config.late_arrival_window_ms > 0) {
                policies.push_back({
                    {"metric", ""},
                    {"name", "late_arrival_window"},
                    {"window_ms", config.late_arrival_window_ms},
                    {"source", "tsstore_config"}
                });
            }
        }

        nlohmann::json response = {
            {"policies", policies},
            {"policy_count",static_cast<int>(policies.size())},
            {"source", policy_source},
            {"degraded_mode", degraded_mode}
        };
        if (degraded_mode) {
            response["degraded_reason"] =
                "No retention provider is wired; returning storage-derived retention metadata only";
        }
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
    auto& ts_store = *ts_store_;
    
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
        
        auto metrics = ts_store.getMetrics();
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
        auto stats = ts_store.getStats();
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

http::response<http::string_body> TimeSeriesApiHandler::handlePrometheusRemoteWrite(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handlePrometheusRemoteWrite");

    if (!ts_store_) {
        span.setStatus(false, "feature_disabled");
        return makeErrorResponse(http::status::not_implemented,
                                 "Time-series feature not enabled", req);
    }
    auto& ts_store = *ts_store_;

    try {
        const std::string& body = req.body();
        if (body.empty()) {
            span.setStatus(false, "empty_body");
            return makeErrorResponse(http::status::bad_request,
                                     "Request body is empty", req);
        }

        // Determine encoding from Content-Encoding header (default: snappy)
        std::string content_encoding = "snappy";
        if (req.count(http::field::content_encoding)) {
            content_encoding = std::string(req[http::field::content_encoding]);
        }

        // Reject unsupported encodings early
        if (content_encoding != "snappy" && content_encoding != "identity"
                && !content_encoding.empty()) {
            span.setStatus(false, "unsupported_encoding");
            return makeErrorResponse(http::status::bad_request,
                                     "Unsupported Content-Encoding: " + content_encoding
                                     + " (expected 'snappy')", req);
        }

        // Decode the Prometheus WriteRequest
        themis::Result<themis::timeseries::PromWriteRequest> decode_result =
            (content_encoding == "identity")
            ? themis::timeseries::PromWriteRequest::decode(
                reinterpret_cast<const uint8_t*>(body.data()),static_cast<int>(body.size()))
            : themis::timeseries::PromWriteRequest::decodeSnappy(
                reinterpret_cast<const uint8_t*>(body.data()),static_cast<int>(body.size()));

        if (!decode_result) {
            span.setStatus(false, "decode_failed");
            return makeErrorResponse(http::status::bad_request,
                                     "Failed to decode Prometheus WriteRequest: "
                                     + decode_result.error().message(), req);
        }

        const auto& write_req = *decode_result;
        size_t accepted = 0;
        size_t rejected = 0;

        // Convert each Prometheus TimeSeries + Sample to a TSStore DataPoint
        std::vector<TSStore::DataPoint> batch;
        batch.reserve(64);

        for (const auto& ts : write_req.timeseries) {
            const std::string metric_name = ts.metricName();
            if (metric_name.empty()) {
                // Skip time series without a metric name label
                rejected += ts.samples.size();
                continue;
            }

            // Build a tags object from all non-__name__ labels
            nlohmann::json tags = nlohmann::json::object();
            std::string entity = {};
            for (const auto& label : ts.labels) {
                if (label.name == "__name__") {
                  continue;
                }
                if (label.name == "instance") {
                    entity = label.value;
                }
                tags[label.name] = label.value;
            }
            // Fall back to "default" entity when no 'instance' label present
            if (entity.empty()) {
              entity = "default";
            }

            for (const auto& sample : ts.samples) {
                TSStore::DataPoint dp;
                dp.metric       = metric_name;
                dp.entity       = entity;
                dp.timestamp_ms = sample.timestamp_ms;
                dp.value        = sample.value;
                dp.tags         = tags;
                batch.push_back(std::move(dp));
            }
        }

        if (!batch.empty()) {
            auto result = ts_store.putDataPoints(batch);
            if (!result) {
                span.setStatus(false, "put_failed");
                return makeErrorResponse(http::status::internal_server_error,
                                         "Failed to store time series data: "
                                         + result.error().message(), req);
            }
            accepted = batch.size();
        }

        span.setAttribute("accepted_samples", static_cast<int64_t>(accepted));
        span.setAttribute("rejected_samples", static_cast<int64_t>(rejected));
        span.setStatus(true);

        // Prometheus remote-write spec: return 204 No Content on success
        http::response<http::string_body> res{http::status::no_content, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;

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
