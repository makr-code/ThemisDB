#include <gtest/gtest.h>

#include "ethics_ai/ethics_evaluator.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace themis::plugins::ethics;

namespace {

EthicalDecision validDecision() {
    EthicalDecision decision;
    decision.confidence = 0.5;
    decision.consensus_level = 0.5;
    return decision;
}

EthicalArgument argumentFrom(std::string school) {
    EthicalArgument argument;
    argument.philosophy_school = std::move(school);
    return argument;
}

} // namespace

TEST(EthicsAiPolicyEvaluation, RejectsInvalidWeightConfiguration) {
    EthicsEvaluator::Config config;
    config.weight_fairness = -0.1;
    EXPECT_THROW({ EthicsEvaluator evaluator(config); }, std::invalid_argument);

    config = {};
    config.weight_decision_quality = 0.0;
    config.weight_consistency = 0.0;
    config.weight_fairness = 0.0;
    config.weight_alignment = 0.0;
    config.weight_transparency = 0.0;
    EXPECT_THROW({ EthicsEvaluator evaluator(config); }, std::invalid_argument);

    config = {};
    config.weight_alignment = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW({ EthicsEvaluator evaluator(config); }, std::invalid_argument);

    config = {};
    config.weight_decision_quality = std::numeric_limits<double>::max();
    config.weight_consistency = std::numeric_limits<double>::max();
    EXPECT_THROW({ EthicsEvaluator evaluator(config); }, std::invalid_argument);
}

TEST(EthicsAiPolicyEvaluation, RejectsNonFiniteAndOutOfRangeDecisionScores) {
    EthicsEvaluator evaluator;
    const std::vector<double> invalid_values{
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -0.01,
        1.01,
    };

    for (const double value : invalid_values) {
        auto decision = validDecision();
        decision.confidence = value;
        auto confidence_result = evaluator.evaluateDecision(decision, {});
        ASSERT_TRUE(std::holds_alternative<Status>(confidence_result));
        EXPECT_FALSE(std::get<Status>(confidence_result).isOK());

        decision = validDecision();
        decision.consensus_level = value;
        auto consensus_result = evaluator.evaluateDecision(decision, {});
        ASSERT_TRUE(std::holds_alternative<Status>(consensus_result));
        EXPECT_FALSE(std::get<Status>(consensus_result).isOK());
    }
}

TEST(EthicsAiPolicyEvaluation, FairnessReportsOnlyNamedArgumentEvidence) {
    EthicsEvaluator evaluator;
    auto decision = validDecision();
    decision.supporting_philosophies = {"kant", "kant", ""};
    const std::vector<EthicalArgument> arguments{
        argumentFrom("kant"),
        argumentFrom(""),
        argumentFrom(""),
    };

    auto result = evaluator.evaluateDecision(decision, arguments);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    const auto& evaluation = std::get<EthicsEvaluationResult>(result);

    EXPECT_DOUBLE_EQ(evaluation.fairness_score, 0.8);
    EXPECT_DOUBLE_EQ(evaluation.detailed_metrics.at("fairness_evidence_available"), 1.0);
    EXPECT_DOUBLE_EQ(evaluation.detailed_metrics.at("fairness_evidence_argument_count"), 1.0);
    EXPECT_DOUBLE_EQ(evaluation.detailed_metrics.at("fairness_evidence_school_count"), 1.0);
}

TEST(EthicsAiPolicyEvaluation, MissingArgumentSchoolEvidenceIsReportedAsUnavailable) {
    EthicsEvaluator evaluator;
    auto decision = validDecision();
    decision.supporting_philosophies = {"kant", "rawls"};

    auto result = evaluator.evaluateDecision(decision, {});
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(result));
    const auto& evaluation = std::get<EthicsEvaluationResult>(result);

    EXPECT_DOUBLE_EQ(evaluation.fairness_score, 0.0);
    EXPECT_DOUBLE_EQ(evaluation.detailed_metrics.at("fairness_evidence_available"), 0.0);
    EXPECT_DOUBLE_EQ(evaluation.detailed_metrics.at("fairness_evidence_argument_count"), 0.0);
    EXPECT_DOUBLE_EQ(evaluation.detailed_metrics.at("fairness_evidence_school_count"), 0.0);
}
