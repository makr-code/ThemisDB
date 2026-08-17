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

    // ── Phase 3 Schema-Governance ──────────────────────────────────────────

    /**
     * @brief Capture a canonical snapshot of the current registered-route set.
     *
     * Serialises the full set of registered route entries to a deterministic
     * canonical form and returns it as an opaque string (e.g. a SHA-256 hex
     * digest or canonical JSON blob).  The snapshot can later be passed to
     * detectDrift() to identify changes.
     *
     * Complexity: O(N) in the number of registered routes.
     *
     * @return  Opaque snapshot string; empty only when the registry is empty.
     * @note    Thread-safe; acquires the internal shared_mutex for reading.
     */
    [[nodiscard]] std::string captureSpecSnapshot() const;

    /**
     * @brief Diff report returned by detectDrift().
     *
     * Contains three disjoint lists of route paths (HTTP-method + path pairs)
     * that differ between the baseline snapshot and the current state.
     *
     * - **added**   — routes present now but absent in the baseline.
     * - **removed** — routes present in the baseline but absent now.
     * - **changed** — routes present in both but with a different schema hash.
     */
    struct DriftReport {
        std::vector<std::string> added;   ///< New routes not in baseline
        std::vector<std::string> removed; ///< Routes removed since baseline
        std::vector<std::string> changed; ///< Routes modified since baseline

        /// @return true if any drift was detected.
        [[nodiscard]] bool hasDrift() const noexcept {
            return !added.empty() || !removed.empty() || !changed.empty();
        }
    };

    /**
     * @brief Compare the current registry against a previously captured snapshot.
     *
     * Returns a DriftReport describing which routes were added, removed, or
     * changed since the snapshot was captured.  Can be used at server startup
     * (when THEMIS_OPENAPI_STRICT=1) to fail-fast on spec divergence.
     *
     * Complexity: O(N) in the number of registered routes.
     *
     * @param baseline_snapshot  Snapshot string previously returned by
     *                           captureSpecSnapshot().
     * @return DriftReport describing all detected differences.
     * @note Thread-safe; acquires the internal shared_mutex for reading.
     */
    [[nodiscard]] DriftReport detectDrift(const std::string& baseline_snapshot) const;

private:
    RouteRegistry() = default;

    mutable std::shared_mutex mutex_;
    std::vector<RouteEntry>  entries_;
};

} // namespace server
} // namespace themis
