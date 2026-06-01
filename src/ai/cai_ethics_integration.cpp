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

namespace {

void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

std::string mapPrincipleToEthicsDomain(const std::string& principle_id) {
    if (principle_id == "human_autonomy" ||
        principle_id == "consent_and_agency" ||
        principle_id == "non_manipulation" ||
        principle_id == "robustness_under_ambiguity") {
        return "autonomy";
    }
    if (principle_id == "fairness" ||
        principle_id == "vulnerability_protection" ||
        principle_id == "age_appropriate_safety") {
        return "fairness";
    }
    if (principle_id == "transparency" ||
        principle_id == "source_traceability" ||
        principle_id == "uncertainty_calibration" ||
        principle_id == "accountability") {
        return "transparency";
    }
    if (principle_id == "do_no_harm" ||
        principle_id == "escalation_prevention" ||
        principle_id == "medical_caution" ||
        principle_id == "financial_caution") {
        return "safety";
    }
    if (principle_id == "privacy_protection") {
        return "privacy";
    }
    if (principle_id == "lawful_compliance") {
        return "legality";
    }
    if (principle_id == "security_hardening" ||
        principle_id == "misuse_resistance") {
        return "security";
    }
    if (principle_id == "factual_reliability") {
        return "reliability";
    }
    if (principle_id == "respectful_tone") {
        return "respect";
    }

    return "constitutional_ai";
}

std::vector<std::string> collectFormalizedPrinciples(
    const llm::ConstitutionalReasoningResult& cai_result) {
    std::vector<std::string> principles;
    principles.reserve(cai_result.violated_principles.size() + cai_result.applied_principles.size());

    for (const auto& principle_id : cai_result.violated_principles) {
        appendUnique(principles, principle_id);
    }
    for (const auto& principle_id : cai_result.applied_principles) {
        appendUnique(principles, principle_id);
    }

    return principles;
}

std::vector<std::string> collectFormalizedDomains(const std::vector<std::string>& principle_ids) {
    std::vector<std::string> domains;
    domains.reserve(principle_ids.size());

    for (const auto& principle_id : principle_ids) {
        appendUnique(domains, mapPrincipleToEthicsDomain(principle_id));
    }

    return domains;
}

std::vector<std::string> makeArgumentChainIds(const std::vector<std::string>& domains) {
    std::vector<std::string> chain_ids;
    chain_ids.reserve(domains.size());

    for (const auto& domain : domains) {
        chain_ids.push_back("constitutional_chain:" + domain);
    }

    return chain_ids;
}

std::string joinValues(const std::vector<std::string>& values) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0u) {
            oss << ',';
        }
        oss << values[i];
    }
    return oss.str();
}

} // namespace

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
    std::function<std::string(const std::string&)> llm_fn
)
{
    const auto t0 = std::chrono::steady_clock::now();

    // --- 1. Run CAI critique-revision loop (rule-based path when llm_fn is empty) ---
    // The ConstitutionalReasoningEngine::reason() accepts a void* llm_wrapper;
    // when a prompt runner is supplied we forward its address for critique/revision
    // completions, otherwise the engine uses its deterministic fallback path.
    auto* llm_wrapper = llm_fn ? static_cast<void*>(&llm_fn) : nullptr;
    llm::ConstitutionalReasoningResult cai_result =
        cai_engine_->reason(response, query, llm_wrapper);
    const auto formalized_principles = collectFormalizedPrinciples(cai_result);
    const auto formalized_domains = collectFormalizedDomains(formalized_principles);
    const auto argument_chain_ids = makeArgumentChainIds(formalized_domains);

    // --- 2. Build EthicsEvaluator inputs from CAI result ---
    plugins::ethics::EthicalDecision   decision  =
        buildDecision(cai_result, query, formalized_principles, formalized_domains, argument_chain_ids);
    std::vector<plugins::ethics::EthicalArgument> args = buildArguments(cai_result);
    decision.confidence = std::clamp(
        (decision.confidence + plugins::ethics::EthicsEvaluator::computeConfidence(args)) / 2.0,
        0.0,
        1.0);
    decision.consensus_level = plugins::ethics::EthicsEvaluator::computeConsensus(args);

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
    result.ethics_framework_principles = formalized_principles;
    result.ethics_framework_domains = formalized_domains;
    result.ethics_argument_chain_ids = argument_chain_ids;
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
    const std::string& query,
    const std::vector<std::string>& formalized_principles,
    const std::vector<std::string>& formalized_domains,
    const std::vector<std::string>& argument_chain_ids) const
{
    plugins::ethics::EthicalDecision decision;
    decision.decision_id        = "cai_eval";
    decision.dilemma_id         = "cai_response_check";
    decision.decision_text      = cai_result.was_revised
                                      ? cai_result.revised_response
                                      : cai_result.original_response;
    decision.primary_philosophy = formalized_domains.empty()
                                      ? "constitutional_ai"
                                      : formalized_domains.front();
    decision.supporting_philosophies = formalized_domains;
    decision.argument_chain_ids = argument_chain_ids;
    decision.confidence         = static_cast<double>(cai_result.revised_score);
    decision.consensus_level    = cai_result.violated_principles.empty() ? 1.0 : 0.0;
    decision.metadata["query"]  = query;
    decision.metadata["evaluation_framework"] = "constitutional_ai";
    decision.metadata["formalized_principles"] = joinValues(formalized_principles);
    decision.metadata["formalized_domains"] = joinValues(formalized_domains);
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
        arg.philosophy_school = mapPrincipleToEthicsDomain(pid);
        arg.argument_type    = plugins::ethics::ArgumentType::PRO;
        arg.content          = "Principle applied: " + pid;
        arg.strength         = plugins::ethics::ArgumentStrength::STRONG;
        arg.principle_basis  = {pid};
        arg.metadata["constitutional_principle_id"] = pid;
        arg.metadata["ethics_domain"] = arg.philosophy_school;
        args.push_back(std::move(arg));
    }

    for (const auto& pid : cai_result.violated_principles) {
        plugins::ethics::EthicalArgument arg;
        arg.id               = "cai_violated_" + pid;
        arg.philosophy_school = mapPrincipleToEthicsDomain(pid);
        arg.argument_type    = plugins::ethics::ArgumentType::CONTRA;
        arg.content          = "Principle violated: " + pid;
        arg.strength         = plugins::ethics::ArgumentStrength::MODERATE;
        arg.principle_basis  = {pid};
        arg.metadata["constitutional_principle_id"] = pid;
        arg.metadata["ethics_domain"] = arg.philosophy_school;
        args.push_back(std::move(arg));
    }

    return args;
}

} // namespace ai
} // namespace themis
