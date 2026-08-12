#include <gtest/gtest.h>
#include "governance/policy_version_history.h"
#include "governance/policy_manager_versioned.h"
#include "server/policy_versioning_api_handler.h"
#include <boost/beast/http.hpp>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis::governance;
namespace http = boost::beast::http;

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
    
    EXPECT_EQ(version, "1.0.0");
}

TEST_F(PolicyVersionHistoryTest, RecordMultipleVersions) {
    auto rule = createTestRule("rule_002", "Test Rule");
    
    std::string v1 = history->recordVersion(rule.id, rule, "user1", "Version 1");
    EXPECT_EQ(v1, "1.0.0");
    
    rule.name = "Updated Rule";
    std::string v2 = history->recordVersion(rule.id, rule, "user1", "Version 2");
    EXPECT_EQ(v2, "1.0.1");
    
    rule.description = "Updated description";
    std::string v3 = history->recordVersion(rule.id, rule, "user2", "Version 3");
    EXPECT_EQ(v3, "1.0.2");
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
    EXPECT_EQ(versions[0].version, "1.0.2");
    EXPECT_EQ(versions[1].version, "1.0.1");
    EXPECT_EQ(versions[2].version, "1.0.0");
}

TEST_F(PolicyVersionHistoryTest, GetSpecificVersion) {
    auto rule = createTestRule("rule_004", "Test Rule");
    
    history->recordVersion(rule.id, rule, "user1", "V1");
    rule.name = "Updated Name";
    history->recordVersion(rule.id, rule, "user1", "V2");
    
    auto v1 = history->getVersion(rule.id, "1.0.0");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->rule_id, "rule_004");
    EXPECT_EQ(v1->version, "1.0.0");
    EXPECT_EQ(v1->change_description, "V1");
    
    auto v2 = history->getVersion(rule.id, "1.0.1");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->rule_id, "rule_004");
    EXPECT_EQ(v2->version, "1.0.1");
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
    EXPECT_EQ(history->getLatestVersion(rule.id), "1.0.0");
    
    history->recordVersion(rule.id, rule, "user1", "V2");
    EXPECT_EQ(history->getLatestVersion(rule.id), "1.0.1");
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
    EXPECT_EQ(*prev, "1.0.0");
    
    // Three versions
    history->recordVersion(rule.id, rule, "user1", "V3");
    prev = history->getPreviousVersion(rule.id);
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(*prev, "1.0.1");
}

TEST_F(PolicyVersionHistoryTest, CompareVersions) {
    auto rule = createTestRule("rule_008", "Original Name");
    rule.priority = 10;
    history->recordVersion(rule.id, rule, "user1", "V1");
    
    rule.name = "Updated Name";
    rule.priority = 20;
    rule.enabled = false;
    history->recordVersion(rule.id, rule, "user2", "V2");
    
    auto diff = history->compareVersions(rule.id, "1.0.0", "1.0.1");
    
    EXPECT_EQ(diff.rule_id, rule.id);
    EXPECT_EQ(diff.version1, "1.0.0");
    EXPECT_EQ(diff.version2, "1.0.1");
    
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
    
    auto diff = manager->compareVersions(rule.id, "1.0.0", "1.0.1");
    
    EXPECT_EQ(diff.version1, "1.0.0");
    EXPECT_EQ(diff.version2, "1.0.1");
    EXPECT_GE(diff.changes.size(), 2);
}

// ========== Real-Time Conflict Detection Tests ==========

TEST_F(PolicyManagerVersionedTest, CheckConflictsForRule_EncryptionConflict) {
    // Add an existing rule that requires encryption
    auto existing = createTestRule("conflict_base", "Base Rule");
    existing.resources = {"sensitive/*"};
    existing.actions = {"read"};
    existing.require_encryption = true;
    manager->addRuleVersioned(existing, "admin", "baseline rule");

    // New rule with same resource/action but opposite encryption requirement
    auto new_rule = createTestRule("conflict_new", "New Rule");
    new_rule.resources = {"sensitive/*"};
    new_rule.actions = {"read"};
    new_rule.require_encryption = false;

    auto conflicts = manager->checkConflictsForRule(new_rule);

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].severity, "critical");
    ASSERT_FALSE(conflicts[0].conflicting_rule_ids.empty());
    EXPECT_EQ(conflicts[0].conflicting_rule_ids[0], "conflict_base");
    EXPECT_FALSE(conflicts[0].description.empty());
    EXPECT_FALSE(conflicts[0].resolution_suggestions.empty());
}

TEST_F(PolicyManagerVersionedTest, CheckConflictsForRule_ExportConflict) {
    auto existing = createTestRule("export_base", "Export Base");
    existing.resources = {"reports/*"};
    existing.actions = {"*"};
    existing.allow_export = false;
    manager->addRuleVersioned(existing, "admin", "no-export rule");

    auto new_rule = createTestRule("export_new", "Export New");
    new_rule.resources = {"reports/*"};
    new_rule.actions = {"read"};
    new_rule.allow_export = true;

    auto conflicts = manager->checkConflictsForRule(new_rule);

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].severity, "high");
}

TEST_F(PolicyManagerVersionedTest, CheckConflictsForRule_OverlapSamePriority) {
    auto existing = createTestRule("overlap_base", "Overlap Base");
    existing.resources = {"data/*"};
    existing.actions = {"read"};
    existing.priority = 5;
    manager->addRuleVersioned(existing, "admin", "baseline");

    // Same resource/action and same priority but no contradictory effects
    auto new_rule = createTestRule("overlap_new", "Overlap New");
    new_rule.resources = {"data/*"};
    new_rule.actions = {"read"};
    new_rule.priority = 5;

    auto conflicts = manager->checkConflictsForRule(new_rule);

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "overlapping");
    EXPECT_EQ(conflicts[0].severity, "low");
}

TEST_F(PolicyManagerVersionedTest, CheckConflictsForRule_NoConflict) {
    auto existing = createTestRule("noconflict_base", "No Conflict Base");
    existing.resources = {"logs/*"};
    existing.actions = {"write"};
    manager->addRuleVersioned(existing, "admin", "baseline");

    // Different resource pattern – no overlap
    auto new_rule = createTestRule("noconflict_new", "No Conflict New");
    new_rule.resources = {"data/*"};
    new_rule.actions = {"read"};

    auto conflicts = manager->checkConflictsForRule(new_rule);

    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyManagerVersionedTest, CheckConflictsForRule_DifferentPriorityNoConflict) {
    // Same resource/action but different priorities (higher priority wins, no cycle)
    auto existing = createTestRule("prio_base", "Priority Base");
    existing.resources = {"keys/*"};
    existing.actions = {"*"};
    existing.require_encryption = true;
    existing.priority = 10;
    manager->addRuleVersioned(existing, "admin", "high-prio rule");

    auto new_rule = createTestRule("prio_new", "Priority New");
    new_rule.resources = {"keys/*"};
    new_rule.actions = {"read"};
    new_rule.require_encryption = false;
    new_rule.priority = 5;  // lower priority – not an irresolvable conflict

    auto conflicts = manager->checkConflictsForRule(new_rule);

    // Should still report contradictory effects (severity) even with different priorities
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
}

TEST_F(PolicyManagerVersionedTest, GetActiveConflicts_ReturnsAllConflicts) {
    auto rule_a = createTestRule("active_a", "Rule A");
    rule_a.resources = {"shared/*"};
    rule_a.actions = {"read"};
    rule_a.require_encryption = true;
    manager->addRuleVersioned(rule_a, "admin", "rule a");

    auto rule_b = createTestRule("active_b", "Rule B");
    rule_b.resources = {"shared/*"};
    rule_b.actions = {"read"};
    rule_b.require_encryption = false;
    manager->addRuleVersioned(rule_b, "admin", "rule b – conflicts with a");

    auto conflicts = manager->getActiveConflicts();

    ASSERT_FALSE(conflicts.empty());
    bool found_pair = false;
    for (const auto& c : conflicts) {
        const bool contains_a =
            (c.new_rule_id == "active_a") ||
            (std::find(c.conflicting_rule_ids.begin(), c.conflicting_rule_ids.end(), "active_a") !=
             c.conflicting_rule_ids.end());
        const bool contains_b =
            (c.new_rule_id == "active_b") ||
            (std::find(c.conflicting_rule_ids.begin(), c.conflicting_rule_ids.end(), "active_b") !=
             c.conflicting_rule_ids.end());
        if (contains_a && contains_b) {
            found_pair = true;
            break;
        }
    }
    EXPECT_TRUE(found_pair) << "Expected to find the active_a / active_b conflict pair";
}

TEST_F(PolicyManagerVersionedTest, GetActiveConflicts_EmptyWhenNoConflicts) {
    auto rule_a = createTestRule("clean_a", "Clean A");
    rule_a.resources = {"data/*"};
    rule_a.actions = {"read"};
    manager->addRuleVersioned(rule_a, "admin", "clean a");

    auto rule_b = createTestRule("clean_b", "Clean B");
    rule_b.resources = {"logs/*"};
    rule_b.actions = {"write"};
    manager->addRuleVersioned(rule_b, "admin", "clean b");

    auto conflicts = manager->getActiveConflicts();

    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyManagerVersionedTest, ConflictInfoJson) {
    ConflictInfo info;
    info.conflict_type = "contradictory";
    info.severity = "critical";
    info.new_rule_id = "rule_x";
    info.conflicting_rule_ids = {"rule_y"};
    info.description = "Test description";
    info.resolution_suggestions = {"Fix it"};
    info.detected_at = 1234567890;

    auto j = info.toJson();

    EXPECT_EQ(j["conflict_type"], "contradictory");
    EXPECT_EQ(j["severity"], "critical");
    EXPECT_EQ(j["new_rule_id"], "rule_x");
    EXPECT_EQ(j["conflicting_rule_ids"][0], "rule_y");
    EXPECT_EQ(j["description"], "Test description");
    EXPECT_EQ(j["resolution_suggestions"][0], "Fix it");
    EXPECT_EQ(j["detected_at"], 1234567890);
}

// ========== PolicyVersioningApiHandler Conflict Endpoint Tests ==========

class PolicyVersioningApiHandlerConflictTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_mgr_versioned = std::make_shared<PolicyManagerWithVersioning>();
        handler = std::make_unique<themis::server::PolicyVersioningApiHandler>(
            policy_mgr_versioned, nullptr);
    }

    http::request<http::string_body> makeGet(const std::string& target) {
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::content_type, "application/json");
        // No auth header -- handler allows access when auth is null (dev mode)
        req.prepare_payload();
        return req;
    }

    PolicyRule createRule(const std::string& id, const std::string& name,
                          const std::string& resource = "data/*") {
        PolicyRule r;
        r.id   = id;
        r.name = name;
        r.resources = {resource};
        r.actions   = {"read"};
        r.enabled   = true;
        r.priority  = 5;
        return r;
    }

    std::shared_ptr<PolicyManagerWithVersioning> policy_mgr_versioned;
    std::unique_ptr<themis::server::PolicyVersioningApiHandler> handler;
};

TEST_F(PolicyVersioningApiHandlerConflictTest, GetConflicts_EmptyWhenNoConflicts) {
    auto rule = createRule("nc_a", "No Conflict A", "logs/*");
    policy_mgr_versioned->addRuleVersioned(rule, "admin", "baseline");

    auto req = makeGet("/policies/conflicts");
    auto res = handler->handleGetConflicts(req);

    EXPECT_EQ(res.result(), http::status::ok);

    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("conflicts"));
    EXPECT_TRUE(body.contains("conflict_count"));
    EXPECT_EQ(body["conflict_count"].get<int>(), 0);
    EXPECT_FALSE(body["has_critical_conflicts"].get<bool>());
}

TEST_F(PolicyVersioningApiHandlerConflictTest, GetConflicts_ReturnsConflictsAsJson) {
    // Rule A – requires encryption
    auto rule_a = createRule("hc_a", "High Conflict A", "sensitive/*");
    rule_a.require_encryption = true;
    policy_mgr_versioned->addRuleVersioned(rule_a, "admin", "rule a");

    // Rule B – same resource, same action but NO encryption requirement → conflict
    auto rule_b = createRule("hc_b", "High Conflict B", "sensitive/*");
    rule_b.require_encryption = false;
    policy_mgr_versioned->addRuleVersioned(rule_b, "admin", "rule b");

    auto req = makeGet("/policies/conflicts");
    auto res = handler->handleGetConflicts(req);

    EXPECT_EQ(res.result(), http::status::ok);

    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("conflicts"));
    EXPECT_GT(body["conflict_count"].get<int>(), 0);
    EXPECT_TRUE(body["has_critical_conflicts"].get<bool>());

    // Each conflict entry must carry resolution suggestions
    const auto& first = body["conflicts"][0];
    EXPECT_TRUE(first.contains("conflict_type"));
    EXPECT_TRUE(first.contains("severity"));
    EXPECT_TRUE(first.contains("description"));
    EXPECT_TRUE(first.contains("resolution_suggestions"));
    EXPECT_FALSE(first["resolution_suggestions"].empty());
}

TEST_F(PolicyVersioningApiHandlerConflictTest, ListVersions_InvalidRuleIdReturns400) {
    auto req = makeGet("/policies/rules/../bad/versions");
    auto res = handler->handleListVersions(req, "../bad");

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyVersioningApiHandlerConflictTest, QueryAudit_InvalidStartTimeReturns400) {
    auto req = makeGet("/policies/audit?start_time=not-a-number");
    auto res = handler->handleQueryAudit(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyVersioningApiHandlerConflictTest, QueryAudit_InvalidUserReturns400) {
    auto req = makeGet("/policies/audit?user=admin%0d%0aInjected:1");
    auto res = handler->handleQueryAudit(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

// Run all tests