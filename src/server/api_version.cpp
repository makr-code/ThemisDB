/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_version.cpp                                    ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     251                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
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
        if (found) return best;
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
        if (found) return best;
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

} // namespace themis::server
