/**
 * @file cache_admin_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "cache/adaptive_query_cache.h"
#include "cache/cache_hit_rate_slo_monitor.h"

#include <memory>
#include <mutex>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

class CacheAdminApiHandler {
public:
    CacheAdminApiHandler(
        std::shared_ptr<AdaptiveQueryCache> cache,
        std::shared_ptr<AuthMiddleware> auth
    );

    /**
     * @brief Handle Health.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealth(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Evict Key.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEvictKey(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Evict Tenant.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEvictTenant(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Circuit Breaker Reset.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCircuitBreakerReset(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Circuit Breaker Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCircuitBreakerStatus(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Warmup.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleWarmup(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Snapshot.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSnapshot(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle List Tenants.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListTenants(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Tenant Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTenantStats(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Update Tenant Quota.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateTenantQuota(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Pii Evict.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiEvict(
        const http::request<http::string_body>& req);

    /**
     * @brief Set Slo Monitor.
     * @param[in] monitor Input parameter.
     */
    void setSloMonitor(std::shared_ptr<themis::cache::CacheHitRateSloMonitor> monitor);

private:
    std::shared_ptr<AdaptiveQueryCache> cache_;
    std::shared_ptr<AuthMiddleware> auth_;
    mutable std::mutex slo_monitor_mutex_;
    std::shared_ptr<themis::cache::CacheHitRateSloMonitor> slo_monitor_;

    /**
     * @brief Check Auth.
     * @param[in] req Input parameter.
     * @param[in] required_scope Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool checkAuth(const http::request<http::string_body>& req,
                   const std::string& required_scope,
                   http::response<http::string_body>& out);

    /**
     * @brief Extract Path Param.
     * @param[in] target Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    static std::string extractPathParam(std::string_view target,
                                        std::string_view prefix);

    /**
     * @brief Base64 Decode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string base64Decode(const std::string& input);

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req);

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
