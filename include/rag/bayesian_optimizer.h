/**
 * @file bayesian_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag::learning {

/**
 * @brief Parameter bounds for optimization
 */
struct ParameterBounds {
    double min_value = 0;
    double max_value;
};

/**
 * @brief A point in parameter space with observed objective value
 */
struct Observation {
    std::unordered_map<std::string, double> params;
    double objective_value;
};

/**
 * @brief Bayesian optimizer for parameter tuning
 *
 * Uses a simplified Expected Improvement acquisition function with
 * random exploration for practical RAG parameter optimization.
 */
class BayesianOptimizer {
  public:
    /**
     * @brief Constructor
     * @param param_bounds Map of parameter names to their bounds
     */
    explicit BayesianOptimizer(const std::unordered_map<std::string, ParameterBounds> &param_bounds);

    ~BayesianOptimizer();

    /**
     * @brief Suggest next parameter configuration to evaluate
     * @return Suggested parameters
     */
    std::unordered_map<std::string, double> suggest();

    /**
     * @brief Record observation of objective value for given parameters
     * @param params Parameters that were evaluated
     * @param objective_value Objective value (higher is better)
     */
    void observe(const std::unordered_map<std::string, double> &params, double objective_value);

    /**
     * @brief Get the best parameters observed so far
     * @return Best parameter configuration
     */
    std::unordered_map<std::string, double> getBestParams() const;

    /**
     * @brief Get the best objective value observed so far
     * @return Best objective value
     */
    double getBestObjective() const;

    /**
     * @brief Get number of observations recorded
     */
    size_t getNumObservations() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Helper methods
    std::unordered_map<std::string, double> sampleRandom();
    std::unordered_map<std::string, double> sampleAroundBest();
};

} // namespace themis::rag::learning
