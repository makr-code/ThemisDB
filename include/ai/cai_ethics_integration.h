/*
 * ThemisDB | File: cai_ethics_integration.h | Version: 0.0.1 | Last Modified: 2026-06-01 11:42:53
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 180
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file cai_ethics_integration.h
 * @brief CAI Safety Module — Constitutional AI + EthicsEvaluator integration
 *
 * Bridges the ConstitutionalReasoningEngine (CAI self-critique / revision loop)
 * with the multi-dimensional EthicsEvaluator for Wave C C1 acceptance-gate
 * scoring.
 *
 * Design:
 *  - CAIEthicsIntegration wraps a ConstitutionalReasoningEngine and an
 *    EthicsEvaluator.
 *  - evaluate() runs the CAI critique-revision cycle (≤ 2 rounds by default)
 *    and then scores the result through the EthicsEvaluator's five dimensions.
 *  - The returned CAIEvaluationResult exposes both the CAI metrics and the
 *    ethics scores so callers can enforce acceptance criteria from issue #5040:
 *      · safety score alignment ≥ 0.80
 *      · false-positive rate ≤ 10 %
 *      · latency overhead ≤ 2.0 s per response
 *
 * References:
 *  - Bai et al. (2022) arXiv:2212.08073 — Constitutional AI
 *  - Wave C issue: #5040
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

/**
 * @brief Result returned by CAIEthicsIntegration::evaluate().
 */
struct CAIEvaluationResult {
    // --- CAI layer ---
    std::string original_response;
    std::string revised_response;
    bool was_revised              = false;
    int  cai_iterations           = 0;
    float cai_original_score      = 0.0f;   ///< 0–1, pre-revision CAI score
    float cai_revised_score       = 0.0f;   ///< 0–1, post-revision CAI score
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
    /// Combined safety score: average of cai_revised_score and ethics_overall_score.
    double safety_score() const {
        return (static_cast<double>(cai_revised_score) + ethics_overall_score) / 2.0;
    }

    // --- Timing ---
    std::chrono::milliseconds total_latency{0};
};

/**
 * @brief Configuration for CAIEthicsIntegration.
 */
struct CAIEthicsConfig {
    /// Maximum CAI critique-revision rounds (issue requires ≤ 2).
    int max_cai_rounds = 2;

    /// Minimum CAI score delta required to continue iterating.
    float improvement_threshold = 0.05f;

    /// Minimum acceptable CAI score; below this the response is flagged.
    float min_cai_score = 0.7f;

    /// EthicsEvaluator dimension weights (forwarded to EthicsEvaluator::Config).
    plugins::ethics::EthicsEvaluator::Config ethics_weights;

    /// If true, loadDefaultPrinciples() is called on the CAI engine at construction.
    bool load_default_principles = true;
};

/**
 * @brief CAI Safety Module integrating ConstitutionalReasoningEngine with EthicsEvaluator.
 *
 * Thread-safety: instances are NOT shared across threads; create one per request context.
 */
class CAIEthicsIntegration {
public:
    /**
     * @brief Construct with explicit configuration.
     * @param config  Integration configuration.
     */
    explicit CAIEthicsIntegration(const CAIEthicsConfig& config = {});

    ~CAIEthicsIntegration() = default;

    // Non-copyable, movable
    CAIEthicsIntegration(const CAIEthicsIntegration&)            = delete;
    CAIEthicsIntegration& operator=(const CAIEthicsIntegration&) = delete;
    CAIEthicsIntegration(CAIEthicsIntegration&&)                 = default;
    CAIEthicsIntegration& operator=(CAIEthicsIntegration&&)      = default;

    // -------------------------------------------------------------------------
    // Core evaluation
    // -------------------------------------------------------------------------

    /**
     * @brief Run CAI critique-revision loop, then score via EthicsEvaluator.
     *
     * @param response   LLM-generated response to evaluate.
     * @param query      Original user query (context for critique prompts).
     * @param llm_fn     Callable that generates LLM completions; signature:
     *                   std::string(const std::string& prompt).
     *                   Pass nullptr to use the rule-based fallback (no LLM).
     * @return CAIEvaluationResult with CAI and ethics scores.
     */
    CAIEvaluationResult evaluate(
        const std::string& response,
        const std::string& query,
        std::function<std::string(const std::string&)> llm_fn = nullptr
    );

    // -------------------------------------------------------------------------
    // Principle management (delegated to ConstitutionalReasoningEngine)
    // -------------------------------------------------------------------------

    /// Add a custom constitutional principle to the registry.
    void addPrinciple(const llm::ConstitutionalPrinciple& principle);

    /// Return all registered constitutional principles.
    std::vector<llm::ConstitutionalPrinciple> getPrinciples() const;

    /// Quick-access: number of registered principles.
    std::size_t principleCount() const;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    const CAIEthicsConfig& config() const { return config_; }

    /// Check whether the given response passes the acceptance gate
    /// (safety_score() ≥ 0.80 for non-flagged content).
    static bool passesAcceptanceCriteria(const CAIEvaluationResult& result,
                                         double min_safety_score = 0.80);

private:
    CAIEthicsConfig                                        config_;
    std::unique_ptr<llm::ConstitutionalReasoningEngine>    cai_engine_;
    plugins::ethics::EthicsEvaluator                       ethics_evaluator_;

    /// Build an EthicalDecision from a CAI result for EthicsEvaluator input.
    plugins::ethics::EthicalDecision buildDecision(
        const llm::ConstitutionalReasoningResult& cai_result,
        const std::string& query,
        const std::vector<std::string>& formalized_principles,
        const std::vector<std::string>& formalized_domains,
        const std::vector<std::string>& argument_chain_ids
    ) const;

    /// Build supporting EthicalArguments from violated/applied principles.
    std::vector<plugins::ethics::EthicalArgument> buildArguments(
        const llm::ConstitutionalReasoningResult& cai_result
    ) const;
};

} // namespace ai
} // namespace themis
