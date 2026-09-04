/**
 * @file test_prompt_version_control.cpp
 * @brief Unit tests for PromptVersionControl
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_version_control.h"

using namespace themis::prompt_engineering;

class PromptVersionControlTest : public ::testing::Test {
protected:
    void SetUp() override {
        vc_ = std::make_unique<PromptVersionControl>();
    }
    
    std::unique_ptr<PromptVersionControl> vc_;
};

TEST_F(PromptVersionControlTest, CommitAndRetrieve) {
    std::string prompt_id = "test_prompt_1";
    std::string content = "Prompt content version 1";
    std::string message = "Initial commit";
    
    // Commit a version
    std::string version_id = vc_->commit(prompt_id, content, message);
    
    EXPECT_FALSE(version_id.empty());
    EXPECT_EQ(version_id.length(), 32);  // SHA-like hash
    
    // Retrieve the version
    auto version = vc_->getVersion(version_id);
    
    ASSERT_TRUE(version.has_value());
    EXPECT_EQ(version->version_id, version_id);
    EXPECT_EQ(version->prompt_id, prompt_id);
    EXPECT_EQ(version->content, content);
    EXPECT_EQ(version->commit_message, message);
    EXPECT_EQ(version->branch, "main");  // Default branch
    EXPECT_EQ(version->author, "system");  // Default author
    EXPECT_TRUE(version->parent_version.empty());  // First commit
}

TEST_F(PromptVersionControlTest, MultipleCommits) {
    std::string prompt_id = "test_prompt_2";
    
    // First commit
    std::string v1 = vc_->commit(prompt_id, "Content v1", "First");
    EXPECT_FALSE(v1.empty());
    
    // Second commit
    std::string v2 = vc_->commit(prompt_id, "Content v2", "Second");
    EXPECT_FALSE(v2.empty());
    EXPECT_NE(v1, v2);
    
    // Third commit
    std::string v3 = vc_->commit(prompt_id, "Content v3", "Third");
    EXPECT_FALSE(v3.empty());
    EXPECT_NE(v2, v3);
    
    // Check parent relationships
    auto version2 = vc_->getVersion(v2);
    ASSERT_TRUE(version2.has_value());
    EXPECT_EQ(version2->parent_version, v1);
    
    auto version3 = vc_->getVersion(v3);
    ASSERT_TRUE(version3.has_value());
    EXPECT_EQ(version3->parent_version, v2);
}

TEST_F(PromptVersionControlTest, GetHistory) {
    std::string prompt_id = "test_prompt_3";
    
    // Create several versions
    vc_->commit(prompt_id, "v1", "msg1");
    vc_->commit(prompt_id, "v2", "msg2");
    vc_->commit(prompt_id, "v3", "msg3");
    vc_->commit(prompt_id, "v4", "msg4");
    
    // Get full history
    auto history = vc_->getHistory(prompt_id);
    
    EXPECT_EQ(history.size(), 4);
    
    // Should be in reverse chronological order
    EXPECT_EQ(history[0].content, "v4");
    EXPECT_EQ(history[1].content, "v3");
    EXPECT_EQ(history[2].content, "v2");
    EXPECT_EQ(history[3].content, "v1");
}

TEST_F(PromptVersionControlTest, GetHistoryWithLimit) {
    std::string prompt_id = "test_prompt_4";
    
    vc_->commit(prompt_id, "v1", "msg1");
    vc_->commit(prompt_id, "v2", "msg2");
    vc_->commit(prompt_id, "v3", "msg3");
    
    // Get limited history
    auto history = vc_->getHistory(prompt_id, "", 2);
    
    EXPECT_EQ(history.size(), 2);
    EXPECT_EQ(history[0].content, "v3");
    EXPECT_EQ(history[1].content, "v2");
}

TEST_F(PromptVersionControlTest, GetLatest) {
    std::string prompt_id = "test_prompt_5";
    
    vc_->commit(prompt_id, "v1", "msg1");
    vc_->commit(prompt_id, "v2", "msg2");
    std::string v3 = vc_->commit(prompt_id, "v3", "msg3");
    
    auto latest = vc_->getLatest(prompt_id, "main");
    
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->version_id, v3);
    EXPECT_EQ(latest->content, "v3");
}

TEST_F(PromptVersionControlTest, Rollback) {
    std::string prompt_id = "test_prompt_6";
    
    std::string v1 = vc_->commit(prompt_id, "Good version", "v1");
    vc_->commit(prompt_id, "Bad version", "v2");
    vc_->commit(prompt_id, "Another bad", "v3");
    
    // Rollback to v1
    std::string rollback_id = vc_->rollback(prompt_id, v1, "Rollback to good version");
    
    EXPECT_FALSE(rollback_id.empty());
    
    // Check that latest now has v1 content
    auto latest = vc_->getLatest(prompt_id);
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->content, "Good version");
    EXPECT_TRUE(latest->commit_message.find("Rollback") != std::string::npos);
}

TEST_F(PromptVersionControlTest, RollbackN) {
    std::string prompt_id = "test_prompt_7";
    
    std::string v1 = vc_->commit(prompt_id, "v1", "msg1");
    vc_->commit(prompt_id, "v2", "msg2");
    vc_->commit(prompt_id, "v3", "msg3");
    vc_->commit(prompt_id, "v4", "msg4");
    
    // Rollback 2 versions
    std::string rollback_id = vc_->rollbackN(prompt_id, 2);
    
    EXPECT_FALSE(rollback_id.empty());
    
    // Should be at v2 content
    auto latest = vc_->getLatest(prompt_id);
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->content, "v2");
}

TEST_F(PromptVersionControlTest, CreateBranch) {
    std::string prompt_id = "test_prompt_8";
    
    // Create main branch versions
    vc_->commit(prompt_id, "main v1", "msg1");
    std::string v2 = vc_->commit(prompt_id, "main v2", "msg2");
    
    // Create experimental branch
    bool created = vc_->createBranch(prompt_id, "experimental", v2);
    
    EXPECT_TRUE(created);
    
    // Verify branch exists
    auto branches = vc_->listBranches(prompt_id);
    EXPECT_EQ(branches.size(), 2);  // main + experimental
    
    bool found_exp = false;
    for (const auto& branch : branches) {
        if (branch.name == "experimental") {
            found_exp = true;
            EXPECT_EQ(branch.head_version, v2);
        }
    }
    EXPECT_TRUE(found_exp);
}

TEST_F(PromptVersionControlTest, CommitToBranch) {
    std::string prompt_id = "test_prompt_9";
    
    // Main branch
    vc_->commit(prompt_id, "main v1", "msg1");
    std::string v2 = vc_->commit(prompt_id, "main v2", "msg2");
    
    // Create and commit to experimental branch
    vc_->createBranch(prompt_id, "experimental", v2);
    std::string exp_v1 = vc_->commit(prompt_id, "exp v1", "exp commit", "system", "experimental");
    
    // Verify both branches have different content
    auto main_latest = vc_->getLatest(prompt_id, "main");
    auto exp_latest = vc_->getLatest(prompt_id, "experimental");
    
    ASSERT_TRUE(main_latest.has_value());
    ASSERT_TRUE(exp_latest.has_value());
    
    EXPECT_EQ(main_latest->content, "main v2");
    EXPECT_EQ(exp_latest->content, "exp v1");
    EXPECT_NE(main_latest->version_id, exp_latest->version_id);
}

TEST_F(PromptVersionControlTest, Diff) {
    std::string prompt_id = "test_prompt_10";
    
    std::string v1 = vc_->commit(prompt_id, "Line 1\nLine 2\nLine 3", "v1");
    std::string v2 = vc_->commit(prompt_id, "Line 1\nLine 2 modified\nLine 3\nLine 4", "v2");
    
    auto diff = vc_->diff(v1, v2);
    
    EXPECT_EQ(diff.version_a, v1);
    EXPECT_EQ(diff.version_b, v2);
    EXPECT_GT(diff.additions, 0);
    EXPECT_FALSE(diff.unified_diff.empty());
}

TEST_F(PromptVersionControlTest, MergeSimple) {
    std::string prompt_id = "test_prompt_11";
    
    // Main branch
    std::string v1 = vc_->commit(prompt_id, "base content", "base");
    
    // Create branch and add changes
    vc_->createBranch(prompt_id, "feature", v1);
    vc_->commit(prompt_id, "feature content", "feature work", "system", "feature");
    
    // Merge feature into main
    auto result = vc_->merge(prompt_id, "feature", "main", "theirs", "Merge feature");
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.merged_version_id.empty());
    EXPECT_EQ(result.merged_content, "feature content");
}

TEST_F(PromptVersionControlTest, GetGenealogy) {
    std::string prompt_id = "test_prompt_12";
    
    std::string v1 = vc_->commit(prompt_id, "v1", "msg1");
    std::string v2 = vc_->commit(prompt_id, "v2", "msg2");
    std::string v3 = vc_->commit(prompt_id, "v3", "msg3");
    
    auto genealogy = vc_->getGenealogy(prompt_id);
    
    EXPECT_EQ(genealogy.size(), 3);
    EXPECT_TRUE(genealogy[v1].empty());  // v1 has no parent
    EXPECT_EQ(genealogy[v2], v1);
    EXPECT_EQ(genealogy[v3], v2);
}

TEST_F(PromptVersionControlTest, Tagging) {
    std::string prompt_id = "test_prompt_13";
    
    std::string v1 = vc_->commit(prompt_id, "v1", "msg1");
    std::string v2 = vc_->commit(prompt_id, "v2", "msg2");
    
    // Tag versions
    bool tagged1 = vc_->tag(v1, "v1.0");
    bool tagged2 = vc_->tag(v2, "production");
    
    EXPECT_TRUE(tagged1);
    EXPECT_TRUE(tagged2);
    
    // Retrieve by tag
    auto tagged_version = vc_->getByTag(prompt_id, "production");
    ASSERT_TRUE(tagged_version.has_value());
    EXPECT_EQ(tagged_version->version_id, v2);
    
    // List tags
    auto tags = vc_->listTags(prompt_id);
    EXPECT_EQ(tags.size(), 2);
    EXPECT_EQ(tags["v1.0"], v1);
    EXPECT_EQ(tags["production"], v2);
}

TEST_F(PromptVersionControlTest, PerformanceScore) {
    std::string prompt_id = "test_prompt_14";
    
    std::string v1 = vc_->commit(prompt_id, "content", "msg");
    
    // Update performance score
    vc_->updatePerformanceScore(v1, 0.85);
    
    auto version = vc_->getVersion(v1);
    ASSERT_TRUE(version.has_value());
    EXPECT_DOUBLE_EQ(version->performance_score, 0.85);
}

TEST_F(PromptVersionControlTest, Stats) {
    std::string prompt_id = "test_prompt_15";
    
    vc_->commit(prompt_id, "v1", "msg1");
    vc_->commit(prompt_id, "v2", "msg2");
    vc_->createBranch(prompt_id, "exp", "");
    vc_->commit(prompt_id, "v3", "msg3", "system", "exp");
    
    std::string v2 = vc_->getLatest(prompt_id, "main")->version_id;
    vc_->tag(v2, "stable");
    
    auto stats = vc_->getStats(prompt_id);
    
    EXPECT_EQ(stats["prompt_id"], prompt_id);
    EXPECT_EQ(stats["total_versions"].get<size_t>(), 3);
    EXPECT_EQ(stats["branch_count"].get<size_t>(), 2);
    EXPECT_EQ(stats["tag_count"].get<size_t>(), 1);
}

TEST_F(PromptVersionControlTest, VersionSerialization) {
    PromptVersion version;
    version.version_id = "abc123";
    version.prompt_id = "prompt1";
    version.branch = "main";
    version.content = "test content";
    version.commit_message = "test message";
    version.author = "tester";
    version.parent_version = "parent123";
    version.performance_score = 0.9;
    version.metadata = {{"key", "value"}};
    version.timestamp = std::chrono::system_clock::now();
    
    // Serialize
    auto json = version.toJson();
    
    EXPECT_EQ(json["version_id"], "abc123");
    EXPECT_EQ(json["content"], "test content");
    EXPECT_EQ(json["performance_score"], 0.9);
    
    // Deserialize
    auto reconstructed = PromptVersion::fromJson(json);
    
    EXPECT_EQ(reconstructed.version_id, version.version_id);
    EXPECT_EQ(reconstructed.content, version.content);
    EXPECT_DOUBLE_EQ(reconstructed.performance_score, version.performance_score);
}

TEST_F(PromptVersionControlTest, ListBranches) {
    std::string prompt_id = "test_prompt_16";
    
    vc_->commit(prompt_id, "v1", "msg1");
    vc_->commit(prompt_id, "v2", "msg2");
    
    vc_->createBranch(prompt_id, "dev", "");
    vc_->createBranch(prompt_id, "staging", "");
    
    auto branches = vc_->listBranches(prompt_id);
    
    EXPECT_EQ(branches.size(), 3);  // main, dev, staging
    
    // Verify all branches are listed
    std::vector<std::string> names = {};

    for (const auto& branch : branches) {
        names.push_back(branch.name);
        EXPECT_GT(branch.commit_count, 0);
    }
    
    EXPECT_TRUE(std::find(names.begin(), names.end(), "main") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "dev") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "staging") != names.end());
}
