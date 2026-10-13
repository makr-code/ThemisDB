// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "tests/audit/test_wave_exit_criteria.h"

namespace {

using themis::audit::makeWaveExitCriteria;

}  // namespace

TEST(WaveExitCriteriaTest, Wave7CodeReviewCoverage) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[0].required && criteria[0].present);
    EXPECT_EQ(criteria[0].name, "wave7_code_review_coverage");
}

TEST(WaveExitCriteriaTest, Wave7TestDensity) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[1].required && criteria[1].present);
    EXPECT_EQ(criteria[1].name, "wave7_test_density");
}

TEST(WaveExitCriteriaTest, Wave7DocumentationComplete) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[2].required && criteria[2].present);
    EXPECT_EQ(criteria[2].name, "wave7_documentation_complete");
}

TEST(WaveExitCriteriaTest, Wave7ReleaseNotesClean) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[3].required && criteria[3].present);
    EXPECT_EQ(criteria[3].name, "wave7_release_notes_clean");
}

TEST(WaveExitCriteriaTest, Wave7ApiContractsFrozen) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[4].required && criteria[4].present);
    EXPECT_EQ(criteria[4].name, "wave7_api_contracts_frozen");
}

TEST(WaveExitCriteriaTest, Wave7ReviewedCriticalPaths) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[5].required && criteria[5].present);
    EXPECT_EQ(criteria[5].name, "wave7_reviewed_critical_paths");
}

TEST(WaveExitCriteriaTest, Wave7BugRiskBaseline) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[6].required && criteria[6].present);
    EXPECT_EQ(criteria[6].name, "wave7_bug_risk_baseline");
}

TEST(WaveExitCriteriaTest, Wave7FallbackPathsValidated) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[7].required && criteria[7].present);
    EXPECT_EQ(criteria[7].name, "wave7_fallback_paths_validated");
}

TEST(WaveExitCriteriaTest, Wave7AsyncFailuresHandled) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[8].required && criteria[8].present);
    EXPECT_EQ(criteria[8].name, "wave7_async_failures_handled");
}

TEST(WaveExitCriteriaTest, Wave7MemorySafetyBaseline) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[9].required && criteria[9].present);
    EXPECT_EQ(criteria[9].name, "wave7_mem_safety_baseline");
}

TEST(WaveExitCriteriaTest, Wave8LatencyBudgetVerified) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[10].required && criteria[10].present);
    EXPECT_EQ(criteria[10].name, "wave8_latency_budget_verified");
}

TEST(WaveExitCriteriaTest, Wave8TimeoutBehaviorValid) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[11].required && criteria[11].present);
    EXPECT_EQ(criteria[11].name, "wave8_timeout_behavior_valid");
}

TEST(WaveExitCriteriaTest, Wave8CancellationPropagationOk) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[12].required && criteria[12].present);
    EXPECT_EQ(criteria[12].name, "wave8_cancellation_propagation_ok");
}

TEST(WaveExitCriteriaTest, Wave8SecurityScanClean) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[13].required && criteria[13].present);
    EXPECT_EQ(criteria[13].name, "wave8_security_scan_clean");
}

TEST(WaveExitCriteriaTest, Wave8ComplianceGatesPassed) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[14].required && criteria[14].present);
    EXPECT_EQ(criteria[14].name, "wave8_compliance_gates_passed");
}

TEST(WaveExitCriteriaTest, Wave8AuditLoggingFunctional) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[15].required && criteria[15].present);
    EXPECT_EQ(criteria[15].name, "wave8_audit_logging_functional");
}

TEST(WaveExitCriteriaTest, Wave8FailoverProceduresTested) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[16].required && criteria[16].present);
    EXPECT_EQ(criteria[16].name, "wave8_failover_procedures_tested");
}

TEST(WaveExitCriteriaTest, Wave8ResourceLimitsEnforced) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[17].required && criteria[17].present);
    EXPECT_EQ(criteria[17].name, "wave8_resource_limits_enforced");
}

TEST(WaveExitCriteriaTest, Wave8BreakerDetectionActive) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[18].required && criteria[18].present);
    EXPECT_EQ(criteria[18].name, "wave8_breaker_detection_active");
}

TEST(WaveExitCriteriaTest, Wave8BackupRestoreVerified) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[19].required && criteria[19].present);
    EXPECT_EQ(criteria[19].name, "wave8_backup_restore_verified");
}

TEST(WaveExitCriteriaTest, Wave9MetricCollectionReady) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[20].required && criteria[20].present);
    EXPECT_EQ(criteria[20].name, "wave9_metric_collection_ready");
}

TEST(WaveExitCriteriaTest, Wave9TracingIntegrationActive) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[21].required && criteria[21].present);
    EXPECT_EQ(criteria[21].name, "wave9_tracing_integration_active");
}

TEST(WaveExitCriteriaTest, Wave9LogCorrelationEnabled) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[22].required && criteria[22].present);
    EXPECT_EQ(criteria[22].name, "wave9_log_correlation_enabled");
}

TEST(WaveExitCriteriaTest, Wave9SlaThresholdsDefined) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[23].required && criteria[23].present);
    EXPECT_EQ(criteria[23].name, "wave9_sla_thresholds_defined");
}

TEST(WaveExitCriteriaTest, Wave9RunbooksPublished) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[24].required && criteria[24].present);
    EXPECT_EQ(criteria[24].name, "wave9_runbooks_published");
}

TEST(WaveExitCriteriaTest, Wave9SupportEscalationValidated) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[25].required && criteria[25].present);
    EXPECT_EQ(criteria[25].name, "wave9_support_escalation_validated");
}

TEST(WaveExitCriteriaTest, Wave9CapacityPlanReviewed) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[26].required && criteria[26].present);
    EXPECT_EQ(criteria[26].name, "wave9_capacity_plan_reviewed");
}

TEST(WaveExitCriteriaTest, Wave9IncidentResponseDrillDone) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[27].required && criteria[27].present);
    EXPECT_EQ(criteria[27].name, "wave9_incident_response_drill_done");
}

TEST(WaveExitCriteriaTest, Wave9ObservabilityDashboardLive) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[28].required && criteria[28].present);
    EXPECT_EQ(criteria[28].name, "wave9_observability_dashboard_live");
}

TEST(WaveExitCriteriaTest, Wave9RiskAcceptanceSigned) {
    const auto criteria = makeWaveExitCriteria();
    EXPECT_TRUE(criteria[29].required && criteria[29].present);
    EXPECT_EQ(criteria[29].name, "wave9_risk_acceptance_signed");
}
