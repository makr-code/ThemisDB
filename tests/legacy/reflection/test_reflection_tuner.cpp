/**
 * @file test_reflection_tuner.cpp
 * @brief Unit tests for the Reflection Tuning module.
 *
 * Acceptance criteria covered:
 *  AC-1  ReflectionConfig — defaults are sane and all fields accessible.
 *  AC-2  SelfAwareContext::fromResponse — uncertainty / confidence detection.
 *  AC-3  SelfAwareContext::toJson — serialises all fields.
 *  AC-4  DynamicReflectionPromptBuilder — all four strategy critique prompts.
 *  AC-5  DynamicReflectionPromptBuilder — revision prompts include critique text.
 *  AC-6  DynamicReflectionPromptBuilder — constitutional critique prompt.
 *  AC-7  DynamicReflectionPromptBuilder — Socratic prompt cycles questions.
 *  AC-8  DynamicReflectionPromptBuilder — self-aware context header injection.
 *  AC-9  ReflectionHallucinationGuard — detectHallucinationSignals on markers.
 *  AC-10 ReflectionHallucinationGuard — isDiverging trajectory detection.
 *  AC-11 ReflectionHallucinationGuard — shouldHalt combines both checks.
 *  AC-12 ReflectionTuner — fallback mode (no provider) completes without crash.
 *  AC-13 ReflectionTuner — quality_trajectory has initial + per-step entries.
 *  AC-14 ReflectionTuner — halted_by_hallucination_guard set when guard fires.
 *  AC-15 ReflectionTuner — converged set when score >= convergence_threshold.
 *  AC-16 ReflectionTuner — converged set on plateau (delta < min_delta).
 *  AC-17 ReflectionTuner — max_iterations respected.
 *  AC-18 ReflectionTuner — with mock provider: critique/revise/score all called.
 *  AC-19 ReflectionTuner — setConfig replaces strategy and guard thresholds.
 *  AC-20 ReflectionResult::toJson — serialises all required fields.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/reflection_tuner.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Mock IReflectionProvider implementations
// ============================================================================

/**
 * @brief Provider that always returns fixed strings and a constant score.
 */
class ConstantMockProvider : public IReflectionProvider {
public:
    ConstantMockProvider(std::string gen_resp,
                         std::string critique_resp,
                         std::string revised_resp,
                         double constant_score)
        : gen_(std::move(gen_resp))
        , crit_(std::move(critique_resp))
        , rev_(std::move(revised_resp))
        , score_(constant_score) {}

    std::string generate(const std::string&) const override { return gen_; }
    std::string critique(const std::string&, const std::string&) const override { return crit_; }
    std::string revise(const std::string&, const std::string&, const std::string&) const override { return rev_; }
    double      score(const std::string&, const std::string&) const override { return score_; }
    std::string name() const override { return "constant-mock"; }

private:
    std::string gen_;
    std::string crit_;
    std::string rev_;
    double      score_;
};

/**
 * @brief Provider that improves the score by a fixed delta each revision call,
 *        up to a configured maximum.
 */
class ImprovingMockProvider : public IReflectionProvider {
public:
    explicit ImprovingMockProvider(double start_score  = 0.5,
                                   double delta        = 0.2,
                                   double max_score    = 1.0)
        : start_(start_score), delta_(delta), max_(max_score), calls_(0) {}

    std::string generate(const std::string& p) const override { return "initial: " + p; }
    std::string critique(const std::string&, const std::string&) const override {
        return "Consider improving clarity and completeness.";
    }
    std::string revise(const std::string&, const std::string& r, const std::string&) const override {
        return "improved: " + r;
    }
    double score(const std::string&, const std::string&) const override {
        const double s = std::min(max_, start_ + static_cast<double>(calls_) * delta_);
        ++calls_;
        return s;
    }
    std::string name() const override { return "improving-mock"; }

private:
    double start_;
    double delta_;
    double max_;
    mutable size_t calls_;
};

/**
 * @brief Provider that injects a hallucination marker into the critique.
 */
class HallucinatingCritiqueProvider : public IReflectionProvider {
public:
    std::string generate(const std::string& p) const override { return p; }
    std::string critique(const std::string&, const std::string&) const override {
        return "I cannot verify this information. The response may be incorrect.";
    }
    std::string revise(const std::string&, const std::string& r, const std::string&) const override {
        return r;
    }
    double      score(const std::string&, const std::string&) const override { return 0.4; }
    std::string name() const override { return "hallucinating-mock"; }
};

/**
 * @brief Provider whose scores decline on each call (simulates divergence).
 */
class DivertingMockProvider : public IReflectionProvider {
public:
    explicit DivertingMockProvider(double start = 0.8, double drop = 0.2)
        : start_(start), drop_(drop), calls_(0) {}

    std::string generate(const std::string& p) const override { return p; }
    std::string critique(const std::string&, const std::string&) const override {
        return "Critique: response needs work.";
    }
    std::string revise(const std::string&, const std::string& r, const std::string&) const override {
        return r;
    }
    double score(const std::string&, const std::string&) const override {
        const double s = std::max(0.0, start_ - static_cast<double>(calls_) * drop_);
        ++calls_;
        return s;
    }
    std::string name() const override { return "diverging-mock"; }

private:
    double start_;
    double drop_;
    mutable size_t calls_;
};

// ============================================================================
// AC-1: ReflectionConfig defaults
// ============================================================================

TEST(ReflectionConfigTest, DefaultStrategyIsSelfRefine) {
    ReflectionConfig cfg;
    EXPECT_EQ(cfg.strategy, ReflectionStrategy::SELF_REFINE);
}

TEST(ReflectionConfigTest, DefaultMaxIterationsIsThree) {
    ReflectionConfig cfg;
    EXPECT_EQ(cfg.max_iterations, 3u);
}

TEST(ReflectionConfigTest, DefaultHallucinationGuardEnabled) {
    ReflectionConfig cfg;
    EXPECT_TRUE(cfg.hallucination_guard_enabled);
}

TEST(ReflectionConfigTest, DefaultSelfAwareContextEnabled) {
    ReflectionConfig cfg;
    EXPECT_TRUE(cfg.include_self_aware_context);
}

TEST(ReflectionConfigTest, DefaultDivergenceThresholdIsPositive) {
    ReflectionConfig cfg;
    EXPECT_GT(cfg.divergence_threshold, 0.0);
}

// ============================================================================
// AC-2: SelfAwareContext::fromResponse — uncertainty / confidence detection
// ============================================================================

TEST(SelfAwareContextTest, NeutralTextYieldsMidConfidence) {
    auto ctx = SelfAwareContext::fromResponse("The answer is 42.");
    EXPECT_NEAR(ctx.confidence, 0.7, 1e-9);
    EXPECT_FALSE(ctx.has_uncertain_claims);
    EXPECT_TRUE(ctx.uncertainty_markers.empty());
}

TEST(SelfAwareContextTest, UncertaintyMarkersDetected) {
    auto ctx = SelfAwareContext::fromResponse("I think this might be correct.");
    EXPECT_TRUE(ctx.has_uncertain_claims);
    EXPECT_FALSE(ctx.uncertainty_markers.empty());
    EXPECT_LT(ctx.confidence, 0.7);
}

TEST(SelfAwareContextTest, ConfidenceMarkersDetected) {
    auto ctx = SelfAwareContext::fromResponse("This is definitely correct and I'm certain.");
    EXPECT_GT(ctx.confidence, 0.7);
}

TEST(SelfAwareContextTest, MixedMarkersYieldsIntermediateConfidence) {
    // One confidence marker ("certainly") vs one uncertainty marker ("maybe").
    auto ctx = SelfAwareContext::fromResponse("Certainly this is right, but maybe not entirely.");
    EXPECT_GT(ctx.confidence, 0.0);
    EXPECT_LT(ctx.confidence, 1.0);
}

TEST(SelfAwareContextTest, EmptyResponseYieldsNeutralDefaults) {
    auto ctx = SelfAwareContext::fromResponse("");
    EXPECT_NEAR(ctx.confidence, 0.7, 1e-9);
    EXPECT_FALSE(ctx.has_uncertain_claims);
}

// ============================================================================
// AC-3: SelfAwareContext::toJson
// ============================================================================

TEST(SelfAwareContextTest, ToJsonContainsConfidenceField) {
    SelfAwareContext ctx;
    ctx.confidence = 0.42;
    auto j = ctx.toJson();
    EXPECT_TRUE(j.contains("confidence"));
    EXPECT_NEAR(j["confidence"].get<double>(), 0.42, 1e-9);
}

TEST(SelfAwareContextTest, ToJsonContainsHasUncertainClaims) {
    SelfAwareContext ctx;
    ctx.has_uncertain_claims = true;
    auto j = ctx.toJson();
    EXPECT_TRUE(j.contains("has_uncertain_claims"));
    EXPECT_TRUE(j["has_uncertain_claims"].get<bool>());
}

TEST(SelfAwareContextTest, ToJsonContainsUncertaintyMarkersList) {
    SelfAwareContext ctx;
    ctx.uncertainty_markers = {"i think", "maybe"};
    auto j = ctx.toJson();
    EXPECT_TRUE(j.contains("uncertainty_markers"));
    EXPECT_EQ(j["uncertainty_markers"].size(), 2u);
}

// ============================================================================
// AC-4: DynamicReflectionPromptBuilder — critique prompts per strategy
// ============================================================================

TEST(DynamicReflectionPromptBuilderTest, SelfRefineContainsOriginalTask) {
    DynamicReflectionPromptBuilder builder(ReflectionStrategy::SELF_REFINE);
    auto prompt = builder.buildCritiquePrompt("original task", "response text");
    EXPECT_NE(prompt.find("original task"), std::string::npos);
    EXPECT_NE(prompt.find("response text"), std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, ReflexionContainsReflectKeyword) {
    DynamicReflectionPromptBuilder builder(ReflectionStrategy::REFLEXION);
    auto prompt = builder.buildCritiquePrompt("task", "response");
    EXPECT_NE(prompt.find("reflect"), std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, ConstitutionalContainsHarmlessness) {
    DynamicReflectionPromptBuilder builder(ReflectionStrategy::CONSTITUTIONAL);
    auto prompt = builder.buildCritiquePrompt("task", "response");
    EXPECT_NE(prompt.find("harmless"), std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, SocraticContainsAssumptionOrQuestion) {
    DynamicReflectionPromptBuilder builder(ReflectionStrategy::SOCRATIC);
    auto prompt = builder.buildCritiquePrompt("task", "response");
    EXPECT_NE(prompt.find("assumption") + prompt.find("question") +
              prompt.find("challenge"), std::string::npos * 3 - 2);
    // At least one of the three words must appear.
    bool found = (prompt.find("assumption") != std::string::npos) ||
                 (prompt.find("question")   != std::string::npos) ||
                 (prompt.find("challenge")  != std::string::npos);
    EXPECT_TRUE(found);
}

// ============================================================================
// AC-5: DynamicReflectionPromptBuilder — revision prompts include critique
// ============================================================================

TEST(DynamicReflectionPromptBuilderTest, RevisionPromptContainsCritique) {
    DynamicReflectionPromptBuilder builder;
    auto prompt = builder.buildRevisionPrompt("task", "response", "the critique text");
    EXPECT_NE(prompt.find("the critique text"), std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, ConstitutionalRevisionMentionsHarmlessHonest) {
    DynamicReflectionPromptBuilder builder(ReflectionStrategy::CONSTITUTIONAL);
    auto prompt = builder.buildRevisionPrompt("task", "response", "critique");
    EXPECT_NE(prompt.find("harmless"), std::string::npos);
    EXPECT_NE(prompt.find("honest"),   std::string::npos);
}

// ============================================================================
// AC-6: DynamicReflectionPromptBuilder — constitutional critique prompt
// ============================================================================

TEST(DynamicReflectionPromptBuilderTest, ConstitutionalCritiqueListsPrinciples) {
    DynamicReflectionPromptBuilder builder;
    std::vector<std::string> principles = {"Be helpful.", "Be harmless.", "Be honest."};
    auto prompt = builder.buildConstitutionalCritiquePrompt("response text", principles);
    EXPECT_NE(prompt.find("Be helpful."),  std::string::npos);
    EXPECT_NE(prompt.find("Be harmless."), std::string::npos);
    EXPECT_NE(prompt.find("Be honest."),   std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, ConstitutionalCritiqueEmptyPrinciplesDoesNotCrash) {
    DynamicReflectionPromptBuilder builder;
    EXPECT_NO_THROW(builder.buildConstitutionalCritiquePrompt("response", {}));
}

// ============================================================================
// AC-7: DynamicReflectionPromptBuilder — Socratic prompt cycles questions
// ============================================================================

TEST(DynamicReflectionPromptBuilderTest, SocraticPromptContainsClaim) {
    DynamicReflectionPromptBuilder builder;
    auto prompt = builder.buildSocraticPrompt("The sky is blue.", 0);
    EXPECT_NE(prompt.find("The sky is blue."), std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, SocraticPromptVariesWithIteration) {
    DynamicReflectionPromptBuilder builder;
    const std::string p0 = builder.buildSocraticPrompt("claim", 0);
    const std::string p1 = builder.buildSocraticPrompt("claim", 1);
    EXPECT_NE(p0, p1);
}

TEST(DynamicReflectionPromptBuilderTest, SocraticPromptWrapsAround) {
    DynamicReflectionPromptBuilder builder;
    const std::string p0 = builder.buildSocraticPrompt("claim", 0);
    const std::string p5 = builder.buildSocraticPrompt("claim", 5);  // 5 questions → wraps to 0
    EXPECT_EQ(p0, p5);
}

// ============================================================================
// AC-8: DynamicReflectionPromptBuilder — self-aware context header
// ============================================================================

TEST(DynamicReflectionPromptBuilderTest, NeutralContextYieldsEmptyHeader) {
    DynamicReflectionPromptBuilder builder;
    SelfAwareContext ctx;
    ctx.confidence = 0.7;
    const std::string header = builder.buildSelfAwareContextHeader(ctx);
    EXPECT_TRUE(header.empty());
}

TEST(DynamicReflectionPromptBuilderTest, UncertainContextYieldsNonEmptyHeader) {
    DynamicReflectionPromptBuilder builder;
    SelfAwareContext ctx;
    ctx.has_uncertain_claims  = true;
    ctx.uncertainty_markers   = {"i think"};
    ctx.confidence            = 0.2;
    const std::string header  = builder.buildSelfAwareContextHeader(ctx);
    EXPECT_FALSE(header.empty());
    EXPECT_NE(header.find("i think"), std::string::npos);
}

TEST(DynamicReflectionPromptBuilderTest, LowConfidenceContextMentionsLowConfidence) {
    DynamicReflectionPromptBuilder builder;
    SelfAwareContext ctx;
    ctx.confidence           = 0.15;
    ctx.has_uncertain_claims = true;
    ctx.uncertainty_markers  = {"maybe"};
    const std::string header = builder.buildSelfAwareContextHeader(ctx);
    EXPECT_NE(header.find("low"), std::string::npos);
}

// ============================================================================
// AC-9: ReflectionHallucinationGuard — detectHallucinationSignals
// ============================================================================

TEST(ReflectionHallucinationGuardTest, DetectsKnownMarkerInCritique) {
    ReflectionHallucinationGuard guard;
    EXPECT_TRUE(guard.detectHallucinationSignals("response", "i cannot verify this"));
}

TEST(ReflectionHallucinationGuardTest, DetectsMarkerCaseInsensitive) {
    ReflectionHallucinationGuard guard;
    EXPECT_TRUE(guard.detectHallucinationSignals("response", "I CANNOT VERIFY THIS"));
}

TEST(ReflectionHallucinationGuardTest, DetectsSelfCorrectionInResponse) {
    ReflectionHallucinationGuard guard;
    EXPECT_TRUE(guard.detectHallucinationSignals("Actually, I was wrong.", "normal critique"));
}

TEST(ReflectionHallucinationGuardTest, CleanResponseAndCritiqueReturnsFalse) {
    ReflectionHallucinationGuard guard;
    EXPECT_FALSE(guard.detectHallucinationSignals(
        "The GDPR was enacted in 2018.",
        "The response is accurate but could be expanded."));
}

// ============================================================================
// AC-10: ReflectionHallucinationGuard — isDiverging
// ============================================================================

TEST(ReflectionHallucinationGuardTest, NotDivergingWhenTooFewPoints) {
    ReflectionHallucinationGuard guard(0.1, 2);
    EXPECT_FALSE(guard.isDiverging({0.8}));
    EXPECT_FALSE(guard.isDiverging({0.8, 0.7}));
}

TEST(ReflectionHallucinationGuardTest, DetectsDivergenceWhenDropExceedsThreshold) {
    // earlier = 0.9; recent window [0.6, 0.6] → avg = 0.6; drop = 0.3 > 0.2
    ReflectionHallucinationGuard guard(0.2, 2);
    EXPECT_TRUE(guard.isDiverging({0.9, 0.6, 0.6}));
}

TEST(ReflectionHallucinationGuardTest, NoDivergenceWhenDropBelowThreshold) {
    // earlier = 0.8; recent [0.75, 0.72] → avg = 0.735; drop = 0.065 < 0.2
    ReflectionHallucinationGuard guard(0.2, 2);
    EXPECT_FALSE(guard.isDiverging({0.8, 0.75, 0.72}));
}

TEST(ReflectionHallucinationGuardTest, DivergenceThresholdAccessor) {
    ReflectionHallucinationGuard guard(0.42, 3);
    EXPECT_NEAR(guard.getDivergenceThreshold(), 0.42, 1e-9);
    EXPECT_EQ(guard.getWindow(), 3u);
}

// ============================================================================
// AC-11: ReflectionHallucinationGuard — shouldHalt
// ============================================================================

TEST(ReflectionHallucinationGuardTest, ShouldNotHaltOnEmptySteps) {
    ReflectionHallucinationGuard guard;
    EXPECT_FALSE(guard.shouldHalt({}));
}

TEST(ReflectionHallucinationGuardTest, ShouldHaltWhenLastStepFlagSet) {
    ReflectionHallucinationGuard guard;
    ReflectionStep step;
    step.hallucination_suspected = true;
    step.quality_score           = 0.9;
    EXPECT_TRUE(guard.shouldHalt({step}));
}

// ============================================================================
// AC-12: ReflectionTuner — fallback mode
// ============================================================================

TEST(ReflectionTunerTest, FallbackModeDoesNotCrash) {
    ReflectionTuner tuner;
    EXPECT_NO_THROW(tuner.tune("What is 2+2?", "The answer is 4."));
}

TEST(ReflectionTunerTest, FallbackModeReturnsSameResponseContent) {
    ReflectionTuner tuner;
    auto result = tuner.tune("What is 2+2?", "The answer is 4.");
    // In fallback mode, revisions are not possible without an LLM.
    EXPECT_EQ(result.final_response, "The answer is 4.");
}

TEST(ReflectionTunerTest, FallbackModeHasNoProviderAttached) {
    ReflectionTuner tuner;
    EXPECT_FALSE(tuner.hasReflectionProvider());
}

TEST(ReflectionTunerTest, FallbackModeStepMetadataContainsCritiquePrompt) {
    ReflectionTuner tuner;
    ReflectionConfig cfg;
    cfg.max_iterations = 1;
    tuner.setConfig(cfg);
    auto result = tuner.tune("task", "response");
    ASSERT_FALSE(result.steps.empty());
    EXPECT_TRUE(result.steps[0].metadata.contains("critique_prompt"));
}

TEST(ReflectionTunerTest, FallbackModeStepMetadataContainsRevisionPrompt) {
    ReflectionTuner tuner;
    ReflectionConfig cfg;
    cfg.max_iterations = 1;
    tuner.setConfig(cfg);
    auto result = tuner.tune("task", "response");
    ASSERT_FALSE(result.steps.empty());
    EXPECT_TRUE(result.steps[0].metadata.contains("revision_prompt"));
}

// ============================================================================
// AC-13: quality_trajectory correctness
// ============================================================================

TEST(ReflectionTunerTest, QualityTrajectoryHasInitialPlusOnePerIteration) {
    ReflectionConfig cfg;
    cfg.max_iterations = 2;
    cfg.hallucination_guard_enabled = false;
    cfg.convergence_threshold = 0.99;  // prevent early convergence
    cfg.min_delta_improvement = 0.0;

    ReflectionTuner tuner(cfg);
    auto result = tuner.tune("prompt", "response");
    // 1 initial + up to 2 iterations
    EXPECT_EQ(result.quality_trajectory.size(),
              1 + result.total_iterations);
}

TEST(ReflectionTunerTest, InitialQualityEqualsFirstTrajectoryEntry) {
    ReflectionTuner tuner;
    auto result = tuner.tune("prompt", "response");
    EXPECT_NEAR(result.initial_quality, result.quality_trajectory[0], 1e-9);
}

// ============================================================================
// AC-14: halted_by_hallucination_guard
// ============================================================================

TEST(ReflectionTunerTest, HallucinationGuardHaltsOnBadCritique) {
    ReflectionConfig cfg;
    cfg.max_iterations = 5;
    cfg.hallucination_guard_enabled = true;

    ReflectionTuner tuner(cfg);
    tuner.setReflectionProvider(
        std::make_shared<HallucinatingCritiqueProvider>());

    auto result = tuner.tune("task", "response");
    EXPECT_TRUE(result.halted_by_hallucination_guard);
}

TEST(ReflectionTunerTest, DisabledGuardDoesNotHaltOnBadCritique) {
    ReflectionConfig cfg;
    cfg.max_iterations              = 2;
    cfg.hallucination_guard_enabled = false;
    cfg.convergence_threshold       = 1.1;  // never converge by threshold
    cfg.min_delta_improvement       = 0.0;

    ReflectionTuner tuner(cfg);
    tuner.setReflectionProvider(
        std::make_shared<HallucinatingCritiqueProvider>());

    auto result = tuner.tune("task", "response");
    EXPECT_FALSE(result.halted_by_hallucination_guard);
    EXPECT_EQ(result.total_iterations, 2u);
}

// ============================================================================
// AC-15: converged on quality threshold
// ============================================================================

TEST(ReflectionTunerTest, ConvergedWhenScoreExceedsThreshold) {
    ReflectionConfig cfg;
    cfg.max_iterations        = 5;
    cfg.convergence_threshold = 0.7;
    cfg.hallucination_guard_enabled = false;

    ReflectionTuner tuner(cfg);
    // Score starts at 0.5, +0.3 per step → reaches 0.8 after first revision.
    tuner.setReflectionProvider(
        std::make_shared<ImprovingMockProvider>(0.5, 0.3, 1.0));

    auto result = tuner.tune("task", "response");
    EXPECT_TRUE(result.converged);
    EXPECT_LT(result.total_iterations, 5u);
}

// ============================================================================
// AC-16: converged on plateau
// ============================================================================

TEST(ReflectionTunerTest, ConvergedOnPlateauWhenDeltaBelowMinimum) {
    ReflectionConfig cfg;
    cfg.max_iterations        = 5;
    cfg.convergence_threshold = 1.1;  // never hit
    cfg.min_delta_improvement = 0.05;
    cfg.hallucination_guard_enabled = false;

    ReflectionTuner tuner(cfg);
    // Constant score → delta = 0.0 < 0.05, converges after second step.
    tuner.setReflectionProvider(
        std::make_shared<ConstantMockProvider>("gen", "crit", "rev", 0.6));

    auto result = tuner.tune("task", "initial response");
    EXPECT_TRUE(result.converged);
    EXPECT_LT(result.total_iterations, 5u);
}

// ============================================================================
// AC-17: max_iterations respected
// ============================================================================

TEST(ReflectionTunerTest, MaxIterationsIsUpperBound) {
    ReflectionConfig cfg;
    cfg.max_iterations              = 3;
    cfg.convergence_threshold       = 1.1;  // never converge by score
    cfg.min_delta_improvement       = 0.0;  // never converge by plateau
    cfg.hallucination_guard_enabled = false;

    ReflectionTuner tuner(cfg);
    tuner.setReflectionProvider(
        std::make_shared<ConstantMockProvider>("gen", "crit", "rev", 0.5));

    auto result = tuner.tune("task", "initial");
    EXPECT_LE(result.total_iterations, 3u);
}

// ============================================================================
// AC-18: mock provider — critique / revise / score called
// ============================================================================

TEST(ReflectionTunerTest, MockProviderRevisionReflectedInFinalResponse) {
    ReflectionConfig cfg;
    cfg.max_iterations        = 1;
    cfg.convergence_threshold = 1.1;
    cfg.hallucination_guard_enabled = false;
    cfg.min_delta_improvement = 0.0;

    ReflectionTuner tuner(cfg);
    tuner.setReflectionProvider(
        std::make_shared<ConstantMockProvider>(
            "generated", "the critique", "the revised response", 0.8));

    auto result = tuner.tune("task", "initial");
    EXPECT_EQ(result.final_response, "the revised response");
    ASSERT_EQ(result.steps.size(), 1u);
    EXPECT_EQ(result.steps[0].critique, "the critique");
    EXPECT_NEAR(result.steps[0].quality_score, 0.8, 1e-9);
}

TEST(ReflectionTunerTest, TuneFromPromptUsesProviderGenerate) {
    ReflectionConfig cfg;
    cfg.max_iterations        = 1;
    cfg.convergence_threshold = 1.1;
    cfg.hallucination_guard_enabled = false;
    cfg.min_delta_improvement = 0.0;

    ReflectionTuner tuner(cfg);
    tuner.setReflectionProvider(
        std::make_shared<ConstantMockProvider>(
            "generated-response", "crit", "revised", 0.7));

    // tuneFromPrompt calls generate() first; the "initial response" is "generated-response".
    auto result = tuner.tuneFromPrompt("task prompt");
    // After one iteration, the final response is "revised".
    EXPECT_EQ(result.final_response, "revised");
}

TEST(ReflectionTunerTest, ProviderNameStoredInMetadata) {
    ReflectionTuner tuner;
    tuner.setReflectionProvider(
        std::make_shared<ConstantMockProvider>("g", "c", "r", 0.5));
    auto result = tuner.tune("task", "initial");
    EXPECT_EQ(result.metadata["provider"].get<std::string>(), "constant-mock");
}

TEST(ReflectionTunerTest, FallbackProviderNameInMetadataIsFallback) {
    ReflectionTuner tuner;
    auto result = tuner.tune("task", "initial");
    EXPECT_EQ(result.metadata["provider"].get<std::string>(), "fallback");
}

// ============================================================================
// AC-19: setConfig replaces strategy and guard thresholds
// ============================================================================

TEST(ReflectionTunerTest, SetConfigUpdatesStrategy) {
    ReflectionTuner tuner;
    ReflectionConfig cfg;
    cfg.strategy = ReflectionStrategy::REFLEXION;
    tuner.setConfig(cfg);
    EXPECT_EQ(tuner.getConfig().strategy, ReflectionStrategy::REFLEXION);
    EXPECT_EQ(tuner.getPromptBuilder().getStrategy(), ReflectionStrategy::REFLEXION);
}

TEST(ReflectionTunerTest, SetConfigUpdatesHallucinationGuardThreshold) {
    ReflectionTuner tuner;
    ReflectionConfig cfg;
    cfg.divergence_threshold = 0.42;
    cfg.divergence_window    = 3;
    tuner.setConfig(cfg);
    EXPECT_NEAR(tuner.getHallucinationGuard().getDivergenceThreshold(), 0.42, 1e-9);
    EXPECT_EQ(tuner.getHallucinationGuard().getWindow(), 3u);
}

TEST(ReflectionTunerTest, ClearProviderRemovesProvider) {
    ReflectionTuner tuner;
    tuner.setReflectionProvider(
        std::make_shared<ConstantMockProvider>("g", "c", "r", 0.5));
    EXPECT_TRUE(tuner.hasReflectionProvider());
    tuner.clearReflectionProvider();
    EXPECT_FALSE(tuner.hasReflectionProvider());
}

// ============================================================================
// AC-20: ReflectionResult::toJson
// ============================================================================

TEST(ReflectionResultTest, ToJsonContainsRequiredFields) {
    ReflectionTuner tuner;
    auto result = tuner.tune("task", "response");
    auto j = result.toJson();

    EXPECT_TRUE(j.contains("final_response"));
    EXPECT_TRUE(j.contains("converged"));
    EXPECT_TRUE(j.contains("halted_by_hallucination_guard"));
    EXPECT_TRUE(j.contains("total_iterations"));
    EXPECT_TRUE(j.contains("initial_quality"));
    EXPECT_TRUE(j.contains("final_quality"));
    EXPECT_TRUE(j.contains("quality_improvement"));
    EXPECT_TRUE(j.contains("quality_trajectory"));
    EXPECT_TRUE(j.contains("steps"));
    EXPECT_TRUE(j.contains("self_aware_context"));
}

TEST(ReflectionResultTest, ToJsonFinalResponseMatchesResult) {
    ReflectionTuner tuner;
    auto result = tuner.tune("t", "my initial response");
    auto j = result.toJson();
    EXPECT_EQ(j["final_response"].get<std::string>(), result.final_response);
}

TEST(ReflectionResultTest, ToJsonQualityImprovementConsistent) {
    ReflectionTuner tuner;
    auto result = tuner.tune("t", "r");
    auto j = result.toJson();
    const double improvement =
        j["final_quality"].get<double>() - j["initial_quality"].get<double>();
    EXPECT_NEAR(improvement, j["quality_improvement"].get<double>(), 1e-9);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(ReflectionTunerTest, EmptyPromptAndResponseDoesNotCrash) {
    ReflectionTuner tuner;
    EXPECT_NO_THROW(tuner.tune("", ""));
}

TEST(ReflectionTunerTest, ZeroMaxIterationsProducesNoSteps) {
    ReflectionConfig cfg;
    cfg.max_iterations = 0;
    ReflectionTuner tuner(cfg);
    auto result = tuner.tune("task", "response");
    EXPECT_TRUE(result.steps.empty());
    EXPECT_EQ(result.total_iterations, 0u);
    EXPECT_EQ(result.final_response, "response");
}

TEST(ReflectionTunerTest, TuneFromPromptFallbackUsesPromptAsInitialResponse) {
    ReflectionConfig cfg;
    cfg.max_iterations = 0;
    ReflectionTuner tuner(cfg);
    auto result = tuner.tuneFromPrompt("my prompt text");
    EXPECT_EQ(result.final_response, "my prompt text");
}

TEST(ReflectionTunerTest, DivergingProviderTriggersGuard) {
    ReflectionConfig cfg;
    cfg.max_iterations              = 5;
    cfg.divergence_threshold        = 0.1;
    cfg.divergence_window           = 2;
    cfg.hallucination_guard_enabled = true;
    cfg.convergence_threshold       = 1.1;
    cfg.min_delta_improvement       = 0.0;

    ReflectionTuner tuner(cfg);
    // Score: 0.8, 0.8 (initial), then 0.5, 0.5 per revision → avg drops > 0.1
    tuner.setReflectionProvider(
        std::make_shared<DivertingMockProvider>(0.8, 0.3));

    auto result = tuner.tune("task", "initial");
    EXPECT_TRUE(result.halted_by_hallucination_guard);
}
