/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_ai_types.h                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     228                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
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

} // namespace ethics
} // namespace plugins
} // namespace themis
