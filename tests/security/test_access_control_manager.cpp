/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_access_control_manager.cpp                    ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     307                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/access_control_manager.h"
#include "server/auth_middleware.h"
#include <fstream>
#include <filesystem>

using namespace themis;
using namespace themis::security;

class AccessControlManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary config files
        temp_dir_ = std::filesystem::temp_directory_path() / "themis_acl_test";
        std::filesystem::create_directories(temp_dir_);
        
        rbac_config_path_ = temp_dir_ / "rbac.json";
        user_roles_path_ = temp_dir_ / "users.json";
        
        // Create test RBAC config
        std::ofstream rbac_file(rbac_config_path_);
        rbac_file << R"({
            "roles": [
                {
                    "name": "admin",
                    "description": "Administrator",
                    "permissions": [
                        {"resource": "*", "action": "*"}
                    ],
                    "inherits": []
                },
                {
                    "name": "user",
                    "description": "Regular user",
                    "permissions": [
                        {"resource": "data", "action": "read"}
                    ],
                    "inherits": []
                }
            ]
        })";
        rbac_file.close();
        
        // Create test user-roles mapping
        std::ofstream users_file(user_roles_path_);
        users_file << R"({
            "users": [
                {
                    "user_id": "admin@test.com",
                    "roles": ["admin"],
                    "attributes": {}
                },
                {
                    "user_id": "user@test.com",
                    "roles": ["user"],
                    "attributes": {}
                }
            ]
        })";
        users_file.close();
    }
    
    void TearDown() override {
        // Clean up temp files
        std::filesystem::remove_all(temp_dir_);
    }
    
    std::filesystem::path temp_dir_;
    std::filesystem::path rbac_config_path_;
    std::filesystem::path user_roles_path_;
};

TEST_F(AccessControlManagerTest, InitializationSuccess) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    EXPECT_TRUE(acm.initialize());
}

TEST_F(AccessControlManagerTest, AuthorizationWithAdminRole) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Create security context for admin
    SecurityContext admin_context;
    admin_context.user_id = "admin@test.com";
    admin_context.roles = {"admin"};
    
    // Admin should have access to everything
    auto decision = acm.authorize(admin_context, "data", "write");
    EXPECT_TRUE(decision.granted);
    
    decision = acm.authorize(admin_context, "keys", "rotate");
    EXPECT_TRUE(decision.granted);
    
    decision = acm.authorize(admin_context, "config", "delete");
    EXPECT_TRUE(decision.granted);
}

TEST_F(AccessControlManagerTest, AuthorizationWithUserRole) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Create security context for regular user
    SecurityContext user_context;
    user_context.user_id = "user@test.com";
    user_context.roles = {"user"};
    
    // User should have read access to data
    auto decision = acm.authorize(user_context, "data", "read");
    EXPECT_TRUE(decision.granted);
    
    // User should NOT have write access to data
    decision = acm.authorize(user_context, "data", "write");
    EXPECT_FALSE(decision.granted);
    
    // User should NOT have access to keys
    decision = acm.authorize(user_context, "keys", "read");
    EXPECT_FALSE(decision.granted);
}

TEST_F(AccessControlManagerTest, RoleManagement) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Assign new role to user
    acm.assignRole("newuser@test.com", "user");
    
    auto roles = acm.getUserRoles("newuser@test.com");
    EXPECT_EQ(roles.size(), 1);
    EXPECT_EQ(roles[0], "user");
    
    // Revoke role
    acm.revokeRole("newuser@test.com", "user");
    roles = acm.getUserRoles("newuser@test.com");
    EXPECT_EQ(roles.size(), 0);
}

TEST_F(AccessControlManagerTest, GetUserPermissions) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Get permissions for admin
    auto perms = acm.getUserPermissions("admin@test.com");
    EXPECT_GT(perms.size(), 0);
    
    // Admin should have wildcard permission
    bool has_wildcard = false;
    for (const auto& perm : perms) {
        if (perm.resource == "*" && perm.action == "*") {
            has_wildcard = true;
            break;
        }
    }
    EXPECT_TRUE(has_wildcard);
}

TEST_F(AccessControlManagerTest, CustomAuthorizerHook) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    // Add custom authorizer that allows all access for specific user
    config.custom_authorizer = [](const SecurityContext& ctx, 
                                   const std::string& resource,
                                   const std::string& action) -> AccessDecision {
        if (ctx.user_id == "special@test.com") {
            return AccessDecision::Allow("Custom authorizer bypass");
        }
        // Return deny to fall through to RBAC
        return AccessDecision::Deny("Not authorized by custom logic");
    };
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Special user should have access even without roles
    SecurityContext special_context;
    special_context.user_id = "special@test.com";
    special_context.roles = {};
    
    auto decision = acm.authorize(special_context, "data", "write");
    EXPECT_TRUE(decision.granted);
    EXPECT_EQ(decision.reason, "Custom authorizer bypass");
}

TEST_F(AccessControlManagerTest, SecurityContextHelpers) {
    SecurityContext context;
    context.user_id = "test@example.com";
    context.roles = {"admin", "developer"};
    context.groups = {"engineering", "operations"};
    
    EXPECT_TRUE(context.hasRole("admin"));
    EXPECT_TRUE(context.hasRole("developer"));
    EXPECT_FALSE(context.hasRole("analyst"));
    
    EXPECT_TRUE(context.hasGroup("engineering"));
    EXPECT_TRUE(context.hasGroup("operations"));
    EXPECT_FALSE(context.hasGroup("marketing"));
}

TEST_F(AccessControlManagerTest, ConfigurationReload) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Add a new user
    acm.assignRole("test@example.com", "user");
    
    // Save configuration
    EXPECT_TRUE(acm.saveConfiguration());
    
    // Create new instance and reload
    AccessControlManager acm2(config);
    acm2.initialize();
    
    // Verify user was persisted
    auto roles = acm2.getUserRoles("test@example.com");
    EXPECT_EQ(roles.size(), 1);
    EXPECT_EQ(roles[0], "user");
}

TEST_F(AccessControlManagerTest, FailClosedMode) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.fail_closed = true; // Deny on errors
    
    AccessControlManager acm(config);
    acm.initialize();
    
    // Create context with invalid role
    SecurityContext context;
    context.user_id = "test@example.com";
    context.roles = {"nonexistent_role"};
    
    // Should be denied because role doesn't exist
    auto decision = acm.authorize(context, "data", "read");
    EXPECT_FALSE(decision.granted);
}

TEST_F(AccessControlManagerTest, MetricsTracking) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    
    AccessControlManager acm(config);
    acm.initialize();
    
    SecurityContext admin_context;
    admin_context.user_id = "admin@test.com";
    admin_context.roles = {"admin"};
    
    SecurityContext user_context;
    user_context.user_id = "user@test.com";
    user_context.roles = {"user"};
    
    // Perform some authorization checks
    acm.authorize(admin_context, "data", "write");  // Success
    acm.authorize(user_context, "data", "write");   // Denied
    acm.authorize(user_context, "keys", "read");    // Denied
    
    // Check metrics
    const auto& metrics = acm.getMetrics();
    EXPECT_EQ(metrics.authorization_success.load(), 1);
    EXPECT_EQ(metrics.access_denied.load(), 2);
}

// ============================================================================
// ABAC Integration Tests
// ============================================================================

TEST_F(AccessControlManagerTest, ABACDisabledByDefault) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    // enable_abac defaults to false

    AccessControlManager acm(config);
    acm.initialize();

    // Admin should be granted by RBAC alone (ABAC is disabled)
    SecurityContext ctx;
    ctx.user_id = "admin@test.com";
    ctx.roles = {"admin"};
    ctx.source_ip = "1.2.3.4";

    auto decision = acm.authorize(ctx, "data", "write");
    EXPECT_TRUE(decision.granted);
}

TEST_F(AccessControlManagerTest, ABACAllowsWhenNoPoliciesDefined) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.enable_abac = true; // ABAC enabled but no policies loaded

    AccessControlManager acm(config);
    acm.initialize();

    // When ABAC is enabled but no policies are defined, PolicyEngine defaults to allow
    SecurityContext ctx;
    ctx.user_id = "admin@test.com";
    ctx.roles = {"admin"};

    auto decision = acm.authorize(ctx, "data", "write");
    EXPECT_TRUE(decision.granted);
}

TEST_F(AccessControlManagerTest, ABACDeniesWhenIPNotInAllowedRange) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.enable_abac = true;

    AccessControlManager acm(config);
    acm.initialize();

    // Add an ABAC policy that allows only internal IPs
    PolicyEngine::Policy p;
    p.id = "internal-only";
    p.subjects = {"admin@test.com"};
    p.actions = {"write"};
    p.resources = {"data"};
    p.effect_allow = true;
    p.allowed_ip_prefixes = {"10.0."};
    acm.addABACPolicy(p);

    SecurityContext ctx;
    ctx.user_id = "admin@test.com";
    ctx.roles = {"admin"};
    ctx.source_ip = "192.168.1.1"; // Not in allowed IP range

    // RBAC allows (admin role), but ABAC denies (wrong IP)
    auto decision = acm.authorize(ctx, "data", "write");
    EXPECT_FALSE(decision.granted);
    EXPECT_NE(decision.reason.find("ABAC"), std::string::npos);
}

TEST_F(AccessControlManagerTest, ABACAllowsWhenConditionsMatch) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.enable_abac = true;

    AccessControlManager acm(config);
    acm.initialize();

    // Add an ABAC policy that allows only internal IPs
    PolicyEngine::Policy p;
    p.id = "internal-only";
    p.subjects = {"admin@test.com"};
    p.actions = {"write"};
    p.resources = {"data"};
    p.effect_allow = true;
    p.allowed_ip_prefixes = {"10.0."};
    acm.addABACPolicy(p);

    SecurityContext ctx;
    ctx.user_id = "admin@test.com";
    ctx.roles = {"admin"};
    ctx.source_ip = "10.0.1.5"; // In allowed IP range

    // RBAC allows (admin role), ABAC also allows (matching IP)
    auto decision = acm.authorize(ctx, "data", "write");
    EXPECT_TRUE(decision.granted);
}

TEST_F(AccessControlManagerTest, ABACUserAgentCondition) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.enable_abac = true;

    AccessControlManager acm(config);
    acm.initialize();

    // Add an ABAC policy restricting to trusted clients
    PolicyEngine::Policy p;
    p.id = "trusted-clients-only";
    p.subjects = {"*"};
    p.actions = {"read"};
    p.resources = {"data"};
    p.effect_allow = true;
    p.allowed_user_agent_patterns = {"ThemisClient"};
    acm.addABACPolicy(p);

    SecurityContext ctx_trusted;
    ctx_trusted.user_id = "user@test.com";
    ctx_trusted.roles = {"user"};
    ctx_trusted.user_agent = "ThemisClient/2.0";

    SecurityContext ctx_untrusted;
    ctx_untrusted.user_id = "user@test.com";
    ctx_untrusted.roles = {"user"};
    ctx_untrusted.user_agent = "curl/7.68.0";

    // Trusted client: RBAC allows (user can read data) + ABAC allows (matching UA)
    auto d1 = acm.authorize(ctx_trusted, "data", "read");
    EXPECT_TRUE(d1.granted);

    // Untrusted client: RBAC allows but ABAC denies (non-matching UA)
    auto d2 = acm.authorize(ctx_untrusted, "data", "read");
    EXPECT_FALSE(d2.granted);
}

TEST_F(AccessControlManagerTest, ABACRemovePolicy) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.enable_abac = true;

    AccessControlManager acm(config);
    acm.initialize();

    // Add an ABAC policy that restricts access
    PolicyEngine::Policy p;
    p.id = "restrict-ip";
    p.subjects = {"admin@test.com"};
    p.actions = {"write"};
    p.resources = {"data"};
    p.effect_allow = true;
    p.allowed_ip_prefixes = {"10.0."};
    acm.addABACPolicy(p);

    SecurityContext ctx;
    ctx.user_id = "admin@test.com";
    ctx.roles = {"admin"};
    ctx.source_ip = "192.168.1.1"; // Not in allowed range

    // ABAC denies
    auto d1 = acm.authorize(ctx, "data", "write");
    EXPECT_FALSE(d1.granted);

    // Remove the ABAC policy
    EXPECT_TRUE(acm.removeABACPolicy("restrict-ip"));

    // Now ABAC has no policies -> default allow, so RBAC decision stands
    auto d2 = acm.authorize(ctx, "data", "write");
    EXPECT_TRUE(d2.granted);
}

TEST_F(AccessControlManagerTest, ABACDoesNotAffectRBACDeny) {
    AccessControlConfig config;
    config.rbac_config_path = rbac_config_path_.string();
    config.user_role_store_path = user_roles_path_.string();
    config.enable_abac = true;

    AccessControlManager acm(config);
    acm.initialize();

    // Add a permissive ABAC policy (allow all)
    PolicyEngine::Policy p;
    p.id = "allow-all";
    p.subjects = {"*"};
    p.actions = {"write"};
    p.resources = {"data"};
    p.effect_allow = true;
    acm.addABACPolicy(p);

    SecurityContext ctx;
    ctx.user_id = "user@test.com";
    ctx.roles = {"user"}; // "user" role only has data:read, not data:write

    // RBAC denies (user role has no write permission) - ABAC is not evaluated
    auto decision = acm.authorize(ctx, "data", "write");
    EXPECT_FALSE(decision.granted);
}
