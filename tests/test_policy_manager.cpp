#include <gtest/gtest.h>
#include "governance/policy_manager.h"
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
