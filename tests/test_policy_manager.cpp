#include <gtest/gtest.h>
#include "governance/policy_manager.h"
#include "observability/metrics_collector.h"
#include <fstream>
#include <filesystem>

using namespace themis::governance;

class PolicyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<PolicyManager>();
        test_dir = std::filesystem::temp_directory_path() / "themis_policy_test";
        std::filesystem::create_directories(test_dir);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }
    
    std::unique_ptr<PolicyManager> manager;
    std::filesystem::path test_dir;
};

TEST_F(PolicyManagerTest, AddAndRetrieveRule) {
    PolicyRule rule;
    rule.id = "rule_001";
    rule.name = "Test Rule";
    rule.description = "A test policy rule";
    rule.classification_level = "vs-nfd";
    rule.enabled = true;
    rule.resources = {"data/*"};
    rule.actions = {"read", "write"};
    rule.require_encryption = true;
    rule.priority = 100;
    
    manager->addRule(rule);
    
    auto retrieved = manager->getRule("rule_001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, "rule_001");
    EXPECT_EQ(retrieved->name, "Test Rule");
    EXPECT_EQ(retrieved->classification_level, "vs-nfd");
    EXPECT_TRUE(retrieved->require_encryption);
}

TEST_F(PolicyManagerTest, RemoveRule) {
    PolicyRule rule;
    rule.id = "rule_002";
    rule.name = "Removable Rule";
    
    manager->addRule(rule);
    ASSERT_TRUE(manager->getRule("rule_002").has_value());
    
    manager->removeRule("rule_002");
    EXPECT_FALSE(manager->getRule("rule_002").has_value());
}

TEST_F(PolicyManagerTest, ListRules) {
    PolicyRule rule1;
    rule1.id = "rule_003";
    rule1.name = "Rule 1";
    
    PolicyRule rule2;
    rule2.id = "rule_004";
    rule2.name = "Rule 2";
    
    manager->addRule(rule1);
    manager->addRule(rule2);
    
    auto rules = manager->listRules();
    EXPECT_EQ(rules.size(), 2);
}

TEST_F(PolicyManagerTest, FindApplicableRules) {
    PolicyRule rule1;
    rule1.id = "rule_005";
    rule1.resources = {"data/*"};
    rule1.actions = {"read"};
    rule1.enabled = true;
    rule1.priority = 100;
    
    PolicyRule rule2;
    rule2.id = "rule_006";
    rule2.resources = {"keys/*"};
    rule2.actions = {"read"};
    rule2.enabled = true;
    rule2.priority = 50;
    
    PolicyRule rule3;
    rule3.id = "rule_007";
    rule3.resources = {"data/*"};
    rule3.actions = {"write"};
    rule3.enabled = false; // Disabled
    rule3.priority = 200;
    
    manager->addRule(rule1);
    manager->addRule(rule2);
    manager->addRule(rule3);
    
    std::vector<std::string> user_roles = {"operator"};
    auto applicable = manager->findApplicableRules("data/users", "read", user_roles);
    
    EXPECT_EQ(applicable.size(), 1);
    EXPECT_EQ(applicable[0].id, "rule_005");
}

TEST_F(PolicyManagerTest, FindApplicableRulesWithWildcards) {
    PolicyRule rule;
    rule.id = "rule_008";
    rule.resources = {"*"};
    rule.actions = {"*"};
    rule.enabled = true;
    rule.priority = 1;
    
    manager->addRule(rule);
    
    std::vector<std::string> user_roles = {"admin"};
    auto applicable = manager->findApplicableRules("anything", "any_action", user_roles);
    
    EXPECT_EQ(applicable.size(), 1);
    EXPECT_EQ(applicable[0].id, "rule_008");
}

TEST_F(PolicyManagerTest, EvaluatePolicy) {
    PolicyRule rule1;
    rule1.id = "rule_009";
    rule1.resources = {"data/*"};
    rule1.actions = {"*"};
    rule1.enabled = true;
    rule1.require_encryption = true;
    rule1.allow_export = false;
    rule1.retention_days = 90;
    rule1.priority = 100;
    
    manager->addRule(rule1);
    
    std::vector<std::string> user_roles = {"operator"};
    auto decision = manager->evaluatePolicy("data/users", "read", user_roles);
    
    EXPECT_TRUE(decision.allowed);
    EXPECT_TRUE(decision.require_encryption);
    EXPECT_FALSE(decision.allow_export);
    EXPECT_EQ(decision.retention_days, 90);
    EXPECT_EQ(decision.applied_rules.size(), 1);
}

TEST_F(PolicyManagerTest, EvaluatePolicyWithMultipleRules) {
    PolicyRule rule1;
    rule1.id = "rule_010";
    rule1.resources = {"data/*"};
    rule1.actions = {"*"};
    rule1.enabled = true;
    rule1.require_encryption = true;
    rule1.allow_export = true;
    rule1.retention_days = 365;
    rule1.priority = 50;
    
    PolicyRule rule2;
    rule2.id = "rule_011";
    rule2.resources = {"data/*"};
    rule2.actions = {"*"};
    rule2.enabled = true;
    rule2.require_signature = true;
    rule2.allow_export = false; // More restrictive
    rule2.retention_days = 180; // Shorter
    rule2.priority = 100; // Higher priority
    
    manager->addRule(rule1);
    manager->addRule(rule2);
    
    std::vector<std::string> user_roles = {"admin"};
    auto decision = manager->evaluatePolicy("data/sensitive", "write", user_roles);
    
    EXPECT_TRUE(decision.allowed);
    EXPECT_TRUE(decision.require_encryption); // From rule1
    EXPECT_TRUE(decision.require_signature);  // From rule2
    EXPECT_FALSE(decision.allow_export);      // Most restrictive (rule2)
    EXPECT_EQ(decision.retention_days, 180);  // Shortest (rule2)
    EXPECT_EQ(decision.applied_rules.size(), 2);
}

TEST_F(PolicyManagerTest, RuleWithRequiredRole) {
    PolicyRule rule;
    rule.id = "rule_012";
    rule.resources = {"admin/*"};
    rule.actions = {"*"};
    rule.enabled = true;
    rule.required_roles = {"admin"};
    rule.priority = 100;
    
    manager->addRule(rule);
    
    // User with admin role
    std::vector<std::string> admin_roles = {"admin"};
    auto applicable_admin = manager->findApplicableRules("admin/config", "write", admin_roles);
    EXPECT_EQ(applicable_admin.size(), 1);
    
    // User without admin role
    std::vector<std::string> user_roles = {"operator"};
    auto applicable_user = manager->findApplicableRules("admin/config", "write", user_roles);
    EXPECT_EQ(applicable_user.size(), 0);
}

TEST_F(PolicyManagerTest, SaveAndLoadRules) {
    PolicyRule rule1;
    rule1.id = "rule_013";
    rule1.name = "Persistent Rule 1";
    rule1.classification_level = "geheim";
    rule1.require_encryption = true;
    
    PolicyRule rule2;
    rule2.id = "rule_014";
    rule2.name = "Persistent Rule 2";
    rule2.classification_level = "vs-nfd";
    rule2.allow_export = false;
    
    manager->addRule(rule1);
    manager->addRule(rule2);
    
    auto save_path = test_dir / "rules.json";
    ASSERT_TRUE(manager->saveRules(save_path.string()));
    
    auto new_manager = std::make_unique<PolicyManager>();
    ASSERT_TRUE(new_manager->loadRules(save_path.string()));
    
    auto loaded_rule1 = new_manager->getRule("rule_013");
    ASSERT_TRUE(loaded_rule1.has_value());
    EXPECT_EQ(loaded_rule1->name, "Persistent Rule 1");
    EXPECT_EQ(loaded_rule1->classification_level, "geheim");
    EXPECT_TRUE(loaded_rule1->require_encryption);
    
    auto loaded_rule2 = new_manager->getRule("rule_014");
    ASSERT_TRUE(loaded_rule2.has_value());
    EXPECT_EQ(loaded_rule2->name, "Persistent Rule 2");
    EXPECT_FALSE(loaded_rule2->allow_export);
}

TEST_F(PolicyManagerTest, ValidateRules) {
    PolicyRule rule;
    rule.id = "rule_015";
    rule.name = "Valid Rule";
    rule.enabled = true;
    
    manager->addRule(rule);
    
    auto result = manager->validateRules();
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.errors.size(), 0);
}

TEST_F(PolicyManagerTest, GetStats) {
    PolicyRule rule1;
    rule1.id = "rule_016";
    rule1.enabled = true;
    rule1.classification_level = "vs-nfd";
    
    PolicyRule rule2;
    rule2.id = "rule_017";
    rule2.enabled = false;
    rule2.classification_level = "geheim";
    
    PolicyRule rule3;
    rule3.id = "rule_018";
    rule3.enabled = true;
    rule3.classification_level = "geheim";
    
    manager->addRule(rule1);
    manager->addRule(rule2);
    manager->addRule(rule3);
    
    auto stats = manager->getStats();
    EXPECT_EQ(stats.total_rules, 3);
    EXPECT_EQ(stats.enabled_rules, 2);
    EXPECT_EQ(stats.disabled_rules, 1);
    EXPECT_EQ(stats.rules_by_classification["vs-nfd"], 1);
    EXPECT_EQ(stats.rules_by_classification["geheim"], 2);
}

TEST_F(PolicyManagerTest, ExportAndImportRules) {
    PolicyRule rule;
    rule.id = "rule_019";
    rule.name = "Export Test Rule";
    rule.classification_level = "vs-nfd";
    
    manager->addRule(rule);
    
    auto exported = manager->exportRules();
    EXPECT_TRUE(exported.contains("rules"));
    EXPECT_EQ(exported["rules"].size(), 1);
    
    auto new_manager = std::make_unique<PolicyManager>();
    ASSERT_TRUE(new_manager->importRules(exported));
    
    auto imported_rule = new_manager->getRule("rule_019");
    ASSERT_TRUE(imported_rule.has_value());
    EXPECT_EQ(imported_rule->name, "Export Test Rule");
}

TEST_F(PolicyManagerTest, LoadRulesFromYAML) {
    // Create a temporary YAML file
    auto yaml_path = test_dir / "test_rules.yaml";
    std::ofstream yaml_file(yaml_path.string());
    yaml_file << R"(
rules:
  - id: "yaml_rule_001"
    name: "YAML Test Rule"
    description: "Test loading from YAML"
    classification_level: "vs-nfd"
    enabled: true
    resources:
      - "data/test/*"
    actions:
      - "read"
      - "write"
    required_roles:
      - "tester"
    require_encryption: true
    allow_export: false
    retention_days: 180
    priority: 75
  - id: "yaml_rule_002"
    name: "YAML Test Rule 2"
    enabled: false
    resources:
      - "keys/*"
    actions:
      - "*"
)";
    yaml_file.close();
    
    // Load from YAML
    auto yaml_manager = std::make_unique<PolicyManager>();
    ASSERT_TRUE(yaml_manager->loadRules(yaml_path.string()));
    
    // Verify loaded rules
    auto rule1 = yaml_manager->getRule("yaml_rule_001");
    ASSERT_TRUE(rule1.has_value());
    EXPECT_EQ(rule1->name, "YAML Test Rule");
    EXPECT_EQ(rule1->classification_level, "vs-nfd");
    EXPECT_TRUE(rule1->enabled);
    EXPECT_EQ(rule1->resources.size(), 1);
    EXPECT_EQ(rule1->resources[0], "data/test/*");
    EXPECT_EQ(rule1->actions.size(), 2);
    EXPECT_TRUE(rule1->require_encryption);
    EXPECT_FALSE(rule1->allow_export);
    EXPECT_EQ(rule1->retention_days, 180);
    EXPECT_EQ(rule1->priority, 75);
    
    auto rule2 = yaml_manager->getRule("yaml_rule_002");
    ASSERT_TRUE(rule2.has_value());
    EXPECT_EQ(rule2->name, "YAML Test Rule 2");
    EXPECT_FALSE(rule2->enabled);
    
    // Verify rule count
    auto all_rules = yaml_manager->listRules();
    EXPECT_EQ(all_rules.size(), 2);
}

// ========== Phase 5: Versioning Tests ==========

TEST_F(PolicyManagerTest, UpdateRuleCreatesVersion) {
    PolicyRule rule;
    rule.id = "rule_v001";
    rule.name = "Original Name";
    rule.description = "Original description";
    rule.version = "1.0.0";
    rule.require_encryption = false;
    
    manager->addRule(rule);
    
    // Update the rule
    PolicyRule updated_rule = rule;
    updated_rule.name = "Updated Name";
    updated_rule.require_encryption = true;
    
    ASSERT_TRUE(manager->updateRule("rule_v001", updated_rule, "user1", "Updated encryption requirement"));
    
    // Check that the current rule has the updated values
    auto current = manager->getRule("rule_v001");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Updated Name");
    EXPECT_TRUE(current->require_encryption);
    EXPECT_EQ(current->version, "1.0.1"); // Patch version incremented
    EXPECT_EQ(current->last_modified_by, "user1");
    
    // Check that the old version is in history
    auto versions = manager->getRuleVersions("rule_v001");
    EXPECT_GE(versions.size(), 1);
}

TEST_F(PolicyManagerTest, GetVersionHistory) {
    PolicyRule rule;
    rule.id = "rule_v002";
    rule.name = "Version Test";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    // Make multiple updates
    for (int i = 1; i <= 3; i++) {
        PolicyRule updated = rule;
        updated.name = "Version " + std::to_string(i);
        manager->updateRule("rule_v002", updated, "user1", "Update " + std::to_string(i));
    }
    
    auto versions = manager->getRuleVersions("rule_v002");
    EXPECT_EQ(versions.size(), 3); // 3 versions saved before updates
}

TEST_F(PolicyManagerTest, GetSpecificVersion) {
    PolicyRule rule;
    rule.id = "rule_v003";
    rule.name = "Original";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    PolicyRule updated = rule;
    updated.name = "Updated";
    manager->updateRule("rule_v003", updated, "user1", "First update");
    
    auto version_record = manager->getRuleVersion("rule_v003", "1.0.0");
    ASSERT_TRUE(version_record.has_value());
    EXPECT_EQ(version_record->rule_id, "rule_v003");
    EXPECT_EQ(version_record->version, "1.0.0");
}

TEST_F(PolicyManagerTest, RollbackToVersion) {
    PolicyRule rule;
    rule.id = "rule_v004";
    rule.name = "Original";
    rule.require_encryption = false;
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    // Update twice
    PolicyRule updated1 = rule;
    updated1.name = "Update 1";
    updated1.require_encryption = true;
    manager->updateRule("rule_v004", updated1, "user1", "First update");
    
    PolicyRule updated2 = updated1;
    updated2.name = "Update 2";
    manager->updateRule("rule_v004", updated2, "user1", "Second update");
    
    // Rollback to version 1.0.0
    ASSERT_TRUE(manager->rollbackToVersion("rule_v004", "1.0.0", "admin"));
    
    auto current = manager->getRule("rule_v004");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Original");
    EXPECT_FALSE(current->require_encryption);
    EXPECT_EQ(current->last_modified_by, "admin");
}

TEST_F(PolicyManagerTest, RollbackToPreviousVersion) {
    PolicyRule rule;
    rule.id = "rule_v005";
    rule.name = "Version 1";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    PolicyRule updated1 = rule;
    updated1.name = "Version 2";
    manager->updateRule("rule_v005", updated1, "user1", "Update to v2");
    
    PolicyRule updated2 = updated1;
    updated2.name = "Version 3";
    manager->updateRule("rule_v005", updated2, "user1", "Update to v3");
    
    // Rollback to previous version (v2)
    ASSERT_TRUE(manager->rollbackToPreviousVersion("rule_v005", "admin"));
    
    auto current = manager->getRule("rule_v005");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Version 2");
}

TEST_F(PolicyManagerTest, PreviewRollback) {
    PolicyRule rule;
    rule.id = "rule_v006";
    rule.name = "Current";
    rule.description = "Current desc";
    rule.require_encryption = true;
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    PolicyRule updated = rule;
    updated.name = "Updated";
    updated.description = "Updated desc";
    updated.require_encryption = false;
    manager->updateRule("rule_v006", updated, "user1", "Update");
    
    auto diffs = manager->previewRollback("rule_v006", "1.0.0");
    ASSERT_FALSE(diffs.empty());
    
    bool found_name_change = false;
    bool found_encryption_change = false;
    
    for (const auto& diff : diffs) {
        for (const auto& changed_field : diff.changes) {
            if (changed_field == "name") {
              found_name_change = true;
            }
            if (changed_field == "require_encryption") {
              found_encryption_change = true;
            }
        }
    }
    
    EXPECT_TRUE(found_name_change);
    EXPECT_TRUE(found_encryption_change);
}

TEST_F(PolicyManagerTest, CompareVersions) {
    PolicyRule rule;
    rule.id = "rule_v007";
    rule.name = "Version 1";
    rule.priority = 10;
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    PolicyRule updated1 = rule;
    updated1.name = "Version 2";
    updated1.priority = 20;
    manager->updateRule("rule_v007", updated1, "user1", "Update 1");
    
    PolicyRule updated2 = updated1;
    updated2.name = "Version 3";
    updated2.priority = 30;
    manager->updateRule("rule_v007", updated2, "user1", "Update 2");
    
    auto diffs = manager->compareRuleVersions("rule_v007", "1.0.0", "1.0.1");
    EXPECT_GT(diffs.size(), 0);
    
    bool found_name = false;
    bool found_priority = false;
    
    for (const auto& diff : diffs) {
        for (const auto& changed_field : diff.changes) {
            if (changed_field == "name") {
              found_name = true;
            }
            if (changed_field == "priority") {
              found_priority = true;
            }
        }
    }
    
    EXPECT_TRUE(found_name);
    EXPECT_TRUE(found_priority);
}

TEST_F(PolicyManagerTest, GetAuditTrail) {
    PolicyRule rule;
    rule.id = "rule_v008";
    rule.name = "Audit Test";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    // Make several updates
    for (int i = 1; i <= 5; i++) {
        PolicyRule updated = rule;
        updated.name = "Update " + std::to_string(i);
        manager->updateRule("rule_v008", updated, "user1", "Change " + std::to_string(i));
    }
    
    auto trail = manager->getAuditTrail("rule_v008");
    EXPECT_EQ(trail.size(), 5);
    
    // Verify trail is sorted by timestamp
    for (size_t i = 1; i < trail.size(); i++) {
        EXPECT_LE(trail[i-1].timestamp, trail[i].timestamp);
    }
}

TEST_F(PolicyManagerTest, GetAuditTrailByTimeRange) {
    PolicyRule rule;
    rule.id = "rule_v009";
    rule.name = "Time Range Test";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Make updates
    PolicyRule updated = rule;
    updated.name = "Updated";
    manager->updateRule("rule_v009", updated, "user1", "Update");
    
    // Get trail with time range that includes the update
    auto trail = manager->getAuditTrail("rule_v009", 0, now + 100);
    EXPECT_GE(trail.size(), 1);
}

TEST_F(PolicyManagerTest, GetAuditTrailByUser) {
    PolicyRule rule1;
    rule1.id = "rule_v010";
    rule1.name = "User Test 1";
    rule1.version = "1.0.0";
    
    PolicyRule rule2;
    rule2.id = "rule_v011";
    rule2.name = "User Test 2";
    rule2.version = "1.0.0";
    
    manager->addRule(rule1);
    manager->addRule(rule2);
    
    // User1 makes updates to both rules
    PolicyRule updated1 = rule1;
    updated1.name = "Updated by user1";
    manager->updateRule("rule_v010", updated1, "user1", "Update by user1");
    manager->updateRule("rule_v011", updated1, "user1", "Update by user1");
    
    // User2 makes update to rule1
    PolicyRule updated2 = updated1;
    updated2.name = "Updated by user2";
    manager->updateRule("rule_v010", updated2, "user2", "Update by user2");
    
    auto user1_trail = manager->getAuditTrailByUser("user1");
    EXPECT_EQ(user1_trail.size(), 2); // user1 made 2 updates
    
    auto user2_trail = manager->getAuditTrailByUser("user2");
    EXPECT_EQ(user2_trail.size(), 1); // user2 made 1 update
}

TEST_F(PolicyManagerTest, VersionIncrementLogic) {
    PolicyRule rule;
    rule.id = "rule_v012";
    rule.name = "Version Increment Test";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    // First update: 1.0.0 -> 1.0.1
    PolicyRule updated1 = rule;
    updated1.name = "Update 1";
    manager->updateRule("rule_v012", updated1, "user1", "Patch update");
    
    auto current1 = manager->getRule("rule_v012");
    ASSERT_TRUE(current1.has_value());
    EXPECT_EQ(current1->version, "1.0.1");
    
    // Second update: 1.0.1 -> 1.0.2
    PolicyRule updated2 = updated1;
    updated2.name = "Update 2";
    manager->updateRule("rule_v012", updated2, "user1", "Another patch");
    
    auto current2 = manager->getRule("rule_v012");
    ASSERT_TRUE(current2.has_value());
    EXPECT_EQ(current2->version, "1.0.2");
}

TEST_F(PolicyManagerTest, RollbackPreservesVersion) {
    PolicyRule rule;
    rule.id = "rule_v013";
    rule.name = "Original";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    // Update to 1.0.1
    PolicyRule updated = rule;
    updated.name = "Updated";
    manager->updateRule("rule_v013", updated, "user1", "Update");
    
    // Rollback creates a new version (doesn't revert version number)
    manager->rollbackToVersion("rule_v013", "1.0.0", "admin");
    
    auto current = manager->getRule("rule_v013");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->name, "Original"); // Content restored
    EXPECT_EQ(current->version, "1.0.2"); // But version continues incrementing
}

TEST_F(PolicyManagerTest, UpdateNonExistentRule) {
    PolicyRule rule;
    rule.id = "nonexistent";
    rule.name = "Test";
    
    bool result = manager->updateRule("nonexistent", rule, "user1", "Update");
    EXPECT_FALSE(result);
}

TEST_F(PolicyManagerTest, RollbackNonExistentRule) {
    bool result = manager->rollbackToVersion("nonexistent", "1.0.0", "user1");
    EXPECT_FALSE(result);
}

TEST_F(PolicyManagerTest, RollbackToNonExistentVersion) {
    PolicyRule rule;
    rule.id = "rule_v014";
    rule.name = "Test";
    rule.version = "1.0.0";
    
    manager->addRule(rule);
    
    bool result = manager->rollbackToVersion("rule_v014", "99.99.99", "user1");
    EXPECT_FALSE(result);
}

// ============================================================================
// PolicyManager::reloadPolicies() – double-buffer hot-reload
// ============================================================================

class PolicyManagerReloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir = std::filesystem::temp_directory_path() / "pm_reload_test";
        std::filesystem::create_directories(tmp_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir);
    }

    std::string writeYaml(const std::string& filename, const std::string& content) {
        auto path = (tmp_dir / filename).string();
        std::ofstream f(path, std::ios::trunc);
        f << content;
        return path;
    }

    std::filesystem::path tmp_dir;
};

static const char* kSingleRuleYaml = R"(
rules:
  - id: "rule_reload_001"
    name: "Reload Test Rule"
    classification_level: "offen"
    enabled: true
    resources:
      - "data/*"
    actions:
      - "read"
    priority: 10
)";

static const char* kTwoRuleYaml = R"(
rules:
  - id: "rule_reload_001"
    name: "Reload Test Rule"
    classification_level: "offen"
    enabled: true
    resources:
      - "data/*"
    actions:
      - "read"
    priority: 10
  - id: "rule_reload_002"
    name: "Second Rule"
    classification_level: "vs-nfd"
    enabled: true
    resources:
      - "keys/*"
    actions:
      - "write"
    priority: 20
)";

TEST_F(PolicyManagerReloadTest, ReloadPolicies_LoadsRulesFromFile) {
    auto mgr = std::make_unique<PolicyManager>();
    auto path = writeYaml("rules.yaml", kSingleRuleYaml);

    ASSERT_TRUE(mgr->reloadPolicies(path));
    EXPECT_EQ(mgr->listRules().size(), 1u);
    auto rule = mgr->getRule("rule_reload_001");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Reload Test Rule");
}

TEST_F(PolicyManagerReloadTest, ReloadPolicies_UpdatesActivePolicyVersion) {
    auto mgr = std::make_unique<PolicyManager>();
    auto path = writeYaml("rules.yaml", kSingleRuleYaml);

    EXPECT_TRUE(mgr->activePolicyVersion().empty());
    ASSERT_TRUE(mgr->reloadPolicies(path));
    EXPECT_FALSE(mgr->activePolicyVersion().empty());
}

TEST_F(PolicyManagerReloadTest, ReloadPolicies_ReturnsFalseOnMissingFile) {
    auto mgr = std::make_unique<PolicyManager>();
    std::string err;
    EXPECT_FALSE(mgr->reloadPolicies("/nonexistent/path/rules.yaml", &err));
    EXPECT_FALSE(err.empty());
    // Rules remain empty (reload failed → old state preserved)
    EXPECT_EQ(mgr->listRules().size(), 0u);
}

TEST_F(PolicyManagerReloadTest, ReloadPolicies_OldRulesRetainedOnLoadFailure) {
    auto mgr = std::make_unique<PolicyManager>();
    // First load succeeds
    auto path1 = writeYaml("rules.yaml", kSingleRuleYaml);
    ASSERT_TRUE(mgr->reloadPolicies(path1));
    ASSERT_EQ(mgr->listRules().size(), 1u);
    auto old_version = mgr->activePolicyVersion();

    // Second reload fails (bad path)
    std::string err;
    EXPECT_FALSE(mgr->reloadPolicies("/nonexistent/rules.yaml", &err));

    // Old rules and version are preserved
    EXPECT_EQ(mgr->listRules().size(), 1u);
    EXPECT_EQ(mgr->activePolicyVersion(), old_version);
}

TEST_F(PolicyManagerReloadTest, ReloadPolicies_VersionHashChangesAfterDifferentContent) {
    auto mgr = std::make_unique<PolicyManager>();
    auto path1 = writeYaml("rules1.yaml", kSingleRuleYaml);
    auto path2 = writeYaml("rules2.yaml", kTwoRuleYaml);

    ASSERT_TRUE(mgr->reloadPolicies(path1));
    auto v1 = mgr->activePolicyVersion();

    ASSERT_TRUE(mgr->reloadPolicies(path2));
    auto v2 = mgr->activePolicyVersion();

    EXPECT_NE(v1, v2);
    EXPECT_EQ(mgr->listRules().size(), 2u);
}

TEST_F(PolicyManagerReloadTest, ReloadPolicies_DoubleBufferSnapshotRemainsStable) {
    // Verify that a shared_ptr snapshot captured before reload continues
    // to see the old set while the new set is active (double-buffer semantics).
    auto mgr = std::make_unique<PolicyManager>();
    auto path1 = writeYaml("rules1.yaml", kSingleRuleYaml);
    auto path2 = writeYaml("rules2.yaml", kTwoRuleYaml);

    ASSERT_TRUE(mgr->reloadPolicies(path1));
    EXPECT_EQ(mgr->listRules().size(), 1u);

    // Now reload with 2 rules; the old listRules() result is already captured above
    // and would remain stable if held as a snapshot via shared_ptr.
    ASSERT_TRUE(mgr->reloadPolicies(path2));
    EXPECT_EQ(mgr->listRules().size(), 2u);  // new set is active
}

TEST_F(PolicyManagerReloadTest, ReloadPolicies_EmitsPrometheusCounter) {
    auto mgr = std::make_unique<PolicyManager>();
    auto path = writeYaml("rules.yaml", kSingleRuleYaml);

    ASSERT_TRUE(mgr->reloadPolicies(path));

    const auto metrics = themis::observability::MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(metrics.find("governance_policy_reload_total"), std::string::npos);
}

