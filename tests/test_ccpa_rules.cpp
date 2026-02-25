/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_ccpa_rules.cpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_ccpa_rules.cpp
 * @brief Unit tests for the CCPA/CPRA compliance rule set
 *
 * Tests cover:
 * - Opt-out of sale / sharing (§ 1798.120 / CPRA § 1798.135)
 * - Right to delete (§ 1798.105)
 * - Right to know (§ 1798.110 / § 1798.115)
 * - Data portability (§ 1798.100)
 * - evaluateAll() aggregate logic
 * - generateReport() aggregation
 * - CcpaSubjectRecord JSON serialisation round-trip
 * - CcpaReport JSON serialisation
 */

#include <gtest/gtest.h>
#include "governance/ccpa_rules.h"
#include <chrono>

using namespace themis::governance;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static CcpaSubjectRecord makeSubject(
    const std::string& id,
    bool opt_out_sale    = false,
    bool opt_out_sharing = false,
    bool del_req         = false,
    bool rtk_req         = false,
    bool port_req        = false
) {
    CcpaSubjectRecord rec;
    rec.subject_id             = id;
    rec.opt_out_of_sale        = opt_out_sale;
    rec.opt_out_of_sharing     = opt_out_sharing;
    rec.deletion_requested     = del_req;
    rec.right_to_know_requested= rtk_req;
    rec.portability_requested  = port_req;
    rec.data_categories        = {"email", "purchase_history"};
    rec.third_party_disclosures= {"analytics_co", "ad_network"};
    rec.last_updated           = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return rec;
}

static CcpaEvaluationContext makeCtx(
    const std::string& subject_id,
    const std::string& action,
    bool is_third_party    = false,
    bool is_service_provider = false
) {
    CcpaEvaluationContext ctx;
    ctx.subject_id        = subject_id;
    ctx.requesting_party  = "test_consumer";
    ctx.action            = action;
    ctx.is_third_party    = is_third_party;
    ctx.is_service_provider = is_service_provider;
    return ctx;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class CcpaRulesTest : public ::testing::Test {
protected:
    CcpaRuleSet rules;

    void SetUp() override {
        rules.registerSubject(makeSubject("alice",
            /*opt_out_sale*/   true,
            /*opt_out_sharing*/false,
            /*del_req*/        false,
            /*rtk_req*/        true,
            /*port_req*/       false));

        rules.registerSubject(makeSubject("bob",
            /*opt_out_sale*/   false,
            /*opt_out_sharing*/true,
            /*del_req*/        true,
            /*rtk_req*/        false,
            /*port_req*/       true));

        // carol has no flags set – plain registered subject
        rules.registerSubject(makeSubject("carol"));
    }
};

// ---------------------------------------------------------------------------
// Registration / Lookup
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, RegisterAndLookup) {
    EXPECT_EQ(rules.subjectCount(), 3);
    auto alice = rules.getSubject("alice");
    ASSERT_TRUE(alice.has_value());
    EXPECT_EQ(alice->subject_id, "alice");
    EXPECT_FALSE(rules.getSubject("nonexistent").has_value());
}

TEST_F(CcpaRulesTest, RemoveSubject) {
    EXPECT_TRUE(rules.removeSubject("carol"));
    EXPECT_EQ(rules.subjectCount(), 2);
    EXPECT_EQ(rules.getSubject("carol"), nullptr);

    // Removing a non-existent subject returns false
    EXPECT_FALSE(rules.removeSubject("nobody"));
}

TEST_F(CcpaRulesTest, RegisterOverwritesExisting) {
    auto updated = makeSubject("alice");
    updated.opt_out_of_sale = false;  // flip the flag
    rules.registerSubject(updated);

    auto alice = rules.getSubject("alice");
    ASSERT_TRUE(alice.has_value());
    EXPECT_FALSE(alice->opt_out_of_sale);
    EXPECT_EQ(rules.subjectCount(), 3);  // count unchanged
}

// ---------------------------------------------------------------------------
// Opt-Out of Sale
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, OptOutOfSale_Blocked_WhenOptedOut) {
    auto result = rules.evaluateOptOutOfSale(makeCtx("alice", "sell"));
    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(result.rule_id, CcpaRuleSet::RULE_OPT_OUT_OF_SALE);
    EXPECT_FALSE(result.required_actions.empty());
}

TEST_F(CcpaRulesTest, OptOutOfSale_Allowed_WhenNotOptedOut) {
    auto result = rules.evaluateOptOutOfSale(makeCtx("bob", "sell"));
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, OptOutOfSale_ServiceProviderExempt) {
    auto ctx = makeCtx("alice", "sell", /*third_party*/true, /*service_provider*/true);
    auto result = rules.evaluateOptOutOfSale(ctx);
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, OptOutOfSale_IrrelevantAction) {
    auto result = rules.evaluateOptOutOfSale(makeCtx("alice", "read"));
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, OptOutOfSale_UnknownSubject_Allowed) {
    auto result = rules.evaluateOptOutOfSale(makeCtx("unknown_user", "sell"));
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, OptOutOfSharing_Blocked_WhenOptedOut) {
    auto result = rules.evaluateOptOutOfSale(makeCtx("bob", "share"));
    EXPECT_FALSE(result.allowed);
}

TEST_F(CcpaRulesTest, OptOutOfSharing_Allowed_WhenNotOptedOut) {
    auto result = rules.evaluateOptOutOfSale(makeCtx("alice", "share"));
    EXPECT_TRUE(result.allowed);
}

// ---------------------------------------------------------------------------
// Right to Delete
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, RightToDelete_Blocked_WhenPending) {
    auto result = rules.evaluateRightToDelete(makeCtx("bob", "read"));
    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(result.rule_id, CcpaRuleSet::RULE_RIGHT_TO_DELETE);
}

TEST_F(CcpaRulesTest, RightToDelete_Allowed_WhenNotPending) {
    auto result = rules.evaluateRightToDelete(makeCtx("alice", "read"));
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, RightToDelete_IrrelevantAction) {
    // "delete" action itself is not blocked by this rule
    auto result = rules.evaluateRightToDelete(makeCtx("bob", "delete"));
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, RightToDelete_ShareBlocked_WhenPending) {
    auto result = rules.evaluateRightToDelete(makeCtx("bob", "share"));
    EXPECT_FALSE(result.allowed);
}

// ---------------------------------------------------------------------------
// Right to Know
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, RightToKnow_Allowed_WhenRequestOnRecord) {
    auto result = rules.evaluateRightToKnow(
        makeCtx("alice", "disclose_categories"));
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.rule_id, CcpaRuleSet::RULE_RIGHT_TO_KNOW);
    EXPECT_FALSE(result.required_actions.empty());
}

TEST_F(CcpaRulesTest, RightToKnow_Blocked_WhenNoRequest) {
    auto result = rules.evaluateRightToKnow(
        makeCtx("bob", "disclose_categories"));
    EXPECT_FALSE(result.allowed);
}

TEST_F(CcpaRulesTest, RightToKnow_UnknownSubject_AllowedEmptyDisclosure) {
    auto result = rules.evaluateRightToKnow(
        makeCtx("ghost", "disclose_categories"));
    EXPECT_TRUE(result.allowed);
    EXPECT_FALSE(result.required_actions.empty());
}

TEST_F(CcpaRulesTest, RightToKnow_IrrelevantAction) {
    auto result = rules.evaluateRightToKnow(makeCtx("alice", "read"));
    EXPECT_TRUE(result.allowed);
}

// ---------------------------------------------------------------------------
// Data Portability
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, DataPortability_Allowed_WhenRequestOnRecord) {
    auto result = rules.evaluateDataPortability(makeCtx("bob", "export"));
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.rule_id, CcpaRuleSet::RULE_DATA_PORTABILITY);
}

TEST_F(CcpaRulesTest, DataPortability_Blocked_WhenNoRequest) {
    auto result = rules.evaluateDataPortability(makeCtx("alice", "export"));
    EXPECT_FALSE(result.allowed);
}

TEST_F(CcpaRulesTest, DataPortability_IrrelevantAction) {
    auto result = rules.evaluateDataPortability(makeCtx("bob", "read"));
    EXPECT_TRUE(result.allowed);
}

TEST_F(CcpaRulesTest, DataPortability_UnknownSubject_Blocked) {
    auto result = rules.evaluateDataPortability(makeCtx("nobody", "export"));
    EXPECT_FALSE(result.allowed);
}

// ---------------------------------------------------------------------------
// evaluateAll
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, EvaluateAll_DenyBeatsAllow) {
    // alice has opt_out_sale=true; selling should be denied
    std::vector<CcpaEvaluationResult> individual;
    auto result = rules.evaluateAll(makeCtx("alice", "sell"), &individual);
    EXPECT_FALSE(result.allowed);
    EXPECT_EQ(individual.size(), 4u);  // four rules evaluated
}

TEST_F(CcpaRulesTest, EvaluateAll_AllAllowed_WhenNoFlags) {
    // carol has no flags; a "read" action should pass all rules
    std::vector<CcpaEvaluationResult> individual;
    auto result = rules.evaluateAll(makeCtx("carol", "read"), &individual);
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(individual.size(), 4u);
}

TEST_F(CcpaRulesTest, EvaluateAll_IndividualResultsOptional) {
    // Should not crash when nullptr is passed for individual results
    EXPECT_NO_THROW(rules.evaluateAll(makeCtx("alice", "read")));
}

// ---------------------------------------------------------------------------
// generateReport
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, GenerateReport_Counts) {
    CcpaReport report = rules.generateReport();

    // All 3 subjects have last_updated > 0 so they fall in the open window
    EXPECT_EQ(report.total_subjects, 3);
    EXPECT_EQ(report.opted_out_of_sale, 1);   // alice
    EXPECT_EQ(report.opted_out_of_sharing, 1); // bob
    EXPECT_EQ(report.deletion_requests, 1);    // bob
    EXPECT_EQ(report.right_to_know_requests, 1); // alice
    EXPECT_EQ(report.portability_requests, 1); // bob
}

TEST_F(CcpaRulesTest, GenerateReport_CategoryAndDisclosureCounts) {
    CcpaReport report = rules.generateReport();

    // Each subject has {"email", "purchase_history"} so each category count = 3
    EXPECT_EQ(report.subjects_by_category.at("email"), 3);
    EXPECT_EQ(report.subjects_by_category.at("purchase_history"), 3);

    // Each subject lists {"analytics_co", "ad_network"}
    EXPECT_EQ(report.disclosures_by_third_party.at("analytics_co"), 3);
    EXPECT_EQ(report.disclosures_by_third_party.at("ad_network"), 3);
}

TEST_F(CcpaRulesTest, GenerateReport_HasReportId) {
    CcpaReport report = rules.generateReport();
    EXPECT_FALSE(report.report_id.empty());
    EXPECT_NE(report.generated_at, 0);
}

TEST_F(CcpaRulesTest, GenerateReport_TimeWindowFilter) {
    // Use a window in the past so no subjects qualify (last_updated is now)
    CcpaReport report = rules.generateReport(0, 1000); // window ends at 1 second epoch
    EXPECT_EQ(report.total_subjects, 0);
}

// ---------------------------------------------------------------------------
// JSON serialisation round-trip
// ---------------------------------------------------------------------------

TEST_F(CcpaRulesTest, SubjectRecord_JsonRoundTrip) {
    CcpaSubjectRecord original = makeSubject("dave", true, true, true, true, true);
    original.third_party_disclosures = {"partner_a"};
    original.data_categories = {"geo_location"};

    nlohmann::json j = original.toJson();
    CcpaSubjectRecord restored = CcpaSubjectRecord::fromJson(j);

    EXPECT_EQ(restored.subject_id, original.subject_id);
    EXPECT_EQ(restored.opt_out_of_sale, original.opt_out_of_sale);
    EXPECT_EQ(restored.opt_out_of_sharing, original.opt_out_of_sharing);
    EXPECT_EQ(restored.deletion_requested, original.deletion_requested);
    EXPECT_EQ(restored.right_to_know_requested, original.right_to_know_requested);
    EXPECT_EQ(restored.portability_requested, original.portability_requested);
    EXPECT_EQ(restored.data_categories, original.data_categories);
    EXPECT_EQ(restored.third_party_disclosures, original.third_party_disclosures);
    EXPECT_EQ(restored.opt_out_timestamp, original.opt_out_timestamp);
    EXPECT_EQ(restored.last_updated, original.last_updated);
}

TEST_F(CcpaRulesTest, Report_JsonContainsRequiredFields) {
    CcpaReport report = rules.generateReport();
    nlohmann::json j = report.toJson();

    EXPECT_TRUE(j.contains("report_id"));
    EXPECT_TRUE(j.contains("generated_at"));
    EXPECT_TRUE(j.contains("total_subjects"));
    EXPECT_TRUE(j.contains("opted_out_of_sale"));
    EXPECT_TRUE(j.contains("opted_out_of_sharing"));
    EXPECT_TRUE(j.contains("deletion_requests"));
    EXPECT_TRUE(j.contains("right_to_know_requests"));
    EXPECT_TRUE(j.contains("portability_requests"));
    EXPECT_TRUE(j.contains("subjects_by_category"));
    EXPECT_TRUE(j.contains("disclosures_by_third_party"));
}
