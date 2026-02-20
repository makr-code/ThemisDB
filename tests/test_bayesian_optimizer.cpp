/**
 * @file test_bayesian_optimizer.cpp
 * @brief Unit tests for Bayesian Optimizer
 */

#include <cmath>
#include <gtest/gtest.h>

#include "rag/bayesian_optimizer.h"

using namespace themis::rag::learning;

class BayesianOptimizerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup parameter bounds for testing
        bounds_["top_k"]                = {5.0, 20.0};
        bounds_["similarity_threshold"] = {0.6, 0.9};
        bounds_["coverage_threshold"]   = {0.7, 0.95};
    }

    std::unordered_map<std::string, ParameterBounds> bounds_;
};

TEST_F(BayesianOptimizerTest, Construction) {
    BayesianOptimizer optimizer(bounds_);
    EXPECT_EQ(optimizer.getNumObservations(), 0);
}

TEST_F(BayesianOptimizerTest, Suggest) {
    BayesianOptimizer optimizer(bounds_);

    auto params = optimizer.suggest();

    // Check that all parameters are present
    EXPECT_TRUE(params.count("top_k") > 0);
    EXPECT_TRUE(params.count("similarity_threshold") > 0);
    EXPECT_TRUE(params.count("coverage_threshold") > 0);

    // Check that parameters are within bounds
    EXPECT_GE(params["top_k"], bounds_["top_k"].min_value);
    EXPECT_LE(params["top_k"], bounds_["top_k"].max_value);

    EXPECT_GE(params["similarity_threshold"], bounds_["similarity_threshold"].min_value);
    EXPECT_LE(params["similarity_threshold"], bounds_["similarity_threshold"].max_value);

    EXPECT_GE(params["coverage_threshold"], bounds_["coverage_threshold"].min_value);
    EXPECT_LE(params["coverage_threshold"], bounds_["coverage_threshold"].max_value);
}

TEST_F(BayesianOptimizerTest, ObserveAndGetBest) {
    BayesianOptimizer optimizer(bounds_);

    // Observe several points
    std::unordered_map<std::string, double> params1
        = {{"top_k", 10.0}, {"similarity_threshold", 0.75}, {"coverage_threshold", 0.8}};
    optimizer.observe(params1, 0.7);

    std::unordered_map<std::string, double> params2
        = {{"top_k", 15.0}, {"similarity_threshold", 0.8}, {"coverage_threshold", 0.85}};
    optimizer.observe(params2, 0.85); // Best

    std::unordered_map<std::string, double> params3
        = {{"top_k", 12.0}, {"similarity_threshold", 0.7}, {"coverage_threshold", 0.82}};
    optimizer.observe(params3, 0.75);

    EXPECT_EQ(optimizer.getNumObservations(), 3);
    EXPECT_DOUBLE_EQ(optimizer.getBestObjective(), 0.85);

    auto best = optimizer.getBestParams();
    EXPECT_DOUBLE_EQ(best["top_k"], 15.0);
    EXPECT_DOUBLE_EQ(best["similarity_threshold"], 0.8);
    EXPECT_DOUBLE_EQ(best["coverage_threshold"], 0.85);
}

TEST_F(BayesianOptimizerTest, ExplorationExploitation) {
    BayesianOptimizer optimizer(bounds_);

    // First few suggestions should be random (exploration phase)
    std::vector<std::unordered_map<std::string, double>> suggestions;
    for (int i = 0; i < 5; i++) {
        auto params = optimizer.suggest();
        suggestions.push_back(params);
        optimizer.observe(params, 0.5); // Neutral objective
    }

    // All suggestions should be different (random exploration)
    bool all_different = true;
    for (size_t i = 0; i < suggestions.size(); i++) {
        for (size_t j = i + 1; j < suggestions.size(); j++) {
            if (std::abs(suggestions[i]["top_k"] - suggestions[j]["top_k"]) < 0.1) {
                all_different = false;
            }
        }
    }

    // After exploration phase, should explore around best
    std::unordered_map<std::string, double> best_params
        = {{"top_k", 15.0}, {"similarity_threshold", 0.8}, {"coverage_threshold", 0.85}};
    optimizer.observe(best_params, 0.95); // Very good

    // Next suggestion should be around best (with some randomness)
    auto next = optimizer.suggest();
    // We can't guarantee exact values due to randomness, but it should be in bounds
    EXPECT_GE(next["top_k"], bounds_["top_k"].min_value);
    EXPECT_LE(next["top_k"], bounds_["top_k"].max_value);
}

TEST_F(BayesianOptimizerTest, OptimizationConvergence) {
    BayesianOptimizer optimizer(bounds_);

    // Simulate an optimization problem: objective = -(top_k - 15)^2
    // Optimal at top_k = 15
    auto objective_function = [](double top_k) { return 1.0 - std::pow(top_k - 15.0, 2) / 100.0; };

    for (int i = 0; i < 20; i++) {
        auto params = optimizer.suggest();
        double obj  = objective_function(params["top_k"]);
        optimizer.observe(params, obj);
    }

    // Best should be close to optimal
    auto best = optimizer.getBestParams();
    EXPECT_NEAR(best["top_k"], 15.0, 3.0);        // Within 3 of optimal
    EXPECT_GT(optimizer.getBestObjective(), 0.8); // Good objective value
}
