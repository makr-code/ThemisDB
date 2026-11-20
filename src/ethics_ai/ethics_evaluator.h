/**
 * @file ethics_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
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

class EthicsEvaluator {
public:
    /**
     * @brief Weights for the five evaluator dimensions.
     * @note Every weight must be finite and non-negative, and their sum must be positive.
     *       Accepted weights are normalized before scoring.
     */
    struct Config {
        double weight_decision_quality = 0.25; ///< Decision Quality dimension weight
        double weight_consistency      = 0.20; ///< Consistency dimension weight
        double weight_fairness         = 0.20; ///< Fairness dimension weight
        double weight_alignment        = 0.20; ///< Alignment dimension weight
        double weight_transparency     = 0.15; ///< Transparency dimension weight
    };

    EthicsEvaluator() = default;

    /**
     * @brief Creates an evaluator with normalized dimension weights.
     * @param config Per-dimension weights; every value must be finite and non-negative.
     * @throws std::invalid_argument if any weight is non-finite/negative or the sum is not finite and positive.
     */
    explicit EthicsEvaluator(const Config& config);

    ~EthicsEvaluator() = default;

    /**
     * @brief Scores a decision and reports the evidence used by the fairness proxy.
     * @param decision Decision to score; confidence and consensus_level must be finite and in [0, 1].
     * @param arguments Arguments considered for the decision; empty or unnamed school evidence is reported as unavailable.
     * @return Evaluation scores and detailed metrics, or Status::Error for invalid decision scores.
     * @note Fairness is a philosophy-school source-diversity proxy, not a population or subgroup fairness assessment.
     *       With no named argument schools, fairness_score is zero and fairness_evidence_available is zero.
     */
    std::variant<EthicsEvaluationResult, Status> evaluateDecision(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );

    /**
     * @brief Compute Confidence.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    static double computeConfidence(const std::vector<EthicalArgument>& arguments);

    /**
     * @brief Compute Consensus.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    static double computeConsensus(const std::vector<EthicalArgument>& arguments);

    // -----------------------------------------------------------------------
    // Prometheus Metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Record Decision.
     * @param[in] confidence Input parameter.
     * @param[in] rag_hit Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordDecision(double confidence, bool rag_hit, uint64_t latency_ms);

    /**
     * @brief Set Argument Store Size.
     * @param[in] count Input parameter.
     */
    void setArgumentStoreSize(uint64_t count);

    /**
     * @brief Get Metrics Text.
     * @return Return value.
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
    mutable std::atomic<uint64_t> confidence_sum_micro_{0};

    // Dimension evaluators
    /**
     * @brief Evaluate Decision Quality.
     * @param[in] decision Input parameter.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    double evaluateDecisionQuality(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    /**
     * @brief Evaluate Consistency.
     * @param[in] decision Input parameter.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    double evaluateConsistency(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    /**
     * @brief Evaluate Fairness.
     * @param[in] decision Input parameter.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    double evaluateFairness(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    /**
     * @brief Evaluate Alignment.
     * @param[in] decision Input parameter.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    double evaluateAlignment(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    /**
     * @brief Evaluate Transparency.
     * @param[in] decision Input parameter.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    double evaluateTransparency(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );

    Config config_; ///< Active weight configuration
};

} // namespace ethics
} // namespace plugins
} // namespace themis
