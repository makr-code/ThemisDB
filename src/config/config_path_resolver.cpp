#include "config/config_path_resolver.h"
#include "config/config_errors.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace config {

// ═══════════════════════════════════════════════════════════
// Static Members Initialization
// ═══════════════════════════════════════════════════════════

ConfigPathResolver::Metrics ConfigPathResolver::metrics_;
LRUCacheWithTTL<std::string, std::string> ConfigPathResolver::cache_(1000, 300); // 1000 entries, 5 min TTL
std::atomic<bool> ConfigPathResolver::caching_enabled_{true};

// ═══════════════════════════════════════════════════════════
// Path Mapping Table: Legacy → New
// ═══════════════════════════════════════════════════════════

const std::map<std::string, std::string> ConfigPathResolver::PATH_MAPPING = {
    // AI/ML Configurations
    {"config/lora_training_config.yaml", "config/ai_ml/lora_training_config.yaml"},
    {"config/vision_config.yaml", "config/ai_ml/vision/config.yaml"},
    {"config/llm_system_prompts.yaml", "config/ai_ml/llm/system_prompts.yaml"},
    {"config/rag_judge.yaml", "config/ai_ml/rag_judge.yaml"},
    {"config/voice_assistant.yaml", "config/ai_ml/voice_assistant.yaml"},
    
    // Security Configurations
    {"config/pii_patterns.yaml", "config/security/pii_patterns.yaml"},
    {"config/rbac_roles.json", "config/security/rbac_roles.json"},
    {"config/user_roles.json", "config/security/user_roles.json"},
    {"config/graph_protection.yaml", "config/security/graph_protection.yaml"},
    {"config/auth_kerberos.example.yaml", "config/security/auth_kerberos.example.yaml"},
    
    // Compliance & Ethics
    {"config/ethical_guidelines.yaml", "config/compliance/ethical_guidelines.yaml"},
    {"config/governance.yaml", "config/compliance/governance.yaml"},
    {"config/audit.yaml", "config/compliance/audit/audit.yaml"},
    {"config/ai_audit_config.yaml", "config/compliance/audit/ai_audit_config.yaml"},
    
    // Data Management
    {"config/mime_types.yaml", "config/data_management/mime_types.yaml"},
    {"config/storage_redundancy.yaml", "config/data_management/storage_redundancy.yaml"},
    {"config/retention_policies.yaml", "config/data_management/retention_policies.yaml"},
    
    // Performance Configurations
    {"config/scaling_optimizations.yaml", "config/performance/scaling_optimizations.yaml"},
    {"config/acceleration.yaml", "config/performance/acceleration.yaml"},
    {"config/config_2ssd_performance.yaml", "config/performance/config_2ssd_performance.yaml"},
    {"config/config_multi_ssd.yaml", "config/performance/config_multi_ssd.yaml"},
    {"config/query_cache_mixed.yaml", "config/performance/query_cache/mixed.yaml"},
    {"config/query_cache_olap.yaml", "config/performance/query_cache/olap.yaml"},
    {"config/query_cache_oltp.yaml", "config/performance/query_cache/oltp.yaml"},
    
    // Deprecated/Backup Files
    {"config/phase2_optimizations.json", "config/deprecated/phase2_optimizations.json"},
    {"config/phase3_optimizations.json", "config/deprecated/phase3_optimizations.json"},
    {"config/policies.json.backup", "config/deprecated/policies.json.backup"},
    
    // Core Configurations
    {"config/config.yaml", "config/core/config.yaml"},
    {"config/config-minimal.yaml", "config/core/config-minimal.yaml"},
    {"config/security.yaml", "config/core/security.yaml"},
    {"config/updates.yaml", "config/core/updates.yaml"},
    
    // Platform Configurations
    {"config/config.rpi3.json", "config/platform/rpi3.json"},
    {"config/config.rpi4.json", "config/platform/rpi4.json"},
    {"config/config.rpi5.json", "config/platform/rpi5.json"},
    {"config/config.qnap.json", "config/platform/qnap.json"},
    
    // Networking
    {"config/connection_pool_config.yaml", "config/networking/connection_pool_config.yaml"},
    
    // Content Processing
    {"config/content_processors.yaml", "config/content/processors.yaml"},
    {"config/fem_edge_type_defaults.yaml", "config/content/fem_edge_type_defaults.yaml"},
    
    // Monitoring
    {"config/prometheus_arm.yml", "config/monitoring/prometheus/arm.yml"},
    {"config/prometheus_ethics.yml", "config/monitoring/prometheus/ethics.yml"},
    
    // Features
    {"config/features.yaml.example", "config/features/features.example.yaml"},
    {"config/capability_auto_generation.yaml", "config/features/capability_auto_generation.yaml"},
    
    // Assistants
    {"config/docs_assistant.yaml", "config/assistants/docs_assistant.yaml"},
    {"config/feedback_config.yaml", "config/assistants/feedback_config.yaml"},
    
    // Processing
    {"config/cep_rules.yaml", "config/processing/cep_rules.yaml"},
    
    // Licensing
    {"config/community_license.default.json", "config/licensing/community/default.json"},
    {"config/community_license.example.json", "config/licensing/community/example.json"},
    {"config/enterprise_license.example.json", "config/licensing/enterprise/example.json"},
    {"config/enterprise_license.test.json", "config/licensing/enterprise/test.json"},
};

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

std::string ConfigPathResolver::resolve(const std::string& legacy_path) {
    auto result = tryResolve(legacy_path);
    if (result) {
        metrics_.resolution_hits++;
        return *result;
    }
    
    metrics_.resolution_misses++;
    
    // Build list of attempted paths for error message
    std::vector<std::string> attempted_paths;
    std::string normalized = normalizePath(legacy_path);
    std::string new_path = mapLegacyToNew(normalized);
    
    if (!new_path.empty() && new_path != normalized) {
        attempted_paths.push_back(new_path);
    }
    attempted_paths.push_back(normalized);
    
    throw ConfigNotFoundException(legacy_path, attempted_paths);
}

std::optional<std::string> ConfigPathResolver::tryResolve(const std::string& legacy_path) {
    std::string normalized = normalizePath(legacy_path);
    
    // Check cache first if enabled
    if (caching_enabled_.load()) {
        auto cached = cache_.get(normalized);
        if (cached) {
            metrics_.cache_hits++;
            return *cached;
        }
        metrics_.cache_misses++;
    }
    
    // Validate path to prevent security issues
    try {
        validatePath(normalized);
    } catch (const InvalidPathException&) {
        return std::nullopt;
    }
    
    // Try new path first
    std::string new_path = mapLegacyToNew(normalized);
    std::string resolved_path;
    
    if (!new_path.empty() && std::filesystem::exists(new_path)) {
        if (normalized != new_path) {
            spdlog::debug("ConfigPathResolver: Using new config path: {} -> {}", 
                         normalized, new_path);
            metrics_.new_path_hits++;
        }
        resolved_path = new_path;
    }
    // Fall back to legacy path with warning
    else if (std::filesystem::exists(normalized)) {
        if (!new_path.empty() && new_path != normalized) {
            spdlog::warn("ConfigPathResolver: Using legacy config path: {}. "
                        "Please migrate to: {}", normalized, new_path);
            metrics_.legacy_fallbacks++;
        }
        resolved_path = normalized;
    }
    else {
        // Track unmapped requests
        if (new_path.empty() || new_path == normalized) {
            metrics_.unmapped_requests++;
        }
        // Neither path exists
        return std::nullopt;
    }
    
    // Cache the resolved path if caching is enabled
    if (caching_enabled_.load()) {
        cache_.put(normalized, resolved_path);
    }
    
    return resolved_path;
}

std::string ConfigPathResolver::mapLegacyToNew(const std::string& legacy_path) {
    std::string normalized = normalizePath(legacy_path);
    
    auto it = PATH_MAPPING.find(normalized);
    if (it != PATH_MAPPING.end()) {
        return it->second;
    }
    
    // No mapping found - return the path as-is
    return normalized;
}

bool ConfigPathResolver::isLegacyPath(const std::string& path) {
    std::string normalized = normalizePath(path);
    return PATH_MAPPING.find(normalized) != PATH_MAPPING.end();
}

// ═══════════════════════════════════════════════════════════
// Private Helper Methods
// ═══════════════════════════════════════════════════════════

std::string ConfigPathResolver::normalizePath(const std::string& path) {
    std::string normalized = path;
    
    // Replace backslashes with forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Remove leading "./"
    if (normalized.starts_with("./")) {
        normalized = normalized.substr(2);
    }
    
    // Remove trailing slashes
    while (normalized.ends_with("/") && normalized.length() > 1) {
        normalized.pop_back();
    }
    
    return normalized;
}

void ConfigPathResolver::validatePath(const std::string& path) {
    // Check for path traversal attempts
    if (path.find("..") != std::string::npos) {
        throw InvalidPathException(path, "path traversal not allowed");
    }
    
    // Check for absolute paths outside config directory
    std::filesystem::path fs_path(path);
    if (fs_path.is_absolute() && !path.starts_with("/config") && 
        !path.starts_with("config") && path.find(":\\config") == std::string::npos) {
        // Allow absolute paths that point to config directory
        // This is a basic check; more sophisticated validation may be needed
    }
}

void ConfigPathResolver::resetMetrics() {
    metrics_.resolution_hits = 0;
    metrics_.resolution_misses = 0;
    metrics_.legacy_fallbacks = 0;
    metrics_.new_path_hits = 0;
    metrics_.unmapped_requests = 0;
    metrics_.cache_hits = 0;
    metrics_.cache_misses = 0;
}

void ConfigPathResolver::setCachingEnabled(bool enabled) {
    caching_enabled_.store(enabled);
    if (!enabled) {
        cache_.clear();
    }
}

} // namespace config
} // namespace themis
