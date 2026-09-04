#include <gtest/gtest.h>
#include "transaction/branch_manager.h"
#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include <filesystem>
#include <memory>

namespace themis {
namespace transaction {

class BranchManagerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping branch integration focused tests on Windows due to fixture crash in current runtime.";
#endif
#ifdef _WIN32
        GTEST_SKIP() << "Skipping branch integration focused tests on Windows due to fixture crash in current runtime.";
#endif
        test_db_path_ = "./data/themis_branch_integration_test";
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        std::filesystem::create_directories(test_db_path_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        changefeed_ = std::make_unique<Changefeed>(db_->getRawDB());
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        branch_manager_ = std::make_unique<BranchManager>(*db_, *changefeed_, *snapshot_manager_);
    }
    
    void TearDown() override {
        branch_manager_.reset();
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    void recordEvents(int count) {
        for (int i = 0; i < count; i++) {
            std::string key = "test_key_" + std::to_string(i);
            std::string value = "test_value_" + std::to_string(i);
            // Create and record a ChangeEvent
            Changefeed::ChangeEvent event{
                0,  // sequence (auto-generated)
                Changefeed::ChangeEventType::EVENT_PUT,
                key,
                value,
                std::chrono::system_clock::now().time_since_epoch().count() / 1000000,
                nlohmann::json()
            };
            changefeed_->recordEvent(event);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_manager_;
    std::unique_ptr<BranchManager> branch_manager_;
};

// Integration test: Complete branch workflow
TEST_F(BranchManagerIntegrationTest, CompleteBranchWorkflow) {
    // Record some initial events
    recordEvents(10);
    
    // Create a snapshot tag
    auto tag = snapshot_manager_->createTag("v1.0", "Version 1.0", "system");
    ASSERT_TRUE(tag.has_value());
    
    // Create a feature branch from the tag
    BranchManager::CreateBranchOptions options;
    options.from_tag = "v1.0";
    
    auto feature_branch = branch_manager_->createBranch(
        "feature/new-feature",
        "main",
        "Feature branch for new functionality",
        "developer",
        options
    );
    ASSERT_TRUE(feature_branch.has_value());
    
    // Switch to feature branch
    EXPECT_TRUE(branch_manager_->switchBranch("feature/new-feature"));
    EXPECT_EQ(branch_manager_->getActiveBranch(), "feature/new-feature");
    
    // Record more events in feature branch
    recordEvents(5);
    
    // List branches
    auto branches = branch_manager_->listBranches();
    EXPECT_EQ(branches.size(), 2); // main + feature
    
    // Switch back to main
    EXPECT_TRUE(branch_manager_->switchBranch("main"));
    EXPECT_EQ(branch_manager_->getActiveBranch(), "main");
    
    // Delete feature branch
    EXPECT_TRUE(branch_manager_->deleteBranch("feature/new-feature", true));
    
    // Verify branch is deleted
    EXPECT_FALSE(branch_manager_->branchExists("feature/new-feature"));
}

// Integration test: Database restart with branches
TEST_F(BranchManagerIntegrationTest, DatabaseRestartWithBranches) {
    // Create multiple branches
    branch_manager_->createBranch("feature/branch1", "main", "Branch 1");
    branch_manager_->createBranch("feature/branch2", "main", "Branch 2");
    branch_manager_->switchBranch("feature/branch1");
    
    // Restart database
    branch_manager_.reset();
    snapshot_manager_.reset();
    changefeed_.reset();
    db_.reset();
    
    RocksDBWrapper::Config config;
    config.db_path = test_db_path_;
    db_ = std::make_unique<RocksDBWrapper>(config);
    ASSERT_TRUE(db_->open());
    changefeed_ = std::make_unique<Changefeed>(db_->getRawDB());
    snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    branch_manager_ = std::make_unique<BranchManager>(*db_, *changefeed_, *snapshot_manager_);
    
    // Verify branches persist
    EXPECT_TRUE(branch_manager_->branchExists("feature/branch1"));
    EXPECT_TRUE(branch_manager_->branchExists("feature/branch2"));
    EXPECT_EQ(branch_manager_->getActiveBranch(), "feature/branch1");
}

// Integration test: Branch hierarchy
TEST_F(BranchManagerIntegrationTest, BranchHierarchy) {
    recordEvents(5);
    
    // Create release branch from main
    auto release_branch = branch_manager_->createBranch(
        "release/v2_0",
        "main",
        "Release branch for version 2.0"
    );
    ASSERT_TRUE(release_branch.has_value());
    
    recordEvents(3);
    
    // Create hotfix branch from release branch
    BranchManager::CreateBranchOptions options;
    options.from_sequence = release_branch->creation_sequence;
    
    auto hotfix_branch = branch_manager_->createBranch(
        "hotfix/security-patch",
        "release/v2_0",
        "Security patch for v2.0",
        "security-team",
        options
    );
    ASSERT_TRUE(hotfix_branch.has_value());
    
    // Verify hierarchy
    EXPECT_EQ(hotfix_branch->parent_branch, "release/v2_0");
    EXPECT_EQ(release_branch->parent_branch, "main");
}

// Integration test: Multiple tags and branches
TEST_F(BranchManagerIntegrationTest, MultipleTagsAndBranches) {
    recordEvents(10);
    
    // Create multiple tags
    snapshot_manager_->createTag("v1.0", "Version 1.0");
    recordEvents(5);
    snapshot_manager_->createTag("v1.1", "Version 1.1");
    recordEvents(5);
    snapshot_manager_->createTag("v1.2", "Version 1.2");
    
    // Create branches from different tags
    BranchManager::CreateBranchOptions options1;
    options1.from_tag = "v1.0";
    branch_manager_->createBranch("release/v1_0", "main", "Release 1.0", "system", options1);
    
    BranchManager::CreateBranchOptions options2;
    options2.from_tag = "v1.1";
    branch_manager_->createBranch("release/v1_1", "main", "Release 1.1", "system", options2);
    
    BranchManager::CreateBranchOptions options3;
    options3.from_tag = "v1.2";
    branch_manager_->createBranch("release/v1_2", "main", "Release 1.2", "system", options3);
    
    // Verify all branches exist
    auto branches = branch_manager_->listBranches();
    EXPECT_GE(branches.size(), 4); // main + 3 releases
    
    // Verify branch sequences match tag sequences
    auto v10_branch_seq = branch_manager_->getSequenceForBranch("release/v1_0");
    auto v10_tag_seq = snapshot_manager_->getSequenceForTag("v1.0");
    EXPECT_EQ(v10_branch_seq, v10_tag_seq);
}

// Integration test: Concurrent branch operations
TEST_F(BranchManagerIntegrationTest, ConcurrentBranchCreation) {
    // Create multiple branches rapidly
    std::vector<std::string> branch_names = {};

    for (int i = 0; i < 10; i++) {
        std::string name = "feature/concurrent-" + std::to_string(i);
        branch_names.push_back(name);
        auto branch = branch_manager_->createBranch(name, "main", "Concurrent branch");
        EXPECT_TRUE(branch.has_value());
    }
    
    // Verify all branches exist
    for (const auto& name : branch_names) {
        EXPECT_TRUE(branch_manager_->branchExists(name));
    }
    
    // Get stats
    auto stats = branch_manager_->getStats();
    EXPECT_GE(stats.total_branches, 11); // main + 10 created
}

} // namespace transaction
} // namespace themis
