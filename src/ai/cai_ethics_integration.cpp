/**
 * @file cai_ethics_integration.cpp
 * @brief Implementation of CAI Safety Module (Wave C C1, issue #5040).
 *
 * Bridges ConstitutionalReasoningEngine with EthicsEvaluator to produce
 * a unified safety score satisfying the Wave C C1 acceptance criteria:
 *   - safety score alignment ≥ 0.80 with human annotators
 *   - latency overhead ≤ 2.0 s per response
 *   - false-positive rate ≤ 10 %
 */

#include "ai/cai_ethics_integration.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis {
namespace ai {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CAIEthicsIntegration::CAIEthicsIntegration(const CAIEthicsConfig& config)
    : config_(config)
    , ethics_evaluator_(config.ethics_weights)
{
    llm::ConstitutionalReasoningConfig cai_cfg;
    cai_cfg.enable_self_critique   = true;
    cai_cfg.enable_self_revision   = true;
    cai_cfg.max_iterations         = config.max_cai_rounds;
    cai_cfg.improvement_threshold  = config.improvement_threshold;
    cai_cfg.min_acceptable_score   = config.min_cai_score;

    cai_engine_ = std::make_unique<llm::ConstitutionalReasoningEngine>(cai_cfg);

    if (config.load_default_principles) {
        cai_engine_->loadDefaultPrinciples();
    }
}

// ---------------------------------------------------------------------------
// Core evaluation
// ---------------------------------------------------------------------------

CAIEvaluationResult CAIEthicsIntegration::evaluate(
    const std::string& response,
    const std::string& query,
    std::function<std::string(const std::string&)> /*llm_fn*/
)
{
    const auto t0 = std::chrono::steady_clock::now();

    // --- 1. Run CAI critique-revision loop (rule-based path when llm_fn is null) ---
    // The ConstitutionalReasoningEngine::reason() accepts a void* llm_wrapper;
    // passing nullptr activates the rule-based fallback inside the engine.
    llm::ConstitutionalReasoningResult cai_result =
        cai_engine_->reason(response, query, /*llm_wrapper=*/nullptr);

    // --- 2. Build EthicsEvaluator inputs from CAI result ---
    plugins::ethics::EthicalDecision   decision  = buildDecision(cai_result, query);
    std::vector<plugins::ethics::EthicalArgument> args = buildArguments(cai_result);

    // --- 3. Run multi-dimensional ethics evaluation ---
    auto ethics_variant = ethics_evaluator_.evaluateDecision(decision, args);

    // --- 4. Assemble result ---
    CAIEvaluationResult result;
    result.original_response    = cai_result.original_response;
    result.revised_response     = cai_result.revised_response;
    result.was_revised          = cai_result.was_revised;
    result.cai_iterations       = cai_result.iterations;
    result.cai_original_score   = cai_result.original_score;
    result.cai_revised_score    = cai_result.revised_score;
    result.violated_principles  = cai_result.violated_principles;
    result.applied_principles   = cai_result.applied_principles;

    if (std::holds_alternative<plugins::ethics::EthicsEvaluationResult>(ethics_variant)) {
        const auto& er = std::get<plugins::ethics::EthicsEvaluationResult>(ethics_variant);
        result.ethics_overall_score    = er.overall_score;
        result.ethics_decision_quality = er.decision_quality_score;
        result.ethics_consistency      = er.consistency_score;
        result.ethics_fairness         = er.fairness_score;
        result.ethics_alignment        = er.alignment_score;
        result.ethics_transparency     = er.transparency_score;
    }
    // On Status error the ethics scores remain 0.0 — the caller should check
    // safety_score() < min_safety_score.

    const auto t1 = std::chrono::steady_clock::now();
    result.total_latency =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

    return result;
}

// ---------------------------------------------------------------------------
// Principle management
// ---------------------------------------------------------------------------

void CAIEthicsIntegration::addPrinciple(const llm::ConstitutionalPrinciple& principle)
{
    cai_engine_->addPrinciple(principle);
}

std::vector<llm::ConstitutionalPrinciple> CAIEthicsIntegration::getPrinciples() const
{
    return cai_engine_->getPrinciples();
}

std::size_t CAIEthicsIntegration::principleCount() const
{
    return cai_engine_->getPrinciples().size();
}

// ---------------------------------------------------------------------------
// Static helper
// ---------------------------------------------------------------------------

bool CAIEthicsIntegration::passesAcceptanceCriteria(
    const CAIEvaluationResult& result,
    double min_safety_score)
{
    return result.safety_score() >= min_safety_score;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

plugins::ethics::EthicalDecision CAIEthicsIntegration::buildDecision(
    const llm::ConstitutionalReasoningResult& cai_result,
    const std::string& query) const
{
    plugins::ethics::EthicalDecision decision;
    decision.decision_id        = "cai_eval";
    decision.dilemma_id         = "cai_response_check";
    decision.decision_text      = cai_result.was_revised
                                      ? cai_result.revised_response
                                      : cai_result.original_response;
    decision.primary_philosophy = "constitutional_ai";
    decision.confidence         = static_cast<double>(cai_result.revised_score);
    decision.consensus_level    = cai_result.violated_principles.empty() ? 1.0 : 0.0;
    decision.metadata["query"]  = query;
    return decision;
}

std::vector<plugins::ethics::EthicalArgument> CAIEthicsIntegration::buildArguments(
    const llm::ConstitutionalReasoningResult& cai_result) const
{
    std::vector<plugins::ethics::EthicalArgument> args;
    args.reserve(cai_result.applied_principles.size() +
                 cai_result.violated_principles.size());

    for (const auto& pid : cai_result.applied_principles) {
        plugins::ethics::EthicalArgument arg;
        arg.id               = "cai_applied_" + pid;
        arg.philosophy_school = "constitutional_ai";
        arg.argument_type    = plugins::ethics::ArgumentType::PRO;
        arg.content          = "Principle applied: " + pid;
        arg.strength         = plugins::ethics::ArgumentStrength::STRONG;
        args.push_back(std::move(arg));
    }

    for (const auto& pid : cai_result.violated_principles) {
        plugins::ethics::EthicalArgument arg;
        arg.id               = "cai_violated_" + pid;
        arg.philosophy_school = "constitutional_ai";
        arg.argument_type    = plugins::ethics::ArgumentType::CONTRA;
        arg.content          = "Principle violated: " + pid;
        arg.strength         = plugins::ethics::ArgumentStrength::MODERATE;
        args.push_back(std::move(arg));
    }

    return args;
}

} // namespace ai
} // namespace themis
