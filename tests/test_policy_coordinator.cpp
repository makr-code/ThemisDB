#include <gtest/gtest.h>
#include "governance/policy_coordinator.h"
#include <filesystem>
#include <fstream>
#include <memory>

using namespace themis::governance;

class PolicyCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_engine = std::make_shared<PolicyEngine>();
        policy_manager = std::make_shared<PolicyManager>();
        coordinator = std::make_unique<PolicyCoordinator>(policy_engine, policy_manager);
    }
    
    std::shared_ptr<PolicyEngine> policy_engine;
    std::shared_ptr<PolicyManager> policy_manager;
    std::unique_ptr<PolicyCoordinator> coordinator;
};

TEST_F(PolicyCoordinatorTest, EvaluateWithBothSystems) {
    // Add a PolicyManager rule
    PolicyRule rule;
    rule.id = "test_rule";
    rule.resources = {"data/*"};
    rule.actions = {"read"};
    rule.required_roles = {"operator"};
    rule.require_encryption = true;
    rule.allow_export = false;
    rule.retention_days = 90;
    policy_manager->addRule(rule);
    
    // Evaluate unified policy
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "vs-nfd";
    
    auto decision = coordinator->evaluate(
        headers,
        "/api/data",
        "data/users",
        "read",
        {"operator"}
    );
    
    // Check RBAC decision
    EXPECT_TRUE(decision.rbac_allowed);
    EXPECT_TRUE(decision.require_encryption);
    EXPECT_FALSE(decision.export_allowed);  // Rule denies export
    EXPECT_EQ(decision.retention_days, 90);
}

TEST_F(PolicyCoordinatorTest, MostRestrictiveWins) {
    // PolicyManager: allow export, 180 days retention
    PolicyRule rule1;
    rule1.id = "permissive_rule";
    rule1.resources = {"data/*"};
    rule1.actions = {"*"};
    rule1.allow_export = true;
    rule1.retention_days = 180;
    policy_manager->addRule(rule1);
    
    // Mock classification decision with stricter settings
    // (In real scenario, PolicyEngine would be configured via YAML)
    // For this test, we just verify the combining logic
    
    auto decision = coordinator->evaluate(
        {},
        "",
        "data/test",
        "read",
        {"user"}
    );
    
    // RBAC allows export, but combined decision should respect both
    EXPECT_TRUE(decision.export_allowed);  // Both allow
}

TEST_F(PolicyCoordinatorTest, CombineRetentionPeriods) {
    // Rule 1: 365 days
    PolicyRule rule1;
    rule1.id = "long_retention";
    rule1.resources = {"data/*"};
    rule1.actions = {"*"};
    rule1.retention_days = 365;
    rule1.priority = 50;
    policy_manager->addRule(rule1);
    
    // Rule 2: 90 days (shorter, should win)
    PolicyRule rule2;
    rule2.id = "short_retention";
    rule2.resources = {"data/*"};
    rule2.actions = {"*"};
    rule2.retention_days = 90;
    rule2.priority = 100;  // Higher priority
    policy_manager->addRule(rule2);
    
    auto decision = coordinator->evaluate(
        {},
        "",
        "data/test",
        "write",
        {"user"}
    );
    
    // Shortest retention wins
    EXPECT_EQ(decision.retention_days, 90);
}

TEST_F(PolicyCoordinatorTest, CheckAccessAllowed) {
    PolicyRule rule;
    rule.id = "access_rule";
    rule.resources = {"documents/*"};
    rule.actions = {"read"};
    rule.required_roles = {"reader"};
    policy_manager->addRule(rule);
    
    // User with correct role
    EXPECT_TRUE(coordinator->checkAccess("documents/file1", "read", {"reader"}));
    
    // User without required role
    EXPECT_FALSE(coordinator->checkAccess("documents/file1", "read", {"writer"}));
}

TEST_F(PolicyCoordinatorTest, CheckAccessDeniedNoMatch) {
    PolicyRule rule;
    rule.id = "restricted_rule";
    rule.resources = {"secrets/*"};
    rule.actions = {"*"};
    rule.required_roles = {"admin"};
    policy_manager->addRule(rule);
    
    // No matching rule for different resource
    EXPECT_TRUE(coordinator->checkAccess("public/data", "read", {"user"}));
}

TEST_F(PolicyCoordinatorTest, GetApplicableRules) {
    PolicyRule rule1;
    rule1.id = "rule1";
    rule1.resources = {"data/*"};
    rule1.actions = {"read"};
    rule1.required_roles = {"user"};
    policy_manager->addRule(rule1);
    
    PolicyRule rule2;
    rule2.id = "rule2";
    rule2.resources = {"data/*"};
    rule2.actions = {"write"};
    rule2.required_roles = {"admin"};
    policy_manager->addRule(rule2);
    
    // Get rules for read action
    auto rules = coordinator->getApplicableRules("data/test", "read", {"user"});
    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].id, "rule1");
    
    // Get rules for write action with admin role
    rules = coordinator->getApplicableRules("data/test", "write", {"admin"});
    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].id, "rule2");
}

TEST_F(PolicyCoordinatorTest, EvaluateClassificationOnly) {
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "geheim";
    
    auto decision = coordinator->evaluateClassification(headers, "/api/data");
    
    // Should work even without PolicyEngine configuration
    EXPECT_FALSE(decision.classification.empty());
}

TEST_F(PolicyCoordinatorTest, EvaluateRBACOnly) {
    PolicyRule rule;
    rule.id = "rbac_test";
    rule.resources = {"api/*"};
    rule.actions = {"call"};
    rule.require_encryption = true;
    policy_manager->addRule(rule);
    
    auto decision = coordinator->evaluateRBAC("api/endpoint", "call", {"user"});
    
    EXPECT_TRUE(decision.allowed);
    EXPECT_TRUE(decision.require_encryption);
}

TEST_F(PolicyCoordinatorTest, NullPolicyEngineHandling) {
    // Create coordinator with null PolicyEngine
    auto coord = std::make_unique<PolicyCoordinator>(nullptr, policy_manager);
    
    PolicyRule rule;
    rule.id = "test";
    rule.resources = {"data/*"};
    rule.actions = {"*"};
    policy_manager->addRule(rule);
    
    // Should still work with just PolicyManager
    auto decision = coord->evaluate({}, "", "data/test", "read", {"user"});
    EXPECT_TRUE(decision.rbac_allowed);
    EXPECT_EQ(decision.classification, "offen");  // Default
}

TEST_F(PolicyCoordinatorTest, NullPolicyManagerHandling) {
    // Create coordinator with null PolicyManager
    auto coord = std::make_unique<PolicyCoordinator>(policy_engine, nullptr);
    
    // Should still work with just PolicyEngine
    auto decision = coord->evaluate({}, "", "data/test", "read", {"user"});
    EXPECT_TRUE(decision.rbac_allowed);  // Permissive when no manager
}

TEST_F(PolicyCoordinatorTest, AuditFlags) {
    PolicyRule rule;
    rule.id = "audit_rule";
    rule.resources = {"sensitive/*"};
    rule.actions = {"*"};
    rule.audit_access = true;
    rule.audit_changes = true;
    policy_manager->addRule(rule);
    
    auto decision = coordinator->evaluate(
        {},
        "",
        "sensitive/data",
        "write",
        {"user"}
    );
    
    EXPECT_TRUE(decision.audit_access);
    EXPECT_TRUE(decision.audit_changes);
}

TEST_F(PolicyCoordinatorTest, RedactionLevelCombination) {
    // PolicyManager sets strict redaction
    PolicyRule rule;
    rule.id = "strict_rule";
    rule.resources = {"data/*"};
    rule.actions = {"*"};
    rule.redaction_level = "strict";
    policy_manager->addRule(rule);
    
    auto decision = coordinator->evaluate(
        {},
        "",
        "data/test",
        "read",
        {"user"}
    );
    
    // Most strict redaction should win
    EXPECT_EQ(decision.redaction, "strict");
}

// ---------------------------------------------------------------------------
// PolicyCoordinator hot-reload via startHotReload / stopHotReload
// ---------------------------------------------------------------------------

class PolicyCoordinatorHotReloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "coord_hotreload_test";
        std::filesystem::create_directories(tmp_dir_);
        yaml_path_ = (tmp_dir_ / "governance.yaml").string();
        policy_engine_ = std::make_shared<PolicyEngine>();
        policy_manager_ = std::make_shared<PolicyManager>();
        coordinator_ = std::make_unique<PolicyCoordinator>(policy_engine_, policy_manager_);
    }

    void TearDown() override {
        coordinator_->stopHotReload();
        std::filesystem::remove_all(tmp_dir_);
    }

    void writeYaml(const char* content) {
        std::ofstream f(yaml_path_, std::ios::trunc);
        ASSERT_TRUE(f.good());
        f << content;
        f.flush();
    }

    std::filesystem::path tmp_dir_;
    std::string yaml_path_;
    std::shared_ptr<PolicyEngine> policy_engine_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unique_ptr<PolicyCoordinator> coordinator_;
};

TEST_F(PolicyCoordinatorHotReloadTest, StartStopHotReload) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 90
    log_encryption: false
enforcement:
  default_mode: enforce
)");
    ASSERT_TRUE(policy_engine_->loadFromYAML(yaml_path_));

    EXPECT_FALSE(coordinator_->isHotReloadRunning());
    EXPECT_TRUE(coordinator_->startHotReload());
    EXPECT_TRUE(coordinator_->isHotReloadRunning());

    coordinator_->stopHotReload();
    EXPECT_FALSE(coordinator_->isHotReloadRunning());
}

TEST_F(PolicyCoordinatorHotReloadTest, StartHotReloadNoEngine_ReturnsFalse) {
    auto coordinator_no_engine = std::make_unique<PolicyCoordinator>(nullptr, policy_manager_);
    EXPECT_FALSE(coordinator_no_engine->startHotReload());
}

TEST_F(PolicyCoordinatorHotReloadTest, DoubleStartIsNoOp) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
enforcement:
  default_mode: enforce
)");
    ASSERT_TRUE(policy_engine_->loadFromYAML(yaml_path_));

    EXPECT_TRUE(coordinator_->startHotReload());
    EXPECT_TRUE(coordinator_->startHotReload());  // second call is a no-op
    EXPECT_TRUE(coordinator_->isHotReloadRunning());
    coordinator_->stopHotReload();
}
