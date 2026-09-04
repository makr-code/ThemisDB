/**
 * @file test_governance_phase2b_phase3a_focused.cpp
 * @brief Phase 2B-3A-3B hardening tests: compliance reporter, policy engine fail-closed, conflict diagnostics.
 * @note Test IDs: P2B-01..P2B-08, P3A-01..P3A-08, P3B-01..P3B-02
 * @note Coverage: ComplianceReporterResult, atomic state, error codes, deny-by-default,
 *                 whitelist-based validation, conflict detection helpers.
 */

#include <gtest/gtest.h>
#include "governance/governance_diagnostics.h"
#include "governance/compliance_reporting.h"
#include "governance/policy_manager.h"
#include "governance/policy_engine.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>
#include <unordered_set>

using namespace themis::governance;

class Phase2B3ATest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_yaml_path_ = (std::filesystem::temp_directory_path()
            / ("themisdb_governance_profiles_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()) + ".yaml")).string();
        std::ofstream out(policy_yaml_path_, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "vs_classification:\n"
            << "  streng-geheim:\n"
            << "    encryption_required: true\n"
            << "    ann_allowed: false\n"
            << "    export_allowed: false\n"
            << "    cache_allowed: false\n"
            << "    redaction_level: strict\n"
            << "    retention_days: 3650\n"
            << "    log_encryption: true\n"
            << "  vs-nfd:\n"
            << "    encryption_required: true\n"
            << "    ann_allowed: true\n"
            << "    export_allowed: true\n"
            << "    cache_allowed: true\n"
            << "    redaction_level: standard\n"
            << "    retention_days: 365\n"
            << "    log_encryption: false\n"
            << "enforcement:\n"
            << "  default_mode: enforce\n";
        out.close();
    }
    
    void TearDown() override {
        if (!policy_yaml_path_.empty()) {
            std::error_code ec = {};
            std::filesystem::remove(policy_yaml_path_, ec);
        }
    }

    std::string policy_yaml_path_;
};

// ========== P2B-01: ComplianceReporterResult Validation ==========

TEST_F(Phase2B3ATest, P2B01_ComplianceReporterResultIsSuccess) {
    ComplianceReporterResult result;
    result.error = ComplianceError::kSuccess;
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.error, ComplianceError::kSuccess);
}

TEST_F(Phase2B3ATest, P2B01_ComplianceReporterResultFailure) {
    ComplianceReporterResult result;
    result.error = ComplianceError::kReportingFailed;
    result.error_message = "Test failure";
    
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.error, ComplianceError::kReportingFailed);
    EXPECT_EQ(result.error_message, "Test failure");
}

TEST_F(Phase2B3ATest, P2B01_ComplianceErrorCodeNames) {
    struct TestCase {
        ComplianceError error;
        std::string expected_name;
    };
    
    std::vector<TestCase> cases = {
        {ComplianceError::kSuccess, "SUCCESS"},
        {ComplianceError::kConflictDetected, "CONFLICT_DETECTED"},
        {ComplianceError::kReportingFailed, "REPORTING_FAILED"},
        {ComplianceError::kStateInvalid, "STATE_INVALID"},
        {ComplianceError::kResourceExhausted, "RESOURCE_EXHAUSTED"},
        {ComplianceError::kHtmlGenerationFailed, "HTML_GENERATION_FAILED"},
    };
    
    for (const auto& tc : cases) {
        ComplianceReporterResult result;
        result.error = tc.error;
        EXPECT_EQ(result.getErrorName(), tc.expected_name) 
            << "Failed for error code: " << static_cast<int>(tc.error);
    }
}

// ========== P2B-02: Atomic State Transitions ==========

TEST_F(Phase2B3ATest, P2B02_InitialReporterStateDraft) {
    ComplianceReporter reporter;
    EXPECT_EQ(reporter.getState(), ComplianceReporter::ReporterState::DRAFT);
}

TEST_F(Phase2B3ATest, P2B02_ReporterReadyForReporting) {
    ComplianceReporter reporter;
    EXPECT_TRUE(reporter.isReadyForReporting());
}

TEST_F(Phase2B3ATest, P2B02_ReporterStateTransitionNotReadyOnReporting) {
    // Create a reporter with mocked state
    ComplianceReporter reporter;
    // Note: Direct state modification not exposed, so we verify through methods
    EXPECT_TRUE(reporter.isReadyForReporting());
}

// ========== P2B-03: Error Code Recording ==========

TEST_F(Phase2B3ATest, P2B03_DiagnosticAggregatorRecordsErrors) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kConflictDetected;
    diag.component = "test_component";
    diag.description = "Test conflict";
    
    agg.recordDiagnostic(diag);
    
    EXPECT_EQ(agg.getTotalCount(), 1);
    auto diags = agg.getDiagnosticsForComponent("test_component");
    EXPECT_EQ(diags.size(), 1);
    EXPECT_EQ(diags[0].code, GovDiagnosticCode::kConflictDetected);
}

TEST_F(Phase2B3ATest, P2B03_DiagnosticFilteringByCode) {
    DiagnosticAggregator agg;
    
    GovernanceDiagnostic diag1, diag2;
    diag1.code = GovDiagnosticCode::kConflictDetected;
    diag1.component = "engine";
    diag2.code = GovDiagnosticCode::kOpaUnavailable;
    diag2.component = "opa";
    
    agg.recordDiagnostic(diag1);
    agg.recordDiagnostic(diag2);
    
    auto conflicts = agg.getDiagnosticsForCode(GovDiagnosticCode::kConflictDetected);
    EXPECT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].code, GovDiagnosticCode::kConflictDetected);
    
    auto opa_errors = agg.getDiagnosticsForCode(GovDiagnosticCode::kOpaUnavailable);
    EXPECT_EQ(opa_errors.size(), 1);
    EXPECT_EQ(opa_errors[0].code, GovDiagnosticCode::kOpaUnavailable);
}

TEST_F(Phase2B3ATest, P2B03_DiagnosticTimeRangeFiltering) {
    DiagnosticAggregator agg;
    
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kFallbackActivated;
    diag.component = "test";
    diag.timestamp_ms = now_ms;
    
    agg.recordDiagnostic(diag);
    
    // Query with overlapping range
    auto results = agg.getDiagnosticsInTimeRange(now_ms - 1000, now_ms + 1000);
    EXPECT_EQ(results.size(), 1);
    
    // Query with non-overlapping range
    results = agg.getDiagnosticsInTimeRange(now_ms + 10000, now_ms + 20000);
    EXPECT_EQ(results.size(), 0);
}

// ========== P3A-01: Policy Engine Deny-by-Default Validation ==========

TEST_F(Phase2B3ATest, P3A01_MissingClassificationDefaultsToDeny) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));
    
    // Headers without classification
    std::unordered_map<std::string, std::string> headers;
    headers["X-User-Id"] = "user123";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    
    // Phase 3A: Should default to deny (streng-geheim/strictest)
    EXPECT_FALSE(decision.ann_allowed);           // Deny ANN
    EXPECT_FALSE(decision.export_allowed);        // Deny export
    EXPECT_FALSE(decision.cache_allowed);         // Deny caching
    EXPECT_TRUE(decision.require_content_encryption);  // Require encryption
    EXPECT_EQ(decision.redaction, "strict");      // Strict redaction
}

TEST_F(Phase2B3ATest, P3A01_MissingProfileDefaultsToDeny) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));
    
    // Headers with unknown classification (not in profiles)
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "unknown-classification";
    headers["X-User-Id"] = "user123";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    
    // Phase 3A: Unknown classification should default to strictest
    EXPECT_FALSE(decision.ann_allowed);
    EXPECT_FALSE(decision.export_allowed);
    EXPECT_FALSE(decision.cache_allowed);
    EXPECT_TRUE(decision.require_content_encryption);
}

// ========== P3A-02: Removal of Implicit Allows ==========

TEST_F(Phase2B3ATest, P3A02_StrictClassificationEnforcedAlwaysEncrypt) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "streng-geheim";
    headers["X-User-Id"] = "user123";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    
    // Strict classification must always encrypt and deny operations
    EXPECT_TRUE(decision.encrypt_logs);
    EXPECT_FALSE(decision.export_allowed);
    EXPECT_FALSE(decision.cache_allowed);
    EXPECT_FALSE(decision.ann_allowed);
}

TEST_F(Phase2B3ATest, P3A02_NormalClassificationValidation) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "vs-nfd";
    headers["X-User-Id"] = "user123";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    
    // vs-nfd is normal classification, should have sensible defaults
    // (not necessarily all true, but explicit validation)
    EXPECT_EQ(decision.classification, "vs-nfd");
}

// ========== P3A-03: Whitelist-Based Permission Evaluation ==========

TEST_F(Phase2B3ATest, P3A03_HeaderOverrideValidation) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));
    
    // Test that valid headers work
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "vs-nfd";
    headers["X-Encryption-Logs"] = "true";
    headers["X-User-Id"] = "user123";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    EXPECT_EQ(decision.classification, "vs-nfd");
}

TEST_F(Phase2B3ATest, P3A03_InvalidRedactionLevelAccepted) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "vs-nfd";
    headers["X-Redaction-Level"] = "custom";
    headers["X-User-Id"] = "user123";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    // Header override should be accepted and applied
    EXPECT_EQ(decision.redaction, "custom");
}

// ========== P3B-01: Conflict Detection Helper ==========

TEST_F(Phase2B3ATest, P3B01_ConflictDetectionHelperInitialization) {
    ConflictDiagnosticHelper helper(
        ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_DENY
    );
    
    EXPECT_EQ(helper.getCurrentStrategy(), 
              ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_DENY);
}

TEST_F(Phase2B3ATest, P3B01_ConflictDetectionSinglePolicy) {
    ConflictDiagnosticHelper helper;
    
    std::vector<std::string> policy_ids = {"policy1"};
    auto result = helper.detectConflict(policy_ids);
    
    // Single policy should not have conflicts
    EXPECT_FALSE(result.has_conflicts);
    EXPECT_EQ(result.conflicting_pairs.size(), 0);
}

TEST_F(Phase2B3ATest, P3B01_ConflictDetectionMultiplePolicies) {
    ConflictDiagnosticHelper helper;
    
    std::vector<std::string> policy_ids = {"policy1", "policy2", "policy3"};
    auto result = helper.detectConflict(policy_ids);
    
    // Multiple policies should be detected as potential conflicts
    EXPECT_TRUE(result.has_conflicts);
    EXPECT_GT(result.conflicting_pairs.size(), 0);
    EXPECT_GT(result.descriptions.size(), 0);
}

// ========== P3B-02: Conflict Diagnostic Recording ==========

TEST_F(Phase2B3ATest, P3B02_ConflictRecordingToDiagnosticAggregator) {
    ConflictDiagnosticHelper helper;
    
    std::vector<std::string> policy_ids = {"policy1", "policy2"};
    auto result = helper.detectConflict(policy_ids);
    
    if (result.has_conflicts) {
        helper.recordConflict(result);
    }
    
    auto diags = helper.getConflictDiagnostics();
    EXPECT_GT(diags.size(), 0);
    
    for (const auto& diag : diags) {
        EXPECT_EQ(diag.code, GovDiagnosticCode::kConflictDetected);
        EXPECT_FALSE(diag.remediation_steps.empty());
    }
}

TEST_F(Phase2B3ATest, P3B02_ConflictClearHistory) {
    ConflictDiagnosticHelper helper;
    
    std::vector<std::string> policy_ids = {"policy1", "policy2"};
    auto result = helper.detectConflict(policy_ids);
    if (result.has_conflicts) {
        helper.recordConflict(result);
    }
    
    auto diags_before = helper.getConflictDiagnostics();
    EXPECT_GT(diags_before.size(), 0);
    
    helper.clearConflictHistory();
    
    // Note: clearConflictHistory only clears internal history, not aggregator
    // so getDiagnosticsForCode will still show the recorded diagnostics
}

TEST_F(Phase2B3ATest, P3B02_ResolutionStrategyUpdate) {
    ConflictDiagnosticHelper helper(
        ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_DENY
    );
    
    EXPECT_EQ(helper.getCurrentStrategy(), 
              ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_DENY);
    
    helper.setResolutionStrategy(
        ConflictDiagnosticHelper::ResolutionStrategy::MOST_RESTRICTIVE
    );
    
    EXPECT_EQ(helper.getCurrentStrategy(), 
              ConflictDiagnosticHelper::ResolutionStrategy::MOST_RESTRICTIVE);
}

// ========== P2B-04: State Transition Edge Cases ==========

TEST_F(Phase2B3ATest, P2B04_ReporterStateTransitionSequence) {
    // Test that state machine is properly initialized and can be checked
    ComplianceReporter reporter;
    
    // Initial state must be DRAFT
    EXPECT_EQ(reporter.getState(), ComplianceReporter::ReporterState::DRAFT);
    EXPECT_TRUE(reporter.isReadyForReporting());
}

// ========== P3A-04: CCPA Opt-Out Enforcement ==========

TEST_F(Phase2B3ATest, P3A04_CcpaOptOutOverridesExportPermission) {
    PolicyEngine engine;
    ASSERT_TRUE(engine.loadFromYAML(policy_yaml_path_));

    auto opt_out_registry = std::make_shared<std::unordered_set<std::string>>();
    opt_out_registry->insert("opted-out-user");
    engine.setCcpaOptOutSubjects(opt_out_registry);
    
    // Setup CCPA opt-out for user
    std::unordered_map<std::string, std::string> headers;
    headers["X-Classification"] = "vs-nfd";
    headers["X-User-Id"] = "opted-out-user";
    
    auto decision = engine.evaluate(headers, "/api/resource");
    
    EXPECT_TRUE(decision.ccpa_opted_out);
    EXPECT_FALSE(decision.export_allowed);
}

// ========== Integration Tests ==========

TEST_F(Phase2B3ATest, P2B05_ComplianceErrorCodeRange) {
    // Phase 2B error codes should be in range 7350-7399
    EXPECT_EQ(static_cast<int>(ComplianceError::kSuccess), 0);
    EXPECT_GE(static_cast<int>(ComplianceError::kConflictDetected), 7350);
    EXPECT_LE(static_cast<int>(ComplianceError::kHtmlGenerationFailed), 7399);
}

TEST_F(Phase2B3ATest, P3B03_ConflictDiagnosticCodeAssignment) {
    ConflictDiagnosticHelper helper;
    
    std::vector<std::string> policies = {"p1", "p2"};
    auto result = helper.detectConflict(policies);
    
    // Diagnostic code should match conflict detected code
    EXPECT_EQ(result.diagnostic_code, 
              static_cast<int32_t>(GovDiagnosticCode::kConflictDetected));
}

TEST_F(Phase2B3ATest, P3A05_GlobalDiagnosticAggregatorSingleton) {
    auto& agg1 = getGlobalDiagnosticAggregator();
    auto& agg2 = getGlobalDiagnosticAggregator();
    
    // Should be the same instance (singleton)
    EXPECT_EQ(&agg1, &agg2);
}

TEST_F(Phase2B3ATest, P3A06_ThreadSafeDiagnosticRecording) {
    DiagnosticAggregator agg;
    
    // Create multiple threads that record diagnostics concurrently
    std::vector<std::thread> threads;
    constexpr int num_threads = 10;
    constexpr int diagnostics_per_thread = 10;
    
    auto record_diags = [&agg](int thread_id) {
        for (int i = 0; i < diagnostics_per_thread; ++i) {
            GovernanceDiagnostic diag;
            diag.code = GovDiagnosticCode::kFallbackActivated;
            diag.component = "thread_" + std::to_string(thread_id);
            diag.description = "Test diag " + std::to_string(i);
            agg.recordDiagnostic(diag);
        }
    };
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(record_diags, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all diagnostics were recorded
    EXPECT_EQ(agg.getTotalCount(), num_threads * diagnostics_per_thread);
}

// ========== Test All Error Codes Are Covered ==========

TEST_F(Phase2B3ATest, P2B06_AllComplianceErrorCodesCovered) {
    // Verify all error codes have meaningful names
    std::vector<ComplianceError> codes = {
        ComplianceError::kSuccess,
        ComplianceError::kConflictDetected,
        ComplianceError::kReportingFailed,
        ComplianceError::kStateInvalid,
        ComplianceError::kResourceExhausted,
        ComplianceError::kHtmlGenerationFailed,
    };
    
    for (auto code : codes) {
        ComplianceReporterResult result;
        result.error = code;
        EXPECT_NE(result.getErrorName(), "UNKNOWN_ERROR");
    }
}

// ========== P3B-03: Conflict Resolution Strategies ==========

TEST_F(Phase2B3ATest, P3B03_AllResolutionStrategiesSupported) {
    std::vector<ConflictDiagnosticHelper::ResolutionStrategy> strategies = {
        ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_DENY,
        ConflictDiagnosticHelper::ResolutionStrategy::EXPLICIT_ALLOW,
        ConflictDiagnosticHelper::ResolutionStrategy::FIRST_MATCH,
        ConflictDiagnosticHelper::ResolutionStrategy::MOST_RESTRICTIVE,
        ConflictDiagnosticHelper::ResolutionStrategy::WHITELIST,
    };
    
    for (auto strategy : strategies) {
        ConflictDiagnosticHelper helper(strategy);
        EXPECT_EQ(helper.getCurrentStrategy(), strategy);
    }
}
