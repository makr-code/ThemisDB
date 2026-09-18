/**
 * @file response_transformer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ResponseTransformer {
public:
    using TransformFn = std::function<nlohmann::json(nlohmann::json)>;

    ResponseTransformer() = default;

    // -----------------------------------------------------------------------
    // Version registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register Version.
     * @param[in] version_key Input parameter.
     * @param[in] fn Input parameter.
     */
    void registerVersion(const std::string& version_key, TransformFn fn);

    // -----------------------------------------------------------------------
    // Convenience helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Add Field Rename.
     * @param[in] version_key Input parameter.
     * @param[in] old_name Name of the old.
     * @param[in] new_name Name of the new.
     */
    void addFieldRename(const std::string& version_key,
                        const std::string& old_name,
                        const std::string& new_name);

    /**
     * @brief Add Default Value.
     * @param[in] version_key Input parameter.
     * @param[in] field_name Name of the field.
     * @param[in] default_value Input parameter.
     */
    void addDefaultValue(const std::string& version_key,
                         const std::string& field_name,
                         const nlohmann::json& default_value);

    // -----------------------------------------------------------------------
    // Transformation
    // -----------------------------------------------------------------------

    /**
     * @brief Transform.
     * @param[in] payload Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    nlohmann::json transform(const nlohmann::json& payload,
                             const APIVersion& version) const;

    /**
     * @brief Has Version.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasVersion(const APIVersion& version) const;

    /**
     * @brief Registered Versions.
     * @return Return value.
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
     * @brief Resolve Key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<std::string> resolveKey(const APIVersion& version) const;

    /**
     * @brief Apply Field Mappings.
     * @param[in,out] obj Input/output parameter.
     * @param[in] version_key Input parameter.
     */
    void applyFieldMappings(nlohmann::json& obj, const std::string& version_key) const;
};

} // namespace themis::server
