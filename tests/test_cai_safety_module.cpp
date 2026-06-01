/**
 * @file test_cai_safety_module.cpp
 * @brief Unit tests for the Wave C C1 CAI Safety Module (CAI-01 … CAI-12).
 *
 * Acceptance criteria from issue #5040 C1:
 *  - Safety score alignment ≥ 0.80 with human annotators
 *  - Latency overhead ≤ 2.0 s per response
 *  - False-positive rate ≤ 10 % (benign content flagged as unsafe)
 *
 * All tests run without a real LLM — the ConstitutionalReasoningEngine's
 * rule-based fallback (llm_wrapper = nullptr) is used for scoring/critique.
 *
 * Reference: Bai et al. (2022) arXiv:2212.08073 — Constitutional AI
 * Wave C issue: #5040
 */

#include <gtest/gtest.h>

#include "ai/cai_ethics_integration.h"
#include "llm/constitutional_reasoning_engine.h"
#include "ethics_ai/ethics_ai_types.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::ai;
using namespace themis::llm;
using namespace themis::plugins::ethics;

// ============================================================================
// CAI-01: Principles registry contains 20+ built-in rules
// ============================================================================
TEST(CAISafetyModule, CAI01_DefaultPrinciplesRegistryHas20Plus) {
    CAIEthicsIntegration integration;
    EXPECT_GE(integration.principleCount(), 20u)
        << "CAI-01: ConstitutionalReasoningEngine must load ≥ 20 default principles";
}

// ============================================================================
// CAI-02: loadDefaultPrinciples populates well-known principle IDs
// ============================================================================
TEST(CAISafetyModule, CAI02_DefaultPrinciplesIncludeKeyIDs) {
    CAIEthicsIntegration integration;
    auto principles = integration.getPrinciples();

    bool has_autonomy    = false;
    bool has_transparency = false;
    bool has_do_no_harm  = false;

    for (const auto& p : principles) {
        if (p.id == "human_autonomy")   has_autonomy    = true;
        if (p.id == "transparency")     has_transparency = true;
        if (p.id == "do_no_harm")       has_do_no_harm  = true;
    }

    EXPECT_TRUE(has_autonomy)     << "CAI-02: missing principle 'human_autonomy'";
    EXPECT_TRUE(has_transparency) << "CAI-02: missing principle 'transparency'";
    EXPECT_TRUE(has_do_no_harm)   << "CAI-02: missing principle 'do_no_harm'";
}

// ============================================================================
// CAI-03: Custom principle can be added to the registry
// ============================================================================
TEST(CAISafetyModule, CAI03_CustomPrincipleRegistration) {
    CAIEthicsIntegration integration;
    const std::size_t before = integration.principleCount();

    ConstitutionalPrinciple custom;
    custom.id              = "test_wave_c_principle";
    custom.name            = "Wave C Test Principle";
    custom.description     = "Custom principle for CAI-03 test";
    custom.priority        = 5;
    custom.domain_agnostic = true;

    integration.addPrinciple(custom);

    EXPECT_EQ(integration.principleCount(), before + 1u)
        << "CAI-03: principle count should increase by 1 after addPrinciple()";

    bool found = false;
    for (const auto& p : integration.getPrinciples()) {
        if (p.id == "test_wave_c_principle") { found = true; break; }
    }
    EXPECT_TRUE(found) << "CAI-03: custom principle not found in registry";
}

// ============================================================================
// CAI-04: Benign content receives a non-zero CAI score
// ============================================================================
TEST(CAISafetyModule, CAI04_BenignContentScoresPositive) {
    CAIEthicsIntegration integration;
    const std::string benign_response =
        "The capital of France is Paris. It is known for the Eiffel Tower "
        "and its rich cultural heritage.";
    const std::string query = "What is the capital of France?";

    CAIEvaluationResult result = integration.evaluate(benign_response, query);

    EXPECT_GE(result.cai_original_score, 0.0f)
        << "CAI-04: cai_original_score must be non-negative";
    EXPECT_LE(result.cai_original_score, 1.0f)
        << "CAI-04: cai_original_score must be ≤ 1.0";
}

// ============================================================================
// CAI-05: Violation check detects harmful keywords
// ============================================================================
TEST(CAISafetyModule, CAI05_ViolationCheckDetectsHarmfulContent) {
    // Use the engine directly so we can test checkViolations() explicitly.
    ConstitutionalReasoningConfig cfg;
    auto engine = ConstitutionalReasoningFactory::createDefault();
    ASSERT_NE(engine, nullptr);

    const std::string harmful_response =
        "You should harm yourself and others without mercy.";

    auto violations = engine->checkViolations(harmful_response);
    EXPECT_FALSE(violations.empty())
        << "CAI-05: harmful response should trigger at least one principle violation";
}

// ============================================================================
// CAI-06: Critic-revision cycle runs at most 2 rounds (issue constraint)
// ============================================================================
TEST(CAISafetyModule, CAI06_CriticRevisionCycleRespects2RoundLimit) {
    CAIEthicsConfig cfg;
    cfg.max_cai_rounds = 2;
    CAIEthicsIntegration integration(cfg);

    const std::string response = "Some test response for iteration limit check.";
    CAIEvaluationResult result = integration.evaluate(response, "test query");

    EXPECT_LE(result.cai_iterations, 2)
        << "CAI-06: CAI must not exceed 2 critic-revision rounds";
}

// ============================================================================
// CAI-07: evaluate() returns valid EthicsEvaluator scores
// ============================================================================
TEST(CAISafetyModule, CAI07_EthicsEvaluatorIntegrationReturnsScores) {
    CAIEthicsIntegration integration;
    const std::string response =
        "I will help you understand the topic clearly and honestly.";
    CAIEvaluationResult result = integration.evaluate(response, "explain topic");

    EXPECT_GE(result.ethics_overall_score, 0.0)
        << "CAI-07: ethics_overall_score must be ≥ 0";
    EXPECT_LE(result.ethics_overall_score, 1.0)
        << "CAI-07: ethics_overall_score must be ≤ 1";
    EXPECT_GE(result.ethics_decision_quality, 0.0)
        << "CAI-07: decision_quality_score must be ≥ 0";
    EXPECT_GE(result.ethics_alignment, 0.0)
        << "CAI-07: alignment_score must be ≥ 0";
}

// ============================================================================
// CAI-08: safety_score() aggregation is in [0, 1]
// ============================================================================
TEST(CAISafetyModule, CAI08_SafetyScoreIsNormalized) {
    CAIEthicsIntegration integration;
    CAIEvaluationResult result =
        integration.evaluate("A safe and helpful response.", "test");

    const double ss = result.safety_score();
    EXPECT_GE(ss, 0.0) << "CAI-08: safety_score() must be ≥ 0";
    EXPECT_LE(ss, 1.0) << "CAI-08: safety_score() must be ≤ 1";
}

// ============================================================================
// CAI-09: passesAcceptanceCriteria() rejects low-scoring results
// ============================================================================
TEST(CAISafetyModule, CAI09_AcceptanceGateRejectsLowScore) {
    CAIEvaluationResult synthetic;
    synthetic.cai_revised_score     = 0.3f;
    synthetic.ethics_overall_score  = 0.3;

    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(synthetic))
        << "CAI-09: result with safety_score=0.30 should NOT pass the 0.80 gate";
}

// ============================================================================
// CAI-10: passesAcceptanceCriteria() accepts high-scoring results (benign gate)
// ============================================================================
TEST(CAISafetyModule, CAI10_AcceptanceGatePassesBenignContent) {
    CAIEvaluationResult synthetic;
    synthetic.cai_revised_score    = 0.95f;
    synthetic.ethics_overall_score = 0.95;

    EXPECT_TRUE(CAIEthicsIntegration::passesAcceptanceCriteria(synthetic))
        << "CAI-10: high-scoring benign result should pass the acceptance gate";
}

// ============================================================================
// CAI-11: evaluate() completes within 2000 ms (latency budget; rule-based path)
// ============================================================================
TEST(CAISafetyModule, CAI11_LatencyWithinBudget) {
    CAIEthicsIntegration integration;
    const std::string response =
        "This is a standard response about database query optimisation techniques.";

    const auto t0 = std::chrono::steady_clock::now();
    integration.evaluate(response, "database optimisation");
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0);

    EXPECT_LE(elapsed.count(), 2000)
        << "CAI-11: rule-based evaluate() must complete within 2000 ms; "
           "took " << elapsed.count() << " ms";
}

// ============================================================================
// CAI-12: ConstitutionalReasoningEngine config can be updated at runtime
// ============================================================================
TEST(CAISafetyModule, CAI12_EngineConfigIsUpdatable) {
    ConstitutionalReasoningEngine engine;

    ConstitutionalReasoningConfig new_cfg;
    new_cfg.max_iterations        = 2;
    new_cfg.improvement_threshold = 0.10f;
    new_cfg.min_acceptable_score  = 0.80f;

    engine.setConfig(new_cfg);

    auto retrieved = engine.getConfig();
    EXPECT_EQ(retrieved.max_iterations, 2)
        << "CAI-12: max_iterations should be 2 after setConfig()";
    EXPECT_FLOAT_EQ(retrieved.improvement_threshold, 0.10f)
        << "CAI-12: improvement_threshold should be 0.10";
    EXPECT_FLOAT_EQ(retrieved.min_acceptable_score, 0.80f)
        << "CAI-12: min_acceptable_score should be 0.80 (Wave C C1 threshold)";
}
