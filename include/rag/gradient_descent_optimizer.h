// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace themis::rag {

/// @brief Gradient descent optimizer for RAG system parameters.
///
/// Finds optimal settings (retrieval threshold, reranker confidence cutoff,
/// budget allocation) that minimize cost while meeting quality constraints.
/// Uses stochastic gradient descent with learning rate decay and convergence
/// detection.
///
/// @details
/// Optimization problem:
/// minimize: cost_usd
/// subject to: NDCG@10 >= 0.50, rerank_beneficial_rate >= 0.75
///
/// Variables (parameters to optimize):
/// - retrieval_count: # retrieved documents (affects latency)
/// - rerank_threshold: Min relevance to rerank (affects cost)
/// - budget_allocation: % of budget for each tenant
/// - freshness_refresh_interval: Refresh frequency (tradeoff with cost)
///
/// Algorithm:
/// 1. Initialize with current settings
/// 2. Compute loss (cost) with current parameters
/// 3. Estimate gradients via finite differences or Monte Carlo
/// 4. Update parameters: theta = theta - alpha * grad(L)
/// 5. Decay learning rate: alpha = alpha * 0.999
/// 6. Repeat until convergence (gradient norm < epsilon)
///
/// Constraints are enforced via penalty terms in loss function.
///
/// @code
/// auto optimizer = std::make_unique<GradientDescentOptimizer>();
/// optimizer->SetObjective("minimize_cost");
/// optimizer->AddConstraint("ndcg_10_min", 0.50);
/// optimizer->AddConstraint("beneficial_rate_min", 0.75);
/// 
/// auto result = optimizer->Optimize(initial_params);
/// // result.optimal_params: best settings found
/// // result.final_cost: achieved cost at optimum
/// // result.iterations: # iterations to convergence
/// @endcode
class GradientDescentOptimizer {
 public:
  /// @brief Parameter configuration.
  struct ParameterConfig {
    std::string param_name;
    float min_value;
    float max_value;
    float initial_value;
    bool is_integer;
  };

  /// @brief Optimization result.
  struct OptimizationResult {
    std::map<std::string, float> optimal_params;  ///< Best parameters found
    float final_loss;                             ///< Loss at optimum
    std::vector<float> loss_history;              ///< Loss at each iteration
    uint32_t num_iterations;
    bool converged;
    float convergence_time_sec;
    std::string status;  ///< "converged" | "max_iter" | "constraint_infeasible"
  };

  /// @brief Constraint type.
  enum class ConstraintType {
    GreaterThan,   ///< param >= value
    LessThan,      ///< param <= value
    EqualTo        ///< param == value
  };

  /// @brief Constructor.
  /// @param learning_rate Initial learning rate.
  /// @param max_iterations Max iterations before stopping.
  /// @param convergence_epsilon Convergence threshold.
  GradientDescentOptimizer(
      float learning_rate = 0.01f,
      uint32_t max_iterations = 1000,
      float convergence_epsilon = 1e-6f);

  /// @brief Set objective function (loss to minimize).
  /// @param loss_fn Function: params -> loss value.
  void SetObjective(std::function<float(const std::map<std::string, float>&)> loss_fn);

  /// @brief Register parameter to optimize.
  /// @param config Parameter configuration.
  void RegisterParameter(const ParameterConfig& config);

  /// @brief Add constraint on parameter.
  /// @param param_name Parameter to constrain.
  /// @param type Constraint type.
  /// @param value Constraint value.
  void AddConstraint(
      const std::string& param_name,
      ConstraintType type,
      float value);

  /// @brief Run optimization.
  /// @param initial_params Starting parameter values.
  /// @return Optimization result with optimal parameters.
  OptimizationResult Optimize(const std::map<std::string, float>& initial_params);

  /// @brief Set learning rate.
  /// @param lr Learning rate.
  void SetLearningRate(float lr);

  /// @brief Set learning rate decay.
  /// @param decay Decay rate per iteration (typically 0.999).
  void SetLearningRateDecay(float decay);

  /// @brief Set gradient estimation method.
  /// @param method "finite_diff" or "monte_carlo".
  void SetGradientEstimation(const std::string& method);

  /// @brief Get optimization progress (mid-run).
  /// @return Map of stat_name → value.
  std::map<std::string, float> GetProgress();

 private:
  float learning_rate_;
  float learning_rate_decay_;
  uint32_t max_iterations_;
  float convergence_epsilon_;
  std::function<float(const std::map<std::string, float>&)> loss_fn_;
  std::map<std::string, ParameterConfig> parameters_;
  std::vector<std::tuple<std::string, ConstraintType, float>> constraints_;
  std::string gradient_estimation_method_;

  float ComputeLoss(const std::map<std::string, float>& params);
  std::map<std::string, float> EstimateGradient(
      const std::map<std::string, float>& params);
};

}  // namespace themis::rag
