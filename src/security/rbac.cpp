/**
 * @file rbac.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/rbac.h"
#include <stdexcept>
#include "themis/runtime_license_gate.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace themis {
namespace security {

// ============================================================================
// Permission
// ============================================================================

bool Permission::matches(const std::string& res, const std::string& act) const {
    bool res_match = (resource == "*" || resource == res);
    bool act_match = (action == "*" || action == act);
    return res_match && act_match;
}

// ============================================================================
// Role
// ============================================================================

nlohmann::json Role::toJson() const {
    nlohmann::json perms_arr = nlohmann::json::array();
    for (const auto& p : permissions) {
        perms_arr.push_back({
            {"resource", p.resource},
            {"action", p.action}
        });
    }
    
    return {
        {"name", name},
        {"description", description},
        {"permissions", perms_arr},
        {"inherits", inherits}
    };
}

Role Role::fromJson(const nlohmann::json& j) {
    Role r;
    r.name = j.value("name", "");
    r.description = j.value("description", "");
    
    if (j.contains("permissions") && j["permissions"].is_array()) {
        for (const auto& p : j["permissions"]) {
            Permission perm;
            perm.resource = p.value("resource", "");
            perm.action = p.value("action", "");
            r.permissions.push_back(perm);
        }
    }
    
    if (j.contains("inherits") && j["inherits"].is_array()) {
        for (const auto& inherit : j["inherits"]) {
            r.inherits.push_back(inherit.get<std::string>());
        }
    }
    
    return r;
}

// ============================================================================
// RBAC - Built-in Roles
// ============================================================================

std::vector<Role> RBAC::getBuiltinRoles() {
    std::vector<Role> builtin;
    
    // Admin: Full access to everything
    builtin.push_back({
        "admin",
        "Administrator with full system access",
        {
            {"*", "*"}  // Wildcard: all resources, all actions
        },
        {}
    });
    
    // Operator: Data operations + key rotation
    builtin.push_back({
        "operator",
        "Operator with data and key management permissions",
        {
            {"data", "read"},
            {"data", "write"},
            {"data", "delete"},
            {"keys", "read"},
            {"keys", "rotate"},
            {"audit", "read"}
        },
        {"analyst"}  // Inherits analyst permissions
    });
    
    // Analyst: Read-only access to data and audit logs
    builtin.push_back({
        "analyst",
        "Analyst with read-only data access",
        {
            {"data", "read"},
            {"audit", "read"},
            {"metrics", "read"}
        },
        {"readonly"}
    });
    
    // Readonly: Minimal read access
    builtin.push_back({
        "readonly",
        "Read-only user with limited access",
        {
            {"metrics", "read"},
            {"health", "read"}
        },
        {}
    });
    
    return builtin;
}

// ============================================================================
// RBAC - Constructor and Configuration
// ============================================================================

RBAC::RBAC(const RBACConfig& config) : config_(config) {
    // Load built-in roles first
    if (config_.use_builtin_roles) {
        for (const auto& role : getBuiltinRoles()) {
            roles_[role.name] = role;
        }
        THEMIS_INFO("Loaded {} built-in roles",static_cast<int>(roles_.size()));
    }
    
    // Load custom roles from config file
    if (!config_.config_path.empty()) {
        // RB-3: loadConfig() → loadFromJson() acquires mutex_ internally.
        // This is intentional: the constructor is the sole owner of *this at
        // this point (the object has not yet been shared with other threads),
        // so the lock is logically redundant but harmless — it keeps the
        // locking discipline consistent with all other RBAC mutating paths.
        if (loadConfig(config_.config_path)) {
            THEMIS_INFO("Loaded RBAC configuration from {}", config_.config_path);
        } else {
            THEMIS_WARN("Failed to load RBAC config from {}, using built-in roles only", 
                config_.config_path);
        }
    }
    
    // Validate role hierarchy
    if (!validateRoleHierarchy()) {
        THEMIS_ERROR("[SECURITY] RBAC: cyclic role hierarchy detected. "
                     "RBAC system marked invalid — all permission checks will be DENIED.");
        hierarchy_valid_ = false;
    }
}

bool RBAC::loadConfig(const std::string& path) {
    try {
        std::ifstream ifs(path);
        if (!ifs) {
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        
        // Try JSON first
        if (path.ends_with(".json")) {
            nlohmann::json j = nlohmann::json::parse(content);
            return loadFromJson(j);
        } else if (path.ends_with(".yaml") || path.ends_with(".yml")) {
            return loadFromYaml(content);
        }
        
        // Auto-detect
        try {
            nlohmann::json j = nlohmann::json::parse(content);
            return loadFromJson(j);
        } catch (...) {
            return loadFromYaml(content);
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load RBAC config: {}", e.what());
        return false;
    }
}

bool RBAC::loadFromJson(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!j.contains("roles") || !j["roles"].is_array()) {
        return false;
    }
    
    // Clear existing custom roles (keep built-in)
    std::unordered_map<std::string, Role> builtin_backup = {};

    if (config_.use_builtin_roles) {
        for (const auto& br : getBuiltinRoles()) {
            builtin_backup[br.name] = br;
        }
    }
    
    roles_.clear();
    roles_ = builtin_backup;
    
    // Load roles from JSON
    for (const auto& role_json : j["roles"]) {
        Role r = Role::fromJson(role_json);
        roles_[r.name] = r;
    }
    
    return true;
}

bool RBAC::loadFromYaml([[maybe_unused]] const std::string& content) {
    // unused parameter
    // Simple YAML-to-JSON conversion (limited parser)
    // For production, use a real YAML library (yaml-cpp)
    THEMIS_WARN("YAML support not fully implemented, falling back to JSON");
    return false;
}

bool RBAC::saveConfig(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        nlohmann::json j;
        j["roles"] = nlohmann::json::array();
        
        for (const auto& [name, role] : roles_) {
            j["roles"].push_back(role.toJson());
        }
        
        std::ofstream ofs(path);
        ofs << j.dump(2);
        
        THEMIS_INFO("Saved RBAC configuration to {}", path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save RBAC config: {}", e.what());
        return false;
    }
}

// ============================================================================
// RBAC - Role Management
// ============================================================================

void RBAC::addRole(const Role& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    roles_[role.name] = role;
    THEMIS_INFO("Added role '{}' with {} permissions", role.name,static_cast<int>(role.permissions.size()));
}

void RBAC::removeRole(const std::string& role_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    roles_.erase(role_name);
    THEMIS_INFO("Removed role '{}'", role_name);
}

std::optional<Role> RBAC::getRole(const std::string& role_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = roles_.find(role_name);
    if (it != roles_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> RBAC::listRoles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names = {};

    names.reserve(roles_.size());
    for (const auto& [name, _] : roles_) {
        names.push_back(name);
    }
    return names;
}

// ============================================================================
// RBAC - Permission Checking
// ============================================================================

bool RBAC::checkPermission(
    const std::vector<std::string>& user_roles,
    const std::string& resource,
    const std::string& action
) const {
    // Fail-closed: if the role hierarchy is invalid (cycle detected), deny all access.
    if (!hierarchy_valid_) {
        THEMIS_WARN("[SECURITY] RBAC: checkPermission called but hierarchy is invalid "
                    "(cyclic dependency detected). Denying access to resource='{}' action='{}'.",
                    resource, action);
        return false;
    }

    // Runtime license gate: RBAC is an Enterprise/Hyperscaler feature.
    // [static_cast<int>(RB - 1)] Grace period: a transient license server outage must not immediately
    // lock out all users. If the last successful check is within the grace window,
    // allow access and log a warning. Update the timestamp on every success.
    std::string license_error = {};
    if (!license::RuntimeLicenseGate::instance().isFeatureAllowed("rbac", license_error)) {
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const int64_t last_success = last_license_success_ms_.load(std::memory_order_relaxed);
        if (last_success > 0 && (now_ms - last_success) < LICENSE_GRACE_PERIOD_MS) {
            THEMIS_WARN("RBAC::checkPermission: license check failed ({}) — "
                        "operating within grace period ({} s remaining)",
                        license_error,
                        (LICENSE_GRACE_PERIOD_MS - (now_ms - last_success)) / 1000);
            // Fall through to normal permission evaluation
        } else {
            THEMIS_WARN("RBAC::checkPermission blocked – {}", license_error);
            return false;
        }
    } else {
        // Record successful license check timestamp
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        last_license_success_ms_.store(now_ms, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get all effective permissions (with inheritance)
    std::vector<Permission> all_perms;
    std::unordered_set<std::string> visited;
    
    for (const auto& role_name : user_roles) {
        auto role_perms = expandRolePermissions(role_name, visited);
        all_perms.insert(all_perms.end(), role_perms.begin(), role_perms.end());
    }
    
    // Check if any permission matches
    for (const auto& perm : all_perms) {
        if (perm.matches(resource, action)) {
            THEMIS_DEBUG("Permission granted: resource='{}' action='{}' via permission '{}'",
                resource, action, perm.toString());
            return true;
        }
    }
    
    THEMIS_DEBUG("Permission denied: resource='{}' action='{}'", resource, action);
    return false;
}

std::vector<Permission> RBAC::getUserPermissions(const std::vector<std::string>& user_roles) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Permission> all_perms;
    std::unordered_set<std::string> visited;
    
    for (const auto& role_name : user_roles) {
        auto role_perms = expandRolePermissions(role_name, visited);
        all_perms.insert(all_perms.end(), role_perms.begin(), role_perms.end());
    }
    
    // Remove duplicates
    std::sort(all_perms.begin(), all_perms.end(), [](const auto& a, const auto& b) {
        return a.toString() < b.toString();
    });
    all_perms.erase(std::unique(all_perms.begin(), all_perms.end()), all_perms.end());
    
    return all_perms;
}

std::vector<Permission> RBAC::expandRolePermissions(
    const std::string& role_name,
    std::unordered_set<std::string>& visited
) const {
    // Prevent cycles
    if (visited.count(role_name)) {
        THEMIS_WARN("Cyclic role dependency detected: {}", role_name);
        return {};
    }
    visited.insert(role_name);
    
    auto it = roles_.find(role_name);
    if (it == roles_.end()) {
        THEMIS_WARN("Role not found: {}", role_name);
        return {};
    }
    
    const Role& role = it->second;
    std::vector<Permission> perms = role.permissions;
    
    // Add inherited permissions
    if (config_.enable_role_inheritance) {
        for (const auto& inherited_role : role.inherits) {
            auto inherited_perms = expandRolePermissions(inherited_role, visited);
            perms.insert(perms.end(), inherited_perms.begin(), inherited_perms.end());
        }
    }
    
    return perms;
}

// ============================================================================
// RBAC - Validation
// ============================================================================

bool RBAC::validateRoleHierarchy() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;

    auto has_no_cycle = [&](const auto& self, const std::string& role_name) -> bool {
        if (visiting.count(role_name)) {
            THEMIS_ERROR("Cyclic dependency in role hierarchy: {}", role_name);
            return false;
        }

        if (visited.count(role_name)) {
            return true;
        }

        visiting.insert(role_name);

        auto role_it = roles_.find(role_name);
        if (role_it != roles_.end()) {
            const Role& role = role_it->second;
            for (const auto& inherited_role : role.inherits) {
                if (roles_.count(inherited_role)) {
                    if (!self(self, inherited_role)) {
                        return false;
                    }
                }
            }
        }

        visiting.erase(role_name);
        visited.insert(role_name);
        return true;
    };

    for (const auto& [role_name, _] : roles_) {
        if (!visited.count(role_name)) {
            if (!has_no_cycle(has_no_cycle, role_name)) {
                return false;
            }
        }
    }
    
    return true;
}

nlohmann::json RBAC::getRoleHierarchy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json hierarchy = nlohmann::json::object();
    
    for (const auto& [role_name, role] : roles_) {
        hierarchy[role_name] = {
            {"description", role.description},
            {"inherits", role.inherits},
            {"direct_permissions", nlohmann::json::array()}
        };
        
        for (const auto& perm : role.permissions) {
            hierarchy[role_name]["direct_permissions"].push_back(perm.toString());
        }
    }
    
    return hierarchy;
}

// ============================================================================
// User
// ============================================================================

nlohmann::json User::toJson() const {
    return {
        {"user_id", user_id},
        {"roles", roles},
        {"attributes", attributes}
    };
}

User User::fromJson(const nlohmann::json& j) {
    User u;
    u.user_id = j.value("user_id", "");
    
    if (j.contains("roles") && j["roles"].is_array()) {
        for (const auto& r : j["roles"]) {
            u.roles.push_back(r.get<std::string>());
        }
    }
    
    if (j.contains("attributes") && j["attributes"].is_object()) {
        for (auto it = j["attributes"].begin(); it != j["attributes"].end(); ++it) {
            u.attributes[it.key()] = it.value().get<std::string>();
        }
    }
    
    return u;
}

// ============================================================================
// UserRoleStore
// ============================================================================

void UserRoleStore::assignRole(const std::string& user_id, const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& user = users_[user_id];
    user.user_id = user_id;
    
    if (std::find(user.roles.begin(), user.roles.end(), role) == user.roles.end()) {
        user.roles.push_back(role);
        THEMIS_INFO("Assigned role '{}' to user '{}'", role, user_id);
    }
}

void UserRoleStore::revokeRole(const std::string& user_id, const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        auto& roles = it->second.roles;
        roles.erase(std::remove(roles.begin(), roles.end(), role), roles.end());
        THEMIS_INFO("Revoked role '{}' from user '{}'", role, user_id);
    }
}

std::vector<std::string> UserRoleStore::getUserRoles(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        return it->second.roles;
    }
    return {};
}

std::vector<std::string> UserRoleStore::getRoleUsers(const std::string& role) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> users = {};

    for (const auto& [user_id, user] : users_) {
        if (std::find(user.roles.begin(), user.roles.end(), role) != user.roles.end()) {
            users.push_back(user_id);
        }
    }
    return users;
}

bool UserRoleStore::load(const std::string& path) {
    try {
        std::ifstream ifs(path);
        if (!ifs) {
            return false;
        }
        
        nlohmann::json j;
        ifs >> j;
        
        std::lock_guard<std::mutex> lock(mutex_);
        users_.clear();
        
        if (j.contains("users") && j["users"].is_array()) {
            for (const auto& user_json : j["users"]) {
                User u = User::fromJson(user_json);
                users_[u.user_id] = u;
            }
        }
        
        THEMIS_INFO("Loaded {} users from {}",static_cast<int>(users_.size()), path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load user-role mappings: {}", e.what());
        return false;
    }
}

bool UserRoleStore::save(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        nlohmann::json j;
        j["users"] = nlohmann::json::array();
        
        for (const auto& [_, user] : users_) {
            j["users"].push_back(user.toJson());
        }
        
        std::ofstream ofs(path);
        ofs << j.dump(2);
        
        THEMIS_INFO("Saved {} users to {}",static_cast<int>(users_.size()), path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save user-role mappings: {}", e.what());
        return false;
    }
}

std::optional<User> UserRoleStore::getUser(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void UserRoleStore::setUser(const User& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    users_[user.user_id] = user;
    THEMIS_INFO("Set user '{}' with {} roles", user.user_id,static_cast<int>(user.roles.size()));
}

} // namespace security
} // namespace themis

