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

struct VersionDescriptor {
    int major_version = 0;

    int minor_version = 0;

    std::string label;

    std::optional<std::string> deprecation_date; // ISO-8601 date string, e.g. "2027-01-01"

    std::optional<std::string> sunset_date; // ISO-8601 date string

    std::optional<std::string> successor_url; // e.g. "https://docs.example.com/migrate-v1-v2"

    static VersionDescriptor current(int major, int minor = 0, std::string label = {}) {
        return {major, minor, std::move(label), std::nullopt, std::nullopt, std::nullopt};
    }

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

struct RouteEntry {
    std::string method;   ///< HTTP method, e.g. "GET", "POST"
    std::string path;     ///< URL path (without version prefix), e.g. "/entity/{id}"
    std::shared_ptr<IHttpHandler> handler;
};

using HandlerSet = std::vector<RouteEntry>;

// ---------------------------------------------------------------------------
// IAPIVersionRouter — pure-virtual versioned routing interface
// ---------------------------------------------------------------------------

class IAPIVersionRouter {
public:
    /**
     * @brief IAPIVersion Router.
     * @return Return value.
     */
    virtual ~IAPIVersionRouter() = default;

    /**
     * @brief Register Version.
     * @param[in] version Input parameter.
     * @param[in] handlers Input parameter.
     */
    virtual void registerVersion(VersionDescriptor version, HandlerSet handlers) = 0;

    virtual IHttpHandler& route(std::string_view method,
                                std::string_view path,
                                std::unordered_map<std::string, std::string>* out_deprecation_headers = nullptr) = 0;

    /**
     * @brief Registered Versions.
     * @return Return value.
     */
    virtual std::vector<VersionDescriptor> registeredVersions() const = 0;
};

} // namespace api
} // namespace themis
