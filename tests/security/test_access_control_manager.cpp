/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_access_control_manager.cpp                    ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:39:18                                ║
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
