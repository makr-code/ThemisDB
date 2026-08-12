/**
 * @file api_auth_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/api_auth_config.h"

namespace themis {
namespace server {

std::optional<EndpointAuthConfig> ApiAuthConfig::getEndpointConfig(const std::string& path, const std::string& method) const {
    const bool method_any = method.empty() || method == "*";

    // Check for exact match first (path + method)
    for (const auto& config : endpoint_configs) {
        if (config.endpoint_pattern == path) {
            // Match if method matches or config allows all methods
            if (method_any || config.http_method == "*" || config.http_method == method) {
                return config;
            }
        }
    }
    
    // Check for wildcard patterns, preferring the longest (most specific) prefix
    std::optional<EndpointAuthConfig> best_match;
    std::size_t best_prefix_len = 0;
    
    for (const auto& config : endpoint_configs) {
        const auto& pattern = config.endpoint_pattern;
        
        // Handle wildcard patterns like "/entities/*"
        if (!pattern.empty() && pattern.back() == '*') {
            std::string prefix = pattern.substr(0, pattern.length() - 1);
            if (!prefix.empty() && path.rfind(prefix, 0) == 0) {
                // Match if method matches or config allows all methods
                if (method_any || config.http_method == "*" || config.http_method == method) {
                    // Prefer longer (more specific) prefix
                    if (prefix.length() > best_prefix_len) {
                        best_prefix_len = prefix.length();
                        best_match = config;
                    }
                }
            }
        }
    }
    
    if (best_match.has_value()) {
        return best_match;
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
        {"/entities/*", "GET", "data:read", "read", true, 1000, 100},
        {"/entities/*", "POST", "data:write", "write", true, 500, 50},
        {"/entities/*", "PUT", "data:write", "write", true, 500, 50},
        {"/entities/*", "DELETE", "data:write", "delete", true, 500, 50},
        {"/entities", "GET", "data:read", "read", true, 1000, 100},
        {"/entities", "POST", "data:write", "write", true, 500, 50},
        {"/entities", "PUT", "data:write", "write", true, 500, 50},
        {"/entities", "DELETE", "data:write", "delete", true, 500, 50},
        {"/query", "*", "data:read", "query", true, 500, 50},
        {"/query/aql", "*", "data:read", "query", true, 300, 30},
        
        // Index management - require admin or data:write
        {"/index/*", "*", "data:write", "index", true, 100, 20},
        
        // Vector operations
        {"/vector/*", "GET", "data:read", "vector", true, 500, 50},
        {"/vector/*", "POST", "data:write", "vector", true, 500, 50},
        {"/vector/*", "PUT", "data:write", "vector", true, 500, 50},
        {"/vector/*", "DELETE", "data:write", "vector", true, 500, 50},
        
        // Graph operations
        {"/graph/*", "GET", "data:read", "graph", true, 500, 50},
        {"/graph/*", "POST", "data:write", "graph", true, 500, 50},
        {"/graph/*", "DELETE", "data:write", "graph", true, 500, 50},
        
        // Content management
        {"/content/*", "GET", "content:read", "content", true, 500, 50},
        {"/content/*", "POST", "content:write", "content", true, 500, 50},
        {"/content/*", "PUT", "content:write", "content", true, 500, 50},
        {"/content/*", "DELETE", "content:write", "content", true, 500, 50},
        {"/contentfs/*", "GET", "content:read", "content", true, 500, 50},
        {"/contentfs/*", "POST", "content:write", "content", true, 500, 50},
        {"/contentfs/*", "PUT", "content:write", "content", true, 500, 50},
        {"/contentfs/*", "DELETE", "content:write", "content", true, 500, 50},
        
        // Transaction management
        {"/transaction/*", "*", "data:write", "transaction", true, 100, 20},
        
        // Changefeed/CDC
        {"/changefeed/*", "*", "cdc:read", "cdc", true, 100, 20},
        
        // Time-series
        {"/timeseries/*", "GET", "timeseries:read", "timeseries", true, 500, 50},
        {"/timeseries/*", "POST", "timeseries:write", "timeseries", true, 500, 50},
        {"/timeseries/*", "PUT", "timeseries:write", "timeseries", true, 500, 50},
        
        // Spatial/GIS
        {"/spatial/*", "*", "data:read", "spatial", true, 500, 50},
        
        // Cache operations
        {"/cache/*", "*", "cache:read", "cache", true, 1000, 100},
        
        // Prompt management
        {"/prompt/*", "GET", "llm:read", "prompt", true, 200, 30},
        {"/prompt/*", "POST", "llm:write", "prompt", true, 200, 30},
        {"/prompt/*", "PUT", "llm:write", "prompt", true, 200, 30},
        {"/prompt/*", "DELETE", "llm:write", "prompt", true, 200, 30},
        
        // Admin endpoints - require admin scope
        {"/admin/*", "*", "admin", "admin", true, 50, 10},
        {"/config", "GET", "config:read", "config.read", true, 10, 5},
        {"/config", "POST", "config:write", "config.write", true, 10, 5},
        {"/config", "PUT", "config:write", "config.write", true, 10, 5},
        
        // Audit endpoints - very restrictive
        {"/api/audit/*", "*", "audit:read", "audit.read", true, 50, 10},
        
        // Session management endpoints
        {"/auth/sessions", "POST",   "auth:sessions", "session.create",         true, 100, 20},
        {"/auth/sessions", "GET",    "auth:sessions", "session.list",           true, 100, 20},
        {"/auth/sessions", "DELETE", "auth:sessions", "session.revoke_others",  true,  50, 10},
        {"/auth/sessions/*", "DELETE","auth:sessions", "session.revoke",        true,  50, 10},
        
        // PKI endpoints - more specific patterns first
        {"/api/pki/hsm/*", "*", "pki:sign", "pki.hsm", true, 100, 20},
        {"/api/pki/timestamp/*", "*", "pki:timestamp", "pki.timestamp", true, 100, 20},
        {"/api/pki/eidas/*", "*", "pki:eidas", "pki.eidas", true, 100, 20},
        {"/api/pki/*", "*", "pki:read", "pki", true, 100, 20},
        
        // PII endpoints
        {"/pii/*", "GET", "pii:read", "pii", true, 50, 10},
        {"/pii/*", "POST", "pii:write", "pii", true, 50, 10},
        {"/pii/*", "DELETE", "pii:write", "pii", true, 50, 10},
        
        // Monitoring/health endpoints - public but rate-limited
        {"/health", "*", "", "", false, 1000, 100},
        {"/version", "*", "", "", false, 1000, 100},
        {"/stats", "*", "", "", false, 100, 20},
        {"/capabilities", "*", "", "", false, 100, 20},
        {"/api/capabilities", "*", "", "", false, 100, 20},
        {"/metrics", "*", "", "", false, 100, 20}
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
