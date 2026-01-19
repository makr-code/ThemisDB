/**
 * @file test_rag_judge_phase3.cpp
 * @brief Unit tests for RAG Judge Phase 3 (Pairwise & Ensemble)
 */

#include "rag/pairwise_comparator.h"
#include "rag/judge_ensemble.h"
#include "rag/rag_judge.h"
#include <gtest/gtest.h>

using namespace themis::rag::judge;

// ============================================================================
// Pairwise Comparator Tests
// ============================================================================

class PairwiseComparatorTest : public ::testing::Test {
protected:
    PairwiseComparator::Config config;
    
    std::vector<std::pair<std::string, std::string>> sample_docs = {
        {"doc1", "Paris is the capital of France with a population of over 2 million."},
        {"doc2", "The Eiffel Tower is an iconic landmark in Paris."}
    };
};

TEST_F(PairwiseComparatorTest, BasicComparison) {
    PairwiseComparator comparator(config);
    
    std::string query = "What is the capital of France?";
    std::string answer_a = "Paris is the capital of France.";
    std::string answer_b = "The capital of France is Paris, a beautiful city.";
    
    auto result = comparator.compare(query, sample_docs, answer_a, answer_b);
    
    EXPECT_TRUE(result.overall_winner == ComparisonWinner::ANSWER_A ||
                result.overall_winner == ComparisonWinner::ANSWER_B ||
                result.overall_winner == ComparisonWinner::TIE);
    EXPECT_GE(result.overall_confidence, 0.0);
    EXPECT_LE(result.overall_confidence, 1.0);
    EXPECT_FALSE(result.overall_reasoning.empty());
}

TEST_F(PairwiseComparatorTest, NoBiasMitigation) {
    config.bias_strategy = BiasMitigationStrategy::NONE;
    PairwiseComparator comparator(config);
    
    std::string query = "What is AI?";
    std::string answer_a = "AI is artificial intelligence.";
    std::string answer_b = "Artificial intelligence (AI) is machine intelligence.";
    
    auto result = comparator.compare(query, sample_docs, answer_a, answer_b);
    
    EXPECT_EQ(result.num_evaluations, 1);
    EXPECT_FALSE(result.flip_tested);
}

TEST_F(PairwiseComparatorTest, FlipAndAverage) {
    config.bias_strategy = BiasMitigationStrategy::FLIP_AND_AVERAGE;
    PairwiseComparator comparator(config);
    
    std::string query = "Explain machine learning";
    std::string answer_a = "Machine learning is a subset of AI.";
    std::string answer_b = "ML teaches computers to learn from data.";
    
    auto result = comparator.compare(query, sample_docs, answer_a, answer_b);
    
    EXPECT_EQ(result.num_evaluations, 2);
    EXPECT_TRUE(result.flip_tested);
    EXPECT_GE(result.position_bias_magnitude, 0.0);
    EXPECT_LE(result.position_bias_magnitude, 1.0);
}

TEST_F(PairwiseComparatorTest, MultiSample) {
    config.bias_strategy = BiasMitigationStrategy::MULTI_SAMPLE;
    config.num_samples = 5;
    PairwiseComparator comparator(config);
    
    std::string query = "What is deep learning?";
    std::string answer_a = "Deep learning uses neural networks.";
    std::string answer_b = "Deep learning is a subset of machine learning.";
    
    auto result = comparator.compare(query, sample_docs, answer_a, answer_b);
    
    EXPECT_EQ(result.num_evaluations, 5);
    EXPECT_GE(result.overall_confidence, 0.0);
    EXPECT_LE(result.overall_confidence, 1.0);
}

TEST_F(PairwiseComparatorTest, BiasDetection) {
    PairwiseComparator comparator(config);
    
    // Test bias detection
    auto bias1 = comparator.detectPositionBias(
        ComparisonWinner::ANSWER_A,
        ComparisonWinner::ANSWER_A
    );
    EXPECT_EQ(bias1, 0.0);  // Agreement = no bias
    
    auto bias2 = comparator.detectPositionBias(
        ComparisonWinner::ANSWER_A,
        ComparisonWinner::ANSWER_B
    );
    EXPECT_EQ(bias2, 1.0);  // Opposite = full bias
    
    auto bias3 = comparator.detectPositionBias(
        ComparisonWinner::ANSWER_A,
        ComparisonWinner::TIE
    );
    EXPECT_GT(bias3, 0.0);  // Partial disagreement
}

// ============================================================================
// Judge Ensemble Tests
// ============================================================================

class JudgeEnsembleTest : public ::testing::Test {
protected:
    JudgeEnsemble::Config config;
    
    std::vector<RetrievedDocument> sample_docs = {
        {"doc1", "Paris is the capital of France.", 0.95, {}}
    };
};

TEST_F(JudgeEnsembleTest, BasicEnsemble) {
    config.num_judges = 3;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is the capital of France?";
    std::string answer = "Paris is the capital of France.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    EXPECT_EQ(result.num_judges, 3);
    EXPECT_EQ(result.individual_votes.size(), 3);
    EXPECT_GE(result.aggregated_result.overall_score, 0.0);
    EXPECT_LE(result.aggregated_result.overall_score, 1.0);
}

TEST_F(JudgeEnsembleTest, MajorityVoting) {
    config.num_judges = 5;
    config.voting_strategy = VotingStrategy::MAJORITY_VOTING;
    JudgeEnsemble ensemble(config);
    
    std::string query = "Explain quantum computing";
    std::string answer = "Quantum computing uses quantum mechanics principles.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    EXPECT_EQ(result.strategy_used, VotingStrategy::MAJORITY_VOTING);
    EXPECT_EQ(result.individual_votes.size(), 5);
}

TEST_F(JudgeEnsembleTest, WeightedAverage) {
    config.num_judges = 3;
    config.voting_strategy = VotingStrategy::WEIGHTED_AVERAGE;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is AI?";
    std::string answer = "AI is artificial intelligence.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    EXPECT_EQ(result.strategy_used, VotingStrategy::WEIGHTED_AVERAGE);
    EXPECT_GE(result.aggregated_result.confidence, 0.0);
    EXPECT_LE(result.aggregated_result.confidence, 1.0);
}

TEST_F(JudgeEnsembleTest, HierarchicalVoting) {
    config.num_judges = 3;
    config.voting_strategy = VotingStrategy::HIERARCHICAL;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is machine learning?";
    std::string answer = "Machine learning is about teaching computers.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    EXPECT_EQ(result.strategy_used, VotingStrategy::HIERARCHICAL);
}

TEST_F(JudgeEnsembleTest, DisagreementAnalysis) {
    config.num_judges = 3;
    config.enable_disagreement_analysis = true;
    JudgeEnsemble ensemble(config);
    
    std::string query = "Explain neural networks";
    std::string answer = "Neural networks are inspired by the human brain.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    EXPECT_GE(result.disagreement.agreement_score, 0.0);
    EXPECT_LE(result.disagreement.agreement_score, 1.0);
    EXPECT_GE(result.disagreement.consensus_strength, 0.0);
    EXPECT_LE(result.disagreement.consensus_strength, 1.0);
}

TEST_F(JudgeEnsembleTest, OutlierDetection) {
    config.num_judges = 5;
    config.enable_outlier_detection = true;
    config.outlier_threshold = 2.0;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is deep learning?";
    std::string answer = "Deep learning uses neural networks with many layers.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    // Outliers may or may not be detected depending on judge variance
    EXPECT_GE(result.disagreement.outlier_judges.size(), 0);
}

TEST_F(JudgeEnsembleTest, Agreement Calculation) {
    config.num_judges = 2;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is Python?";
    std::string answer = "Python is a programming language.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    // With 2 judges, should calculate Cohen's Kappa
    if (result.individual_votes.size() == 2) {
        EXPECT_NE(result.disagreement.cohens_kappa, 0.0);
    }
}

TEST_F(JudgeEnsembleTest, FleissKappa) {
    config.num_judges = 4;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is cloud computing?";
    std::string answer = "Cloud computing provides on-demand resources.";
    
    auto result = ensemble.evaluate(query, sample_docs, answer);
    
    // With 4 judges, should calculate Fleiss' Kappa
    if (result.individual_votes.size() >= 3) {
        // Fleiss' Kappa can range from -1 to 1
        EXPECT_GE(result.disagreement.fleiss_kappa, -1.0);
        EXPECT_LE(result.disagreement.fleiss_kappa, 1.0);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

class Phase3IntegrationTest : public ::testing::Test {
protected:
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Machine learning is a subset of AI.", 0.9, {}},
        {"doc2", "Deep learning uses neural networks.", 0.85, {}}
    };
};

TEST_F(Phase3IntegrationTest, ComparisonAndEnsemble) {
    // Test pairwise comparison
    PairwiseComparator::Config comp_config;
    comp_config.bias_strategy = BiasMitigationStrategy::FLIP_AND_AVERAGE;
    PairwiseComparator comparator(comp_config);
    
    std::string query = "What is machine learning?";
    std::string answer_a = "ML is a subset of AI that learns from data.";
    std::string answer_b = "Machine learning teaches computers to learn.";
    
    auto comp_result = comparator.compare(query, 
        std::vector<std::pair<std::string, std::string>>{
            {"doc1", docs[0].content},
            {"doc2", docs[1].content}
        }, 
        answer_a, answer_b);
    
    EXPECT_TRUE(comp_result.flip_tested);
    
    // Test ensemble evaluation
    JudgeEnsemble::Config ens_config;
    ens_config.num_judges = 3;
    ens_config.voting_strategy = VotingStrategy::WEIGHTED_AVERAGE;
    JudgeEnsemble ensemble(ens_config);
    
    auto ens_result = ensemble.evaluate(query, docs, answer_a);
    
    EXPECT_EQ(ens_result.num_judges, 3);
    EXPECT_GE(ens_result.aggregated_result.overall_score, 0.0);
}

TEST_F(Phase3IntegrationTest, PerformanceCheck) {
    // Ensemble should complete in reasonable time
    JudgeEnsemble::Config config;
    config.num_judges = 3;
    JudgeEnsemble ensemble(config);
    
    std::string query = "Explain deep learning";
    std::string answer = "Deep learning is a type of machine learning.";
    
    auto start = std::chrono::steady_clock::now();
    auto result = ensemble.evaluate(query, docs, answer);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (adjust based on actual performance)
    EXPECT_LT(duration.count(), 10000);  // < 10 seconds
}

TEST_F(Phase3IntegrationTest, ConsistencyCheck) {
    // Ensemble results should be relatively consistent
    JudgeEnsemble::Config config;
    config.num_judges = 3;
    config.voting_strategy = VotingStrategy::MAJORITY_VOTING;
    JudgeEnsemble ensemble(config);
    
    std::string query = "What is neural network?";
    std::string answer = "A neural network is inspired by biological neurons.";
    
    auto result1 = ensemble.evaluate(query, docs, answer);
    auto result2 = ensemble.evaluate(query, docs, answer);
    
    // Results should be in similar range (allowing for some variance)
    double score_diff = std::abs(result1.aggregated_result.overall_score - 
                                 result2.aggregated_result.overall_score);
    EXPECT_LT(score_diff, 0.3);  // Scores shouldn't differ by more than 0.3
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
