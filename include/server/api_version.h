/**
 * @file api_version.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief API Version representation following semantic versioning
 * 
 * Supports version format: v{major}.{minor}.{patch}
 * Example: v1.3.0, v2.0.0
 */
struct APIVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    
    /**
     * @brief Parse version string (e.g., "v1.3.0", "1.3", "1")
     * @return Parsed version or std::nullopt if invalid
     */
    static std::optional<APIVersion> parse(const std::string& version_str);
    
    /**
     * @brief Convert to string format (e.g., "v1.3.0")
     */
    std::string toString() const;
    
    /**
     * @brief Compare versions
     */
    bool operator==(const APIVersion& other) const;
    bool operator!=(const APIVersion& other) const;
    bool operator<(const APIVersion& other) const;
    bool operator<=(const APIVersion& other) const;
    bool operator>(const APIVersion& other) const;
    bool operator>=(const APIVersion& other) const;
};

/**
 * @brief API deprecation information
 */
struct APIDeprecationInfo {
    APIVersion deprecated_in;           // Version when feature was deprecated
    APIVersion removed_in;              // Version when feature will be removed
    std::chrono::system_clock::time_point deprecation_date;
    std::chrono::system_clock::time_point removal_date;
    std::string reason;                 // Why it's being deprecated
    std::string migration_guide_url;    // Link to migration guide
    std::string alternative;            // Alternative endpoint/feature
};

/**
 * @brief Represents a client-declared API version range (e.g., "1.0-2.0")
 *
 * Used with the Accept-API-Version request header to indicate the range of
 * API versions the client can accept.
 */
struct APIVersionRange {
    APIVersion min_version;  ///< Inclusive minimum version
    APIVersion max_version;  ///< Inclusive maximum version

    /**
     * @brief Parse a version range string (e.g., "1.0-2.0", "1.2.0-1.4.0")
     * @return Parsed range or std::nullopt if the string is not a valid range
     */
    static std::optional<APIVersionRange> parse(const std::string& range_str);

    /**
     * @brief Check whether @p version falls within this range (inclusive)
     */
    bool contains(const APIVersion& version) const;
};

/**
 * @brief Information about a breaking change between two API versions
 */
struct BreakingChangeInfo {
    APIVersion introduced_in;       ///< First version that contains the breaking change
    std::string endpoint;           ///< Affected endpoint path (empty = all endpoints)
    std::string description;        ///< Human-readable description
    std::string migration_guide_url;///< Link to migration documentation
};

/**
 * @brief API Version Manager - Handles version negotiation and compatibility
 */
class APIVersionManager {
public:
    /**
     * @brief Construct version manager with supported versions
     */
    APIVersionManager();
    
    /**
     * @brief Get current stable API version
     */
    APIVersion getCurrentVersion() const { return current_version_; }
    
    /**
     * @brief Get minimum supported API version
     */
    APIVersion getMinimumVersion() const { return minimum_version_; }
    
    /**
     * @brief Check if a version is supported
     */
    bool isVersionSupported(const APIVersion& version) const;
    
    /**
     * @brief Resolve version from request header (Accept-Version)
     * @param version_header Header value (e.g., "v1.3", "1.0", "latest")
     * @return Resolved version or current version if not specified/invalid
     */
    APIVersion resolveVersion(const std::string& version_header) const;
    
    /**
     * @brief Check if an API endpoint is deprecated in given version
     * @param endpoint API endpoint path
     * @param version Version to check
     * @return Deprecation info if deprecated, std::nullopt otherwise
     */
    std::optional<APIDeprecationInfo> getDeprecationInfo(
        const std::string& endpoint, 
        const APIVersion& version
    ) const;
    
    /**
     * @brief Register a deprecated endpoint
     */
    void registerDeprecation(
        const std::string& endpoint,
        const APIDeprecationInfo& info
    );
    
    /**
     * @brief Get all supported versions
     */
    std::vector<APIVersion> getSupportedVersions() const;

    /**
     * @brief Resolve the best matching API version within a client-declared range.
     *
     * Selects the highest supported version that falls within [@p range.min_version,
     * @p range.max_version].  Falls back to the current version when no supported
     * version fits in the range.
     *
     * @param range  Client-declared acceptable version range.
     * @return Best matching supported version, or the current version as fallback.
     */
    APIVersion resolveVersionRange(const APIVersionRange& range) const;

    /**
     * @brief Check whether upgrading from @p from to @p to introduces a breaking change.
     *
     * Returns the first registered BreakingChangeInfo whose introduced_in version is
     * in the half-open interval ( @p from, @p to ] and whose endpoint matches (or is
     * the catch-all empty endpoint), or std::nullopt when no breaking change is found.
     *
     * @param from      The version the client currently uses.
     * @param to        The target (server) version.
     * @param endpoint  Specific endpoint path to check (empty = global).
     */
    std::optional<BreakingChangeInfo> isBreakingChange(
        const APIVersion& from,
        const APIVersion& to,
        const std::string& endpoint = ""
    ) const;

    /**
     * @brief Register a breaking change between API versions.
     *
     * @param info  Breaking change metadata (endpoint, version, description).
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

/**
 * @brief API Version Header Constants
 */
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

/**
 * @brief Descriptor for a single field in a versioned RPC/REST schema.
 *
 * Used by CompatChecker to verify that changes to message schemas satisfy
 * the active CompatPolicy before a handler is registered.
 */
struct SchemaFieldDescriptor {
    std::string name;      ///< Field name (must be stable across minor versions)
    std::string type;      ///< Wire type string (e.g. "int64", "string", "bool")
    bool        required;  ///< Whether the field is mandatory on the wire

    bool operator==(const SchemaFieldDescriptor& other) const noexcept {
        return name == other.name && type == other.type && required == other.required;
    }
};

/**
 * @brief Compatibility policy enforced at handler-registration time.
 *
 * All flags default to false (strictest possible mode): additive-only
 * changes are permitted by default; any structural change to existing fields
 * requires explicit opt-in via the corresponding flag.
 *
 * The policy is evaluated by CompatChecker::validate() before a versioned
 * handler is accepted into the route registry.  Violations are surfaced at
 * startup time — never silently at runtime on the hot path.
 */
struct CompatPolicy {
    bool allow_field_removal{false};  ///< If false, removing a field is a violation
    bool allow_field_rename{false};   ///< If false, renaming a field is a violation
    bool allow_type_narrowing{false}; ///< If false, narrowing a field's wire type is a violation
    bool allow_path_rename{false};    ///< If false, renaming an endpoint path is a violation
};

/**
 * @brief Result of a CompatChecker validation.
 *
 * Carries a pass/fail flag and, on failure, a human-readable reason string
 * that can be surfaced in startup logs or error messages without overhead
 * on the request hot path.
 */
struct CompatCheckResult {
    bool        passed;            ///< true if the change satisfies the policy
    std::string violation_reason;  ///< Non-empty when passed == false

    /// @return true if the check passed with no violations.
    explicit operator bool() const noexcept { return passed; }
};

/**
 * @brief Registration-time backward-compatibility contract enforcer.
 *
 * CompatChecker validates that any versioned proto/schema change made at
 * handler-registration time satisfies the active CompatPolicy.  All checks
 * are performed once, at registration time, imposing zero overhead on the
 * request hot path.
 *
 * ## Usage
 *
 * ```cpp
 * CompatChecker checker;
 * CompatPolicy  policy; // default: strictest mode
 *
 * // New optional field added to an existing message — passes
 * SchemaFieldDescriptor old_field{"user_id", "int64", true};
 * SchemaFieldDescriptor new_field{"user_id", "int64", true}; // unchanged
 * auto result = checker.validate(old_field, new_field, policy);
 * assert(result.passed);
 *
 * // Required field removed — fails
 * SchemaFieldDescriptor removed{"name", "string", true};
 * auto result2 = checker.validate(removed, std::nullopt, policy);
 * assert(!result2.passed);
 * ```
 *
 * @note All public methods are reentrant; CompatChecker holds no mutable state.
 * @see CompatPolicy, CompatCheckResult, SchemaFieldDescriptor
 */
class CompatChecker {
public:
    CompatChecker() = default;

    /**
     * @brief Validate a field-level schema change against a CompatPolicy.
     *
     * Compares @p old_field to @p updated_field and returns a failure result
     * if the change violates the policy.
     *
     * Rules (when the corresponding policy flag is false):
     * - Field name change                  → violation (field rename)
     * - Wire type change to narrower type  → violation (type narrowing)
     * - required=true → required=false     → always allowed (widening)
     * - required=false → required=true     → violation (breaking wire change)
     *
     * @param old_field     Previous field descriptor (version N).
     * @param updated_field Updated field descriptor (version N+1 candidate).
     * @param policy        Active CompatPolicy to enforce.
     * @return CompatCheckResult with passed=true if the change is compliant.
     */
    [[nodiscard]] CompatCheckResult validate(
        const SchemaFieldDescriptor& old_field,
        const SchemaFieldDescriptor& updated_field,
        const CompatPolicy&          policy) const noexcept;

    /**
     * @brief Validate the removal of a field against a CompatPolicy.
     *
     * If allow_field_removal is false (default), removing any field is a
     * violation regardless of whether the field was required or optional.
     *
     * @param removed_field  Field that would be removed.
     * @param policy         Active CompatPolicy.
     * @return CompatCheckResult with passed=false if removal is disallowed.
     */
    [[nodiscard]] CompatCheckResult validateRemoval(
        const SchemaFieldDescriptor& removed_field,
        const CompatPolicy&          policy) const noexcept;

    /**
     * @brief Validate an endpoint path rename against a CompatPolicy.
     *
     * REST path renames are breaking changes for any client that has
     * hard-coded the old path.  If allow_path_rename is false (default),
     * this method returns a failure result.
     *
     * @param old_path  Current endpoint path (e.g. "/api/v1/users").
     * @param new_path  Proposed new endpoint path.
     * @param policy    Active CompatPolicy.
     * @return CompatCheckResult with passed=false if rename is disallowed.
     */
    [[nodiscard]] CompatCheckResult validateEndpointRename(
        const std::string& old_path,
        const std::string& new_path,
        const CompatPolicy& policy) const noexcept;
};

} // namespace themis::server
