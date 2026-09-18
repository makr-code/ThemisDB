/**
 * @file timeseries_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class TimeSeriesApiHandler {
public:
    using RetentionPoliciesProviderFn = std::function<std::vector<nlohmann::json>()>;

    TimeSeriesApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<TSStore> ts_store,
        std::shared_ptr<ContinuousAggregateManager> agg_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Set Retention Policies Provider Fn.
     * @param[in] fn Input parameter.
     */
    void setRetentionPoliciesProviderFn(RetentionPoliciesProviderFn fn);

    /**
     * @brief Handle Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Query.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQuery(const http::request<http::string_body>& req);

    /**
     * @brief Handle Aggregate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAggregate(const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Aggregates Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAggregatesGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Retention Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionGet(const http::request<http::string_body>& req);

    using AggregateTypesProviderFn = std::function<nlohmann::json()>;
    /**
     * @brief Set Aggregate Types Provider.
     * @param[in] fn Input parameter.
     */
    void setAggregateTypesProvider(AggregateTypesProviderFn fn);

    
    /**
     * @brief Handle Metrics Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetricsGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Prometheus Remote Write.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePrometheusRemoteWrite(const http::request<http::string_body>& req);

    // -------------------------------------------------------------------------
    // Metadata-provider injection (stub #301)
    // -------------------------------------------------------------------------

    using AggregatesFn = std::function<std::vector<std::string>()>;

    using RetentionsFn = std::function<std::map<std::string, int64_t>()>;

    /**
     * @brief Set Aggregates Provider.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setAggregatesProvider(AggregatesFn fn) { aggregates_fn_ = std::move(fn); }

    /**
     * @brief Set Retention Policies Provider.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setRetentionPoliciesProvider(RetentionsFn fn) { retentions_fn_ = std::move(fn); }

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<TSStore> ts_store_;
    std::shared_ptr<ContinuousAggregateManager> agg_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    AggregateTypesProviderFn aggregate_types_provider_;
    RetentionPoliciesProviderFn retention_policies_provider_;

    std::shared_ptr<ContinuousAggMaterializationEngine> agg_engine_;
    std::shared_ptr<RetentionManager> retention_manager_;

    // Retention-policy injection bridge (stub #301)
    RetentionPoliciesProviderFn retentionPoliciesFn_;
    mutable std::mutex retentionPoliciesMutex_;

    AggregatesFn aggregates_fn_;  ///< Optional live aggregates provider (stub #301)
    RetentionsFn retentions_fn_;  ///< Optional live retention-policy provider (stub #301)

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
};

} // namespace server
} // namespace themis
