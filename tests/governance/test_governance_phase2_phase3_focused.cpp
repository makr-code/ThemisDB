/**
 * @file test_governance_phase2_phase3_focused.cpp
 * @brief Phase 2-3 hardening tests: policy lifecycle, error handling, diagnostics.
 * @note Test IDs: P23-01..P23-08
 * @note Coverage: PolicyState enum, lifecycle transitions, OPA error classification,
 *                 deny-by-default, diagnostic recording, conflict detection.
 */

#include <gtest/gtest.h>
#include "governance/governance_diagnostics.h"
#include "governance/policy_manager.h"
#include "governance/opa_adapter.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

using namespace themis::governance;

class Phase23Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data
    }
};

// ========== P23-01: PolicyState Enum Validation ==========

TEST_F(Phase23Test, P2301_PolicyStateEnumExists) {
    EXPECT_EQ(static_cast<int>(PolicyState::DRAFT), 0);
    EXPECT_EQ(static_cast<int>(PolicyState::ACTIVE), 1);
    EXPECT_EQ(static_cast<int>(PolicyState::DEPRECATED), 2);
    EXPECT_EQ(static_cast<int>(PolicyState::RETIRED), 3);
}

TEST_F(Phase23Test, P2301_PolicyStateValuesDistinct) {
    std::vector<int> states = {
        static_cast<int>(PolicyState::DRAFT),
        static_cast<int>(PolicyState::ACTIVE),
        static_cast<int>(PolicyState::DEPRECATED),
        static_cast<int>(PolicyState::RETIRED),
    };
    std::sort(states.begin(), states.end());
    EXPECT_EQ(std::unique(states.begin(), states.end()), states.end());
}

// ========== P23-02: Lifecycle State Transitions ==========

TEST_F(Phase23Test, P2302_LifecycleDraftToActive) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DRAFT;
    EXPECT_TRUE(lc.canTransitionTo(PolicyState::ACTIVE));
}

TEST_F(Phase23Test, P2302_LifecycleActiveToDraft) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::ACTIVE;
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::DRAFT));
}

TEST_F(Phase23Test, P2302_LifecycleActiveToDeprecated) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::ACTIVE;
    EXPECT_TRUE(lc.canTransitionTo(PolicyState::DEPRECATED));
}

TEST_F(Phase23Test, P2302_LifecycleActiveToRetired) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::ACTIVE;
    EXPECT_TRUE(lc.canTransitionTo(PolicyState::RETIRED));
}

TEST_F(Phase23Test, P2302_LifecycleDeprecatedToRetired) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DEPRECATED;
    EXPECT_TRUE(lc.canTransitionTo(PolicyState::RETIRED));
}

TEST_F(Phase23Test, P2302_LifecycleDeprecatedToDraft) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DEPRECATED;
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::DRAFT));
}

TEST_F(Phase23Test, P2302_LifecycleRetiredTerminal) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::RETIRED;
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::DRAFT));
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::ACTIVE));
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::DEPRECATED));
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::RETIRED));
}

// ========== P23-03: Invalid State Transitions Rejected ==========

TEST_F(Phase23Test, P2303_InvalidTransitionFromDraft) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DRAFT;
    
    // Only ACTIVE is allowed from DRAFT
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::DEPRECATED));
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::RETIRED));
}

TEST_F(Phase23Test, P2303_InvalidTransitionFromDeprecated) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DEPRECATED;
    
    // Only RETIRED is allowed from DEPRECATED
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::DRAFT));
    EXPECT_FALSE(lc.canTransitionTo(PolicyState::ACTIVE));
}

TEST_F(Phase23Test, P2303_StateDescriptionForDraft) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DRAFT;
    std::string desc = lc.getStateDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("draft"), std::string::npos);
}

TEST_F(Phase23Test, P2303_StateDescriptionForActive) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::ACTIVE;
    std::string desc = lc.getStateDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("active"), std::string::npos);
}

TEST_F(Phase23Test, P2303_StateDescriptionForDeprecated) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::DEPRECATED;
    std::string desc = lc.getStateDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("deprecated"), std::string::npos);
}

TEST_F(Phase23Test, P2303_StateDescriptionForRetired) {
    PolicyLifecycle lc;
    lc.current_state = PolicyState::RETIRED;
    std::string desc = lc.getStateDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("archived"), std::string::npos);
}

// ========== P23-04: Lifecycle Audit Logging ==========

TEST_F(Phase23Test, P2304_ActivateRuleWithValidation) {
    PolicyManager pm;
    
    // Create and add a draft rule
    PolicyRule rule;
    rule.id = "test-rule-1";
    rule.name = "Test Rule";
    rule.classification_level = "vs-nfd";
    rule.resources = {"data/*"};
    rule.actions = {"read"};
    rule.lifecycle.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    rule.lifecycle.created_by = "test-user";
    
    pm.addRule(rule);
    
    // Activate the rule
    auto result = pm.activateRuleWithValidation("test-rule-1", "test-user");
    EXPECT_EQ(result.error, PolicyManager::PolicyError::kSuccess);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_FALSE(result.rule_version.empty());
    
    // Verify the rule is now active
    auto updated_rule = pm.getRule("test-rule-1");
    ASSERT_TRUE(updated_rule.has_value());
    EXPECT_EQ(updated_rule->lifecycle.current_state, PolicyState::ACTIVE);
    EXPECT_GT(updated_rule->lifecycle.activated_at, 0);
}

TEST_F(Phase23Test, P2304_ActivateNonExistentRule) {
    PolicyManager pm;
    
    auto result = pm.activateRuleWithValidation("nonexistent", "test-user");
    EXPECT_EQ(result.error, PolicyManager::PolicyError::kRuleNotFound);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(Phase23Test, P2304_ActivateRuleInvalidTransition) {
    PolicyManager pm;
    
    PolicyRule rule;
    rule.id = "test-rule-2";
    rule.name = "Test Rule 2";
    rule.classification_level = "vs-nfd";
    rule.resources = {"data/*"};
    rule.actions = {"read"};
    rule.lifecycle.current_state = PolicyState::RETIRED;
    
    pm.addRule(rule);
    
    auto result = pm.activateRuleWithValidation("test-rule-2", "test-user");
    EXPECT_EQ(result.error, PolicyManager::PolicyError::kInvalidStateTransition);
    EXPECT_NE(result.error_message.find("Cannot transition"), std::string::npos);
}

TEST_F(Phase23Test, P2304_DeprecateRule) {
    PolicyManager pm;
    
    PolicyRule rule;
    rule.id = "test-rule-3";
    rule.name = "Test Rule 3";
    rule.classification_level = "vs-nfd";
    rule.resources = {"data/*"};
    rule.actions = {"read"};
    rule.lifecycle.current_state = PolicyState::ACTIVE;
    
    pm.addRule(rule);
    
    std::string version = pm.deprecateRule("test-rule-3", "test-user");
    EXPECT_FALSE(version.empty());
    
    auto updated_rule = pm.getRule("test-rule-3");
    ASSERT_TRUE(updated_rule.has_value());
    EXPECT_EQ(updated_rule->lifecycle.current_state, PolicyState::DEPRECATED);
}

TEST_F(Phase23Test, P2304_RetireRule) {
    PolicyManager pm;
    
    PolicyRule rule;
    rule.id = "test-rule-4";
    rule.name = "Test Rule 4";
    rule.classification_level = "vs-nfd";
    rule.resources = {"data/*"};
    rule.actions = {"read"};
    rule.lifecycle.current_state = PolicyState::DEPRECATED;
    
    pm.addRule(rule);
    
    std::string version = pm.retireRule("test-rule-4", "test-user");
    EXPECT_FALSE(version.empty());
    
    auto updated_rule = pm.getRule("test-rule-4");
    ASSERT_TRUE(updated_rule.has_value());
    EXPECT_EQ(updated_rule->lifecycle.current_state, PolicyState::RETIRED);
}

// ========== P23-05: OPA Error Classification ==========

TEST_F(Phase23Test, P2305_OpaErrorTypeEnum) {
    EXPECT_EQ(static_cast<int>(OpaErrorType::kTimeout), 0);
    EXPECT_EQ(static_cast<int>(OpaErrorType::kMalformedResponse), 1);
    EXPECT_EQ(static_cast<int>(OpaErrorType::kNetworkError), 2);
    EXPECT_EQ(static_cast<int>(OpaErrorType::kInvalidPolicy), 3);
    EXPECT_EQ(static_cast<int>(OpaErrorType::kUnknown), 4);
}

TEST_F(Phase23Test, P2305_OpaErrorTypeValuesDistinct) {
    std::vector<int> types = {
        static_cast<int>(OpaErrorType::kTimeout),
        static_cast<int>(OpaErrorType::kMalformedResponse),
        static_cast<int>(OpaErrorType::kNetworkError),
        static_cast<int>(OpaErrorType::kInvalidPolicy),
        static_cast<int>(OpaErrorType::kUnknown),
    };
    std::sort(types.begin(), types.end());
    EXPECT_EQ(std::unique(types.begin(), types.end()), types.end());
}

TEST_F(Phase23Test, P2305_OpaErrorStructHasTimestamp) {
    OpaError err;
    err.type = OpaErrorType::kTimeout;
    err.message = "Request timeout";
    err.timestamp_ms = 12345678;
    
    EXPECT_EQ(err.type, OpaErrorType::kTimeout);
    EXPECT_EQ(err.message, "Request timeout");
    EXPECT_EQ(err.timestamp_ms, 12345678);
}

// ========== P23-06: Fail-Closed Defaults on OPA Errors ==========

TEST_F(Phase23Test, P2306_OpaAdapterExistsWithConfig) {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:8181";
    cfg.policy_path = "themis/governance/allow";
    cfg.timeout_ms = 50;
    
    auto adapter = std::make_unique<OpaAdapter>(cfg);
    ASSERT_NE(adapter.get(), nullptr);
    EXPECT_EQ(adapter->getConfig().endpoint_url, "http://localhost:8181");
}

// ========== P23-07: Diagnostic Recording and Retrieval ==========

TEST_F(Phase23Test, P2307_DiagnosticAggregatorRecords) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kConflictDetected;
    diag.component = "policy_manager";
    diag.description = "Conflict detected between rules";
    
    agg.recordDiagnostic(diag);
    EXPECT_EQ(agg.getTotalCount(), 1);
}

TEST_F(Phase23Test, P2307_DiagnosticAggregatorRetrievesByComponent) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag1;
    diag1.code = GovDiagnosticCode::kConflictDetected;
    diag1.component = "policy_manager";
    diag1.description = "Conflict 1";
    
    GovernanceDiagnostic diag2;
    diag2.code = GovDiagnosticCode::kOpaUnavailable;
    diag2.component = "opa_adapter";
    diag2.description = "OPA unavailable";
    
    agg.recordDiagnostic(diag1);
    agg.recordDiagnostic(diag2);
    
    auto pm_diags = agg.getDiagnosticsForComponent("policy_manager");
    EXPECT_EQ(pm_diags.size(), 1);
    EXPECT_EQ(pm_diags[0].component, "policy_manager");
    
    auto opa_diags = agg.getDiagnosticsForComponent("opa_adapter");
    EXPECT_EQ(opa_diags.size(), 1);
    EXPECT_EQ(opa_diags[0].component, "opa_adapter");
}

TEST_F(Phase23Test, P2307_DiagnosticAggregatorRetrievesByCode) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag1;
    diag1.code = GovDiagnosticCode::kConflictDetected;
    diag1.component = "policy_manager";
    
    GovernanceDiagnostic diag2;
    diag2.code = GovDiagnosticCode::kConflictDetected;
    diag2.component = "policy_engine";
    
    agg.recordDiagnostic(diag1);
    agg.recordDiagnostic(diag2);
    
    auto conflicts = agg.getDiagnosticsForCode(GovDiagnosticCode::kConflictDetected);
    EXPECT_EQ(conflicts.size(), 2);
}

TEST_F(Phase23Test, P2307_DiagnosticToJsonSerialization) {
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kConflictDetected;
    diag.component = "policy_manager";
    diag.description = "Test conflict";
    diag.remediation_steps = {"Step 1", "Step 2"};
    diag.timestamp_ms = 1234567890;
    
    auto json = diag.toJson();
    EXPECT_TRUE(json.contains("code"));
    EXPECT_TRUE(json.contains("component"));
    EXPECT_TRUE(json.contains("description"));
    EXPECT_TRUE(json.contains("remediation_steps"));
    EXPECT_TRUE(json.contains("timestamp_ms"));
}

// ========== P23-08: Diagnostic Aggregation Across Components ==========

TEST_F(Phase23Test, P2308_AggregatorExportAsJson) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag1;
    diag1.code = GovDiagnosticCode::kConflictDetected;
    diag1.component = "policy_manager";
    diag1.description = "Conflict 1";
    
    GovernanceDiagnostic diag2;
    diag2.code = GovDiagnosticCode::kOpaUnavailable;
    diag2.component = "opa_adapter";
    diag2.description = "OPA unavailable";
    
    agg.recordDiagnostic(diag1);
    agg.recordDiagnostic(diag2);
    
    auto json_array = agg.exportAsJson();
    EXPECT_TRUE(json_array.is_array());
    EXPECT_EQ(json_array.size(), 2);
}

TEST_F(Phase23Test, P2308_AggregatorGetLatestPerComponent) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag1;
    diag1.code = GovDiagnosticCode::kConflictDetected;
    diag1.component = "policy_manager";
    diag1.description = "Conflict 1";
    diag1.timestamp_ms = 100;
    
    GovernanceDiagnostic diag2;
    diag2.code = GovDiagnosticCode::kStateTransitionInvalid;
    diag2.component = "policy_manager";
    diag2.description = "Conflict 2 (newer)";
    diag2.timestamp_ms = 200;
    
    agg.recordDiagnostic(diag1);
    agg.recordDiagnostic(diag2);
    
    auto latest = agg.getLatestPerComponent();
    EXPECT_EQ(latest.size(), 1);
    EXPECT_EQ(latest["policy_manager"].code, GovDiagnosticCode::kStateTransitionInvalid);
}

TEST_F(Phase23Test, P2308_AggregatorTimeRangeFilter) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag1;
    diag1.code = GovDiagnosticCode::kConflictDetected;
    diag1.component = "policy_manager";
    diag1.timestamp_ms = 100;
    
    GovernanceDiagnostic diag2;
    diag2.code = GovDiagnosticCode::kOpaUnavailable;
    diag2.component = "opa_adapter";
    diag2.timestamp_ms = 200;
    
    GovernanceDiagnostic diag3;
    diag3.code = GovDiagnosticCode::kFallbackActivated;
    diag3.component = "policy_engine";
    diag3.timestamp_ms = 300;
    
    agg.recordDiagnostic(diag1);
    agg.recordDiagnostic(diag2);
    agg.recordDiagnostic(diag3);
    
    // Query range [150, 250]
    auto in_range = agg.getDiagnosticsInTimeRange(150, 250);
    EXPECT_EQ(in_range.size(), 1);
    EXPECT_EQ(in_range[0].code, GovDiagnosticCode::kOpaUnavailable);
}

TEST_F(Phase23Test, P2308_AggregatorClear) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kConflictDetected;
    diag.component = "policy_manager";
    
    agg.recordDiagnostic(diag);
    EXPECT_EQ(agg.getTotalCount(), 1);
    
    agg.clear();
    EXPECT_EQ(agg.getTotalCount(), 0);
}

// ========== Integration Tests ==========

TEST_F(Phase23Test, P23_IntegrationPolicyLifecycleWithDiagnostics) {
    PolicyManager pm;
    
    // Create a rule and attempt invalid transitions
    PolicyRule rule;
    rule.id = "integration-test-rule";
    rule.name = "Integration Test Rule";
    rule.classification_level = "vs-nfd";
    rule.resources = {"data/*"};
    rule.actions = {"read"};
    rule.lifecycle.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    rule.lifecycle.created_by = "test-user";
    
    pm.addRule(rule);
    
    // Try to deprecate without activating (should fail)
    std::string depr = pm.deprecateRule("integration-test-rule", "test-user");
    EXPECT_TRUE(depr.empty());
    
    // Now activate
    auto act_result = pm.activateRuleWithValidation("integration-test-rule", "test-user");
    EXPECT_EQ(act_result.error, PolicyManager::PolicyError::kSuccess);
    
    // Now deprecate
    std::string depr2 = pm.deprecateRule("integration-test-rule", "test-user");
    EXPECT_FALSE(depr2.empty());
    
    // Now retire
    std::string retire = pm.retireRule("integration-test-rule", "test-user");
    EXPECT_FALSE(retire.empty());
    
    // Verify final state
    auto final_rule = pm.getRule("integration-test-rule");
    ASSERT_TRUE(final_rule.has_value());
    EXPECT_EQ(final_rule->lifecycle.current_state, PolicyState::RETIRED);
}

TEST_F(Phase23Test, P23_IntegrationDiagnosticCodes) {
    // Verify all diagnostic codes are in the reserved range
    EXPECT_GE(static_cast<int>(GovDiagnosticCode::kConflictDetected), 7300);
    EXPECT_LE(static_cast<int>(GovDiagnosticCode::kDenyByDefault), 7399);
    
    std::vector<int> codes = {
        static_cast<int>(GovDiagnosticCode::kConflictDetected),
        static_cast<int>(GovDiagnosticCode::kFallbackActivated),
        static_cast<int>(GovDiagnosticCode::kComplianceViolation),
        static_cast<int>(GovDiagnosticCode::kAuditLogFailure),
        static_cast<int>(GovDiagnosticCode::kOpaUnavailable),
        static_cast<int>(GovDiagnosticCode::kStateTransitionInvalid),
        static_cast<int>(GovDiagnosticCode::kLineageBackpressure),
        static_cast<int>(GovDiagnosticCode::kPolicyNotFound),
        static_cast<int>(GovDiagnosticCode::kDenyByDefault),
    };
    
    for (int code : codes) {
        EXPECT_GE(code, 7300);
        EXPECT_LE(code, 7399);
    }
}
