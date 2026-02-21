/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            access_control_manager.cpp                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:42:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     328                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2ea529912  2026-01-22  Add security and access control framework (#802) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "security/access_control_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/audit_logger.h"
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace security {

// ============================================================================
// SecurityContext
// ============================================================================

bool SecurityContext::hasRole(const std::string& role) const {
    return std::find(roles.begin(), roles.end(), role) != roles.end();
}

bool SecurityContext::hasGroup(const std::string& group) const {
    return std::find(groups.begin(), groups.end(), group) != groups.end();
}

// ============================================================================
// AccessControlManager
// ============================================================================

AccessControlManager::AccessControlManager(const AccessControlConfig& config)
    : config_(config) {
    
    // Initialize RBAC
    RBACConfig rbac_config;
    rbac_config.config_path = config_.rbac_config_path;
    rbac_config.enable_resource_wildcards = config_.enable_resource_wildcards;
    rbac_config.use_builtin_roles = true;
    
    rbac_ = std::make_shared<RBAC>(rbac_config);
    
    // Initialize user-role store
    user_store_ = std::make_shared<UserRoleStore>();
    
    THEMIS_INFO("AccessControlManager initialized");
}

bool AccessControlManager::initialize() {
    try {
        // Load RBAC configuration
        if (!config_.rbac_config_path.empty()) {
            if (!rbac_->loadConfig(config_.rbac_config_path)) {
                THEMIS_WARN("Failed to load RBAC config from {}, using built-in roles", 
                    config_.rbac_config_path);
            }
        }
        
        // Load user-role mappings
        if (!config_.user_role_store_path.empty()) {
            if (!user_store_->load(config_.user_role_store_path)) {
                THEMIS_WARN("Failed to load user-role mappings from {}", 
                    config_.user_role_store_path);
            }
        }
        
        THEMIS_INFO("AccessControlManager initialization complete");
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("AccessControlManager initialization failed: {}", e.what());
        return false;
    }
}

std::optional<SecurityContext> AccessControlManager::authenticate(
    const std::string& token,
    const std::string& source_ip
) {
    try {
        // Step 1: Validate token via AuthMiddleware
        if (!auth_middleware_) {
            THEMIS_ERROR("AuthMiddleware not configured");
            metrics_.authentication_failure++;
            return std::nullopt;
        }
        
        auto auth_result = auth_middleware_->validateToken(token);
        if (!auth_result.authorized) {
            THEMIS_DEBUG("Authentication failed: {}", auth_result.reason);
            metrics_.authentication_failure++;
            return std::nullopt;
        }
        
        // Step 2: Build security context
        SecurityContext context;
        context.user_id = auth_result.user_id;
        context.groups = auth_result.groups;
        context.source_ip = source_ip;
        
        // Step 3: Load user roles from user-role store
        context.roles = user_store_->getUserRoles(context.user_id);
        
        // If user has no roles, check if we should use default roles
        if (context.roles.empty()) {
            THEMIS_DEBUG("User '{}' has no assigned roles", context.user_id);
        }
        
        metrics_.authentication_success++;
        THEMIS_DEBUG("Authentication successful for user '{}' with {} roles", 
            context.user_id, context.roles.size());
        
        return context;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Authentication error: {}", e.what());
        metrics_.authentication_failure++;
        return std::nullopt;
    }
}

AccessDecision AccessControlManager::authorize(
    const SecurityContext& context,
    const std::string& resource,
    const std::string& action
) {
    try {
        // Step 1: Check custom authorizer if configured
        if (config_.custom_authorizer) {
            auto decision = config_.custom_authorizer(context, resource, action);
            if (decision.granted) {
                auditAccessDecision(context, resource, action, decision);
                metrics_.authorization_success++;
                return decision;
            }
            // Continue to RBAC if custom authorizer denied
        }
        
        // Step 2: Check RBAC permissions
        bool has_permission = rbac_->checkPermission(context.roles, resource, action);
        
        AccessDecision decision;
        if (has_permission) {
            decision = AccessDecision::Allow("Permission granted via RBAC");
            
            // Get which permissions were applied
            auto user_perms = rbac_->getUserPermissions(context.roles);
            for (const auto& perm : user_perms) {
                if (perm.matches(resource, action)) {
                    decision.applied_permissions.push_back(perm.toString());
                }
            }
            
            metrics_.authorization_success++;
            THEMIS_DEBUG("Authorization granted for user '{}' on '{}:{}'", 
                context.user_id, resource, action);
        } else {
            decision = AccessDecision::Deny(
                "User does not have required permission for " + resource + ":" + action
            );
            metrics_.access_denied++;
            THEMIS_INFO("Authorization denied for user '{}' on '{}:{}'", 
                context.user_id, resource, action);
        }
        
        // Step 3: Audit the decision
        auditAccessDecision(context, resource, action, decision);
        
        return decision;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Authorization error: {}", e.what());
        metrics_.authorization_failure++;
        
        // Fail closed: deny access on errors
        if (config_.fail_closed) {
            return AccessDecision::Deny("Authorization error: " + std::string(e.what()));
        } else {
            return AccessDecision::Allow("Authorization bypassed due to error (fail-open mode)");
        }
    }
}

AccessDecision AccessControlManager::checkAccess(
    const std::string& token,
    const std::string& resource,
    const std::string& action,
    const std::string& source_ip
) {
    // Step 1: Authenticate
    auto context = authenticate(token, source_ip);
    if (!context) {
        return AccessDecision::Deny("Authentication failed");
    }
    
    // Step 2: Authorize
    return authorize(*context, resource, action);
}

void AccessControlManager::assignRole(const std::string& user_id, const std::string& role) {
    user_store_->assignRole(user_id, role);
    THEMIS_INFO("Assigned role '{}' to user '{}'", role, user_id);
}

void AccessControlManager::revokeRole(const std::string& user_id, const std::string& role) {
    user_store_->revokeRole(user_id, role);
    THEMIS_INFO("Revoked role '{}' from user '{}'", role, user_id);
}

std::vector<std::string> AccessControlManager::getUserRoles(const std::string& user_id) const {
    return user_store_->getUserRoles(user_id);
}

std::vector<Permission> AccessControlManager::getUserPermissions(const std::string& user_id) const {
    auto roles = getUserRoles(user_id);
    return rbac_->getUserPermissions(roles);
}

void AccessControlManager::setAuthMiddleware(std::shared_ptr<AuthMiddleware> auth_middleware) {
    auth_middleware_ = auth_middleware;
    THEMIS_INFO("AuthMiddleware configured for AccessControlManager");
}

bool AccessControlManager::reloadConfiguration() {
    try {
        if (!config_.rbac_config_path.empty()) {
            if (!rbac_->loadConfig(config_.rbac_config_path)) {
                THEMIS_ERROR("Failed to reload RBAC config");
                return false;
            }
        }
        
        if (!config_.user_role_store_path.empty()) {
            if (!user_store_->load(config_.user_role_store_path)) {
                THEMIS_ERROR("Failed to reload user-role mappings");
                return false;
            }
        }
        
        THEMIS_INFO("Configuration reloaded successfully");
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Configuration reload failed: {}", e.what());
        return false;
    }
}

bool AccessControlManager::saveConfiguration() {
    try {
        if (!config_.rbac_config_path.empty()) {
            if (!rbac_->saveConfig(config_.rbac_config_path)) {
                THEMIS_ERROR("Failed to save RBAC config");
                return false;
            }
        }
        
        if (!config_.user_role_store_path.empty()) {
            if (!user_store_->save(config_.user_role_store_path)) {
                THEMIS_ERROR("Failed to save user-role mappings");
                return false;
            }
        }
        
        THEMIS_INFO("Configuration saved successfully");
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Configuration save failed: {}", e.what());
        return false;
    }
}

void AccessControlManager::auditAccessDecision(
    const SecurityContext& context,
    const std::string& resource,
    const std::string& action,
    const AccessDecision& decision
) {
    if (!config_.enable_audit_logging) {
        return;
    }
    
    try {
        // Create audit log entry
        nlohmann::json audit_entry = {
            {"event_type", "access_control"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"user_id", context.user_id},
            {"roles", context.roles},
            {"source_ip", context.source_ip},
            {"resource", resource},
            {"action", action},
            {"decision", decision.granted ? "allow" : "deny"},
            {"reason", decision.reason},
            {"applied_permissions", decision.applied_permissions}
        };
        
        // Log to audit system (using standard logging)
        THEMIS_INFO("AUDIT [ACCESS_CONTROL]: {}", audit_entry.dump());
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to audit access decision: {}", e.what());
    }
}

} // namespace security
} // namespace themis
