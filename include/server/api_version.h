/**
 * @file api_version.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <chrono>

namespace themis::server {

struct APIVersion {
    uint32_t major = 0;
    uint32_t minor;
    uint32_t patch;
    
    /**
     * @brief Parse.
     * @param[in] version_str Input parameter.
     * @return Return value.
     */
    static std::optional<APIVersion> parse(const std::string& version_str);
    
    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
    
    bool operator==(const APIVersion& other) const;
    bool operator!=(const APIVersion& other) const;
    bool operator<(const APIVersion& other) const;
    bool operator<=(const APIVersion& other) const;
    bool operator>(const APIVersion& other) const;
    bool operator>=(const APIVersion& other) const;
};

struct APIDeprecationInfo {
    APIVersion deprecated_in;           // Version when feature was deprecated
    APIVersion removed_in;              // Version when feature will be removed
    std::chrono::system_clock::time_point deprecation_date;
    std::chrono::system_clock::time_point removal_date;
    std::string reason;                 // Why it's being deprecated
    std::string migration_guide_url;    // Link to migration guide
    std::string alternative;            // Alternative endpoint/feature
};

struct APIVersionRange {
    APIVersion min_version;  ///< Inclusive minimum version
    APIVersion max_version;  ///< Inclusive maximum version

    /**
     * @brief Parse.
     * @param[in] range_str Input parameter.
     * @return Return value.
     */
    static std::optional<APIVersionRange> parse(const std::string& range_str);

    /**
     * @brief Contains.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool contains(const APIVersion& version) const;
};

struct BreakingChangeInfo {
    APIVersion introduced_in;       ///< First version that contains the breaking change
    std::string endpoint;           ///< Affected endpoint path (empty = all endpoints)
    std::string description;        ///< Human-readable description
    std::string migration_guide_url;///< Link to migration documentation
};

class APIVersionManager {
public:
    APIVersionManager();
    
    APIVersion getCurrentVersion() const { return current_version_; }
    
    APIVersion getMinimumVersion() const { return minimum_version_; }
    
    /**
     * @brief Is Version Supported.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool isVersionSupported(const APIVersion& version) const;
    
    /**
     * @brief Resolve Version.
     * @param[in] version_header Input parameter.
     * @return Return value.
     */
    APIVersion resolveVersion(const std::string& version_header) const;
    
    /**
     * @brief Get Deprecation Info.
     * @param[in] endpoint Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<APIDeprecationInfo> getDeprecationInfo(
        const std::string& endpoint, 
        const APIVersion& version
    ) const;
    
    /**
     * @brief Register Deprecation.
     * @param[in] endpoint Input parameter.
     * @param[in] info Input parameter.
     */
    void registerDeprecation(
        const std::string& endpoint,
        const APIDeprecationInfo& info
    );
    
    /**
     * @brief Get Supported Versions.
     * @return Return value.
     */
    std::vector<APIVersion> getSupportedVersions() const;

    /**
     * @brief Resolve Version Range.
     * @param[in] range Input parameter.
     * @return Return value.
     */
    APIVersion resolveVersionRange(const APIVersionRange& range) const;

    std::optional<BreakingChangeInfo> isBreakingChange(
        const APIVersion& from,
        const APIVersion& to,
        const std::string& endpoint = ""
    ) const;

    /**
     * @brief Register Breaking Change.
     * @param[in] info Input parameter.
     */
    void registerBreakingChange(const BreakingChangeInfo& info);

private:
    APIVersion current_version_;
    APIVersion minimum_version_;
    std::vector<APIVersion> supported_versions_;
    
    // Map of endpoint to deprecation info
    std::unordered_map<std::string, APIDeprecationInfo> deprecations_;

    // List of registered breaking changes (per endpoint or global)
    std::vector<BreakingChangeInfo> breaking_changes_;
    
    // 24-month deprecation policy in seconds (approximation: 730 days)
    // Note: Uses 730 days as approximation of 24 months for consistency
    static constexpr int64_t DEPRECATION_PERIOD_SECONDS = 730 * 24 * 3600; // 24 months (~2 years)
};

namespace APIHeaders {
    constexpr const char* ACCEPT_VERSION = "Accept-Version";
    constexpr const char* API_VERSION = "API-Version";             ///< Request & response version header
    constexpr const char* ACCEPT_API_VERSION = "Accept-API-Version"; ///< Client version range header
    constexpr const char* API_DEPRECATED = "API-Deprecated";       ///< Deprecation notice response header
    constexpr const char* DEPRECATION_WARNING = "Deprecation";
    constexpr const char* SUNSET = "Sunset";  // RFC 8594 - Sunset header
    constexpr const char* LINK = "Link";  // Link to migration guide
}

// ── Phase 3 Schema-Governance: Backward-Compatibility Contract ────────────────

struct SchemaFieldDescriptor {
    std::string name;      ///< Field name (must be stable across minor versions)
    std::string type;      ///< Wire type string (e.g. "int64", "string", "bool")
    bool        required;  ///< Whether the field is mandatory on the wire

    bool operator==(const SchemaFieldDescriptor& other) const noexcept {
        return name == other.name && type == other.type && required == other.required;
    }
};

struct CompatPolicy {
    bool allow_field_removal{false};  ///< If false, removing a field is a violation
    bool allow_field_rename{false};   ///< If false, renaming a field is a violation
    bool allow_type_narrowing{false}; ///< If false, narrowing a field's wire type is a violation
    bool allow_path_rename{false};    ///< If false, renaming an endpoint path is a violation
};

struct CompatCheckResult {
    bool        passed = 0;            ///< true if the change satisfies the policy
    std::string violation_reason;  ///< Non-empty when passed == false

    explicit operator bool() const noexcept { return passed; }
};

class CompatChecker {
public:
    CompatChecker() = default;

    [[nodiscard]] CompatCheckResult validate(
        const SchemaFieldDescriptor& old_field,
        const SchemaFieldDescriptor& updated_field,
        const CompatPolicy&          policy) const noexcept;

    [[nodiscard]] CompatCheckResult validateRemoval(
        const SchemaFieldDescriptor& removed_field,
        const CompatPolicy&          policy) const noexcept;

    [[nodiscard]] CompatCheckResult validateEndpointRename(
        const std::string& old_path,
        const std::string& new_path,
        const CompatPolicy& policy) const noexcept;
};

} // namespace themis::server
