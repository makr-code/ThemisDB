#include <gtest/gtest.h>
#include "transaction/branch_manager.h"
#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include <filesystem>
#include <memory>

namespace themis {
namespace transaction {

class BranchManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping branch manager focused tests on Windows due to fixture crash in current runtime.";
#endif
#ifdef _WIN32
        GTEST_SKIP() << "Skipping branch manager focused tests on Windows due to fixture crash in current runtime.";
#endif
        // Create unique test database path
        test_db_path_ = "./data/themis_branch_manager_test";
        
        // Remove if exists
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create directory
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize database
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        // Initialize changefeed
        changefeed_ = std::make_unique<Changefeed>(db_->getRawDB());
        
        // Initialize snapshot manager
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        
        // Initialize branch manager
        branch_manager_ = std::make_unique<BranchManager>(*db_, *changefeed_, *snapshot_manager_);
    }
    
    void TearDown() override {
        // Reset in correct order
        branch_manager_.reset();
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();
        
        // Remove test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    // Helper: Record some events in changefeed
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

// Test: Default branch exists after initialization
TEST_F(BranchManagerTest, DefaultBranchExists) {
    EXPECT_TRUE(branch_manager_->branchExists("main"));
    
    auto main_branch = branch_manager_->getBranch("main");
    ASSERT_TRUE(main_branch.has_value());
    EXPECT_EQ(main_branch->branch_name, "main");
    EXPECT_EQ(main_branch->parent_branch, "");
    EXPECT_EQ(main_branch->description, "Default main branch");
}

// Test: Create branch
TEST_F(BranchManagerTest, CreateBranch) {
    recordEvents(10);
    
    auto branch = branch_manager_->createBranch(
        "feature/test",
        "main",
        "Test feature branch",
        "test_user"
    );
    
    ASSERT_TRUE(branch.has_value());
    EXPECT_EQ(branch->branch_name, "feature/test");
    EXPECT_EQ(branch->parent_branch, "main");
    EXPECT_EQ(branch->description, "Test feature branch");
    EXPECT_EQ(branch->created_by, "test_user");
    EXPECT_FALSE(branch->is_active);
    EXPECT_GT(branch->creation_sequence, 0);
}

// Test: Create duplicate branch fails
TEST_F(BranchManagerTest, CreateDuplicateBranchFails) {
    auto branch1 = branch_manager_->createBranch(
        "feature/test",
        "main",
        "First branch"
    );
    ASSERT_TRUE(branch1.has_value());
    
    auto branch2 = branch_manager_->createBranch(
        "feature/test",
        "main",
        "Duplicate branch"
    );
    EXPECT_FALSE(branch2.has_value());
}

// Test: Create branch with invalid name fails
TEST_F(BranchManagerTest, CreateBranchWithInvalidNameFails) {
    // Empty name
    auto branch1 = branch_manager_->createBranch("", "main", "Empty name");
    EXPECT_FALSE(branch1.has_value());
    
    // Invalid characters
    auto branch2 = branch_manager_->createBranch("feature@test", "main", "Invalid chars");
    EXPECT_FALSE(branch2.has_value());
    
    // Reserved name
    auto branch3 = branch_manager_->createBranch("HEAD", "main", "Reserved name");
    EXPECT_FALSE(branch3.has_value());
}

// Test: Create branch with non-existent parent fails
TEST_F(BranchManagerTest, CreateBranchWithNonExistentParentFails) {
    auto branch = branch_manager_->createBranch(
        "feature/test",
        "nonexistent",
        "Test branch"
    );
    EXPECT_FALSE(branch.has_value());
}

// Test: Get branch
TEST_F(BranchManagerTest, GetBranch) {
    branch_manager_->createBranch(
        "feature/test",
        "main",
        "Test branch"
    );
    
    auto branch = branch_manager_->getBranch("feature/test");
    ASSERT_TRUE(branch.has_value());
    EXPECT_EQ(branch->branch_name, "feature/test");
}

// Test: Get non-existent branch
TEST_F(BranchManagerTest, GetNonExistentBranch) {
    auto branch = branch_manager_->getBranch("nonexistent");
    EXPECT_FALSE(branch.has_value());
}

// Test: List branches
TEST_F(BranchManagerTest, ListBranches) {
    branch_manager_->createBranch("feature/test1", "main", "Test 1");
    branch_manager_->createBranch("feature/test2", "main", "Test 2");
    branch_manager_->createBranch("bugfix/issue123", "main", "Bugfix");
    
    auto branches = branch_manager_->listBranches();
    
    // Should have 4 branches: main + 3 created
    EXPECT_EQ(branches.size(), 4);
}

// Test: List branches with limit
TEST_F(BranchManagerTest, ListBranchesWithLimit) {
    branch_manager_->createBranch("feature/test1", "main", "Test 1");
    branch_manager_->createBranch("feature/test2", "main", "Test 2");
    branch_manager_->createBranch("bugfix/issue123", "main", "Bugfix");
    
    auto branches = branch_manager_->listBranches(2);
    EXPECT_EQ(branches.size(), 2);
}

// Test: List branches sorted by name
TEST_F(BranchManagerTest, ListBranchesSortedByName) {
    branch_manager_->createBranch("zebra", "main", "Z");
    branch_manager_->createBranch("alpha", "main", "A");
    branch_manager_->createBranch("beta", "main", "B");
    
    auto branches = branch_manager_->listBranches(0, "name", true);
    
    EXPECT_EQ(branches[0].branch_name, "alpha");
    EXPECT_EQ(branches[1].branch_name, "beta");
    EXPECT_EQ(branches[2].branch_name, "main");
    EXPECT_EQ(branches[3].branch_name, "zebra");
}

// Test: Switch branch
TEST_F(BranchManagerTest, SwitchBranch) {
    branch_manager_->createBranch("feature/test", "main", "Test");
    
    EXPECT_EQ(branch_manager_->getActiveBranch(), "main");
    
    bool success = branch_manager_->switchBranch("feature/test");
    EXPECT_TRUE(success);
    EXPECT_EQ(branch_manager_->getActiveBranch(), "feature/test");
}

// Test: Switch to non-existent branch fails
TEST_F(BranchManagerTest, SwitchToNonExistentBranchFails) {
    bool success = branch_manager_->switchBranch("nonexistent");
    EXPECT_FALSE(success);
    EXPECT_EQ(branch_manager_->getActiveBranch(), "main");
}

// Test: Delete branch
TEST_F(BranchManagerTest, DeleteBranch) {
    branch_manager_->createBranch("feature/test", "main", "Test");
    
    EXPECT_TRUE(branch_manager_->branchExists("feature/test"));
    
    bool success = branch_manager_->deleteBranch("feature/test", true);
    EXPECT_TRUE(success);
    EXPECT_FALSE(branch_manager_->branchExists("feature/test"));
}

// Test: Cannot delete default branch
TEST_F(BranchManagerTest, CannotDeleteDefaultBranch) {
    bool success = branch_manager_->deleteBranch("main", true);
    EXPECT_FALSE(success);
    EXPECT_TRUE(branch_manager_->branchExists("main"));
}

// Test: Cannot delete active branch
TEST_F(BranchManagerTest, CannotDeleteActiveBranch) {
    branch_manager_->createBranch("feature/test", "main", "Test");
    branch_manager_->switchBranch("feature/test");
    
    bool success = branch_manager_->deleteBranch("feature/test", true);
    EXPECT_FALSE(success);
    EXPECT_TRUE(branch_manager_->branchExists("feature/test"));
}

// Test: Delete non-existent branch fails
TEST_F(BranchManagerTest, DeleteNonExistentBranchFails) {
    bool success = branch_manager_->deleteBranch("nonexistent", true);
    EXPECT_FALSE(success);
}

// Test: Get statistics
TEST_F(BranchManagerTest, GetStats) {
    branch_manager_->createBranch("feature/test1", "main", "Test 1");
    branch_manager_->createBranch("feature/test2", "main", "Test 2");
    
    auto stats = branch_manager_->getStats();
    
    EXPECT_EQ(stats.total_branches, 3);  // main + 2 created
    EXPECT_EQ(stats.default_branch, "main");
    EXPECT_GT(stats.newest_creation_timestamp_ms, 0);
}

// Test: Get sequence for branch
TEST_F(BranchManagerTest, GetSequenceForBranch) {
    recordEvents(10);
    
    auto branch = branch_manager_->createBranch("feature/test", "main", "Test");
    ASSERT_TRUE(branch.has_value());
    
    auto seq = branch_manager_->getSequenceForBranch("feature/test");
    ASSERT_TRUE(seq.has_value());
    EXPECT_EQ(seq.value(), branch->creation_sequence);
}

// Test: Get timestamp for branch
TEST_F(BranchManagerTest, GetTimestampForBranch) {
    auto branch = branch_manager_->createBranch("feature/test", "main", "Test");
    ASSERT_TRUE(branch.has_value());
    
    auto ts = branch_manager_->getTimestampForBranch("feature/test");
    ASSERT_TRUE(ts.has_value());
    EXPECT_EQ(ts.value(), branch->creation_timestamp_ms);
}

// Test: Create branch from tag
TEST_F(BranchManagerTest, CreateBranchFromTag) {
    recordEvents(10);
    
    // Create a tag
    auto tag = snapshot_manager_->createTag("v1.0", "Version 1.0", "system");
    ASSERT_TRUE(tag.has_value());
    
    // Create branch from tag
    BranchManager::CreateBranchOptions options;
    options.from_tag = "v1.0";
    
    auto branch = branch_manager_->createBranch(
        "release/v1_0",
        "main",
        "Release branch for v1.0",
        "system",
        options
    );
    
    ASSERT_TRUE(branch.has_value());
    EXPECT_EQ(branch->creation_sequence, tag->sequence_number);
}

// Test: Create branch from sequence
TEST_F(BranchManagerTest, CreateBranchFromSequence) {
    recordEvents(10);
    uint64_t target_seq = 5;
    
    BranchManager::CreateBranchOptions options;
    options.from_sequence = target_seq;
    
    auto branch = branch_manager_->createBranch(
        "feature/from-seq",
        "main",
        "Branch from sequence",
        "system",
        options
    );
    
    ASSERT_TRUE(branch.has_value());
    EXPECT_EQ(branch->creation_sequence, target_seq);
}

// Test: Create and set active branch
TEST_F(BranchManagerTest, CreateAndSetActiveBranch) {
    BranchManager::CreateBranchOptions options;
    options.set_active = true;
    
    auto branch = branch_manager_->createBranch(
        "feature/active",
        "main",
        "Active branch",
        "system",
        options
    );
    
    ASSERT_TRUE(branch.has_value());
    EXPECT_EQ(branch_manager_->getActiveBranch(), "feature/active");
}

// Test: Branch name validation
TEST_F(BranchManagerTest, BranchNameValidation) {
    // Valid names
    EXPECT_TRUE(BranchManager::isValidBranchName("main"));
    EXPECT_TRUE(BranchManager::isValidBranchName("feature/test"));
    EXPECT_TRUE(BranchManager::isValidBranchName("bugfix/issue-123"));
    EXPECT_TRUE(BranchManager::isValidBranchName("release_v1_0"));
    
    // Invalid names
    EXPECT_FALSE(BranchManager::isValidBranchName(""));
    EXPECT_FALSE(BranchManager::isValidBranchName("HEAD"));
    EXPECT_FALSE(BranchManager::isValidBranchName("FETCH_HEAD"));
    EXPECT_FALSE(BranchManager::isValidBranchName("feature@test"));
    EXPECT_FALSE(BranchManager::isValidBranchName("feature test"));
    EXPECT_FALSE(BranchManager::isValidBranchName("feature#test"));
    EXPECT_FALSE(BranchManager::isValidBranchName("release_v1.0"));
}

// Test: Merge branches (fast-forward)
TEST_F(BranchManagerTest, MergeBranchesFastForward) {
    recordEvents(5);
    
    // Create source branch
    auto source = branch_manager_->createBranch(
        "feature/source",
        "main",
        "Source branch"
    );
    ASSERT_TRUE(source.has_value());
    
    recordEvents(5);
    
    // Create target branch (earlier sequence)
    BranchManager::CreateBranchOptions options;
    options.from_sequence = source->creation_sequence - 3;
    
    auto target = branch_manager_->createBranch(
        "feature/target",
        "main",
        "Target branch",
        "system",
        options
    );
    ASSERT_TRUE(target.has_value());
    
    // Merge (fast-forward)
    auto result = branch_manager_->mergeBranches(
        "feature/source",
        "feature/target"
    );
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.conflicts.empty());
}

// Test: JSON serialization and deserialization
TEST_F(BranchManagerTest, JsonSerialization) {
    auto branch = branch_manager_->createBranch(
        "feature/test",
        "main",
        "Test branch",
        "test_user"
    );
    ASSERT_TRUE(branch.has_value());
    
    // Serialize to JSON
    auto json_obj = branch->toJson();
    
    // Check fields
    EXPECT_EQ(json_obj["branch_name"], "feature/test");
    EXPECT_EQ(json_obj["parent_branch"], "main");
    EXPECT_EQ(json_obj["description"], "Test branch");
    EXPECT_EQ(json_obj["created_by"], "test_user");
    
    // Deserialize from JSON
    auto deserialized = BranchManager::Branch::fromJson(json_obj);
    EXPECT_EQ(deserialized.branch_name, branch->branch_name);
    EXPECT_EQ(deserialized.parent_branch, branch->parent_branch);
    EXPECT_EQ(deserialized.creation_sequence, branch->creation_sequence);
}

// Test: Persistence across restart
TEST_F(BranchManagerTest, PersistenceAcrossRestart) {
    // Create branches
    branch_manager_->createBranch("feature/test1", "main", "Test 1");
    branch_manager_->createBranch("feature/test2", "main", "Test 2");
    branch_manager_->switchBranch("feature/test1");
    
    // Destroy and recreate manager
    branch_manager_.reset();
    branch_manager_ = std::make_unique<BranchManager>(*db_, *changefeed_, *snapshot_manager_);
    
    // Check branches still exist
    EXPECT_TRUE(branch_manager_->branchExists("feature/test1"));
    EXPECT_TRUE(branch_manager_->branchExists("feature/test2"));
    
    // Check active branch persisted
    EXPECT_EQ(branch_manager_->getActiveBranch(), "feature/test1");
}

// ---- Phase 5: Branch History tests ----

TEST_F(BranchManagerTest, BranchHistoryRecordedOnCreate) {
    branch_manager_->createBranch("hist-branch", "main", "History test", "admin");

    auto history = branch_manager_->getBranchHistory("hist-branch");
    ASSERT_FALSE(history.empty());
    EXPECT_EQ(history[0].event_type, "created");
    EXPECT_EQ(history[0].branch_name, "hist-branch");
    EXPECT_EQ(history[0].performed_by, "admin");
    EXPECT_GT(history[0].timestamp_ms, 0);
}

TEST_F(BranchManagerTest, BranchHistoryRecordedOnSwitch) {
    branch_manager_->createBranch("switch-branch", "main", "Switch test", "admin");
    branch_manager_->switchBranch("switch-branch");

    auto history = branch_manager_->getBranchHistory("switch-branch");
    // Should have at least "created" + "switched_to"
    EXPECT_GE(history.size(), 2u);

    bool found_switch = false;
    for (const auto& e : history) {
        if (e.event_type == "switched_to") { found_switch = true; break; }
    }
    EXPECT_TRUE(found_switch);
}

TEST_F(BranchManagerTest, BranchHistoryEmptyForUnknownBranch) {
    auto history = branch_manager_->getBranchHistory("does_not_exist");
    EXPECT_TRUE(history.empty());
}

TEST_F(BranchManagerTest, BranchHistoryLimitRespected) {
    branch_manager_->createBranch("limit-branch", "main", "Limit test", "admin");
    branch_manager_->switchBranch("limit-branch");
    branch_manager_->switchBranch("main");
    branch_manager_->switchBranch("limit-branch");

    // Requesting only 1 entry
    auto history = branch_manager_->getBranchHistory("limit-branch", 1);
    EXPECT_EQ(history.size(), 1u);
}

TEST_F(BranchManagerTest, BranchHistoryEntryJsonRoundtrip) {
    BranchManager::BranchHistoryEntry e;
    e.event_type   = "created";
    e.branch_name  = "rj-branch";
    e.details      = "detail";
    e.performed_by = "user1";
    e.timestamp_ms = 123456789LL;
    e.sequence     = 42;

    auto j = e.toJson();
    auto e2 = BranchManager::BranchHistoryEntry::fromJson(j);

    EXPECT_EQ(e2.event_type,   e.event_type);
    EXPECT_EQ(e2.branch_name,  e.branch_name);
    EXPECT_EQ(e2.details,      e.details);
    EXPECT_EQ(e2.performed_by, e.performed_by);
    EXPECT_EQ(e2.timestamp_ms, e.timestamp_ms);
    EXPECT_EQ(e2.sequence,     e.sequence);
}

// ---- Phase 5: Branch GC tests ----

TEST_F(BranchManagerTest, BranchGCPrunedByAge) {
    // Create a branch; set max_age_ms = 0 (disabled) first then check
    branch_manager_->createBranch("gc-branch", "main", "GC test", "admin");

    BranchManager::BranchGCPolicy pol;
    pol.max_age_ms   = 1;       // prune branches older than 1 ms
    pol.only_merged  = false;   // prune even unmerged
    pol.protect_default = true;
    branch_manager_->setBranchGCPolicy(pol);

    // Sleep a bit so the branch is older than 1 ms
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    size_t pruned = branch_manager_->pruneMergedBranches();
    EXPECT_GE(pruned, 1u);
    EXPECT_FALSE(branch_manager_->branchExists("gc-branch"));
}

TEST_F(BranchManagerTest, BranchGCDoesNotPruneDefaultBranch) {
    BranchManager::BranchGCPolicy pol;
    pol.max_age_ms      = 1;
    pol.only_merged     = false;
    pol.protect_default = true;
    branch_manager_->setBranchGCPolicy(pol);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    branch_manager_->pruneMergedBranches();

    // Default branch must survive
    EXPECT_TRUE(branch_manager_->branchExists("main"));
}

TEST_F(BranchManagerTest, BranchGCDoesNotPruneActiveBranch) {
    branch_manager_->createBranch("active-gc", "main", "Active GC", "admin");
    branch_manager_->switchBranch("active-gc");

    BranchManager::BranchGCPolicy pol;
    pol.max_age_ms  = 1;
    pol.only_merged = false;
    branch_manager_->setBranchGCPolicy(pol);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    branch_manager_->pruneMergedBranches();

    // Active branch must NOT be pruned
    EXPECT_TRUE(branch_manager_->branchExists("active-gc"));

    // Switch back so teardown can clean up
    branch_manager_->switchBranch("main");
}

} // namespace transaction
} // namespace themis
