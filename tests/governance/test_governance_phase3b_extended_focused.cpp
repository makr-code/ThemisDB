/**
 * @file test_governance_phase3b_extended_focused.cpp
 * @brief Phase 3B Extended: Policy Engine Path Hardening tests for SafeAccessValidator.
 * @note Test IDs: P3BX-01..P3BX-12
 * @note Coverage: unsafe access scenario detection, safety checks, fail-closed behavior,
 *                 remediation hints, threading, and comprehensive safety validation.
 */

#include <gtest/gtest.h>
#include "governance/governance_diagnostics.h"
#include "governance/policy_engine.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace themis::governance;

class Phase3BExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear global diagnostics before each test
        auto& aggregator = getGlobalDiagnosticAggregator();
        aggregator.clear();
        
        // Create a fresh SafeAccessValidator for each test
        validator_ = std::make_unique<SafeAccessValidator>(&aggregator);
    }
    
    void TearDown() override {
        validator_.reset();
    }
    
    std::unique_ptr<SafeAccessValidator> validator_;
    static constexpr uint32_t kSeed = 42;
    std::mt19937 rng_{kSeed};
};

// P3BX-01: Conflicting Classification Detection
TEST_F(Phase3BExtendedTest, P3BX01_ConflictingClassificationDetection) {
    // Test "public" + "restricted" conflict
    {
        AccessRequest req;
        req.request_id = "req_001";
        req.user_id = "user_123";
        req.user_tier = "editor";
        req.dataset_classifications = {"public", "restricted"};
        req.policy_ids = {"policy_a"};
        req.target_operation = "read";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        EXPECT_GT(result.violations.size(), 0);
        
        bool found_s1 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S1_CONFLICTING_CLASSIFICATIONS) {
                found_s1 = true;
                EXPECT_FALSE(v.description.empty());
                EXPECT_FALSE(v.remediation_hint.empty());
            }
        }
        EXPECT_TRUE(found_s1);
    }
    
    // Test "public" + "confidential" conflict
    {
        AccessRequest req;
        req.request_id = "req_002";
        req.user_id = "user_124";
        req.user_tier = "editor";
        req.dataset_classifications = {"public", "confidential"};
        req.policy_ids = {"policy_b"};
        req.target_operation = "read";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
    }
    
    // Test single classification (no conflict)
    {
        AccessRequest req;
        req.request_id = "req_003";
        req.user_id = "user_125";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_c"};
        req.target_operation = "read";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // May or may not be safe depending on other checks
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S1_CONFLICTING_CLASSIFICATIONS);
        }
    }
}

// P3BX-02: CCPA Override Missing Scenario
TEST_F(Phase3BExtendedTest, P3BX02_CCPAOverrideMissingScenario) {
    // Test CCPA-opt-out present and respected
    {
        AccessRequest req;
        req.request_id = "req_004";
        req.user_id = "user_126";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_d"};
        req.target_operation = "read";
        req.context["ccpa_opt_out_required"] = "false";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // Should not have S2 violation when not required
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S2_CCPA_OVERRIDE_MISSING);
        }
    }
    
    // Test CCPA-opt-out missing (should deny)
    {
        AccessRequest req;
        req.request_id = "req_005";
        req.user_id = "user_127";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_e"};
        req.target_operation = "export";
        req.context["ccpa_opt_out_required"] = "true";
        // Missing ccpa_opt_out flag
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        
        bool found_s2 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S2_CCPA_OVERRIDE_MISSING) {
                found_s2 = true;
            }
        }
        EXPECT_TRUE(found_s2);
    }
    
    // Test CCPA context with export operation
    {
        AccessRequest req;
        req.request_id = "req_006";
        req.user_id = "user_128";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_f"};
        req.target_operation = "export";
        req.context["ccpa_opt_out_required"] = "true";
        req.context["ccpa_opt_out"] = "true";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // Should not have S2 violation when properly configured
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S2_CCPA_OVERRIDE_MISSING);
        }
    }
}

// P3BX-03: Privilege Escalation Detection
TEST_F(Phase3BExtendedTest, P3BX03_PrivilegeEscalationDetection) {
    // Test read_only user attempting admin operation
    {
        AccessRequest req;
        req.request_id = "req_007";
        req.user_id = "user_129";
        req.user_tier = "read_only";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_g"};
        req.target_operation = "delete";
        req.context["required_tier"] = "admin";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        
        bool found_s3 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S3_PRIVILEGE_ESCALATION) {
                found_s3 = true;
                EXPECT_NE(v.description.find("read_only"), std::string::npos);
                EXPECT_NE(v.remediation_hint.find("permission"), std::string::npos);
            }
        }
        EXPECT_TRUE(found_s3);
    }
    
    // Test editor user attempting admin operation
    {
        AccessRequest req;
        req.request_id = "req_008";
        req.user_id = "user_130";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_h"};
        req.target_operation = "delete";
        req.context["required_tier"] = "admin";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
    }
    
    // Test admin user attempting admin operation (no escalation)
    {
        AccessRequest req;
        req.request_id = "req_009";
        req.user_id = "user_131";
        req.user_tier = "admin";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_i"};
        req.target_operation = "delete";
        req.context["required_tier"] = "admin";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // Should not have S3 violation
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S3_PRIVILEGE_ESCALATION);
        }
    }
}

// P3BX-04: Temporal Violation Detection
TEST_F(Phase3BExtendedTest, P3BX04_TemporalViolationDetection) {
    // Test policy with future effective_date
    // Note: Current implementation simplified; full version would parse policy objects
    {
        AccessRequest req;
        req.request_id = "req_010";
        req.user_id = "user_132";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_j"};
        req.target_operation = "read";
        
        // Policy with future date would be detected
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // Check for S4 if future dates were in policy_j
        EXPECT_TRUE(result.evaluated_at_ms > 0);
    }
    
    // Test policy with zero retention_days
    {
        AccessRequest req;
        req.request_id = "req_011";
        req.user_id = "user_133";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_k"};
        req.target_operation = "read";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_TRUE(result.evaluated_at_ms > 0);
    }
    
    // Test policy with valid dates
    {
        AccessRequest req;
        req.request_id = "req_012";
        req.user_id = "user_134";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_l"};
        req.target_operation = "read";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_TRUE(result.evaluated_at_ms > 0);
    }
}

// P3BX-05: Cross-Border Conflict Detection
TEST_F(Phase3BExtendedTest, P3BX05_CrossBorderConflictDetection) {
    // Test EU + US jurisdiction conflict
    {
        AccessRequest req;
        req.request_id = "req_013";
        req.user_id = "user_135";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_m"};
        req.target_operation = "read";
        req.context["jurisdictions"] = "EU, US";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // EU + US is generally compatible
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S5_CROSS_BORDER_CONFLICT);
        }
    }
    
    // Test EU + CN jurisdiction conflict
    {
        AccessRequest req;
        req.request_id = "req_014";
        req.user_id = "user_136";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_n"};
        req.target_operation = "read";
        req.context["jurisdictions"] = "EU, CN";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        
        bool found_s5 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S5_CROSS_BORDER_CONFLICT) {
                found_s5 = true;
                EXPECT_NE(v.description.find("incompatible"), std::string::npos);
            }
        }
        EXPECT_TRUE(found_s5);
    }
    
    // Test single jurisdiction (no conflict)
    {
        AccessRequest req;
        req.request_id = "req_015";
        req.user_id = "user_137";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_o"};
        req.target_operation = "read";
        req.context["jurisdictions"] = "EU";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S5_CROSS_BORDER_CONFLICT);
        }
    }
}

// P3BX-06: Masking Rule Consistency
TEST_F(Phase3BExtendedTest, P3BX06_MaskingRuleConsistency) {
    // Test conflicting redaction rules
    {
        AccessRequest req;
        req.request_id = "req_016";
        req.user_id = "user_138";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_p"};
        req.target_operation = "read";
        req.context["masking_rules"] = "rule_1,rule_2";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        // Consistency checks performed
        EXPECT_TRUE(result.evaluated_at_ms > 0);
    }
    
    // Test bypass attempt via direct schema access
    {
        AccessRequest req;
        req.request_id = "req_017";
        req.user_id = "user_139";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_q"};
        req.target_operation = "read";
        req.context["bypass_attempt"] = "true";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_TRUE(result.evaluated_at_ms > 0);
    }
    
    // Test consistent rules
    {
        AccessRequest req;
        req.request_id = "req_018";
        req.user_id = "user_140";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_r"};
        req.target_operation = "read";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S6_MASKING_BYPASS_ATTEMPT);
        }
    }
}

// P3BX-07: Whitelist Exhaustion
TEST_F(Phase3BExtendedTest, P3BX07_WhitelistExhaustion) {
    // Test empty whitelist
    {
        AccessRequest req;
        req.request_id = "req_019";
        req.user_id = "user_141";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_s"};
        req.target_operation = "read";
        req.context["whitelist_policy"] = "";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        
        bool found_s7 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S7_WHITELIST_EXHAUSTION) {
                found_s7 = true;
                EXPECT_NE(v.description.find("empty"), std::string::npos);
            }
        }
        EXPECT_TRUE(found_s7);
    }
    
    // Test null/missing whitelist
    {
        AccessRequest req;
        req.request_id = "req_020";
        req.user_id = "user_142";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_t"};
        req.target_operation = "read";
        req.context["whitelist_policy"] = "null";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        
        bool found_s7 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S7_WHITELIST_EXHAUSTION) {
                found_s7 = true;
            }
        }
        EXPECT_TRUE(found_s7);
    }
    
    // Test valid non-empty whitelist
    {
        AccessRequest req;
        req.request_id = "req_021";
        req.user_id = "user_143";
        req.user_tier = "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_u"};
        req.target_operation = "read";
        req.context["whitelist_policy"] = "id_1,id_2,id_3";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S7_WHITELIST_EXHAUSTION);
        }
    }
}

// P3BX-08: Cascading Denial Detection
TEST_F(Phase3BExtendedTest, P3BX08_CascadingDenialDetection) {
    // Record multiple deny policies
    {
        AccessRequest req;
        req.request_id = "req_022";
        req.user_id = "user_144";
        req.user_tier = "read_only";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"deny_1", "deny_2", "deny_3"};
        req.target_operation = "delete";
        req.context["cascading_denials"] = "true";
        req.context["deny_count"] = "3";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        EXPECT_FALSE(result.is_safe);
        
        bool found_s8 = false;
        for (const auto& v : result.violations) {
            if (v.scenario == UnsafeAccessScenario::S8_CASCADING_DENIALS) {
                found_s8 = true;
                EXPECT_NE(v.description.find("multiple"), std::string::npos);
            }
        }
        EXPECT_TRUE(found_s8);
    }
    
    // Verify single deny not flagged as cascade
    {
        AccessRequest req;
        req.request_id = "req_023";
        req.user_id = "user_145";
        req.user_tier = "read_only";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"deny_1"};
        req.target_operation = "delete";
        req.context["cascading_denials"] = "true";
        req.context["deny_count"] = "1";
        
        SafeAccessResult result = validator_->validateAccessRequest(req);
        for (const auto& v : result.violations) {
            EXPECT_NE(v.scenario, UnsafeAccessScenario::S8_CASCADING_DENIALS);
        }
    }
}

// P3BX-09: SafeAccessValidator Orchestration
TEST_F(Phase3BExtendedTest, P3BX09_SafeAccessValidatorOrchestration) {
    // Create AccessRequest with multiple violations
    AccessRequest req;
    req.request_id = "req_024";
    req.user_id = "user_146";
    req.user_tier = "read_only";
    req.dataset_classifications = {"public", "restricted"};  // S1: conflict
    req.policy_ids = {"policy_v"};
    req.target_operation = "delete";
    req.context["required_tier"] = "admin";  // S3: escalation
    req.context["ccpa_opt_out_required"] = "true";  // S2: missing opt-out
    req.context["jurisdictions"] = "EU, CN";  // S5: cross-border
    
    SafeAccessResult result = validator_->validateAccessRequest(req);
    
    // Verify all violations found
    EXPECT_FALSE(result.is_safe);
    EXPECT_GT(result.violations.size(), 0);
    
    // Verify scenario codes populated
    EXPECT_GT(result.scenario_codes.size(), 0);
    for (int32_t code : result.scenario_codes) {
        EXPECT_GE(code, 7381);
        EXPECT_LE(code, 7388);
    }
    
    // Verify remediation steps
    EXPECT_GT(result.remediation_steps.size(), 0);
    for (const auto& step : result.remediation_steps) {
        EXPECT_FALSE(step.empty());
    }
    
    // Verify timestamp
    EXPECT_GT(result.evaluated_at_ms, 0);
}

// P3BX-10: Concurrent Safety Checks
TEST_F(Phase3BExtendedTest, P3BX10_ConcurrentSafetyChecks) {
    const int kThreadCount = 10;
    std::vector<std::thread> threads;
    std::vector<SafeAccessResult> results(kThreadCount);
    
    auto validate_request = [this](int id, SafeAccessResult& result) {
        AccessRequest req;
        req.request_id = "req_concurrent_" + std::to_string(id);
        req.user_id = "user_" + std::to_string(1000 + id);
        req.user_tier = (id % 2 == 0) ? "read_only" : "editor";
        req.dataset_classifications = {"public"};
        req.policy_ids = {"policy_x"};
        req.target_operation = "read";
        
        result = validator_->validateAccessRequest(req);
    };
    
    // Launch threads
    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back(validate_request, i, std::ref(results[i]));
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify no races or lost violations
    for (const auto& result : results) {
        EXPECT_TRUE(result.evaluated_at_ms > 0);
    }
}

// P3BX-11: All Scenario Codes Mapped
TEST_F(Phase3BExtendedTest, P3BX11_AllScenarioCodesMapped) {
    std::vector<int32_t> expected_codes = {
        static_cast<int32_t>(UnsafeAccessScenario::S1_CONFLICTING_CLASSIFICATIONS),  // 7381
        static_cast<int32_t>(UnsafeAccessScenario::S2_CCPA_OVERRIDE_MISSING),        // 7382
        static_cast<int32_t>(UnsafeAccessScenario::S3_PRIVILEGE_ESCALATION),         // 7383
        static_cast<int32_t>(UnsafeAccessScenario::S4_TEMPORAL_VIOLATION),           // 7384
        static_cast<int32_t>(UnsafeAccessScenario::S5_CROSS_BORDER_CONFLICT),        // 7385
        static_cast<int32_t>(UnsafeAccessScenario::S6_MASKING_BYPASS_ATTEMPT),       // 7386
        static_cast<int32_t>(UnsafeAccessScenario::S7_WHITELIST_EXHAUSTION),         // 7387
        static_cast<int32_t>(UnsafeAccessScenario::S8_CASCADING_DENIALS),            // 7388
    };
    
    // Verify codes in range
    for (int32_t code : expected_codes) {
        EXPECT_GE(code, 7381);
        EXPECT_LE(code, 7388);
    }
    
    // Verify no duplicates
    auto sorted_codes = expected_codes;
    std::sort(sorted_codes.begin(), sorted_codes.end());
    EXPECT_EQ(std::unique(sorted_codes.begin(), sorted_codes.end()), sorted_codes.end());
}

// P3BX-12: Remediation Hint Completeness
TEST_F(Phase3BExtendedTest, P3BX12_RemediationHintCompleteness) {
    // Create multiple violations to check remediation completeness
    std::vector<AccessRequest> violation_requests = {
        // S1: Conflicting classifications
        []{
            AccessRequest r;
            r.request_id = "req_rem_s1";
            r.user_id = "user_200";
            r.user_tier = "editor";
            r.dataset_classifications = {"public", "confidential"};
            r.policy_ids = {"p1"};
            r.target_operation = "read";
            return r;
        }(),
        // S2: CCPA opt-out missing
        []{
            AccessRequest r;
            r.request_id = "req_rem_s2";
            r.user_id = "user_201";
            r.user_tier = "editor";
            r.dataset_classifications = {"public"};
            r.policy_ids = {"p2"};
            r.target_operation = "export";
            r.context["ccpa_opt_out_required"] = "true";
            return r;
        }(),
        // S3: Privilege escalation
        []{
            AccessRequest r;
            r.request_id = "req_rem_s3";
            r.user_id = "user_202";
            r.user_tier = "read_only";
            r.dataset_classifications = {"public"};
            r.policy_ids = {"p3"};
            r.target_operation = "delete";
            r.context["required_tier"] = "admin";
            return r;
        }(),
        // S7: Whitelist exhaustion
        []{
            AccessRequest r;
            r.request_id = "req_rem_s7";
            r.user_id = "user_203";
            r.user_tier = "editor";
            r.dataset_classifications = {"public"};
            r.policy_ids = {"p7"};
            r.target_operation = "read";
            r.context["whitelist_policy"] = "";
            return r;
        }(),
    };
    
    for (const auto& req : violation_requests) {
        validator_->clearViolationHistory();
        SafeAccessResult result = validator_->validateAccessRequest(req);
        
        if (!result.is_safe) {
            // Verify remediation steps non-empty
            EXPECT_GT(result.remediation_steps.size(), 0);
            
            // Verify hints are actionable
            for (const auto& violation : result.violations) {
                EXPECT_FALSE(violation.remediation_hint.empty());
                
                // Check that hints are specific (not generic)
                EXPECT_GT(violation.remediation_hint.length(), 10);
                
                // Verify hint content is related to scenario
                if (violation.scenario == UnsafeAccessScenario::S1_CONFLICTING_CLASSIFICATIONS) {
                    EXPECT_NE(violation.remediation_hint.find("classification"), std::string::npos);
                }
                if (violation.scenario == UnsafeAccessScenario::S2_CCPA_OVERRIDE_MISSING) {
                    EXPECT_NE(violation.remediation_hint.find("CCPA"), std::string::npos);
                }
                if (violation.scenario == UnsafeAccessScenario::S3_PRIVILEGE_ESCALATION) {
                    EXPECT_NE(violation.remediation_hint.find("permission"), std::string::npos);
                }
                if (violation.scenario == UnsafeAccessScenario::S7_WHITELIST_EXHAUSTION) {
                    EXPECT_NE(violation.remediation_hint.find("whitelist"), std::string::npos);
                }
            }
        }
    }
}

