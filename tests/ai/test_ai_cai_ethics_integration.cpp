#include <gtest/gtest.h>

#include "ai/cai_ethics_integration.h"

#include <limits>
#include <stdexcept>
#include <string>

using themis::ai::CAIEthicsIntegration;
using themis::ai::CAIEvaluationResult;

TEST(CAIEthicsIntegrationTest, RuleBasedFallbackIsExplicitAndFailsAcceptanceGate) {
    CAIEthicsIntegration integration;
    const auto result = integration.evaluate("You must do this immediately.", "What should I do?");

    EXPECT_TRUE(result.used_rule_based_fallback);
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 0.0));
}

TEST(CAIEthicsIntegrationTest, EmptyProviderOutputIsReportedAsFallback) {
    CAIEthicsIntegration integration;
    const auto result = integration.evaluate(
        "You must do this immediately.",
        "What should I do?",
        [](const std::string&) { return std::string{}; });

    EXPECT_TRUE(result.used_rule_based_fallback);
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 0.0));
}

TEST(CAIEthicsIntegrationTest, NonEmptyProviderOutputIsNotReportedAsFallback) {
    CAIEthicsIntegration integration;
    int callback_calls = 0;
    const auto result = integration.evaluate(
        "You must do this immediately.",
        "What should I do?",
        [&callback_calls](const std::string&) {
            ++callback_calls;
            return std::string{"Use a less directive response."};
        });

    EXPECT_GT(callback_calls, 0);
    EXPECT_TRUE(result.ethics_evaluation_succeeded);
    EXPECT_FALSE(result.used_rule_based_fallback);
}

TEST(CAIEthicsIntegrationTest, ThrowingProviderIsReportedAsFallback) {
    CAIEthicsIntegration integration;
    const auto result = integration.evaluate(
        "You must do this immediately.",
        "What should I do?",
        [](const std::string&) -> std::string {
            throw std::runtime_error("provider unavailable");
        });

    EXPECT_TRUE(result.used_rule_based_fallback);
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 0.0));
}

TEST(CAIEthicsIntegrationTest, InvalidAcceptanceThresholdFailsClosed) {
    CAIEvaluationResult result;
    result.ethics_evaluation_succeeded = true;
    result.cai_revised_score = 1.0f;
    result.ethics_overall_score = 1.0;

    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(
        result, std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 1.01));
}

TEST(CAIEthicsIntegrationTest, EthicsEvaluationFailureCannotPassZeroThreshold) {
    CAIEvaluationResult result;
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 0.0));
}

TEST(CAIEthicsIntegrationTest, OutOfRangeScoreCannotPassAcceptanceGate) {
    CAIEvaluationResult result;
    result.ethics_evaluation_succeeded = true;
    result.cai_revised_score = -0.2f;
    result.ethics_overall_score = 1.2;
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 0.4));

    result.cai_revised_score = 0.9f;
    result.ethics_overall_score = 0.9;
    result.ethics_fairness = 1.1;
    EXPECT_FALSE(CAIEthicsIntegration::passesAcceptanceCriteria(result, 0.8));
}
