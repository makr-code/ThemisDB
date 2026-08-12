/**
 * @file test_reflection_integration.cpp
 * @brief Unit tests for the Reflection Tuning integration layer (v1.6.0).
 *
 * Acceptance criteria covered:
 *  AC-1  ILLMProviderReflectionAdapter — wraps ILLMProvider::complete().
 *  AC-2  ILLMProviderReflectionAdapter::generate() — forwards to complete().
 *  AC-3  ILLMProviderReflectionAdapter::critique() — builds critique prompt + calls complete().
 *  AC-4  ILLMProviderReflectionAdapter::revise() — builds revision prompt + calls complete().
 *  AC-5  ILLMProviderReflectionAdapter::score() — heuristic fallback when no scorer.
 *  AC-6  ILLMProviderReflectionAdapter::score() — delegates to IReflectionScorer when set.
 *  AC-7  ILLMProviderReflectionAdapter::name() — includes wrapped provider name.
 *  AC-8  ILLMProviderReflectionAdapter — strategy change propagates to prompt builder.
 *  AC-9  ILLMProviderReflectionAdapter — null provider does not crash.
 *  AC-10 ILLMProviderReflectionAdapter — scorer set/clear/has cycle.
 *  AC-11 PromptEngineeringMetrics — recordReflectionCycleStart increments counter.
 *  AC-12 PromptEngineeringMetrics — recordReflectionCycleComplete accumulates iterations.
 *  AC-13 PromptEngineeringMetrics — recordReflectionCycleComplete tracks improved flag.
 *  AC-14 PromptEngineeringMetrics — recordReflectionGuardFired increments counter.
 *  AC-15 PromptEngineeringMetrics — recordReflectionQualityDelta accumulates sum.
 *  AC-16 PromptEngineeringMetrics — exportMetrics() contains all new reflection metrics.
 *  AC-17 PromptEngineeringMetrics — snapshotToJson() contains reflection counters.
 *  AC-18 PromptEngineeringMetrics — restoreFromJson() restores reflection counters.
 *  AC-19 PromptEngineeringMetrics — reset() zeroes all reflection counters.
 *  AC-20 IntegrationConfig — reflection_enabled defaults to false.
 *  AC-21 IntegrationConfig — toJson() / fromJson() round-trip for reflection fields.
 *  AC-22 ILLMProviderReflectionAdapter — can be passed to ReflectionTuner.
 *  AC-23 ILLMProviderReflectionAdapter — critique uses SelfAwareContext for high-conf text.
 *  AC-24 PromptEngineeringMetrics — negative quality delta accumulated correctly.
 *  AC-25 ILLMProviderReflectionAdapter::heuristic — empty response scores 0.
 *  AC-26 ILLMProviderReflectionAdapter::heuristic — hallucination marker reduces score.
 *  AC-27 ILLMProviderReflectionAdapter::heuristic — structured response scores higher.
 *  AC-28 ILLMProviderReflectionAdapter — REFLEXION strategy yields different critique prompt.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/llm_reflection_adapter.h"
#include "prompt_engineering/reflection_tuner.h"
#include "prompt_engineering/prompt_engineering_metrics.h"
#include "prompt_engineering/prompt_engineering_integration.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Mock ILLMProvider
// ============================================================================

class RecordingLLMProvider : public ILLMProvider {
public:
    explicit RecordingLLMProvider(std::string echo_prefix = "LLM:")
        : prefix_(std::move(echo_prefix)) {}

    std::string complete(const std::string& prompt) const override {
        calls_.push_back(prompt);
        return prefix_ + prompt.substr(0, std::min(prompt.size(), size_t{40}));
    }
    std::string name() const override { return "recording-llm"; }

    const std::vector<std::string>& calls() const { return calls_; }
    void resetCalls() { calls_.clear(); }

private:
    std::string prefix_;
    mutable std::vector<std::string> calls_;
};

// ============================================================================
// Mock IReflectionScorer
// ============================================================================

class FixedScorer : public IReflectionScorer {
public:
    explicit FixedScorer(double s) : s_(s) {}
    double score(const std::string&, const std::string&) const override { return s_; }
private:
    double s_;
};

// ============================================================================
// AC-1 through AC-10: ILLMProviderReflectionAdapter
// ============================================================================

TEST(ILLMProviderReflectionAdapterTest, ConstructsWithProvider) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    EXPECT_FALSE(adapter.hasScorer());
}

TEST(ILLMProviderReflectionAdapterTest, GenerateForwardsToComplete) {
    auto llm = std::make_shared<RecordingLLMProvider>("ECHO:");
    ILLMProviderReflectionAdapter adapter(llm);
    const std::string result = adapter.generate("hello world");
    EXPECT_FALSE(result.empty());
    ASSERT_EQ(llm->calls().size(), 1u);
    EXPECT_EQ(llm->calls()[0], "hello world");
}

TEST(ILLMProviderReflectionAdapterTest, CritiqueCallsComplete) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    const std::string result = adapter.critique("task", "response");
    EXPECT_FALSE(result.empty());
    ASSERT_EQ(llm->calls().size(), 1u);
    // The critique prompt should contain the original task and response.
    EXPECT_NE(llm->calls()[0].find("task"), std::string::npos);
    EXPECT_NE(llm->calls()[0].find("response"), std::string::npos);
}

TEST(ILLMProviderReflectionAdapterTest, ReviseCallsComplete) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    const std::string result = adapter.revise("task", "response", "the critique");
    EXPECT_FALSE(result.empty());
    ASSERT_EQ(llm->calls().size(), 1u);
    EXPECT_NE(llm->calls()[0].find("the critique"), std::string::npos);
}

TEST(ILLMProviderReflectionAdapterTest, ScoreHeuristicFallbackNonEmpty) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    const double s = adapter.score("task", "some response text");
    EXPECT_GT(s, 0.0);
    EXPECT_LE(s, 1.0);
}

TEST(ILLMProviderReflectionAdapterTest, ScoreDelegatesToCustomScorer) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    adapter.setScorer(std::make_shared<FixedScorer>(0.77));
    EXPECT_NEAR(adapter.score("task", "response"), 0.77, 1e-9);
    EXPECT_TRUE(adapter.hasScorer());
}

TEST(ILLMProviderReflectionAdapterTest, NameContainsWrappedProviderName) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    EXPECT_NE(adapter.name().find("recording-llm"), std::string::npos);
}

TEST(ILLMProviderReflectionAdapterTest, StrategyChangePropagatesIntoCritiquePrompt) {
    auto llm_a = std::make_shared<RecordingLLMProvider>();
    auto llm_b = std::make_shared<RecordingLLMProvider>();

    ILLMProviderReflectionAdapter adapterA(llm_a, ReflectionStrategy::SELF_REFINE);
    ILLMProviderReflectionAdapter adapterB(llm_b, ReflectionStrategy::REFLEXION);

    adapterA.critique("task", "response");
    adapterB.critique("task", "response");

    ASSERT_EQ(llm_a->calls().size(), 1u);
    ASSERT_EQ(llm_b->calls().size(), 1u);
    // The two strategies produce different critique prompts.
    EXPECT_NE(llm_a->calls()[0], llm_b->calls()[0]);
}

TEST(ILLMProviderReflectionAdapterTest, NullProviderDoesNotCrashOnGenerate) {
    ILLMProviderReflectionAdapter adapter(nullptr);
    EXPECT_NO_THROW(adapter.generate("prompt"));
    EXPECT_TRUE(adapter.generate("prompt").empty());
}

TEST(ILLMProviderReflectionAdapterTest, ScorerSetClearHasCycle) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    EXPECT_FALSE(adapter.hasScorer());
    adapter.setScorer(std::make_shared<FixedScorer>(0.5));
    EXPECT_TRUE(adapter.hasScorer());
    adapter.clearScorer();
    EXPECT_FALSE(adapter.hasScorer());
}

// ============================================================================
// AC-11 through AC-19: PromptEngineeringMetrics reflection counters
// ============================================================================

TEST(ReflectionMetricsTest, RecordCycleStartIncrements) {
    PromptEngineeringMetrics m;
    m.recordReflectionCycleStart("p1");
    m.recordReflectionCycleStart("p1");
    const auto snapshot = m.snapshotToJson();
    EXPECT_EQ(snapshot["reflection_cycle_starts"].get<int64_t>(), 2);
}

TEST(ReflectionMetricsTest, RecordCycleCompleteAccumulatesIterations) {
    PromptEngineeringMetrics m;
    m.recordReflectionCycleComplete("p1", 3, true);
    m.recordReflectionCycleComplete("p1", 2, false);
    const auto snapshot = m.snapshotToJson();
    EXPECT_EQ(snapshot["reflection_cycle_completions"].get<int64_t>(), 2);
    EXPECT_EQ(snapshot["reflection_iterations_total"].get<int64_t>(), 5);
}

TEST(ReflectionMetricsTest, RecordCycleCompleteTracksImprovedFlag) {
    PromptEngineeringMetrics m;
    m.recordReflectionCycleComplete("p1", 1, true);
    m.recordReflectionCycleComplete("p2", 1, false);
    const auto snapshot = m.snapshotToJson();
    EXPECT_EQ(snapshot["reflection_improvements"].get<int64_t>(), 1);
}

TEST(ReflectionMetricsTest, RecordGuardFiredIncrements) {
    PromptEngineeringMetrics m;
    m.recordReflectionGuardFired("p1");
    const auto snapshot = m.snapshotToJson();
    EXPECT_EQ(snapshot["reflection_guard_fires"].get<int64_t>(), 1);
}

TEST(ReflectionMetricsTest, RecordQualityDeltaAccumulates) {
    PromptEngineeringMetrics m;
    m.recordReflectionQualityDelta("p1",  0.15);
    m.recordReflectionQualityDelta("p1",  0.05);
    const auto snapshot = m.snapshotToJson();
    EXPECT_NEAR(snapshot["reflection_quality_delta_sum"].get<double>(), 0.20, 1e-9);
}

TEST(ReflectionMetricsTest, ExportMetricsContainsReflectionEntries) {
    PromptEngineeringMetrics m;
    m.recordReflectionCycleStart("p1");
    m.recordReflectionGuardFired("p1");
    const std::string exported = m.exportMetrics();
    EXPECT_NE(exported.find("reflection_cycle_starts_total"), std::string::npos);
    EXPECT_NE(exported.find("reflection_guard_fires_total"),  std::string::npos);
    EXPECT_NE(exported.find("reflection_quality_delta_sum"),  std::string::npos);
}

TEST(ReflectionMetricsTest, SnapshotContainsAllReflectionKeys) {
    PromptEngineeringMetrics m;
    const auto j = m.snapshotToJson();
    EXPECT_TRUE(j.contains("reflection_cycle_starts"));
    EXPECT_TRUE(j.contains("reflection_cycle_completions"));
    EXPECT_TRUE(j.contains("reflection_iterations_total"));
    EXPECT_TRUE(j.contains("reflection_improvements"));
    EXPECT_TRUE(j.contains("reflection_guard_fires"));
    EXPECT_TRUE(j.contains("reflection_quality_delta_sum"));
}

TEST(ReflectionMetricsTest, RestoreFromJsonRestoresReflectionCounters) {
    PromptEngineeringMetrics m;
    m.recordReflectionCycleStart("p1");
    m.recordReflectionCycleStart("p1");
    m.recordReflectionGuardFired("p1");
    m.recordReflectionQualityDelta("p1", 0.3);
    const auto snapshot = m.snapshotToJson();

    PromptEngineeringMetrics m2;
    m2.restoreFromJson(snapshot);
    const auto snapshot2 = m2.snapshotToJson();

    EXPECT_EQ(snapshot2["reflection_cycle_starts"].get<int64_t>(), 2);
    EXPECT_EQ(snapshot2["reflection_guard_fires"].get<int64_t>(), 1);
    EXPECT_NEAR(snapshot2["reflection_quality_delta_sum"].get<double>(), 0.3, 1e-9);
}

TEST(ReflectionMetricsTest, ResetZeroesReflectionCounters) {
    PromptEngineeringMetrics m;
    m.recordReflectionCycleStart("p1");
    m.recordReflectionGuardFired("p1");
    m.recordReflectionQualityDelta("p1", 0.5);
    m.reset();
    const auto snapshot = m.snapshotToJson();
    EXPECT_EQ(snapshot["reflection_cycle_starts"].get<int64_t>(), 0);
    EXPECT_EQ(snapshot["reflection_guard_fires"].get<int64_t>(), 0);
    EXPECT_NEAR(snapshot["reflection_quality_delta_sum"].get<double>(), 0.0, 1e-9);
}

// ============================================================================
// AC-20 through AC-21: IntegrationConfig reflection fields
// ============================================================================

TEST(IntegrationConfigTest, ReflectionEnabledDefaultsFalse) {
    IntegrationConfig cfg;
    EXPECT_FALSE(cfg.enable_reflection_tuning);
}

TEST(IntegrationConfigTest, ReflectionMaxIterationsDefaultsThree) {
    IntegrationConfig cfg;
    EXPECT_EQ(cfg.reflection_max_iterations, 3u);
}

TEST(IntegrationConfigTest, ToFromJsonRoundTripReflectionFields) {
    IntegrationConfig cfg;
    cfg.enable_reflection_tuning  = true;
    cfg.reflection_max_iterations = 5;
    const auto j = cfg.toJson();
    const auto restored = IntegrationConfig::fromJson(j);
    EXPECT_TRUE(restored.enable_reflection_tuning);
    EXPECT_EQ(restored.reflection_max_iterations, 5u);
}

TEST(IntegrationConfigTest, FromJsonDefaultsReflectionEnabledFalse) {
    const auto j = nlohmann::json::object();  // empty → use defaults
    const auto cfg = IntegrationConfig::fromJson(j);
    EXPECT_FALSE(cfg.enable_reflection_tuning);
}

// ============================================================================
// AC-22: ILLMProviderReflectionAdapter can be passed to ReflectionTuner
// ============================================================================

TEST(ReflectionAdapterIntegrationTest, AdapterWorksAsReflectionTunerProvider) {
    auto llm = std::make_shared<RecordingLLMProvider>("ECHO:");
    auto adapter = std::make_shared<ILLMProviderReflectionAdapter>(llm);

    ReflectionConfig cfg;
    cfg.max_iterations = 1;
    cfg.hallucination_guard_enabled = false;
    cfg.convergence_threshold = 1.1;
    cfg.min_delta_improvement = 0.0;

    ReflectionTuner tuner(cfg);
    tuner.setReflectionProvider(adapter);

    EXPECT_NO_THROW(tuner.tune("task prompt", "initial response"));
    // At least generate(), critique(), revise(), score() must have been called.
    EXPECT_GE(llm->calls().size(), 2u);
}

// ============================================================================
// AC-23: critique injects self-aware context for confident response
// ============================================================================

TEST(ILLMProviderReflectionAdapterTest, CritiqueInjectsSelfAwareHeaderForHighConfidence) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm, ReflectionStrategy::SELF_REFINE);

    // High-confidence language triggers the overconfidence warning.
    const std::string confident_response =
        "Definitely and certainly this is exactly correct. I'm sure beyond doubt.";
    adapter.critique("task", confident_response);

    ASSERT_GE(llm->calls().size(), 1u);
    // The self-aware context header should mention overconfidence.
    const std::string& sent_prompt = llm->calls()[0];
    const bool header_present =
        sent_prompt.find("overconfidence") != std::string::npos ||
        sent_prompt.find("Self-Awareness") != std::string::npos;
    EXPECT_TRUE(header_present);
}

// ============================================================================
// AC-24: negative quality delta accumulated correctly
// ============================================================================

TEST(ReflectionMetricsTest, NegativeDeltaAccumulatedCorrectly) {
    PromptEngineeringMetrics m;
    m.recordReflectionQualityDelta("p1",  0.3);
    m.recordReflectionQualityDelta("p1", -0.5);
    const auto snapshot = m.snapshotToJson();
    EXPECT_NEAR(snapshot["reflection_quality_delta_sum"].get<double>(), -0.2, 1e-9);
}

// ============================================================================
// AC-25 through AC-27: heuristic scoring edge cases
// ============================================================================

TEST(ILLMProviderReflectionAdapterTest, HeuristicEmptyResponseScoresZero) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);
    EXPECT_NEAR(adapter.score("task", ""), 0.0, 1e-9);
}

TEST(ILLMProviderReflectionAdapterTest, HeuristicHallucinationMarkerReducesScore) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);

    // A clean response should score higher than one with a hallucination marker.
    const double clean  = adapter.score("t", "The GDPR was enacted in 2018.");
    const double bad    = adapter.score("t", "I cannot verify this information.");
    EXPECT_GT(clean, bad);
}

TEST(ILLMProviderReflectionAdapterTest, HeuristicStructuredResponseScoresHigher) {
    auto llm = std::make_shared<RecordingLLMProvider>();
    ILLMProviderReflectionAdapter adapter(llm);

    const double plain      = adapter.score("t", "The answer is yes.");
    const double structured = adapter.score("t",
        "Summary:\n1. Point A\n2. Point B\n- Detail: extra info here");
    EXPECT_GT(structured, plain);
}

// ============================================================================
// AC-28: REFLEXION strategy yields different critique prompt
// ============================================================================

TEST(ILLMProviderReflectionAdapterTest, ReflexionStrategyYieldsDifferentCritiquePrompt) {
    auto llm_sr = std::make_shared<RecordingLLMProvider>();
    auto llm_rf = std::make_shared<RecordingLLMProvider>();

    ILLMProviderReflectionAdapter self_refine_adapter(llm_sr, ReflectionStrategy::SELF_REFINE);
    ILLMProviderReflectionAdapter reflexion_adapter(llm_rf, ReflectionStrategy::REFLEXION);

    self_refine_adapter.critique("task", "response");
    reflexion_adapter.critique("task", "response");

    ASSERT_EQ(llm_sr->calls().size(), 1u);
    ASSERT_EQ(llm_rf->calls().size(), 1u);
    EXPECT_NE(llm_sr->calls()[0], llm_rf->calls()[0]);
}
