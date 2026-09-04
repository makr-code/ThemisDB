/**
 * @file route_version_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Header-Only Utilities**: Provides inline versioned URL routing functions.
 *       No separate .cpp implementation needed. Functions are inline for performance.
 */


#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <utility>

namespace themis {
namespace server {

/**
 * @brief Versioned URL path router for ThemisDB HTTP API.
 *
 * Handles `/v1/` and `/v2/` path prefixes according to the API versioning policy:
 *
 * ## Routing rules
 * 1. **Prefixed paths** (`/v1/<rest>`, `/v2/<rest>`): version is extracted; the
 *    canonical path (without the prefix) is returned for routing.
 * 2. **Unversioned paths** (e.g., `/documents/123`): the caller should issue a
 *    301 redirect to `/v1/<path>` for backward compatibility.
 * 3. **Admin and API-prefixed paths** (`/api/v1/…`, `/v1/admin/…`): treated as
 *    already versioned; not redirected.
 *
 * ## Typical usage in the HTTP server
 * ```cpp
 * RouteVersionRouter vr;
 *
 * // In routeRequest():
 * if (auto redirect_target = vr.getRedirectTarget(path)) {
 *     // 301 Moved Permanently → /v1/<path>
 *     return make301Response(*redirect_target);
 * }
 * auto [version, canonical_path] = vr.normalize(path);
 * // Route using canonical_path; version carries 1 or 2 (or 0 if unversioned).
 * ```
 *
 * This class is header-only and has no dependencies on the HTTP server
 * internals — it operates purely on path strings and can be unit-tested
 * independently.
 *
 * @see src/server/http_server.cpp (integration point)
 * @see src/api/FUTURE_ENHANCEMENTS.md – "Versioned API Routing"
 */
class RouteVersionRouter {
public:
    /// Result of path normalization: API major version (1, 2, …) and canonical path.
    struct NormalizedPath {
        int     version = 0;        ///< Extracted major version, or 0 if unversioned.
        std::string path;       ///< Canonical path (prefix stripped; never empty).
    };

    RouteVersionRouter() = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Normalize a request path.
     *
     * - `/v1/documents/abc` → `{1, "/documents/abc"}`
     * - `/v2/query/stream`  → `{2, "/query/stream"}`
     * - `/documents/abc`    → `{0, "/documents/abc"}`  (unversioned)
     * - `/api/v1/graphql`   → `{1, "/api/v1/graphql"}` (already prefixed – kept as-is)
     *
     * Path components after the version prefix are preserved verbatim including
     * query-string fragments, so the caller can append query parameters before
     * routing.
     *
     * @param path  Full request path, e.g. `/v1/documents/123?filter=...`
     * @return NormalizedPath with extracted version and canonical path.
     */
    NormalizedPath normalize(std::string_view path) const;

    /**
     * @brief Return the redirect target for an unversioned path, if one is needed.
     *
     * Returns `"/v1" + path` when @p path is an unversioned endpoint that should
     * receive a 301 redirect, or `std::nullopt` when no redirect is needed (i.e.
     * the path is already versioned or belongs to a non-redirected namespace).
     *
     * Paths that are **never** redirected:
     * - Already versioned: starts with `/v1/`, `/v2/`, `/api/v1/`, `/api/v2/`
     * - Internal / health: `/health`, `/metrics`, `/_internal/`, `/_ready`
     * - WebSocket upgrade paths: `/graphql`, `/v2/changes`
     * - Static and admin paths starting with `/static/`, `/admin/`
     *
     * @param path  Full request path.
     * @return Redirect target (e.g., `"/v1/documents/abc"`) or `std::nullopt`.
     */
    std::optional<std::string> getRedirectTarget(std::string_view path) const;

    /**
     * @brief Return true if the path already carries an explicit `/v{N}/` version prefix.
     *
     * Accepts both top-level (`/v1/`, `/v2/`) and nested (`/api/v1/`, `/api/v2/`)
     * prefixes.
     */
    static bool isVersioned(std::string_view path) noexcept;

    /**
     * @brief Extract the major API version from a versioned path.
     *
     * Returns 0 if the path is not versioned.
     *
     * Examples:
     *   `/v1/documents`   → 1
     *   `/v2/query/stream` → 2
     *   `/documents`      → 0
     */
    static int extractVersion(std::string_view path) noexcept;

    /**
     * @brief Strip the leading `/v{N}` prefix from a versioned path.
     *
     * Returns the original path unchanged if it is not versioned.
     *
     * Examples:
     *   `/v1/documents/abc` → `/documents/abc`
     *   `/v2/query/stream`  → `/query/stream`
     *   `/documents`        → `/documents`
     */
    static std::string_view stripVersionPrefix(std::string_view path) noexcept;

private:
    // Paths that start with these prefixes are never redirected.
    static bool isExemptFromRedirect(std::string_view path) noexcept;
};

// ---------------------------------------------------------------------------
// Inline implementation (header-only)
// ---------------------------------------------------------------------------

inline bool RouteVersionRouter::isVersioned(std::string_view path) noexcept {
    // /v1/... /v2/...
    if (path.size() >= 3 && path[0] == '/' && path[1] == 'v' &&
        path[2] >= '1' && path[2] <= '9') {
        // Must be followed by '/' or end of string
        if (path.size() == 3 || path[3] == '/' || path[3] == '?') {
          return true;
        }
    }
    // /api/v1/... /api/v2/...
    if (path.rfind("/api/v", 0) == 0 && path.size() > 6) {
        char c = path[6];
        if (c >= '1' && c <= '9') {
          return true;
        }
    }
    return false;
}

inline int RouteVersionRouter::extractVersion(std::string_view path) noexcept {
    // /vN/... (top-level)
    if (path.size() >= 3 && path[0] == '/' && path[1] == 'v') {
        if (path[2] >= '1' && path[2] <= '9') {
            if (path.size() == 3 || path[3] == '/' || path[3] == '?') {
                return path[2] - '0';
            }
        }
    }
    // /api/vN/... (nested)
    if (path.rfind("/api/v", 0) == 0 && path.size() > 6) {
        char c = path[6];
        if (c >= '1' && c <= '9') {
            if (path.size() == 7 || path[7] == '/' || path[7] == '?') {
                return c - '0';
            }
        }
    }
    return 0;
}

inline std::string_view RouteVersionRouter::stripVersionPrefix(
    std::string_view path) noexcept
{
    // /vN/ → strip "/vN"
    if (path.size() >= 4 && path[0] == '/' && path[1] == 'v' &&
        path[2] >= '1' && path[2] <= '9' && path[3] == '/') {
        return path.substr(3); // keeps leading '/'
    }
    // /vN (no trailing slash — unlikely but safe)
    if (path.size() == 3 && path[0] == '/' && path[1] == 'v' &&
        path[2] >= '1' && path[2] <= '9') {
        return "/";
    }
    return path;
}

inline bool RouteVersionRouter::isExemptFromRedirect(
    std::string_view path) noexcept
{
    // Already versioned
    if (isVersioned(path)) {
      return true;
    }

    // Well-known paths that must NOT be redirected
    static constexpr std::string_view exempt[] = {
        "/health",
        "/version",
        "/stats",
        "/metrics",
        "/ready",
        "/_internal/",
        "/_ready",
        "/graphql",
        "/static/",
        "/admin/",
        "/changefeed",
        "/favicon.ico",
        // Core CRUD / query endpoints registered directly without a /v1/ prefix
        "/entities",
        "/query/",
        "/api/aql",
        "/indexes",
        "/index/",
        "/spatial/",
        "/graph/",
        "/transaction",
        "/config",
        "/ts/",
        "/streams",
        "/vector/",
        "/geo/",
        "/search",
        "/triggers",
        "/jobs",
        "/bpmn/",
        "/crdt/",
        "/timeseries/",
    };
    for (auto& ex : exempt) {
        if (path == ex || path.rfind(ex, 0) == 0) {
          return true;
        }
    }

    // WebSocket upgrade targets
    if (path.rfind("/v2/", 0) == 0) {
      return true;
    }

    return false;
}

inline RouteVersionRouter::NormalizedPath
RouteVersionRouter::normalize(std::string_view path) const
{
    NormalizedPath result;
    result.version = extractVersion(path);
    if (result.version > 0) {
        // Strip the prefix for top-level /vN/ paths only; nested /api/vN/ kept intact.
        if (path.size() >= 4 && path[0] == '/' && path[1] == 'v') {
            result.path = std::string(stripVersionPrefix(path));
        } else {
            result.path = std::string(path);
        }
    } else {
        result.path = std::string(path);
    }
    return result;
}

inline std::optional<std::string>
RouteVersionRouter::getRedirectTarget(std::string_view path) const
{
    if (isExemptFromRedirect(path)) {
      return std::nullopt;
    }

    // Strip query string before computing the redirect target; re-append after.
    std::string_view path_only = path;
    std::string_view query = {};
    auto qpos = path.find('?');
    if (qpos != std::string_view::npos) {
        path_only = path.substr(0, qpos);
        query     = path.substr(qpos);
    }

    if (path_only.empty() || path_only == "/") {
      return std::nullopt;
    }

    return "/v1" + std::string(path_only) + std::string(query);
}

} // namespace server
} // namespace themis

