/**
 * @file cdn_cache_middleware.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/cdn_cache_middleware.h"
#include "utils/hash_util.h"

#include <boost/beast/core/string.hpp>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace themis {
namespace server {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Policy management
// ─────────────────────────────────────────────────────────────────────────────

void CdnCacheMiddleware::registerPolicy(
    const std::string& path_prefix,
    const CdnRoutePolicy& policy)
{
    policies_[path_prefix] = policy;
}

const CdnRoutePolicy* CdnCacheMiddleware::findPolicy(const std::string& path) const {
    const CdnRoutePolicy* best = nullptr;
    std::size_t best_len = 0;

    for (const auto& [prefix, policy] : policies_) {
        if (path.rfind(prefix, 0) == 0) {        // path starts with prefix
            if (prefix.size() >= best_len) {
                best_len = prefix.size();
                best     = &policy;
            }
        }
    }
    return best;
}

// ─────────────────────────────────────────────────────────────────────────────
// Static utilities
// ─────────────────────────────────────────────────────────────────────────────

std::string CdnCacheMiddleware::extractPath(
    const http::request<http::string_body>& req)
{
    std::string target = std::string(req.target());
    auto q = target.find('?');
    if (q != std::string::npos) {
        target = target.substr(0, q);
    }
    return target;
}

bool CdnCacheMiddleware::isWriteMethod(http::verb method) {
    return method == http::verb::post   ||
           method == http::verb::put    ||
           method == http::verb::patch  ||
           method == http::verb::delete_;
}

bool CdnCacheMiddleware::isServerError(http::status status) {
    auto code = static_cast<unsigned>(status);
    return code >= 500 && code < 600;
}

std::string CdnCacheMiddleware::generateETag(const std::string& body) {
    uint64_t h = themis::hash::fnv1a64(body);
    std::ostringstream oss;
    oss << "W/\"" << std::hex << std::setfill('0') << std::setw(16) << h << "\"";
    return oss.str();
}

std::string CdnCacheMiddleware::buildCacheControlValue(
    const CdnRoutePolicy& policy,
    bool is_write)
{
    // Write methods and server errors always produce no-store.
    if (is_write) {
        return "no-store";
    }

    std::string value;

    switch (policy.directive) {
        case CacheDirective::PUBLIC:
            value = "public";
            break;
        case CacheDirective::PRIVATE:
            value = "private";
            break;
        case CacheDirective::NO_CACHE:
            value = "no-cache";
            break;
        case CacheDirective::NO_STORE:
        default:
            return "no-store";
    }

    if (policy.max_age_seconds > 0) {
        value += ", max-age=" + std::to_string(policy.max_age_seconds);
    }
    if (policy.cdn_max_age_seconds > 0 && policy.directive == CacheDirective::PUBLIC) {
        // s-maxage only applies for public shared caches.
        value += ", s-maxage=" + std::to_string(policy.cdn_max_age_seconds);
    }
    if (policy.stale_while_revalidate_seconds > 0) {
        value += ", stale-while-revalidate=" +
                 std::to_string(policy.stale_while_revalidate_seconds);
    }
    if (policy.stale_if_error_seconds > 0) {
        value += ", stale-if-error=" +
                 std::to_string(policy.stale_if_error_seconds);
    }

    return value;
}

// ─────────────────────────────────────────────────────────────────────────────
// Middleware interface
// ─────────────────────────────────────────────────────────────────────────────

void CdnCacheMiddleware::apply(
    const http::request<http::string_body>&  req,
    http::response<http::string_body>&        res) const
{
    const std::string path     = extractPath(req);
    const bool        is_write = isWriteMethod(req.method());
    const bool        is_error = isServerError(res.result());

    // Resolve route policy (fall back to a conservative no-store default).
    static const CdnRoutePolicy k_default_no_store{};   // directive = NO_STORE
    const CdnRoutePolicy* matched = findPolicy(path);
    const CdnRoutePolicy& policy  = matched ? *matched : k_default_no_store;

    // Governance override: if the server has marked this response as
    // non-cacheable (X-Themis-Cache: disabled) honour that decision.
    bool governance_no_cache = false;
    {
        auto it = res.find("X-Themis-Cache");
        if (it != res.end() && beast::iequals(it->value(), "disabled")) {
            governance_no_cache = true;
        }
    }

    // Determine effective directive.
    const bool force_no_store = is_write || is_error || governance_no_cache;

    // ── Cache-Control ────────────────────────────────────────────────────────
    const std::string cc_value = force_no_store
        ? "no-store"
        : buildCacheControlValue(policy, false);

    res.set(http::field::cache_control, cc_value);

    // ── CDN-Cache-Control (Cloudflare / Fastly) ──────────────────────────────
    if (!force_no_store && policy.emit_cdn_cache_control) {
        std::string cdn_cc = cc_value;
        // When CDN max-age differs from browser max-age, express it via
        // s-maxage in CDN-Cache-Control even for PRIVATE routes (CDN-only
        // override semantics).
        if (policy.cdn_max_age_seconds > 0 && policy.directive == CacheDirective::PRIVATE) {
            cdn_cc = "public, s-maxage=" + std::to_string(policy.cdn_max_age_seconds);
            if (policy.stale_while_revalidate_seconds > 0) {
                cdn_cc += ", stale-while-revalidate=" +
                          std::to_string(policy.stale_while_revalidate_seconds);
            }
        }
        res.set("CDN-Cache-Control", cdn_cc);
    }

    // ── Surrogate-Control (Varnish / Fastly) ─────────────────────────────────
    if (!force_no_store && policy.emit_surrogate_control && policy.cdn_max_age_seconds > 0) {
        res.set("Surrogate-Control",
                "max-age=" + std::to_string(policy.cdn_max_age_seconds));
    }

    // ── Surrogate-Key / Cache-Tag ─────────────────────────────────────────────
    if (!force_no_store && !policy.surrogate_keys.empty()) {
        res.set("Surrogate-Key", policy.surrogate_keys);
        // CloudFront-compatible alias.
        res.set("Cache-Tag", policy.surrogate_keys);
    }

    // ── ETag (GET / HEAD only) ────────────────────────────────────────────────
    if (!force_no_store && policy.enable_etag &&
        (req.method() == http::verb::get || req.method() == http::verb::head) &&
        !res.body().empty())
    {
        res.set(http::field::etag, generateETag(res.body()));
    }
}

bool CdnCacheMiddleware::checkConditional(
    const http::request<http::string_body>&  req,
    const http::response<http::string_body>& res) const
{
    // Only applicable for safe (non-mutating) methods.
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return false;
    }

    // Compare ETag with If-None-Match header.
    auto inm_it  = req.find(http::field::if_none_match);
    auto etag_it = res.find(http::field::etag);

    if (inm_it != req.end() && etag_it != res.end()) {
        // Support both exact match and wildcard "*".
        std::string_view inm  = inm_it->value();
        std::string_view etag = etag_it->value();
        if (inm == "*" || inm == etag) {
            return true;
        }
    }

    return false;
}

} // namespace server
} // namespace themis
