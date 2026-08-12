/**
 * @file monitoring_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include "core/concerns/concerns_context.h"
#include "observability/alertmanager.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;

namespace sharding {
class PrometheusMetrics;
class SLOMonitor;
}

namespace observability {
class IProvenanceStore;
}

namespace rag::learning {
class ContinuousLearningOrchestrator;
}

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

class ShardingMetricsHandler;

} // namespace server
} // namespace themis

namespace themis {
class AuthMiddleware;
class SchemaManager;
} // namespace themis

namespace themis {
namespace server {

/**
 * @brief Handler for Monitoring and System Information
 * 
 * This handler manages all monitoring and system information endpoints:
 * - GET / or GET /health - Health check endpoint
 * - GET /version - Get version information
 * - GET /stats - Get runtime statistics
 * - GET /api/capabilities - Get server capabilities
 * - GET /metrics - Get Prometheus-compatible metrics
 * - GET /metrics/html - Lightweight HTML metrics dashboard
 * - GET /config or POST /config - Get/update server configuration
 * - GET /api/v1/observability/alerts - List active alerts (JSON)
 * - POST /api/v1/observability/alerts/{id}/silence - Silence an alert
 * - GET /api/v1/observability/health - Aggregate observability health
 * - GET /api/v1/observability/provenance - Query persisted retrieval provenance records
 * 
 * Features:
 * - Health monitoring
 * - Version information
 * - Runtime statistics
 * - Prometheus metrics export
 * - Dynamic configuration
 * - Operator alert management API
 * 
 * Extracted from http_server.cpp (~300 lines) to improve maintainability.
 */
class MonitoringApiHandler {
public:
    /**
     * @brief Construct a new Monitoring API Handler
     * 
     * @param storage Storage backend
     * @param auth Authentication/authorization middleware  
     * @param request_count Request counter (shared atomic)
     * @param error_count Error counter (shared atomic)
     * @param start_time Server start time (shared)
     * @param secondary_index Secondary index manager (for stats)
     * @param schema_manager Schema manager (for capabilities, optional)
     * @param sharding_metrics Sharding metrics handler (for metrics/slo endpoints, optional)
     * @param is_running Flag indicating whether the server is running (optional)
     * @param active_requests Active request counter (optional)
     * @param active_connections Active connection counter (optional)
     * @param concerns ConcernsContext for lifecycle and health probes (optional).
     *        When provided, both handleLiveness() and handleReadiness() include
     *        per-concern health status from concerns->healthCheck() and
     *        concerns->readinessCheck() respectively.  If any concern is
     *        unhealthy, the probe returns HTTP 503.
     */
    MonitoringApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<AuthMiddleware> auth,
        std::atomic<uint64_t>* request_count,
        std::atomic<uint64_t>* error_count,
        const std::chrono::steady_clock::time_point* start_time,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        ::themis::SchemaManager* schema_manager = nullptr,
        std::shared_ptr<ShardingMetricsHandler> sharding_metrics = nullptr,
        const std::atomic<bool>* is_running = nullptr,
        const std::atomic<uint64_t>* active_requests = nullptr,
        const std::atomic<uint64_t>* active_connections = nullptr,
        std::shared_ptr<core::concerns::ConcernsContext> concerns = nullptr
    );

    /**
     * @brief Handle GET / or GET /health request
     * @param req HTTP request
     * @return HTTP response with health status
     */
    http::response<http::string_body> handleHealthCheck(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /health/live request (liveness probe)
     * Returns 200 if server is running and all core concerns are healthy.
     * Returns 503 otherwise.
     * When a ConcernsContext is supplied, the response includes a
     * "concerns" section with per-concern health details.
     * @param req HTTP request
     * @return HTTP response with liveness status
     */
    http::response<http::string_body> handleLiveness(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /health/ready request (readiness probe)
     * Returns 200 if storage is accessible, server is ready to serve
     * traffic, and all core concerns pass readiness checks.
     * Returns 503 if not ready (e.g. storage unavailable, still starting
     * up, or a core concern reports not ready).
     * When a ConcernsContext is supplied, the response includes a
     * "concerns" section with per-concern readiness details.
     * @param req HTTP request
     * @return HTTP response with readiness status
     */
    http::response<http::string_body> handleReadiness(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/openapi.json request
     * Returns an OpenAPI 3.1 specification describing the ThemisDB REST API.
     * The document is assembled from route annotations registered via
     * RouteRegistry::instance(), so adding new handler registrations
     * automatically extends the generated specification.
     * @param req HTTP request
     * @return HTTP response with OpenAPI 3.1 JSON document
     */
    http::response<http::string_body> handleOpenApi(const http::request<http::string_body>& req);

    /**
     * @brief Register all monitoring-handler routes into the global RouteRegistry.
     *
     * Called once during server startup so that handleOpenApi() can produce an
     * up-to-date OpenAPI 3.1 document without any manually maintained JSON
     * duplication.  Safe to call multiple times (last registration wins).
     */
    static void registerRoutes();

    /**
     * @brief Handle GET /version request
     * @param req HTTP request
     * @return HTTP response with version information
     */
    http::response<http::string_body> handleVersion(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /stats request
     * @param req HTTP request
     * @return HTTP response with runtime statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/capabilities request
     * @param req HTTP request
     * @return HTTP response with server capabilities
     */
    http::response<http::string_body> handleCapabilities(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /metrics request (Prometheus format)
     * @param req HTTP request
     * @return HTTP response with metrics in Prometheus format
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /metrics/html request (lightweight HTML dashboard)
     * Renders the current Prometheus metrics as a human-readable HTML page
     * for quick operator inspection without a dedicated Grafana instance.
     * @param req HTTP request
     * @return HTTP response with HTML metrics dashboard
     */
    http::response<http::string_body> handleMetricsHtml(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /api/plugins/metrics request
     * @param req HTTP request
     * @return HTTP response with plugin metrics in JSON format
     */
    http::response<http::string_body> handlePluginMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /metrics/sharding request (Sharding metrics in Prometheus format)
     * @param req HTTP request
     * @return HTTP response with sharding metrics
     */
    http::response<http::string_body> handleShardingMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /slo or GET /api/slo request (SLO status in JSON)
     * @param req HTTP request
     * @return HTTP response with SLO compliance and error budgets
     */
    http::response<http::string_body> handleSLOStatus(const http::request<http::string_body>& req);

    // -------------------------------------------------------------------------
    // Operator Observability REST API
    // -------------------------------------------------------------------------

    /**
     * @brief GET /api/v1/observability/alerts
     * Returns the list of currently active (firing) alerts as a JSON array.
     * @param req HTTP request
     * @return HTTP 200 with JSON array of active alerts
     */
    http::response<http::string_body> handleObservabilityAlerts(
        const http::request<http::string_body>& req);

    /**
     * @brief POST /api/v1/observability/alerts/{id}/silence
     * Silences the named alert for a configurable duration.
     * Body: { "duration_minutes": <int> }  (default: 60)
     * @param req HTTP request (path contains alert_id)
     * @return HTTP 200 on success, 400/404 on error
     */
    http::response<http::string_body> handleObservabilityAlertSilence(
        const http::request<http::string_body>& req);

    /**
     * @brief GET /api/v1/observability/health
     * Returns aggregate observability subsystem health:
     * Alertmanager status, tracing status, metrics collector stats,
     * and exporter health counters.
     * @param req HTTP request
     * @return HTTP 200 with JSON health document
     */
    http::response<http::string_body> handleObservabilityHealth(
        const http::request<http::string_body>& req);

    /**
     * @brief GET /api/v1/observability/provenance
     * Returns persisted retrieval provenance records for operational analysis.
     *
     * Supported query parameters:
     * - query_id=<id>: return full provenance chain for one query.
     * - start_ts_ms=<epoch_ms>&end_ts_ms=<epoch_ms>: return records in time window.
     * - limit=<n>: cap returned records (default 1000, max 10000).
     *
     * If both query_id and a time window are provided, the query chain is
     * filtered to records inside the time window.
     *
     * @param req HTTP request
     * @return HTTP 200 with JSON response, 400 for invalid query params, 503
     *         when no provenance store is configured
     */
    http::response<http::string_body> handleObservabilityProvenance(
        const http::request<http::string_body>& req);

    /**
     * @brief GET /api/v1/license/status
     * Returns runtime license state as a JSON document:
     *   - initialized:      whether the RuntimeLicenseGate has been set up
     *   - status:           "active" | "grace" | "expired" | "invalid" | ...
     *   - grace_days_remaining: days left in the grace period (0 if not in grace)
     *   - organization:     licensee organisation name (from embedded license)
     *   - edition:          "COMMUNITY" | "ENTERPRISE" | "HYPERSCALER"
     *   - expiry_date:      ISO 8601 date (YYYY-MM-DD), empty if perpetual
     *   - days_until_expiry: integer; negative means already expired
     *   - license_key:      first 8 chars + "..." (masked for security)
     * @param req HTTP request
     * @return HTTP 200 with JSON license status document
     */
    http::response<http::string_body> handleLicenseStatus(
        const http::request<http::string_body>& req);

    /**
     * @brief Replace the ConcernsContext used for health/readiness probes.
     *
     * Can be called after construction (e.g. from HttpServer::setConcerns()).
     * Thread-safety: must not be called concurrently with handleLiveness() or
     * handleReadiness(); call only during server initialization.
     */
    void setConcerns(std::shared_ptr<core::concerns::ConcernsContext> concerns) {
        concerns_ = std::move(concerns);
    }

    /**
     * @brief Set the Alertmanager instance used by the Operator REST API.
     *
     * Optional: when not set, the observability alert endpoints return empty
     * lists / a disabled status instead of errors.
     */
    void setAlertmanager(std::shared_ptr<observability::DefaultAlertmanager> alertmanager) {
        alertmanager_ = std::move(alertmanager);
    }

    /**
     * @brief Set the provenance store used by observability export endpoints.
     *
     * Optional: when not set, provenance export endpoints return service
     * unavailable instead of failing implicitly.
     */
    void setProvenanceStore(std::shared_ptr<observability::IProvenanceStore> provenance_store) {
        provenance_store_ = std::move(provenance_store);
    }

    /**
     * @brief Inject sharding / repair metrics into the monitoring handler.
     *
     * Optional: when set, GET /metrics and GET /v1/monitoring/sharding/{name} expose
     * shard-level Prometheus metrics including anti-entropy repair statistics.
     * Must be called before start() for the first scrape to include repair data.
     */
    void setShardingMetrics(std::shared_ptr<ShardingMetricsHandler> sharding_metrics) {
        sharding_metrics_ = std::move(sharding_metrics);
    }

    /**
     * @brief Inject the SchemaManager after deferred initialization.
     *
     * SchemaManager is initialized after MonitoringApiHandler due to construction
     * ordering constraints (schema_manager_ depends on storage being fully open).
     * Call this from HttpServer once schema_manager_ is available so that
     * GET /api/v1/capabilities returns accurate schema capability information.
     *
     * Optional: when not set the capabilities endpoint omits schema details.
     */
    void setSchemaManager(::themis::SchemaManager* schema_manager) {
        schema_manager_ = schema_manager;
    }

    /**
     * @brief Inject the continuous-learning orchestrator for live ML loop status.
     *
     * When set, GET /stats and GET /metrics/html include the latest serialized
     * signal and guardrail context for the live continuous-learning loops.
     */
    void setContinuousLearningOrchestrator(
        std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator> orchestrator) {
        continuous_learning_orchestrator_ = std::move(orchestrator);
    }

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<AuthMiddleware> auth_;
    std::atomic<uint64_t>* request_count_;
    std::atomic<uint64_t>* error_count_;
    const std::chrono::steady_clock::time_point* start_time_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    ::themis::SchemaManager* schema_manager_;
    std::shared_ptr<ShardingMetricsHandler> sharding_metrics_;
    const std::atomic<bool>* is_running_{nullptr};
    const std::atomic<uint64_t>* active_requests_{nullptr};
    const std::atomic<uint64_t>* active_connections_{nullptr};
    std::shared_ptr<core::concerns::ConcernsContext> concerns_;
    std::shared_ptr<observability::DefaultAlertmanager> alertmanager_;
    std::shared_ptr<observability::IProvenanceStore> provenance_store_;
    std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator>
        continuous_learning_orchestrator_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);

    /// Build the JSON "concerns" block from a HealthStatus and update @p ok.
    static json buildConcernsJson(
        const core::concerns::HealthStatus& status, bool& ok);
};

} // namespace server
} // namespace themis
