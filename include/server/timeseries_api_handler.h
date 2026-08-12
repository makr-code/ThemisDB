/**
 * @file timeseries_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class TSStore;
class ContinuousAggregateManager;
class ContinuousAggMaterializationEngine;
class RetentionManager;

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
     * @brief Provider function type for runtime retention-policy introspection.
     *
     * Each returned JSON object should have at least the following keys:
     *   - "metric"          : string — metric name the policy applies to
     *                         (empty string means a global / catch-all policy)
     *   - "retain_seconds"  : number — retention window in seconds (0 = unlimited)
     *
     * The function is called once per request to @c handleRetentionGet; it must
     * be thread-safe.
     */
    using RetentionPoliciesProviderFn = std::function<std::vector<nlohmann::json>()>;

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
     * @brief Inject a runtime retention-policy provider (stub #301 resolved).
     *
     * When set, @c handleRetentionGet queries this function for the active
     * retention policies instead of returning an empty list.  The provider
     * must be thread-safe; it is called under no internal lock.
     *
     * @param fn Callable returning active retention policies as JSON objects.
     *           Each object must contain at least "metric" and "retain_seconds".
     */
    void setRetentionPoliciesProviderFn(RetentionPoliciesProviderFn fn);

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
     * @brief Inject a provider for the supported-aggregate-types list.
     *
     * When set, `handleAggregatesGet()` calls the provider to obtain the
     * current aggregate function names from the running TSStore / engine
     * instead of returning the built-in static list.
     *
     * @param fn  Callable returning a JSON array of aggregate name strings.
     *            Pass `nullptr` to revert to the static default list.
     */
    using AggregateTypesProviderFn = std::function<nlohmann::json()>;
    void setAggregateTypesProvider(AggregateTypesProviderFn fn);

    /**
     * @brief Inject a provider for the active retention-policy list.
     *
     * When set, `handleRetentionGet()` calls the provider to obtain the
     * persisted retention policies from the backend instead of returning
     * an empty list.
     */
    
    /**
     * @brief Handle GET /ts/metrics request
     * @param req HTTP request
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

    // -------------------------------------------------------------------------
    // Metadata-provider injection (stub #301)
    // -------------------------------------------------------------------------

    /// Callback type that returns the list of supported aggregate-function names.
    /// When set via setAggregatesProvider(), handleAggregatesGet() delegates to
    /// this function instead of returning the built-in static list.
    using AggregatesFn = std::function<std::vector<std::string>()>;

    /// Callback type that returns retention-policy metadata as a map of
    /// metric name → retention seconds (0 = no explicit policy).
    /// When set via setRetentionPoliciesProvider(), handleRetentionGet()
    /// delegates to this function instead of returning an empty policy list.
    using RetentionsFn = std::function<std::map<std::string, int64_t>()>;

    /// @brief Inject a provider that supplies real aggregate-function names.
    /// @param fn Callable returning a vector of aggregate names; pass nullptr
    ///           to revert to the built-in static list.
    void setAggregatesProvider(AggregatesFn fn) { aggregates_fn_ = std::move(fn); }

    /// @brief Inject a provider that supplies live retention-policy metadata.
    /// @param fn Callable returning metric→retention-seconds map; pass nullptr
    ///           to revert to the built-in empty-list response.
    void setRetentionPoliciesProvider(RetentionsFn fn) { retentions_fn_ = std::move(fn); }

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<TSStore> ts_store_;
    std::shared_ptr<ContinuousAggregateManager> agg_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    AggregateTypesProviderFn aggregate_types_provider_;
    RetentionPoliciesProviderFn retention_policies_provider_;

    /// Optional: exposes listAggregates() for the /aggregates endpoint.
    std::shared_ptr<ContinuousAggMaterializationEngine> agg_engine_;
    /// Optional: exposes getPolicy() for the /retention endpoint.
    std::shared_ptr<RetentionManager> retention_manager_;

    // Retention-policy injection bridge (stub #301)
    RetentionPoliciesProviderFn retentionPoliciesFn_;
    mutable std::mutex retentionPoliciesMutex_;

    AggregatesFn aggregates_fn_;  ///< Optional live aggregates provider (stub #301)
    RetentionsFn retentions_fn_;  ///< Optional live retention-policy provider (stub #301)

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
