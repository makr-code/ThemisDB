/**
 * @file openapi_route_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief Metadata for a single query/path/header parameter.
 */
struct RouteParam {
    std::string name;
    std::string in;          ///< "path", "query", or "header"
    bool        required{false};
    std::string description;
    json        schema;      ///< JSON Schema object, e.g. {{"type","string"}}
};

/**
 * @brief Metadata for one HTTP operation on a route (e.g. GET /entities).
 */
struct RouteOperation {
    std::string              summary;
    std::string              description;
    std::string              operationId;
    std::vector<std::string> tags;
    std::vector<RouteParam>  parameters;
    json                     requestBody; ///< empty json object {} if none
    json                     responses;   ///< map of status-code → response object
    bool                     deprecated{false};
};

/**
 * @brief A single registered route: path + HTTP method + operation metadata.
 */
struct RouteEntry {
    std::string    path;    ///< e.g. "/entities/{key}"
    std::string    method;  ///< lowercase: "get", "post", "put", "delete", "patch"
    RouteOperation operation;
};

/**
 * @brief Thread-safe singleton registry for OpenAPI route annotations.
 *
 * Handlers call RouteRegistry::instance().registerRoute() during startup to
 * publish their path/operation metadata. MonitoringApiHandler::handleOpenApi()
 * then calls buildOpenApiSpec() to produce a complete OpenAPI 3.1.0 document
 * from all registered entries without any duplication.
 *
 * Usage (in a handler's registration helper):
 * @code
 *   RouteRegistry::instance().registerRoute({
 *       "/entities",
 *       "get",
 *       { "List entities", "", "listEntities", {"entities"}, {}, {}, responses }
 *   });
 * @endcode
 */
class RouteRegistry {
public:
    /// Returns the process-wide singleton instance.
    static RouteRegistry& instance();

    /**
     * @brief Register one route/operation pair.
     *
     * Calling this multiple times with the same path+method is allowed; the
     * last registration wins (useful for unit-test reset scenarios).
     */
    void registerRoute(RouteEntry entry);

    /**
     * @brief Returns a snapshot copy of all currently registered entries.
     *
     * Returns a value (not a reference) so the caller does not need to hold
     * the registry mutex during iteration.  Safe to call from any thread.
     */
    std::vector<RouteEntry> entries() const;

    /**
     * @brief Build and return a complete OpenAPI 3.1.0 JSON document.
     *
     * The document contains info, servers, paths (assembled from registered
     * entries), reusable components (schemas, securitySchemes, headers,
     * parameters) and global security/tags arrays.
     *
     * @param api_version  Value placed in info.version (e.g. "0.1.0").
     * @return             Fully populated nlohmann::json object.
     */
    json buildOpenApiSpec(const std::string& api_version) const;

    /**
     * @brief Remove all registered routes.
     *
     * Intended for unit tests that need a clean registry state between cases.
     */
    void clear();

private:
    RouteRegistry() = default;

    mutable std::shared_mutex mutex_;
    std::vector<RouteEntry>  entries_;
};

} // namespace server
} // namespace themis
