/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            timeseries_api_handler.h                           ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:28:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     174                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • fd0023de98  2026-02-28  feat(timeseries): implement Prometheus remote-write endpo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class TSStore;
class ContinuousAggregateManager;

namespace server {

/**
 * @brief Handler for Time Series Operations
 * 
 * This handler manages all time series-related endpoints:
 * - POST /ts/put - Insert time series data points
 * - POST /ts/query - Query time series data
 * - POST /ts/aggregate - Execute aggregation queries
 * - GET /ts/config - Get time series configuration
 * - PUT /ts/config - Update time series configuration
 * - GET /ts/aggregates - List continuous aggregates
 * - GET /ts/retention - Get retention policies
 * 
 * Features:
 * - High-performance time series storage
 * - Gorilla compression
 * - Continuous aggregates
 * - Retention policies
 * - Downsampling support
 * 
 * Extracted from http_server.cpp (~350 lines) to improve maintainability.
 */
class TimeSeriesApiHandler {
public:
    /**
     * @brief Construct a new Time Series API Handler
     * 
     * @param storage Storage backend
     * @param ts_store Time series storage engine
     * @param agg_manager Continuous aggregate manager
     * @param auth Authentication/authorization middleware
     */
    TimeSeriesApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<TSStore> ts_store,
        std::shared_ptr<ContinuousAggregateManager> agg_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /ts/put request
     * @param req HTTP request with time series data points
     * @return HTTP response with insertion status
     */
    http::response<http::string_body> handlePut(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /ts/query request
     * @param req HTTP request with query parameters
     * @return HTTP response with time series data
     */
    http::response<http::string_body> handleQuery(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /ts/aggregate request
     * @param req HTTP request with aggregation specification
     * @return HTTP response with aggregated results
     */
    http::response<http::string_body> handleAggregate(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ts/config request
     * @param req HTTP request
     * @return HTTP response with current configuration
     */
    http::response<http::string_body> handleConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /ts/config request
     * @param req HTTP request with new configuration
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ts/aggregates request
     * @param req HTTP request
     * @return HTTP response with list of continuous aggregates
     */
    http::response<http::string_body> handleAggregatesGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ts/retention request
     * @param req HTTP request
     * @return HTTP response with retention policies
     */
    http::response<http::string_body> handleRetentionGet(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /ts/metrics request
     * @param req HTTP request
     * @param format Output format: "json" or "prometheus" (default: json)
     * @return HTTP response with time series metrics
     */
    http::response<http::string_body> handleMetricsGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/prom/write – Prometheus remote-write endpoint
     *
     * Accepts a snappy-compressed Protocol Buffer payload conforming to the
     * Prometheus remote-write 1.0 specification and stores all received
     * samples in the TSStore.
     *
     * Expected request headers:
     *   Content-Encoding: snappy
     *   Content-Type: application/x-protobuf
     *   X-Prometheus-Remote-Write-Version: 0.1.0
     *
     * On success the handler returns HTTP 204 No Content as specified by
     * the Prometheus remote-write spec.
     *
     * @param req HTTP request carrying the snappy-encoded WriteRequest body.
     * @return HTTP 204 on success, 400 on malformed input, 501 when the
     *         time-series feature is disabled.
     */
    http::response<http::string_body> handlePrometheusRemoteWrite(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<TSStore> ts_store_;
    std::shared_ptr<ContinuousAggregateManager> agg_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
