#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "rag/faithfulness_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"

using namespace themis::rag::judge;

class NLIVerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.entailment_threshold = 0.7;
        config_.neutral_threshold = 0.4;
        config_.contradiction_threshold = 0.7;
        config_.batch_size = 8;
        config_.use_gpu = false;
    }

    NLIFaithfulnessVerifier::Config config_;
};

TEST_F(NLIVerifierTest, CheckEntailmentReturnsNormalizedScores) {
    NLIFaithfulnessVerifier verifier(config_);

    const auto result = verifier.checkEntailment(
        "Paris is the capital of France and is located in Europe.",
        "Paris is the capital of France.");

    EXPECT_GE(result.entailment_score, 0.0);
    EXPECT_LE(result.entailment_score, 1.0);
    EXPECT_GE(result.neutral_score, 0.0);
    EXPECT_LE(result.neutral_score, 1.0);
    EXPECT_GE(result.contradiction_score, 0.0);
    EXPECT_LE(result.contradiction_score, 1.0);

    const double sum = result.entailment_score + result.neutral_score + result.contradiction_score;
    EXPECT_NEAR(sum, 1.0, 1e-6);
}

TEST_F(NLIVerifierTest, CheckEntailmentPrefersEntailmentOnStrongOverlap) {
    NLIFaithfulnessVerifier verifier(config_);

    const auto result = verifier.checkEntailment(
        "The Eiffel Tower is in Paris and Paris is the capital of France.",
        "The Eiffel Tower is in Paris.");

    EXPECT_EQ(result.label, NLILabel::ENTAILMENT);
    EXPECT_GT(result.entailment_score, result.contradiction_score);
}

TEST_F(NLIVerifierTest, CheckEntailmentDetectsContradictionWithNegation) {
    NLIFaithfulnessVerifier verifier(config_);

    const auto result = verifier.checkEntailment(
        "Paris is the capital of France.",
        "Paris is not the capital of France.");

    EXPECT_EQ(result.label, NLILabel::CONTRADICTION);
    EXPECT_GT(result.contradiction_score, result.entailment_score);
}

TEST_F(NLIVerifierTest, VerifyRejectsEmptyInputs) {
    NLIFaithfulnessVerifier verifier(config_);

    const std::vector<std::pair<std::string, std::string>> docs = {
        {"d1", "Paris is the capital of France."}
    };

    const auto empty_answer = verifier.verify("", docs);
    EXPECT_DOUBLE_EQ(empty_answer.faithfulness_score, 0.0);
    EXPECT_FALSE(empty_answer.is_faithful);

    const std::vector<std::pair<std::string, std::string>> empty_docs;
    const auto empty_documents = verifier.verify("Paris is the capital of France.", empty_docs);
    EXPECT_DOUBLE_EQ(empty_documents.faithfulness_score, 0.0);
    EXPECT_FALSE(empty_documents.is_faithful);
}

TEST_F(NLIVerifierTest, VerifyProducesConsistentClaimCounts) {
    NLIFaithfulnessVerifier verifier(config_);

    const std::string answer =
        "Paris is the capital of France. "
        "The Eiffel Tower is in Paris.";

    const std::vector<std::pair<std::string, std::string>> docs = {
        {"d1", "Paris is the capital of France and the Eiffel Tower is in Paris."},
        {"d2", "France is in Europe."}
    };

    const auto result = verifier.verify(answer, docs);

    EXPECT_GT(result.total_claims, 0u);
    EXPECT_EQ(result.claims.size(), result.total_claims);
    EXPECT_EQ(
        result.total_claims,
        result.supported_claims + result.partially_supported_claims + result.unsupported_claims + result.contradicted_claims);
    EXPECT_GE(result.faithfulness_score, 0.0);
    EXPECT_LE(result.faithfulness_score, 1.0);
}

TEST_F(NLIVerifierTest, ConfigRoundtripWorks) {
    NLIFaithfulnessVerifier verifier;

    config_.entailment_threshold = 0.8;
    config_.batch_size = 16;
    verifier.setConfig(config_);

    const auto cfg = verifier.getConfig();
    EXPECT_DOUBLE_EQ(cfg.entailment_threshold, 0.8);
    EXPECT_EQ(cfg.batch_size, 16u);
}

TEST_F(NLIVerifierTest, LoadModelSetsLoadedFlag) {
    NLIFaithfulnessVerifier verifier;

    EXPECT_FALSE(verifier.isModelLoaded());
    verifier.loadModel("dummy-model-path");
    EXPECT_TRUE(verifier.isModelLoaded());
}

TEST_F(NLIVerifierTest, CheckEntailmentIsFastEnoughForHeuristicPath) {
    NLIFaithfulnessVerifier verifier(config_);

    const auto start = std::chrono::steady_clock::now();
    const auto result = verifier.checkEntailment(
        "Paris is the capital of France.",
        "Paris is the capital of France.");
    const auto end = std::chrono::steady_clock::now();

    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);

    const auto duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration_ms, 100);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
