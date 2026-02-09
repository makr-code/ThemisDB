#include "server/api_auth_config.h"
#include <algorithm>

namespace themis {
namespace server {

std::optional<EndpointAuthConfig> ApiAuthConfig::getEndpointConfig(const std::string& path) const {
    // Check for exact match first
    for (const auto& config : endpoint_configs) {
        if (config.endpoint_pattern == path) {
            return config;
        }
    }
    
    // Check for wildcard patterns
    for (const auto& config : endpoint_configs) {
        const auto& pattern = config.endpoint_pattern;
        
        // Handle wildcard patterns like "/entities/*"
        if (pattern.back() == '*') {
            std::string prefix = pattern.substr(0, pattern.length() - 1);
            if (path.rfind(prefix, 0) == 0) {
                return config;
            }
        }
    }
    
    return std::nullopt;
}

ApiAuthConfig ApiAuthConfig::createSecureDefaults() {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;
    config.global_rate_limit_per_minute = 100;
    config.global_rate_limit_burst = 100;
    config.audit_rate_limit_per_minute = 50; // More restrictive for audit
    
    // Configure endpoint-specific settings
    config.endpoint_configs = {
        // Data access endpoints - require authentication
        {"/entities/*", "data:read", "read", true, 1000, 100},
        {"/entities", "data:write", "write", true, 500, 50},
        {"/query", "data:read", "query", true, 500, 50},
        {"/query/aql", "data:read", "query", true, 300, 30},
        
        // Index management - require admin or data:write
        {"/index/*", "data:write", "index", true, 100, 20},
        
        // Vector operations
        {"/vector/*", "data:read", "vector", true, 500, 50},
        
        // Graph operations
        {"/graph/*", "data:read", "graph", true, 500, 50},
        
        // Content management
        {"/content/*", "content:read", "content", true, 500, 50},
        {"/contentfs/*", "content:read", "content", true, 500, 50},
        
        // Transaction management
        {"/transaction/*", "data:write", "transaction", true, 100, 20},
        
        // Changefeed/CDC
        {"/changefeed/*", "cdc:read", "cdc", true, 100, 20},
        
        // Time-series
        {"/timeseries/*", "timeseries:read", "timeseries", true, 500, 50},
        
        // Spatial/GIS
        {"/spatial/*", "data:read", "spatial", true, 500, 50},
        
        // Cache operations
        {"/cache/*", "cache:read", "cache", true, 1000, 100},
        
        // Prompt management
        {"/prompt/*", "llm:read", "prompt", true, 200, 30},
        
        // Admin endpoints - require admin scope
        {"/admin/*", "admin", "admin", true, 50, 10},
        {"/config", "config:read", "config.read", true, 10, 5},
        
        // Audit endpoints - very restrictive
        {"/api/audit/*", "audit:read", "audit.read", true, 50, 10},
        
        // PKI endpoints
        {"/api/pki/*", "pki:read", "pki", true, 100, 20},
        
        // PII endpoints
        {"/pii/*", "pii:read", "pii", true, 50, 10},
        
        // Monitoring/health endpoints - public but rate-limited
        {"/health", "", "", false, 1000, 100},
        {"/version", "", "", false, 1000, 100},
        {"/stats", "", "", false, 100, 20},
        {"/capabilities", "", "", false, 100, 20},
        {"/metrics", "", "", false, 100, 20}
    };
    
    return config;
}

ApiAuthConfig ApiAuthConfig::createDevDefaults() {
    ApiAuthConfig config;
    config.auth_enabled = false;
    config.rate_limiting_enabled = true;
    config.global_rate_limit_per_minute = 10000; // Very lenient for development
    config.global_rate_limit_burst = 1000;
    config.audit_rate_limit_per_minute = 1000;
    
    // No endpoint-specific restrictions in dev mode
    config.endpoint_configs = {};
    
    return config;
}

} // namespace server
} // namespace themis
