/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_multi_level_storage.cpp                       ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:28:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "../include/multi_level_storage.hpp"
#include "../include/gocryptfs_backend.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>

using namespace themis::plugins::user_storage;

class MultiLevelStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = "/tmp/themis_test_storage_" + std::to_string(getpid());
        mkdir(test_dir_.c_str(), 0700);
        
        storage_ = std::make_unique<MultiLevelEncryptedStorage>();
    }
    
    void TearDown() override {
        if (storage_) {
            storage_->shutdown();
        }
        
        // Clean up test directory using C++17 filesystem
        try {
            std::filesystem::remove_all(test_dir_);
        } catch (const std::exception& e) {
            // Ignore cleanup errors
        }
    }
    
    std::string getSimpleConfig() {
        std::ostringstream config;
        config << "{";
        config << "  \"multi_level_storage\": {";
        config << "    \"levels\": [";
        config << "      {";
        config << "        \"name\": \"offen\",";
        config << "        \"path\": \"" << test_dir_ << "/offen\",";
        config << "        \"encrypted\": false";
        config << "      }";
        config << "    ]";
        config << "  }";
        config << "}";
        return config.str();
    }
    
    std::string test_dir_;
    std::unique_ptr<MultiLevelEncryptedStorage> storage_;
};

TEST_F(MultiLevelStorageTest, InitializeWithSimpleConfig) {
    std::string config = getSimpleConfig();
    bool result = storage_->initialize(config.c_str());
    EXPECT_TRUE(result);
}

TEST_F(MultiLevelStorageTest, CreateAndGetUser) {
    std::string config = getSimpleConfig();
    ASSERT_TRUE(storage_->initialize(config.c_str()));
    
    // Create user
    User user;
    user.user_id = "test_user_001";
    user.username = "testuser";
    user.email = "test@example.com";
    user.full_name = "Test User";
    user.roles = {"admin"};
    user.classification = SecurityLevel::OFFEN;
    user.created_at_ms = 1234567890;
    user.updated_at_ms = 1234567890;
    
    auto create_result = storage_->createUser(user, SecurityLevel::OFFEN);
    ASSERT_TRUE(create_result.isSuccess()) << create_result.error();
    
    // Get user
    auto get_result = storage_->getUser("test_user_001", SecurityLevel::OFFEN);
    ASSERT_TRUE(get_result.isSuccess()) << get_result.error();
    
    const User& retrieved = get_result.value();
    EXPECT_EQ(retrieved.user_id, "test_user_001");
    EXPECT_EQ(retrieved.username, "testuser");
    EXPECT_EQ(retrieved.email, "test@example.com");
    EXPECT_EQ(retrieved.full_name, "Test User");
    EXPECT_EQ(retrieved.roles.size(), 1);
    EXPECT_EQ(retrieved.roles[0], "admin");
}

TEST_F(MultiLevelStorageTest, UpdateUser) {
    std::string config = getSimpleConfig();
    ASSERT_TRUE(storage_->initialize(config.c_str()));
    
    // Create user
    User user;
    user.user_id = "test_user_002";
    user.username = "testuser2";
    user.email = "test2@example.com";
    user.full_name = "Test User 2";
    user.classification = SecurityLevel::OFFEN;
    
    ASSERT_TRUE(storage_->createUser(user, SecurityLevel::OFFEN).isSuccess());
    
    // Update user
    user.email = "updated@example.com";
    user.full_name = "Updated User";
    
    auto update_result = storage_->updateUser(user, SecurityLevel::OFFEN);
    ASSERT_TRUE(update_result.isSuccess());
    
    // Verify update
    auto get_result = storage_->getUser("test_user_002", SecurityLevel::OFFEN);
    ASSERT_TRUE(get_result.isSuccess());
    
    const User& retrieved = get_result.value();
    EXPECT_EQ(retrieved.email, "updated@example.com");
    EXPECT_EQ(retrieved.full_name, "Updated User");
}

TEST_F(MultiLevelStorageTest, DeleteUser) {
    std::string config = getSimpleConfig();
    ASSERT_TRUE(storage_->initialize(config.c_str()));
    
    // Create user
    User user;
    user.user_id = "test_user_003";
    user.username = "testuser3";
    user.classification = SecurityLevel::OFFEN;
    
    ASSERT_TRUE(storage_->createUser(user, SecurityLevel::OFFEN).isSuccess());
    
    // Verify user exists
    ASSERT_TRUE(storage_->getUser("test_user_003", SecurityLevel::OFFEN).isSuccess());
    
    // Delete user
    auto delete_result = storage_->deleteUser("test_user_003", SecurityLevel::OFFEN);
    ASSERT_TRUE(delete_result.isSuccess());
    
    // Verify user is deleted
    auto get_result = storage_->getUser("test_user_003", SecurityLevel::OFFEN);
    EXPECT_TRUE(get_result.isError());
}

TEST_F(MultiLevelStorageTest, CreateAndGetGroup) {
    std::string config = getSimpleConfig();
    ASSERT_TRUE(storage_->initialize(config.c_str()));
    
    // Create group
    Group group;
    group.group_id = "test_group_001";
    group.name = "Test Group";
    group.description = "A test group";
    group.member_ids = {"user_001", "user_002"};
    group.classification = SecurityLevel::OFFEN;
    group.created_at_ms = 1234567890;
    
    auto create_result = storage_->createGroup(group, SecurityLevel::OFFEN);
    ASSERT_TRUE(create_result.isSuccess()) << create_result.error();
    
    // Get group
    auto get_result = storage_->getGroup("test_group_001", SecurityLevel::OFFEN);
    ASSERT_TRUE(get_result.isSuccess()) << get_result.error();
    
    const Group& retrieved = get_result.value();
    EXPECT_EQ(retrieved.group_id, "test_group_001");
    EXPECT_EQ(retrieved.name, "Test Group");
    EXPECT_EQ(retrieved.description, "A test group");
    EXPECT_EQ(retrieved.member_ids.size(), 2);
}

TEST_F(MultiLevelStorageTest, HealthCheck) {
    std::string config = getSimpleConfig();
    ASSERT_TRUE(storage_->initialize(config.c_str()));
    
    auto health_result = storage_->checkHealth();
    ASSERT_TRUE(health_result.isSuccess());
    
    const HealthStatus& status = health_result.value();
    EXPECT_TRUE(status.healthy);
    EXPECT_GT(status.checked_at_ms, 0);
}

TEST_F(MultiLevelStorageTest, SecurityLevelConversion) {
    EXPECT_EQ(securityLevelToString(SecurityLevel::OFFEN), "offen");
    EXPECT_EQ(securityLevelToString(SecurityLevel::VS_NFD), "vs-nfd");
    EXPECT_EQ(securityLevelToString(SecurityLevel::GEHEIM), "geheim");
    EXPECT_EQ(securityLevelToString(SecurityLevel::STRENG_GEHEIM), "streng-geheim");
    
    EXPECT_EQ(stringToSecurityLevel("offen"), SecurityLevel::OFFEN);
    EXPECT_EQ(stringToSecurityLevel("vs-nfd"), SecurityLevel::VS_NFD);
    EXPECT_EQ(stringToSecurityLevel("geheim"), SecurityLevel::GEHEIM);
    EXPECT_EQ(stringToSecurityLevel("streng-geheim"), SecurityLevel::STRENG_GEHEIM);
    
    EXPECT_THROW(stringToSecurityLevel("invalid"), std::invalid_argument);
}

// Mock test for gocryptfs backend (requires gocryptfs to be installed)
TEST(GocryptfsBackendTest, CheckAvailability) {
    GocryptfsBackend backend;
    backend.initialize("{}");
    
    // This test may fail if gocryptfs is not installed
    // In CI, we can skip this test or install gocryptfs
    auto result = backend.checkAvailability();
    if (result.isError()) {
        GTEST_SKIP() << "gocryptfs not available: " << result.error();
    }
}

TEST(GocryptfsBackendTest, GetBackendInfo) {
    GocryptfsBackend backend;
    EXPECT_EQ(backend.getBackendName(), "gocryptfs");
}

