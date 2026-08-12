#include <gtest/gtest.h>
#include "ethics_ai/ethics_evaluator.h"

using namespace themis::plugins::ethics;

class EthicsEvaluatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        evaluator_ = std::make_unique<EthicsEvaluator>();
    }
    
    EthicalDecision createTestDecision() {
        EthicalDecision decision;
        decision.decision_id = "dec_001";
        decision.dilemma_id = "dilemma_001";
        decision.decision_text = "Test decision with detailed reasoning";
        decision.primary_philosophy = "kant";
        decision.supporting_philosophies = {"kant", "utilitarianism", "virtue_ethics"};
        decision.argument_chain_ids = {"chain_1", "chain_2"};
        decision.confidence = 0.85;
        decision.consensus_level = 0.75;
        return decision;
    }
    
    std::vector<EthicalArgument> createTestArguments() {
        std::vector<EthicalArgument> args;
        
        EthicalArgument arg1;
        arg1.id = "arg_1";
        arg1.philosophy_school = "kant";
        arg1.argument_type = ArgumentType::PRO;
        arg1.strength = ArgumentStrength::STRONG;
        arg1.content = "Strong Kantian argument";
        args.push_back(arg1);
        
        EthicalArgument arg2;
        arg2.id = "arg_2";
        arg2.philosophy_school = "kant";
        arg2.argument_type = ArgumentType::CONTRA;
        arg2.strength = ArgumentStrength::MODERATE;
        arg2.content = "Moderate counter-argument";
        args.push_back(arg2);
        
        EthicalArgument arg3;
        arg3.id = "arg_3";
        arg3.philosophy_school = "utilitarianism";
        arg3.argument_type = ArgumentType::PRO;
        arg3.strength = ArgumentStrength::STRONG;
        arg3.content = "Strong utilitarian argument";
        args.push_back(arg3);
        
        return args;
    }
    
    std::unique_ptr<EthicsEvaluator> evaluator_;
};

TEST_F(EthicsEvaluatorTest, EvaluateBasicDecision) {
    auto decision = createTestDecision();
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Check that all scores are in valid range [0, 1]
    EXPECT_GE(eval.overall_score, 0.0);
    EXPECT_LE(eval.overall_score, 1.0);
    EXPECT_GE(eval.decision_quality_score, 0.0);
    EXPECT_LE(eval.decision_quality_score, 1.0);
    EXPECT_GE(eval.consistency_score, 0.0);
    EXPECT_LE(eval.consistency_score, 1.0);
    EXPECT_GE(eval.fairness_score, 0.0);
    EXPECT_LE(eval.fairness_score, 1.0);
    EXPECT_GE(eval.alignment_score, 0.0);
    EXPECT_LE(eval.alignment_score, 1.0);
    EXPECT_GE(eval.transparency_score, 0.0);
    EXPECT_LE(eval.transparency_score, 1.0);
    
    // Overall score should be positive for a well-formed decision
    EXPECT_GT(eval.overall_score, 0.0);
}

TEST_F(EthicsEvaluatorTest, HighConfidenceDecision) {
    auto decision = createTestDecision();
    decision.confidence = 0.95;
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // High confidence should contribute to higher decision quality
    EXPECT_GT(eval.decision_quality_score, 0.7);
}

TEST_F(EthicsEvaluatorTest, LowConfidenceDecision) {
    auto decision = createTestDecision();
    decision.confidence = 0.3;
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Low confidence should result in lower decision quality
    EXPECT_LT(eval.decision_quality_score, 0.8);
}

TEST_F(EthicsEvaluatorTest, MultiPhilosophyDecision) {
    auto decision = createTestDecision();
    decision.supporting_philosophies = {"kant", "utilitarianism", "virtue_ethics", "care_ethics"};
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Multi-philosophy consideration should improve fairness score
    EXPECT_GT(eval.fairness_score, 0.6);
}

TEST_F(EthicsEvaluatorTest, SinglePhilosophyDecision) {
    auto decision = createTestDecision();
    decision.supporting_philosophies = {"kant"};
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Single philosophy might have lower fairness
    EXPECT_LT(eval.fairness_score, 0.9);
}

TEST_F(EthicsEvaluatorTest, DecisionWithArgumentChains) {
    auto decision = createTestDecision();
    decision.argument_chain_ids = {"chain_1", "chain_2", "chain_3"};
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Having argument chains should improve alignment
    EXPECT_GT(eval.alignment_score, 0.5);
}

TEST_F(EthicsEvaluatorTest, DecisionWithoutArgumentChains) {
    auto decision = createTestDecision();
    decision.argument_chain_ids.clear();
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Without argument chains, alignment might be lower
    EXPECT_LT(eval.alignment_score, 0.9);
}

TEST_F(EthicsEvaluatorTest, DetailedMetricsIncluded) {
    auto decision = createTestDecision();
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Check that detailed metrics are populated
    EXPECT_FALSE(eval.detailed_metrics.empty());
    EXPECT_TRUE(eval.detailed_metrics.count("decision_quality") > 0);
    EXPECT_TRUE(eval.detailed_metrics.count("consistency") > 0);
    EXPECT_TRUE(eval.detailed_metrics.count("fairness") > 0);
    EXPECT_TRUE(eval.detailed_metrics.count("alignment") > 0);
    EXPECT_TRUE(eval.detailed_metrics.count("transparency") > 0);
    EXPECT_TRUE(eval.detailed_metrics.count("confidence") > 0);
    EXPECT_TRUE(eval.detailed_metrics.count("consensus_level") > 0);
}

TEST_F(EthicsEvaluatorTest, EmptyDecisionText) {
    auto decision = createTestDecision();
    decision.decision_text = "";
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Empty decision text should lower transparency
    EXPECT_LT(eval.transparency_score, 0.8);
}

TEST_F(EthicsEvaluatorTest, CompleteDocumentation) {
    auto decision = createTestDecision();
    decision.decision_text = "Detailed decision with comprehensive reasoning";
    decision.primary_philosophy = "kant";
    decision.supporting_philosophies = {"kant", "utilitarianism"};
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Complete documentation should improve transparency
    EXPECT_GT(eval.transparency_score, 0.6);
}

TEST_F(EthicsEvaluatorTest, ConsistentPhilosophyAlignment) {
    auto decision = createTestDecision();
    decision.primary_philosophy = "kant";
    
    auto arguments = createTestArguments();
    // Make all arguments Kantian
    for (auto& arg : arguments) {
        arg.philosophy_school = "kant";
    }
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Consistent philosophy alignment should improve consistency score
    EXPECT_GT(eval.consistency_score, 0.7);
}

TEST_F(EthicsEvaluatorTest, NoArguments) {
    auto decision = createTestDecision();
    std::vector<EthicalArgument> empty_args;
    
    auto result = evaluator_->evaluateDecision(decision, empty_args);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Should still produce valid scores even without arguments
    EXPECT_GE(eval.overall_score, 0.0);
    EXPECT_LE(eval.overall_score, 1.0);
}

TEST_F(EthicsEvaluatorTest, OverallScoreIsWeightedAverage) {
    auto decision = createTestDecision();
    auto arguments = createTestArguments();
    
    auto result = evaluator_->evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    
    auto eval = std::get<EthicsEvaluationResult>(result);
    
    // Overall score should be approximately the weighted average
    // of the 5 dimensions (weights: 0.25, 0.20, 0.20, 0.20, 0.15)
    double expected = eval.decision_quality_score * 0.25 +
                     eval.consistency_score * 0.20 +
                     eval.fairness_score * 0.20 +
                     eval.alignment_score * 0.20 +
                     eval.transparency_score * 0.15;
    
    EXPECT_NEAR(eval.overall_score, expected, 0.001);
}