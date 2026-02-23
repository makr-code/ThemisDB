/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_admin_api_handler.h                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-23 03:57:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03f3c2a45  2026-02-22  feat(cache): warmup from query log and export snapshot – ... ║
    • 65af13dcb  2026-02-22  Code audit bugfixes: fix L3 eviction pattern and strength... ║
    • d8bc55d98  2026-02-22  Add Admin API for cache operations and monitoring ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "server/auth_middleware.h"
#include "cache/adaptive_query_cache.h"

#include <memory>
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
 *   GET    /v1/admin/cache/stats                 – JSON snapshot of cache metrics
 *   DELETE /v1/admin/cache/key/{encoded_key}     – Evict one entry (key base64-encoded)
 *   DELETE /v1/admin/cache/tenant/{tenant_id}    – Evict all entries for a tenant
 *   POST   /v1/admin/cache/circuit-breaker/reset – Force circuit breaker to CLOSED
 *   GET    /v1/admin/cache/circuit-breaker       – Current circuit breaker state
 *   POST   /v1/admin/cache/warmup                – Load entries from NDJSON log file
 *   POST   /v1/admin/cache/snapshot              – Export live entries to NDJSON file
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

private:
    std::shared_ptr<AdaptiveQueryCache> cache_;
    std::shared_ptr<AuthMiddleware> auth_;

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
