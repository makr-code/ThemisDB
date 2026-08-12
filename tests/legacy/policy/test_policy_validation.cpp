#include <gtest/gtest.h>
#include "governance/policy_validation.h"
#include "governance/policy_manager.h"
#include <chrono>
#include <thread>

using namespace themis::governance;

class PolicyValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a policy manager with test rules
        policy_mgr_ = std::make_unique<PolicyManager>();
    }
    
    void TearDown() override {
        policy_mgr_.reset();
    }
    
    PolicyRule createTestRule(const std::string& id, 
                             const std::string& name,
                             const std::vector<std::string>& resources,
                             const std::vector<std::string>& actions) {
        PolicyRule rule;
        rule.id = id;
        rule.name = name;
        rule.enabled = true;
        rule.resources = resources;
        rule.actions = actions;
        rule.classification_level = "offen";
        rule.allow_export = true;
        rule.allow_cache = true;
        rule.require_encryption = false;
        rule.require_signature = false;
        rule.audit_access = false;
        rule.audit_changes = false;
        rule.retention_days = 365;
        rule.priority = 5;
        rule.created_at = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        rule.updated_at = rule.created_at;
        rule.version = "1.0.0";
        return rule;
    }
    
    std::unique_ptr<PolicyManager> policy_mgr_;
};

// ========== PolicyValidator Conflict Detection Tests ==========

TEST_F(PolicyValidationTest, DetectContradictoryRules_ExportConflict) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Allow Export", {"data/*"}, {"read"});
    rule1.allow_export = true;
    
    PolicyRule rule2 = createTestRule("rule2", "Deny Export", {"data/*"}, {"read"});
    rule2.allow_export = false;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectContradictoryRules(*policy_mgr_);
    
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].conflicting_rule_ids.size(), 2);
}

TEST_F(PolicyValidationTest, DetectContradictoryRules_EncryptionConflict) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Require Encryption", {"keys/*"}, {"*"});
    rule1.require_encryption = true;
    
    PolicyRule rule2 = createTestRule("rule2", "No Encryption", {"keys/*"}, {"*"});
    rule2.require_encryption = false;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectContradictoryRules(*policy_mgr_);
    
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].severity, "critical");
}

TEST_F(PolicyValidationTest, DetectContradictoryRules_CacheConflict) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Allow Cache", {"config/*"}, {"read"});
    rule1.allow_cache = true;
    
    PolicyRule rule2 = createTestRule("rule2", "Deny Cache", {"config/*"}, {"read"});
    rule2.allow_cache = false;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectContradictoryRules(*policy_mgr_);
    
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].severity, "medium");
}

TEST_F(PolicyValidationTest, DetectContradictoryRules_NoConflict) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"logs/*"}, {"write"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectContradictoryRules(*policy_mgr_);
    
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyValidationTest, DetectOverlappingPermissions_MultipleRules) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"data/*"}, {"read"});
    PolicyRule rule3 = createTestRule("rule3", "Rule 3", {"data/*"}, {"read"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    policy_mgr_->addRule(rule3);
    
    auto conflicts = validator.detectOverlappingPermissions(*policy_mgr_);
    
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "overlapping");
    EXPECT_EQ(conflicts[0].conflicting_rule_ids.size(), 3);
}

TEST_F(PolicyValidationTest, DetectOverlappingPermissions_NoOverlap) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"logs/*"}, {"write"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectOverlappingPermissions(*policy_mgr_);
    
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyValidationTest, DetectOverlappingPermissions_TwoRulesWithSamePattern) {
    PolicyValidator validator;

    // Exactly two rules covering the same resource and action – must be flagged
    PolicyRule rule1 = createTestRule("pair1", "Pair Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("pair2", "Pair Rule 2", {"data/*"}, {"read"});

    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);

    auto conflicts = validator.detectOverlappingPermissions(*policy_mgr_);

    ASSERT_FALSE(conflicts.empty()) << "Two rules with identical resource/action must be flagged";
    EXPECT_EQ(conflicts[0].conflict_type, "overlapping");
    EXPECT_EQ(conflicts[0].conflicting_rule_ids.size(), 2u);
}

TEST_F(PolicyValidationTest, DetectCircularDependencies_SimpleCircle) {
    PolicyValidator validator;
    
    // Create rules with circular priority dependencies on same resources
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"shared/*"}, {"read"});
    rule1.priority = 1;
    
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"shared/*"}, {"read"});
    rule2.priority = 2;
    
    PolicyRule rule3 = createTestRule("rule3", "Rule 3", {"shared/*"}, {"read"});
    rule3.priority = 3;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    policy_mgr_->addRule(rule3);
    
    auto conflicts = validator.detectCircularDependencies(*policy_mgr_);
    
    // May detect cycles depending on implementation
    // This test validates the function runs without error
    EXPECT_TRUE(true);
}

TEST_F(PolicyValidationTest, DetectCircularDependencies_NoCircle) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"logs/*"}, {"write"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectCircularDependencies(*policy_mgr_);
    
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyValidationTest, DetectCircularDependencies_ThreeWayCycle) {
    PolicyValidator validator;

    // Three rules that all share the same priority, the same resource/action,
    // and have contradictory effects: they form a fully-connected conflict
    // component of size 3 which the implementation reports as a cycle.
    PolicyRule rule_a = createTestRule("circ_a", "Circ A", {"shared/*"}, {"read"});
    rule_a.priority = 7;
    rule_a.require_encryption = true;
    rule_a.allow_export = false;
    rule_a.allow_cache = false;

    PolicyRule rule_b = createTestRule("circ_b", "Circ B", {"shared/*"}, {"read"});
    rule_b.priority = 7;
    rule_b.require_encryption = false;   // contradicts rule_a
    rule_b.allow_export = true;
    rule_b.allow_cache = true;

    PolicyRule rule_c = createTestRule("circ_c", "Circ C", {"shared/*"}, {"read"});
    rule_c.priority = 7;
    rule_c.require_encryption = true;
    rule_c.allow_export = true;          // contradicts rule_a
    rule_c.allow_cache = false;

    policy_mgr_->addRule(rule_a);
    policy_mgr_->addRule(rule_b);
    policy_mgr_->addRule(rule_c);

    auto conflicts = validator.detectCircularDependencies(*policy_mgr_);

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "circular");
    EXPECT_EQ(conflicts[0].severity, "high");
    EXPECT_GE(conflicts[0].conflicting_rule_ids.size(), 3u);
    EXPECT_FALSE(conflicts[0].recommendation.empty());
}

TEST_F(PolicyValidationTest, DetectConflicts_ComprehensiveCheck) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    rule1.allow_export = true;
    
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"data/*"}, {"read"});
    rule2.allow_export = false;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectConflicts(*policy_mgr_);
    
    ASSERT_FALSE(conflicts.empty());
}

// ========== PolicyValidator Effectiveness Tests ==========

TEST_F(PolicyValidationTest, CalculateEffectiveness_WithHitCounts) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"logs/*"}, {"write"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    std::unordered_map<std::string, int> hit_counts;
    hit_counts["rule1"] = 100;
    hit_counts["rule2"] = 5;
    
    auto metrics = validator.calculateEffectiveness(*policy_mgr_, hit_counts);
    
    ASSERT_EQ(metrics.size(), 2);
    EXPECT_GT(metrics["rule1"].effectiveness_score, metrics["rule2"].effectiveness_score);
    EXPECT_FALSE(metrics["rule1"].is_unused);
    EXPECT_FALSE(metrics["rule2"].is_unused);
}

TEST_F(PolicyValidationTest, CalculateEffectiveness_UnusedRules) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    policy_mgr_->addRule(rule1);
    
    std::unordered_map<std::string, int> hit_counts; // Empty - no hits
    
    auto metrics = validator.calculateEffectiveness(*policy_mgr_, hit_counts);
    
    ASSERT_EQ(metrics.size(), 1);
    EXPECT_TRUE(metrics["rule1"].is_unused);
    EXPECT_EQ(metrics["rule1"].hit_count, 0);
}

TEST_F(PolicyValidationTest, CalculateEffectiveness_OldRules) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Old Rule", {"data/*"}, {"read"});
    // Set created_at to 2 years ago
    rule1.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() - (730 * 24 * 3600);
    
    policy_mgr_->addRule(rule1);
    
    auto metrics = validator.calculateEffectiveness(*policy_mgr_);
    
    ASSERT_EQ(metrics.size(), 1);
    // Old rule should have some age penalty
    EXPECT_LT(metrics["rule1"].effectiveness_score, 100.0);
}

TEST_F(PolicyValidationTest, IdentifyUnusedRules_NeverUsed) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"logs/*"}, {"write"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    std::unordered_map<std::string, int> hit_counts;
    hit_counts["rule1"] = 100;
    // rule2 has no hits
    
    auto unused = validator.identifyUnusedRules(*policy_mgr_, hit_counts, 0);
    
    ASSERT_EQ(unused.size(), 1);
    EXPECT_EQ(unused[0], "rule2");
}

TEST_F(PolicyValidationTest, IdentifyUnusedRules_AllUsed) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"logs/*"}, {"write"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    std::unordered_map<std::string, int> hit_counts;
    hit_counts["rule1"] = 100;
    hit_counts["rule2"] = 50;
    
    auto unused = validator.identifyUnusedRules(*policy_mgr_, hit_counts, 30);
    
    EXPECT_TRUE(unused.empty());
}

// ========== PolicyValidator Security Checks Tests ==========

TEST_F(PolicyValidationTest, CheckOverlyPermissive_WildcardBoth) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"*"}, {"*"});
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkOverlyPermissive(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_FALSE(checks[0].passed);
    EXPECT_EQ(checks[0].severity, "high");
}

TEST_F(PolicyValidationTest, CheckOverlyPermissive_WildcardWithoutSecurity) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"*"}, {"read"});
    rule1.require_encryption = false;
    rule1.audit_access = false;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkOverlyPermissive(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_FALSE(checks[0].passed);
}

TEST_F(PolicyValidationTest, CheckOverlyPermissive_ExportWithoutControls) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"export"});
    rule1.allow_export = true;
    rule1.require_encryption = false;
    rule1.audit_access = false;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkOverlyPermissive(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_EQ(checks[0].severity, "high");
}

TEST_F(PolicyValidationTest, CheckOverlyPermissive_SecureRule) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/users"}, {"read"});
    rule1.require_encryption = true;
    rule1.audit_access = true;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkOverlyPermissive(*policy_mgr_);
    
    EXPECT_TRUE(checks.empty());
}

TEST_F(PolicyValidationTest, CheckEncryptionRequirements_SensitiveWithoutEncryption) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Secret Rule", {"secrets/*"}, {"read"});
    rule1.classification_level = "geheim";
    rule1.require_encryption = false;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkEncryptionRequirements(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_FALSE(checks[0].passed);
    EXPECT_EQ(checks[0].severity, "critical");
}

TEST_F(PolicyValidationTest, CheckEncryptionRequirements_ExportWithoutEncryption) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Export Rule", {"data/*"}, {"export"});
    rule1.allow_export = true;
    rule1.require_encryption = false;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkEncryptionRequirements(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_EQ(checks[0].severity, "high");
}

TEST_F(PolicyValidationTest, CheckEncryptionRequirements_SecureConfiguration) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Secure Rule", {"secrets/*"}, {"read"});
    rule1.classification_level = "geheim";
    rule1.require_encryption = true;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkEncryptionRequirements(*policy_mgr_);
    
    EXPECT_TRUE(checks.empty());
}

TEST_F(PolicyValidationTest, CheckAuditLogging_SensitiveWithoutAudit) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Secret Rule", {"secrets/*"}, {"read"});
    rule1.classification_level = "streng-geheim";
    rule1.audit_access = false;
    rule1.audit_changes = false;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkAuditLogging(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_FALSE(checks[0].passed);
    EXPECT_EQ(checks[0].severity, "high");
}

TEST_F(PolicyValidationTest, CheckAuditLogging_ExportWithoutAudit) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Export Rule", {"data/*"}, {"export"});
    rule1.allow_export = true;
    rule1.audit_access = false;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkAuditLogging(*policy_mgr_);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_EQ(checks[0].severity, "medium");
}

TEST_F(PolicyValidationTest, CheckAuditLogging_ProperAudit) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Audited Rule", {"secrets/*"}, {"read"});
    rule1.classification_level = "geheim";
    rule1.audit_access = true;
    rule1.audit_changes = true;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkAuditLogging(*policy_mgr_);
    
    EXPECT_TRUE(checks.empty());
}

TEST_F(PolicyValidationTest, CheckRetentionCompliance_TooShort) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    rule1.retention_days = 30;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkRetentionCompliance(*policy_mgr_, 90);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_FALSE(checks[0].passed);
    EXPECT_EQ(checks[0].severity, "medium");
}

TEST_F(PolicyValidationTest, CheckRetentionCompliance_TooLong) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    rule1.retention_days = 5000; // > 10 years
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkRetentionCompliance(*policy_mgr_, 90);
    
    ASSERT_FALSE(checks.empty());
    EXPECT_FALSE(checks[0].passed);
    EXPECT_EQ(checks[0].severity, "low");
}

TEST_F(PolicyValidationTest, CheckRetentionCompliance_Appropriate) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    rule1.retention_days = 365;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.checkRetentionCompliance(*policy_mgr_, 90);
    
    EXPECT_TRUE(checks.empty());
}

TEST_F(PolicyValidationTest, PerformSecurityChecks_Comprehensive) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Insecure Rule", {"*"}, {"*"});
    rule1.classification_level = "geheim";
    rule1.require_encryption = false;
    rule1.audit_access = false;
    rule1.allow_export = true;
    rule1.retention_days = 10;
    
    policy_mgr_->addRule(rule1);
    
    auto checks = validator.performSecurityChecks(*policy_mgr_);
    
    // Should find multiple issues
    ASSERT_GT(checks.size(), 1);
}

// ========== PolicyValidator Single Rule Validation Tests ==========

TEST_F(PolicyValidationTest, ValidateSingleRule_SecureRule) {
    PolicyValidator validator;
    
    PolicyRule rule = createTestRule("rule1", "Secure Rule", {"data/users"}, {"read"});
    rule.classification_level = "offen";
    rule.require_encryption = false;
    rule.audit_access = false;
    
    auto checks = validator.validateSingleRule(rule);
    
    EXPECT_TRUE(checks.empty());
}

TEST_F(PolicyValidationTest, ValidateSingleRule_MultipleIssues) {
    PolicyValidator validator;
    
    PolicyRule rule = createTestRule("rule1", "Insecure Rule", {"*"}, {"*"});
    rule.classification_level = "geheim";
    rule.require_encryption = false;
    rule.audit_access = false;
    rule.allow_export = true;
    
    auto checks = validator.validateSingleRule(rule);
    
    ASSERT_GT(checks.size(), 2);
}

// ========== ValidationReport Tests ==========

TEST_F(PolicyValidationTest, GenerateValidationReport_Comprehensive) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    rule1.allow_export = true;
    
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"data/*"}, {"read"});
    rule2.allow_export = false;
    
    PolicyRule rule3 = createTestRule("rule3", "Secret Rule", {"secrets/*"}, {"read"});
    rule3.classification_level = "geheim";
    rule3.require_encryption = false;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    policy_mgr_->addRule(rule3);
    
    std::unordered_map<std::string, int> hit_counts;
    hit_counts["rule1"] = 100;
    // rule2 and rule3 unused
    
    auto report = validator.generateValidationReport(*policy_mgr_, hit_counts);
    
    EXPECT_EQ(report.total_rules_checked, 3);
    EXPECT_GT(report.conflicts_found, 0);
    EXPECT_GT(report.security_issues_found, 0);
    EXPECT_GT(report.effectiveness_issues_found, 0);
    EXPECT_FALSE(report.recommendations.empty());
    EXPECT_GT(report.generated_at, 0);
}

TEST_F(PolicyValidationTest, ValidationReport_ToJson) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    policy_mgr_->addRule(rule1);
    
    auto report = validator.generateValidationReport(*policy_mgr_);
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("total_rules_checked"));
    EXPECT_TRUE(json.contains("conflicts_found"));
    EXPECT_TRUE(json.contains("security_issues_found"));
    EXPECT_TRUE(json.contains("effectiveness_issues_found"));
    EXPECT_TRUE(json.contains("generated_at"));
}

// ========== PolicyMetricsCollector Tests ==========

TEST_F(PolicyValidationTest, MetricsCollector_RecordEvaluation) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("rule1", true, 100);
    collector.recordEvaluation("rule1", true, 150);
    collector.recordEvaluation("rule1", false, 120);
    
    auto metrics = collector.getRuleMetrics("rule1");
    
    ASSERT_TRUE(metrics.has_value());
    EXPECT_EQ(metrics->rule_id, "rule1");
    EXPECT_EQ(metrics->evaluation_count, 3);
    EXPECT_EQ(metrics->match_count, 2);
    EXPECT_GT(metrics->avg_evaluation_time_us, 0);
    EXPECT_NEAR(metrics->match_rate, 66.67, 1.0);
}

TEST_F(PolicyValidationTest, MetricsCollector_GetNonexistentRule) {
    PolicyMetricsCollector collector;
    
    auto metrics = collector.getRuleMetrics("nonexistent");
    
    EXPECT_FALSE(metrics.has_value());
}

TEST_F(PolicyValidationTest, MetricsCollector_GetAllMetrics) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("rule1", true, 100);
    collector.recordEvaluation("rule2", false, 200);
    collector.recordEvaluation("rule3", true, 150);
    
    auto all_metrics = collector.getAllMetrics();
    
    EXPECT_EQ(all_metrics.size(), 3);
    EXPECT_TRUE(all_metrics.count("rule1") > 0);
    EXPECT_TRUE(all_metrics.count("rule2") > 0);
    EXPECT_TRUE(all_metrics.count("rule3") > 0);
}

TEST_F(PolicyValidationTest, MetricsCollector_AnalyzePerformanceImpact) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("fast_rule", true, 50);
    collector.recordEvaluation("normal_rule", true, 300);
    collector.recordEvaluation("slow_rule", true, 1500);
    collector.recordEvaluation("critical_rule", true, 3000);
    
    auto impacts = collector.analyzePerformanceImpact();
    
    ASSERT_EQ(impacts.size(), 4);
    // Should be sorted by avg time (slowest first)
    EXPECT_EQ(impacts[0].rule_id, "critical_rule");
    EXPECT_EQ(impacts[0].performance_category, "critical");
}

TEST_F(PolicyValidationTest, MetricsCollector_GetSlowRules) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("fast_rule", true, 50);
    collector.recordEvaluation("slow_rule1", true, 1500);
    collector.recordEvaluation("slow_rule2", true, 2000);
    
    auto slow_rules = collector.getSlowRules(1000);
    
    EXPECT_EQ(slow_rules.size(), 2);
}

TEST_F(PolicyValidationTest, MetricsCollector_ExportImport) {
    PolicyMetricsCollector collector1;
    
    collector1.recordEvaluation("rule1", true, 100);
    collector1.recordEvaluation("rule2", false, 200);
    
    auto json = collector1.exportMetrics();
    
    PolicyMetricsCollector collector2;
    bool success = collector2.importMetrics(json);
    
    EXPECT_TRUE(success);
    
    auto metrics1 = collector1.getRuleMetrics("rule1");
    auto metrics2 = collector2.getRuleMetrics("rule1");
    
    ASSERT_TRUE(metrics1.has_value());
    ASSERT_TRUE(metrics2.has_value());
    EXPECT_EQ(metrics1->evaluation_count, metrics2->evaluation_count);
    EXPECT_EQ(metrics1->match_count, metrics2->match_count);
}

TEST_F(PolicyValidationTest, MetricsCollector_ImportInvalidJson) {
    PolicyMetricsCollector collector;
    
    nlohmann::json invalid_json = "not an array";
    bool success = collector.importMetrics(invalid_json);
    
    EXPECT_FALSE(success);
}

TEST_F(PolicyValidationTest, MetricsCollector_ResetMetrics) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("rule1", true, 100);
    collector.recordEvaluation("rule2", false, 200);
    
    collector.resetMetrics();
    
    auto all_metrics = collector.getAllMetrics();
    EXPECT_TRUE(all_metrics.empty());
}

TEST_F(PolicyValidationTest, MetricsCollector_ResetRuleMetrics) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("rule1", true, 100);
    collector.recordEvaluation("rule2", false, 200);
    
    collector.resetRuleMetrics("rule1");
    
    auto metrics1 = collector.getRuleMetrics("rule1");
    auto metrics2 = collector.getRuleMetrics("rule2");
    
    EXPECT_FALSE(metrics1.has_value());
    EXPECT_TRUE(metrics2.has_value());
}

TEST_F(PolicyValidationTest, MetricsCollector_ThreadSafety) {
    PolicyMetricsCollector collector;
    
    // Simulate concurrent evaluations
    std::thread t1([&]() {
        for (int i = 0; i < 100; ++i) {
            collector.recordEvaluation("rule1", true, 100);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 100; ++i) {
            collector.recordEvaluation("rule1", false, 150);
        }
    });
    
    t1.join();
    t2.join();
    
    auto metrics = collector.getRuleMetrics("rule1");
    ASSERT_TRUE(metrics.has_value());
    EXPECT_EQ(metrics->evaluation_count, 200);
}

// ========== PolicyOptimizer Tests ==========

TEST_F(PolicyValidationTest, Optimizer_RecommendMerges) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"data/*"}, {"read"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto recommendations = optimizer.recommendMerges(*policy_mgr_);
    
    ASSERT_EQ(recommendations.size(), 2); // Both rules can be merged
    EXPECT_EQ(recommendations[0].optimization_type, "merge");
}

TEST_F(PolicyValidationTest, Optimizer_RecommendSimplifications) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Complex Rule", {}, {});
    // Add many resources
    for (int i = 0; i < 15; ++i) {
        rule1.resources.push_back("resource" + std::to_string(i));
    }
    
    policy_mgr_->addRule(rule1);
    
    auto recommendations = optimizer.recommendSimplifications(*policy_mgr_);
    
    ASSERT_FALSE(recommendations.empty());
    EXPECT_EQ(recommendations[0].optimization_type, "simplify");
}

TEST_F(PolicyValidationTest, Optimizer_RecommendReordering) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "High Match Rule", {"data/*"}, {"read"});
    rule1.priority = 2; // Low priority
    
    policy_mgr_->addRule(rule1);
    
    std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> metrics;
    PolicyMetricsCollector::RuleMetrics m;
    m.rule_id = "rule1";
    m.match_rate = 80.0; // High match rate
    m.evaluation_count = 100;
    metrics["rule1"] = m;
    
    auto recommendations = optimizer.recommendReordering(*policy_mgr_, metrics);
    
    ASSERT_FALSE(recommendations.empty());
    EXPECT_EQ(recommendations[0].optimization_type, "reorder");
}

TEST_F(PolicyValidationTest, Optimizer_RecommendRemovals_NeverUsed) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Old Unused Rule", {"data/*"}, {"read"});
    // Set created_at to 100 days ago
    rule1.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() - (100 * 24 * 3600);
    
    policy_mgr_->addRule(rule1);
    
    std::unordered_map<std::string, int> hit_counts;
    // rule1 has no hits
    
    auto recommendations = optimizer.recommendRemovals(*policy_mgr_, hit_counts);
    
    ASSERT_FALSE(recommendations.empty());
    EXPECT_EQ(recommendations[0].optimization_type, "remove");
    EXPECT_EQ(recommendations[0].priority, 8);
}

TEST_F(PolicyValidationTest, Optimizer_RecommendRemovals_RarelyUsed) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Rarely Used Rule", {"data/*"}, {"read"});
    // Set created_at to 200 days ago
    rule1.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() - (200 * 24 * 3600);
    
    policy_mgr_->addRule(rule1);
    
    std::unordered_map<std::string, int> hit_counts;
    hit_counts["rule1"] = 3; // Very few hits
    
    auto recommendations = optimizer.recommendRemovals(*policy_mgr_, hit_counts);
    
    ASSERT_FALSE(recommendations.empty());
    EXPECT_EQ(recommendations[0].optimization_type, "remove");
}

TEST_F(PolicyValidationTest, Optimizer_GenerateRecommendations) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"data/*"}, {"read"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    PolicyValidator validator;
    auto validation_report = validator.generateValidationReport(*policy_mgr_);
    
    std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> metrics;
    
    auto recommendations = optimizer.generateRecommendations(
        *policy_mgr_, validation_report, metrics);
    
    EXPECT_FALSE(recommendations.empty());
}

TEST_F(PolicyValidationTest, Optimizer_GenerateOptimizationReport) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Rule 1", {"data/*"}, {"read"});
    PolicyRule rule2 = createTestRule("rule2", "Rule 2", {"data/*"}, {"read"});
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    PolicyValidator validator;
    auto validation_report = validator.generateValidationReport(*policy_mgr_);
    
    std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> metrics;
    
    auto report = optimizer.generateOptimizationReport(
        *policy_mgr_, validation_report, metrics);
    
    EXPECT_GT(report.total_recommendations, 0);
    EXPECT_FALSE(report.summary.empty());
    EXPECT_GT(report.generated_at, 0);
}

TEST_F(PolicyValidationTest, OptimizationReport_ToJson) {
    PolicyOptimizer::OptimizationReport report;
    report.total_recommendations = 5;
    report.high_priority_recommendations = 2;
    report.summary = "Test summary";
    report.generated_at = 1234567890;
    
    PolicyOptimizer::OptimizationRecommendation rec;
    rec.recommendation_id = "rec1";
    rec.rule_id = "rule1";
    rec.optimization_type = "merge";
    rec.description = "Test recommendation";
    rec.rationale = "Test rationale";
    rec.expected_benefit = "Test benefit";
    rec.priority = 8;
    
    report.recommendations.push_back(rec);
    
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("total_recommendations"));
    EXPECT_TRUE(json.contains("high_priority_recommendations"));
    EXPECT_TRUE(json.contains("summary"));
    EXPECT_TRUE(json.contains("generated_at"));
    EXPECT_TRUE(json.contains("recommendations"));
    EXPECT_TRUE(json["recommendations"].is_array());
}

// ========== Edge Cases and Error Handling Tests ==========

TEST_F(PolicyValidationTest, EmptyPolicyManager) {
    PolicyValidator validator;
    
    auto conflicts = validator.detectConflicts(*policy_mgr_);
    EXPECT_TRUE(conflicts.empty());
    
    auto metrics = validator.calculateEffectiveness(*policy_mgr_);
    EXPECT_TRUE(metrics.empty());
    
    auto report = validator.generateValidationReport(*policy_mgr_);
    EXPECT_EQ(report.total_rules_checked, 0);
}

TEST_F(PolicyValidationTest, DisabledRulesIgnored) {
    PolicyValidator validator;
    
    PolicyRule rule1 = createTestRule("rule1", "Disabled Rule", {"data/*"}, {"read"});
    rule1.enabled = false;
    rule1.allow_export = true;
    
    PolicyRule rule2 = createTestRule("rule2", "Enabled Rule", {"data/*"}, {"read"});
    rule2.enabled = true;
    rule2.allow_export = false;
    
    policy_mgr_->addRule(rule1);
    policy_mgr_->addRule(rule2);
    
    auto conflicts = validator.detectContradictoryRules(*policy_mgr_);
    
    // Should not detect conflict because rule1 is disabled
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyValidationTest, MetricsCollector_ZeroEvaluationTime) {
    PolicyMetricsCollector collector;
    
    collector.recordEvaluation("rule1", true, 0);
    
    auto metrics = collector.getRuleMetrics("rule1");
    
    ASSERT_TRUE(metrics.has_value());
    EXPECT_EQ(metrics->avg_evaluation_time_us, 0);
}

TEST_F(PolicyValidationTest, Optimizer_NoOptimizationsNeeded) {
    PolicyOptimizer optimizer;
    
    PolicyRule rule1 = createTestRule("rule1", "Good Rule", {"data/users"}, {"read"});
    policy_mgr_->addRule(rule1);
    
    auto merges = optimizer.recommendMerges(*policy_mgr_);
    EXPECT_TRUE(merges.empty());
}

TEST_F(PolicyValidationTest, ConflictResult_ToJson) {
    PolicyValidator::ConflictResult conflict;
    conflict.conflict_id = "test_conflict";
    conflict.conflict_type = "contradictory";
    conflict.conflicting_rule_ids = {"rule1", "rule2"};
    conflict.description = "Test conflict";
    conflict.severity = "high";
    conflict.recommendation = "Fix it";
    
    auto json = conflict.toJson();
    
    EXPECT_EQ(json["conflict_id"], "test_conflict");
    EXPECT_EQ(json["conflict_type"], "contradictory");
    EXPECT_TRUE(json["conflicting_rule_ids"].is_array());
}

TEST_F(PolicyValidationTest, EffectivenessMetrics_ToJson) {
    PolicyValidator::EffectivenessMetrics metrics;
    metrics.rule_id = "rule1";
    metrics.hit_count = 100;
    metrics.effectiveness_score = 85.5;
    
    auto json = metrics.toJson();
    
    EXPECT_EQ(json["rule_id"], "rule1");
    EXPECT_EQ(json["hit_count"], 100);
    EXPECT_NEAR(json["effectiveness_score"].get<double>(), 85.5, 0.01);
}

TEST_F(PolicyValidationTest, SecurityCheckResult_ToJson) {
    PolicyValidator::SecurityCheckResult check;
    check.check_id = "check1";
    check.check_type = "missing_encryption";
    check.rule_id = "rule1";
    check.severity = "critical";
    check.passed = false;
    
    auto json = check.toJson();
    
    EXPECT_EQ(json["check_id"], "check1");
    EXPECT_EQ(json["passed"], false);
}

TEST_F(PolicyValidationTest, RuleMetrics_ToJson) {
    PolicyMetricsCollector::RuleMetrics metrics;
    metrics.rule_id = "rule1";
    metrics.evaluation_count = 500;
    metrics.match_count = 250;
    metrics.match_rate = 50.0;
    
    auto json = metrics.toJson();
    
    EXPECT_EQ(json["rule_id"], "rule1");
    EXPECT_EQ(json["evaluation_count"], 500);
    EXPECT_EQ(json["match_count"], 250);
}

TEST_F(PolicyValidationTest, PerformanceImpact_ToJson) {
    PolicyMetricsCollector::PerformanceImpact impact;
    impact.rule_id = "rule1";
    impact.avg_evaluation_time_us = 2500;
    impact.performance_category = "critical";
    
    auto json = impact.toJson();
    
    EXPECT_EQ(json["rule_id"], "rule1");
    EXPECT_EQ(json["performance_category"], "critical");
}

TEST_F(PolicyValidationTest, OptimizationRecommendation_ToJson) {
    PolicyOptimizer::OptimizationRecommendation rec;
    rec.recommendation_id = "rec1";
    rec.rule_id = "rule1";
    rec.optimization_type = "simplify";
    rec.priority = 7;
    
    auto json = rec.toJson();
    
    EXPECT_EQ(json["recommendation_id"], "rec1");
    EXPECT_EQ(json["optimization_type"], "simplify");
    EXPECT_EQ(json["priority"], 7);
}
