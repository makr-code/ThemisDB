/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_version.h                                      ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     164                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    
private:
    APIVersion current_version_;
    APIVersion minimum_version_;
    std::vector<APIVersion> supported_versions_;
    
    // Map of endpoint to deprecation info
    std::unordered_map<std::string, APIDeprecationInfo> deprecations_;
    
    // 24-month deprecation policy in seconds (approximation: 730 days)
    // Note: Uses 730 days as approximation of 24 months for consistency
    static constexpr int64_t DEPRECATION_PERIOD_SECONDS = 730 * 24 * 3600; // 24 months (~2 years)
};

/**
 * @brief API Version Header Constants
 */
namespace APIHeaders {
    constexpr const char* ACCEPT_VERSION = "Accept-Version";
    constexpr const char* API_VERSION = "API-Version";  // Response header
    constexpr const char* DEPRECATION_WARNING = "Deprecation";
    constexpr const char* SUNSET = "Sunset";  // RFC 8594 - Sunset header
    constexpr const char* LINK = "Link";  // Link to migration guide
}

} // namespace themis::server
