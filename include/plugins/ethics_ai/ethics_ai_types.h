/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_ai_types.h                                  ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:36:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     228                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
 * @brief Philosophy Profile Definition
 * 
 * Defines a philosophical school with its theses and decision framework.
 */
struct PhilosophyProfile {
    std::string school_id;                              ///< Unique school identifier
    std::string name;                                   ///< Display name
    std::vector<std::string> main_theses;              ///< Core theses
    std::vector<std::string> secondary_theses;         ///< Supporting theses
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
