/**
 * @file cache_admin_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Admin API handler for AdaptiveQueryCache operations.
 *
 * Exposes the following REST endpoints (all under /v1/admin/cache/):
 *
 *   GET    /v1/admin/cache/stats                       – JSON snapshot of cache metrics
 *   DELETE /v1/admin/cache/key/{encoded_key}           – Evict one entry (key base64-encoded)
 *   DELETE /v1/admin/cache/tenant/{tenant_id}          – Evict all entries for a tenant
 *   POST   /v1/admin/cache/circuit-breaker/reset       – Force circuit breaker to CLOSED
 *   GET    /v1/admin/cache/circuit-breaker             – Current circuit breaker state
 *   POST   /v1/admin/cache/warmup                      – Load entries from NDJSON log file
 *   POST   /v1/admin/cache/snapshot                    – Export live entries to NDJSON file
 *   GET    /v1/admin/cache/tenants                     – Per-tenant stats (all tenants)
 *   GET    /v1/admin/cache/tenant/{tenant_id}/stats    – Per-tenant stats (single tenant)
 *   PATCH  /v1/admin/cache/tenant/{tenant_id}/quota   – Update quota for a specific tenant
 *   DELETE /v1/admin/cache/pii/{pii_uuid}              – GDPR Art.17 PII purge across all tiers
 *
 * Authentication: requires JWT with "admin:cache:read" scope for read endpoints
 * and "admin:cache:write" scope for write/mutation endpoints.
 */
class CacheAdminApiHandler {
public:
    CacheAdminApiHandler(
        std::shared_ptr<AdaptiveQueryCache> cache,
        std::shared_ptr<AuthMiddleware> auth
    );

    /** GET /v1/admin/cache/health
     *
     * Returns per-tier status (L1/L2/L3) and circuit breaker state.
     * Responds with HTTP 200 when healthy, 503 when degraded/unavailable.
     */
    http::response<http::string_body> handleHealth(
        const http::request<http::string_body>& req);

    /** GET /v1/admin/cache/stats */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req);

    /** DELETE /v1/admin/cache/key/{encoded_key}
     *
     * Evicts all cache entries (across L1, L2, and L3) whose fingerprint
     * matches the provided key.  The key must be the base64-encoded SHA-256
     * fingerprint of the query as returned by
     * AdaptiveQueryCache::generateFingerprint().  Entries for all tenants
     * sharing that fingerprint are evicted.
     */
    http::response<http::string_body> handleEvictKey(
        const http::request<http::string_body>& req);

    /** DELETE /v1/admin/cache/tenant/{tenant_id} */
    http::response<http::string_body> handleEvictTenant(
        const http::request<http::string_body>& req);

    /** POST /v1/admin/cache/circuit-breaker/reset */
    http::response<http::string_body> handleCircuitBreakerReset(
        const http::request<http::string_body>& req);

    /** GET /v1/admin/cache/circuit-breaker */
    http::response<http::string_body> handleCircuitBreakerStatus(
        const http::request<http::string_body>& req);

    /**
     * POST /v1/admin/cache/warmup
     *
     * Body: {"log_path":"<path>","max_entries":<optional uint>}
     *
     * Loads cache entries from the NDJSON log at log_path.
     * Returns a JSON summary of the load operation.
     */
    http::response<http::string_body> handleWarmup(
        const http::request<http::string_body>& req);

    /**
     * POST /v1/admin/cache/snapshot
     *
     * Body: {"out_path":"<path>"}
     *
     * Exports all live L1/L2 entries to the specified NDJSON file.
     * Returns a JSON summary of the export operation.
     */
    http::response<http::string_body> handleSnapshot(
        const http::request<http::string_body>& req);

    /**
     * GET /v1/admin/cache/tenants
     *
     * Returns an array of all known tenants with aggregated statistics
     * (bytes_used, quota, utilization, hits, misses, hit_rate, evictions).
     * Requires "admin:cache:read" scope.
     */
    http::response<http::string_body> handleListTenants(
        const http::request<http::string_body>& req);

    /**
     * GET /v1/admin/cache/tenant/{tenant_id}/stats
     *
     * Returns per-tenant statistics for the given tenant_id.
     * Requires "admin:cache:read" scope.
     * Returns 404 when the tenant has no recorded cache activity.
     */
    http::response<http::string_body> handleTenantStats(
        const http::request<http::string_body>& req);

    /**
     * PATCH /v1/admin/cache/tenant/{tenant_id}/quota
     *
     * Body: {"quota_bytes": <uint64>}
     *
     * Updates the cache size quota for the specified tenant.
     * A quota_bytes value of 0 resets the tenant to the global default quota.
     * Requires "admin:cache:write" scope.
     * Returns 404 when tenant isolation is disabled.
     */
    http::response<http::string_body> handleUpdateTenantQuota(
        const http::request<http::string_body>& req);

    /**
     * DELETE /v1/admin/cache/pii/{pii_uuid}
     *
     * Purges all cache entries that were tagged with the given PII UUID via
     * AdaptiveQueryCache::invalidatePII(pii_uuid).  This triggers GDPR Art. 17
     * erasure propagation across all three cache tiers (L1, L2, L3).
     *
     * Requires "admin:cache:write" scope.
     * Returns 200 with {"evicted": <count>, "pii_uuid": "<uuid>"} on success.
     * Returns 400 when pii_uuid is missing or empty.
     */
    http::response<http::string_body> handlePiiEvict(
        const http::request<http::string_body>& req);

    /**
     * @brief Attach an SLO monitor whose latency status is included in
     *        GET /v1/admin/cache/stats responses.
     *
     * May be nullptr (default) to omit the "slo" block from the response.
     * Thread-safe: updates are synchronized with request-time reads.
     */
    void setSloMonitor(std::shared_ptr<themis::cache::CacheHitRateSloMonitor> monitor);

private:
    std::shared_ptr<AdaptiveQueryCache> cache_;
    std::shared_ptr<AuthMiddleware> auth_;
    mutable std::mutex slo_monitor_mutex_;
    std::shared_ptr<themis::cache::CacheHitRateSloMonitor> slo_monitor_;

    // Returns false and fills `out` with a 401/403 response if auth fails.
    bool checkAuth(const http::request<http::string_body>& req,
                   const std::string& required_scope,
                   http::response<http::string_body>& out);

    // Extract the trailing path segment after `prefix` from req.target().
    // Returns empty string if the target does not start with prefix.
    static std::string extractPathParam(std::string_view target,
                                        std::string_view prefix);

    // Minimal base64 URL-safe decode (RFC 4648 §5, no padding required).
    static std::string base64Decode(const std::string& input);

    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req);

    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
