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
    struct Config {
        double weight_decision_quality = 0.25; ///< Decision Quality dimension weight
        double weight_consistency      = 0.20; ///< Consistency dimension weight
        double weight_fairness         = 0.20; ///< Fairness dimension weight
        double weight_alignment        = 0.20; ///< Alignment dimension weight
        double weight_transparency     = 0.15; ///< Transparency dimension weight
    };

    EthicsEvaluator() = default;

    /**
     * @brief Ethics Evaluator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit EthicsEvaluator(const Config& config);

    ~EthicsEvaluator() = default;
    
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
