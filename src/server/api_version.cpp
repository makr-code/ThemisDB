/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_version.cpp                                    ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     199                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
    
    auto parsed = APIVersion::parse(version_header);
    if (!parsed) {
        spdlog::warn("Invalid API version format: '{}', using current version", version_header);
        return current_version_;
    }
    
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
