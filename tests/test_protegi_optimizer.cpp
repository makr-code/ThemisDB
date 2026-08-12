#include <gtest/gtest.h>
#include "prompt_engineering/protegi_optimizer.h"

using namespace themis::prompt_engineering;

static const std::string kInitialPrompt = "Classify the sentiment of the text.";

static std::vector<TestCase> makeCases(size_t n = 10) {
    std::vector<TestCase> cases;
    for (size_t i = 0; i < n; ++i) {
        cases.push_back({"input " + std::to_string(i), "expected " + std::to_string(i), {}});
    }
    return cases;
}

static EvaluationFunction makeConstEval(double score) {
    return [score](const std::string&, const std::vector<TestCase>&) { return score; };
}

// ============================================================================
// HeuristicProTeGiProvider
// ============================================================================

TEST(HeuristicProTeGiProviderTest, ComputeGradientNonEmpty) {
    HeuristicProTeGiProvider provider;
    auto grad = provider.computeGradient(kInitialPrompt, {"err1", "err2", ""});
    EXPECT_FALSE(grad.critique.empty());
    EXPECT_EQ(grad.errors.size(), 3u);
}

TEST(HeuristicProTeGiProviderTest, ErrorRateCalculation) {
    HeuristicProTeGiProvider provider;
    auto grad = provider.computeGradient(kInitialPrompt, {"e1", "", "e3", ""});
    EXPECT_DOUBLE_EQ(grad.error_rate, 0.5);
}

TEST(HeuristicProTeGiProviderTest, ZeroErrorsRateIsZero) {
    HeuristicProTeGiProvider provider;
    auto grad = provider.computeGradient(kInitialPrompt, {"", "", ""});
    EXPECT_DOUBLE_EQ(grad.error_rate, 0.0);
}

TEST(HeuristicProTeGiProviderTest, GeneratesKCandidates) {
    HeuristicProTeGiProvider provider;
    ProTeGiGradient grad;
    grad.critique  = "Improve clarity.";
    grad.error_rate = 0.3;
    auto candidates = provider.generateCandidates(kInitialPrompt, grad, 4);
    EXPECT_EQ(candidates.size(), 4u);
}

TEST(HeuristicProTeGiProviderTest, CandidatesAreNonEmpty) {
    HeuristicProTeGiProvider provider;
    ProTeGiGradient grad;
    grad.critique  = "Add examples.";
    grad.error_rate = 0.6;
    auto candidates = provider.generateCandidates(kInitialPrompt, grad, 3);
    for (const auto& c : candidates) {
        EXPECT_FALSE(c.empty());
    }
}

// ============================================================================
// ProTeGiOptimizer – configuration
// ============================================================================

TEST(ProTeGiOptimizerTest, DefaultConfig) {
    ProTeGiOptimizer opt;
    EXPECT_EQ(opt.getConfig().max_steps,      5u);
    EXPECT_EQ(opt.getConfig().beam_width,     4u);
    EXPECT_EQ(opt.getConfig().num_candidates, 4u);
}

TEST(ProTeGiOptimizerTest, SetConfigUpdates) {
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps  = 3;
    cfg.beam_width = 2;
    opt.setConfig(cfg);
    EXPECT_EQ(opt.getConfig().max_steps,  3u);
    EXPECT_EQ(opt.getConfig().beam_width, 2u);
}

// ============================================================================
// ProTeGiOptimizer – optimize()
// ============================================================================

TEST(ProTeGiOptimizerTest, ReturnsResultWithNoProvider) {
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps  = 2;
    cfg.beam_width = 2;
    opt.setConfig(cfg);

    auto result = opt.optimize(kInitialPrompt, makeCases(), makeConstEval(0.7));
    EXPECT_FALSE(result.best_prompt.empty());
    EXPECT_GE(result.best_score, 0.0);
}

TEST(ProTeGiOptimizerTest, EmptyTestCasesReturnsEmpty) {
    ProTeGiOptimizer opt;
    auto result = opt.optimize(kInitialPrompt, {}, makeConstEval(0.5));
    EXPECT_TRUE(result.best_prompt.empty());
}

TEST(ProTeGiOptimizerTest, NullEvalFnReturnsEmpty) {
    ProTeGiOptimizer opt;
    auto result = opt.optimize(kInitialPrompt, makeCases(), nullptr);
    EXPECT_TRUE(result.best_prompt.empty());
}

TEST(ProTeGiOptimizerTest, ConvergesWhenTargetReached) {
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps    = 5;
    cfg.target_score = 0.5;
    opt.setConfig(cfg);

    auto result = opt.optimize(kInitialPrompt, makeCases(), makeConstEval(0.9));
    EXPECT_TRUE(result.converged);
}

TEST(ProTeGiOptimizerTest, ScoreHistoryHasEntries) {
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps  = 2;
    cfg.beam_width = 2;
    opt.setConfig(cfg);

    auto result = opt.optimize(kInitialPrompt, makeCases(), makeConstEval(0.6));
    EXPECT_FALSE(result.score_history.empty());
}

TEST(ProTeGiOptimizerTest, BeamIsNonEmpty) {
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps  = 1;
    cfg.beam_width = 2;
    opt.setConfig(cfg);

    auto result = opt.optimize(kInitialPrompt, makeCases(), makeConstEval(0.5));
    EXPECT_FALSE(result.beam.empty());
}

TEST(ProTeGiOptimizerTest, BestPromptEqualsBeamFront) {
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps  = 1;
    cfg.beam_width = 2;
    opt.setConfig(cfg);

    auto result = opt.optimize(kInitialPrompt, makeCases(), makeConstEval(0.5));
    EXPECT_EQ(result.best_prompt, result.beam.front());
}

TEST(ProTeGiOptimizerTest, CustomLLMProviderIsUsed) {
    class CountingProvider : public IProTeGiLLMProvider {
    public:
        int calls = 0;
        ProTeGiGradient computeGradient(const std::string&,
                                        const std::vector<std::string>&) override {
            ++calls;
            ProTeGiGradient g;
            g.critique   = "test";
            g.error_rate = 0.1;
            return g;
        }
        std::vector<std::string> generateCandidates(const std::string& p,
                                                    const ProTeGiGradient&,
                                                    size_t k) override {
            return std::vector<std::string>(k, p + " [improved]");
        }
    };

    auto prov = std::make_shared<CountingProvider>();
    ProTeGiOptimizer opt;
    ProTeGiConfig cfg;
    cfg.max_steps  = 2;
    cfg.beam_width = 2;
    opt.setConfig(cfg);
    opt.setLLMProvider(prov);

    opt.optimize(kInitialPrompt, makeCases(), makeConstEval(0.5));
    EXPECT_GT(prov->calls, 0);
}

// ============================================================================
// Static prompt builders
// ============================================================================

TEST(ProTeGiOptimizerTest, BuildGradientPromptContainsPrompt) {
    auto p = ProTeGiOptimizer::buildGradientPrompt(kInitialPrompt, {"err1"});
    EXPECT_NE(p.find(kInitialPrompt), std::string::npos);
    EXPECT_NE(p.find("err1"), std::string::npos);
}

TEST(ProTeGiOptimizerTest, BuildCandidatePromptContainsCritique) {
    ProTeGiGradient grad;
    grad.critique = "Be more specific.";
    auto p = ProTeGiOptimizer::buildCandidatePrompt(kInitialPrompt, grad, 3);
    EXPECT_NE(p.find("Be more specific."), std::string::npos);
    EXPECT_NE(p.find("3"), std::string::npos);
}
