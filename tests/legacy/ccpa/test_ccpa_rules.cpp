#include <gtest/gtest.h>
#include "governance/ccpa_rules.h"
#include "governance/policy_manager.h"
#include "governance/policy_engine.h"
#include "governance/compliance_reporting.h"
#include "governance/policy_validation.h"

#include <chrono>
#include <unordered_set>
#include <memory>

using namespace themis::governance;

// ============================================================================
// Helpers
// ============================================================================

namespace {

PolicyRule makeRule(const std::string& id,
                    bool audit_access    = false,
                    bool audit_changes   = false,
                    bool allow_export    = true,
                    bool require_signature = false,
                    int  retention_days  = 365,
                    const std::string& classification = "offen") {
    PolicyRule r;
    r.id                  = id;
    r.name                = "Rule " + id;
    r.enabled             = true;
    r.resources           = {"data/" + id};
    r.actions             = {"read"};
    r.classification_level = classification;
    r.audit_access        = audit_access;
    r.audit_changes       = audit_changes;
    r.allow_export        = allow_export;
    r.require_signature   = require_signature;
    r.retention_days      = retention_days;
    r.require_encryption  = false;
    return r;
}

} // anonymous namespace

// ============================================================================
// RightToKnow evaluator
// ============================================================================

TEST(RightToKnow, CompliantWhenAuditAccessEnabled) {
    RightToKnow rtk;
    PolicyRule rule = makeRule("r1", /*audit_access=*/true);
    EXPECT_TRUE(rtk.evaluate(rule));
}

TEST(RightToKnow, NonCompliantWhenAuditAccessDisabled) {
    RightToKnow rtk;
    PolicyRule rule = makeRule("r1", /*audit_access=*/false);
    EXPECT_FALSE(rtk.evaluate(rule));
}

TEST(RightToKnow, DisabledRuleAlwaysCompliant) {
    RightToKnow rtk;
    PolicyRule rule = makeRule("r1", /*audit_access=*/false);
    rule.enabled = false;
    EXPECT_TRUE(rtk.evaluate(rule)); // Disabled rules are exempt
}

TEST(RightToKnow, FrameworkAndId) {
    RightToKnow rtk;
    EXPECT_EQ(rtk.id(), "ccpa_right_to_know");
    EXPECT_EQ(rtk.framework(), "CCPA");
    EXPECT_FALSE(rtk.description().empty());
}

// ============================================================================
// RightToDelete evaluator
// ============================================================================

TEST(RightToDelete, CompliantWithAuditChangesAndReasonableRetention) {
    RightToDelete rtd;
    PolicyRule rule = makeRule("r1", false, /*audit_changes=*/true,
                               true, false, /*retention_days=*/365);
    EXPECT_TRUE(rtd.evaluate(rule));
}

TEST(RightToDelete, NonCompliantWhenAuditChangesMissing) {
    RightToDelete rtd;
    PolicyRule rule = makeRule("r1", false, /*audit_changes=*/false,
                               true, false, 365);
    EXPECT_FALSE(rtd.evaluate(rule));
}

TEST(RightToDelete, NonCompliantWhenRetentionTooLong) {
    RightToDelete rtd;
    PolicyRule rule = makeRule("r1", false, /*audit_changes=*/true,
                               true, false, /*retention_days=*/4000); // > 3650
    EXPECT_FALSE(rtd.evaluate(rule));
}

TEST(RightToDelete, NonCompliantWhenRetentionZero) {
    RightToDelete rtd;
    PolicyRule rule = makeRule("r1", false, true, true, false, 0);
    EXPECT_FALSE(rtd.evaluate(rule));
}

TEST(RightToDelete, DisabledRuleAlwaysCompliant) {
    RightToDelete rtd;
    PolicyRule rule = makeRule("r1", false, false, true, false, 9999);
    rule.enabled = false;
    EXPECT_TRUE(rtd.evaluate(rule));
}

// ============================================================================
// OptOutOfSale evaluator
// ============================================================================

TEST(OptOutOfSale, CompliantWhenExportDisabled) {
    OptOutOfSale oos;
    PolicyRule rule = makeRule("r1", false, false, /*allow_export=*/false);
    EXPECT_TRUE(oos.evaluate(rule));
}

TEST(OptOutOfSale, CompliantWhenExportEnabledButSignatureRequired) {
    OptOutOfSale oos;
    PolicyRule rule = makeRule("r1", false, false, /*allow_export=*/true,
                               /*require_signature=*/true);
    EXPECT_TRUE(oos.evaluate(rule));
}

TEST(OptOutOfSale, NonCompliantWhenExportEnabledWithoutSignature) {
    OptOutOfSale oos;
    PolicyRule rule = makeRule("r1", false, false, /*allow_export=*/true,
                               /*require_signature=*/false);
    EXPECT_FALSE(oos.evaluate(rule));
}

TEST(OptOutOfSale, DisabledRuleAlwaysCompliant) {
    OptOutOfSale oos;
    PolicyRule rule = makeRule("r1", false, false, true, false);
    rule.enabled = false;
    EXPECT_TRUE(oos.evaluate(rule));
}

// ============================================================================
// DataPortability evaluator
// ============================================================================

TEST(DataPortability, CompliantWhenExportAllowed) {
    DataPortability dp;
    PolicyRule rule = makeRule("r1", false, false, /*allow_export=*/true);
    EXPECT_TRUE(dp.evaluate(rule));
}

TEST(DataPortability, CompliantWhenAuditAccessEnabled) {
    // audit_access=true provides a path for manual portability fulfilment
    DataPortability dp;
    PolicyRule rule = makeRule("r1", /*audit_access=*/true, false, /*allow_export=*/false);
    EXPECT_TRUE(dp.evaluate(rule));
}

TEST(DataPortability, NonCompliantWhenBothExportAndAuditBlocked) {
    // No path to honour a portability request exists
    DataPortability dp;
    PolicyRule rule = makeRule("r1", /*audit_access=*/false, false, /*allow_export=*/false);
    EXPECT_FALSE(dp.evaluate(rule));
}

TEST(DataPortability, DisabledRuleAlwaysCompliant) {
    DataPortability dp;
    PolicyRule rule = makeRule("r1", false, false, /*allow_export=*/false);
    rule.enabled = false;
    EXPECT_TRUE(dp.evaluate(rule));
}

// ============================================================================
// CcpaRuleSet – opt-out registry
// ============================================================================

class CcpaRuleSetTest : public ::testing::Test {
protected:
    CcpaRuleSet ccpa;
};

TEST_F(CcpaRuleSetTest, SubjectNotOptedOutByDefault) {
    EXPECT_FALSE(ccpa.isOptedOut("user123"));
}

TEST_F(CcpaRuleSetTest, AddOptOut) {
    ccpa.addOptOut("user123");
    EXPECT_TRUE(ccpa.isOptedOut("user123"));
}

TEST_F(CcpaRuleSetTest, RemoveOptOut) {
    ccpa.addOptOut("user123");
    ccpa.removeOptOut("user123");
    EXPECT_FALSE(ccpa.isOptedOut("user123"));
}

TEST_F(CcpaRuleSetTest, OptOutCountReflectsRegistry) {
    EXPECT_EQ(ccpa.optOutCount(), 0u);
    ccpa.addOptOut("a");
    ccpa.addOptOut("b");
    EXPECT_EQ(ccpa.optOutCount(), 2u);
    ccpa.removeOptOut("a");
    EXPECT_EQ(ccpa.optOutCount(), 1u);
}

TEST_F(CcpaRuleSetTest, SetOptOutRegistry) {
    ccpa.setOptOutRegistry({"user1", "user2", "user3"});
    EXPECT_EQ(ccpa.optOutCount(), 3u);
    EXPECT_TRUE(ccpa.isOptedOut("user2"));
    EXPECT_FALSE(ccpa.isOptedOut("user4"));
}

TEST_F(CcpaRuleSetTest, EmptySubjectIdNeverOptedOut) {
    ccpa.addOptOut("");
    EXPECT_FALSE(ccpa.isOptedOut(""));
}

// ============================================================================
// CcpaRuleSet – rule evaluation
// ============================================================================

TEST_F(CcpaRuleSetTest, EvaluateFullyCompliantRule) {
    // A rule that satisfies all CCPA checks
    PolicyRule rule = makeRule("r1",
        /*audit_access*/   true,
        /*audit_changes*/  true,
        /*allow_export*/   false, // opt-out-of-sale satisfied
        /*require_sig*/    false,
        /*retention*/      365);
    EXPECT_TRUE(ccpa.isRuleCompliant(rule));
}

TEST_F(CcpaRuleSetTest, EvaluateNonCompliantRule) {
    // Missing audit_access → fails RightToKnow
    PolicyRule rule = makeRule("r1", false, true, false);
    EXPECT_FALSE(ccpa.isRuleCompliant(rule));
}

TEST_F(CcpaRuleSetTest, EvaluateRuleReturnsResultsForAllChecks) {
    PolicyRule rule = makeRule("r1");
    auto results = ccpa.evaluateRule(rule);
    EXPECT_EQ(results.size(), 4u); // 4 CCPA rules
    for (const auto& r : results) {
        EXPECT_FALSE(r.ccpa_check_id.empty());
        EXPECT_EQ(r.rule_id, "r1");
    }
}

// ============================================================================
// CcpaRuleSet – HIPAA conflict detection
// ============================================================================

TEST_F(CcpaRuleSetTest, NoHipaaConflictWhenBothAuditFlagsSet) {
    PolicyRule rule = makeRule("r1",
        /*audit_access*/  true,
        /*audit_changes*/ true,
        true, false, 2190);
    auto conflicts = ccpa.detectHipaaConflicts(rule);
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(CcpaRuleSetTest, HipaaConflictWhenAuditChangeMissing) {
    PolicyRule rule = makeRule("r1",
        /*audit_access*/  true,
        /*audit_changes*/ false,
        true, false, 2190);
    auto conflicts = ccpa.detectHipaaConflicts(rule);
    EXPECT_FALSE(conflicts.empty());
    // Description must mention both frameworks
    EXPECT_NE(conflicts[0].find("HIPAA"), std::string::npos);
    EXPECT_NE(conflicts[0].find("CCPA"),  std::string::npos);
}

TEST_F(CcpaRuleSetTest, HipaaConflictWhenRetentionBelowMinimum) {
    PolicyRule rule = makeRule("r1",
        /*audit_access*/  true,
        /*audit_changes*/ true,
        true, false, /*retention_days=*/365); // < 2190 HIPAA minimum
    auto conflicts = ccpa.detectHipaaConflicts(rule);
    EXPECT_FALSE(conflicts.empty());
}

TEST_F(CcpaRuleSetTest, NoHipaaConflictForDisabledRule) {
    PolicyRule rule = makeRule("r1", true, false, true, false, 100);
    rule.enabled = false;
    auto conflicts = ccpa.detectHipaaConflicts(rule);
    EXPECT_TRUE(conflicts.empty());
}

// ============================================================================
// CcpaRuleSet – data subject request recording
// ============================================================================

TEST_F(CcpaRuleSetTest, RecordAndRetrieveRequest) {
    DataSubjectRequest req;
    req.request_id   = "req-001";
    req.subject_id   = "user123";
    req.request_type = "right_to_delete";
    req.timestamp    = 1000;
    req.status       = "pending";
    ccpa.recordRequest(req);

    auto requests = ccpa.getRequestsForSubject("user123");
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0].request_id, "req-001");
}

TEST_F(CcpaRuleSetTest, GetRequestsByType) {
    DataSubjectRequest r1;
    r1.request_id   = "opt1";
    r1.subject_id   = "u1";
    r1.request_type = "opt_out_of_sale";
    r1.timestamp    = 500;
    r1.status       = "fulfilled";
    ccpa.recordRequest(r1);

    DataSubjectRequest r2;
    r2.request_id   = "del1";
    r2.subject_id   = "u2";
    r2.request_type = "right_to_delete";
    r2.timestamp    = 600;
    r2.status       = "pending";
    ccpa.recordRequest(r2);

    auto opt_outs = ccpa.getRequestsByType("opt_out_of_sale");
    EXPECT_EQ(opt_outs.size(), 1u);

    auto deletes = ccpa.getRequestsByType("right_to_delete");
    EXPECT_EQ(deletes.size(), 1u);
}

TEST_F(CcpaRuleSetTest, CountOptOutRequests) {
    for (int i = 0; i < 5; ++i) {
        DataSubjectRequest r;
        r.request_id   = "opt" + std::to_string(i);
        r.subject_id   = "u" + std::to_string(i);
        r.request_type = "opt_out_of_sale";
        r.timestamp    = static_cast<int64_t>(i * 100);
        r.status       = "fulfilled";
        ccpa.recordRequest(r);
    }
    EXPECT_EQ(ccpa.countOptOutRequests(), 5);
    // Within a time window
    EXPECT_EQ(ccpa.countOptOutRequests(100, 300), 3); // timestamps 100, 200, 300
}

// ============================================================================
// PolicyEngine CCPA opt-out integration
// ============================================================================

TEST(PolicyEngineWithCcpa, NotOptedOutByDefault) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers = {{"X-User-Id", "user42"}};
    auto decision = engine.evaluate(headers, "/api/data");
    EXPECT_FALSE(decision.ccpa_opted_out);
}

TEST(PolicyEngineWithCcpa, OptedOutSubjectBlocksExport) {
    PolicyEngine engine;

    auto registry = std::make_shared<std::unordered_set<std::string>>();
    registry->insert("opted-out-user");
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "opted-out-user"}
    };
    auto decision = engine.evaluate(headers, "/api/data");

    EXPECT_TRUE(decision.ccpa_opted_out);
    EXPECT_FALSE(decision.export_allowed);
}

TEST(PolicyEngineWithCcpa, NonOptedOutSubjectKeepsDefaultExport) {
    PolicyEngine engine;

    auto registry = std::make_shared<std::unordered_set<std::string>>();
    registry->insert("some-other-user");
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "normal-user"}
    };
    auto decision = engine.evaluate(headers, "/api/data");

    EXPECT_FALSE(decision.ccpa_opted_out);
}

TEST(PolicyEngineWithCcpa, IsCcpaOptedOut) {
    PolicyEngine engine;
    EXPECT_FALSE(engine.isCcpaOptedOut("user1"));

    auto registry = std::make_shared<std::unordered_set<std::string>>();
    registry->insert("user1");
    engine.setCcpaOptOutSubjects(registry);

    EXPECT_TRUE(engine.isCcpaOptedOut("user1"));
    EXPECT_FALSE(engine.isCcpaOptedOut("user2"));
}

TEST(PolicyEngineWithCcpa, EmptyRegistryNobodyOptedOut) {
    PolicyEngine engine;
    auto registry = std::make_shared<std::unordered_set<std::string>>();
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "any-user"}
    };
    auto decision = engine.evaluate(headers, "/api/data");
    EXPECT_FALSE(decision.ccpa_opted_out);
}

TEST(PolicyEngineWithCcpa, NullRegistryNobodyOptedOut) {
    PolicyEngine engine;
    engine.setCcpaOptOutSubjects(nullptr);

    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "any-user"}
    };
    auto decision = engine.evaluate(headers, "/api/data");
    EXPECT_FALSE(decision.ccpa_opted_out);
}

// ============================================================================
// ComplianceReporter::generateCcpaReport
// ============================================================================

class CcpaReportTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_mgr = std::make_unique<PolicyManager>();
    }

    std::unique_ptr<PolicyManager> policy_mgr;
    ComplianceReporter reporter;
};

TEST_F(CcpaReportTest, EmptyPoliciesProducesEmptyReport) {
    auto report = reporter.generateCcpaReport(*policy_mgr);
    EXPECT_EQ(report.ccpa_compliant_rules,     0);
    EXPECT_EQ(report.ccpa_non_compliant_rules,  0);
    EXPECT_TRUE(report.data_categories.empty());
    EXPECT_EQ(report.opt_out_count, 0);
}

TEST_F(CcpaReportTest, FullyCompliantRuleCountedCorrectly) {
    PolicyRule r = makeRule("r1", true, true, false, false, 365, "vs-nfd");
    policy_mgr->addRule(r);

    auto report = reporter.generateCcpaReport(*policy_mgr);
    EXPECT_EQ(report.ccpa_compliant_rules,    1);
    EXPECT_EQ(report.ccpa_non_compliant_rules, 0);
}

TEST_F(CcpaReportTest, NonCompliantRuleDetected) {
    // Missing audit_access → RightToKnow fails
    PolicyRule r = makeRule("r1", false, true, false, false, 365, "geheim");
    policy_mgr->addRule(r);

    auto report = reporter.generateCcpaReport(*policy_mgr);
    EXPECT_EQ(report.ccpa_non_compliant_rules, 1);
    EXPECT_FALSE(report.missing_right_to_know.empty());
}

TEST_F(CcpaReportTest, ThirdPartyDisclosureRulesIdentified) {
    PolicyRule r1 = makeRule("exporter", false, false, /*allow_export=*/true);
    PolicyRule r2 = makeRule("restricted", false, false, /*allow_export=*/false);
    policy_mgr->addRule(r1);
    policy_mgr->addRule(r2);

    auto report = reporter.generateCcpaReport(*policy_mgr);
    EXPECT_EQ(report.third_party_disclosure_rule_ids.size(), 1u);
    EXPECT_EQ(report.third_party_disclosure_rule_ids[0], "exporter");
}

TEST_F(CcpaReportTest, OptOutCountPassedThrough) {
    auto report = reporter.generateCcpaReport(*policy_mgr, /*opt_out_count=*/42);
    EXPECT_EQ(report.opt_out_count, 42);
}

TEST_F(CcpaReportTest, DataCategoriesCollectedFromRules) {
    PolicyRule r1 = makeRule("r1", true, true, false, false, 365, "offen");
    PolicyRule r2 = makeRule("r2", true, true, false, false, 365, "geheim");
    policy_mgr->addRule(r1);
    policy_mgr->addRule(r2);

    auto report = reporter.generateCcpaReport(*policy_mgr);
    EXPECT_GE(report.data_categories.size(), 2u);
}

TEST_F(CcpaReportTest, ToJsonContainsExpectedKeys) {
    auto report = reporter.generateCcpaReport(*policy_mgr, 7);
    auto j = report.toJson();
    EXPECT_TRUE(j.contains("opt_out_count"));
    EXPECT_TRUE(j.contains("ccpa_compliant_rules"));
    EXPECT_TRUE(j.contains("ccpa_non_compliant_rules"));
    EXPECT_TRUE(j.contains("data_categories"));
    EXPECT_TRUE(j.contains("third_party_disclosure_rule_ids"));
    EXPECT_TRUE(j.contains("missing_right_to_know"));
    EXPECT_TRUE(j.contains("missing_right_to_delete"));
    EXPECT_TRUE(j.contains("missing_opt_out_of_sale"));
    EXPECT_TRUE(j.contains("missing_data_portability"));
    EXPECT_EQ(j["opt_out_count"].get<int>(), 7);
}

// ============================================================================
// PolicyValidator CCPA/HIPAA cross-framework conflict detection
// ============================================================================

TEST(PolicyValidatorCcpaHipaa, NoConflictsForCompliantRule) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*audit_access*/  true,
        /*audit_changes*/ true,
        true, false, 2190);
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectCcpaHipaaConflicts(mgr);
    EXPECT_TRUE(conflicts.empty());
}

TEST(PolicyValidatorCcpaHipaa, DetectsAuditChangesMissing) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*audit_access*/  true,
        /*audit_changes*/ false, // triggers HIPAA/CCPA conflict
        true, false, 2190);
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectCcpaHipaaConflicts(mgr);
    EXPECT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "ccpa_hipaa");
    EXPECT_FALSE(conflicts[0].description.empty());
    EXPECT_FALSE(conflicts[0].recommendation.empty());
}

TEST(PolicyValidatorCcpaHipaa, IntegratedIntoDetectConflicts) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*audit_access*/  true,
        /*audit_changes*/ false,
        true, false, 2190);
    mgr.addRule(rule);

    PolicyValidator validator;
    auto all_conflicts = validator.detectConflicts(mgr);
    bool found_ccpa_hipaa = false;
    for (const auto& c : all_conflicts) {
        if (c.conflict_type == "ccpa_hipaa") {
            found_ccpa_hipaa = true;
        }
    }
    EXPECT_TRUE(found_ccpa_hipaa);
}

TEST(PolicyValidatorCcpaHipaa, DisabledRulesSkipped) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1", true, false, true, false, 100);
    rule.enabled = false;
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectCcpaHipaaConflicts(mgr);
    EXPECT_TRUE(conflicts.empty());
}

// ============================================================================
// DataSubjectRequest JSON serialization
// ============================================================================

TEST(DataSubjectRequest, ToJson) {
    DataSubjectRequest req;
    req.request_id   = "req-xyz";
    req.subject_id   = "consumer-001";
    req.request_type = "right_to_know";
    req.timestamp    = 1234567890;
    req.status       = "fulfilled";
    req.denial_reason = "";

    auto j = req.toJson();
    EXPECT_EQ(j["request_id"].get<std::string>(),   "req-xyz");
    EXPECT_EQ(j["subject_id"].get<std::string>(),   "consumer-001");
    EXPECT_EQ(j["request_type"].get<std::string>(), "right_to_know");
    EXPECT_EQ(j["status"].get<std::string>(),       "fulfilled");
}

// ============================================================================
// CcpaRuleEvalResult JSON serialization
// ============================================================================

TEST(CcpaRuleEvalResult, ToJson) {
    CcpaRuleEvalResult res;
    res.rule_id       = "pol-1";
    res.ccpa_check_id = "ccpa_right_to_know";
    res.compliant     = true;
    res.description   = "passes";
    res.recommendation = "";

    auto j = res.toJson();
    EXPECT_EQ(j["rule_id"].get<std::string>(),       "pol-1");
    EXPECT_EQ(j["ccpa_check_id"].get<std::string>(), "ccpa_right_to_know");
    EXPECT_TRUE(j["compliant"].get<bool>());
}
