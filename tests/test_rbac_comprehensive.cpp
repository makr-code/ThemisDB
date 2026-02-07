/**
 * @file test_rbac_comprehensive.cpp
 * @brief Comprehensive tests for RBAC covering permission evaluation, role hierarchy, and edge cases
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include "security/rbac.h"

using namespace themis::security;
namespace fs = std::filesystem;

class RBACComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / ("rbac_test_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        fs::create_directories(test_dir_);
        
        RBACConfig config;
        config.enable_role_inheritance = true;
        config.enable_resource_wildcards = true;
        config.use_builtin_roles = false;
        rbac_ = std::make_unique<RBAC>(config);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
    std::unique_ptr<RBAC> rbac_;
};

// ============================================================================
// Permission Matching Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, ExactPermissionMatch_Allowed) {
    Role role;
    role.name = "reader";
    role.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(role);

    EXPECT_TRUE(rbac_->checkPermission({"reader"}, "data", "read"));
}

TEST_F(RBACComprehensiveTest, NoMatchingPermission_Denied) {
    Role role;
    role.name = "reader";
    role.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(role);

    EXPECT_FALSE(rbac_->checkPermission({"reader"}, "data", "write"));
    EXPECT_FALSE(rbac_->checkPermission({"reader"}, "config", "read"));
}

TEST_F(RBACComprehensiveTest, WildcardResource_MatchesAll) {
    Role role;
    role.name = "superuser";
    role.permissions.push_back(Permission{"*", "read"});
    rbac_->addRole(role);

    EXPECT_TRUE(rbac_->checkPermission({"superuser"}, "data", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"superuser"}, "config", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"superuser"}, "anything", "read"));
}

TEST_F(RBACComprehensiveTest, WildcardAction_MatchesAll) {
    Role role;
    role.name = "admin";
    role.permissions.push_back(Permission{"data", "*"});
    rbac_->addRole(role);

    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data", "write"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data", "delete"));
}

TEST_F(RBACComprehensiveTest, FullWildcard_AllowsEverything) {
    Role role;
    role.name = "god";
    role.permissions.push_back(Permission{"*", "*"});
    rbac_->addRole(role);

    EXPECT_TRUE(rbac_->checkPermission({"god"}, "anything", "anything"));
    EXPECT_TRUE(rbac_->checkPermission({"god"}, "data", "delete"));
    EXPECT_TRUE(rbac_->checkPermission({"god"}, "system", "reboot"));
}

// ============================================================================
// Multiple Roles Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, MultipleRoles_CombinePermissions) {
    Role reader;
    reader.name = "reader";
    reader.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(reader);

    Role writer;
    writer.name = "writer";
    writer.permissions.push_back(Permission{"data", "write"});
    rbac_->addRole(writer);

    EXPECT_TRUE(rbac_->checkPermission({"reader", "writer"}, "data", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"reader", "writer"}, "data", "write"));
    EXPECT_FALSE(rbac_->checkPermission({"reader", "writer"}, "data", "delete"));
}

TEST_F(RBACComprehensiveTest, EmptyRoleList_Denied) {
    EXPECT_FALSE(rbac_->checkPermission({}, "data", "read"));
}

TEST_F(RBACComprehensiveTest, NonExistentRole_Denied) {
    EXPECT_FALSE(rbac_->checkPermission({"nonexistent"}, "data", "read"));
}

// ============================================================================
// Role Inheritance Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, SimpleInheritance_InheritsPermissions) {
    Role base;
    base.name = "base";
    base.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(base);

    Role derived;
    derived.name = "derived";
    derived.inherits.push_back("base");
    derived.permissions.push_back(Permission{"data", "write"});
    rbac_->addRole(derived);

    // Derived should have both read and write
    EXPECT_TRUE(rbac_->checkPermission({"derived"}, "data", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"derived"}, "data", "write"));
}

TEST_F(RBACComprehensiveTest, MultiLevelInheritance_Works) {
    Role level1;
    level1.name = "level1";
    level1.permissions.push_back(Permission{"resource1", "read"});
    rbac_->addRole(level1);

    Role level2;
    level2.name = "level2";
    level2.inherits.push_back("level1");
    level2.permissions.push_back(Permission{"resource2", "read"});
    rbac_->addRole(level2);

    Role level3;
    level3.name = "level3";
    level3.inherits.push_back("level2");
    level3.permissions.push_back(Permission{"resource3", "read"});
    rbac_->addRole(level3);

    // Level3 should inherit all permissions
    EXPECT_TRUE(rbac_->checkPermission({"level3"}, "resource1", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"level3"}, "resource2", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"level3"}, "resource3", "read"));
}

TEST_F(RBACComprehensiveTest, MultipleInheritance_CombinesPermissions) {
    Role base1;
    base1.name = "base1";
    base1.permissions.push_back(Permission{"resource1", "read"});
    rbac_->addRole(base1);

    Role base2;
    base2.name = "base2";
    base2.permissions.push_back(Permission{"resource2", "write"});
    rbac_->addRole(base2);

    Role derived;
    derived.name = "derived";
    derived.inherits.push_back("base1");
    derived.inherits.push_back("base2");
    derived.permissions.push_back(Permission{"resource3", "delete"});
    rbac_->addRole(derived);

    EXPECT_TRUE(rbac_->checkPermission({"derived"}, "resource1", "read"));
    EXPECT_TRUE(rbac_->checkPermission({"derived"}, "resource2", "write"));
    EXPECT_TRUE(rbac_->checkPermission({"derived"}, "resource3", "delete"));
}

TEST_F(RBACComprehensiveTest, CircularInheritance_DetectedAndHandled) {
    Role role1;
    role1.name = "role1";
    role1.inherits.push_back("role2");
    rbac_->addRole(role1);

    Role role2;
    role2.name = "role2";
    role2.inherits.push_back("role1");
    rbac_->addRole(role2);

    // Should detect cycle and handle gracefully
    bool valid = rbac_->validateRoleHierarchy();
    EXPECT_FALSE(valid);
}

TEST_F(RBACComprehensiveTest, InheritFromNonExistent_HandledGracefully) {
    Role role;
    role.name = "orphan";
    role.inherits.push_back("nonexistent");
    role.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(role);

    // Should still have its own permissions
    EXPECT_TRUE(rbac_->checkPermission({"orphan"}, "data", "read"));
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, EmptyPermissionList_DeniesEverything) {
    Role role;
    role.name = "empty";
    // No permissions added
    rbac_->addRole(role);

    EXPECT_FALSE(rbac_->checkPermission({"empty"}, "data", "read"));
}

TEST_F(RBACComprehensiveTest, EmptyResourceOrAction_HandledGracefully) {
    Role role;
    role.name = "test";
    role.permissions.push_back(Permission{"", ""});
    rbac_->addRole(role);

    // Empty strings should match empty resource and action
    EXPECT_TRUE(rbac_->checkPermission({"test"}, "", ""));
    // But not match non-empty inputs
    EXPECT_FALSE(rbac_->checkPermission({"test"}, "data", "read"));
}

TEST_F(RBACComprehensiveTest, SpecialCharactersInResourceAction_Handled) {
    Role role;
    role.name = "special";
    role.permissions.push_back(Permission{"data/sub/path", "read:write"});
    rbac_->addRole(role);

    EXPECT_TRUE(rbac_->checkPermission({"special"}, "data/sub/path", "read:write"));
}

TEST_F(RBACComprehensiveTest, VeryLongRoleNames_Supported) {
    std::string long_name(1000, 'x');
    Role role;
    role.name = long_name;
    role.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(role);

    EXPECT_TRUE(rbac_->checkPermission({long_name}, "data", "read"));
}

TEST_F(RBACComprehensiveTest, CaseSensitiveRoleNames_Enforced) {
    Role lower;
    lower.name = "reader";
    lower.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(lower);

    Role upper;
    upper.name = "READER";
    upper.permissions.push_back(Permission{"config", "write"});
    rbac_->addRole(upper);

    EXPECT_TRUE(rbac_->checkPermission({"reader"}, "data", "read"));
    EXPECT_FALSE(rbac_->checkPermission({"reader"}, "config", "write"));
    EXPECT_TRUE(rbac_->checkPermission({"READER"}, "config", "write"));
    EXPECT_FALSE(rbac_->checkPermission({"READER"}, "data", "read"));
}

// ============================================================================
// Built-in Roles Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, BuiltinRoles_Available) {
    auto builtin_roles = RBAC::getBuiltinRoles();
    EXPECT_GT(builtin_roles.size(), 0);
    
    // Should have common roles
    bool has_admin = false;
    bool has_readonly = false;
    for (const auto& role : builtin_roles) {
        if (role.name == "admin") has_admin = true;
        if (role.name == "readonly") has_readonly = true;
    }
    
    EXPECT_TRUE(has_admin || has_readonly);
}

// ============================================================================
// Configuration File Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, LoadFromJson_Success) {
    auto config_path = test_dir_ / "roles.json";
    std::ofstream f(config_path);
    f << R"JSON([
        {
            "name": "analyst",
            "description": "Data analyst role",
            "permissions": [
                {"resource": "data", "action": "read"},
                {"resource": "reports", "action": "generate"}
            ]
        }
    ])JSON";
    f.close();

    ASSERT_TRUE(rbac_->loadConfig(config_path.string()));
    
    auto role = rbac_->getRole("analyst");
    ASSERT_TRUE(role.has_value());
    EXPECT_EQ(role->name, "analyst");
    EXPECT_EQ(role->permissions.size(), 2);
}

TEST_F(RBACComprehensiveTest, SaveAndReload_PreservesRoles) {
    Role role;
    role.name = "test_role";
    role.description = "Test Role Description";
    role.permissions.push_back(Permission{"data", "read"});
    role.permissions.push_back(Permission{"data", "write"});
    role.inherits.push_back("base_role");
    rbac_->addRole(role);

    auto save_path = test_dir_ / "saved_roles.json";
    ASSERT_TRUE(rbac_->saveConfig(save_path.string()));

    // Create new RBAC instance and load
    RBACConfig config;
    config.use_builtin_roles = false;
    RBAC rbac2(config);
    ASSERT_TRUE(rbac2.loadConfig(save_path.string()));

    auto loaded_role = rbac2.getRole("test_role");
    ASSERT_TRUE(loaded_role.has_value());
    EXPECT_EQ(loaded_role->name, "test_role");
    EXPECT_EQ(loaded_role->description, "Test Role Description");
    EXPECT_EQ(loaded_role->permissions.size(), 2);
    EXPECT_EQ(loaded_role->inherits.size(), 1);
}

TEST_F(RBACComprehensiveTest, LoadInvalidJson_ReturnsFalse) {
    auto config_path = test_dir_ / "invalid.json";
    std::ofstream f(config_path);
    f << "{ invalid json }";
    f.close();

    EXPECT_FALSE(rbac_->loadConfig(config_path.string()));
}

TEST_F(RBACComprehensiveTest, LoadNonExistentFile_ReturnsFalse) {
    EXPECT_FALSE(rbac_->loadConfig("/nonexistent/path.json"));
}

// ============================================================================
// UserRoleStore Tests
// ============================================================================

class UserRoleStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<UserRoleStore>();
        test_dir_ = fs::temp_directory_path() / ("user_role_test_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    std::unique_ptr<UserRoleStore> store_;
    fs::path test_dir_;
};

TEST_F(UserRoleStoreTest, AssignRole_Success) {
    store_->assignRole("alice", "admin");
    
    auto roles = store_->getUserRoles("alice");
    ASSERT_EQ(roles.size(), 1);
    EXPECT_EQ(roles[0], "admin");
}

TEST_F(UserRoleStoreTest, AssignMultipleRoles_AllStored) {
    store_->assignRole("bob", "reader");
    store_->assignRole("bob", "writer");
    store_->assignRole("bob", "analyst");
    
    auto roles = store_->getUserRoles("bob");
    EXPECT_EQ(roles.size(), 3);
}

TEST_F(UserRoleStoreTest, RevokeRole_Removes) {
    store_->assignRole("charlie", "admin");
    store_->assignRole("charlie", "operator");
    
    store_->revokeRole("charlie", "admin");
    
    auto roles = store_->getUserRoles("charlie");
    ASSERT_EQ(roles.size(), 1);
    EXPECT_EQ(roles[0], "operator");
}

TEST_F(UserRoleStoreTest, GetRoleUsers_ReturnsAllUsers) {
    store_->assignRole("alice", "admin");
    store_->assignRole("bob", "admin");
    store_->assignRole("charlie", "operator");
    
    auto admin_users = store_->getRoleUsers("admin");
    EXPECT_EQ(admin_users.size(), 2);
    
    auto operator_users = store_->getRoleUsers("operator");
    EXPECT_EQ(operator_users.size(), 1);
}

TEST_F(UserRoleStoreTest, GetNonExistentUser_ReturnsEmpty) {
    auto roles = store_->getUserRoles("nonexistent");
    EXPECT_TRUE(roles.empty());
}

TEST_F(UserRoleStoreTest, SaveAndLoad_PreservesData) {
    store_->assignRole("alice", "admin");
    store_->assignRole("alice", "operator");
    store_->assignRole("bob", "analyst");
    
    User user;
    user.user_id = "alice";
    user.roles = {"admin", "operator"};
    user.attributes["department"] = "engineering";
    store_->setUser(user);
    
    auto save_path = test_dir_ / "users.json";
    ASSERT_TRUE(store_->save(save_path.string()));
    
    UserRoleStore store2;
    ASSERT_TRUE(store2.load(save_path.string()));
    
    auto alice_roles = store2.getUserRoles("alice");
    EXPECT_EQ(alice_roles.size(), 2);
    
    auto alice_user = store2.getUser("alice");
    ASSERT_TRUE(alice_user.has_value());
    EXPECT_EQ(alice_user->attributes["department"], "engineering");
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, ConcurrentReads_ThreadSafe) {
    Role role;
    role.name = "concurrent";
    role.permissions.push_back(Permission{"data", "read"});
    rbac_->addRole(role);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &success_count]() {
            for (int j = 0; j < 100; ++j) {
                if (rbac_->checkPermission({"concurrent"}, "data", "read")) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, 1000);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(RBACComprehensiveTest, LargeRoleSet_PerformanceAcceptable) {
    // Add 100 roles
    for (int i = 0; i < 100; ++i) {
        Role role;
        role.name = "role_" + std::to_string(i);
        role.permissions.push_back(Permission{"resource_" + std::to_string(i), "read"});
        rbac_->addRole(role);
    }

    auto roles = rbac_->listRoles();
    EXPECT_EQ(roles.size(), 100);

    // Check permission should still be fast (simple correctness check)
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(rbac_->checkPermission({"role_50"}, "resource_50", "read"));
    }
}

TEST_F(RBACComprehensiveTest, DeepInheritanceChain_PerformanceAcceptable) {
    // Create chain: role_0 -> role_1 -> ... -> role_19
    for (int i = 0; i < 20; ++i) {
        Role role;
        role.name = "role_" + std::to_string(i);
        if (i > 0) {
            role.inherits.push_back("role_" + std::to_string(i - 1));
        }
        role.permissions.push_back(Permission{"resource_" + std::to_string(i), "read"});
        rbac_->addRole(role);
    }

    // Last role should have all permissions through inheritance
    auto permissions = rbac_->getUserPermissions({"role_19"});
    EXPECT_GE(permissions.size(), 20);
}
