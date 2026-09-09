/**
 * @file cache_admin_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/cache_admin_api_handler.h"
#include "utils/input_validator.h"
#include "utils/logger.h"

#include <regex>
#include <stdexcept>
#include "utils/tracing.h"

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxCacheAdminPathParamLength = 256;
constexpr size_t kMaxCacheWarmupEntries = 1000000;

bool isValidPathSegmentParam(std::string_view value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(std::string(value), kMaxCacheAdminPathParamLength) &&
           validator.validatePathSegment(std::string(value)) &&
           validator.validateHeaderValue(std::string(value));
}

bool isLikelyValidBase64PathToken(std::string_view value) {
    if (value.empty() || static_cast<int>(value.size()) > kMaxCacheAdminPathParamLength) {
        return false;
    }

    bool saw_padding = false;
    size_t padding_count = 0;
    for (char ch : value) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (c == '=') {
            saw_padding = true;
            ++padding_count;
            if (padding_count > 2) {
                return false;
            }
            continue;
        }

        if (saw_padding) {
            // Padding is only valid at the end.
            return false;
        }

        // Keep the encoded token in a single path segment.
        if (!(((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '+' || c == '-' || c == '_'))) {
            return false;
        }
    }

    return true;
}

bool isValidCacheAdminFilePath(const std::string& value) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(value, 1024) &&
           validator.validateHeaderValue(value) &&
           validator.validateFilePath(value);
}

} // namespace

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string CacheAdminApiHandler::base64Decode(const std::string& input) {
    std::string output = {};
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; ++i) {
        T[static_cast<unsigned char>(kBase64Chars[i])] = i;
    }
    // Also accept URL-safe variant ('-' -> 62, '_' -> 63)
    T[static_cast<unsigned char>('-')] = 62;
    T[static_cast<unsigned char>('_')] = 63;

    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (c == '=') {
          break;
        }
        int tv = T[c];
        if (tv == -1) {
          break;
        }
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

void CacheAdminApiHandler::setSloMonitor(
    std::shared_ptr<themis::cache::CacheHitRateSloMonitor> monitor) {
    std::lock_guard<std::mutex> lock(slo_monitor_mutex_);
    slo_monitor_ = std::move(monitor);
}

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

    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        out = makeErrorResponse(http::status::unauthorized,
                                "Missing Authorization header", req);
        return false;
    }

    auto token = AuthMiddleware::extractBearerToken(std::string(auth_header.data(),static_cast<int>(auth_header.size())));
    if (!token) {
        out = makeErrorResponse(http::status::unauthorized,
                                "Invalid Authorization header", req);
        return false;
    }

    auto ar = auth_->authorize(*token, required_scope);
    if (!ar.authorized) {
        THEMIS_WARN("[AUDIT] authorize result=DENY scope={}", required_scope);
        THEMIS_WARN("Cache admin API auth denied: user={}, scope={}, reason={}",
                    ar.user_id, required_scope,
                    ar.reason.empty() ? "insufficient_scope" : ar.reason);
        out = makeErrorResponse(http::status::forbidden,
                                "Insufficient scope: " + required_scope, req);
        return false;
    }
    THEMIS_INFO("[AUDIT] authorize result=ALLOW scope={}", required_scope);

    return true;
}

// ---------------------------------------------------------------------------
// Endpoint handlers
//
// Thread-safety note: all handlers in this class are invoked concurrently by
// the HTTP server's worker-thread pool.  All calls on `cache_` below delegate
// to AdaptiveQueryCache, which serialises its own state via internal mutexes
// (l1_mutex_, l2_mutex_, l3_mutex_, tenant_mutex_, coordinator_mutex_).
// No additional external lock is required in this handler class.
// Static-analysis data-race alerts on cache.method() calls are false
// positives: AdaptiveQueryCache is designed as a thread-safe shared resource.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// GET /v1/admin/cache/health
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleHealth(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleHealth");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    try {
        nlohmann::json body = cache.getHealthStatus();
        bool healthy = body.value("healthy", true);
        auto status = healthy ? http::status::ok : http::status::service_unavailable;
        return makeResponse(status, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin health error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> CacheAdminApiHandler::handleStats(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleStats");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    try {
        nlohmann::json body = cache.getDetailedInfo();
        body["health"] = cache.getHealthStatus();
        body["tenant_stats"] = cache.getTenantStats();
        body["circuit_breaker"] = cache.getCircuitBreakerStatus();

        const auto& metrics = cache.getEnhancedMetrics();
        body["rate_limiter"] = {
            {"total_hits", metrics.l1_hits.load() + metrics.l2_hits.load() + metrics.l3_hits.load()},
            {"throttled", metrics.rate_limited_requests.load()}
        };

        // Latency percentiles from the SLO monitor (if one is attached)
        std::shared_ptr<themis::cache::CacheHitRateSloMonitor> slo_monitor;
        {
            std::lock_guard<std::mutex> lock(slo_monitor_mutex_);
            slo_monitor = slo_monitor_;
        }
        if (slo_monitor) {
            auto status = slo_monitor->getStatus();
            if (status.contains("latency")) {
                body["slo"] = status["latency"];
            }
        }

        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin stats error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> CacheAdminApiHandler::handleEvictKey(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleEvictKey");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    // Extract base64-encoded key from path: /v1/admin/cache/key/{encoded_key}
    std::string encoded = extractPathParam(std::string(req.target()),
                                           "/v1/admin/cache/key/");
    if (encoded.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing key path parameter", req);
    }
    if (!isLikelyValidBase64PathToken(encoded)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid base64-encoded key", req);
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
        size_t count = cache.invalidate(pattern);

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
    auto span = Tracer::startSpan("handleEvictTenant");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    std::string tenant_id = extractPathParam(std::string(req.target()),
                                             "/v1/admin/cache/tenant/");
    if (tenant_id.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing tenant_id path parameter", req);
    }
    if (!isValidPathSegmentParam(tenant_id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid tenant_id path parameter", req);
    }

    try {
        size_t count = cache.invalidateTenant(tenant_id);

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
    auto span = Tracer::startSpan("handleCircuitBreakerReset");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    try {
        cache.resetCircuitBreaker();

        nlohmann::json body = {
            {"status", "ok"},
            {"circuit_breaker", cache.getCircuitBreakerStatus()}
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
    auto span = Tracer::startSpan("handleCircuitBreakerStatus");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    try {
        nlohmann::json body = cache.getCircuitBreakerStatus();
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
    auto span = Tracer::startSpan("handleWarmup");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    std::string log_path = {};
    size_t max_entries = 0;

    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("log_path") || !body["log_path"].is_string() ||
            body["log_path"].get<std::string>().empty()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Missing required field: log_path", req);
        }
        log_path    = body["log_path"].get<std::string>();
        if (!isValidCacheAdminFilePath(log_path)) {
            return makeErrorResponse(http::status::bad_request,
                                     "Invalid log_path", req);
        }
        if (body.contains("max_entries")) {
            if (!body["max_entries"].is_number_unsigned()) {
                return makeErrorResponse(http::status::bad_request,
                                         "Invalid max_entries", req);
            }
            max_entries = body["max_entries"].get<size_t>();
            if (max_entries > kMaxCacheWarmupEntries) {
                return makeErrorResponse(http::status::bad_request,
                                         "max_entries exceeds limit", req);
            }
        }
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    }

    try {
        auto result = cache.warmupFromLog(log_path, max_entries);

        if (!result.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                                     result.error, req);
        }

        nlohmann::json resp = {
            {"status",                      "ok"},
            {"entries_loaded",              result.entries_loaded},
            {"entries_skipped",             result.entries_skipped},
            {"entries_total",               result.entries_total},
            {"warmup_duration_ms",          result.warmup_duration_ms},
            {"warmup_entries_per_second",   result.warmup_entries_per_second},
            {"log_path",                    log_path}
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
    auto span = Tracer::startSpan("handleSnapshot");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    std::string out_path = {};

    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("out_path") || !body["out_path"].is_string() ||
            body["out_path"].get<std::string>().empty()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Missing required field: out_path", req);
        }
        out_path = body["out_path"].get<std::string>();
        if (!isValidCacheAdminFilePath(out_path)) {
            return makeErrorResponse(http::status::bad_request,
                                     "Invalid out_path", req);
        }
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    }

    try {
        auto result = cache.exportSnapshot(out_path);

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

// ---------------------------------------------------------------------------
// GET /v1/admin/cache/tenants
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleListTenants(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleListTenants");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    try {
        nlohmann::json body = cache.getTenantStats();
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin list-tenants error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ---------------------------------------------------------------------------
// GET /v1/admin/cache/tenant/{tenant_id}/stats
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleTenantStats(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleTenantStats");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:read", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    // Extract tenant_id from: /v1/admin/cache/tenant/{tenant_id}/stats
    std::string_view target = req.target();
    constexpr std::string_view prefix = "/v1/admin/cache/tenant/";
    if (target.rfind(prefix, 0) != 0) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid path", req);
    }
    auto rest = target.substr(prefix.size());
    constexpr std::string_view suffix = "/stats";
    if (static_cast<int>(rest.size()) <= suffix.size() ||
        rest.substr(static_cast<int>(rest.size()) - static_cast<int>(suffix.size()) ) != suffix) {
        return makeErrorResponse(http::status::bad_request,
                                 "Path must end with /stats", req);
    }
    std::string tenant_id(rest.substr(0, static_cast<int>(rest.size()) - static_cast<int>(suffix.size()) ));
    if (tenant_id.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing tenant_id path parameter", req);
    }
    if (!isValidPathSegmentParam(tenant_id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid tenant_id path parameter", req);
    }

    try {
        nlohmann::json body = cache.getTenantStatsForTenant(tenant_id);
        if (body.contains("found") && !body["found"].get<bool>()) {
            return makeErrorResponse(http::status::not_found,
                                     "Tenant not found: " + tenant_id, req);
        }
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin tenant-stats error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ---------------------------------------------------------------------------
// PATCH /v1/admin/cache/tenant/{tenant_id}/quota
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handleUpdateTenantQuota(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleUpdateTenantQuota");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    // Extract tenant_id from: /v1/admin/cache/tenant/{tenant_id}/quota
    std::string_view target = req.target();
    constexpr std::string_view prefix = "/v1/admin/cache/tenant/";
    if (target.rfind(prefix, 0) != 0) {
        return makeErrorResponse(http::status::bad_request, "Invalid path", req);
    }
    auto rest = target.substr(prefix.size());
    constexpr std::string_view suffix = "/quota";
    if (static_cast<int>(rest.size()) <= suffix.size() ||
        rest.substr(static_cast<int>(rest.size()) - static_cast<int>(suffix.size()) ) != suffix) {
        return makeErrorResponse(http::status::bad_request,
                                 "Path must end with /quota", req);
    }
    std::string tenant_id(rest.substr(0, static_cast<int>(rest.size()) - static_cast<int>(suffix.size()) ));
    if (tenant_id.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing tenant_id path parameter", req);
    }
    if (!isValidPathSegmentParam(tenant_id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid tenant_id path parameter", req);
    }

    size_t quota_bytes = 0;
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("quota_bytes") || !body["quota_bytes"].is_number_unsigned()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Missing or invalid field: quota_bytes (must be unsigned integer)", req);
        }
        quota_bytes = body["quota_bytes"].get<size_t>();
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    }

    try {
        bool ok = cache.updateTenantQuota(tenant_id, quota_bytes);
        if (!ok) {
            return makeErrorResponse(http::status::not_found,
                                     "Tenant isolation is disabled or tenant_id is empty", req);
        }

        nlohmann::json resp = {
            {"status",      "ok"},
            {"tenant_id",   tenant_id},
            {"quota_bytes", quota_bytes}
        };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin update-tenant-quota error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ---------------------------------------------------------------------------
// DELETE /v1/admin/cache/pii/{pii_uuid}
// ---------------------------------------------------------------------------

http::response<http::string_body> CacheAdminApiHandler::handlePiiEvict(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handlePiiEvict");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:cache:write", auth_resp)) {
        return auth_resp;
    }

    if (!cache_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Cache not available", req);
    }
    auto& cache = *cache_;

    std::string pii_uuid = extractPathParam(std::string(req.target()),
                                            "/v1/admin/cache/pii/");
    if (pii_uuid.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing pii_uuid path parameter", req);
    }
    if (!isValidPathSegmentParam(pii_uuid)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid pii_uuid path parameter", req);
    }

    try {
        size_t evicted = cache.invalidatePII(pii_uuid);
        THEMIS_INFO("Cache admin PII evict: pii_uuid={} evicted={}", pii_uuid, evicted);

        nlohmann::json body = {
            {"evicted",  evicted},
            {"pii_uuid", pii_uuid}
        };
        return makeResponse(http::status::ok, body.dump(), req);
    } catch (const std::exception& e) {
        THEMIS_WARN("Cache admin pii-evict error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

} // namespace server
} // namespace themis
