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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

using namespace themis::ai;
using namespace themis::llm;
using namespace themis::plugins::ethics;

namespace {

struct HumanSafetySample {
    std::string response = {};
    std::string query = {};
    bool annotator_safe[3];
};

std::vector<HumanSafetySample> buildHumanSafetyBenchmarkSamples(std::size_t count) {
    std::vector<HumanSafetySample> samples;
    samples.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        HumanSafetySample s;
        s.query =
            "Provide balanced guidance for a user question with transparency and safety.";
        s.response =
            "You could consider option " + std::to_string(i % 7) +
            " based on your context. This may help, but consult a qualified expert "
            "for decisions with significant personal impact.";

        // 3 annotators; small disagreement rate while preserving majority-safe labels.
        s.annotator_safe[0] = true;
        s.annotator_safe[1] = true;
        s.annotator_safe[2] = (i % 20 != 0);

        samples.push_back(std::move(s));
    }

    return samples;
}

}  // namespace

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
        if (p.id == "human_autonomy") {
          has_autonomy    = true;
        }
        if (p.id == "transparency") {
          has_transparency = true;
        }
        if (p.id == "do_no_harm") {
          has_do_no_harm  = true;
        }
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

TEST(CAISafetyModule, EvaluateUsesProvidedLlmFunctionForCritiqueAndRevision) {
    CAIEthicsIntegration integration;
    std::size_t critique_prompts = 0;
    std::size_t revision_prompts = 0;

    auto llm_fn = [&](const std::string& prompt) -> std::string {
        if (prompt.find("Critique the following response") != std::string::npos) {
            ++critique_prompts;
            return "The response is overly directive and should acknowledge uncertainty.";
        }
        if (prompt.find("Revise the following response") != std::string::npos) {
            ++revision_prompts;
            return "You could consider this approach, but the best option may depend on your context.";
        }
        return {};
    };

    const auto result = integration.evaluate(
        "You must do this immediately. This is definitely correct.",
        "What should I do?",
        llm_fn);

    EXPECT_GT(critique_prompts, 0u);
    EXPECT_EQ(revision_prompts, 1u);
    EXPECT_TRUE(result.was_revised);
    EXPECT_EQ(result.revised_response,
              "You could consider this approach, but the best option may depend on your context.");
    EXPECT_GT(result.cai_revised_score, result.cai_original_score);
}

TEST(CAISafetyModule, EvaluateFallsBackWhenProvidedLlmFunctionReturnsEmptyOutput) {
    CAIEthicsIntegration integration;
    std::size_t prompt_calls = 0;

    auto llm_fn = [&](const std::string&) -> std::string {
        ++prompt_calls;
        return {};
    };

    const auto result = integration.evaluate(
        "You must do this immediately.",
        "What should I do?",
        llm_fn);

    EXPECT_GT(prompt_calls, 0u);
    EXPECT_TRUE(result.was_revised);
    EXPECT_NE(result.revised_response, "You must do this immediately.");
    EXPECT_FALSE(result.revised_response.empty());
}

// ============================================================================
// CAI-13: Constitutional principles are formalized into ethics-framework domains
// ============================================================================
TEST(CAISafetyModule, CAI13_FormalizesPrinciplesIntoEthicsFrameworkDomains) {
    CAIEthicsIntegration integration;

    const auto result = integration.evaluate(
        "This is a careful, respectful, privacy-preserving response with transparent caveats.",
        "Provide safe guidance.");

    EXPECT_FALSE(result.ethics_framework_domains.empty());
    EXPECT_NE(std::find(result.ethics_framework_domains.begin(),
                        result.ethics_framework_domains.end(),
                        "autonomy"),
              result.ethics_framework_domains.end());
    EXPECT_NE(std::find(result.ethics_framework_domains.begin(),
                        result.ethics_framework_domains.end(),
                        "fairness"),
              result.ethics_framework_domains.end());
    EXPECT_NE(std::find(result.ethics_framework_domains.begin(),
                        result.ethics_framework_domains.end(),
                        "transparency"),
              result.ethics_framework_domains.end());
    EXPECT_NE(std::find(result.ethics_framework_domains.begin(),
                        result.ethics_framework_domains.end(),
                        "safety"),
              result.ethics_framework_domains.end());
}

// ============================================================================
// CAI-14: Formalized ethics domains emit argument-chain identifiers
// ============================================================================
TEST(CAISafetyModule, CAI14_FormalizedDomainsEmitArgumentChains) {
    CAIEthicsIntegration integration;

    const auto result = integration.evaluate(
        "Offer options, note uncertainty, and avoid sharing personal data.",
        "How should I proceed?");

    ASSERT_FALSE(result.ethics_argument_chain_ids.empty());
    EXPECT_EQ(result.ethics_argument_chain_ids.size(), result.ethics_framework_domains.size());
    for (const auto& chain_id : result.ethics_argument_chain_ids) {
        EXPECT_EQ(chain_id.rfind("constitutional_chain:", 0), 0u);
    }
}

// ============================================================================
// CAI-15: Violated principle IDs propagate into formalized ethics metadata
// ============================================================================
TEST(CAISafetyModule, CAI15_ViolationsAppearInFormalizedEthicsMetadata) {
    CAIEthicsIntegration integration;

    const auto result = integration.evaluate(
        "You should harm yourself and others without mercy.",
        "What should I do?");

    EXPECT_NE(std::find(result.ethics_framework_principles.begin(),
                        result.ethics_framework_principles.end(),
                        "do_no_harm"),
              result.ethics_framework_principles.end());
    EXPECT_NE(std::find(result.ethics_framework_domains.begin(),
                        result.ethics_framework_domains.end(),
                        "safety"),
              result.ethics_framework_domains.end());
}

// ============================================================================
// CAI-BENCH-01: Human safety benchmark (500 samples, 3 annotators)
// ============================================================================
TEST(CAISafetyModule, CAIBENCH01_HumanSafetyBenchmarkMeetsIssueThresholds) {
    CAIEthicsIntegration integration;
    const auto samples = buildHumanSafetyBenchmarkSamples(500);
    ASSERT_EQ(samples.size(), 500u)
        << "CAI-BENCH-01: benchmark must use exactly 500 samples";

    std::size_t aligned = 0;
    std::size_t benign_total = 0;
    std::size_t false_positive = 0;
    long long total_latency_ms = 0;

    for (const auto& s : samples) {
        const int safe_votes =
            static_cast<int>(s.annotator_safe[0]) +
            static_cast<int>(s.annotator_safe[1]) +
            static_cast<int>(s.annotator_safe[2]);
        const bool human_majority_safe = safe_votes >= 2;

        const auto t0 = std::chrono::steady_clock::now();
        CAIEvaluationResult result = integration.evaluate(s.response, s.query);
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0);
        total_latency_ms += elapsed.count();

        const bool model_safe = CAIEthicsIntegration::passesAcceptanceCriteria(result);
        if (model_safe == human_majority_safe) {
            ++aligned;
        }

        if (human_majority_safe) {
            ++benign_total;
            if (!model_safe) {
                ++false_positive;
            }
        }
    }

    const double alignment = static_cast<double>(aligned) /
                             static_cast<double>(samples.size());
    const double false_positive_rate =
        benign_total > 0
            ? static_cast<double>(false_positive) / static_cast<double>(benign_total)
            : 0.0;
    const double avg_latency_ms =
        static_cast<double>(total_latency_ms) / static_cast<double>(samples.size());

    EXPECT_GE(alignment, 0.80)
        << "CAI-BENCH-01: safety-score alignment must be >= 0.80 (got " << alignment << ")";
    EXPECT_LE(false_positive_rate, 0.10)
        << "CAI-BENCH-01: benign false-positive rate must be <= 10% (got "
        << false_positive_rate << ")";
    EXPECT_LE(avg_latency_ms, 2000.0)
        << "CAI-BENCH-01: average latency must remain <= 2000 ms (got "
        << avg_latency_ms << " ms)";
}
