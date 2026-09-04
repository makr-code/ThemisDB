/**
 * @file access_control_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/access_control_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/audit_logger.h"
#include <algorithm>
#include <chrono>
#include <fstream>
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
        
        // Load ABAC policies if configured
        if (config_.enable_abac && !config_.abac_policy_path.empty()) {
            std::string err = {};
            if (!policy_engine_.loadFromFile(config_.abac_policy_path, &err)) {
                THEMIS_WARN("Failed to load ABAC policies from {}: {}", 
                    config_.abac_policy_path, err);
            } else {
                THEMIS_INFO("Loaded ABAC policies from {}", config_.abac_policy_path);
            }
        }
        
        // Load RLS policies if configured
        if (config_.enable_rls && !config_.rls_policy_path.empty()) {
            try {
                std::ifstream rls_file(config_.rls_policy_path);
                if (!rls_file.is_open()) {
                    THEMIS_WARN("Failed to open RLS policy file: {}", config_.rls_policy_path);
                } else {
                    nlohmann::json rls_json = nlohmann::json::parse(rls_file);
                    size_t loaded = rls_manager_.loadFromJson(rls_json);
                    THEMIS_INFO("Loaded {} RLS policies from {}", loaded, config_.rls_policy_path);
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to load RLS policies from {}: {}", config_.rls_policy_path, e.what());
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
            if (config_.enable_audit_logging) {
                nlohmann::json audit_entry = {
                    {"event_type", "authentication"},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                    {"user_id", "unknown"},
                    {"source_ip", source_ip},
                    {"outcome", "failure"},
                    {"reason", "AuthMiddleware not configured"}
                };
                THEMIS_INFO("AUDIT [AUTHENTICATION]: {}", audit_entry.dump());
            }
            return std::nullopt;
        }
        
        auto auth_result = auth_middleware_->validateToken(token);
        if (!auth_result.authorized) {
            THEMIS_DEBUG("Authentication failed: {}", auth_result.reason);
            metrics_.authentication_failure++;
            if (config_.enable_audit_logging) {
                nlohmann::json audit_entry = {
                    {"event_type", "authentication"},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                    {"user_id", "unknown"},
                    {"source_ip", source_ip},
                    {"outcome", "failure"},
                    {"reason", auth_result.reason}
                };
                THEMIS_INFO("AUDIT [AUTHENTICATION]: {}", audit_entry.dump());
            }
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
            context.user_id,static_cast<int>(context.roles.size()));
        if (config_.enable_audit_logging) {
            nlohmann::json audit_entry = {
                {"event_type", "authentication"},
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                {"user_id", context.user_id},
                {"source_ip", source_ip},
                {"roles", context.roles},
                {"outcome", "success"}
            };
            THEMIS_INFO("AUDIT [AUTHENTICATION]: {}", audit_entry.dump());
        }
        
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
        
        AccessDecision decision = {};
        if (has_permission) {
            // Step 3: If RBAC grants access and ABAC is enabled, evaluate ABAC policies
            if (config_.enable_abac) {
                std::optional<std::string> client_ip = context.source_ip.empty()
                    ? std::nullopt
                    : std::make_optional(context.source_ip);
                std::optional<std::string> user_agent = context.user_agent;
                
                auto abac_decision = policy_engine_.authorize(
                    context.user_id, action, resource, client_ip, user_agent);
                
                if (!abac_decision.allowed) {
                    decision = AccessDecision::Deny(
                        "ABAC policy denied access: " + abac_decision.reason +
                        (abac_decision.policy_id.empty() ? "" : " [policy: " + abac_decision.policy_id + "]")
                    );
                    metrics_.access_denied++;
                    THEMIS_INFO("ABAC denied access for user '{}' on '{}:{}' - {}", 
                        context.user_id, resource, action, abac_decision.reason);
                    auditAccessDecision(context, resource, action, decision);
                    return decision;
                }
            }
            
            decision = AccessDecision::Allow("Permission granted via RBAC");
            
            // Get which permissions were applied
            auto user_perms = rbac_->getUserPermissions(context.roles);
            decision.applied_permissions.reserve(user_perms.size());
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

        const bool requested_fail_open =
            (config_.failure_mode == AuthorizationFailureMode::AllowOnErrorExplicit) ||
            !config_.fail_closed;
        const bool explicit_fail_open = requested_fail_open && !config_.fail_open_reason.empty();

        AccessDecision decision;
        if (explicit_fail_open) {
            THEMIS_WARN("[SECURITY] Authorization error bypassed under explicit fail-open override: {}",
                        config_.fail_open_reason);
            decision = AccessDecision::Allow(
                "Authorization bypassed due to explicit fail-open policy: " + config_.fail_open_reason
            );
        } else if (requested_fail_open) {
            decision = AccessDecision::Deny(
                "Authorization error: " + std::string(e.what()) +
                " (fail-open override requires a documented reason)"
            );
        } else {
            decision = AccessDecision::Deny("Authorization error: " + std::string(e.what()));
        }
        auditAccessDecision(context, resource, action, decision);
        return decision;
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

    // Step 2: Zero-trust per-request identity verification (optional)
    if (config_.enable_zero_trust && zero_trust_enforcer_) {
        ZeroTrustContext zt_ctx;
        zt_ctx.user_id   = context->user_id;
        zt_ctx.client_ip = source_ip;
        zt_ctx.token     = token;
        zt_ctx.resource  = resource;
        zt_ctx.action    = action;
        zt_ctx.timestamp = std::chrono::system_clock::now();

        auto zt_result = zero_trust_enforcer_->verify(zt_ctx);
        if (!zt_result.verified) {
            metrics_.access_denied++;
            THEMIS_WARN("Zero-trust denied user='{}' resource='{}' action='{}' reason='{}'",
                        context->user_id, resource, action, zt_result.reason);
            auto decision = AccessDecision::Deny("Zero-trust verification failed: " + zt_result.reason);
            auditAccessDecision(*context, resource, action, decision);
            return decision;
        }
        THEMIS_DEBUG("Zero-trust passed for user='{}' trust_score={:.2f}",
                     context->user_id, zt_result.trust_score);
    }

    // Step 3: Authorize
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

void AccessControlManager::setZeroTrustEnforcer(ZeroTrustPolicyEnforcer* enforcer) {
    zero_trust_enforcer_ = enforcer;
    if (enforcer) {
        THEMIS_INFO("ZeroTrustPolicyEnforcer configured for AccessControlManager");
    } else {
        THEMIS_INFO("ZeroTrustPolicyEnforcer removed from AccessControlManager");
    }
}

void AccessControlManager::addABACPolicy(const PolicyEngine::Policy& policy) {
    policy_engine_.addPolicy(policy);
    THEMIS_INFO("Added ABAC policy '{}' to AccessControlManager", policy.id);
}

bool AccessControlManager::removeABACPolicy(const std::string& policy_id) {
    bool removed = policy_engine_.removePolicy(policy_id);
    if (removed) {
        THEMIS_INFO("Removed ABAC policy '{}' from AccessControlManager", policy_id);
    }
    return removed;
}

// ── Row-level security (RLS) ─────────────────────────────────────────────────

void AccessControlManager::addRLSPolicy(const RLSPolicy& policy) {
    rls_manager_.addPolicy(policy);
    THEMIS_INFO("Added RLS policy '{}' for collection '{}' to AccessControlManager",
                policy.id, policy.collection.empty() ? "*" : policy.collection);
}

bool AccessControlManager::removeRLSPolicy(const std::string& policy_id) {
    bool removed = rls_manager_.removePolicy(policy_id);
    if (removed) {
        THEMIS_INFO("Removed RLS policy '{}' from AccessControlManager", policy_id);
    }
    return removed;
}

nlohmann::json AccessControlManager::filterQueryResults(
    const std::string& collection,
    const SecurityContext& ctx,
    const nlohmann::json& rows
) const {
    return rls_manager_.filterRows(collection, ctx, rows);
}

bool AccessControlManager::isRLSActive(
    const std::string& collection,
    const SecurityContext& ctx
) const {
    return rls_manager_.isActive(collection, ctx);
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
        
        // Reload ABAC policies if configured
        if (config_.enable_abac && !config_.abac_policy_path.empty()) {
            std::string err = {};
            if (!policy_engine_.loadFromFile(config_.abac_policy_path, &err)) {
                THEMIS_ERROR("Failed to reload ABAC policies from {}: {}",
                    config_.abac_policy_path, err);
                return false;
            }
            THEMIS_INFO("Reloaded ABAC policies from {}", config_.abac_policy_path);
        }
        
        // Reload RLS policies if configured
        if (config_.enable_rls && !config_.rls_policy_path.empty()) {
            try {
                std::ifstream rls_file(config_.rls_policy_path);
                if (!rls_file.is_open()) {
                    THEMIS_ERROR("Failed to open RLS policy file for reload: {}", config_.rls_policy_path);
                    return false;
                }
                nlohmann::json rls_json = nlohmann::json::parse(rls_file);
                rls_manager_.clearAllPolicies();
                size_t loaded = rls_manager_.loadFromJson(rls_json);
                THEMIS_INFO("Reloaded {} RLS policies from {}", loaded, config_.rls_policy_path);
            } catch (const std::exception& e) {
                THEMIS_ERROR("Failed to reload RLS policies from {}: {}", config_.rls_policy_path, e.what());
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
