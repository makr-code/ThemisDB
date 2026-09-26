// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/gradient_descent_optimizer.h"

#include <cmath>
#include <numeric>

namespace themis::rag {

GradientDescentOptimizer::GradientDescentOptimizer(
    float learning_rate,
    uint32_t max_iterations,
    float convergence_epsilon)
    : learning_rate_(learning_rate),
      learning_rate_decay_(0.999f),
      max_iterations_(max_iterations),
      convergence_epsilon_(convergence_epsilon),
      gradient_estimation_method_("finite_diff") {}

void GradientDescentOptimizer::SetObjective(
    std::function<float(const std::map<std::string, float>&)> loss_fn) {
  loss_fn_ = loss_fn;
}

void GradientDescentOptimizer::RegisterParameter(const ParameterConfig& config) {
  parameters_[config.param_name] = config;
}

void GradientDescentOptimizer::AddConstraint(
    const std::string& param_name,
    ConstraintType type,
    float value) {
  constraints_.push_back({param_name, type, value});
}

GradientDescentOptimizer::OptimizationResult GradientDescentOptimizer::Optimize(
    const std::map<std::string, float>& initial_params) {
  OptimizationResult result;
  result.status = "converged";

  // Initialize parameters
  auto current_params = initial_params;
  float current_loss = ComputeLoss(current_params);
  result.loss_history.push_back(current_loss);

  // Gradient descent loop
  for (uint32_t iter = 0; iter < max_iterations_; iter++) {
    // Estimate gradient
    auto gradient = EstimateGradient(current_params);

    // Check convergence
    float gradient_norm = 0.0f;
    for (const auto& [param, grad] : gradient) {
      gradient_norm += grad * grad;
    }
    gradient_norm = std::sqrt(gradient_norm);

    if (gradient_norm < convergence_epsilon_) {
      result.num_iterations = iter;
      result.converged = true;
      break;
    }

    // Update parameters: theta = theta - alpha * grad(L)
    for (auto& [param_name, value] : current_params) {
      if (gradient.find(param_name) != gradient.end()) {
        value -= learning_rate_ * gradient[param_name];

        // Enforce bounds
        auto param_config_it = parameters_.find(param_name);
        if (param_config_it != parameters_.end()) {
          value = std::max(value, param_config_it->second.min_value);
          value = std::min(value, param_config_it->second.max_value);
        }
      }
    }

    // Decay learning rate
    learning_rate_ *= learning_rate_decay_;

    // Compute new loss
    current_loss = ComputeLoss(current_params);
    result.loss_history.push_back(current_loss);
  }

  result.optimal_params = current_params;
  result.final_loss = current_loss;

  if (result.num_iterations >= max_iterations_) {
    result.status = "max_iter";
    result.converged = false;
  }

  return result;
}

void GradientDescentOptimizer::SetLearningRate(float lr) {
  learning_rate_ = lr;
}

void GradientDescentOptimizer::SetLearningRateDecay(float decay) {
  learning_rate_decay_ = decay;
}

void GradientDescentOptimizer::SetGradientEstimation(const std::string& method) {
  gradient_estimation_method_ = method;
}

std::map<std::string, float> GradientDescentOptimizer::GetProgress() {
  std::map<std::string, float> progress;
  progress["learning_rate"] = learning_rate_;
  progress["convergence_epsilon"] = convergence_epsilon_;
  return progress;
}

float GradientDescentOptimizer::ComputeLoss(const std::map<std::string, float>& params) {
  if (!loss_fn_) {
    return 0.0f;
  }

  float loss = loss_fn_(params);

  // Add constraint penalties
  for (const auto& [param_name, type, value] : constraints_) {
    auto param_it = params.find(param_name);
    if (param_it != params.end()) {
      float param_value = param_it->second;

      float penalty = 0.0f;
      if (type == ConstraintType::GreaterThan && param_value < value) {
        penalty = (value - param_value) * (value - param_value);
      } else if (type == ConstraintType::LessThan && param_value > value) {
        penalty = (param_value - value) * (param_value - value);
      } else if (type == ConstraintType::EqualTo && param_value != value) {
        penalty = (param_value - value) * (param_value - value);
      }

      loss += 100.0f * penalty;  // Penalty weight
    }
  }

  return loss;
}

std::map<std::string, float> GradientDescentOptimizer::EstimateGradient(
    const std::map<std::string, float>& params) {
  std::map<std::string, float> gradient;

  float epsilon = 1e-5f;
  float base_loss = ComputeLoss(params);

  for (const auto& [param_name, param_config] : parameters_) {
    auto perturbed_params = params;
    perturbed_params[param_name] += epsilon;

    float perturbed_loss = ComputeLoss(perturbed_params);
    float grad = (perturbed_loss - base_loss) / epsilon;

    gradient[param_name] = grad;
  }

  return gradient;
}

}  // namespace themis::rag
