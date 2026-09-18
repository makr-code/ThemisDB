/**
 * @file ethics_ai_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

enum class ArgumentType {
    PRO,            ///< Argument in favor
    CONTRA,         ///< Argument against
    REBUTTAL,       ///< Rebuttal to another argument
    SYNTHESIS,      ///< Synthesis of multiple arguments
    QUESTION,       ///< Clarifying question
    CLARIFICATION   ///< Clarification statement
};

enum class ArgumentStrength {
    WEAK,       ///< Weak argument
    MODERATE,   ///< Moderate strength
    STRONG,     ///< Strong argument
    DECISIVE    ///< Decisive/conclusive argument
};

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

struct PhilosophyThesis {
    std::string thesis_id;                      ///< Unique thesis identifier (e.g. "kant:kategorischer_imperativ")
    std::string name;                           ///< Short display name
    std::string description;                    ///< Core statement of the thesis

    int token_budget{-1};

    std::vector<int> activation_rounds;

    std::map<std::string, float> round_role_weights;
};

struct PhilosophyProfile {
    std::string school_id;                              ///< Unique school identifier
    std::string name;                                   ///< Display name
    std::vector<std::string> main_theses;              ///< Core theses (plain-text, backward compat)
    std::vector<std::string> secondary_theses;         ///< Supporting theses (plain-text, backward compat)
    std::vector<PhilosophyThesis> typed_theses;
    std::map<std::string, std::string> decision_framework;  ///< Decision-making rules
    std::vector<std::string> strengths;                ///< Philosophical strengths
    std::vector<std::string> weaknesses;               ///< Philosophical weaknesses
    std::map<std::string, std::string> internal_debate;     ///< Internal debate points
    std::map<std::string, std::string> philosophical_positioning;  ///< Positioning relative to others
};

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

struct RAGContext {
    std::vector<std::string> similar_dilemmas;              ///< Similar historical dilemmas
    std::map<std::string, std::vector<std::string>> philosophy_arguments;  ///< Arguments by philosophy
    std::vector<std::string> best_practices;                ///< Best practice examples
    std::vector<std::string> recent_debates;                ///< Recent debate references
    std::vector<std::string> consensus_decisions;           ///< Consensus decisions
    std::map<std::string, double> relevance_scores;        ///< Relevance scores for retrieved items
};

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

struct EthicsEvaluationResult {
    double overall_score = 0;               ///< Overall score (0.0-1.0)
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

struct Status {
    bool ok = 0;
    std::string message = {};
    int code = {};
    
    Status() : ok(true), code(0) {}
    Status(bool ok_, const std::string& msg = "", int code_ = 0) 
        : ok(ok_), message(msg), code(code_) {}
    
    /**
     * @brief OK.
     * @return Return value.
     * @details Calls: Status().
     */
    static Status OK() { return Status(true); }
    static Status Error(const std::string& msg, int code = -1) { 
        return Status(false, msg, code); 
    }
    
    bool isOK() const { return ok; }
    operator bool() const { return ok; }
};

/**
 * @brief Argument Type To String.
 * @param[in] type Input parameter.
 * @return Pointer to the result.
 */
const char* argumentTypeToString(ArgumentType type);
/**
 * @brief String To Argument Type.
 * @param[in] str Input parameter.
 * @return Return value.
 */
ArgumentType stringToArgumentType(const std::string& str);

/**
 * @brief Argument Strength To String.
 * @param[in] strength Input parameter.
 * @return Pointer to the result.
 */
const char* argumentStrengthToString(ArgumentStrength strength);
/**
 * @brief String To Argument Strength.
 * @param[in] str Input parameter.
 * @return Return value.
 */
ArgumentStrength stringToArgumentStrength(const std::string& str);

// ============================================================================
// LDM — Layered Discourse Model types (LDM-1 through LDM-5)
// ============================================================================

// Forward declaration — full definition in ethics_selection_router.h.
// Declared here so MetaVerdict (below) can reference it without introducing
// a circular include chain (ethics_profile_registry.h → ethics_ai_types.h).
enum class DiscourseMode : uint8_t;

enum class DiscourseVerdict : uint8_t {
    PROHIBIT    = 0, ///< School recommends prohibition.
    PERMIT      = 1, ///< School permits the action.
    CONDITIONAL = 2, ///< School permits under stated conditions.
    ABSTAIN     = 3, ///< Fail-closed: LLM timeout or indeterminate.
};

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

struct EthicsError {
    EthicsErrorCode code{EthicsErrorCode::OK};
    std::string     message;

    [[nodiscard]] static EthicsError ok() noexcept {
        return EthicsError{EthicsErrorCode::OK, {}};
    }

    [[nodiscard]] bool isOk() const noexcept {
        return code == EthicsErrorCode::OK;
    }

    [[nodiscard]] explicit operator bool() const noexcept { return !isOk(); }
};

// ============================================================================
// Cross-cultural policy
// ============================================================================

enum class CrossCulturalSensitivity : uint8_t {
    OFF    = 0, ///< Mirror schools disabled.
    LOW    = 1, ///< Activate for explicitly flagged domains only.
    MEDIUM = 2, ///< Activate for bioethics, family_law, end_of_life, minority_rights.
    HIGH   = 3, ///< Activate for all domains including ai_governance and data_protection.
};

struct MirrorSchoolPolicy {
    CrossCulturalSensitivity cross_cultural_sensitivity{CrossCulturalSensitivity::OFF};

    std::vector<std::string> mirror_school_ids{
        "islamische_ethik",
        "konfuzianismus",
        "buddhistische_ethik",
        "juedische_bioethik",
    };

    std::map<std::string, CrossCulturalSensitivity> domain_overrides;

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

    DiscourseVerdict ldm_verdict{DiscourseVerdict::ABSTAIN};

    double initial_weight{0.0};

    bool timed_out{false};
};

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

struct ClusterPosition {
    std::string cluster_name;                ///< e.g. "Deontological"
    std::vector<std::string> school_ids;     ///< Active (non-ABSTAIN) schools in this cluster.
    DiscourseVerdict verdict{DiscourseVerdict::ABSTAIN}; ///< Majority verdict in cluster.
    double confidence{0.0};                  ///< Confidence in [0, 1].
    std::vector<std::string> thesis_ids;     ///< Supporting thesis identifiers.
};

struct LegalGrounding {
    std::vector<std::string> citation_ids;   ///< Document reference IDs from Legal-DB.
    std::vector<std::string> norm_refs;      ///< e.g. {"GG Art. 1", "DSGVO Art. 5"}.
    std::string retrieval_timestamp_utc;     ///< ISO-8601 retrieval timestamp from legal_db query.
    bool override_permitted{false};          ///< From dominant school's regulatory_constraints.
    bool grounding_available{false};         ///< false when Legal-DB is unavailable.
    bool legal_db_unavailable{false};        ///< Explicit availability flag for compliance export.
};

struct NormCitation {
    std::string citation_id;         ///< Stable citation identifier (e.g. "eu-ai-act-art-22").
    std::string article_ref;         ///< Human-readable article reference.
    std::string citation_source;     ///< Source system identifier (e.g. "legal_db").
    std::string retrieved_at_utc;    ///< ISO-8601 retrieval timestamp.
};

struct NormEvidence {
    std::vector<NormCitation> citations; ///< Norm citations relevant for the decision.
    bool legal_db_unavailable{false};    ///< True when legal_db could not be queried.
};

struct MetaVerdictSchoolVote {
    std::string school_id;                                  ///< Participating school identifier.
    DiscourseVerdict vote{DiscourseVerdict::ABSTAIN};      ///< Explicit vote (incl. ABSTAIN).
    std::string reason;                                     ///< Reason (e.g. "unavailable").
};

struct MetaVerdict {
    enum class ConvergenceVerdict : uint8_t {
        CLEAR_CONSENSUS = 0, ///< > 0.75 agreement.
        TENDENCY        = 1, ///< 0.60–0.75 agreement.
        CONTESTED       = 2, ///< 0.40–0.60 agreement.
        DISSENT         = 3, ///< < 0.40 agreement (also: all-ABSTAIN).
    };

    ConvergenceVerdict convergence_verdict{ConvergenceVerdict::DISSENT};
    double             convergence_score{0.0};

    std::vector<std::string> participating_schools;

    std::vector<std::string> dissenting_schools;

    bool cross_cultural_flag{false};

    std::vector<DiscourseRoundOutput> minority_dissent;

    std::vector<MetaVerdictSchoolVote> participating_school_votes;

    NormEvidence norm_evidence;

    LegalGrounding legal_grounding;  ///< Legal-DB citation, or flagged unavailable.

    DiscourseMode discourse_mode{};  // default-initialised; full type via router header.

    DiscourseVerdict dominant_verdict{DiscourseVerdict::ABSTAIN};
};

[[nodiscard]] constexpr MetaVerdict::ConvergenceVerdict
MetaVerdictThreshold(double score) noexcept {
    if (score > 0.75) {
      return MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS;
    }
    if (score > 0.60) {
      return MetaVerdict::ConvergenceVerdict::TENDENCY;
    }
    if (score > 0.40) {
      return MetaVerdict::ConvergenceVerdict::CONTESTED;
    }
    return MetaVerdict::ConvergenceVerdict::DISSENT;
}

// ============================================================================
// EU AI Act Art. 13/22 Audit Infrastructure (Target: Q4 2026)
// ============================================================================

enum class AuditError : int {
    OK                  = 0, ///< No error.
    IMMUTABLE_VIOLATION = 1, ///< Attempt to modify or delete an already-emitted entry.
    INDEX_OUT_OF_RANGE  = 2, ///< Provided index exceeds log size.
};

struct RoundAuditEntry {
    std::string round_id;
    std::string timestamp_utc;
    std::string dilemma_hash;
    std::vector<std::string> participating_schools;
    std::string verdict;
    double convergence_score{0.0};
    std::vector<std::string> norm_citations;
    uint32_t round_index{0};
};

class EthicsAuditLog {
public:
    EthicsAuditLog() = default;

    // Non-copyable; movable.
    EthicsAuditLog(const EthicsAuditLog&)            = delete;
    EthicsAuditLog& operator=(const EthicsAuditLog&) = delete;
    EthicsAuditLog(EthicsAuditLog&&)                 noexcept = default;
    EthicsAuditLog& operator=(EthicsAuditLog&&)      noexcept = default;

    /**
     * @brief Append.
     * @param[in] entry Input parameter.
     * @return Return value.
     * @details Calls: lock(), size(), push_back(), std::move().
     */
    size_t append(RoundAuditEntry entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        entry.round_index = static_cast<uint32_t>(entries_.size());
        entries_.push_back(std::move(entry));
        return entries_.size() - 1u;
    }

    [[nodiscard]] AuditError tryOverwrite(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] const RoundAuditEntry& replacement) const noexcept {
        return AuditError::IMMUTABLE_VIOLATION;
    }

    [[nodiscard]] AuditError tryErase(
        [[maybe_unused]] size_t index) const noexcept {
        return AuditError::IMMUTABLE_VIOLATION;
    }

    [[nodiscard]] std::vector<RoundAuditEntry> exportAuditLog() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_;
    }

    [[nodiscard]] size_t size() const noexcept {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.empty();
    }

private:
    mutable std::mutex          mutex_;
    std::vector<RoundAuditEntry> entries_;
};

// ============================================================================
// LDM-6 — Dynamic Clustering via cross_school_tensions graph (Q1 2027)
// ============================================================================

struct CrossSchoolTensionEdge {
    std::string school_a;    ///< Source school identifier.
    std::string school_b;    ///< Target school identifier.
    double tension_score{0.0};
};

struct CrossSchoolTensionGraph {
    std::vector<std::string> schools;
    std::vector<CrossSchoolTensionEdge> edges;

    [[nodiscard]] double tensionBetween(const std::string& a, const std::string& b) const noexcept {
        for (const auto& e : edges) {
            if ((e.school_a == a && e.school_b == b) ||
                (e.school_a == b && e.school_b == a)) {
                return e.tension_score;
            }
        }
        return 0.0;
    }
};

struct ClusterAssignment {
    std::map<std::string, std::size_t> school_to_cluster;
    std::size_t cluster_count{0};

    [[nodiscard]] std::vector<std::string> schoolsInCluster(std::size_t cluster_index) const {
        std::vector<std::string> result = {};

        for (const auto& [school, idx] : school_to_cluster) {
            if (idx == cluster_index) { result.push_back(school); }
        }
        return result;
    }
};

class DynamicClusteringEngine {
public:
    explicit DynamicClusteringEngine(std::size_t target_cluster_count = 0) noexcept
        : target_cluster_count_(target_cluster_count) {}

    [[nodiscard]] ClusterAssignment cluster(const CrossSchoolTensionGraph& graph) const;

private:
    std::size_t target_cluster_count_;
};

// ============================================================================
// LDM-7 — Māori Ethics & Latin-American Liberation Theology (Q1 2027)
// ============================================================================

namespace LDM7Schools {
    inline constexpr const char* MAORI_ETHICS              = "maori_tikanga";
    inline constexpr const char* LATIN_LIBERATION_THEOLOGY = "befreiungstheologie";
} // namespace LDM7Schools

struct CulturalEthicsSchoolDescriptor {
    std::string school_id;
    std::string display_name;
    std::string cultural_context;
    std::vector<std::string> primary_norm_sources;
    double bias_correction_factor{1.0};
};

// ============================================================================
// LDM-8 — AdaLoRA Adapter for non-western school score-bias correction (Q1 2027)
// ============================================================================

class IAdaLoRABiasCorrector {
public:
    /**
     * @brief IAda Lo RABias Corrector.
     * @return Return value.
     */
    virtual ~IAdaLoRABiasCorrector() = default;

    [[nodiscard]] virtual double applyBiasCorrection(
        const std::string& school_id, double raw_score) const noexcept = 0;

    [[nodiscard]] virtual bool hasAdapter(const std::string& school_id) const noexcept = 0;
};

class IdentityAdaLoRABiasCorrector final : public IAdaLoRABiasCorrector {
public:
    [[nodiscard]] double applyBiasCorrection(
        [[maybe_unused]] const std::string& /*school_id*/,
        double raw_score) const noexcept override {
        return raw_score;
    }

    [[nodiscard]] bool hasAdapter(
        [[maybe_unused]] const std::string& /*school_id*/) const noexcept override {
        return false;
    }
};

class ScalarAdaLoRABiasCorrector final : public IAdaLoRABiasCorrector {
public:
    /**
     * @brief Register Adapter.
     * @param[in] school_id Identifier of the school.
     * @param[in] factor Input parameter.
     * @details Calls: lock().
     */
    void registerAdapter(const std::string& school_id, double factor) {
        std::lock_guard<std::mutex> lock(mutex_);
        adapters_[school_id] = factor;
    }

    [[nodiscard]] double applyBiasCorrection(
        const std::string& school_id, double raw_score) const noexcept override {
        double factor = 1.0;
        {
            /**
             * @brief Lock.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = adapters_.find(school_id);
            if (it != adapters_.end()) { factor = it->second; }
        }
        const double corrected = factor * raw_score;
        if (corrected < 0.0) {
          return 0.0;
        }
        if (corrected > 1.0) {
          return 1.0;
        }
        return corrected;
    }

    [[nodiscard]] bool hasAdapter(const std::string& school_id) const noexcept override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return adapters_.count(school_id) != 0u;
    }

private:
    mutable std::mutex            mutex_;
    std::map<std::string, double> adapters_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
