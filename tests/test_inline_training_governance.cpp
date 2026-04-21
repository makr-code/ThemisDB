/**
 * @file test_inline_training_governance.cpp
 * @brief Unit tests for the governance policy gate wired into InlineTrainingEngine
 *        (Gap 3 — AI_ML_IMPACT_ASSESSMENT.md §7, Severity: High/S0).
 *
 * Test groups
 * -----------
 * Group A (4 tests): ModelGovernancePolicy permit / deny behaviour
 *   The gate in InlineTrainingEngine::train() calls
 *   ModelGovernancePolicy::checkExportPermission().  These tests verify that
 *   the policy itself returns the expected decisions; the InlineTrainingEngine
 *   integration with full training data dependencies is covered in
 *   test_advanced_training_features.cpp (themis_core linkage required).
 *
 *   ITE_GOV_A1  Policy with no restrictions → is_permitted=true
 *   ITE_GOV_A2  Policy with restricted collection → is_permitted=false
 *   ITE_GOV_A3  Denial reason is descriptive
 *   ITE_GOV_A4  Geheim classification → always denied regardless of restrictions
 *
 * Group B (2 tests): InlineTrainingConfig compile-time API verification
 *   ITE_GOV_B1  require_policy_gate field defaults to false
 *   ITE_GOV_B2  require_policy_gate can be set to true without side effects
 *
 * Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 3
 * Tracked: src/llm/FUTURE_ENHANCEMENTS.md §"Inline Training Policy Gate"
 */

#include <gtest/gtest.h>

#include "governance/model_governance.h"
#include "llm/inline_training_engine.h"   // for InlineTrainingConfig; validates API

using themis::governance::ModelGovernancePolicy;
using themis::governance::ModelTrainingExportRequest;
using themis::governance::ModelGovernanceDecision;

// ---------------------------------------------------------------------------
// Group A – ModelGovernancePolicy permit / deny behaviour
// ---------------------------------------------------------------------------

class ITE_GOV_PolicyTest : public ::testing::Test {
protected:
    ModelGovernancePolicy policy;

    static ModelTrainingExportRequest makeRequest(
        const std::string& adapter_id = "test-adapter",
        const std::string& classification = "offen")
    {
        ModelTrainingExportRequest r;
        r.export_job_id    = adapter_id;
        r.adapter_id       = adapter_id;
        r.requesting_user  = "InlineTrainingEngine";
        r.purpose          = "MODEL_TRAINING";
        r.classification   = classification;
        return r;
    }
};

TEST_F(ITE_GOV_PolicyTest, ITE_GOV_A1_NoRestrictionsPermits) {
    // No restricted collections and an "offen" classification → PERMIT
    const auto decision = policy.checkExportPermission(makeRequest());
    EXPECT_TRUE(decision.is_permitted)
        << "Policy with no restrictions must permit 'offen' requests; "
        << "denial_reason='" << decision.denial_reason << "'";
}

TEST_F(ITE_GOV_PolicyTest, ITE_GOV_A2_RestrictedCollectionDenies) {
    policy.addRestrictedCollection("confidential-corpus");

    ModelTrainingExportRequest req = makeRequest();
    req.collection_ids = {"confidential-corpus"};

    const auto decision = policy.checkExportPermission(req);
    EXPECT_FALSE(decision.is_permitted)
        << "Policy must deny when a restricted collection is listed";
}

TEST_F(ITE_GOV_PolicyTest, ITE_GOV_A3_DenialReasonIsDescriptive) {
    policy.addRestrictedCollection("restricted-data");

    ModelTrainingExportRequest req = makeRequest();
    req.collection_ids = {"restricted-data"};

    const auto decision = policy.checkExportPermission(req);
    ASSERT_FALSE(decision.is_permitted);
    EXPECT_FALSE(decision.denial_reason.empty())
        << "denial_reason must be non-empty on DENY decisions";
}

TEST_F(ITE_GOV_PolicyTest, ITE_GOV_A4_GeheimClassificationAlwaysDenied) {
    // No restricted collections, but classification="geheim" must still be denied.
    const auto decision = policy.checkExportPermission(makeRequest("any-adapter", "geheim"));
    EXPECT_FALSE(decision.is_permitted)
        << "Classification 'geheim' must always be denied for model training";
}

// ---------------------------------------------------------------------------
// Group B – InlineTrainingConfig compile-time API verification
// ---------------------------------------------------------------------------

TEST(ITE_GOV_Config, ITE_GOV_B1_RequirePolicyGateDefaultsFalse) {
    // Verify the new field defaults to false so existing code is unaffected.
    themis::llm::InlineTrainingConfig cfg;
    EXPECT_FALSE(cfg.require_policy_gate)
        << "require_policy_gate must default to false for backward compatibility";
}

TEST(ITE_GOV_Config, ITE_GOV_B2_RequirePolicyGateCanBeSetTrue) {
    themis::llm::InlineTrainingConfig cfg;
    cfg.require_policy_gate = true;
    EXPECT_TRUE(cfg.require_policy_gate);
}
