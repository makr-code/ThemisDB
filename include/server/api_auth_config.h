/**
 * @file api_auth_config.h
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
#include <optional>
#include <cstdint>

namespace themis {
namespace server {

/**
 * @brief Per-endpoint authentication and rate-limiting configuration
 * 
 * This configuration allows fine-grained control over authentication
 * and rate-limiting for individual API endpoints or endpoint groups.
 */
struct EndpointAuthConfig {
    // Endpoint pattern (e.g., "/entities/*", "/query", "/api/pki/*")
    std::string endpoint_pattern;
    
    // HTTP method (e.g., "GET", "POST", "PUT", "DELETE", "*" for all methods)
    std::string http_method = "*";
    
    // Required scope for this endpoint (e.g., "data:read", "data:write", "admin")
    std::string required_scope;
    
    // Action description for policy engine (e.g., "read", "write", "delete")
    std::string action;
    
    // Whether authentication is required (false = public endpoint)
    bool auth_required = true;
    
    // Per-endpoint rate limit (requests per minute, 0 = no limit)
    uint32_t rate_limit_per_minute = 0;
    
    // Burst capacity for rate limiting (0 = use global default)
    uint32_t rate_limit_burst = 0;
};

/**
 * @brief Global API authentication and rate-limiting configuration
 * 
 * Provides default settings and endpoint-specific overrides for
 * authentication and rate-limiting across all REST/HTTP endpoints.
 */
struct ApiAuthConfig {
    // Global settings
    bool auth_enabled = false;                      // Enable authentication globally
    bool rate_limiting_enabled = true;              // Enable rate limiting globally
    uint32_t global_rate_limit_per_minute = 100;   // Global rate limit
    uint32_t global_rate_limit_burst = 100;        // Global burst capacity
    
    // Audit endpoint rate limiting (more restrictive by default)
    uint32_t audit_rate_limit_per_minute = 100;
    
    // Whitelist IPs (no authentication or rate limiting)
    std::vector<std::string> whitelist_ips;
    
    // Per-endpoint configurations
    std::vector<EndpointAuthConfig> endpoint_configs;
    
    /**
     * @brief Get endpoint configuration for a given path and HTTP method
     * @param path The request path (e.g., "/entities/123")
     * @param method The HTTP method (e.g., "GET", "POST", "PUT", "DELETE")
     * @return Configuration if found, nullopt otherwise
     */
    std::optional<EndpointAuthConfig> getEndpointConfig(const std::string& path, const std::string& method = "*") const;
    
    /**
     * @brief Initialize with default secure configuration
     * 
     * Sets up recommended authentication and rate-limiting defaults
     * for all standard ThemisDB endpoints.
     */
    static ApiAuthConfig createSecureDefaults();
    
    /**
     * @brief Initialize with development-friendly configuration
     * 
     * Disables authentication and uses lenient rate limits.
     * NOT recommended for production use.
     */
    static ApiAuthConfig createDevDefaults();
};

} // namespace server
} // namespace themis
