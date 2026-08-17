/**
 * @file cai_ethics_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟡 HARDENED-IMPLEMENTATION
 * @note Score: 88/100 (focused hardening implemented; full production validation still environment-dependent)
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Result returned by CAIEthicsIntegration::evaluate().
 *
 * Combines Constitutional AI scores with multi-dimensional ethics evaluation scores
 * into a comprehensive safety assessment. All scores are in the 0–1 range; total_latency
 * is reported in elapsed milliseconds and may exceed 100 ms.
 */
struct CAIEvaluationResult {
    // --- CAI layer ---
    /// Original (pre-revision) LLM response.
    std::string original_response;
    /// Revised response after CAI critique-revision loop (may equal original_response).
    std::string revised_response;
    /// Whether the response was modified during the CAI loop.
    bool was_revised              = false;
    /// Number of CAI critique-revision iterations performed (≤config.max_cai_rounds).
    int  cai_iterations           = 0;
    /// Original (pre-revision) CAI score (0–1 range).
    float cai_original_score      = 0.0f;
    /// Revised CAI score after critique-revision (0–1 range).
    float cai_revised_score       = 0.0f;
    /// Constitutional principles violated by the response.
    std::vector<std::string> violated_principles;
    /// Constitutional principles successfully applied to the response.
    std::vector<std::string> applied_principles;

    // --- Ethics-evaluator layer ---
    /// Overall ethics score combining all dimensions (0–1 range).
    double ethics_overall_score         = 0.0;
    /// Decision quality dimension score (0–1 range).
    double ethics_decision_quality      = 0.0;
    /// Consistency dimension score (0–1 range).
    double ethics_consistency           = 0.0;
    /// Fairness dimension score (0–1 range).
    double ethics_fairness              = 0.0;
    /// Alignment dimension score (0–1 range).
    double ethics_alignment             = 0.0;
    /// Transparency dimension score (0–1 range).
    double ethics_transparency          = 0.0;
    /// Ethics framework domains invoked (e.g., "autonomy", "fairness", "safety", …).
    std::vector<std::string> ethics_framework_domains;
    /// Traceability chain IDs for argument reasoning (e.g., "constitutional_chain:autonomy", …).
    std::vector<std::string> ethics_argument_chain_ids;
    /// Ethics framework principles formalized for this evaluation.
    std::vector<std::string> ethics_framework_principles;

    // --- Aggregated acceptance gate ---
    /// Combined safety score: average of cai_revised_score and ethics_overall_score.
    /// Used for acceptance gating: safety_score() >= min_safety_score.
    double safety_score() const {
        return (static_cast<double>(cai_revised_score) + ethics_overall_score) / 2.0;
    }

    // --- Timing ---
    /// Total latency of the evaluate() call (including CAI loop and ethics evaluation).
    std::chrono::milliseconds total_latency{0};
};

/**
 * @brief Configuration for CAIEthicsIntegration constructor.
 *
 * Tunes the behavior of both the Constitutional AI critique-revision loop and the
 * multi-dimensional EthicsEvaluator. Default values are sensible for production use.
 */
struct CAIEthicsConfig {
    /// Maximum CAI critique-revision rounds (issue requires ≤ 2 for latency constraints).
    int max_cai_rounds = 2;

    /// Minimum score improvement to continue CAI iterations (0–1 range; 0.05 = 5% delta).
    float improvement_threshold = 0.05f;

    /// Minimum acceptable CAI score before flagging for further review (0–1 range).
    float min_cai_score = 0.7f;

    /// EthicsEvaluator dimension weights (forwarded to EthicsEvaluator::Config).
    plugins::ethics::EthicsEvaluator::Config ethics_weights;

    /// If true, loadDefaultPrinciples() is called on the CAI engine at construction
    /// to register the 21 built-in constitutional principles.
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
    CAIEthicsIntegration(CAIEthicsIntegration&&)                 noexcept = default;
    CAIEthicsIntegration& operator=(CAIEthicsIntegration&&)      noexcept = default;

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
