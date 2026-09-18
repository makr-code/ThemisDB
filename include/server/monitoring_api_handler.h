/**
 * @file monitoring_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class MonitoringApiHandler {
public:
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
     * @brief Handle Health Check.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealthCheck(const http::request<http::string_body>& req);

    /**
     * @brief Handle Liveness.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLiveness(const http::request<http::string_body>& req);

    /**
     * @brief Handle Readiness.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReadiness(const http::request<http::string_body>& req);

    /**
     * @brief Handle Open Api.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOpenApi(const http::request<http::string_body>& req);

    /**
     * @brief Register Routes.
     */
    static void registerRoutes();

    /**
     * @brief Handle Version.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleVersion(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Capabilities.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCapabilities(const http::request<http::string_body>& req);

    /**
     * @brief Handle Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);

    /**
     * @brief Handle Metrics Html.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetricsHtml(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Plugin Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePluginMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Sharding Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleShardingMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle SLOStatus.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSLOStatus(const http::request<http::string_body>& req);

    // -------------------------------------------------------------------------
    // Operator Observability REST API
    // -------------------------------------------------------------------------

    /**
     * @brief Handle Observability Alerts.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleObservabilityAlerts(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Observability Alert Silence.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleObservabilityAlertSilence(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Observability Health.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleObservabilityHealth(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Observability Provenance.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleObservabilityProvenance(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle License Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLicenseStatus(
        const http::request<http::string_body>& req);

    /**
     * @brief Set Concerns.
     * @param[in] concerns Input parameter.
     * @details Calls: std::move().
     */
    void setConcerns(std::shared_ptr<core::concerns::ConcernsContext> concerns) {
        concerns_ = std::move(concerns);
    }

    /**
     * @brief Set Alertmanager.
     * @param[in] alertmanager Input parameter.
     * @details Calls: std::move().
     */
    void setAlertmanager(std::shared_ptr<observability::DefaultAlertmanager> alertmanager) {
        alertmanager_ = std::move(alertmanager);
    }

    /**
     * @brief Set Provenance Store.
     * @param[in] provenance_store Input parameter.
     * @details Calls: std::move().
     */
    void setProvenanceStore(std::shared_ptr<observability::IProvenanceStore> provenance_store) {
        provenance_store_ = std::move(provenance_store);
    }

    /**
     * @brief Set Sharding Metrics.
     * @param[in] sharding_metrics Input parameter.
     * @details Calls: std::move().
     */
    void setShardingMetrics(std::shared_ptr<ShardingMetricsHandler> sharding_metrics) {
        sharding_metrics_ = std::move(sharding_metrics);
    }

    void setSchemaManager(::themis::SchemaManager* schema_manager) {
        schema_manager_ = schema_manager;
    }

    /**
     * @brief Set Continuous Learning Orchestrator.
     * @param[in] orchestrator Input parameter.
     * @details Calls: std::move().
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

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);

    /**
     * @brief Build Concerns Json.
     * @param[in] status Input parameter.
     * @param[in,out] ok Input/output parameter.
     * @return Return value.
     */
    static json buildConcernsJson(
        const core::concerns::HealthStatus& status, bool& ok);
};

} // namespace server
} // namespace themis
