/**
 * @file test_updates_operator_diagnostics_focused.cpp
 * @brief Focused tests for operator diagnostics module (Phase 6)
 * @date 2026-08-08
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <string>
#include <vector>

#include "include/updates/updates_operator_diagnostics.h"
#include "include/updates/updates_diagnostics.h"
#include "include/updates/updates_diagnostic_emitter.h"

namespace themis::updates {

using json = nlohmann::json;

class OperatorDiagnosticsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    diagnostics_ = std::make_unique<OperatorDiagnostics>();
  }

  void TearDown() override {
    diagnostics_.reset();
  }

  std::unique_ptr<OperatorDiagnostics> diagnostics_;

  // Helper to create ErrorContext for testing
  ErrorContext createErrorContext(
      DiagnosticErrorCode error_code,
      const std::string& operation = "test_op",
      const std::string& node_id = "node_1",
      const std::string& phase = "validate") {
    ErrorContext ctx;
    ctx.error_code = error_code;
    ctx.operation_id = operation;
    ctx.affected_node_id = node_id;
    ctx.current_phase = phase;
    ctx.timestamp = std::chrono::system_clock::now();
    return ctx;
  }
};

// ============================================================================
// Test OD-01: Detect COORDINATOR_UNREACHABLE scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_01_DetectCoordinatorUnreachable) {
  auto ctx = createErrorContext(DiagnosticErrorCode::COORDINATION_TIMEOUT, "update_op_1");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::COORDINATOR_UNREACHABLE);
}

// ============================================================================
// Test OD-02: Detect PARTIAL_MIGRATION_FAILURE scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_02_DetectPartialMigrationFailure) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::MIGRATION_PHASE_ERROR,
      "migrate_data_shards_op");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::PARTIAL_MIGRATION_FAILURE);
}

// ============================================================================
// Test OD-03: Detect CANARY_TIMEOUT_CYCLE scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_03_DetectCanaryTimeoutCycle) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::CANARY_VALIDATION_TIMEOUT,
      "canary_validate_op");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::CANARY_TIMEOUT_CYCLE);
}

// ============================================================================
// Test OD-04: Detect BLUE_GREEN_ROLLBACK_FAILURE scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_04_DetectBlueGreenRollbackFailure) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::BLUE_GREEN_SWITCH_FAILED,
      "blue_green_deploy_op",
      "green_node_5");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE);
}

// ============================================================================
// Test OD-05: Detect RESOURCE_EXHAUSTED scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_05_DetectResourceExhausted) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::RESOURCE_EXHAUSTED,
      "high_load_update_op");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::RESOURCE_EXHAUSTED);
}

// ============================================================================
// Test OD-06: Detect MANIFEST_CORRUPTION scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_06_DetectManifestCorruption) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH,
      "manifest_apply_op");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::MANIFEST_CORRUPTION);
}

// ============================================================================
// Test OD-07: Detect CLUSTER_PARTITION scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_07_DetectClusterPartition) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::NETWORK_PARTITION,
      "update_with_quorum_op");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::CLUSTER_PARTITION);
}

// ============================================================================
// Test OD-08: Detect DEADLOCK_RACE_CONDITION scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_08_DetectDeadlockRaceCondition) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::OPERATION_TIMEOUT,
      "concurrent_update_op");
  
  auto scenario = diagnostics_->detectScenario(ctx);
  
  EXPECT_EQ(scenario, FailureScenario::DEADLOCK_RACE_CONDITION);
}

// ============================================================================
// Test OD-09: Recovery procedure validation
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_09_GetRecoveryProcedureValid) {
  auto proc = diagnostics_->getRecoveryProcedure(
      FailureScenario::COORDINATOR_UNREACHABLE);
  
  EXPECT_FALSE(proc.symptoms.empty());
  EXPECT_FALSE(proc.recovery_steps.empty());
  EXPECT_FALSE(proc.prevention_tips.empty());
  EXPECT_GT(proc.root_cause_analysis.size(), 0);
  EXPECT_GE(proc.expected_recovery_time_seconds, 30);
}

// ============================================================================
// Test OD-10: Alerting rule validation
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_10_GetAlertingRuleValid) {
  auto rule = diagnostics_->getAlertingRule(
      FailureScenario::COORDINATOR_UNREACHABLE);
  
  EXPECT_FALSE(rule.rule_id.empty());
  EXPECT_FALSE(rule.query.empty());
  EXPECT_FALSE(rule.message_template.empty());
  EXPECT_NE(rule.severity, AlertSeverity::UNKNOWN);
  EXPECT_THAT(rule.severity,
      ::testing::AnyOf(AlertSeverity::CRITICAL, AlertSeverity::ERROR));
  // Verify Prometheus query syntax
  EXPECT_NE(rule.query.find("increase("), std::string::npos);
}

// ============================================================================
// Test OD-11: Error context enrichment
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_11_EnrichErrorContextWithRecovery) {
  auto ctx = createErrorContext(
      DiagnosticErrorCode::COORDINATION_TIMEOUT,
      "coordinator_test_op");
  
  auto enriched_ctx = diagnostics_->enrichErrorContext(ctx);
  
  EXPECT_FALSE(enriched_ctx.recovery_action_recommended.empty());
  EXPECT_NE(enriched_ctx.recommended_action, RecoveryAction::UNKNOWN);
  EXPECT_NE(enriched_ctx.recommended_action, RecoveryAction::NONE);
  EXPECT_FALSE(enriched_ctx.json_metadata.empty());
}

// ============================================================================
// Test OD-12: JSON export correctness
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_12_JsonExportIsValid) {
  auto json_obj = diagnostics_->exportScenariosAsJson();
  
  EXPECT_FALSE(json_obj.empty());
  EXPECT_TRUE(json_obj.is_object());
  
  // Verify all 8 scenarios are present
  EXPECT_TRUE(json_obj.contains("scenarios"));
  EXPECT_EQ(json_obj["scenarios"].size(), 8);
  
  // Spot-check first scenario structure
  auto scenarios_arr = json_obj["scenarios"];
  EXPECT_TRUE(scenarios_arr[0].contains("name"));
  EXPECT_TRUE(scenarios_arr[0].contains("description"));
}

// ============================================================================
// Test OD-13: Log patterns for observability integration
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_13_GetLogPatternsForObservability) {
  auto patterns = diagnostics_->getLogPatterns(
      FailureScenario::RESOURCE_EXHAUSTED);
  
  EXPECT_FALSE(patterns.grep_patterns.empty());
  EXPECT_FALSE(patterns.metric_queries.empty());
  EXPECT_FALSE(patterns.alert_triggers.empty());
  
  // Verify patterns are usable for grep
  for (const auto& pattern : patterns.grep_patterns) {
    EXPECT_NE(pattern.find("grep"), std::string::npos);
  }
}

// ============================================================================
// Test OD-14: Metrics tracking per scenario
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_14_TrackMetricsPerScenario) {
  // Simulate multiple occurrences of a scenario
  auto ctx1 = createErrorContext(
      DiagnosticErrorCode::MIGRATION_PHASE_ERROR,
      "migrate_1");
  
  auto ctx2 = createErrorContext(
      DiagnosticErrorCode::MIGRATION_PHASE_ERROR,
      "migrate_2");
  
  diagnostics_->detectScenario(ctx1);
  diagnostics_->detectScenario(ctx2);
  
  auto metrics = diagnostics_->getMetrics(
      FailureScenario::PARTIAL_MIGRATION_FAILURE);
  
  EXPECT_EQ(metrics.total_detections, 2);
  EXPECT_EQ(metrics.recent_occurrence, 2);  // Both occurred
}

// ============================================================================
// Test OD-15: Multi-scenario diagnostic JSON export
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_15_ExportMultipleScenariosAsJson) {
  auto recovery_json = diagnostics_->exportRecoveriesAsJson();
  auto alerting_json = diagnostics_->exportAlertingRulesAsJson();
  
  EXPECT_FALSE(recovery_json.empty());
  EXPECT_FALSE(alerting_json.empty());
  
  EXPECT_TRUE(recovery_json.is_array());
  EXPECT_TRUE(alerting_json.is_array());
  
  // Should have 8 items for 8 scenarios
  EXPECT_EQ(recovery_json.size(), 8);
  EXPECT_EQ(alerting_json.size(), 8);
}

// ============================================================================
// Test OD-16: Scenario detection with all error codes
// ============================================================================
TEST_F(OperatorDiagnosticsTest, OD_16_DetectAllMajorErrorCodes) {
  std::vector<std::pair<DiagnosticErrorCode, FailureScenario>> test_cases = {
    {DiagnosticErrorCode::COORDINATION_QUORUM_LOST, FailureScenario::COORDINATOR_UNREACHABLE},
    {DiagnosticErrorCode::STATE_HISTORY_CORRUPT, FailureScenario::MANIFEST_CORRUPTION},
    {DiagnosticErrorCode::CANARY_HEALTH_CHECK_FAILED, FailureScenario::CANARY_TIMEOUT_CYCLE},
  };
  
  for (const auto& [error_code, expected_scenario] : test_cases) {
    auto ctx = createErrorContext(error_code);
    auto detected = diagnostics_->detectScenario(ctx);
    EXPECT_EQ(detected, expected_scenario) << "Failed for error code: " <<
        static_cast<int>(error_code);
  }
}

}  // namespace themis::updates

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
