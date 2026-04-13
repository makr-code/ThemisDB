/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bayesian_optimizer.h                               ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:25:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bayesian_optimizer.h
 * @brief Bayesian optimization for RAG parameter tuning
 *
 * Implements a simplified Bayesian optimization approach for automatic
 * tuning of retrieval parameters (top_k, similarity_threshold, etc.)
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
    double min_value;
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
