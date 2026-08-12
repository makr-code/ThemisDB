/**
 * @file ethics_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <variant>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Ethics Evaluator
 * 
 * Evaluates ethical decisions across 5 dimensions:
 * 1. Decision Quality
 * 2. Consistency  
 * 3. Fairness
 * 4. Alignment
 * 5. Transparency
 */
class EthicsEvaluator {
public:
    /**
     * @brief Per-dimension weight configuration.
     *
     * Weights are normalised to sum to 1.0 in the constructor, so the caller
     * may supply arbitrary positive values.  Default values reproduce the
     * original hardcoded behaviour: 0.25 / 0.20 / 0.20 / 0.20 / 0.15.
     */
    struct Config {
        double weight_decision_quality = 0.25; ///< Decision Quality dimension weight
        double weight_consistency      = 0.20; ///< Consistency dimension weight
        double weight_fairness         = 0.20; ///< Fairness dimension weight
        double weight_alignment        = 0.20; ///< Alignment dimension weight
        double weight_transparency     = 0.15; ///< Transparency dimension weight
    };

    /// Default constructor — uses default Config weights.
    EthicsEvaluator() = default;

    /// Constructor with explicit configuration.
    explicit EthicsEvaluator(const Config& config);

    ~EthicsEvaluator() = default;
    
    /**
     * @brief Evaluate a decision
     * @param decision The decision to evaluate
     * @param arguments Supporting arguments
     * @return Evaluation result or error
     */
    std::variant<EthicsEvaluationResult, Status> evaluateDecision(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );

    /**
     * @brief Compute confidence from argument strength distribution
     *
     * Returns the strength-weighted average over all arguments mapped as:
     * WEAK=0.25, MODERATE=0.50, STRONG=0.75, DECISIVE=1.00.
     * Returns 0.5 (neutral) when the argument list is empty.
     *
     * @param arguments Generated arguments for this decision
     * @return Confidence score in [0.0, 1.0]
     */
    static double computeConfidence(const std::vector<EthicalArgument>& arguments);

    /**
     * @brief Compute consensus from inter-philosophy argument agreement
     *
     * Each philosophy school's arguments are tallied: PRO/SYNTHESIS count +1,
     * CONTRA/REBUTTAL count -1.  A school "agrees" when its net tally >= 0.
     * Consensus = fraction of schools that agree.
     * A single school always returns 1.0 (unanimous by definition).
     *
     * @param arguments Generated arguments spanning one or more philosophy schools
     * @return Consensus score in [0.0, 1.0]
     */
    static double computeConsensus(const std::vector<EthicalArgument>& arguments);

    // -----------------------------------------------------------------------
    // Prometheus Metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Record one completed `makeDecision()` call.
     *
     * Thread-safe.  Increments `ethics_decisions_total`.
     * Updates rolling confidence average and RAG-hit counter.
     *
     * @param confidence     Confidence from the decision (for running average).
     * @param rag_hit        Whether RAG context returned at least one result.
     * @param latency_ms     End-to-end `makeDecision()` wall-clock latency.
     */
    void recordDecision(double confidence, bool rag_hit, uint64_t latency_ms);

    /**
     * @brief Set the current argument store size (for the gauge metric).
     *
     * @param count  Total number of arguments currently in the store.
     */
    void setArgumentStoreSize(uint64_t count);

    /**
     * @brief Export all collected metrics in Prometheus text format (v0.0.4).
     *
     * Emitted metric families:
     *   - `ethics_decisions_total` — counter
     *   - `ethics_decision_latency_ms_total` — counter (cumulative ms; use for avg)
     *   - `ethics_rag_context_hits_total` — counter
     *   - `ethics_argument_confidence_avg` — gauge (rolling average)
     *   - `ethics_argument_store_size` — gauge
     *
     * Returns an empty string if no decisions have been recorded yet.
     */
    std::string getMetricsText() const;

private:
    // -----------------------------------------------------------------------
    // Metrics state (thread-safe via atomics)
    // -----------------------------------------------------------------------
    mutable std::atomic<uint64_t> decisions_total_{0};
    mutable std::atomic<uint64_t> rag_hits_total_{0};
    mutable std::atomic<uint64_t> latency_ms_total_{0};
    mutable std::atomic<uint64_t> argument_store_size_{0};
    /// Fixed-point running average: stored as sum * 1e6 for precision.
    mutable std::atomic<uint64_t> confidence_sum_micro_{0};

    // Dimension evaluators
    double evaluateDecisionQuality(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateConsistency(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateFairness(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateAlignment(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateTransparency(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );

    Config config_; ///< Active weight configuration
};

} // namespace ethics
} // namespace plugins
} // namespace themis
