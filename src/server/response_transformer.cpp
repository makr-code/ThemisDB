/**
 * @file response_transformer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/response_transformer.h"
#include <spdlog/spdlog.h>

namespace themis::server {

void ResponseTransformer::registerVersion(const std::string& version_key, TransformFn fn) {
    transforms_[version_key] = std::move(fn);
    spdlog::debug("ResponseTransformer: registered transform for version '{}'", version_key);
}

void ResponseTransformer::addFieldRename(const std::string& version_key,
                                         const std::string& old_name,
                                         const std::string& new_name) {
    field_renames_[version_key].emplace_back(old_name, new_name);
    spdlog::debug("ResponseTransformer: registered field rename '{}' → '{}' for version '{}'",
                  old_name, new_name, version_key);
}

void ResponseTransformer::addDefaultValue(const std::string& version_key,
                                           const std::string& field_name,
                                           const nlohmann::json& default_value) {
    default_values_[version_key].emplace_back(field_name, default_value);
    spdlog::debug("ResponseTransformer: registered default value for field '{}' in version '{}'",
                  field_name, version_key);
}

std::optional<std::string> ResponseTransformer::resolveKey(const APIVersion& version) const {
    // Try exact semver key first
    std::string exact = version.toString(); // "v{major}.{minor}.{patch}"
    if (transforms_.count(exact) || field_renames_.count(exact) || default_values_.count(exact)) {
        return exact;
    }

    // Try major.minor key
    std::string major_minor = "v" + std::to_string(version.major) + "." + std::to_string(version.minor);
    if (transforms_.count(major_minor) || field_renames_.count(major_minor) || default_values_.count(major_minor)) {
        return major_minor;
    }

    // Try major-only key
    std::string major_only = "v" + std::to_string(version.major);
    if (transforms_.count(major_only) || field_renames_.count(major_only) || default_values_.count(major_only)) {
        return major_only;
    }

    return std::nullopt;
}

void ResponseTransformer::applyFieldMappings(nlohmann::json& obj,
                                              const std::string& version_key) const {
    // Apply field renames
    auto rename_it = field_renames_.find(version_key);
    if (rename_it != field_renames_.end()) {
        for (const auto& [old_name, new_name] : rename_it->second) {
            if (obj.contains(old_name)) {
                obj[new_name] = obj[old_name];
                obj.erase(old_name);
            }
        }
    }

    // Apply default values for missing fields
    auto default_it = default_values_.find(version_key);
    if (default_it != default_values_.end()) {
        for (const auto& [field_name, default_val] : default_it->second) {
            if (!obj.contains(field_name)) {
                obj[field_name] = default_val;
            }
        }
    }
}

nlohmann::json ResponseTransformer::transform(const nlohmann::json& payload,
                                               const APIVersion& version) const {
    auto key_opt = resolveKey(version);
    if (!key_opt) {
        // No transform registered — return payload unchanged
        return payload;
    }

    const std::string& key = *key_opt;

    // Start with a copy of the native payload
    nlohmann::json result = payload;

    // Apply field renames and default values first
    applyFieldMappings(result, key);

    // Then apply the explicit transform function, if any
    auto fn_it = transforms_.find(key);
    if (fn_it != transforms_.end()) {
        result = fn_it->second(std::move(result));
    }

    return result;
}

bool ResponseTransformer::hasVersion(const APIVersion& version) const {
    return resolveKey(version).has_value();
}

std::vector<std::string> ResponseTransformer::registeredVersions() const {
    std::vector<std::string> keys = {};

    keys.reserve(transforms_.size());
    for (const auto& [k, _] : transforms_) {
        keys.push_back(k);
    }
    // Also include keys that only have field mappings (no explicit transform fn)
    for (const auto& [k, _] : field_renames_) {
        if (!transforms_.count(k)) {
            keys.push_back(k);
        }
    }
    for (const auto& [k, _] : default_values_) {
        if (!transforms_.count(k) && !field_renames_.count(k)) {
            keys.push_back(k);
        }
    }
    return keys;
}

} // namespace themis::server
