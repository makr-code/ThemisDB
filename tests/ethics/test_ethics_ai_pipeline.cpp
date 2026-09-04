// Integration tests for ethics_ai end-to-end pipeline:
// PhilosophyLoader + ArgumentStore + RAGContextEngine + EthicalDiscourseEngine + EthicsEvaluator.

#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/ethics_evaluator.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/rag_context_engine.h"

#include <filesystem>
#include <fstream>

using namespace themis::plugins::ethics;

namespace {

std::filesystem::path writeProfile(
    const std::filesystem::path& dir,
    const std::string& file_name,
    const std::string& school_id,
    const std::string& name,
    const std::string& thesis_line) {
    const auto path = dir / file_name;
    std::ofstream out(path);
    out << "school_id: " << school_id << "\n";
    out << "name: \"" << name << "\"\n";
    out << "main_theses:\n";
    out << "  - \"" << thesis_line << "\"\n";
    out << "decision_framework:\n";
    out << "  principle: \"act responsibly\"\n";
    return path;
}

} // namespace

class EthicsAiPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "themis_ethics_ai_pipeline_test";
        std::error_code ec = {};
        std::filesystem::remove_all(tmp_dir_, ec);
        std::filesystem::create_directories(tmp_dir_, ec);
    }

    void TearDown() override {
        std::error_code ec = {};
        std::filesystem::remove_all(tmp_dir_, ec);
    }

    std::filesystem::path tmp_dir_;
};

TEST_F(EthicsAiPipelineTest, FullPipelineWithRagAndEvaluator) {
    auto loader = std::make_shared<PhilosophyLoader>();

    const auto kant_file = writeProfile(
        tmp_dir_,
        "kant.yaml",
        "kant",
        "Kantian Ethics",
        "Treat every person as an end in themselves.");
    const auto util_file = writeProfile(
        tmp_dir_,
        "utilitarianism.yaml",
        "utilitarianism",
        "Utilitarianism",
        "Choose the action that maximizes well-being.");

    auto s1 = loader->loadFromFile(kant_file.string());
    if (!s1.isOK() && s1.message.find("YAML support not enabled") != std::string::npos) {
        GTEST_SKIP() << "yaml-cpp unavailable in this build; skipping ethics_ai pipeline integration test";
    }
    ASSERT_TRUE(s1.isOK()) << s1.message;
    auto s2 = loader->loadFromFile(util_file.string());
    ASSERT_TRUE(s2.isOK()) << s2.message;

    auto store = std::make_shared<ArgumentStore>();
    auto init_status = store->initialize(nullptr, nullptr); // standalone mode
    ASSERT_TRUE(init_status.isOK()) << init_status.message;

    auto rag_engine = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine discourse(loader, store, rag_engine);

    auto debate = discourse.initializeDebate(
        "Should autonomous emergency systems prioritize the many over the few?",
        {"kant", "utilitarianism"},
        "autonomous_systems");
    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(debate));

    auto decision_result = discourse.makeDecision(
        "Should autonomous emergency systems prioritize the many over the few?",
        {"kant", "utilitarianism"},
        "autonomous_systems",
        true);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));

    const auto& decision = std::get<EthicalDecision>(decision_result);
    EXPECT_FALSE(decision.decision_id.empty());
    EXPECT_FALSE(decision.decision_text.empty());
    EXPECT_EQ(decision.primary_philosophy, "kant");
    EXPECT_EQ(decision.supporting_philosophies.size(), 2u);
    EXPECT_NEAR(decision.confidence, 0.75, 1e-9);
    EXPECT_NEAR(decision.consensus_level, 0.70, 1e-9);

    auto kant_args_result = store->getArgumentsByPhilosophy("kant", {}, 10);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(kant_args_result));
    auto util_args_result = store->getArgumentsByPhilosophy("utilitarianism", {}, 10);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(util_args_result));

    const auto& kant_args = std::get<std::vector<EthicalArgument>>(kant_args_result);
    const auto& util_args = std::get<std::vector<EthicalArgument>>(util_args_result);
    ASSERT_FALSE(kant_args.empty());
    ASSERT_FALSE(util_args.empty());

    std::vector<EthicalArgument> all_args;
    all_args.insert(all_args.end(), kant_args.begin(), kant_args.end());
    all_args.insert(all_args.end(), util_args.begin(), util_args.end());

    EthicsEvaluator evaluator;
    auto eval_result = evaluator.evaluateDecision(decision, all_args);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(eval_result));

    const auto& eval = std::get<EthicsEvaluationResult>(eval_result);
    EXPECT_GE(eval.overall_score, 0.0);
    EXPECT_LE(eval.overall_score, 1.0);

    auto stored_decision_result = store->getDecision(decision.decision_id);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(stored_decision_result));
    const auto& stored_decision = std::get<EthicalDecision>(stored_decision_result);
    EXPECT_EQ(stored_decision.decision_id, decision.decision_id);
    EXPECT_EQ(stored_decision.primary_philosophy, decision.primary_philosophy);
}

TEST_F(EthicsAiPipelineTest, InitializeDebateRejectsUnknownSchool) {
    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
    auto rag_engine = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine discourse(loader, store, rag_engine);

    auto result = discourse.initializeDebate(
        "Test dilemma",
        {"unknown_school"},
        "general");

    ASSERT_TRUE(std::holds_alternative<Status>(result));
    const auto& status = std::get<Status>(result);
    EXPECT_FALSE(status.isOK());
}

TEST_F(EthicsAiPipelineTest, MakeDecisionRejectsEmptySchoolList) {
    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
    auto rag_engine = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine discourse(loader, store, rag_engine);

    auto result = discourse.makeDecision("Test dilemma", {}, "general", false);

    ASSERT_TRUE(std::holds_alternative<Status>(result));
    const auto& status = std::get<Status>(result);
    EXPECT_FALSE(status.isOK());
}

// ---------------------------------------------------------------------------
// EthicsEvaluator::computeConfidence — minimum coverage
// (Exercises static strengthToScore; UNUSED_FUNCTIONS_REPORT KEEP Target v1.5.0)
// ---------------------------------------------------------------------------

// CC-01: Empty argument list returns default confidence (0.5).
TEST(EthicsEvaluatorComputeConfidenceTest, CC01_EmptyArgumentsReturnsDefault) {
    std::vector<EthicalArgument> args;
    double conf = EthicsEvaluator::computeConfidence(args);
    EXPECT_DOUBLE_EQ(conf, 0.5);
}

// CC-02: All WEAK arguments produce confidence < 0.5 (score = 0.25 each).
TEST(EthicsEvaluatorComputeConfidenceTest, CC02_AllWeakProducesLowConfidence) {
    std::vector<EthicalArgument> args(3);
    for (auto& a : args) {
      a.strength = ArgumentStrength::WEAK;
    }
    double conf = EthicsEvaluator::computeConfidence(args);
    EXPECT_NEAR(conf, 0.25, 1e-9);
}

// CC-03: All STRONG arguments produce confidence of 0.75.
TEST(EthicsEvaluatorComputeConfidenceTest, CC03_AllStrongProducesHighConfidence) {
    std::vector<EthicalArgument> args(4);
    for (auto& a : args) {
      a.strength = ArgumentStrength::STRONG;
    }
    double conf = EthicsEvaluator::computeConfidence(args);
    EXPECT_NEAR(conf, 0.75, 1e-9);
}

// CC-04: All DECISIVE arguments produce maximum confidence of 1.0.
TEST(EthicsEvaluatorComputeConfidenceTest, CC04_AllDecisiveProducesMaxConfidence) {
    std::vector<EthicalArgument> args(2);
    for (auto& a : args) {
      a.strength = ArgumentStrength::DECISIVE;
    }
    double conf = EthicsEvaluator::computeConfidence(args);
    EXPECT_NEAR(conf, 1.0, 1e-9);
}

// CC-05: Mixed strengths average correctly.
TEST(EthicsEvaluatorComputeConfidenceTest, CC05_MixedStrengthsAverage) {
    // WEAK(0.25) + DECISIVE(1.0) → average = 0.625
    std::vector<EthicalArgument> args(2);
    args[0].strength = ArgumentStrength::WEAK;
    args[1].strength = ArgumentStrength::DECISIVE;
    double conf = EthicsEvaluator::computeConfidence(args);
    EXPECT_NEAR(conf, 0.625, 1e-9);
}
