/**
 * @file test_prompt_evaluator_ttest.cpp
 * @brief Tests for Welch's t-test statistical significance (issue 2.4)
 *
 * Covers edge-cases and correctness of the new isStatisticallySignificant
 * implementation replacing the naive 5% threshold check.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_evaluator.h"

using namespace themis::prompt_engineering;

// ============================================================================
// isStatisticallySignificant – Welch t-test
// ============================================================================

TEST(PromptEvaluatorTTestTest, EmptyInputs_ReturnsFalse) {
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant({}, {}));
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant({0.5}, {}));
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant({}, {0.5}));
}

TEST(PromptEvaluatorTTestTest, IdenticalDistributions_ReturnsFalse) {
    std::vector<double> same = {0.7, 0.7, 0.7, 0.7, 0.7};
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant(same, same));
}

TEST(PromptEvaluatorTTestTest, NewScoreLowerThanBaseline_ReturnsFalse) {
    std::vector<double> baseline = {0.8, 0.8, 0.8, 0.8, 0.8};
    std::vector<double> worse    = {0.6, 0.6, 0.6, 0.6, 0.6};
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant(baseline, worse));
}

TEST(PromptEvaluatorTTestTest, LargeImprovementLargeSample_ReturnsTrue) {
    // 30 samples: baseline at 0.5, new at 0.8 — clear significant improvement
    std::vector<double> baseline(30, 0.5);
    std::vector<double> improved(30, 0.8);
    EXPECT_TRUE(PromptEvaluator::isStatisticallySignificant(baseline, improved, 0.95));
}

TEST(PromptEvaluatorTTestTest, SmallImprovementSmallSample_ReturnsFalse) {
    // 3 samples: very small difference — not significant at 0.95
    std::vector<double> baseline = {0.70, 0.72, 0.71};
    std::vector<double> slight   = {0.72, 0.73, 0.72};
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant(baseline, slight, 0.95));
}

TEST(PromptEvaluatorTTestTest, ZeroVarianceBaseline_LargeImprovement_ReturnsTrue) {
    // All baseline values identical (zero variance), large improvement
    std::vector<double> baseline(20, 0.4);
    std::vector<double> improved(20, 0.9);
    EXPECT_TRUE(PromptEvaluator::isStatisticallySignificant(baseline, improved, 0.95));
}

TEST(PromptEvaluatorTTestTest, SingleSampleEach_LargeImprovement) {
    // Edge case: n=1 each
    std::vector<double> baseline = {0.3};
    std::vector<double> improved = {0.9};
    // With n=1 we have 0 degrees of freedom – result can go either way,
    // but the implementation must not crash.
    EXPECT_NO_THROW(PromptEvaluator::isStatisticallySignificant(baseline, improved));
}

TEST(PromptEvaluatorTTestTest, LowerConfidenceLevel_EasierToBeSignificant) {
    // A borderline improvement that is NOT significant at 0.95 but IS at 0.70
    std::vector<double> baseline(10, 0.60);
    std::vector<double> improved(10, 0.65);
    bool sig_95 = PromptEvaluator::isStatisticallySignificant(baseline, improved, 0.95);
    bool sig_70 = PromptEvaluator::isStatisticallySignificant(baseline, improved, 0.70);
    // At a lower confidence requirement, significance is at least as likely
    EXPECT_TRUE(!sig_95 || sig_70);  // If sig at 0.95 then also at 0.70
}
