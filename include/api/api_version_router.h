/**
 * @file api_version_router.h
 * @brief Request router for API version negotiation and routing.
 *
 * @details Routes API requests to version-specific handlers based on X-API-Version
 * header and Accept-Version negotiation. Supports multiple concurrent API versions.
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <memory>
#include "api/http_handler.h"
#include "utils/expected.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// VersionDescriptor — metadata for a single API version
// ---------------------------------------------------------------------------

/**
 * @brief Metadata associated with a single registered API version.
 *
 * `IAPIVersionRouter::registerVersion()` accepts a `VersionDescriptor`
 * alongside the handler set for that version.
 */
struct VersionDescriptor {
    /// Major version number, e.g. 1 for `/v1/`, 2 for `/v2/`.
    int major_version = 0;

    /// Minor version (informational; not part of the URL path).
    int minor_version = 0;

    /// Human-readable label, e.g. "v1.4 — Legacy" or "v2.0 — Current".
    std::string label;

    /// If set, this version is deprecated as of the given date.
    /// The router injects a `Deprecation` header into every response for this version.
    std::optional<std::string> deprecation_date; // ISO-8601 date string, e.g. "2027-01-01"

    /// If set, a `Sunset` header is also injected indicating the removal date.
    std::optional<std::string> sunset_date; // ISO-8601 date string

    /// If set, a `Link` header pointing to the migration guide is injected.
    std::optional<std::string> successor_url; // e.g. "https://docs.example.com/migrate-v1-v2"

    /// Convenience factory for a non-deprecated version.
    static VersionDescriptor current(int major, int minor = 0, std::string label = {}) {
        return {major, minor, std::move(label), std::nullopt, std::nullopt, std::nullopt};
    }

    /// Convenience factory for a deprecated version.
    static VersionDescriptor deprecated(int major, int minor,
                                        std::string deprecation_date,
                                        std::string sunset_date = {},
                                        std::string successor_url = {}) {
        VersionDescriptor d;
        d.major_version    = major;
        d.minor_version    = minor;
        d.deprecation_date = std::move(deprecation_date);
        if (!sunset_date.empty()) {
          d.sunset_date   = std::move(sunset_date);
        }
        if (!successor_url.empty()) {
          d.successor_url = std::move(successor_url);
        }
        return d;
    }
};

// ---------------------------------------------------------------------------
// HandlerSet — a mapping of HTTP method+path patterns to IHttpHandlers
// ---------------------------------------------------------------------------

/**
 * @brief A set of routes registered for one API version.
 *
 * Each entry maps an HTTP method + path pattern to an `IHttpHandler`.
 * Path parameters are denoted with `{name}`, e.g. `/entity/{id}`.
 */
struct RouteEntry {
    std::string method;   ///< HTTP method, e.g. "GET", "POST"
    std::string path;     ///< URL path (without version prefix), e.g. "/entity/{id}"
    std::shared_ptr<IHttpHandler> handler;
};

using HandlerSet = std::vector<RouteEntry>;

// ---------------------------------------------------------------------------
// IAPIVersionRouter — pure-virtual versioned routing interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for version-aware HTTP request routing.
 *
 * Routes inbound requests to the correct `IHttpHandler` based on the version
 * prefix in the URL path (`/v1/`, `/v2/`, …).
 *
 * ### Contract
 * - Versions must be registered via `registerVersion()` before the first call
 *   to `route()`.
 * - `route()` never returns a null handler; unknown or unregistered versions
 *   are routed to a built-in error handler that returns HTTP 404.
 * - A `/v2/` handler must expose a superset of the endpoints registered for
 *   `/v1/`; removing endpoints from a newer version is a contract violation.
 * - When a deprecated version is matched, the router injects `Deprecation` and
 *   (optionally) `Sunset` headers into the response.
 *
 * ### Thread safety
 * All `registerVersion()` calls must complete before `route()` is first called.
 * After initialisation, `route()` is safe to call concurrently from any thread.
 */
class IAPIVersionRouter {
public:
    virtual ~IAPIVersionRouter() = default;

    /**
     * @brief Register a versioned handler set.
     *
     * @param version   Version metadata (major, deprecation date, etc.).
     * @param handlers  Handler set for this version.
     * @throws std::invalid_argument if a version with the same `major_version`
     *         is already registered, or if `handlers` is empty.
     */
    virtual void registerVersion(VersionDescriptor version, HandlerSet handlers) = 0;

    /**
     * @brief Resolve a request to the appropriate `IHttpHandler`.
     *
     * Extracts the version prefix from `path` (e.g. `/v1/` → version 1),
     * finds the registered `HandlerSet` for that version, then matches
     * `method` + the remainder of `path` against the registered `RouteEntry`s.
     *
     * Never returns a null pointer.  If no matching handler is found the
     * returned handler produces HTTP 404 when invoked.
     *
     * @param method   HTTP method, e.g. "GET".
     * @param path     Full request path including version prefix, e.g. "/v1/entity/42".
     * @param out_deprecation_headers  If non-null, populated with any deprecation
     *                                 headers that should be injected into the response.
     * @return Reference to the matched `IHttpHandler`.
     */
    virtual IHttpHandler& route(std::string_view method,
                                std::string_view path,
                                std::unordered_map<std::string, std::string>* out_deprecation_headers = nullptr) = 0;

    /**
     * @brief Return the list of registered `VersionDescriptor`s in registration order.
     */
    virtual std::vector<VersionDescriptor> registeredVersions() const = 0;
};

} // namespace api
} // namespace themis
