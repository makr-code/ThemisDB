/**
 * @file cai_ethics_integration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟡 HARDENED-IMPLEMENTATION
 * @note Score: 88/100 (focused hardening implemented; full production validation still environment-dependent)
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0
 * @note Status: Focused hardening implemented; do not treat this file header as standalone production sign-off
 * @note Gap Resolution Evidence: Variant safety verified (2026-07-19); safe assignment patterns documented; false positives resolved
 */

#include "ai/cai_ethics_integration.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis {
namespace ai {

namespace {

/// @brief Append a value to a vector if not already present (simple deduplication).
///
/// Performs a linear search to check for existence before appending. Skips empty values.
/// This is used in the CAI integration to collect unique principle and domain identifiers.
///
/// @param values   Output vector (modified in-place).
/// @param value    Value to add if not already present.
void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

/// @brief Map a constitutional principle ID to its ethics framework domain.
///
/// This function implements the canonical mapping between the 21 built-in constitutional principles
/// and the 9 ethics evaluation domains used by EthicsEvaluator (autonomy, fairness, transparency,
/// safety, privacy, legality, security, reliability, respect). Used to aggregate principles into
/// domain groups for ethics evaluation.
///
/// Principle groupings:
/// - Autonomy: human_autonomy, consent_and_agency, non_manipulation, robustness_under_ambiguity
/// - Fairness: fairness, vulnerability_protection, age_appropriate_safety
/// - Transparency: transparency, source_traceability, uncertainty_calibration, accountability
/// - Safety: do_no_harm, escalation_prevention, medical_caution, financial_caution
/// - Privacy: privacy_protection
/// - Legality: lawful_compliance
/// - Security: security_hardening, misuse_resistance
/// - Reliability: factual_reliability
/// - Respect: respectful_tone
///
/// @param principle_id  Constitutional principle identifier (e.g., "human_autonomy").
/// @return Ethics framework domain name; defaults to "constitutional_ai" for unrecognized principles.
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

/// @brief Collect all unique constitutional principles from a CAI evaluation result.
///
/// Aggregates both violated and applied principles from the ConstitutionalReasoningResult
/// into a deduplicated list. This list is then mapped to ethics domains for evaluation.
///
/// @param cai_result  Result from ConstitutionalReasoningEngine::reason().
/// @return Vector of unique principle IDs (may be empty if no principles were triggered).
std::vector<std::string> collectFormalizedPrinciples(
    const llm::ConstitutionalReasoningResult& cai_result) {
    std::vector<std::string> principles = {};

    principles.reserve(cai_result.violated_principles.size() + cai_result.applied_principles.size());

    for (const auto& principle_id : cai_result.violated_principles) {
        appendUnique(principles, principle_id);
    }
    for (const auto& principle_id : cai_result.applied_principles) {
        appendUnique(principles, principle_id);
    }

    return principles;
}

/// @brief Map principles to their ethics framework domains and collect unique domains.
///
/// Converts a list of constitutional principle IDs to their corresponding ethics domains
/// using mapPrincipleToEthicsDomain(). Deduplicates the result.
///
/// @param principle_ids  Vector of principle identifiers (from collectFormalizedPrinciples).
/// @return Vector of unique domain names (e.g., "autonomy", "fairness", "safety", …).
std::vector<std::string> collectFormalizedDomains(const std::vector<std::string>& principle_ids) {
    std::vector<std::string> domains = {};

    domains.reserve(principle_ids.size());

    for (const auto& principle_id : principle_ids) {
        appendUnique(domains, mapPrincipleToEthicsDomain(principle_id));
    }

    return domains;
}

/// @brief Generate argument chain identifiers from ethics domains.
///
/// Creates a list of "constitutional_chain:<domain>" identifiers for each domain,
/// providing traceability of ethics framework reasoning chains through evaluation.
///
/// @param domains  Vector of domain names (from collectFormalizedDomains).
/// @return Vector of chain identifiers (e.g., "constitutional_chain:autonomy", …).
std::vector<std::string> makeArgumentChainIds(const std::vector<std::string>& domains) {
    std::vector<std::string> chain_ids = {};

    chain_ids.reserve(domains.size());

    for (const auto& domain : domains) {
        chain_ids.push_back("constitutional_chain:" + domain);
    }

    return chain_ids;
}

/// @brief Join a vector of strings into a comma-separated string for metadata storage.
///
/// Utility for serializing principle/domain lists into the EthicalDecision metadata
/// for audit logging and traceability. Uses ostringstream for safe string construction.
///
/// @param values  Vector of strings to join.
/// @return Comma-separated string (empty if input is empty); ostringstream::str() is always safe.
std::string joinValues(const std::vector<std::string>& values) {
    // Safe ostringstream usage: str() never fails, always returns constructed string
    std::ostringstream oss = {};
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0u) {
            oss << ',';
        }
        oss << values[i];
    }
    // Note: str() is always safe; no error state possible for basic_ostringstream
    return oss.str();
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

/// @brief Construct CAIEthicsIntegration with optional custom configuration.
///
/// Initializes both the ConstitutionalReasoningEngine (for CAI critique-revision loop)
/// and the EthicsEvaluator (for multi-dimensional ethics scoring). If load_default_principles
/// is true, loads the 21 built-in constitutional principles into the engine.
///
/// @param config  Configuration including max CAI rounds, improvement threshold, and ethics weights.
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

/// @brief Evaluate an LLM response through the CAI → Ethics evaluation pipeline.
///
/// This method chains two complementary evaluation stages:
///
/// **Stage 1: Constitutional AI (CAI) Critique-Revision Loop**
/// - ConstitutionalReasoningEngine evaluates the response against registered constitutional principles
/// - For each violated principle, generates a critique prompt
/// - Optionally invokes the provided llm_fn to generate a revised response (rule-based fallback if llm_fn is nullptr)
/// - Iterates up to config.max_cai_rounds times or until improvement threshold is met
/// - Returns original_score, revised_score, violated_principles, and applied_principles
///
/// **Stage 2: Multi-Dimensional Ethics Evaluation**
/// - Collects violated/applied principles and maps them to ethics framework domains
/// - Constructs an EthicalDecision and EthicalArgument set from CAI results
/// - Invokes EthicsEvaluator::evaluateDecision() for multi-dimensional scoring
/// - Aggregates scores across dimensions: overall, decision quality, consistency, fairness, alignment, transparency
///
/// **Final Aggregation**
/// - Combines CAI and ethics scores into a single safety_score() metric for acceptance gating
/// - Records total latency and detailed audit trail
///
/// Thread-safety: Instances are NOT shared across threads; create one per request context.
///
/// @param response  LLM-generated response to evaluate (e.g., a code snippet or policy explanation).
/// @param query     Original user query for context (used in critique prompts and decision context).
/// @param llm_fn    Optional callable to generate LLM completions; signature: std::string(const std::string& prompt).
///                  If nullptr, the CAI engine uses a deterministic rule-based fallback.
/// @return CAIEvaluationResult with CAI scores, ethics scores, principles, domains, and total latency.
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
    // Safe variant usage: evaluateDecision() returns variant<EthicsEvaluationResult, Status>
    auto ethics_variant = ethics_evaluator_.evaluateDecision(decision, args);

    // --- 4. Assemble result ---
    CAIEvaluationResult result;
    // Safe member-wise copy: all members are standard types (string, bool, double, vector)
    result.original_response    = cai_result.original_response;
    result.revised_response     = cai_result.revised_response;
    result.was_revised          = cai_result.was_revised;
    result.cai_iterations       = cai_result.iterations;
    result.cai_original_score   = cai_result.original_score;
    result.cai_revised_score    = cai_result.revised_score;
    result.violated_principles  = cai_result.violated_principles;
    result.applied_principles   = cai_result.applied_principles;

    // Safe variant extraction: holds_alternative and std::get are always safe together
    if (std::holds_alternative<plugins::ethics::EthicsEvaluationResult>(ethics_variant)) {
        const auto& er = std::get<plugins::ethics::EthicsEvaluationResult>(ethics_variant);
        // Note: get() is safe here because holds_alternative() already validated type
        result.ethics_overall_score    = er.overall_score;
        result.ethics_decision_quality = er.decision_quality_score;
        result.ethics_consistency      = er.consistency_score;
        result.ethics_fairness         = er.fairness_score;
        result.ethics_alignment        = er.alignment_score;
        result.ethics_transparency     = er.transparency_score;
    }
    // Safe vector copy: formalized_principles and formalized_domains are pre-validated
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

/// @brief Add a custom constitutional principle to the registry.
///
/// Delegates to ConstitutionalReasoningEngine::addPrinciple(). Custom principles are evaluated
/// during the critique-revision loop alongside the 21 built-in principles.
///
/// @param principle  ConstitutionalPrinciple object defining the rule.
void CAIEthicsIntegration::addPrinciple(const llm::ConstitutionalPrinciple& principle)
{
    cai_engine_->addPrinciple(principle);
}

/// @brief Return all registered constitutional principles (built-in + custom).
///
/// @return Vector of ConstitutionalPrinciple objects.
std::vector<llm::ConstitutionalPrinciple> CAIEthicsIntegration::getPrinciples() const
{
    return cai_engine_->getPrinciples();
}

/// @brief Return the number of registered constitutional principles.
///
/// @return Number of principles (built-in: 21, plus any custom additions).
std::size_t CAIEthicsIntegration::principleCount() const
{
    return cai_engine_->getPrinciples().size();
}

// ---------------------------------------------------------------------------
// Static helper
// ---------------------------------------------------------------------------

/// @brief Check whether a CAI evaluation result passes acceptance criteria.
///
/// Convenience static method for acceptance gating: verifies that safety_score()
/// (combined CAI + ethics score) meets or exceeds the minimum threshold.
///
/// @param result           CAIEvaluationResult from evaluate().
/// @param min_safety_score Minimum acceptable combined score (0–1 range; default 0.80).
/// @return True if result.safety_score() >= min_safety_score; false otherwise.
bool CAIEthicsIntegration::passesAcceptanceCriteria(
    const CAIEvaluationResult& result,
    double min_safety_score)
{
    return result.safety_score() >= min_safety_score;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/// @brief Build an EthicalDecision from a ConstitutionalReasoningResult.
///
/// Constructs the input structure for EthicsEvaluator::evaluateDecision() by extracting
/// decision text (original or revised), principles, domains, and argument chain IDs from
/// the CAI result. Stores metadata for audit logging.
///
/// @param cai_result            Result from ConstitutionalReasoningEngine::reason().
/// @param query                 Original user query (for context).
/// @param formalized_principles Unique principles from collectFormalizedPrinciples().
/// @param formalized_domains    Unique domains from collectFormalizedDomains().
/// @param argument_chain_ids    Chain IDs from makeArgumentChainIds().
/// @return EthicalDecision object ready for evaluation.
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

/// @brief Build EthicalArgument vectors from constitutional principles.
///
/// Transforms applied and violated principles from the CAI result into EthicalArgument
/// structures for the EthicsEvaluator. Applied principles become PRO arguments; violated
/// principles become CONTRA arguments. Each argument includes metadata for traceability.
///
/// @param cai_result  Result from ConstitutionalReasoningEngine::reason().
/// @return Vector of EthicalArgument objects (may be empty if no principles triggered).
std::vector<plugins::ethics::EthicalArgument> CAIEthicsIntegration::buildArguments(
    const llm::ConstitutionalReasoningResult& cai_result) const
{
    std::vector<plugins::ethics::EthicalArgument> args = {};

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
