/**
 * @file test_model_governance.cpp
 * @brief Unit tests for AI/ML Model Governance
 *
 * Tests cover:
 * - ModelTrainingExportRequest::toJson()
 * - ModelGovernanceDecision::toJson()
 * - ModelGovernancePolicy::checkExportPermission() – permit and deny paths
 * - ModelGovernancePolicy::addRestrictedCollection() / isCollectionRestricted()
 * - LineageEventType::MODEL_TRAINING is recorded by DataLineageTracker
 * - BiasFieldStats::toJson()
 * - BiasAuditReport::toJson()
 * - ComplianceReporter::generateBiasAuditReport() – PASSED / FLAGGED / FAILED
 * - PolicyEngine::checkExportPermission() – integration via PolicyEngine facade
 */

#include <gtest/gtest.h>
#include "governance/model_governance.h"
#include "governance/policy_engine.h"
#include "governance/compliance_reporter.h"
#include "governance/data_lineage.h"
#include "governance/policy_manager.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>

using namespace themis::governance;
using json = nlohmann::json;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static ModelTrainingExportRequest makeRequest(
    const std::string& cls = "vs-nfd",
    const std::vector<std::string>& collections = {"col_a"},
    const std::string& adapter = "adapter-1",
    const std::string& job = "job-001",
    const std::string& user = "ml-service")
{
    ModelTrainingExportRequest req;
    req.export_job_id   = job;
    req.collection_ids  = collections;
    req.field_selectors = {"content", "label"};
    req.requesting_user = user;
    req.adapter_id      = adapter;
    req.classification  = cls;
    return req;
}

// ─── ModelTrainingExportRequest ───────────────────────────────────────────────

TEST(ModelTrainingExportRequestTest, ToJsonContainsAllFields) {
    auto req = makeRequest("vs-nfd", {"col_a", "col_b"}, "adapter-42", "job-x", "alice");
    json j = req.toJson();

    EXPECT_EQ(j["export_job_id"],   "job-x");
    EXPECT_EQ(j["adapter_id"],      "adapter-42");
    EXPECT_EQ(j["requesting_user"], "alice");
    EXPECT_EQ(j["classification"],  "vs-nfd");
    EXPECT_EQ(j["purpose"],         "MODEL_TRAINING");
    ASSERT_EQ(j["collection_ids"].size(), 2u);
    ASSERT_EQ(j["field_selectors"].size(), 2u);
}

// ─── ModelGovernanceDecision ──────────────────────────────────────────────────

TEST(ModelGovernanceDecisionTest, ToJsonPermitted) {
    ModelGovernanceDecision d;
    d.is_permitted     = true;
    d.lineage_event_id = "lineage-1";
    json j = d.toJson();
    EXPECT_TRUE(j["is_permitted"].get<bool>());
    EXPECT_EQ(j["lineage_event_id"], "lineage-1");
    EXPECT_EQ(j["denial_reason"],    "");
}

TEST(ModelGovernanceDecisionTest, ToJsonDenied) {
    ModelGovernanceDecision d;
    d.is_permitted  = false;
    d.denial_reason = "Classification 'geheim' is not permitted";
    json j = d.toJson();
    EXPECT_FALSE(j["is_permitted"].get<bool>());
    EXPECT_EQ(j["denial_reason"], "Classification 'geheim' is not permitted");
}

// ─── ModelGovernancePolicy ────────────────────────────────────────────────────

class ModelGovernancePolicyTest : public ::testing::Test {
protected:
    ModelGovernancePolicy policy;
    std::shared_ptr<DataLineageTracker> tracker{std::make_shared<DataLineageTracker>()};

    void SetUp() override {
        policy.setLineageTracker(tracker);
    }
};

TEST_F(ModelGovernancePolicyTest, PermitsUnclassifiedRequest) {
    auto req = makeRequest("vs-nfd");
    auto dec = policy.checkExportPermission(req);
    EXPECT_TRUE(dec.is_permitted);
    EXPECT_TRUE(dec.denial_reason.empty());
}

TEST_F(ModelGovernancePolicyTest, PermitsOffenClassification) {
    auto req = makeRequest("offen");
    auto dec = policy.checkExportPermission(req);
    EXPECT_TRUE(dec.is_permitted);
}

TEST_F(ModelGovernancePolicyTest, DeniesGeheimClassification) {
    auto req = makeRequest("geheim");
    auto dec = policy.checkExportPermission(req);
    EXPECT_FALSE(dec.is_permitted);
    EXPECT_FALSE(dec.denial_reason.empty());
}

TEST_F(ModelGovernancePolicyTest, DeniesStrengGeheimClassification) {
    auto req = makeRequest("streng-geheim");
    auto dec = policy.checkExportPermission(req);
    EXPECT_FALSE(dec.is_permitted);
    EXPECT_FALSE(dec.denial_reason.empty());
}

TEST_F(ModelGovernancePolicyTest, ClassificationCheckIsCaseInsensitive) {
    // Mixed case variants of restricted classifications must also be denied
    for (const auto& cls : {"GEHEIM", "Geheim", "STRENG-GEHEIM", "Streng-Geheim"}) {
        auto req = makeRequest(cls);
        auto dec = policy.checkExportPermission(req);
        EXPECT_FALSE(dec.is_permitted) << "Expected denial for classification: " << cls;
    }
}

TEST_F(ModelGovernancePolicyTest, DeniesRestrictedCollection) {
    policy.addRestrictedCollection("restricted_col");
    auto req = makeRequest("vs-nfd", {"restricted_col"});
    auto dec = policy.checkExportPermission(req);
    EXPECT_FALSE(dec.is_permitted);
    EXPECT_FALSE(dec.denial_reason.empty());
}

TEST_F(ModelGovernancePolicyTest, PermitsAfterCollectionRemovedFromRestricted) {
    policy.addRestrictedCollection("temp_col");
    EXPECT_TRUE(policy.isCollectionRestricted("temp_col"));
    policy.removeRestrictedCollection("temp_col");
    EXPECT_FALSE(policy.isCollectionRestricted("temp_col"));

    auto req = makeRequest("vs-nfd", {"temp_col"});
    auto dec = policy.checkExportPermission(req);
    EXPECT_TRUE(dec.is_permitted);
}

TEST_F(ModelGovernancePolicyTest, DeniesIfAnyCollectionRestricted) {
    policy.addRestrictedCollection("secret_col");
    auto req = makeRequest("vs-nfd", {"open_col", "secret_col", "another_col"});
    auto dec = policy.checkExportPermission(req);
    EXPECT_FALSE(dec.is_permitted);
}

TEST_F(ModelGovernancePolicyTest, RecordsLineageOnApproval) {
    const size_t before = tracker->totalEventCount();
    auto req = makeRequest("vs-nfd", {"col_x"}, "adapter-lora", "job-lin");
    auto dec = policy.checkExportPermission(req);

    ASSERT_TRUE(dec.is_permitted);
    EXPECT_EQ(tracker->totalEventCount(), before + 1);

    // Verify the recorded event is of MODEL_TRAINING type
    auto record = tracker->getLineage("job-lin");
    ASSERT_EQ(record.events.size(), 1u);
    EXPECT_EQ(record.events[0].event_type, LineageEventType::MODEL_TRAINING);
    EXPECT_EQ(record.events[0].performed_by, "ml-service");
    EXPECT_EQ(record.events[0].metadata["adapter_id"], "adapter-lora");
}

TEST_F(ModelGovernancePolicyTest, DoesNotRecordLineageOnDenial) {
    const size_t before = tracker->totalEventCount();
    auto req = makeRequest("geheim");
    auto dec = policy.checkExportPermission(req);

    ASSERT_FALSE(dec.is_permitted);
    EXPECT_EQ(tracker->totalEventCount(), before);
}

TEST_F(ModelGovernancePolicyTest, LineageEventIdPopulatedOnApproval) {
    auto req = makeRequest("vs-nfd", {"col_a"}, "adapter-id", "job-evid");
    auto dec = policy.checkExportPermission(req);
    ASSERT_TRUE(dec.is_permitted);
    EXPECT_FALSE(dec.lineage_event_id.empty());
}

// ─── LineageEventType::MODEL_TRAINING ────────────────────────────────────────

TEST(ModelTrainingLineageEventTypeTest, ToStringReturnsCorrectLabel) {
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::MODEL_TRAINING),
              "MODEL_TRAINING");
}

TEST(ModelTrainingLineageEventTypeTest, RecordableByDataLineageTracker) {
    DataLineageTracker tracker;
    LineageEvent ev;
    ev.event_id    = "ml-ev-1";
    ev.dataset_id  = "training-ds";
    ev.event_type  = LineageEventType::MODEL_TRAINING;
    ev.performed_by = "exporter";
    ev.operation   = "export_for_training";
    ev.metadata    = {{"adapter_id", "lora-42"}};
    tracker.recordEvent(ev);

    auto record = tracker.getLineage("training-ds");
    ASSERT_EQ(record.events.size(), 1u);
    EXPECT_EQ(record.events[0].event_type, LineageEventType::MODEL_TRAINING);
    EXPECT_EQ(record.events[0].metadata["adapter_id"], "lora-42");
}

// ─── BiasFieldStats ───────────────────────────────────────────────────────────

TEST(BiasFieldStatsTest, ToJsonContainsRequiredFields) {
    BiasFieldStats fs;
    fs.field_name              = "gender";
    fs.group_counts            = {{"male", 600}, {"female", 400}};
    fs.total_count             = 1000;
    fs.representation_ratio    = 0.667;
    fs.demographic_parity_score = 0.97;

    json j = fs.toJson();
    EXPECT_EQ(j["field_name"],                "gender");
    EXPECT_EQ(j["total_count"],               1000u);
    EXPECT_DOUBLE_EQ(j["representation_ratio"].get<double>(),      0.667);
    EXPECT_DOUBLE_EQ(j["demographic_parity_score"].get<double>(),  0.97);
    EXPECT_EQ(j["group_counts"]["male"],  600u);
    EXPECT_EQ(j["group_counts"]["female"], 400u);
}

// ─── BiasAuditReport ─────────────────────────────────────────────────────────

class BiasAuditReportTest : public ::testing::Test {
protected:
    std::shared_ptr<PolicyManager> pm{std::make_shared<PolicyManager>()};
    ComplianceReporter reporter{pm};
};

TEST_F(BiasAuditReportTest, ToJsonContainsRequiredFields) {
    BiasAuditReport report;
    report.report_id          = "bias-r-1";
    report.adapter_id         = "adapter-x";
    report.dataset_id         = "job-y";
    report.generated_at       = 1700000000000LL;
    report.overall_bias_score = 0.85;
    report.status             = "PASSED";
    report.recommendations    = {};

    BiasFieldStats fs;
    fs.field_name = "age_group";
    fs.group_counts = {{"young", 500}, {"old", 500}};
    fs.total_count = 1000;
    fs.representation_ratio = 1.0;
    fs.demographic_parity_score = 1.0;
    report.field_stats.push_back(fs);

    json j = report.toJson();
    EXPECT_EQ(j["report_id"],          "bias-r-1");
    EXPECT_EQ(j["adapter_id"],         "adapter-x");
    EXPECT_EQ(j["dataset_id"],         "job-y");
    EXPECT_EQ(j["status"],             "PASSED");
    EXPECT_DOUBLE_EQ(j["overall_bias_score"].get<double>(), 0.85);
    ASSERT_EQ(j["field_stats"].size(), 1u);
    EXPECT_EQ(j["field_stats"][0]["field_name"], "age_group");
}

// ─── ComplianceReporter::generateBiasAuditReport ─────────────────────────────

TEST_F(BiasAuditReportTest, PassedWhenGroupsAreBalanced) {
    // Uniform distribution → parity ≈ 1.0 → PASSED
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats = {
        {"gender", {{"male", 500}, {"female", 500}}}
    };
    auto report = reporter.generateBiasAuditReport("adapter-1", "job-1", stats);
    EXPECT_EQ(report.adapter_id, "adapter-1");
    EXPECT_EQ(report.dataset_id, "job-1");
    EXPECT_EQ(report.status,     "PASSED");
    EXPECT_GE(report.overall_bias_score, 0.8);
    EXPECT_EQ(report.field_stats.size(), 1u);
}

TEST_F(BiasAuditReportTest, FlaggedWhenRepresentationIsSkewed) {
    // 9:1 ratio — should lower parity and yield FLAGGED or FAILED
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats = {
        {"ethnicity", {{"group_a", 900}, {"group_b", 100}}}
    };
    auto report = reporter.generateBiasAuditReport("adapter-2", "job-2", stats);
    EXPECT_NE(report.status, "PASSED");  // must be FLAGGED or FAILED
    EXPECT_FALSE(report.recommendations.empty());
}

TEST_F(BiasAuditReportTest, EmptyFieldStatsYieldsNoBiasScore) {
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats;
    auto report = reporter.generateBiasAuditReport("adapter-3", "job-3", stats);
    EXPECT_DOUBLE_EQ(report.overall_bias_score, 1.0);
    EXPECT_EQ(report.status, "PASSED");
    EXPECT_TRUE(report.field_stats.empty());
}

TEST_F(BiasAuditReportTest, MultipleFieldsAggregated) {
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats = {
        {"gender",    {{"male", 500}, {"female", 500}}},   // balanced
        {"age_group", {{"18-30", 800}, {"31-50", 200}}}    // skewed
    };
    auto report = reporter.generateBiasAuditReport("adapter-4", "job-4", stats);
    EXPECT_EQ(report.field_stats.size(), 2u);
    // aggregate score is the mean of both fields; one balanced + one skewed
    EXPECT_GT(report.overall_bias_score, 0.0);
    EXPECT_LE(report.overall_bias_score, 1.0);
}

TEST_F(BiasAuditReportTest, FieldWithEmptyGroupCountsIsSkipped) {
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats = {
        {"empty_field", {}},
        {"gender",      {{"male", 500}, {"female", 500}}}
    };
    auto report = reporter.generateBiasAuditReport("adapter-5", "job-5", stats);
    // Only "gender" should appear in field_stats
    EXPECT_EQ(report.field_stats.size(), 1u);
    EXPECT_EQ(report.field_stats[0].field_name, "gender");
}

TEST_F(BiasAuditReportTest, ReportIdIsNonEmpty) {
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats = {
        {"g", {{"a", 1}, {"b", 1}}}
    };
    auto report = reporter.generateBiasAuditReport("a", "b", stats);
    EXPECT_FALSE(report.report_id.empty());
}

TEST_F(BiasAuditReportTest, GeneratedAtIsPositive) {
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stats = {
        {"g", {{"a", 1}, {"b", 1}}}
    };
    auto report = reporter.generateBiasAuditReport("a", "b", stats);
    EXPECT_GT(report.generated_at, 0LL);
}

// ─── PolicyEngine::checkExportPermission ─────────────────────────────────────
// Verifies that PolicyEngine (the Exporters module entry point) correctly
// delegates to ModelGovernancePolicy and applies the built-in fallback when
// no ModelGovernancePolicy is attached.

class PolicyEngineExportPermissionTest : public ::testing::Test {
protected:
    PolicyEngine engine;
    std::shared_ptr<DataLineageTracker> tracker{std::make_shared<DataLineageTracker>()};
};

TEST_F(PolicyEngineExportPermissionTest, FallbackPermitsUnclassifiedRequest) {
    // No ModelGovernancePolicy attached → fallback path
    auto req = makeRequest("vs-nfd");
    auto dec = engine.checkExportPermission(req);
    EXPECT_TRUE(dec.is_permitted);
    EXPECT_TRUE(dec.denial_reason.empty());
}

TEST_F(PolicyEngineExportPermissionTest, FallbackDeniesGeheim) {
    auto req = makeRequest("geheim");
    auto dec = engine.checkExportPermission(req);
    EXPECT_FALSE(dec.is_permitted);
    EXPECT_FALSE(dec.denial_reason.empty());
}

TEST_F(PolicyEngineExportPermissionTest, FallbackDeniesStrengGeheim) {
    auto req = makeRequest("streng-geheim");
    auto dec = engine.checkExportPermission(req);
    EXPECT_FALSE(dec.is_permitted);
    EXPECT_FALSE(dec.denial_reason.empty());
}

TEST_F(PolicyEngineExportPermissionTest, DelegatesWhenModelGovernancePolicySet) {
    auto mgp = std::make_shared<ModelGovernancePolicy>();
    mgp->setLineageTracker(tracker);
    mgp->addRestrictedCollection("restricted_col");
    engine.setModelGovernancePolicy(mgp);

    // Permitted: no restricted collection, allowed classification
    auto req_ok = makeRequest("offen", {"open_col"}, "adapter-eng", "job-eng-ok");
    EXPECT_TRUE(engine.checkExportPermission(req_ok).is_permitted);

    // Denied: restricted collection
    auto req_deny = makeRequest("offen", {"restricted_col"}, "adapter-eng", "job-eng-deny");
    EXPECT_FALSE(engine.checkExportPermission(req_deny).is_permitted);
}

TEST_F(PolicyEngineExportPermissionTest, DelegateRecordsLineageInTracker) {
    auto mgp = std::make_shared<ModelGovernancePolicy>();
    mgp->setLineageTracker(tracker);
    engine.setModelGovernancePolicy(mgp);

    auto req = makeRequest("vs-nfd", {"col_a"}, "adapter-pe", "job-pe-lin");
    auto dec = engine.checkExportPermission(req);
    ASSERT_TRUE(dec.is_permitted);

    auto record = tracker->getLineage("job-pe-lin");
    ASSERT_EQ(record.events.size(), 1u);
    EXPECT_EQ(record.events[0].event_type, LineageEventType::MODEL_TRAINING);
}

TEST_F(PolicyEngineExportPermissionTest, FallbackCaseInsensitive) {
    for (const auto& cls : {"GEHEIM", "Geheim", "STRENG-GEHEIM"}) {
        auto req = makeRequest(cls);
        EXPECT_FALSE(engine.checkExportPermission(req).is_permitted)
            << "Expected denial for classification: " << cls;
    }
}
