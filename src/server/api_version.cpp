/**
 * @file api_version.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/api_version.h"
#include "server/api_version_config.h"
#include <sstream>
#include <regex>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace themis::server {

// Parse version string
std::optional<APIVersion> APIVersion::parse(const std::string& version_str) {
    if (version_str.empty()) {
        return std::nullopt;
    }
    
    std::string str = version_str;
    
    // Handle "latest" keyword - use current version from config
    if (str == "latest") {
        return APIVersion{
            APIVersionConfig::CURRENT_MAJOR,
            APIVersionConfig::CURRENT_MINOR,
            APIVersionConfig::CURRENT_PATCH
        };
    }
    
    // Remove 'v' prefix if present
    if (str[0] == 'v' || str[0] == 'V') {
        str = str.substr(1);
    }
    
    // Parse version components
    std::regex version_regex(R"(^(\d+)(?:\.(\d+))?(?:\.(\d+))?$)");
    std::smatch match;
    
    if (!std::regex_match(str, match, version_regex)) {
        return std::nullopt;
    }
    
    APIVersion version;
    version.major = std::stoul(match[1].str());
    version.minor = match[2].matched ? std::stoul(match[2].str()) : 0;
    version.patch = match[3].matched ? std::stoul(match[3].str()) : 0;
    
    return version;
}

// Convert to string
std::string APIVersion::toString() const {
    return "v" + std::to_string(major) + "." + 
           std::to_string(minor) + "." + 
           std::to_string(patch);
}

// Comparison operators
bool APIVersion::operator==(const APIVersion& other) const {
    return major == other.major && minor == other.minor && patch == other.patch;
}

bool APIVersion::operator!=(const APIVersion& other) const {
    return !(*this == other);
}

bool APIVersion::operator<(const APIVersion& other) const {
    if (major != other.major) {
      return major < other.major;
    }
    if (minor != other.minor) {
      return minor < other.minor;
    }
    return patch < other.patch;
}

bool APIVersion::operator<=(const APIVersion& other) const {
    return *this < other || *this == other;
}

bool APIVersion::operator>(const APIVersion& other) const {
    return !(*this <= other);
}

bool APIVersion::operator>=(const APIVersion& other) const {
    return !(*this < other);
}

// APIVersionManager implementation
APIVersionManager::APIVersionManager() 
    : current_version_{
        APIVersionConfig::CURRENT_MAJOR,
        APIVersionConfig::CURRENT_MINOR,
        APIVersionConfig::CURRENT_PATCH
      },
      minimum_version_{
        APIVersionConfig::MINIMUM_MAJOR,
        APIVersionConfig::MINIMUM_MINOR,
        APIVersionConfig::MINIMUM_PATCH
      }
{
    // Initialize supported versions
    // Support current major version and previous major version for backward compatibility
    supported_versions_ = {
        {1, 0, 0},
        {1, 1, 0},
        {1, 2, 0},
        {1, 3, 0},
        {1, 4, 0},
        {APIVersionConfig::CURRENT_MAJOR, APIVersionConfig::CURRENT_MINOR, APIVersionConfig::CURRENT_PATCH}
    };
    
    spdlog::info("APIVersionManager initialized: current={}, minimum={}", 
                 current_version_.toString(), minimum_version_.toString());
}

bool APIVersionManager::isVersionSupported(const APIVersion& version) const {
    // Check if version is within supported range
    if (version < minimum_version_ || version > current_version_) {
        return false;
    }
    
    // Check if exact version is in supported list
    return std::find(supported_versions_.begin(), supported_versions_.end(), version) 
           != supported_versions_.end();
}

APIVersion APIVersionManager::resolveVersion(const std::string& version_header) const {
    if (version_header.empty()) {
        // No version specified, use current
        return current_version_;
    }

    // Detect how many version components the client specified so we can resolve
    // partial versions to the latest matching release:
    //   "v1"     → latest minor.patch for major == 1
    //   "v1.4"   → latest patch for major == 1, minor == 4
    //   "v1.4.1" → exact match
    // Strip leading 'v'/'V' for component counting
    std::string stripped = version_header;
    if (!stripped.empty() && (stripped[0] == 'v' || stripped[0] == 'V')) {
        stripped = stripped.substr(1);
    }
    auto dot1 = stripped.find('.');
    auto dot2 = (dot1 != std::string::npos) ? stripped.find('.', dot1 + 1) : std::string::npos;

    auto parsed = APIVersion::parse(version_header);
    if (!parsed) {
        spdlog::warn("Invalid API version format: '{}', using current version", version_header);
        return current_version_;
    }

    // Major-only (e.g. "v1"): resolve to latest supported minor.patch for that major
    if (dot1 == std::string::npos) {
        uint32_t req_major = parsed->major;
        APIVersion best{req_major, 0, 0};
        bool found = false;
        for (const auto& v : supported_versions_) {
            if (v.major == req_major) {
                if (!found || v > best) {
                    best = v;
                    found = true;
                }
            }
        }
        if (found) {
          return best;
        }
        spdlog::warn("No supported version found for major {}, using current version", req_major);
        return current_version_;
    }

    // Major.minor-only (e.g. "v1.4"): resolve to latest supported patch for that major.minor
    if (dot2 == std::string::npos) {
        uint32_t req_major = parsed->major;
        uint32_t req_minor = parsed->minor;
        APIVersion best{req_major, req_minor, 0};
        bool found = false;
        for (const auto& v : supported_versions_) {
            if (v.major == req_major && v.minor == req_minor) {
                if (!found || v.patch > best.patch) {
                    best = v;
                    found = true;
                }
            }
        }
        if (found) {
          return best;
        }
        spdlog::warn("No supported version found for {}.{}, using current version",
                     req_major, req_minor);
        return current_version_;
    }

    // Full version (e.g. "v1.4.1"): exact match
    if (!isVersionSupported(*parsed)) {
        spdlog::warn("Unsupported API version: {}, using current version", parsed->toString());
        return current_version_;
    }

    return *parsed;
}

std::optional<APIDeprecationInfo> APIVersionManager::getDeprecationInfo(
    const std::string& endpoint,
    const APIVersion& version
) const {
    auto it = deprecations_.find(endpoint);
    if (it == deprecations_.end()) {
        return std::nullopt;
    }
    
    const auto& info = it->second;
    
    // Check if this version is affected by deprecation
    if (version >= info.deprecated_in && version < info.removed_in) {
        return info;
    }
    
    return std::nullopt;
}

void APIVersionManager::registerDeprecation(
    const std::string& endpoint,
    const APIDeprecationInfo& info
) {
    deprecations_[endpoint] = info;
    spdlog::info("Registered deprecation for endpoint '{}': deprecated in {}, removed in {}", 
                 endpoint, info.deprecated_in.toString(), info.removed_in.toString());
}

std::vector<APIVersion> APIVersionManager::getSupportedVersions() const {
    return supported_versions_;
}

// APIVersionRange implementation

std::optional<APIVersionRange> APIVersionRange::parse(const std::string& range_str) {
    if (range_str.empty()) {
        return std::nullopt;
    }

    auto dash_pos = range_str.find('-');
    if (dash_pos == std::string::npos) {
        return std::nullopt;
    }

    // Trim leading/trailing whitespace from each token so that header values
    // like "1.0 - 2.0" (with spaces around the dash) parse correctly.
    auto trim = [](const std::string& s) {
        const char* ws = " \t\r\n";
        auto start = s.find_first_not_of(ws);
        if (start == std::string::npos) return std::string{};
        auto end = s.find_last_not_of(ws);
        return s.substr(start, end - start + 1);
    };

    std::string min_str = trim(range_str.substr(0, dash_pos));
    std::string max_str = trim(range_str.substr(dash_pos + 1));

    auto min_v = APIVersion::parse(min_str);
    auto max_v = APIVersion::parse(max_str);

    if (!min_v || !max_v) {
        return std::nullopt;
    }

    if (*min_v > *max_v) {
        return std::nullopt;
    }

    return APIVersionRange{*min_v, *max_v};
}

bool APIVersionRange::contains(const APIVersion& version) const {
    return version >= min_version && version <= max_version;
}

// resolveVersionRange: pick best (highest) supported version within range

APIVersion APIVersionManager::resolveVersionRange(const APIVersionRange& range) const {
    APIVersion best = current_version_;
    bool found = false;

    for (const auto& v : supported_versions_) {
        if (range.contains(v)) {
            if (!found || v > best) {
                best = v;
                found = true;
            }
        }
    }

    if (!found) {
        spdlog::warn("No supported version found in range {}-{}, using current version",
                     range.min_version.toString(), range.max_version.toString());
        return current_version_;
    }

    return best;
}

// Breaking change registry

void APIVersionManager::registerBreakingChange(const BreakingChangeInfo& info) {
    breaking_changes_.push_back(info);
    spdlog::info("Registered breaking change at {}: {} (endpoint='{}')",
                 info.introduced_in.toString(), info.description,
                 info.endpoint.empty() ? "<all>" : info.endpoint);
}

std::optional<BreakingChangeInfo> APIVersionManager::isBreakingChange(
    const APIVersion& from,
    const APIVersion& to,
    const std::string& endpoint
) const {
    for (const auto& bc : breaking_changes_) {
        // Breaking change applies if its version is strictly after `from` and at
        // most `to` (i.e. the client would cross it when upgrading from→to).
        if (bc.introduced_in > from && bc.introduced_in <= to) {
            // Scope: global (empty endpoint) or matching endpoint
            if (bc.endpoint.empty() || bc.endpoint == endpoint) {
                return bc;
            }
        }
    }
    return std::nullopt;
}

} // namespace themis::server

// ── Phase 3 Schema-Governance: CompatChecker ─────────────────────────────────

namespace themis::server {

namespace {

/// Determine whether @p new_type is a narrowing of @p old_type.
/// Narrowing means the new type can represent fewer values than the old type
/// (e.g. int64 → int32 is narrowing; int32 → int64 is widening).
bool isTypeNarrowing(const std::string& old_type, const std::string& new_type) {
    if (old_type == new_type) { return false; }
    // Well-known narrowing pairs
    static const std::vector<std::pair<std::string,std::string>> kNarrowing = {
        {"int64",  "int32"},
        {"int64",  "uint32"},
        {"int64",  "int16"},
        {"int32",  "int16"},
        {"uint64", "uint32"},
        {"uint32", "uint16"},
        {"double", "float"},
        {"string", "bytes"},  // Semantic narrowing: less general encoding
        {"bytes",  "string"},
    };
    for (const auto& [wide, narrow] : kNarrowing) {
        if (old_type == wide && new_type == narrow) { return true; }
    }
    return false;
}

} // anonymous namespace

CompatCheckResult CompatChecker::validate(
    const SchemaFieldDescriptor& old_field,
    const SchemaFieldDescriptor& updated_field,
    const CompatPolicy&          policy) const noexcept
{
    // 1. Field rename check
    if (old_field.name != updated_field.name && !policy.allow_field_rename) {
        return {false,
                "Field rename not allowed by CompatPolicy: '"
                + old_field.name + "' → '" + updated_field.name + "'"};
    }

    // 2. Type narrowing check
    if (isTypeNarrowing(old_field.type, updated_field.type)
        && !policy.allow_type_narrowing) {
        return {false,
                "Type narrowing not allowed by CompatPolicy: '"
                + old_field.name + "' " + old_field.type
                + " → " + updated_field.type};
    }

    // 3. Optional → required is a breaking change (unconditional: not covered
    //    by any CompatPolicy flag because it always breaks existing senders)
    if (!old_field.required && updated_field.required) {
        return {false,
                "Making an optional field required is always a breaking change: '"
                + updated_field.name + "'"};
    }

    return {true, ""};
}

CompatCheckResult CompatChecker::validateRemoval(
    const SchemaFieldDescriptor& removed_field,
    const CompatPolicy&          policy) const noexcept
{
    if (!policy.allow_field_removal) {
        return {false,
                "Field removal not allowed by CompatPolicy: '"
                + removed_field.name + "' (type=" + removed_field.type + ")"};
    }
    return {true, ""};
}

CompatCheckResult CompatChecker::validateEndpointRename(
    const std::string& old_path,
    const std::string& new_path,
    const CompatPolicy& policy) const noexcept
{
    if (old_path == new_path) { return {true, ""}; }
    if (!policy.allow_path_rename) {
        return {false,
                "Endpoint path rename not allowed by CompatPolicy: '"
                + old_path + "' → '" + new_path + "'"};
    }
    return {true, ""};
}

} // namespace themis::server
