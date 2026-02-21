/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_policy_versioning.cpp                         ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:53:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     502                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "governance/policy_version_history.h"
#include "governance/policy_manager_versioned.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis::governance;

class PolicyVersionHistoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        history = std::make_unique<PolicyVersionHistory>();
        test_dir = std::filesystem::temp_directory_path() / "themis_version_test";
        std::filesystem::create_directories(test_dir);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }
    
    PolicyRule createTestRule(const std::string& id, const std::string& name) {
        PolicyRule rule;
        rule.id = id;
        rule.name = name;
        rule.description = "Test rule";
        rule.resources = {"data/*"};
        rule.actions = {"read"};
        rule.enabled = true;
        return rule;
    }
    
    std::unique_ptr<PolicyVersionHistory> history;
    std::filesystem::path test_dir;
};

// ========== PolicyVersionHistory Tests ==========

TEST_F(PolicyVersionHistoryTest, RecordVersion) {
    auto rule = createTestRule("rule_001", "Test Rule");
    
    std::string version = history->recordVersion(
        rule.id,
        rule,
        "user1",
        "Initial version"
    );
    
    EXPECT_EQ(version, "0.0.1");
}

TEST_F(PolicyVersionHistoryTest, RecordMultipleVersions) {
    auto rule = createTestRule("rule_002", "Test Rule");
    
    std::string v1 = history->recordVersion(rule.id, rule, "user1", "Version 1");
    EXPECT_EQ(v1, "0.0.1");
    
    rule.name = "Updated Rule";
    std::string v2 = history->recordVersion(rule.id, rule, "user1", "Version 2");
    EXPECT_EQ(v2, "0.0.2");
    
    rule.description = "Updated description";
    std::string v3 = history->recordVersion(rule.id, rule, "user2", "Version 3");
    EXPECT_EQ(v3, "0.0.3");
}

TEST_F(PolicyVersionHistoryTest, GetVersions) {
    auto rule = createTestRule("rule_003", "Test Rule");
    
    history->recordVersion(rule.id, rule, "user1", "V1");
    rule.name = "Updated 1";
    history->recordVersion(rule.id, rule, "user1", "V2");
    rule.name = "Updated 2";
    history->recordVersion(rule.id, rule, "user1", "V3");
    
    auto versions = history->getVersions(rule.id);
    EXPECT_EQ(versions.size(), 3);
    
    // Should be newest first
    EXPECT_EQ(versions[0].version, "0.0.3");
    EXPECT_EQ(versions[1].version, "0.0.2");
    EXPECT_EQ(versions[2].version, "0.0.1");
}

TEST_F(PolicyVersionHistoryTest, GetSpecificVersion) {
    auto rule = createTestRule("rule_004", "Test Rule");
    
    history->recordVersion(rule.id, rule, "user1", "V1");
    rule.name = "Updated Name";
    history->recordVersion(rule.id, rule, "user1", "V2");
    
    auto v1 = history->getVersion(rule.id, "0.0.1");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->rule_id, "rule_004");
    EXPECT_EQ(v1->version, "0.0.1");
    EXPECT_EQ(v1->change_description, "V1");
    
    auto v2 = history->getVersion(rule.id, "0.0.2");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->rule_id, "rule_004");
    EXPECT_EQ(v2->version, "0.0.2");
    EXPECT_EQ(v2->change_description, "V2");
}

TEST_F(PolicyVersionHistoryTest, GetNonexistentVersion) {
    auto rule = createTestRule("rule_005", "Test Rule");
    history->recordVersion(rule.id, rule, "user1", "V1");
    
    auto v = history->getVersion(rule.id, "9.9.9");
    EXPECT_FALSE(v.has_value());
}

TEST_F(PolicyVersionHistoryTest, GetLatestVersion) {
    auto rule = createTestRule("rule_006", "Test Rule");
    
    // No versions yet
    EXPECT_EQ(history->getLatestVersion(rule.id), "0.0.0");
    
    history->recordVersion(rule.id, rule, "user1", "V1");
    EXPECT_EQ(history->getLatestVersion(rule.id), "0.0.1");
    
    history->recordVersion(rule.id, rule, "user1", "V2");
    EXPECT_EQ(history->getLatestVersion(rule.id), "0.0.2");
}

TEST_F(PolicyVersionHistoryTest, GetPreviousVersion) {
    auto rule = createTestRule("rule_007", "Test Rule");
    
    // No versions yet
    EXPECT_FALSE(history->getPreviousVersion(rule.id).has_value());
    
    // Only one version
    history->recordVersion(rule.id, rule, "user1", "V1");
    EXPECT_FALSE(history->getPreviousVersion(rule.id).has_value());
    
    // Two versions
    history->recordVersion(rule.id, rule, "user1", "V2");
    auto prev = history->getPreviousVersion(rule.id);
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(*prev, "0.0.1");
    
    // Three versions
    history->recordVersion(rule.id, rule, "user1", "V3");
    prev = history->getPreviousVersion(rule.id);
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(*prev, "0.0.2");
}

TEST_F(PolicyVersionHistoryTest, CompareVersions) {
    auto rule = createTestRule("rule_008", "Original Name");
    rule.priority = 10;
    history->recordVersion(rule.id, rule, "user1", "V1");
    
    rule.name = "Updated Name";
    rule.priority = 20;
    rule.enabled = false;
    history->recordVersion(rule.id, rule, "user2", "V2");
    
    auto diff = history->compareVersions(rule.id, "0.0.1", "0.0.2");
    
    EXPECT_EQ(diff.rule_id, rule.id);
    EXPECT_EQ(diff.version1, "0.0.1");
    EXPECT_EQ(diff.version2, "0.0.2");
    
    // Should identify changed fields
    EXPECT_TRUE(std::find(diff.changes.begin(), diff.changes.end(), "name") != diff.changes.end());
    EXPECT_TRUE(std::find(diff.changes.begin(), diff.changes.end(), "priority") != diff.changes.end());
    EXPECT_TRUE(std::find(diff.changes.begin(), diff.changes.end(), "enabled") != diff.changes.end());
}

TEST_F(PolicyVersionHistoryTest, RecordAudit) {
    AuditLogEntry entry;
    entry.rule_id = "rule_009";
    entry.operation = "create";
    entry.user = "admin";
    entry.timestamp = 1234567890;
    entry.new_version = "0.0.1";
    
    history->recordAudit(entry);
    
    auto logs = history->queryAudit();
    EXPECT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0].rule_id, "rule_009");
    EXPECT_EQ(logs[0].operation, "create");
    EXPECT_EQ(logs[0].user, "admin");
}

TEST_F(PolicyVersionHistoryTest, QueryAuditByRuleId) {
    AuditLogEntry entry1;
    entry1.rule_id = "rule_010";
    entry1.operation = "create";
    entry1.user = "user1";
    entry1.timestamp = 1000;
    
    AuditLogEntry entry2;
    entry2.rule_id = "rule_011";
    entry2.operation = "update";
    entry2.user = "user2";
    entry2.timestamp = 2000;
    
    history->recordAudit(entry1);
    history->recordAudit(entry2);
    
    auto logs = history->queryAudit("rule_010");
    EXPECT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0].rule_id, "rule_010");
}

TEST_F(PolicyVersionHistoryTest, QueryAuditByUser) {
    AuditLogEntry entry1;
    entry1.rule_id = "rule_012";
    entry1.operation = "create";
    entry1.user = "alice";
    entry1.timestamp = 1000;
    
    AuditLogEntry entry2;
    entry2.rule_id = "rule_013";
    entry2.operation = "update";
    entry2.user = "bob";
    entry2.timestamp = 2000;
    
    AuditLogEntry entry3;
    entry3.rule_id = "rule_014";
    entry3.operation = "delete";
    entry3.user = "alice";
    entry3.timestamp = 3000;
    
    history->recordAudit(entry1);
    history->recordAudit(entry2);
    history->recordAudit(entry3);
    
    auto logs = history->queryAudit(std::nullopt, "alice");
    EXPECT_EQ(logs.size(), 2);
    EXPECT_EQ(logs[0].user, "alice");
    EXPECT_EQ(logs[1].user, "alice");
}

TEST_F(PolicyVersionHistoryTest, QueryAuditByTimeRange) {
    AuditLogEntry entry1;
    entry1.rule_id = "rule_015";
    entry1.user = "user1";
    entry1.timestamp = 1000;
    
    AuditLogEntry entry2;
    entry2.rule_id = "rule_016";
    entry2.user = "user1";
    entry2.timestamp = 2000;
    
    AuditLogEntry entry3;
    entry3.rule_id = "rule_017";
    entry3.user = "user1";
    entry3.timestamp = 3000;
    
    history->recordAudit(entry1);
    history->recordAudit(entry2);
    history->recordAudit(entry3);
    
    auto logs = history->queryAudit(std::nullopt, std::nullopt, 1500, 2500);
    EXPECT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0].timestamp, 2000);
}

TEST_F(PolicyVersionHistoryTest, DeleteVersionHistory) {
    auto rule = createTestRule("rule_018", "Test Rule");
    
    history->recordVersion(rule.id, rule, "user1", "V1");
    history->recordVersion(rule.id, rule, "user1", "V2");
    
    EXPECT_EQ(history->getVersions(rule.id).size(), 2);
    
    history->deleteVersionHistory(rule.id);
    
    EXPECT_EQ(history->getVersions(rule.id).size(), 0);
}

TEST_F(PolicyVersionHistoryTest, SaveAndLoadFromFile) {
    auto rule = createTestRule("rule_019", "Test Rule");
    
    history->recordVersion(rule.id, rule, "user1", "V1");
    rule.name = "Updated";
    history->recordVersion(rule.id, rule, "user2", "V2");
    
    AuditLogEntry audit;
    audit.rule_id = rule.id;
    audit.operation = "update";
    audit.user = "user2";
    audit.timestamp = 12345;
    history->recordAudit(audit);
    
    // Save to file
    auto file_path = test_dir / "version_history.json";
    ASSERT_TRUE(history->saveToFile(file_path.string()));
    
    // Load into new history
    auto new_history = std::make_unique<PolicyVersionHistory>();
    ASSERT_TRUE(new_history->loadFromFile(file_path.string()));
    
    // Verify versions
    auto versions = new_history->getVersions(rule.id);
    EXPECT_EQ(versions.size(), 2);
    
    // Verify audit log
    auto logs = new_history->queryAudit();
    EXPECT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0].operation, "update");
}

// ========== PolicyManagerWithVersioning Tests ==========

class PolicyManagerVersionedTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<PolicyManagerWithVersioning>();
    }
    
    PolicyRule createTestRule(const std::string& id, const std::string& name) {
        PolicyRule rule;
        rule.id = id;
        rule.name = name;
        rule.description = "Test rule";
        rule.resources = {"data/*"};
        rule.actions = {"read"};
        rule.enabled = true;
        return rule;
    }
    
    std::unique_ptr<PolicyManagerWithVersioning> manager;
};

TEST_F(PolicyManagerVersionedTest, AddRuleVersioned) {
    auto rule = createTestRule("rule_v01", "Test Rule");
    
    std::string version = manager->addRuleVersioned(rule, "user1", "Initial creation");
    
    EXPECT_EQ(version, "1.0.0");
    
    // Verify rule was added to policy manager
    auto retrieved = manager->getPolicyManager()->getRule(rule.id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Test Rule");
    EXPECT_EQ(retrieved->version, "1.0.0");
    
    // Verify version was recorded
    auto versions = manager->getRuleVersions(rule.id);
    EXPECT_EQ(versions.size(), 1);
}

TEST_F(PolicyManagerVersionedTest, UpdateRuleVersioned) {
    auto rule = createTestRule("rule_v02", "Original");
    
    manager->addRuleVersioned(rule, "user1", "Initial");
    
    rule.name = "Updated";
    std::string v2 = manager->updateRuleVersioned(rule.id, rule, "user2", "Updated name");
    
    EXPECT_EQ(v2, "1.0.1");
    
    // Verify update
    auto retrieved = manager->getPolicyManager()->getRule(rule.id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Updated");
    EXPECT_EQ(retrieved->version, "1.0.1");
    
    // Verify versions
    auto versions = manager->getRuleVersions(rule.id);
    EXPECT_EQ(versions.size(), 2);
}

TEST_F(PolicyManagerVersionedTest, DeleteRuleVersioned) {
    auto rule = createTestRule("rule_v03", "Test Rule");
    
    manager->addRuleVersioned(rule, "user1", "Initial");
    ASSERT_TRUE(manager->getPolicyManager()->getRule(rule.id).has_value());
    
    manager->deleteRuleVersioned(rule.id, "admin");
    
    EXPECT_FALSE(manager->getPolicyManager()->getRule(rule.id).has_value());
    
    // Audit should still exist
    auto audit = manager->queryAudit(rule.id);
    EXPECT_GE(audit.size(), 2); // create + delete
}

TEST_F(PolicyManagerVersionedTest, RollbackToVersion) {
    auto rule = createTestRule("rule_v04", "Version 1");
    
    manager->addRuleVersioned(rule, "user1", "V1");
    
    rule.name = "Version 2";
    manager->updateRuleVersioned(rule.id, rule, "user1", "V2");
    
    rule.name = "Version 3";
    manager->updateRuleVersioned(rule.id, rule, "user1", "V3");
    
    // Rollback to version 1
    bool success = manager->rollbackToVersion(rule.id, "1.0.0", "admin");
    ASSERT_TRUE(success);
    
    auto current = manager->getPolicyManager()->getRule(rule.id);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Version 1");
}

TEST_F(PolicyManagerVersionedTest, RollbackToPreviousVersion) {
    auto rule = createTestRule("rule_v05", "Version 1");
    
    manager->addRuleVersioned(rule, "user1", "V1");
    
    rule.name = "Version 2";
    manager->updateRuleVersioned(rule.id, rule, "user1", "V2");
    
    // Rollback to previous
    bool success = manager->rollbackToPreviousVersion(rule.id, "admin");
    ASSERT_TRUE(success);
    
    auto current = manager->getPolicyManager()->getRule(rule.id);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Version 1");
}

TEST_F(PolicyManagerVersionedTest, PreviewRollback) {
    auto rule = createTestRule("rule_v06", "Original");
    rule.priority = 10;
    
    manager->addRuleVersioned(rule, "user1", "V1");
    
    rule.name = "Modified";
    rule.priority = 20;
    manager->updateRuleVersioned(rule.id, rule, "user1", "V2");
    
    auto diff = manager->previewRollback(rule.id, "1.0.0");
    
    EXPECT_EQ(diff.rule_id, rule.id);
    EXPECT_TRUE(std::find(diff.changes.begin(), diff.changes.end(), "name") != diff.changes.end());
    EXPECT_TRUE(std::find(diff.changes.begin(), diff.changes.end(), "priority") != diff.changes.end());
}

TEST_F(PolicyManagerVersionedTest, QueryAuditTrail) {
    auto rule = createTestRule("rule_v07", "Test");
    
    manager->addRuleVersioned(rule, "alice", "Created");
    rule.name = "Updated";
    manager->updateRuleVersioned(rule.id, rule, "bob", "Updated");
    
    // Query all audit entries
    auto all_audit = manager->queryAudit();
    EXPECT_GE(all_audit.size(), 2);
    
    // Query by user
    auto alice_audit = manager->queryAudit(std::nullopt, "alice");
    EXPECT_GE(alice_audit.size(), 1);
    EXPECT_EQ(alice_audit[0].user, "alice");
    
    // Query by rule
    auto rule_audit = manager->queryAudit(rule.id);
    EXPECT_GE(rule_audit.size(), 2);
}

TEST_F(PolicyManagerVersionedTest, CompareVersions) {
    auto rule = createTestRule("rule_v08", "V1");
    rule.priority = 10;
    
    manager->addRuleVersioned(rule, "user1", "Version 1");
    
    rule.name = "V2";
    rule.priority = 20;
    manager->updateRuleVersioned(rule.id, rule, "user1", "Version 2");
    
    auto diff = manager->compareVersions(rule.id, "1.0.0", "0.0.1");
    
    EXPECT_EQ(diff.version1, "1.0.0");
    EXPECT_EQ(diff.version2, "0.0.1");
    EXPECT_GE(diff.changes.size(), 2);
}

// Run all tests