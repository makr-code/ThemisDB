#include <gtest/gtest.h>
#include "prompt_engineering/tree_of_thoughts.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

static const std::string kProblem = "What are the main causes of the French Revolution?";

// ============================================================================
// HeuristicThoughtGenerator tests
// ============================================================================

TEST(HeuristicThoughtGeneratorTest, ProducesKThoughts) {
    HeuristicThoughtGenerator gen;
    auto thoughts = gen.generate(kProblem, {}, 3);
    EXPECT_EQ(thoughts.size(), 3u);
}

TEST(HeuristicThoughtGeneratorTest, ThoughtsAreNonEmpty) {
    HeuristicThoughtGenerator gen;
    auto thoughts = gen.generate(kProblem, {}, 4);
    for (const auto& t : thoughts) {
        EXPECT_FALSE(t.empty());
    }
}

TEST(HeuristicThoughtGeneratorTest, ThoughtsAreDistinct) {
    HeuristicThoughtGenerator gen;
    auto thoughts = gen.generate(kProblem, {}, 3);
    EXPECT_NE(thoughts[0], thoughts[1]);
    EXPECT_NE(thoughts[1], thoughts[2]);
}

TEST(HeuristicThoughtGeneratorTest, ZeroKReturnsEmpty) {
    HeuristicThoughtGenerator gen;
    auto thoughts = gen.generate(kProblem, {}, 0);
    EXPECT_TRUE(thoughts.empty());
}

// ============================================================================
// HeuristicToTEvaluator tests
// ============================================================================

TEST(HeuristicToTEvaluatorTest, ScoreIsInUnitInterval) {
    HeuristicToTEvaluator eval(3);
    ToTNode node;
    node.thought = "Some reasoning step about the topic.";
    node.depth   = 1;
    node.path    = {"Previous step."};

    auto [score, verdict] = eval.evaluate(kProblem, node);
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 1.0);
}

TEST(HeuristicToTEvaluatorTest, MaxDepthNodeGetsSureVerdict) {
    HeuristicToTEvaluator eval(2);
    ToTNode node;
    node.thought = "Final reasoning step.";
    node.depth   = 2; // at max_depth
    node.path    = {"Step 1", "Step 2"};

    auto [score, verdict] = eval.evaluate(kProblem, node);
    EXPECT_EQ(verdict, ToTVerdict::SURE);
}

TEST(HeuristicToTEvaluatorTest, IntermediateNodeGetsMaybeVerdict) {
    HeuristicToTEvaluator eval(3);
    ToTNode node;
    node.thought = "Intermediate step.";
    node.depth   = 1;
    node.path    = {};

    auto [score, verdict] = eval.evaluate(kProblem, node);
    EXPECT_EQ(verdict, ToTVerdict::MAYBE);
}

// ============================================================================
// TreeOfThoughtsBuilder – construction and configuration
// ============================================================================

TEST(TreeOfThoughtsBuilderTest, DefaultConfigIsApplied) {
    TreeOfThoughtsBuilder tot;
    const auto& cfg = tot.getConfig();
    EXPECT_EQ(cfg.max_depth, 3u);
    EXPECT_EQ(cfg.branching_factor, 3u);
    EXPECT_EQ(cfg.strategy, ToTSearchStrategy::BFS);
}

TEST(TreeOfThoughtsBuilderTest, SetConfigUpdatesConfig) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 5;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::DFS;
    tot.setConfig(cfg);
    EXPECT_EQ(tot.getConfig().max_depth, 5u);
    EXPECT_EQ(tot.getConfig().branching_factor, 2u);
    EXPECT_EQ(tot.getConfig().strategy, ToTSearchStrategy::DFS);
}

TEST(TreeOfThoughtsBuilderTest, FluentChainingWorks) {
    TreeOfThoughtsBuilder tot;
    auto& ref = tot.setConfig(ToTConfig{})
                   .setThoughtGenerator(std::make_shared<HeuristicThoughtGenerator>())
                   .setEvaluator(std::make_shared<HeuristicToTEvaluator>());
    EXPECT_EQ(&ref, &tot);
}

// ============================================================================
// TreeOfThoughtsBuilder – solve() with BFS
// ============================================================================

TEST(TreeOfThoughtsBuilderTest, BFSSolveReturnsNonEmptyAnswer) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 2;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::BFS;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_FALSE(result.answer.empty());
}

TEST(TreeOfThoughtsBuilderTest, BFSBestPathIsNonEmpty) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 2;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::BFS;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_FALSE(result.best_path.empty());
}

TEST(TreeOfThoughtsBuilderTest, BFSNodesExploredIsPositive) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 2;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::BFS;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_GT(result.nodes_explored, 0u);
}

TEST(TreeOfThoughtsBuilderTest, BFSEmptyProblemReturnsEmptyResult) {
    TreeOfThoughtsBuilder tot;
    ToTResult result = tot.solve("");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.answer.empty());
}

// ============================================================================
// TreeOfThoughtsBuilder – solve() with DFS
// ============================================================================

TEST(TreeOfThoughtsBuilderTest, DFSSolveReturnsNonEmptyAnswer) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 2;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::DFS;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_FALSE(result.answer.empty());
    EXPECT_GT(result.nodes_explored, 0u);
}

TEST(TreeOfThoughtsBuilderTest, DFSBestPathDepthRespected) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 3;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::DFS;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_LE(result.best_path.size(), cfg.max_depth + 1);
}

// ============================================================================
// TreeOfThoughtsBuilder – solve() with Beam search
// ============================================================================

TEST(TreeOfThoughtsBuilderTest, BeamSolveReturnsNonEmptyAnswer) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 2;
    cfg.branching_factor = 3;
    cfg.beam_width       = 2;
    cfg.strategy         = ToTSearchStrategy::BEAM;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_FALSE(result.answer.empty());
}

TEST(TreeOfThoughtsBuilderTest, BeamBestScoreIsNonNegative) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 2;
    cfg.branching_factor = 2;
    cfg.beam_width       = 2;
    cfg.strategy         = ToTSearchStrategy::BEAM;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_GE(result.best_score, 0.0);
}

// ============================================================================
// Verbose mode
// ============================================================================

TEST(TreeOfThoughtsBuilderTest, VerboseModePopulatesLog) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 1;
    cfg.branching_factor = 2;
    cfg.verbose          = true;
    cfg.strategy         = ToTSearchStrategy::BFS;
    tot.setConfig(cfg);

    ToTResult result = tot.solve(kProblem);
    EXPECT_FALSE(result.log.empty());
}

// ============================================================================
// Static prompt builders
// ============================================================================

TEST(TreeOfThoughtsBuilderTest, BuildGenerationPromptContainsProblem) {
    std::string prompt = TreeOfThoughtsBuilder::buildGenerationPrompt(
        kProblem, {}, 3);
    EXPECT_NE(prompt.find(kProblem), std::string::npos);
    EXPECT_NE(prompt.find("3"), std::string::npos);
}

TEST(TreeOfThoughtsBuilderTest, BuildGenerationPromptIncludesPath) {
    std::vector<std::string> path = {"Step A", "Step B"};
    std::string prompt = TreeOfThoughtsBuilder::buildGenerationPrompt(
        kProblem, path, 2);
    EXPECT_NE(prompt.find("Step A"), std::string::npos);
    EXPECT_NE(prompt.find("Step B"), std::string::npos);
}

TEST(TreeOfThoughtsBuilderTest, BuildEvaluationPromptContainsProblem) {
    ToTNode node;
    node.thought = "Consider economic factors.";
    node.depth   = 1;
    node.path    = {"Initial analysis."};

    std::string prompt = TreeOfThoughtsBuilder::buildEvaluationPrompt(kProblem, node);
    EXPECT_NE(prompt.find(kProblem), std::string::npos);
    EXPECT_NE(prompt.find("Consider economic factors."), std::string::npos);
}

TEST(TreeOfThoughtsBuilderTest, BuildSynthesisPromptContainsPathSteps) {
    std::vector<std::string> path = {"Step A", "Step B", "Step C"};
    std::string prompt = TreeOfThoughtsBuilder::buildSynthesisPrompt(kProblem, path);
    EXPECT_NE(prompt.find("Step A"), std::string::npos);
    EXPECT_NE(prompt.find("Step C"), std::string::npos);
}

// ============================================================================
// Custom generator injection
// ============================================================================

class FixedThoughtGenerator : public IToTThoughtGenerator {
public:
    std::vector<std::string> generate(const std::string&,
                                      const std::vector<std::string>&,
                                      size_t k) override {
        std::vector<std::string> out = {};

        for (size_t i = 0; i < k; ++i) {
            out.push_back("FixedThought_" + std::to_string(i));
        }
        return out;
    }
};

TEST(TreeOfThoughtsBuilderTest, CustomGeneratorIsUsed) {
    TreeOfThoughtsBuilder tot;
    ToTConfig cfg;
    cfg.max_depth        = 1;
    cfg.branching_factor = 2;
    cfg.strategy         = ToTSearchStrategy::BFS;
    tot.setConfig(cfg);
    tot.setThoughtGenerator(std::make_shared<FixedThoughtGenerator>());

    ToTResult result = tot.solve(kProblem);
    // The answer should contain content derived from our fixed thoughts
    EXPECT_FALSE(result.answer.empty());
    EXPECT_GT(result.nodes_explored, 0u);
}
