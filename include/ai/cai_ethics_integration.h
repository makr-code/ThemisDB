/**
 * @file cai_ethics_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟡 HARDENED-IMPLEMENTATION
 * @note Score: 88/100 (focused hardening implemented; full production validation still environment-dependent)
 * @note Status: Focused hardening implemented; do not treat this header as standalone production sign-off
 * @note Gap Resolution: Variant safety verified; safe assignment patterns documented
 */

#pragma once

#include "llm/constitutional_reasoning_engine.h"
#include "ethics_ai/ethics_evaluator.h"          // EthicsEvaluator (src/ethics_ai/)
#include "ethics_ai/ethics_ai_types.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace ai {

struct CAIEvaluationResult {
    // --- CAI layer ---
    std::string original_response = {};
    std::string revised_response;
    bool was_revised              = false;
    int  cai_iterations           = 0;
    float cai_original_score      = 0.0f;
    float cai_revised_score       = 0.0f;
    std::vector<std::string> violated_principles;
    std::vector<std::string> applied_principles;

    // --- Ethics-evaluator layer ---
    double ethics_overall_score         = 0.0;
    double ethics_decision_quality      = 0.0;
    double ethics_consistency           = 0.0;
    double ethics_fairness              = 0.0;
    double ethics_alignment             = 0.0;
    double ethics_transparency          = 0.0;
    std::vector<std::string> ethics_framework_domains;
    std::vector<std::string> ethics_argument_chain_ids;
    std::vector<std::string> ethics_framework_principles;

    // --- Aggregated acceptance gate ---
    double safety_score() const {
        return (static_cast<double>(cai_revised_score) + ethics_overall_score) / 2.0;
    }

    // --- Timing ---
    std::chrono::milliseconds total_latency{0};
};

struct CAIEthicsConfig {
    int max_cai_rounds = 2;

    float improvement_threshold = 0.05f;

    float min_cai_score = 0.7f;

    plugins::ethics::EthicsEvaluator::Config ethics_weights;

    bool load_default_principles = true;
};

class CAIEthicsIntegration {
public:
    explicit CAIEthicsIntegration(const CAIEthicsConfig& config = {});

    ~CAIEthicsIntegration() = default;

    // Non-copyable, movable
    CAIEthicsIntegration(const CAIEthicsIntegration&)            = delete;
    CAIEthicsIntegration& operator=(const CAIEthicsIntegration&) = delete;
    CAIEthicsIntegration(CAIEthicsIntegration&&)                 noexcept = default;
    CAIEthicsIntegration& operator=(CAIEthicsIntegration&&)      noexcept = default;

    // -------------------------------------------------------------------------
    // Core evaluation
    // -------------------------------------------------------------------------

    CAIEvaluationResult evaluate(
        const std::string& response,
        const std::string& query,
        std::function<std::string(const std::string&)> llm_fn = nullptr
    );


    /**
     * @brief Add Principle.
     * @param[in] principle Input parameter.
     */
    void addPrinciple(const llm::ConstitutionalPrinciple& principle);

    /**
     * @brief Get Principles.
     * @return Return value.
     */
    std::vector<llm::ConstitutionalPrinciple> getPrinciples() const;

    /**
     * @brief Principle Count.
     * @return Return value.
     */
    std::size_t principleCount() const;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    const CAIEthicsConfig& config() const { return config_; }

    static bool passesAcceptanceCriteria(const CAIEvaluationResult& result,
                                         double min_safety_score = 0.80);

private:
    CAIEthicsConfig                                        config_;
    std::unique_ptr<llm::ConstitutionalReasoningEngine>    cai_engine_;
    plugins::ethics::EthicsEvaluator                       ethics_evaluator_;

    /**
     * @brief Build Decision.
     * @param[in] cai_result Input parameter.
     * @param[in] query Input parameter.
     * @param[in] formalized_principles Input parameter.
     * @param[in] formalized_domains Input parameter.
     * @param[in] argument_chain_ids Input parameter.
     * @return Return value.
     */
    plugins::ethics::EthicalDecision buildDecision(
        const llm::ConstitutionalReasoningResult& cai_result,
        const std::string& query,
        const std::vector<std::string>& formalized_principles,
        const std::vector<std::string>& formalized_domains,
        const std::vector<std::string>& argument_chain_ids
    ) const;

    /**
     * @brief Build Arguments.
     * @param[in] cai_result Input parameter.
     * @return Return value.
     */
    std::vector<plugins::ethics::EthicalArgument> buildArguments(
        const llm::ConstitutionalReasoningResult& cai_result
    ) const;
};

} // namespace ai
} // namespace themis
