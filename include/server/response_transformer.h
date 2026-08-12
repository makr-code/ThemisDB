/**
 * @file response_transformer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/api_version.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace themis::server {

/**
 * @brief Response/request transformation layer for API evolution without breaking changes.
 *
 * Supports multiple API versions simultaneously by applying version-specific
 * transforms to JSON payloads.  Each transform is a plain function that
 * receives the *native* (current) JSON object and returns the transformed
 * view appropriate for the requested version.
 *
 * ## Usage
 * @code
 * ResponseTransformer transformer;
 *
 * // v1 API renames "id" → "user_id"
 * transformer.registerVersion("v1", [](nlohmann::json res) {
 *     if (res.contains("id")) {
 *         res["user_id"] = res["id"];
 *         res.erase("id");
 *     }
 *     return res;
 * });
 *
 * // v2 is the native format – no transform needed
 * transformer.registerVersion("v2", [](nlohmann::json res) { return res; });
 *
 * nlohmann::json native = {{"id", 123}, {"type", "user"}};
 * auto v1_view = transformer.transform(native, APIVersion{1, 0, 0});
 * // v1_view == {{"user_id", 123}, {"type", "user"}}
 * @endcode
 *
 * ### Field renames and default values
 *
 * Convenience helpers allow registering per-version field renames and default
 * values without writing custom transform lambdas:
 *
 * @code
 * transformer.addFieldRename("v1", "id", "user_id");
 * transformer.addDefaultValue("v2", "type", "user");
 * @endcode
 */
class ResponseTransformer {
public:
    /// Transform function: receives a copy of the native payload and returns the adapted payload.
    using TransformFn = std::function<nlohmann::json(nlohmann::json)>;

    ResponseTransformer() = default;

    // -----------------------------------------------------------------------
    // Version registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register a version-specific serializer/transform function.
     *
     * The version key may be a plain major version string ("v1", "v2"), a
     * major.minor string ("v1.4"), or a full semver string ("v1.4.0").
     * When looking up a transform, the manager tries keys in order:
     *  1. Exact semver key (e.g. "v1.4.0")
     *  2. Major.minor key (e.g. "v1.4")
     *  3. Major-only key (e.g. "v1")
     *
     * @param version_key  Version string, e.g. "v1", "v1.4", or "v1.4.0".
     * @param fn           Transform function applied to the native payload.
     */
    void registerVersion(const std::string& version_key, TransformFn fn);

    // -----------------------------------------------------------------------
    // Convenience helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Register a field rename for a specific version.
     *
     * When transforming for @p version_key, field @p old_name is renamed to
     * @p new_name in the output object.  If @p old_name is absent, the output
     * is unchanged.
     *
     * Multiple renames for the same version are applied in registration order.
     *
     * @param version_key  Version key (e.g. "v1").
     * @param old_name     Source field name in the native payload.
     * @param new_name     Target field name in the versioned payload.
     */
    void addFieldRename(const std::string& version_key,
                        const std::string& old_name,
                        const std::string& new_name);

    /**
     * @brief Register a default value for a missing field in a specific version.
     *
     * When transforming for @p version_key, if @p field_name is absent from the
     * payload, it is populated with @p default_value.
     *
     * @param version_key    Version key (e.g. "v2").
     * @param field_name     Field to populate when missing.
     * @param default_value  Value to assign.
     */
    void addDefaultValue(const std::string& version_key,
                         const std::string& field_name,
                         const nlohmann::json& default_value);

    // -----------------------------------------------------------------------
    // Transformation
    // -----------------------------------------------------------------------

    /**
     * @brief Apply the registered transform for the given API version.
     *
     * Resolution order:
     *  1. Exact semver key (e.g. "v1.4.0")
     *  2. Major.minor key (e.g. "v1.4")
     *  3. Major-only key (e.g. "v1")
     *  4. No transform registered → return @p payload unchanged.
     *
     * Field renames and default values registered via addFieldRename /
     * addDefaultValue are applied *before* any registered TransformFn.
     *
     * @param payload  Native (current-version) JSON payload.
     * @param version  Requested API version.
     * @return Transformed JSON payload for the requested version.
     */
    nlohmann::json transform(const nlohmann::json& payload,
                             const APIVersion& version) const;

    /**
     * @brief Check whether a transform is registered for the given version.
     */
    bool hasVersion(const APIVersion& version) const;

    /**
     * @brief Return all registered version keys.
     */
    std::vector<std::string> registeredVersions() const;

private:
    // Explicit transform functions (registered via registerVersion)
    std::unordered_map<std::string, TransformFn> transforms_;

    // Per-version field renames: version_key → list of (old_name, new_name)
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> field_renames_;

    // Per-version default values: version_key → list of (field_name, value)
    std::unordered_map<std::string, std::vector<std::pair<std::string, nlohmann::json>>> default_values_;

    /**
     * @brief Resolve the best matching transform key for a given version.
     *
     * Tries "v{major}.{minor}.{patch}", "v{major}.{minor}", "v{major}" in order.
     * Returns std::nullopt when no key matches.
     */
    std::optional<std::string> resolveKey(const APIVersion& version) const;

    /**
     * @brief Apply field renames and default values for a version key in-place.
     */
    void applyFieldMappings(nlohmann::json& obj, const std::string& version_key) const;
};

} // namespace themis::server
