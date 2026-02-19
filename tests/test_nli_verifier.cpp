/**
 * @file test_nli_verifier.cpp
 * @brief Unit tests for NLI Faithfulness Verifier
 */

#include "rag/nli_faithfulness_verifier.h"
#include <gtest/gtest.h>

using namespace themis::rag::judge;

// ============================================================================
// NLI Verifier Tests
// ============================================================================

class NLIVerifierTest : public ::testing::Test {
protected:
    NLIFaithfulnessVerifier verifier;
};

TEST_F(NLIVerifierTest, BasicConstruction) {
    EXPECT_TRUE(verifier.isReady());  // Should be ready with heuristic fallback
}

TEST_F(NLIVerifierTest, SimpleEntailment) {
    std::string premise = "Paris is the capital of France. It has over 2 million residents.";
    std::string hypothesis = "Paris is the capital of France.";
    
    auto result = verifier.verify(premise, hypothesis);
    
    EXPECT_EQ(result.prediction, NLIPrediction::ENTAILMENT);
    EXPECT_GT(result.entailment_score, 0.5);
    EXPECT_GT(result.confidence, 0.0);
}

TEST_F(NLIVerifierTest, SimpleContradiction) {
    std::string premise = "Paris is the capital of France.";
    std::string hypothesis = "Paris is not the capital of France.";
    
    auto result = verifier.verify(premise, hypothesis);
    
    // Should detect contradiction due to negation mismatch
    EXPECT_EQ(result.prediction, NLIPrediction::CONTRADICTION);
    EXPECT_GT(result.contradiction_score, 0.4);
}

TEST_F(NLIVerifierTest, NeutralRelationship) {
    std::string premise = "Paris is the capital of France.";
    std::string hypothesis = "London is the capital of England.";
    
    auto result = verifier.verify(premise, hypothesis);
    
    // Unrelated claims should be neutral
    EXPECT_TRUE(
        result.prediction == NLIPrediction::NEUTRAL ||
        result.prediction == NLIPrediction::ENTAILMENT
    );
    EXPECT_GT(result.neutral_score, 0.0);
}

TEST_F(NLIVerifierTest, PartialOverlap) {
    std::string premise = "The Eiffel Tower is located in Paris, France.";
    std::string hypothesis = "The Eiffel Tower is in Paris.";
    
    auto result = verifier.verify(premise, hypothesis);
    
    // Should recognize partial entailment
    EXPECT_EQ(result.prediction, NLIPrediction::ENTAILMENT);
    EXPECT_GT(result.entailment_score, 0.4);
}

TEST_F(NLIVerifierTest, EmptyInputs) {
    std::string premise = "";
    std::string hypothesis = "";
    
    auto result = verifier.verify(premise, hypothesis);
    
    // Should handle gracefully
    EXPECT_EQ(result.prediction, NLIPrediction::NEUTRAL);
}

TEST_F(NLIVerifierTest, BatchVerification) {
    std::vector<std::pair<std::string, std::string>> pairs = {
        {"Paris is in France.", "Paris is a city."},
        {"The sky is blue.", "The sky has color."},
        {"Dogs are animals.", "Cats are animals."}
    };
    
    auto results = verifier.verifyBatch(pairs);
    
    EXPECT_EQ(results.size(), pairs.size());
    
    for (const auto& result : results) {
        EXPECT_GE(result.confidence, 0.0);
        EXPECT_LE(result.confidence, 1.0);
    }
}

TEST_F(NLIVerifierTest, CacheFunctionality) {
    std::string premise = "Paris is the capital of France.";
    std::string hypothesis = "Paris is a capital city.";
    
    // Verify twice with same inputs
    auto result1 = verifier.verify(premise, hypothesis);
    auto result2 = verifier.verify(premise, hypothesis);
    
    // Results should be identical (cached)
    EXPECT_EQ(result1.prediction, result2.prediction);
    EXPECT_DOUBLE_EQ(result1.entailment_score, result2.entailment_score);
    EXPECT_DOUBLE_EQ(result1.confidence, result2.confidence);
}

TEST_F(NLIVerifierTest, ClearCache) {
    // Verify some pairs to populate cache
    verifier.verify("test premise", "test hypothesis");
    
    // Clear cache - should not throw
    EXPECT_NO_THROW(verifier.clearCache());
    
    // Verify again should work
    auto result = verifier.verify("test premise", "test hypothesis");
    EXPECT_GE(result.confidence, 0.0);
}

TEST_F(NLIVerifierTest, Warmup) {
    // Warmup should not throw
    EXPECT_NO_THROW(verifier.warmup());
}

TEST_F(NLIVerifierTest, ModelInfo) {
    std::string info = verifier.getModelInfo();
    
    EXPECT_FALSE(info.empty());
    EXPECT_TRUE(info.find("Model type") != std::string::npos ||
                info.find("Status") != std::string::npos);
}

TEST_F(NLIVerifierTest, SupportLevelConversion) {
    // Test entailment with high confidence
    auto support1 = nliPredictionToSupportLevel(
        NLIPrediction::ENTAILMENT, 0.8
    );
    EXPECT_EQ(support1, SupportLevel::FULLY_SUPPORTED);
    
    // Test entailment with low confidence
    auto support2 = nliPredictionToSupportLevel(
        NLIPrediction::ENTAILMENT, 0.5
    );
    EXPECT_EQ(support2, SupportLevel::PARTIALLY_SUPPORTED);
    
    // Test contradiction
    auto support3 = nliPredictionToSupportLevel(
        NLIPrediction::CONTRADICTION, 0.8
    );
    EXPECT_EQ(support3, SupportLevel::CONTRADICTED);
    
    // Test neutral
    auto support4 = nliPredictionToSupportLevel(
        NLIPrediction::NEUTRAL, 0.8
    );
    EXPECT_EQ(support4, SupportLevel::UNSUPPORTED);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(NLIVerifierConfigTest, CustomConfiguration) {
    NLIFaithfulnessVerifier::Config config;
    config.model_path = "/path/to/model";
    config.entailment_threshold = 0.8;
    config.use_heuristic_fallback = true;
    
    NLIFaithfulnessVerifier verifier(config);
    
    EXPECT_TRUE(verifier.isReady());
}

TEST(NLIVerifierConfigTest, NoFallback) {
    NLIFaithfulnessVerifier::Config config;
    config.use_heuristic_fallback = false;
    
    NLIFaithfulnessVerifier verifier(config);
    
    // Without model and without fallback, should still construct
    // but isReady() should be false
    EXPECT_FALSE(verifier.isReady());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(NLIVerifierTest, VerificationSpeed) {
    std::string premise = "Paris is the capital of France. It has many museums.";
    std::string hypothesis = "Paris is the capital city of France.";
    
    auto start = std::chrono::steady_clock::now();
    auto result = verifier.verify(premise, hypothesis);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // Heuristic should be very fast (< 10ms target)
    EXPECT_LT(duration_ms, 50);
    EXPECT_GE(result.confidence, 0.0);
}

TEST_F(NLIVerifierTest, BatchPerformance) {
    std::vector<std::pair<std::string, std::string>> pairs;
    for (int i = 0; i < 20; i++) {
        pairs.push_back({
            "Test premise number " + std::to_string(i),
            "Test hypothesis number " + std::to_string(i)
        });
    }
    
    auto start = std::chrono::steady_clock::now();
    auto results = verifier.verifyBatch(pairs);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    EXPECT_EQ(results.size(), pairs.size());
    // Should process 20 pairs in reasonable time
    EXPECT_LT(duration_ms, 200);
}
