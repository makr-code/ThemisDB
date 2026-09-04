/**
 * @file bao.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Bao: Making Learned Query Optimization Practical
// Paper: "Bao: Making Learned Query Optimization Practical" (SIGMOD'21)
// Authors: Ryan Marcus et al., MIT
//
// Key idea: ML-based query optimizer using Thompson Sampling
// Expected gain: +30-70% query performance
// Reference: https://dl.acm.org/doi/10.1145/3448016.3452838

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace themis {
namespace performance {
namespace phase3 {

/// Query plan representation
struct QueryPlan {
    std::string plan_id;
    std::vector<std::string> operators;  // JOIN, SCAN, AGGREGATE, etc.
    double estimated_cost;
};

/// Query execution result
struct QueryResult {
    double execution_time_ms = 0;
    size_t rows_returned;
    bool success;
};

/// Bao ML-based query optimizer
class BaoOptimizer {
public:
    BaoOptimizer();
    ~BaoOptimizer();
    
    // Generate alternative query plans
    std::vector<QueryPlan> generate_plans(const std::string& query);
    
    // Select best plan using Thompson Sampling
    QueryPlan select_plan(const std::string& query, const std::vector<QueryPlan>& plans);
    
    // Update model with execution feedback
    void update_model(const QueryPlan& plan, const QueryResult& result);
    
    // Get optimizer statistics
    struct Stats {
        size_t queries_optimized = 0;
        double avg_speedup;
        size_t model_updates;
    };
    Stats get_stats() const;

    /**
     * @brief Return the current plan-selection miss rate in [0.0, 1.0].
     *
     * Miss rate = fraction of queries where the selected plan was later
     * flagged as sub-optimal by the update_model() feedback loop.
     *
     * Used by Loop 1 (HNSW/Query) in ContinuousLearningOrchestrator to decide
     * whether to trigger retraining (threshold: > 0.15).
     *
     * @return Miss rate in [0.0, 1.0].  Returns 0.0 when no queries have
     *         been optimized yet.
     */
    [[nodiscard]] double getMissRate() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Thompson Sampling arms (one per query plan template)
    std::unordered_map<std::string, std::pair<double, double>> arms_;  // mean, variance
};

} // namespace phase3
} // namespace performance
} // namespace themis
