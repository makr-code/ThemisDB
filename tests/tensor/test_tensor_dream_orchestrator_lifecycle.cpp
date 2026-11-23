#include <gtest/gtest.h>

#include "tensor/dream_orchestrator.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

using themis::tensor::DreamOrchestrator;
using themis::tensor::DreamGateStatus;
using themis::tensor::DreamRunAuditReasonClass;
using themis::tensor::DreamRunDescriptor;
using themis::tensor::DreamRunState;
using themis::tensor::DreamValidationEvidence;

namespace {

DreamValidationEvidence passingEvidence() {
    DreamValidationEvidence evidence;
    evidence.isolation_check_completed = true;
    evidence.truth_path_firewall_passed = true;
    evidence.label_integrity_check_completed = true;
    evidence.provenance_labels_valid = true;
    evidence.utility_delta_recall_at_10 = 0.02;
    evidence.utility_delta_ndcg_at_10 = 0.01;
    evidence.online_hallucination_delta = 0.0;
    evidence.online_error_delta = 0.0;
    evidence.ethics_check_completed = true;
    evidence.security_check_completed = true;
    return evidence;
}

void expectReportConformsToSchema(const nlohmann::json& report) {
    std::ifstream report_schema_file(THEMIS_TENSOR_DREAM_GATE_REPORT_SCHEMA_PATH);
    std::ifstream manifest_schema_file(THEMIS_TENSOR_DREAM_MANIFEST_SCHEMA_PATH);
    ASSERT_TRUE(report_schema_file.good());
    ASSERT_TRUE(manifest_schema_file.good());
    const auto report_schema = nlohmann::json::parse(report_schema_file);
    const auto manifest_schema = nlohmann::json::parse(manifest_schema_file);

    const auto& report_required = report_schema.at("required");
    EXPECT_EQ(report.size(), report_required.size());
    for (const auto& property : report_required) {
        EXPECT_TRUE(report.contains(property.get<std::string>()));
    }
    EXPECT_EQ(report.at("report_version"), "1.0");

    const auto& properties = report_schema.at("properties");
    const auto& gate_names = properties.at("gates").at("required");
    const auto& allowed_statuses =
        manifest_schema.at("$defs").at("gateStatus").at("enum");
    const auto has_allowed_status = [&allowed_statuses](const nlohmann::json& value) {
        return std::find(allowed_statuses.begin(), allowed_statuses.end(), value) !=
               allowed_statuses.end();
    };
    const auto& gates = report.at("gates");
    const auto& reasons = report.at("reasons");
    EXPECT_EQ(gates.size(), gate_names.size());
    EXPECT_EQ(reasons.size(), gate_names.size());
    for (const auto& gate_name : gate_names) {
        const auto key = gate_name.get<std::string>();
        ASSERT_TRUE(gates.contains(key));
        EXPECT_TRUE(has_allowed_status(gates.at(key)));
        EXPECT_TRUE(reasons.contains(key));
        EXPECT_FALSE(reasons.at(key).get<std::string>().empty());
    }
    EXPECT_TRUE(has_allowed_status(report.at("status")));
    EXPECT_EQ(report.at("security").size(),
              properties.at("security").at("required").size());
    EXPECT_TRUE(has_allowed_status(report.at("security").at("status")));
    EXPECT_TRUE(report.at("security").at("veto").is_boolean());
    EXPECT_TRUE(report.at("security").at("leakage_detected").is_boolean());
    EXPECT_FALSE(report.at("security").at("reason").get<std::string>().empty());
    const auto& result_names = properties.at("results").at("required");
    const auto& results = report.at("results");
    EXPECT_EQ(results.size(), result_names.size());
    for (const auto& result_name : result_names) {
        const auto& value = results.at(result_name.get<std::string>());
        EXPECT_TRUE(value.is_number() || value.is_null());
    }
    EXPECT_EQ(report.at("vetoes").size(),
              properties.at("vetoes").at("required").size());
    EXPECT_EQ(report.at("decision").size(),
              properties.at("decision").at("required").size());
    EXPECT_FALSE(report.at("decision").at("rationale").get<std::string>().empty());
    const auto& recommendation = report.at("decision").at("recommendation");
    EXPECT_TRUE(recommendation == "no_go" || recommendation == "research_only" ||
                recommendation == "promotion_candidate");
}

} // namespace

TEST(DreamOrchestratorLifecycle, DOR01_StartDreamRunRequiresDreamResearchMode) {
    DreamOrchestrator orchestrator;
    std::string error;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;

    const auto descriptor = orchestrator.StartDreamRun(
        "production",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {7, 11, 13},
        &error,
        &reason_class);

    EXPECT_FALSE(descriptor.has_value());
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::invalid_mode);
    EXPECT_NE(error.find("dream_research"), std::string::npos);
    EXPECT_EQ(orchestrator.auditLog().empty(), false);
}

TEST(DreamOrchestratorLifecycle, DOR02_StartDreamRunPersistsDescriptorByRunId) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {7, 11, 13});

    ASSERT_TRUE(descriptor.has_value());
    EXPECT_EQ(descriptor->mode, "dream_research");
    EXPECT_EQ(descriptor->state, DreamRunState::R0);
    EXPECT_TRUE(descriptor->synthetic);
    EXPECT_TRUE(descriptor->dream_mode);

    const auto stored = orchestrator.getRunById(descriptor->run_id);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->run_id, descriptor->run_id);
    EXPECT_EQ(stored->dataset_ref, descriptor->dataset_ref);
    EXPECT_EQ(stored->seed_set, descriptor->seed_set);
}

TEST(DreamOrchestratorLifecycle, DOR03_AllowedTransitionMatrixRestrictsR0ToR3) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    std::string detail;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;
    ASSERT_TRUE(orchestrator.validateRun(descriptor->run_id, passingEvidence()).has_value());
    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1, &detail, &reason_class));
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::none);

    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R2, &detail, &reason_class));
    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R3, &detail, &reason_class));

    EXPECT_FALSE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R0, &detail, &reason_class));
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::invalid_transition);
    EXPECT_NE(detail.find("Invalid transition"), std::string::npos);
}

TEST(DreamOrchestratorLifecycle, DOR04_FailedStartIsLoggedWithReasonClass) {
    DreamOrchestrator orchestrator;
    std::string error;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;

    (void)orchestrator.StartDreamRun(
        "analysis",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {9, 9},
        &error,
        &reason_class);

    auto log = orchestrator.auditLog();
    ASSERT_FALSE(log.empty());
    EXPECT_EQ(log.back().reason_class, DreamRunAuditReasonClass::invalid_mode);
    EXPECT_EQ(log.back().phase, "start");
    EXPECT_NE(log.back().reason.find("dream_research"), std::string::npos);
}

TEST(DreamOrchestratorLifecycle, DOR05_AbortAndFailureAreAudited) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    EXPECT_TRUE(orchestrator.abortRun(descriptor->run_id, "operator abort"));
    EXPECT_TRUE(orchestrator.failRun(descriptor->run_id, "policy gate failed",
                                   DreamRunAuditReasonClass::generic_failure));

    const auto log = orchestrator.auditLog();
    EXPECT_GE(log.size(), 3u);
    EXPECT_EQ(log.back().reason_class, DreamRunAuditReasonClass::generic_failure);
}

TEST(DreamOrchestratorLifecycle, DOR06_PassReportIsPersistedAndSchemaConformant) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research", "policy-v1", "dataset://alpha/snapshot-42", {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    const auto report = orchestrator.validateRun(descriptor->run_id, passingEvidence());
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->status, DreamGateStatus::pass);
    EXPECT_TRUE(report->permitsPromotionCandidate());

    const auto stored = orchestrator.getGateReportByRunId(descriptor->run_id);
    ASSERT_TRUE(stored.has_value());
    const auto json = stored->toJson();
    EXPECT_EQ(json.at("run_id"), descriptor->run_id);
    EXPECT_EQ(json.at("status"), "pass");
    EXPECT_EQ(json.at("decision").at("recommendation"), "promotion_candidate");
    EXPECT_TRUE(json.at("decision").at("transition_eligible").get<bool>());
    EXPECT_FALSE(json.at("invalidated_by_reset").get<bool>());
    EXPECT_EQ(json.at("gates").size(), 5u);
    EXPECT_EQ(json.at("gates").at("BRAIN-DREAM-ISOLATION"), "pass");
    EXPECT_EQ(json.at("gates").at("BRAIN-DREAM-LABEL-INTEGRITY"), "pass");
    EXPECT_EQ(json.at("gates").at("BRAIN-DREAM-UTILITY"), "pass");
    EXPECT_EQ(json.at("gates").at("BRAIN-DREAM-NONREGRESSION"), "pass");
    EXPECT_EQ(json.at("gates").at("BRAIN-DREAM-ETHICS-BOUNDARY"), "pass");
    EXPECT_EQ(json.at("reasons").size(), 5u);
    EXPECT_EQ(json.at("security").at("status"), "pass");
    EXPECT_EQ(json.at("results").at("online_hallucination_delta"), 0.0);
    EXPECT_EQ(json.at("results").at("online_error_delta"), 0.0);
    EXPECT_TRUE(json.at("vetoes").at("ethics").is_null());
    EXPECT_TRUE(json.at("vetoes").at("security").is_null());
    expectReportConformsToSchema(json);
}

TEST(DreamOrchestratorLifecycle, DOR07_MissingEvidenceHoldsAndBlocksPromotion) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research", "policy-v1", "dataset://alpha/snapshot-42", {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    const auto report =
        orchestrator.validateRun(descriptor->run_id, DreamValidationEvidence{});
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->status, DreamGateStatus::hold);
    EXPECT_FALSE(report->permitsPromotionCandidate());
    EXPECT_EQ(report->isolation.status, DreamGateStatus::hold);
    EXPECT_EQ(report->label_integrity.status, DreamGateStatus::hold);
    EXPECT_EQ(report->utility.status, DreamGateStatus::hold);
    EXPECT_EQ(report->nonregression.status, DreamGateStatus::hold);
    EXPECT_EQ(report->ethics_boundary.status, DreamGateStatus::hold);
    EXPECT_FALSE(report->toJson().at("reasons")
                     .at("BRAIN-DREAM-NONREGRESSION").get<std::string>().empty());

    std::string detail;
    EXPECT_FALSE(orchestrator.transitionRun(
        descriptor->run_id, DreamRunState::R1, &detail));
    EXPECT_NE(detail.find("utility, nonregression, and safety"), std::string::npos);
}

TEST(DreamOrchestratorLifecycle, DOR08_EthicsAndSecurityVetoesAreExplicit) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research", "policy-v1", "dataset://alpha/snapshot-42", {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    auto evidence = passingEvidence();
    evidence.ethics_veto = true;
    evidence.ethics_reason = "policy boundary violation";
    evidence.security_veto = true;
    evidence.security_check_completed = false;
    evidence.security_reason = "unresolved leakage risk";

    const auto report = orchestrator.validateRun(descriptor->run_id, evidence);
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->ethics_boundary.status, DreamGateStatus::fail);
    EXPECT_EQ(report->ethics_boundary.reason, "policy boundary violation");
    EXPECT_EQ(report->security_leakage.status, DreamGateStatus::hold);
    EXPECT_EQ(report->security_leakage.reason, "unresolved leakage risk");
    EXPECT_EQ(report->status, DreamGateStatus::fail);

    const auto json = report->toJson();
    EXPECT_EQ(json.at("vetoes").at("ethics"), "policy boundary violation");
    EXPECT_EQ(json.at("vetoes").at("security"), "unresolved leakage risk");
    EXPECT_FALSE(report->permitsPromotionCandidate());
    EXPECT_FALSE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1));
}

TEST(DreamOrchestratorLifecycle, DOR09_NonregressionFailureBlocksCandidateTransition) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research", "policy-v1", "dataset://alpha/snapshot-42", {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    ASSERT_TRUE(orchestrator.validateRun(descriptor->run_id, passingEvidence()).has_value());
    ASSERT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1));

    auto regressed = passingEvidence();
    regressed.online_hallucination_delta = 0.01;
    const auto report = orchestrator.validateRun(descriptor->run_id, regressed);
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->nonregression.status, DreamGateStatus::fail);
    EXPECT_EQ(report->recommendation, "no_go");

    std::string detail;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;
    EXPECT_FALSE(orchestrator.transitionRun(
        descriptor->run_id, DreamRunState::R2, &detail, &reason_class));
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::gate_rejected);
    EXPECT_NE(detail.find("all Dream-Mode gates"), std::string::npos);
    EXPECT_EQ(orchestrator.getRunById(descriptor->run_id)->state, DreamRunState::R1);
}

TEST(DreamOrchestratorLifecycle, DOR10_UnknownRunCannotPersistGateReport) {
    DreamOrchestrator orchestrator;
    EXPECT_FALSE(orchestrator.validateRun("unknown-run", passingEvidence()).has_value());
    EXPECT_FALSE(orchestrator.getGateReportByRunId("unknown-run").has_value());
}

TEST(DreamOrchestratorLifecycle, DOR11_DetectedSecurityLeakageFailsIsolation) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research", "policy-v1", "dataset://alpha/snapshot-42", {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    auto evidence = passingEvidence();
    evidence.security_leakage_detected = true;
    evidence.security_reason = "synthetic content crossed a truth boundary";
    const auto report = orchestrator.validateRun(descriptor->run_id, evidence);
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->security_leakage.status, DreamGateStatus::fail);
    EXPECT_EQ(report->isolation.status, DreamGateStatus::fail);
    EXPECT_EQ(report->isolation.reason, evidence.security_reason);
    EXPECT_EQ(report->recommendation, "no_go");
    EXPECT_EQ(report->toJson().at("security").at("leakage_detected"), true);
    EXPECT_FALSE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1));
}

TEST(DreamOrchestratorLifecycle, DOR12_AbortAndFailureInvalidateStoredGateReports) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research", "policy-v1", "dataset://alpha/snapshot-42", {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    ASSERT_TRUE(orchestrator.validateRun(descriptor->run_id, passingEvidence()).has_value());
    ASSERT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1));
    ASSERT_TRUE(orchestrator.abortRun(descriptor->run_id, "operator abort"));

    auto report = orchestrator.getGateReportByRunId(descriptor->run_id);
    ASSERT_TRUE(report.has_value());
    EXPECT_TRUE(report->invalidated_by_reset);
    EXPECT_EQ(report->recommendation, "research_only");
    EXPECT_FALSE(report->permitsPromotionCandidate());
    EXPECT_FALSE(
        report->toJson().at("decision").at("transition_eligible").get<bool>());
    expectReportConformsToSchema(report->toJson());
    std::string detail;
    EXPECT_FALSE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1, &detail));
    EXPECT_NE(detail.find("fresh validation"), std::string::npos);

    ASSERT_TRUE(orchestrator.validateRun(descriptor->run_id, passingEvidence()).has_value());
    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1));
    ASSERT_TRUE(orchestrator.failRun(
        descriptor->run_id, "synthetic failure", DreamRunAuditReasonClass::generic_failure));
    EXPECT_FALSE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1, &detail));
    EXPECT_NE(detail.find("fresh validation"), std::string::npos);

    report = orchestrator.getGateReportByRunId(descriptor->run_id);
    ASSERT_TRUE(report.has_value());
    EXPECT_TRUE(report->invalidated_by_reset);
    EXPECT_FALSE(report->permitsPromotionCandidate());
    expectReportConformsToSchema(report->toJson());
}
