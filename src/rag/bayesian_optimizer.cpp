/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bayesian_optimizer.cpp                             ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:13:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bayesian_optimizer.cpp
 * @brief Implementation of Bayesian optimization for parameter tuning
 */

#include "rag/bayesian_optimizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace themis::rag::learning {

struct BayesianOptimizer::Impl {
    std::unordered_map<std::string, ParameterBounds> param_bounds;
    std::vector<Observation> observations;
    std::mt19937 rng{std::random_device{}()};

    double best_objective = -std::numeric_limits<double>::infinity();
    std::unordered_map<std::string, double> best_params;
};

BayesianOptimizer::BayesianOptimizer(const std::unordered_map<std::string, ParameterBounds> &param_bounds)
    : impl_(std::make_unique<Impl>()) {
    impl_->param_bounds = param_bounds;
}

BayesianOptimizer::~BayesianOptimizer() = default;

std::unordered_map<std::string, double> BayesianOptimizer::suggest() {
    // Simplified strategy:
    // - First N iterations: random exploration
    // - Later: mix of exploration around best + random

    const size_t exploration_phase = 5;

    if (impl_->observations.size() < exploration_phase) {
        return sampleRandom();
    } else {
        // 70% around best, 30% random
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(impl_->rng) < 0.7) {
            return sampleAroundBest();
        } else {
            return sampleRandom();
        }
    }
}

void BayesianOptimizer::observe(const std::unordered_map<std::string, double> &params, double objective_value) {
    Observation obs;
    obs.params          = params;
    obs.objective_value = objective_value;
    impl_->observations.push_back(obs);

    // Update best if this is better
    if (objective_value > impl_->best_objective) {
        impl_->best_objective = objective_value;
        impl_->best_params    = params;
    }
}

std::unordered_map<std::string, double> BayesianOptimizer::getBestParams() const {
    return impl_->best_params;
}

double BayesianOptimizer::getBestObjective() const {
    return impl_->best_objective;
}

size_t BayesianOptimizer::getNumObservations() const {
    return impl_->observations.size();
}

std::unordered_map<std::string, double> BayesianOptimizer::sampleRandom() {
    std::unordered_map<std::string, double> params;

    for (const auto &[name, bounds] : impl_->param_bounds) {
        std::uniform_real_distribution<double> dist(bounds.min_value, bounds.max_value);
        params[name] = dist(impl_->rng);
    }

    return params;
}

std::unordered_map<std::string, double> BayesianOptimizer::sampleAroundBest() {
    std::unordered_map<std::string, double> params;

    // Sample within 20% of range around best
    for (const auto &[name, bounds] : impl_->param_bounds) {
        double best_val           = impl_->best_params.at(name);
        double range              = bounds.max_value - bounds.min_value;
        double perturbation_range = range * 0.2;

        std::uniform_real_distribution<double> dist(-perturbation_range, perturbation_range);
        double new_val = best_val + dist(impl_->rng);

        // Clamp to bounds
        new_val      = std::max(bounds.min_value, std::min(bounds.max_value, new_val));
        params[name] = new_val;
    }

    return params;
}

} // namespace themis::rag::learning
