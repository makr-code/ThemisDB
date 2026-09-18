/**
 * @file route_version_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class RouteVersionRouter {
public:
    struct NormalizedPath {
        int     version = 0;        ///< Extracted major version, or 0 if unversioned.
        std::string path;       ///< Canonical path (prefix stripped; never empty).
    };

    RouteVersionRouter() = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Normalize.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    NormalizedPath normalize(std::string_view path) const;

    /**
     * @brief Get Redirect Target.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::optional<std::string> getRedirectTarget(std::string_view path) const;

    /**
     * @brief Is Versioned.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isVersioned(std::string_view path) noexcept;

    /**
     * @brief Extract Version.
     * @param[in] path Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int extractVersion(std::string_view path) noexcept;

    /**
     * @brief Strip Version Prefix.
     * @param[in] path Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string_view stripVersionPrefix(std::string_view path) noexcept;

private:
    /**
     * @brief Is Exempt From Redirect.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
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

