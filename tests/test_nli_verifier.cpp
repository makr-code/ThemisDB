/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_nli_verifier.cpp                              ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:48:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     444                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_nli_verifier.cpp
 * @brief Unit tests for NLI Faithfulness Verifier
 */

#include <gtest/gtest.h>
#include "rag/nli_faithfulness_verifier.h"

using namespace themis::rag::judge;

class NLIVerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.max_sequence_length = 512;
        config_.entailment_threshold = 0.7;
        config_.enable_caching = true;
        config_.num_threads = 4;
        config_.use_gpu = false;
    }
    
    NLIFaithfulnessVerifier::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Constructor Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, DefaultConstructor) {
    NLIFaithfulnessVerifier verifier;
    EXPECT_TRUE(verifier.isReady());  // Heuristic fallback always ready
}

TEST_F(NLIVerifierTest, ConfigConstructor) {
    NLIFaithfulnessVerifier verifier(config_);
    EXPECT_TRUE(verifier.isReady());
}

// ═══════════════════════════════════════════════════════════
// Basic Verification Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, VerifyClaim_Entailment) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Paris is the capital of France";
    std::string document = "Paris is the capital of France and has a population of about 2 million.";
    
    auto result = verifier.verifyClaim(claim, document);
    
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.entailment_prob, 0.0);
    EXPECT_LE(result.entailment_prob, 1.0);
    EXPECT_GE(result.neutral_prob, 0.0);
    EXPECT_LE(result.neutral_prob, 1.0);
    EXPECT_GE(result.contradiction_prob, 0.0);
    EXPECT_LE(result.contradiction_prob, 1.0);
    
    // Probabilities should sum to approximately 1.0
    double sum = result.entailment_prob + result.neutral_prob + result.contradiction_prob;
    EXPECT_NEAR(sum, 1.0, 0.01);
    
    // High overlap should lead to high entailment probability
    EXPECT_GT(result.entailment_prob, 0.5);
}

TEST_F(NLIVerifierTest, VerifyClaim_NoSupport) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "The moon is made of cheese";
    std::string document = "Paris is the capital of France and has a population of about 2 million.";
    
    auto result = verifier.verifyClaim(claim, document);
    
    EXPECT_TRUE(result.success);
    // No overlap should lead to low entailment
    EXPECT_LT(result.entailment_prob, 0.5);
}

TEST_F(NLIVerifierTest, VerifyClaim_PartialMatch) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Paris has many tourists visiting the Eiffel Tower";
    std::string document = "Paris is the capital of France. The Eiffel Tower is located in Paris.";
    
    auto result = verifier.verifyClaim(claim, document);
    
    EXPECT_TRUE(result.success);
    // Partial overlap - should be somewhere in the middle
    EXPECT_GT(result.entailment_prob, 0.3);
    EXPECT_LT(result.entailment_prob, 0.9);
}

TEST_F(NLIVerifierTest, VerifyClaim_EmptyClaim) {
    NLIFaithfulnessVerifier verifier(config_);
    
    auto result = verifier.verifyClaim("", "Some document text");
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.label, NLILabel::NEUTRAL);
}

TEST_F(NLIVerifierTest, VerifyClaim_EmptyDocument) {
    NLIFaithfulnessVerifier verifier(config_);
    
    auto result = verifier.verifyClaim("Some claim", "");
    
    EXPECT_TRUE(result.success);
}

// ═══════════════════════════════════════════════════════════
// Batch Verification Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, VerifyClaimsBatch) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::vector<std::string> claims = {
        "Paris is the capital of France",
        "France is in Western Europe",
        "The Eiffel Tower is in Paris"
    };
    
    std::string document = "Paris is the capital of France, located in Western Europe. "
                          "The Eiffel Tower is a famous landmark in Paris.";
    
    auto results = verifier.verifyClaimsBatch(claims, document);
    
    ASSERT_EQ(results.size(), claims.size());
    
    for (size_t i = 0; i < results.size(); i++) {
        EXPECT_TRUE(results[i].success) << "Claim " << i << " failed";
        EXPECT_GE(results[i].entailment_prob, 0.0);
        EXPECT_LE(results[i].entailment_prob, 1.0);
    }
    
    // All claims should have reasonable entailment probabilities
    for (const auto& result : results) {
        EXPECT_GT(result.entailment_prob, 0.5);
    }
}

TEST_F(NLIVerifierTest, VerifyClaimsBatch_Empty) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::vector<std::string> claims;
    auto results = verifier.verifyClaimsBatch(claims, "Some document");
    
    EXPECT_TRUE(results.empty());
}

// ═══════════════════════════════════════════════════════════
// Multiple Documents Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, VerifyAgainstMultipleDocs) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "The Eiffel Tower is in Paris";
    
    std::vector<std::string> documents = {
        "Paris is the capital of France.",
        "France is in Western Europe.",
        "The Eiffel Tower is a famous landmark in Paris."  // This one supports the claim
    };
    
    auto result = verifier.verifyAgainstMultipleDocs(claim, documents);
    
    EXPECT_TRUE(result.success);
    // Should find the supporting document
    EXPECT_GT(result.entailment_prob, 0.6);
}

TEST_F(NLIVerifierTest, VerifyAgainstMultipleDocs_NoMatch) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "The moon is made of cheese";
    
    std::vector<std::string> documents = {
        "Paris is the capital of France.",
        "France is in Western Europe.",
        "The Eiffel Tower is a famous landmark in Paris."
    };
    
    auto result = verifier.verifyAgainstMultipleDocs(claim, documents);
    
    EXPECT_TRUE(result.success);
    // No document supports this claim
    EXPECT_LT(result.entailment_prob, 0.5);
}

TEST_F(NLIVerifierTest, VerifyAgainstMultipleDocs_Empty) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::vector<std::string> documents;
    auto result = verifier.verifyAgainstMultipleDocs("Some claim", documents);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.label, NLILabel::NEUTRAL);
}

// ═══════════════════════════════════════════════════════════
// Caching Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, Caching_HitOnSecondCall) {
    config_.enable_caching = true;
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Paris is the capital of France";
    std::string document = "Paris is the capital of France.";
    
    // First call - should be a cache miss
    auto result1 = verifier.verifyClaim(claim, document);
    auto stats1 = verifier.getCacheStats();
    
    // Second call - should be a cache hit
    auto result2 = verifier.verifyClaim(claim, document);
    auto stats2 = verifier.getCacheStats();
    
    EXPECT_EQ(stats2.hits, stats1.hits + 1);
    EXPECT_DOUBLE_EQ(result1.entailment_prob, result2.entailment_prob);
}

TEST_F(NLIVerifierTest, Caching_Disabled) {
    config_.enable_caching = false;
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Paris is the capital of France";
    std::string document = "Paris is the capital of France.";
    
    verifier.verifyClaim(claim, document);
    verifier.verifyClaim(claim, document);
    
    auto stats = verifier.getCacheStats();
    EXPECT_EQ(stats.hits, 0);  // No hits because caching disabled
}

TEST_F(NLIVerifierTest, ClearCache) {
    config_.enable_caching = true;
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Paris is the capital of France";
    std::string document = "Paris is the capital of France.";
    
    // Populate cache
    verifier.verifyClaim(claim, document);
    verifier.verifyClaim(claim, document);
    
    auto stats_before = verifier.getCacheStats();
    EXPECT_GT(stats_before.hits, 0);
    
    // Clear cache
    verifier.clearCache();
    
    auto stats_after = verifier.getCacheStats();
    EXPECT_EQ(stats_after.hits, 0);
    EXPECT_EQ(stats_after.misses, 0);
}

// ═══════════════════════════════════════════════════════════
// Utility Function Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, LabelToScore) {
    EXPECT_DOUBLE_EQ(nli_utils::labelToScore(NLILabel::ENTAILMENT), 1.0);
    EXPECT_DOUBLE_EQ(nli_utils::labelToScore(NLILabel::NEUTRAL), 0.5);
    EXPECT_DOUBLE_EQ(nli_utils::labelToScore(NLILabel::CONTRADICTION), 0.0);
}

TEST_F(NLIVerifierTest, AggregateFaithfulness) {
    std::vector<NLIResult> results;
    
    NLIResult r1;
    r1.success = true;
    r1.entailment_prob = 0.9;
    results.push_back(r1);
    
    NLIResult r2;
    r2.success = true;
    r2.entailment_prob = 0.7;
    results.push_back(r2);
    
    NLIResult r3;
    r3.success = true;
    r3.entailment_prob = 0.8;
    results.push_back(r3);
    
    double aggregated = nli_utils::aggregateFaithfulness(results);
    
    EXPECT_NEAR(aggregated, 0.8, 0.01);  // Average of 0.9, 0.7, 0.8
}

TEST_F(NLIVerifierTest, AggregateFaithfulness_Empty) {
    std::vector<NLIResult> results;
    double aggregated = nli_utils::aggregateFaithfulness(results);
    
    EXPECT_DOUBLE_EQ(aggregated, 0.5);  // Default neutral
}

TEST_F(NLIVerifierTest, IsFactualClaim_Factual) {
    EXPECT_TRUE(nli_utils::isFactualClaim("Paris is the capital of France."));
    EXPECT_TRUE(nli_utils::isFactualClaim("The population is 2 million."));
    EXPECT_TRUE(nli_utils::isFactualClaim("It was built in 1889."));
}

TEST_F(NLIVerifierTest, IsFactualClaim_Opinion) {
    EXPECT_FALSE(nli_utils::isFactualClaim("I think Paris is beautiful."));
    EXPECT_FALSE(nli_utils::isFactualClaim("In my opinion, France is the best."));
    EXPECT_FALSE(nli_utils::isFactualClaim("It seems to be a good place."));
}

TEST_F(NLIVerifierTest, IsFactualClaim_Reasoning) {
    EXPECT_FALSE(nli_utils::isFactualClaim("Because Paris is the capital, many tourists visit."));
    EXPECT_FALSE(nli_utils::isFactualClaim("Therefore, France is popular."));
}

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, GetConfig) {
    NLIFaithfulnessVerifier verifier(config_);
    auto retrieved_config = verifier.getConfig();
    
    EXPECT_EQ(retrieved_config.max_sequence_length, config_.max_sequence_length);
    EXPECT_DOUBLE_EQ(retrieved_config.entailment_threshold, config_.entailment_threshold);
    EXPECT_EQ(retrieved_config.enable_caching, config_.enable_caching);
}

TEST_F(NLIVerifierTest, SetConfig) {
    NLIFaithfulnessVerifier verifier;
    
    config_.entailment_threshold = 0.85;
    verifier.setConfig(config_);
    
    auto retrieved_config = verifier.getConfig();
    EXPECT_DOUBLE_EQ(retrieved_config.entailment_threshold, 0.85);
}

// ═══════════════════════════════════════════════════════════
// Performance Tests
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, Performance_SingleClaim) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Paris is the capital of France";
    std::string document = "Paris is the capital of France, located in Western Europe.";
    
    auto start = std::chrono::steady_clock::now();
    auto result = verifier.verifyClaim(claim, document);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Target: <50ms per claim
    // Stub implementation should be very fast
    EXPECT_LT(duration.count(), 50);
}

TEST_F(NLIVerifierTest, Performance_BatchClaims) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::vector<std::string> claims;
    for (int i = 0; i < 10; i++) {
        claims.push_back("Claim number " + std::to_string(i));
    }
    
    std::string document = "This is a test document.";
    
    auto start = std::chrono::steady_clock::now();
    auto results = verifier.verifyClaimsBatch(claims, document);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process 10 claims in reasonable time
    EXPECT_LT(duration.count(), 500);  // 500ms for 10 claims = 50ms per claim average
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(NLIVerifierTest, VeryLongClaim) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string long_claim = std::string(1000, 'a');  // 1000 character claim
    std::string document = "This is a short document.";
    
    auto result = verifier.verifyClaim(long_claim, document);
    
    EXPECT_TRUE(result.success);
}

TEST_F(NLIVerifierTest, VeryLongDocument) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "This is a claim";
    std::string long_document = std::string(5000, 'a');  // 5000 character document
    
    auto result = verifier.verifyClaim(claim, long_document);
    
    EXPECT_TRUE(result.success);
}

TEST_F(NLIVerifierTest, SpecialCharacters) {
    NLIFaithfulnessVerifier verifier(config_);
    
    std::string claim = "Spécial châracters: €£¥ 😀";
    std::string document = "Document with spécial châracters: €£¥";
    
    auto result = verifier.verifyClaim(claim, document);
    
    EXPECT_TRUE(result.success);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
