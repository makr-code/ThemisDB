/**
 * @file ethics_ai_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: ethics_ai_types.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 296
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Argument Type Classification
 */
enum class ArgumentType {
    PRO,            ///< Argument in favor
    CONTRA,         ///< Argument against
    REBUTTAL,       ///< Rebuttal to another argument
    SYNTHESIS,      ///< Synthesis of multiple arguments
    QUESTION,       ///< Clarifying question
    CLARIFICATION   ///< Clarification statement
};

/**
 * @brief Argument Strength Assessment
 */
enum class ArgumentStrength {
    WEAK,       ///< Weak argument
    MODERATE,   ///< Moderate strength
    STRONG,     ///< Strong argument
    DECISIVE    ///< Decisive/conclusive argument
};

/**
 * @brief Ethical Argument Data Structure
 * 
 * Represents a single ethical argument in a philosophical debate.
 */
struct EthicalArgument {
    std::string id;                             ///< Unique identifier
    std::string philosophy_school;              ///< Philosophy school (e.g., "kant", "utilitarianism")
    ArgumentType argument_type;                 ///< Type of argument
    std::string content;                        ///< The actual argument text
    std::vector<std::string> principle_basis;   ///< Core philosophical principles invoked
    ArgumentStrength strength;                  ///< Assessed strength
    std::vector<std::string> counterarguments;  ///< IDs of countering arguments
    std::vector<std::string> supports;          ///< IDs of supported arguments
    std::chrono::system_clock::time_point created_at;  ///< Creation timestamp
    std::map<std::string, std::string> metadata;       ///< Additional metadata
    
    EthicalArgument() 
        : argument_type(ArgumentType::PRO)
        , strength(ArgumentStrength::MODERATE)
        , created_at(std::chrono::system_clock::now()) 
    {}
};

/**
 * @brief Argument Chain for Dialectical Reasoning
 * 
 * Represents a chain of arguments in dialectical reasoning.
 */
struct ArgumentChain {
    std::string id;                         ///< Chain identifier
    std::string dilemma_id;                 ///< Associated dilemma
    std::vector<std::string> argument_ids;  ///< Ordered list of argument IDs
    std::string chain_type;                 ///< Type (e.g., "pro", "contra", "synthesis")
    double coherence_score;                 ///< Chain coherence (0.0-1.0)
    std::chrono::system_clock::time_point created_at;
    
    ArgumentChain() 
        : coherence_score(0.0)
        , created_at(std::chrono::system_clock::now())
    {}
};

/**
 * @brief Per-thesis budget and activation metadata.
 *
 * Each thesis in a philosophy profile may declare how many tokens it may
 * consume in a discourse context window and during which rounds it is fully
 * active.  These fields are optional (defaults: unlimited / all rounds) so
 * that profiles without them continue to behave exactly as before.
 */
struct PhilosophyThesis {
    std::string thesis_id;                      ///< Unique thesis identifier (e.g. "kant:kategorischer_imperativ")
    std::string name;                           ///< Short display name
    std::string description;                    ///< Core statement of the thesis

    /// Maximum tokens to inject for this thesis in the LLM context.
    /// -1 = unlimited (default, backward compatible).
    int token_budget{-1};

    /// Discourse rounds (1–5) in which this thesis is fully injected.
    /// Empty = active in all rounds (default, backward compatible).
    std::vector<int> activation_rounds;

    /// Per-round-role priority weights.  Key = role name (e.g. "PRO",
    /// "REBUTTAL", "SYNTHESIS"), value = weight in [0, 1].  Higher weight
    /// → selected earlier when budget is tight.
    std::map<std::string, float> round_role_weights;
};

/**
 * @brief Philosophy Profile Definition
 * 
 * Defines a philosophical school with its theses and decision framework.
 */
struct PhilosophyProfile {
    std::string school_id;                              ///< Unique school identifier
    std::string name;                                   ///< Display name
    std::vector<std::string> main_theses;              ///< Core theses (plain-text, backward compat)
    std::vector<std::string> secondary_theses;         ///< Supporting theses (plain-text, backward compat)
    /// Typed thesis objects parsed from YAML; populated when YAML theses are
    /// complex objects (with thesis_id field).  Plain-string theses in
    /// `main_theses` / `secondary_theses` are NOT duplicated here.
    std::vector<PhilosophyThesis> typed_theses;
    std::map<std::string, std::string> decision_framework;  ///< Decision-making rules
    std::vector<std::string> strengths;                ///< Philosophical strengths
    std::vector<std::string> weaknesses;               ///< Philosophical weaknesses
    std::map<std::string, std::string> internal_debate;     ///< Internal debate points
    std::map<std::string, std::string> philosophical_positioning;  ///< Positioning relative to others
};

/**
 * @brief Ethical Decision Result
 * 
 * Represents the outcome of an ethical decision-making process.
 */
struct EthicalDecision {
    std::string decision_id;                    ///< Decision identifier
    std::string dilemma_id;                     ///< Associated dilemma
    std::string decision_text;                  ///< The decision text
    std::string primary_philosophy;             ///< Primary philosophy used
    std::vector<std::string> supporting_philosophies;  ///< Supporting philosophies
    std::vector<std::string> argument_chain_ids;       ///< Argument chains used
    double confidence;                          ///< Confidence score (0.0-1.0)
    double consensus_level;                     ///< Multi-philosophy consensus (0.0-1.0)
    std::chrono::system_clock::time_point created_at;
    std::map<std::string, std::string> metadata;
    
    EthicalDecision()
        : confidence(0.0)
        , consensus_level(0.0)
        , created_at(std::chrono::system_clock::now())
    {}
};

/**
 * @brief RAG Context for Ethical Reasoning
 * 
 * Contains retrieved context for ethical decision-making.
 */
struct RAGContext {
    std::vector<std::string> similar_dilemmas;              ///< Similar historical dilemmas
    std::map<std::string, std::vector<std::string>> philosophy_arguments;  ///< Arguments by philosophy
    std::vector<std::string> best_practices;                ///< Best practice examples
    std::vector<std::string> recent_debates;                ///< Recent debate references
    std::vector<std::string> consensus_decisions;           ///< Consensus decisions
    std::map<std::string, double> relevance_scores;        ///< Relevance scores for retrieved items
};

/**
 * @brief Debate Round
 *
 * Represents one round of counter-arguments in a multi-round debate.
 * Each philosophy school responds to arguments generated in previous rounds.
 */
struct DebateRound {
    std::string debate_id;                        ///< Parent debate identifier
    int round_number;                             ///< Round index (1-based)
    std::vector<EthicalArgument> arguments;       ///< Arguments produced in this round
    std::chrono::system_clock::time_point created_at;

    DebateRound()
        : round_number(0)
        , created_at(std::chrono::system_clock::now())
    {}
};

/**
 * @brief Debate Initialization Data
 * 
 * Contains initial setup for an ethical debate.
 */
struct DebateInitialization {
    std::string debate_id;                      ///< Debate identifier
    std::string dilemma_description;            ///< Description of the dilemma
    std::vector<std::string> philosophy_schools;  ///< Participating philosophies
    std::string category;                       ///< Dilemma category
    std::map<std::string, std::string> context;  ///< Additional context
    std::chrono::system_clock::time_point created_at;
    
    DebateInitialization()
        : created_at(std::chrono::system_clock::now())
    {}
};

/**
 * @brief Ethics Evaluation Result (5 Dimensions)
 * 
 * Contains evaluation metrics across 5 key dimensions.
 */
struct EthicsEvaluationResult {
    double overall_score;               ///< Overall score (0.0-1.0)
    double decision_quality_score;      ///< Decision quality dimension
    double consistency_score;           ///< Consistency dimension
    double fairness_score;              ///< Fairness dimension
    double alignment_score;             ///< Alignment dimension
    double transparency_score;          ///< Transparency dimension
    std::map<std::string, double> detailed_metrics;  ///< Detailed sub-metrics
    
    EthicsEvaluationResult()
        : overall_score(0.0)
        , decision_quality_score(0.0)
        , consistency_score(0.0)
        , fairness_score(0.0)
        , alignment_score(0.0)
        , transparency_score(0.0)
    {}
};

/**
 * @brief Status/Error type for operations
 */
struct Status {
    bool ok;
    std::string message;
    int code;
    
    Status() : ok(true), code(0) {}
    Status(bool ok_, const std::string& msg = "", int code_ = 0) 
        : ok(ok_), message(msg), code(code_) {}
    
    static Status OK() { return Status(true); }
    static Status Error(const std::string& msg, int code = -1) { 
        return Status(false, msg, code); 
    }
    
    bool isOK() const { return ok; }
    operator bool() const { return ok; }
};

// Helper functions for enum conversions
const char* argumentTypeToString(ArgumentType type);
ArgumentType stringToArgumentType(const std::string& str);

const char* argumentStrengthToString(ArgumentStrength strength);
ArgumentStrength stringToArgumentStrength(const std::string& str);

// ============================================================================
// LDM — Layered Discourse Model types (LDM-1 through LDM-5)
// ============================================================================

// Forward declaration — full definition in ethics_selection_router.h.
// Declared here so MetaVerdict (below) can reference it without introducing
// a circular include chain (ethics_profile_registry.h → ethics_ai_types.h).
enum class DiscourseMode : uint8_t;

/**
 * @brief Per-school Ebene-1 verdict produced by the Layered Discourse Model.
 *
 * @note ABSTAIN is the fail-closed verdict assigned when an LLM call times out
 *       for a given school.  The school is still included in
 *       MetaVerdict::participating_schools for EU AI Act Art. 13 compliance.
 *
 * @since LDM-2 (Target: Q1 2027)
 */
enum class DiscourseVerdict : uint8_t {
    PROHIBIT    = 0, ///< School recommends prohibition.
    PERMIT      = 1, ///< School permits the action.
    CONDITIONAL = 2, ///< School permits under stated conditions.
    ABSTAIN     = 3, ///< Fail-closed: LLM timeout or indeterminate.
};

/**
 * @brief Error taxonomy for ethics_ai LDM operations.
 *
 * Use EthicsError::ok() to construct a no-error value.
 *
 * @since LDM-1 (Target: Q4 2026)
 */
enum class EthicsErrorCode : int {
    OK                          = 0,
    PROFILE_NOT_FOUND           = 1,  ///< Requested school_id absent from registry.
    PROFILE_VALIDATION_FAILED   = 2,  ///< Profile loaded but failed schema check.
    PROFILE_SCHEMA_INVALID      = 3,  ///< YAML schema does not conform to contract.
    LIFECYCLE_UNINITIALIZED     = 10, ///< Plugin not yet initialised.
    LIFECYCLE_DOUBLE_INIT       = 11, ///< Plugin initialise() called twice.
    CONTEXT_RETRIEVAL_FAILED    = 20, ///< RAG context retrieval failure.
    CONTEXT_SCHEMA_INVALID      = 21, ///< Retrieved context does not match schema.
    ROUTING_NO_SCHOOLS          = 30, ///< Router returned zero candidate schools.
    ROUTING_EMPTY_PLAN          = 31, ///< planDiscourse() produced an empty plan.
    EVALUATOR_SCORE_OUT_OF_RANGE= 40, ///< Score outside [0, 1] contract.
    LDM_LLM_TIMEOUT             = 50, ///< Per-school LLM call timed out → ABSTAIN.
    LDM_ALL_ABSTAINED           = 51, ///< All schools ABSTAINED → DISSENT MetaVerdict.
    LDM_LEGAL_DB_UNAVAILABLE    = 52, ///< Legal-DB offline; MetaVerdict without grounding.
    LDM_CLUSTER_EMPTY           = 53, ///< Cluster has 0 active (non-ABSTAIN) schools.
    LDM_EQUAL_WEIGHT_VIOLATION  = 54, ///< Process-integrity audit event: unequal weights.
};

/**
 * @brief Typed error value for ethics_ai LDM operations.
 *
 * Prefer returning `EthicsError` over throwing exceptions in non-fatal paths.
 *
 * @since LDM-1 (Target: Q4 2026)
 */
struct EthicsError {
    EthicsErrorCode code{EthicsErrorCode::OK};
    std::string     message;

    /// @return A no-error value.
    [[nodiscard]] static EthicsError ok() noexcept {
        return EthicsError{EthicsErrorCode::OK, {}};
    }

    /// @return True when this represents a successful (no-error) state.
    [[nodiscard]] bool isOk() const noexcept {
        return code == EthicsErrorCode::OK;
    }

    /// @return True when this represents an error.
    [[nodiscard]] explicit operator bool() const noexcept { return !isOk(); }
};

// ============================================================================
// Cross-cultural policy
// ============================================================================

/**
 * @brief Activation level for the Mirror-School cross-cultural perspective mode.
 *
 * Higher levels activate more non-western mirror schools per domain.
 *
 * @since LDM-5 (Target: Q2 2027)
 */
enum class CrossCulturalSensitivity : uint8_t {
    OFF    = 0, ///< Mirror schools disabled.
    LOW    = 1, ///< Activate for explicitly flagged domains only.
    MEDIUM = 2, ///< Activate for bioethics, family_law, end_of_life, minority_rights.
    HIGH   = 3, ///< Activate for all domains including ai_governance and data_protection.
};

/**
 * @brief Per-domain Mirror-School activation policy.
 *
 * Controls which non-western schools run in lightweight parallel-mirror mode
 * alongside Ebene-2 cluster discourse.
 *
 * @note Mirror-school output is ALWAYS persisted in MetaVerdict::minority_dissent
 *       regardless of the overall convergence_score (EU AI Act Art. 13).
 *
 * @since LDM-5 (Target: Q2 2027)
 */
struct MirrorSchoolPolicy {
    /// Global cross-cultural sensitivity level.  Defaults to OFF.
    CrossCulturalSensitivity cross_cultural_sensitivity{CrossCulturalSensitivity::OFF};

    /// Default non-western mirror school identifiers.
    std::vector<std::string> mirror_school_ids{
        "islamische_ethik",
        "konfuzianismus",
        "buddhistische_ethik",
        "juedische_bioethik",
    };

    /// Per-domain sensitivity overrides (domain → activation level).
    std::map<std::string, CrossCulturalSensitivity> domain_overrides;

    /**
     * @brief Return true when the mirror-school mode is active for @p domain.
     *
     * Lookup order:
     * 1. Domain-specific override if present.
     * 2. Global `cross_cultural_sensitivity` if ≥ LOW.
     *
     * @param domain  Dilemma domain key, e.g. "bioethics".
     * @return true   when at least one mirror school should be activated.
     */
    [[nodiscard]] bool isActiveFor(const std::string& domain) const noexcept {
        auto it = domain_overrides.find(domain);
        if (it != domain_overrides.end()) {
            return it->second != CrossCulturalSensitivity::OFF;
        }
        return cross_cultural_sensitivity != CrossCulturalSensitivity::OFF;
    }
};

// ============================================================================
// LDM output types
// ============================================================================

/**
 * @brief Output record for a single school in one discourse round.
 *
 * Produced by EthicalDiscourseEngine::runRound() (legacy fields) and by
 * DiscourseOrchestrator::runEbene1() (LDM fields).  The `position_abstract`
 * field implements the DSPy TypedPredictor-equivalent output schema (§12.2.3).
 *
 * LDM additions (fields suffixed @since LDM-2):
 * - `ldm_verdict`    — typed enum version of `verdict` for LDM code paths.
 * - `initial_weight` — w₀ = 1/N equal-weight contract value, filled by orchestrator.
 * - `timed_out`      — true when the per-school LLM call exceeded the timeout;
 *                      forces `ldm_verdict = ABSTAIN` (fail-closed).
 */
struct DiscourseRoundOutput {
    std::string  school_id;
    int          round_number{0};
    std::string  content;                          ///< Full argument text
    std::string  verdict;                          ///< "PROHIBIT"|"PERMIT"|"CONDITIONAL"|"ABSTAIN"
    float        confidence{0.0f};
    std::vector<std::string> core_thesis_ids;      ///< ≤ 3 thesis_ids
    std::string  primary_rebuttal_of;              ///< thesis_id rebutted (R2+)
    std::string  position_abstract;               ///< ≤ 100 tokens — §12.2.3
    bool         schema_valid{false};

    // --- LDM-2 additions ---

    /// Typed Ebene-1 verdict (LDM code paths).  Populated by DiscourseOrchestrator.
    DiscourseVerdict ldm_verdict{DiscourseVerdict::ABSTAIN};

    /// Equal initial weight w₀ = 1/N, filled by DiscourseOrchestrator.
    /// @since LDM-2
    double initial_weight{0.0};

    /// True when the per-school LLM call timed out (forced ABSTAIN, fail-closed).
    /// @since LDM-2
    bool timed_out{false};
};

/**
 * @brief Episodic memory entry for REFLEXION-based memory externalization.
 *
 * Implements the MemGPT Recall Storage pattern (§12.2.4).
 *
 * LDM-3 additions: inter-cluster tension-pair fields (`cluster_a`, `cluster_b`,
 * `tension_axis`, `outcome_summary`, `round_number`).  Legacy per-school fields
 * remain unchanged.
 */
struct EpisodicMemoryEntry {
    // --- Legacy per-school fields (§12.2.4) ---
    std::string school_id;
    int         from_round{0};
    std::string compressed_position;  ///< ≤ 50 tokens
    float       dc_score{0.0f};
    std::string strongest_tension;    ///< thesis_id pair "own:thesis ↔ opponent:thesis"

    // --- LDM-3 inter-cluster fields ---
    std::string cluster_a;       ///< First cluster in inter-cluster tension pair.
    std::string cluster_b;       ///< Second cluster in inter-cluster tension pair.
    std::string tension_axis;    ///< e.g. "Kant↔Utilitarismus"
    std::string outcome_summary; ///< Brief summary of the inter-cluster discourse outcome.
    int         round_number{0}; ///< Ebene-2 discourse round (LDM-3).
};

/**
 * @brief Ebene-2 per-cluster consolidated position.
 *
 * Produced by DiscourseOrchestrator::runEbene2() for each cluster after
 * intra-cluster consolidation.
 *
 * @since LDM-3 (Target: Q2 2027)
 */
struct ClusterPosition {
    std::string cluster_name;                ///< e.g. "Deontological"
    std::vector<std::string> school_ids;     ///< Active (non-ABSTAIN) schools in this cluster.
    DiscourseVerdict verdict{DiscourseVerdict::ABSTAIN}; ///< Majority verdict in cluster.
    double confidence{0.0};                  ///< Confidence in [0, 1].
    std::vector<std::string> thesis_ids;     ///< Supporting thesis identifiers.
};

/**
 * @brief Legal-DB citation grounding for MetaVerdict.
 *
 * Populated from the Legal-DB retriever (CitationHighlighter) — NEVER from
 * LLM-generated text.  When the Legal-DB is offline, `grounding_available`
 * is set to false and the MetaVerdict is produced without grounding.
 *
 * @since LDM-4 (Target: Q2 2027)
 */
struct LegalGrounding {
    std::vector<std::string> citation_ids;   ///< Document reference IDs from Legal-DB.
    std::vector<std::string> norm_refs;      ///< e.g. {"GG Art. 1", "DSGVO Art. 5"}.
    std::string retrieval_timestamp_utc;     ///< ISO-8601 retrieval timestamp from legal_db query.
    bool override_permitted{false};          ///< From dominant school's regulatory_constraints.
    bool grounding_available{false};         ///< false when Legal-DB is unavailable.
    bool legal_db_unavailable{false};        ///< Explicit availability flag for compliance export.
};

/**
 * @brief One legal citation entry used for Art. 22 explainability evidence.
 */
struct NormCitation {
    std::string citation_id;         ///< Stable citation identifier (e.g. "eu-ai-act-art-22").
    std::string article_ref;         ///< Human-readable article reference.
    std::string citation_source;     ///< Source system identifier (e.g. "legal_db").
    std::string retrieved_at_utc;    ///< ISO-8601 retrieval timestamp.
};

/**
 * @brief Norm evidence bundle attached to each ethics decision.
 */
struct NormEvidence {
    std::vector<NormCitation> citations; ///< Norm citations relevant for the decision.
    bool legal_db_unavailable{false};    ///< True when legal_db could not be queried.
};

/**
 * @brief Explicit per-school vote entry required for Art. 13 completeness exports.
 */
struct MetaVerdictSchoolVote {
    std::string school_id;                                  ///< Participating school identifier.
    DiscourseVerdict vote{DiscourseVerdict::ABSTAIN};      ///< Explicit vote (incl. ABSTAIN).
    std::string reason;                                     ///< Reason (e.g. "unavailable").
};

/**
 * @brief Ebene-3 convergence-counting MetaVerdict (EU AI Act Art. 13 compliant).
 *
 * Produced by MetaVerdictBuilder::buildMetaVerdict() after Ebene-1 and,
 * optionally, Ebene-2 cluster discourse.
 *
 * Convergence thresholds:
 * | convergence_score | convergence_verdict |
 * |-------------------|---------------------|
 * | > 0.75            | CLEAR_CONSENSUS     |
 * | 0.60–0.75         | TENDENCY            |
 * | 0.40–0.60         | CONTESTED           |
 * | < 0.40            | DISSENT             |
 *
 * @note `participating_schools` MUST include ALL N schools (including ABSTAIN)
 *       for EU AI Act Art. 13 audit completeness.
 *
 * @since LDM-4 (Target: Q2 2027)
 */
struct MetaVerdict {
    /**
     * @brief Convergence level for the dominant verdict.
     */
    enum class ConvergenceVerdict : uint8_t {
        CLEAR_CONSENSUS = 0, ///< > 0.75 agreement.
        TENDENCY        = 1, ///< 0.60–0.75 agreement.
        CONTESTED       = 2, ///< 0.40–0.60 agreement.
        DISSENT         = 3, ///< < 0.40 agreement (also: all-ABSTAIN).
    };

    ConvergenceVerdict convergence_verdict{ConvergenceVerdict::DISSENT};
    double             convergence_score{0.0};

    /// ALL N participating schools, including ABSTAIN votes (EU AI Act Art. 13).
    std::vector<std::string> participating_schools;

    /// Schools whose verdict differed from the dominant verdict.
    std::vector<std::string> dissenting_schools;

    /// True when ≥ 2 schools from distinct cultural regions share the same verdict.
    bool cross_cultural_flag{false};

    /// Mirror-school outputs, always populated when MirrorSchoolPolicy is active.
    /// Visible in audit trail regardless of convergence_score.
    std::vector<DiscourseRoundOutput> minority_dissent;

    /// Explicit per-school votes including ABSTAIN entries and reasons.
    std::vector<MetaVerdictSchoolVote> participating_school_votes;

    /// Structured norm evidence used for Art. 22 explainability exports.
    NormEvidence norm_evidence;

    LegalGrounding legal_grounding;  ///< Legal-DB citation, or flagged unavailable.

    /// Active discourse mode that produced this verdict.
    DiscourseMode discourse_mode{};  // default-initialised; full type via router header.

    DiscourseVerdict dominant_verdict{DiscourseVerdict::ABSTAIN};
};

/**
 * @brief Static helper: map a convergence_score to a ConvergenceVerdict.
 *
 * Thresholds:
 * - score > 0.75  → CLEAR_CONSENSUS
 * - score in (0.60, 0.75] → TENDENCY
 * - score in (0.40, 0.60] → CONTESTED
 * - score ≤ 0.40  → DISSENT
 *
 * @param score  Convergence score in [0, 1].
 * @return Corresponding ConvergenceVerdict.
 *
 * @since LDM-4 (Target: Q2 2027)
 */
[[nodiscard]] constexpr MetaVerdict::ConvergenceVerdict
MetaVerdictThreshold(double score) noexcept {
    if (score > 0.75) return MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS;
    if (score > 0.60) return MetaVerdict::ConvergenceVerdict::TENDENCY;
    if (score > 0.40) return MetaVerdict::ConvergenceVerdict::CONTESTED;
    return MetaVerdict::ConvergenceVerdict::DISSENT;
}

// ============================================================================
// EU AI Act Art. 13/22 Audit Infrastructure (Target: Q4 2026)
// ============================================================================

/**
 * @brief Error codes for audit-log operations.
 *
 * Distinct from EthicsErrorCode to allow precise pattern matching on
 * audit-specific failure modes (EU AI Act Art. 13 immutability contract).
 *
 * @since LDM-6 (Target: Q4 2026)
 */
enum class AuditError : int {
    OK                  = 0, ///< No error.
    IMMUTABLE_VIOLATION = 1, ///< Attempt to modify or delete an already-emitted entry.
    INDEX_OUT_OF_RANGE  = 2, ///< Provided index exceeds log size.
};

/**
 * @brief Structured audit-log entry per discourse round (EU AI Act Art. 13).
 *
 * Emitted atomically by DiscourseOrchestrator after each Ebene-1/3 run.
 * Once appended to EthicsAuditLog, entries MUST NOT be modified (immutability
 * contract required by Art. 13).
 *
 * JSON schema:
 * @code
 * {
 *   "round_id": "<str>",
 *   "timestamp_utc": "<ISO-8601>",
 *   "dilemma_hash": "<hex-str>",
 *   "participating_schools": ["<school_id>", ...],
 *   "verdict": "<str>",
 *   "convergence_score": 0.0,
 *   "norm_citations": ["<norm_ref>", ...]
 * }
 * @endcode
 *
 * @since LDM-6 (Target: Q4 2026)
 */
struct RoundAuditEntry {
    /// Unique round identifier (monotonically increasing, e.g. "round-001").
    std::string round_id;
    /// ISO-8601 UTC timestamp at emission (e.g. "2026-08-09T17:31:17Z").
    std::string timestamp_utc;
    /// FNV-1a or SHA-256 hex hash of the dilemma text for cross-reference.
    std::string dilemma_hash;
    /// ALL N schools that participated, including those that voted ABSTAIN.
    std::vector<std::string> participating_schools;
    /// Dominant verdict as string (e.g. "PROHIBIT", "PERMIT", "ABSTAIN").
    std::string verdict;
    /// Convergence score in [0.0, 1.0].
    double convergence_score{0.0};
    /// Applicable norm citations (e.g. "GG Art. 1", "EU AI Act Art. 22").
    std::vector<std::string> norm_citations;
    /// Chronological index within the log (0-based, set by EthicsAuditLog::append).
    uint32_t round_index{0};
};

/**
 * @brief Append-only, thread-safe audit log for discourse rounds.
 *
 * Implements the EU AI Act Art. 13 immutability requirement: entries may only
 * be appended, never modified or erased after emission. Any attempt to overwrite
 * or erase an entry returns AuditError::IMMUTABLE_VIOLATION.
 *
 * ### Usage
 * @code
 * EthicsAuditLog log;
 * RoundAuditEntry e;
 * e.round_id = "round-001";
 * e.verdict  = "PROHIBIT";
 * log.append(std::move(e));
 *
 * auto snapshot = log.exportAuditLog();  // chronological copy
 * @endcode
 *
 * @since LDM-6 (Target: Q4 2026)
 */
class EthicsAuditLog {
public:
    EthicsAuditLog() = default;

    // Non-copyable; movable.
    EthicsAuditLog(const EthicsAuditLog&)            = delete;
    EthicsAuditLog& operator=(const EthicsAuditLog&) = delete;
    EthicsAuditLog(EthicsAuditLog&&)                 = default;
    EthicsAuditLog& operator=(EthicsAuditLog&&)      = default;

    /**
     * @brief Append a new immutable audit entry.
     *
     * Sets `entry.round_index` to the current size before insertion.
     * Thread-safe.
     *
     * @param entry  Entry to append (moved into the log).
     * @return Zero-based index of the newly appended entry.
     */
    size_t append(RoundAuditEntry entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        entry.round_index = static_cast<uint32_t>(entries_.size());
        entries_.push_back(std::move(entry));
        return entries_.size() - 1u;
    }

    /**
     * @brief Attempt to overwrite an existing entry.
     *
     * Always returns AuditError::IMMUTABLE_VIOLATION — the log is append-only.
     * This method exists solely to provide an explicit, testable rejection path
     * for the EU AI Act Art. 13 immutability contract.
     *
     * @param index        Ignored (any value).
     * @param replacement  Ignored.
     * @return AuditError::IMMUTABLE_VIOLATION unconditionally.
     */
    [[nodiscard]] AuditError tryOverwrite(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] const RoundAuditEntry& replacement) const noexcept {
        return AuditError::IMMUTABLE_VIOLATION;
    }

    /**
     * @brief Attempt to erase an existing entry.
     *
     * Always returns AuditError::IMMUTABLE_VIOLATION — the log is append-only.
     *
     * @param index  Ignored (any value).
     * @return AuditError::IMMUTABLE_VIOLATION unconditionally.
     */
    [[nodiscard]] AuditError tryErase(
        [[maybe_unused]] size_t index) const noexcept {
        return AuditError::IMMUTABLE_VIOLATION;
    }

    /**
     * @brief Export all entries in chronological order (by round_index).
     *
     * Returns a value-copy snapshot; callers may not modify the log through
     * the returned vector.  Thread-safe.
     *
     * @return Copy of all audit entries in insertion order.
     */
    [[nodiscard]] std::vector<RoundAuditEntry> exportAuditLog() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_;
    }

    /// @return Number of entries in the log.
    [[nodiscard]] size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    /// @return True when no entries have been appended yet.
    [[nodiscard]] bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.empty();
    }

private:
    mutable std::mutex          mutex_;
    std::vector<RoundAuditEntry> entries_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
