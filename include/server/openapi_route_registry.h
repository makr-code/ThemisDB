/**
 * @file openapi_route_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct RouteParam {
    std::string name;
    std::string in;          ///< "path", "query", or "header"
    bool        required{false};
    std::string description;
    json        schema;      ///< JSON Schema object, e.g. {{"type","string"}}
};

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

struct RouteEntry {
    std::string    path;    ///< e.g. "/entities/{key}"
    std::string    method;  ///< lowercase: "get", "post", "put", "delete", "patch"
    RouteOperation operation;
};

class RouteRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static RouteRegistry& instance();

    /**
     * @brief Register Route.
     * @param[in] entry Input parameter.
     */
    void registerRoute(RouteEntry entry);

    /**
     * @brief Entries.
     * @return Return value.
     */
    std::vector<RouteEntry> entries() const;

    /**
     * @brief Build Open Api Spec.
     * @param[in] api_version Input parameter.
     * @return Return value.
     */
    json buildOpenApiSpec(const std::string& api_version) const;

    /**
     * @brief Clear.
     */
    void clear();

    // ── Phase 3 Schema-Governance ──────────────────────────────────────────

    [[nodiscard]] std::string captureSpecSnapshot() const;

    struct DriftReport {
        std::vector<std::string> added;   ///< New routes not in baseline
        std::vector<std::string> removed; ///< Routes removed since baseline
        std::vector<std::string> changed; ///< Routes modified since baseline

        [[nodiscard]] bool hasDrift() const noexcept {
            return !added.empty() || !removed.empty() || !changed.empty();
        }
    };

    [[nodiscard]] DriftReport detectDrift(const std::string& baseline_snapshot) const;

private:
    RouteRegistry() = default;

    mutable std::shared_mutex mutex_;
    std::vector<RouteEntry>  entries_;
};

} // namespace server
} // namespace themis
