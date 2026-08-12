#include <gtest/gtest.h>
#include "governance/policy_validator.h"
#include "governance/policy_manager.h"
#include <memory>
#include <chrono>

using namespace themis::governance;

// ========== Test fixture ==========

class PolicyValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_mgr_ = std::make_shared<PolicyManager>();
    }

    PolicyRule makeRule(const std::string& id,
                        const std::string& name,
                        const std::vector<std::string>& resources,
                        const std::vector<std::string>& actions,
                        bool enabled = true) {
        PolicyRule r;
        r.id = id;
        r.name = name;
        r.resources = resources;
        r.actions = actions;
        r.enabled = enabled;
        r.classification_level = "offen";
        r.allow_export = true;
        r.allow_cache = true;
        r.require_encryption = false;
        r.require_signature = false;
        r.audit_access = false;
        r.audit_changes = false;
        r.retention_days = 365;
        r.priority = 5;
        r.created_at = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        r.updated_at = r.created_at;
        r.version = "1.0.0";
        return r;
    }

    std::shared_ptr<PolicyManager> policy_mgr_;
};

// ========== detectConflicts ==========

TEST_F(PolicyValidatorTest, DetectConflicts_EncryptionContradiction) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("enc1", "Require Enc", {"data/*"}, {"read"});
    r1.require_encryption = true;
    auto r2 = makeRule("enc2", "No Enc", {"data/*"}, {"read"});
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].severity, "critical");
    EXPECT_EQ(conflicts[0].affected_rules.size(), 2u);
}

TEST_F(PolicyValidatorTest, DetectConflicts_ExportContradiction) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("exp1", "Allow Export", {"reports/*"}, {"read"});
    r1.allow_export = true;
    auto r2 = makeRule("exp2", "Deny Export", {"reports/*"}, {"read"});
    r2.allow_export = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].severity, "high");
}

TEST_F(PolicyValidatorTest, DetectConflicts_CacheContradiction) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("cache1", "Allow Cache", {"api/*"}, {"get"});
    r1.allow_cache = true;
    auto r2 = makeRule("cache2", "No Cache", {"api/*"}, {"get"});
    r2.allow_cache = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].severity, "medium");
}

TEST_F(PolicyValidatorTest, DetectConflicts_WildcardResourceOverlap) {
    // Wildcard resource "*" should match any specific resource
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("wc1", "Wildcard Rule", {"*"}, {"read"});
    r1.require_encryption = true;
    auto r2 = makeRule("wc2", "Specific Rule", {"data/secrets"}, {"read"});
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    ASSERT_FALSE(conflicts.empty()) << "Wildcard resource should overlap with specific resource";
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
}

TEST_F(PolicyValidatorTest, DetectConflicts_WildcardActionOverlap) {
    // Wildcard action "*" should match any specific action
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("wa1", "Wildcard Action", {"data/*"}, {"*"});
    r1.require_encryption = true;
    auto r2 = makeRule("wa2", "Specific Action", {"data/*"}, {"write"});
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    ASSERT_FALSE(conflicts.empty()) << "Wildcard action should overlap with specific action";
    EXPECT_EQ(conflicts[0].conflict_type, "contradictory");
    EXPECT_EQ(conflicts[0].severity, "critical");
}

TEST_F(PolicyValidatorTest, DetectConflicts_NoOverlapDifferentResources) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("nr1", "Rule A", {"data/*"}, {"read"});
    r1.require_encryption = true;
    auto r2 = makeRule("nr2", "Rule B", {"logs/*"}, {"read"});
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    EXPECT_TRUE(conflicts.empty()) << "Different resources should not conflict";
}

TEST_F(PolicyValidatorTest, DetectConflicts_NoOverlapDifferentActions) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("na1", "Rule A", {"data/*"}, {"read"});
    r1.require_encryption = true;
    auto r2 = makeRule("na2", "Rule B", {"data/*"}, {"delete"});
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    EXPECT_TRUE(conflicts.empty()) << "Different actions should not conflict";
}

TEST_F(PolicyValidatorTest, DetectConflicts_DisabledRulesIgnored) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("dis1", "Disabled Rule", {"data/*"}, {"read"}, /*enabled=*/false);
    r1.require_encryption = true;
    auto r2 = makeRule("dis2", "Enabled Rule", {"data/*"}, {"read"}, /*enabled=*/true);
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    EXPECT_TRUE(conflicts.empty()) << "Disabled rules must be skipped in conflict detection";
}

TEST_F(PolicyValidatorTest, DetectConflicts_NoRules) {
    PolicyValidator validator(policy_mgr_);
    auto conflicts = validator.detectConflicts();
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyValidatorTest, DetectConflicts_SingleRule) {
    PolicyValidator validator(policy_mgr_);
    auto r1 = makeRule("single", "Only Rule", {"data/*"}, {"read"});
    policy_mgr_->addRule(r1);

    auto conflicts = validator.detectConflicts();
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PolicyValidatorTest, DetectConflicts_ResolutionSuggestionsPresent) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("rs1", "Rule 1", {"data/*"}, {"*"});
    r1.allow_export = true;
    auto r2 = makeRule("rs2", "Rule 2", {"data/*"}, {"*"});
    r2.allow_export = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto conflicts = validator.detectConflicts();

    ASSERT_FALSE(conflicts.empty());
    EXPECT_FALSE(conflicts[0].resolution_suggestions.empty());
    EXPECT_FALSE(conflicts[0].description.empty());
}

// ========== detectOverlappingPermissions ==========

TEST_F(PolicyValidatorTest, DetectOverlapping_SamePriority) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("ov1", "Overlap A", {"shared/*"}, {"read"});
    r1.priority = 5;
    auto r2 = makeRule("ov2", "Overlap B", {"shared/*"}, {"read"});
    r2.priority = 5;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto overlaps = validator.detectOverlappingPermissions();

    ASSERT_FALSE(overlaps.empty());
    EXPECT_EQ(overlaps[0].conflict_type, "overlapping");
}

TEST_F(PolicyValidatorTest, DetectOverlapping_DifferentPrioritiesNoOverlapConflict) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("dp1", "Rule A", {"shared/*"}, {"read"});
    r1.priority = 5;
    auto r2 = makeRule("dp2", "Rule B", {"shared/*"}, {"read"});
    r2.priority = 10;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto overlaps = validator.detectOverlappingPermissions();

    EXPECT_TRUE(overlaps.empty()) << "Different priorities resolve ambiguity";
}

TEST_F(PolicyValidatorTest, DetectOverlapping_DisabledRulesIgnored) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("dis_ov1", "Disabled Overlap", {"shared/*"}, {"read"}, /*enabled=*/false);
    r1.priority = 5;
    auto r2 = makeRule("dis_ov2", "Enabled Overlap", {"shared/*"}, {"read"}, /*enabled=*/true);
    r2.priority = 5;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto overlaps = validator.detectOverlappingPermissions();

    EXPECT_TRUE(overlaps.empty()) << "Disabled rules must be skipped in overlap detection";
}

TEST_F(PolicyValidatorTest, DetectOverlapping_SameResourceDifferentAction_NoConflict) {
    PolicyValidator validator(policy_mgr_);

    // Same resource but completely different actions – not an overlap conflict
    auto r1 = makeRule("diff_act1", "Rule Read", {"data/*"}, {"read"});
    r1.priority = 5;
    auto r2 = makeRule("diff_act2", "Rule Write", {"data/*"}, {"write"});
    r2.priority = 5;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto overlaps = validator.detectOverlappingPermissions();

    EXPECT_TRUE(overlaps.empty())
        << "Same resource but disjoint actions must not produce an overlap conflict";
}

// ========== detectCircularDependencies ==========

TEST_F(PolicyValidatorTest, DetectCircular_ThreeWayCycle) {
    PolicyValidator validator(policy_mgr_);

    // Three rules at the same priority, same resource/action, with contradictory effects
    auto ra = makeRule("circ_a", "Circ A", {"shared/*"}, {"read"});
    ra.priority = 7;
    ra.require_encryption = true;
    ra.allow_export = false;
    ra.allow_cache = false;

    auto rb = makeRule("circ_b", "Circ B", {"shared/*"}, {"read"});
    rb.priority = 7;
    rb.require_encryption = false;  // contradicts ra
    rb.allow_export = true;
    rb.allow_cache = true;

    auto rc = makeRule("circ_c", "Circ C", {"shared/*"}, {"read"});
    rc.priority = 7;
    rc.require_encryption = true;
    rc.allow_export = true;         // contradicts ra
    rc.allow_cache = false;

    policy_mgr_->addRule(ra);
    policy_mgr_->addRule(rb);
    policy_mgr_->addRule(rc);

    auto circular = validator.detectCircularDependencies();

    ASSERT_FALSE(circular.empty());
    EXPECT_EQ(circular[0].conflict_type, "circular");
    EXPECT_EQ(circular[0].severity, "high");
    EXPECT_GE(circular[0].affected_rules.size(), 3u);
    EXPECT_FALSE(circular[0].resolution_suggestions.empty());
}

TEST_F(PolicyValidatorTest, DetectCircular_TwoRulesNoCycle) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("nc1", "No Cycle A", {"data/*"}, {"read"});
    r1.priority = 7;
    r1.require_encryption = true;
    auto r2 = makeRule("nc2", "No Cycle B", {"data/*"}, {"read"});
    r2.priority = 7;
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto circular = validator.detectCircularDependencies();

    // Two rules can't form a cycle (component size < 3)
    EXPECT_TRUE(circular.empty());
}

// ========== validateRuleset includes circular conflicts ==========

TEST_F(PolicyValidatorTest, ValidateRuleset_IncludesCircularConflicts) {
    PolicyValidator validator(policy_mgr_);

    auto ra = makeRule("vcirc_a", "VCirc A", {"shared/*"}, {"read"});
    ra.priority = 3; ra.require_encryption = true; ra.allow_export = false; ra.allow_cache = false;
    auto rb = makeRule("vcirc_b", "VCirc B", {"shared/*"}, {"read"});
    rb.priority = 3; rb.require_encryption = false; rb.allow_export = true; rb.allow_cache = true;
    auto rc = makeRule("vcirc_c", "VCirc C", {"shared/*"}, {"read"});
    rc.priority = 3; rc.require_encryption = true; rc.allow_export = true; rc.allow_cache = false;

    policy_mgr_->addRule(ra);
    policy_mgr_->addRule(rb);
    policy_mgr_->addRule(rc);

    auto report = validator.validateRuleset();

    bool has_circular = false;
    for (const auto& c : report.conflicts) {
        if (c.conflict_type == "circular") {
            has_circular = true;
            break;
        }
    }
    EXPECT_TRUE(has_circular) << "validateRuleset() must include circular dependency conflicts";
}

// ========== validateRuleset ==========

TEST_F(PolicyValidatorTest, ValidateRuleset_ProducesReport) {
    PolicyValidator validator(policy_mgr_);

    auto r1 = makeRule("vr1", "Rule A", {"data/*"}, {"read"});
    r1.require_encryption = true;
    auto r2 = makeRule("vr2", "Rule B", {"data/*"}, {"read"});
    r2.require_encryption = false;

    policy_mgr_->addRule(r1);
    policy_mgr_->addRule(r2);

    auto report = validator.validateRuleset();

    EXPECT_FALSE(report.report_id.empty());
    EXPECT_GT(report.generated_at, 0);
    EXPECT_FALSE(report.conflicts.empty());
    EXPECT_TRUE(report.has_critical_issues);
    EXPECT_GT(report.total_issues, 0);
    EXPECT_LT(report.validation_score, 100.0);
}

TEST_F(PolicyValidatorTest, ValidateRuleset_JsonSerializable) {
    PolicyValidator validator(policy_mgr_);
    auto report = validator.validateRuleset();
    auto j = report.toJson();

    EXPECT_TRUE(j.contains("report_id"));
    EXPECT_TRUE(j.contains("generated_at"));
    EXPECT_TRUE(j.contains("conflicts"));
    EXPECT_TRUE(j.contains("violations"));
    EXPECT_TRUE(j.contains("effectiveness_metrics"));
    EXPECT_TRUE(j.contains("has_critical_issues"));
    EXPECT_TRUE(j.contains("total_issues"));
    EXPECT_TRUE(j.contains("validation_score"));
}

// ========== validateSingleRule ==========

TEST_F(PolicyValidatorTest, ValidateSingleRule_EmptyIdFails) {
    PolicyValidator validator(policy_mgr_);
    PolicyRule bad_rule;
    bad_rule.resources = {"data/*"};
    bad_rule.actions = {"read"};

    auto issues = validator.validateSingleRule(bad_rule);
    EXPECT_FALSE(issues.empty());
}

TEST_F(PolicyValidatorTest, ValidateSingleRule_ValidRulePasses) {
    PolicyValidator validator(policy_mgr_);
    auto r = makeRule("valid", "Valid Rule", {"data/users"}, {"read"});

    auto issues = validator.validateSingleRule(r);
    EXPECT_TRUE(issues.empty());
}

// ========== recordRuleHit / calculateEffectiveness ==========

TEST_F(PolicyValidatorTest, RecordHitAndEffectiveness) {
    PolicyValidator validator(policy_mgr_);

    auto r = makeRule("hit1", "Hit Rule", {"data/*"}, {"read"});
    policy_mgr_->addRule(r);

    validator.recordRuleHit("hit1", 0.5);
    validator.recordRuleHit("hit1", 1.0);

    auto metrics = validator.calculateEffectiveness();
    ASSERT_FALSE(metrics.empty());

    auto it = std::find_if(metrics.begin(), metrics.end(),
        [](const RuleEffectiveness& m) { return m.rule_id == "hit1"; });
    ASSERT_NE(it, metrics.end());
    EXPECT_EQ(it->hit_count, 2);
    EXPECT_FALSE(it->is_unused);
}

TEST_F(PolicyValidatorTest, UnusedRuleDetected) {
    PolicyValidator validator(policy_mgr_);

    auto r = makeRule("unused1", "Unused Rule", {"data/*"}, {"read"});
    policy_mgr_->addRule(r);

    auto unused = validator.detectUnusedRules();
    ASSERT_FALSE(unused.empty());
    EXPECT_EQ(unused[0], "unused1");
}
