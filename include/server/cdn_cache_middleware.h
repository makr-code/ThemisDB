/**
 * @file cdn_cache_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief CDN / edge-caching cache directive enum.
 *
 * Controls the top-level visibility directive placed in the
 * Cache-Control response header.
 */
enum class CacheDirective {
    PUBLIC,     ///< public – CDN and browser may cache
    PRIVATE,    ///< private – browser only, not shared caches
    NO_CACHE,   ///< no-cache – revalidate with origin on every use
    NO_STORE,   ///< no-store – do not persist at all (sensitive data)
};

/**
 * @brief Per-route CDN cache policy.
 *
 * Governs which Cache-Control directives, surrogate keys, and CDN-specific
 * headers (CDN-Cache-Control, Surrogate-Control) are emitted for responses
 * that match a registered path prefix.
 *
 * Defaults produce a conservative `no-store` policy suitable for a database
 * API where most responses contain dynamic, potentially sensitive data.
 */
struct CdnRoutePolicy {
    /// Visibility directive for browser / shared caches.
    CacheDirective directive{CacheDirective::NO_STORE};

    /// Browser-side TTL in seconds (max-age=N). 0 means omit max-age.
    uint32_t max_age_seconds{0};

    /// CDN/shared-cache TTL in seconds (s-maxage=N for CDN-Cache-Control /
    /// Surrogate-Control). 0 means omit s-maxage.
    uint32_t cdn_max_age_seconds{0};

    /// Append stale-while-revalidate=N directive (0 = omit).
    uint32_t stale_while_revalidate_seconds{0};

    /// Append stale-if-error=N directive (0 = omit).
    uint32_t stale_if_error_seconds{0};

    /// Space-separated surrogate / cache tags for targeted CDN purge.
    /// Example: "entities schema" → Surrogate-Key: entities schema
    std::string surrogate_keys;

    /// Emit ETag response header and honour If-None-Match requests.
    bool enable_etag{false};

    /// Emit CDN-Cache-Control header (Cloudflare / Fastly semantics).
    bool emit_cdn_cache_control{true};

    /// Emit Surrogate-Control header (Varnish / Fastly semantics).
    bool emit_surrogate_control{false};
};

/**
 * @brief Edge caching middleware – CDN cache-control header management.
 *
 * Applies RFC 7234-compliant Cache-Control headers and CDN-specific
 * extension headers (CDN-Cache-Control, Surrogate-Control, Surrogate-Key)
 * to HTTP responses.
 *
 * Features
 * --------
 * - Per-route policy registration (longest-prefix match).
 * - Automatic write-method override: POST / PUT / PATCH / DELETE always
 *   receive `no-store` regardless of route policy.
 * - ETag generation (weak, FNV-1a over the response body) for GET responses.
 * - Conditional request support: returns `true` from `checkConditional()`
 *   when a `304 Not Modified` response is appropriate.
 * - Integration with ThemisDB governance: honours `X-Themis-Cache: disabled`
 *   by downgrading to `no-store`.
 *
 * Usage
 * -----
 * @code
 * CdnCacheMiddleware cdn;
 *
 * // Register a cacheable read endpoint
 * CdnRoutePolicy entity_policy;
 * entity_policy.directive         = CacheDirective::PRIVATE;
 * entity_policy.max_age_seconds   = 60;
 * entity_policy.enable_etag       = true;
 * cdn.registerPolicy("/entities/", entity_policy);
 *
 * // Inside request handling
 * cdn.apply(req, res);            // add headers to the response
 * if (cdn.checkConditional(req, res)) {
 *     // Return 304 Not Modified without a body
 * }
 * @endcode
 *
 * Thread-safety: All public methods are stateless with respect to the
 *   registered policy map, which is populated before the server starts
 *   and never mutated afterwards.  No locking is required for read-only
 *   access during request processing.
 */
class CdnCacheMiddleware {
public:
    CdnCacheMiddleware() = default;

    // ── Policy management ────────────────────────────────────────────────────

    /**
     * @brief Register a per-route CDN cache policy.
     *
     * Policies are matched by longest-prefix against the request path
     * (query string stripped).  A policy registered for "/" acts as the
     * catch-all default.
     *
     * @param path_prefix  URL path prefix (e.g., "/entities/", "/health").
     * @param policy       Cache policy to apply for matching requests.
     */
    void registerPolicy(const std::string& path_prefix, const CdnRoutePolicy& policy);

    // ── Middleware interface ─────────────────────────────────────────────────

    /**
     * @brief Apply CDN cache headers to @p res.
     *
     * Selects the best-matching route policy for @p req, then writes:
     *  - `Cache-Control`         (always)
     *  - `CDN-Cache-Control`     (when policy.emit_cdn_cache_control)
     *  - `Surrogate-Control`     (when policy.emit_surrogate_control)
     *  - `Surrogate-Key`         (when policy.surrogate_keys is non-empty)
     *  - `ETag`                  (when policy.enable_etag, GET/HEAD only)
     *
     * Write methods (POST, PUT, PATCH, DELETE) always receive `no-store`.
     * Responses with error status (5xx) always receive `no-store`.
     *
     * If the response already carries an `X-Themis-Cache: disabled`
     * governance header, the policy is overridden to `no-store`.
     *
     * @param req  Incoming HTTP request (method and path are read).
     * @param res  Outgoing HTTP response (headers are written into this).
     */
    void apply(
        const http::request<http::string_body>&  req,
        http::response<http::string_body>&        res) const;

    /**
     * @brief Check whether a `304 Not Modified` response is appropriate.
     *
     * Compares the `If-None-Match` request header against the `ETag`
     * response header set by a previous call to apply().
     *
     * @param req  Incoming HTTP request.
     * @param res  Outgoing HTTP response (must already have ETag set).
     * @return true  if the ETag matches and a 304 may be returned.
     * @return false otherwise.
     */
    bool checkConditional(
        const http::request<http::string_body>&  req,
        const http::response<http::string_body>& res) const;

    // ── Utilities (exposed for testing) ─────────────────────────────────────

    /**
     * @brief Generate a weak ETag value for @p body.
     *
     * Uses FNV-1a 64-bit hashing encoded as a quoted hex string prefixed
     * with `W/` per RFC 7232 § 2.1.
     *
     * @param body  Response body bytes.
     * @return Weak ETag string (e.g., `W/"1a2b3c4d5e6f7080"`).
     */
    static std::string generateETag(const std::string& body);

    /**
     * @brief Build a Cache-Control header value from a policy.
     *
     * @param policy   Cache policy to render.
     * @param is_write true for mutating HTTP methods (POST/PUT/PATCH/DELETE).
     * @return Cache-Control header value string.
     */
    static std::string buildCacheControlValue(const CdnRoutePolicy& policy, bool is_write);

private:
    /// Registered path-prefix → policy map.
    std::unordered_map<std::string, CdnRoutePolicy> policies_;

    /**
     * @brief Find the best-matching policy for @p path.
     *
     * Iterates all registered prefixes and returns the one with the
     * longest matching prefix.  Returns nullptr if no policy matches.
     */
    const CdnRoutePolicy* findPolicy(const std::string& path) const;

    /// Extract path without query string from request target.
    static std::string extractPath(const http::request<http::string_body>& req);

    /// Return true if the HTTP method is a write (mutating) verb.
    static bool isWriteMethod(http::verb method);

    /// Return true if the response status indicates a server error (5xx).
    static bool isServerError(http::status status);
};

} // namespace server
} // namespace themis
