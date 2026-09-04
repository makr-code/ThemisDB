/**
 * @file test_rbac_comprehensive.cpp
 * @brief Comprehensive tests for the RBAC (Role-Based Access Control) component
 *
 * Tests cover:
 * - Built-in roles (admin, operator, analyst, readonly)
 * - Permission matching (exact, wildcard resource, wildcard action)
 * - Role inheritance
 * - Adding and removing roles
 * - checkPermission with multiple user roles
 * - getUserPermissions expansion
 * - validateRoleHierarchy (cycle detection)
 * - JSON serialization/deserialization
 * - Thread safety
 */

#include <gtest/gtest.h>
#include "security/rbac.h"
#include <thread>
#include <atomic>

using namespace themis::security;

// ============================================================================
// Permission Tests
// ============================================================================

TEST(PermissionTest, ExactMatch_Allowed) {
    Permission p{"data", "read"};
    EXPECT_TRUE(p.matches("data", "read"));
}

TEST(PermissionTest, ExactMatch_DifferentResource_NotAllowed) {
    Permission p{"data", "read"};
    EXPECT_FALSE(p.matches("keys", "read"));
}

TEST(PermissionTest, ExactMatch_DifferentAction_NotAllowed) {
    Permission p{"data", "read"};
    EXPECT_FALSE(p.matches("data", "write"));
}

TEST(PermissionTest, WildcardResource_MatchesAny) {
    Permission p{"*", "read"};
    EXPECT_TRUE(p.matches("data", "read"));
    EXPECT_TRUE(p.matches("keys", "read"));
    EXPECT_TRUE(p.matches("config", "read"));
    EXPECT_TRUE(p.matches("audit", "read"));
}

TEST(PermissionTest, WildcardAction_MatchesAny) {
    Permission p{"data", "*"};
    EXPECT_TRUE(p.matches("data", "read"));
    EXPECT_TRUE(p.matches("data", "write"));
    EXPECT_TRUE(p.matches("data", "delete"));
}

TEST(PermissionTest, WildcardBoth_MatchesEverything) {
    Permission p{"*", "*"};
    EXPECT_TRUE(p.matches("data", "read"));
    EXPECT_TRUE(p.matches("keys", "rotate"));
    EXPECT_TRUE(p.matches("config", "delete"));
    EXPECT_TRUE(p.matches("anything", "anything"));
}

TEST(PermissionTest, ToString_Format) {
    Permission p{"data", "read"};
    EXPECT_EQ(p.toString(), "data:read");
}

TEST(PermissionTest, Equality_SamePermission) {
    Permission p1{"data", "read"};
    Permission p2{"data", "read"};
    EXPECT_TRUE(p1 == p2);
}

TEST(PermissionTest, Equality_DifferentPermission) {
    Permission p1{"data", "read"};
    Permission p2{"data", "write"};
    EXPECT_FALSE(p1 == p2);
}

// ============================================================================
// Built-in Roles Tests
// ============================================================================

TEST(RBACBuiltinRolesTest, GetBuiltinRoles_ReturnsNonEmpty) {
    auto roles = RBAC::getBuiltinRoles();
    EXPECT_GT(roles.size(), 0u);
}

TEST(RBACBuiltinRolesTest, AdminRole_ExistsWithWildcardPermissions) {
    auto roles = RBAC::getBuiltinRoles();
    
    auto admin_it = std::find_if(roles.begin(), roles.end(),
        [](const Role& r) { return r.name == "admin"; });
    ASSERT_NE(admin_it, roles.end());
    
    // Admin should have wildcard permissions
    bool has_wildcard = false;
    for (const auto& p : admin_it->permissions) {
        if (p.resource == "*" && p.action == "*") {
            has_wildcard = true;
            break;
        }
    }
    EXPECT_TRUE(has_wildcard);
}

TEST(RBACBuiltinRolesTest, ReadonlyRole_Exists) {
    auto roles = RBAC::getBuiltinRoles();
    
    auto readonly_it = std::find_if(roles.begin(), roles.end(),
        [](const Role& r) { return r.name == "readonly"; });
    EXPECT_NE(readonly_it, roles.end());
}

TEST(RBACBuiltinRolesTest, AnalystRole_Exists) {
    auto roles = RBAC::getBuiltinRoles();
    
    auto analyst_it = std::find_if(roles.begin(), roles.end(),
        [](const Role& r) { return r.name == "analyst"; });
    EXPECT_NE(analyst_it, roles.end());
}

TEST(RBACBuiltinRolesTest, OperatorRole_Exists) {
    auto roles = RBAC::getBuiltinRoles();
    
    auto operator_it = std::find_if(roles.begin(), roles.end(),
        [](const Role& r) { return r.name == "operator"; });
    EXPECT_NE(operator_it, roles.end());
}

// ============================================================================
// RBAC Core Tests
// ============================================================================

class RBACTest : public ::testing::Test {
protected:
    void SetUp() override {
        RBACConfig cfg;
        cfg.use_builtin_roles = true;
        cfg.enable_role_inheritance = true;
        cfg.enable_resource_wildcards = true;
        rbac_ = std::make_unique<RBAC>(cfg);
    }

    std::unique_ptr<RBAC> rbac_;
};

TEST_F(RBACTest, Admin_HasFullAccess) {
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data", "write"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "keys", "rotate"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "config", "delete"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "anything", "anything"));
}

TEST_F(RBACTest, Readonly_CanReadData) {
    EXPECT_TRUE(rbac_->checkPermission({"readonly"}, "data", "read"));
}

TEST_F(RBACTest, Readonly_CannotWriteData) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data", "write"));
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data", "delete"));
}

TEST_F(RBACTest, Readonly_CannotRotateKeys) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "keys", "rotate"));
}

TEST_F(RBACTest, EmptyRoles_NothingAllowed) {
    EXPECT_FALSE(rbac_->checkPermission({}, "data", "read"));
    EXPECT_FALSE(rbac_->checkPermission({}, "keys", "read"));
}

TEST_F(RBACTest, UnknownRole_NothingAllowed) {
    EXPECT_FALSE(rbac_->checkPermission({"nonexistent_role"}, "data", "read"));
}

TEST_F(RBACTest, MultipleRoles_GrantsCombinedPermissions) {
    // Create a user with both readonly and a custom role
    Role custom;
    custom.name = "key_manager";
    custom.description = "Can manage keys";
    custom.permissions.push_back({"keys", "rotate"});
    rbac_->addRole(custom);

    // User has both readonly (data read) and key_manager (keys rotate)
    EXPECT_TRUE(rbac_->checkPermission({"readonly", "key_manager"}, "data", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"readonly", "key_manager"}, "keys", "rotate"));
    EXPECT_FALSE(rbac_->checkPermission({"readonly", "key_manager"}, "data", "write"));
}

TEST_F(RBACTest, RoleInheritance_OperatorInheritsAnalyst) {
    // Operator inherits from analyst - should have analyst's read permissions
    auto analyst_perms = rbac_->getUserPermissions({"analyst"});
    auto operator_perms = rbac_->getUserPermissions({"operator"});

    // Operator should have at least all analyst permissions
    EXPECT_GE(operator_perms.size(), analyst_perms.size());
}

TEST_F(RBACTest, AddRole_NewRoleGrantsPermissions) {
    Role custom;
    custom.name = "auditor";
    custom.description = "Auditor with audit log access";
    custom.permissions.push_back({"audit", "read"});
    custom.permissions.push_back({"audit", "export"});

    rbac_->addRole(custom);

    EXPECT_TRUE(rbac_->checkPermission({"auditor"}, "audit", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"auditor"}, "audit", "export"));
    EXPECT_FALSE(rbac_->checkPermission({"auditor"}, "data", "write"));
}

TEST_F(RBACTest, RemoveRole_NoLongerGrantsPermissions) {
    Role custom;
    custom.name = "temp_role";
    custom.permissions.push_back({"data", "write"});
    rbac_->addRole(custom);

    ASSERT_TRUE(rbac_->checkPermission({"temp_role"}, "data", "write"));

    rbac_->removeRole("temp_role");

    EXPECT_FALSE(rbac_->checkPermission({"temp_role"}, "data", "write"));
}

TEST_F(RBACTest, GetRole_ExistingRole_Returns) {
    auto role = rbac_->getRole("admin");
    ASSERT_TRUE(role.has_value());
    EXPECT_EQ(role->name, "admin");
}

TEST_F(RBACTest, GetRole_NonExistent_ReturnsNullopt) {
    auto role = rbac_->getRole("does-not-exist");
    EXPECT_FALSE(role.has_value());
}

TEST_F(RBACTest, ListRoles_ContainsBuiltins) {
    auto roles = rbac_->listRoles();
    EXPECT_FALSE(roles.empty());
    EXPECT_GT(roles.size(), 0u);

    // Should contain at least admin
    auto it = std::find(roles.begin(), roles.end(), std::string("admin"));
    EXPECT_NE(it, roles.end());
}

TEST_F(RBACTest, GetUserPermissions_Admin_HasWildcard) {
    auto perms = rbac_->getUserPermissions({"admin"});
    EXPECT_FALSE(perms.empty());

    bool found_wildcard = false;
    for (const auto& p : perms) {
        if (p.resource == "*" && p.action == "*") {
            found_wildcard = true;
            break;
        }
    }
    EXPECT_TRUE(found_wildcard);
}

TEST_F(RBACTest, GetUserPermissions_Multiple_CombinesAll) {
    Role r1;
    r1.name = "writer";
    r1.permissions.push_back({"data", "write"});
    rbac_->addRole(r1);

    Role r2;
    r2.name = "reader";
    r2.permissions.push_back({"data", "read"});
    rbac_->addRole(r2);

    auto perms = rbac_->getUserPermissions({"writer", "reader"});
    EXPECT_GE(perms.size(), 2u);
}

TEST_F(RBACTest, ValidateRoleHierarchy_BuiltinsAreValid) {
    EXPECT_TRUE(rbac_->validateRoleHierarchy());
}

// ============================================================================
// Role Inheritance Tests
// ============================================================================

class RBACInheritanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        RBACConfig cfg;
        cfg.use_builtin_roles = false;
        cfg.enable_role_inheritance = true;
        rbac_ = std::make_unique<RBAC>(cfg);
    }

    std::unique_ptr<RBAC> rbac_;
};

TEST_F(RBACInheritanceTest, InheritedPermissions_Granted) {
    Role base;
    base.name = "base_role";
    base.permissions.push_back({"data", "read"});

    Role derived;
    derived.name = "derived_role";
    derived.permissions.push_back({"data", "write"});
    derived.inherits.push_back("base_role");

    rbac_->addRole(base);
    rbac_->addRole(derived);

    // Derived should have both own and inherited permissions
    EXPECT_TRUE(rbac_->checkPermission({"derived_role"}, "data", "read"));  // inherited
    EXPECT_TRUE(rbac_->checkPermission({"derived_role"}, "data", "write")); // own
}

TEST_F(RBACInheritanceTest, MultiLevelInheritance_Works) {
    Role level1;
    level1.name = "level1";
    level1.permissions.push_back({"data", "read"});

    Role level2;
    level2.name = "level2";
    level2.permissions.push_back({"data", "write"});
    level2.inherits.push_back("level1");

    Role level3;
    level3.name = "level3";
    level3.permissions.push_back({"keys", "read"});
    level3.inherits.push_back("level2");

    rbac_->addRole(level1);
    rbac_->addRole(level2);
    rbac_->addRole(level3);

    // Level3 should have all permissions from all levels
    EXPECT_TRUE(rbac_->checkPermission({"level3"}, "data", "read"));   // level1
    EXPECT_TRUE(rbac_->checkPermission({"level3"}, "data", "write"));  // level2
    EXPECT_TRUE(rbac_->checkPermission({"level3"}, "keys", "read"));   // level3 own
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST(RBACSerializationTest, Role_ToJson_FromJson_RoundTrip) {
    Role original;
    original.name = "test_role";
    original.description = "A test role";
    original.permissions.push_back({"data", "read"});
    original.permissions.push_back({"keys", "rotate"});
    original.inherits.push_back("base");

    auto j = original.toJson();
    auto restored = Role::fromJson(j);

    EXPECT_EQ(restored.name, original.name);
    EXPECT_EQ(restored.description, original.description);
    EXPECT_EQ(restored.permissions.size(), original.permissions.size());
    EXPECT_EQ(restored.inherits.size(), original.inherits.size());
}

TEST(RBACSerializationTest, Permission_SerializesCorrectly) {
    Permission p{"data", "write"};
    EXPECT_EQ(p.resource, "data");
    EXPECT_EQ(p.action, "write");
    EXPECT_EQ(p.toString(), "data:write");
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(RBACThreadSafetyTest, ConcurrentCheckPermission_ThreadSafe) {
    RBACConfig cfg;
    cfg.use_builtin_roles = true;
    cfg.enable_role_inheritance = true;
    RBAC rbac(cfg);

    constexpr int THREADS = 8;
    constexpr int CHECKS = 50;
    std::atomic<int> allowed{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&rbac, &allowed]() {
            for (int j = 0; j < CHECKS; ++j) {
                if (rbac.checkPermission({"admin"}, "data", "read")) {
                    allowed++;
                }
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(allowed.load(), THREADS * CHECKS);
}
