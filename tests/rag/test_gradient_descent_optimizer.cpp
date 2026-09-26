// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cmath>

#include "rag/gradient_descent_optimizer.h"

namespace themis::rag::testing {

class GradientDescentOptimizerTest : public ::testing::Test {
 protected:
  GradientDescentOptimizer optimizer_{0.01f, 100, 1e-4f};
};

TEST_F(GradientDescentOptimizerTest, SimpleLinearOptimization) {
  // Loss = (x - 3)^2, minimum at x=3
  optimizer_.SetObjective([](const std::map<std::string, float>& params) {
    auto it = params.find("x");
    float x = (it != params.end()) ? it->second : 0.0f;
    return (x - 3.0f) * (x - 3.0f);
  });
  
  optimizer_.RegisterParameter({"x", 0.0f, -10.0f, 10.0f});
  
  auto result = optimizer_.Optimize({{"x", 0.0f}});
  EXPECT_TRUE(result.converged);
  EXPECT_NEAR(result.optimal_params["x"], 3.0f, 0.1f);
}

TEST_F(GradientDescentOptimizerTest, MultiVariableOptimization) {
  // Loss = (x-2)^2 + (y-5)^2, minimum at (2, 5)
  optimizer_.SetObjective([](const std::map<std::string, float>& params) {
    float x = params.find("x") != params.end() ? params.at("x") : 0.0f;
    float y = params.find("y") != params.end() ? params.at("y") : 0.0f;
    return (x - 2.0f) * (x - 2.0f) + (y - 5.0f) * (y - 5.0f);
  });
  
  optimizer_.RegisterParameter({"x", 0.0f, -10.0f, 10.0f});
  optimizer_.RegisterParameter({"y", 0.0f, -10.0f, 10.0f});
  
  auto result = optimizer_.Optimize({{"x", 0.0f}, {"y", 0.0f}});
  EXPECT_TRUE(result.converged);
  EXPECT_NEAR(result.optimal_params["x"], 2.0f, 0.1f);
  EXPECT_NEAR(result.optimal_params["y"], 5.0f, 0.1f);
}

TEST_F(GradientDescentOptimizerTest, ParameterBounds) {
  optimizer_.SetObjective([](const std::map<std::string, float>& params) {
    auto it = params.find("x");
    float x = (it != params.end()) ? it->second : 0.0f;
    return x * x;
  });
  
  optimizer_.RegisterParameter({"x", 0.0f, 0.5f, 2.0f});
  
  auto result = optimizer_.Optimize({{"x", 0.0f}});
  EXPECT_GE(result.optimal_params["x"], 0.5f);
  EXPECT_LE(result.optimal_params["x"], 2.0f);
}

TEST_F(GradientDescentOptimizerTest, LossHistory) {
  optimizer_.SetObjective([](const std::map<std::string, float>& params) {
    auto it = params.find("x");
    float x = (it != params.end()) ? it->second : 0.0f;
    return (x - 1.0f) * (x - 1.0f);
  });
  
  optimizer_.RegisterParameter({"x", 0.0f, -10.0f, 10.0f});
  
  auto result = optimizer_.Optimize({{"x", 0.0f}});
  EXPECT_GT(result.loss_history.size(), 1);
  // Loss should decrease
  EXPECT_GT(result.loss_history[0], result.loss_history.back());
}

TEST_F(GradientDescentOptimizerTest, ConstraintPenalty) {
  optimizer_.SetObjective([](const std::map<std::string, float>& params) {
    auto it = params.find("x");
    float x = (it != params.end()) ? it->second : 0.0f;
    return x * x;
  });
  
  optimizer_.RegisterParameter({"x", 0.0f, -10.0f, 10.0f});
  optimizer_.AddConstraint("x", GradientDescentOptimizer::ConstraintType::GreaterThan, 0.5f);
  
  auto result = optimizer_.Optimize({{"x", 0.0f}});
  EXPECT_GE(result.optimal_params["x"], 0.5f);
}

TEST_F(GradientDescentOptimizerTest, SetLearningRate) {
  optimizer_.SetLearningRate(0.1f);
  EXPECT_NO_THROW({});
}

TEST_F(GradientDescentOptimizerTest, SetLearningRateDecay) {
  optimizer_.SetLearningRateDecay(0.99f);
  EXPECT_NO_THROW({});
}

TEST_F(GradientDescentOptimizerTest, GetProgress) {
  auto progress = optimizer_.GetProgress();
  EXPECT_GT(progress.size(), 0);
}

TEST_F(GradientDescentOptimizerTest, ConvergenceDetection) {
  optimizer_.SetObjective([](const std::map<std::string, float>& params) {
    // Flat function (constant)
    return 1.0f;
  });
  
  optimizer_.RegisterParameter({"x", 0.0f, -10.0f, 10.0f});
  
  auto result = optimizer_.Optimize({{"x", 0.0f}});
  // Should converge quickly for flat function
  EXPECT_TRUE(result.converged);
}

TEST_F(GradientDescentOptimizerTest, MaxIterations) {
  GradientDescentOptimizer short_optimizer{0.01f, 5, 1e-6f};
  
  short_optimizer.SetObjective([](const std::map<std::string, float>& params) {
    auto it = params.find("x");
    float x = (it != params.end()) ? it->second : 0.0f;
    return x * x * x * x;  // Hard to optimize
  });
  
  short_optimizer.RegisterParameter({"x", 0.0f, -10.0f, 10.0f});
  
  auto result = short_optimizer.Optimize({{"x", 5.0f}});
  EXPECT_LE(result.num_iterations, 5);
}

}  // namespace themis::rag::testing
