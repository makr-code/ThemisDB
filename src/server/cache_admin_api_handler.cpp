/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_admin_api_handler.cpp                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-23 03:58:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     418                                            ║
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

#include "server/cache_admin_api_handler.h"
#include "utils/logger.h"

#include <regex>
#include <stdexcept>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string CacheAdminApiHandler::base64Decode(const std::string& input) {
    std::string output;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; ++i) {
        T[static_cast<unsigned char>(kBase64Chars[i])] = i;
    }
    // Also accept URL-safe variant ('-' -> 62, '_' -> 63)
    T[static_cast<unsigned char>('-')] = 62;
    T[static_cast<unsigned char>('_')] = 63;

    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        int tv = T[c];
        if (tv == -1) break;
        val = (val << 6) + tv;
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

std::string CacheAdminApiHandler::extractPathParam(std::string_view target,
                                                    std::string_view prefix) {
    if (target.rfind(prefix, 0) != 0) {
        return {};
    }
    auto param = target.substr(prefix.size());
    // Strip query string if present
    auto qpos = param.find('?');
    if (qpos != std::string_view::npos) {
        param = param.substr(0, qpos);
    }
    return std::string(param);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

CacheAdminApiHandler::CacheAdminApiHandler(
    std::shared_ptr<AdaptiveQueryCache> cache,
    std::shared_ptr<AuthMiddleware> auth)
    : cache_(std::move(cache))
    , auth_(std::move(auth)) {}

// ---------------------------------------------------------------------------
// Auth helper
// ---------------------------------------------------------------------------

bool CacheAdminApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_scope,
    http::response<http::string_body>& out)
{
    if (!auth_ || !auth_->isEnabled()) {
        return true;  // Auth disabled – allow (dev/test mode)
    }

    auto it = req.find(http::field::authorization);
    if (it == req.end()) {
        out = makeErrorResponse(http::status::unauthorized,
                                "Missing Authorization header", req);
        return false;
    }

    auto token = AuthMiddleware::extractBearerToken(std::string(it->value()));
    if (!token) {
        out = makeErrorResponse(http::status::unauthorized,
                                "Invalid Authorization header", req);
        return false;
    }

    auto ar = auth_->authorize(*token, required_scope);
    if (!ar.authorized) {
        THEMIS_WARN("Cache admin API auth denied: user={}, scope={}, reason={}",
                    ar.user_id, required_scope,
                    ar.reason.empty() ? "insufficient_scope" : ar.reason);
        out = makeErrorResponse(http::status::forbidden,
                                "Insufficient scope: " + required_scope, req);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Endpoint handlers
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleStats(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    try {
        nlohmann::json body = cache_->getDetailedInfo();
        body["health"] = cache_->getHealthStatus();
        body["tenant_stats"] = cache_->getTenantStats();
        body["circuit_breaker"] = cache_->getCircuitBreakerStatus();

        const auto& metrics = cache_->getEnhancedMetrics();
        body["rate_limiter"] = {
            {"total_hits", metrics.l1_hits.load() + metrics.l2_hits.load() + metrics.l3_hits.load()},
            {"throttled", metrics.rate_limited_requests.load()}
        };

        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin stats error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> CacheAdminApiHandler::handleEvictKey(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    // Extract base64-encoded key from path: /v1/admin/cache/key/{encoded_key}
    std::string encoded = extractPathParam(std::string(req.target()),
                                           "/v1/admin/cache/key/");
    if (encoded.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing key path parameter", req);
    }

    std::string key = base64Decode(encoded);
    if (key.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid base64-encoded key", req);
    }

    try {
        // Escape the key for use as a literal regex pattern.
        std::string escaped = std::regex_replace(key,
            std::regex(R"([.^$|()\[\]{}*+?\\])"), R"(\$&)");
        // Build a pattern that matches across all cache tiers:
        // - L1/L2 store keys as either the plain fingerprint ("fp") or a
        //   tenant-scoped composite key ("tenant:{id}:fp").
        // - L3 always stores just the plain fingerprint (after stripping the
        //   "query_cache:" prefix) regardless of tenant isolation.
        // The two-branch alternation therefore matches:
        //   * the plain fingerprint in L1/L2 and in L3, and
        //   * any tenant-scoped form "tenant:{id}:{fingerprint}" in L1/L2.
        std::string pattern = "(^" + escaped + "$)|(^tenant:.+:" + escaped + "$)";
        size_t count = cache_->invalidate(pattern);

        nlohmann::json body = {
            {"evicted", count},
            {"key", key}
        };
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin evict-key error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> CacheAdminApiHandler::handleEvictTenant(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    std::string tenant_id = extractPathParam(std::string(req.target()),
                                             "/v1/admin/cache/tenant/");
    if (tenant_id.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing tenant_id path parameter", req);
    }

    try {
        size_t count = cache_->invalidateTenant(tenant_id);

        nlohmann::json body = {
            {"evicted", count},
            {"tenant_id", tenant_id}
        };
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin evict-tenant error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> CacheAdminApiHandler::handleCircuitBreakerReset(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    try {
        cache_->resetCircuitBreaker();

        nlohmann::json body = {
            {"status", "ok"},
            {"circuit_breaker", cache_->getCircuitBreakerStatus()}
        };
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin circuit-breaker reset error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> CacheAdminApiHandler::handleCircuitBreakerStatus(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    try {
        nlohmann::json body = cache_->getCircuitBreakerStatus();
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin circuit-breaker status error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ---------------------------------------------------------------------------
// Response helpers
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> CacheAdminApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req)
{
    nlohmann::json err = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, err.dump(), req);
}

// ---------------------------------------------------------------------------
// POST /v1/admin/cache/warmup
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleWarmup(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    std::string log_path;
    size_t max_entries = 0;

    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("log_path") || !body["log_path"].is_string() ||
            body["log_path"].get<std::string>().empty()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Missing required field: log_path", req);
        }
        log_path    = body["log_path"].get<std::string>();
        max_entries = body.value("max_entries", static_cast<size_t>(0));
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    }

    try {
        auto result = cache_->warmupFromLog(log_path, max_entries);

        if (!result.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                                     result.error, req);
        }

        nlohmann::json resp = {
            {"status",           "ok"},
            {"entries_loaded",   result.entries_loaded},
            {"entries_skipped",  result.entries_skipped},
            {"entries_total",    result.entries_total},
            {"log_path",         log_path}
        };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin warmup error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ---------------------------------------------------------------------------
// POST /v1/admin/cache/snapshot
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleSnapshot(
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }

    std::string out_path;

    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("out_path") || !body["out_path"].is_string() ||
            body["out_path"].get<std::string>().empty()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Missing required field: out_path", req);
        }
        out_path = body["out_path"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    }

    try {
        auto result = cache_->exportSnapshot(out_path);

        if (!result.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                                     result.error, req);
        }

        nlohmann::json resp = {
            {"status",          "ok"},
            {"entries_exported", result.entries_written},
            {"out_path",         out_path}
        };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin snapshot error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

} // namespace server
} // namespace themis
