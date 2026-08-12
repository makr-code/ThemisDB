/**
 * @file ethics_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace query {
namespace functions {

/**
 * @brief Ethics AI Functions for AQL
 * 
 * This module provides AQL functions for ethical decision-making and analysis:
 * 
 * ## Decision Making
 * - ETHICS_MAKE_DECISION(dilemma, philosophies, category, use_rag) -> Make ethical decision
 * - ETHICS_INITIALIZE_DEBATE(dilemma, philosophies, category) -> Start debate session
 * 
 * ## Evaluation
 * - ETHICS_EVALUATE(decision, arguments) -> Evaluate decision across 5 dimensions
 * - ETHICS_EVALUATE_DIMENSION(decision, dimension) -> Evaluate specific dimension
 * 
 * ## Argument Management
 * - ETHICS_GET_ARGUMENTS(philosophy, type, limit) -> Retrieve arguments
 * - ETHICS_FIND_SIMILAR_DILEMMAS(query, threshold, limit) -> Vector similarity search
 * - ETHICS_TRAVERSE_CHAIN(start_id, max_depth) -> Graph traversal of argument chains
 * 
 * ## Philosophy
 * - ETHICS_LOAD_PROFILE(school) -> Load philosophy profile
 * - ETHICS_LIST_SCHOOLS() -> List available schools
 * 
 * ## RAG Context
 * - ETHICS_BUILD_CONTEXT(dilemma, philosophies, category) -> Build RAG context
 * 
 * ## Statistics
 * - ETHICS_STATS(philosophy) -> Get philosophy statistics
 * - ETHICS_METRICS() -> Get system metrics (Prometheus format)
 */

// ============================================================================
// Decision Making Functions
// ============================================================================

/**
 * @brief ETHICS_MAKE_DECISION - Make ethical decision using multiple philosophies
 * 
 * @code
 * LET decision = ETHICS_MAKE_DECISION(
 *   "Should autonomous vehicles prioritize passenger safety?",
 *   ["kant", "utilitarianism", "virtue_ethics"],
 *   "autonomous_systems",
 *   true
 * )
 * 
 * RETURN {
 *   decision: decision.decision_text,
 *   confidence: decision.confidence,
 *   primary_philosophy: decision.primary_philosophy
 * }
 * @endcode
 */
class EthicsMakeDecisionFunction : public IFunction {
public:
    ~EthicsMakeDecisionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_MAKE_DECISION",
            "Ethics",
            "Make ethical decision using multi-philosophy analysis with optional RAG context",
            {
                {"dilemma_description", ArgType::STRING, true, nullptr, 
                 "Description of the ethical dilemma"},
                {"philosophy_schools", ArgType::ARRAY, true, nullptr,
                 "List of philosophy schools to consult (e.g., ['kant', 'utilitarianism'])"},
                {"category", ArgType::STRING, false, "general",
                 "Dilemma category: general, bioethics, autonomous_systems, data_ethics, etc."},
                {"use_rag", ArgType::BOOLEAN, false, true,
                 "Whether to use RAG context for enhanced decision-making"}
            },
            ArgType::OBJECT,
            true,   // deterministic
            false,  // not aggregate
            {
                "ETHICS_MAKE_DECISION('Should AI prioritize privacy?', ['kant', 'utilitarianism'], 'data_ethics', true)",
                "ETHICS_MAKE_DECISION(doc.dilemma, ['virtue_ethics'], 'general', false)"
            },
            {CostComplexity::LINEAR, 100.0, 20.0, true, true, "ethics_arguments"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief ETHICS_INITIALIZE_DEBATE - Initialize ethical debate session
 * 
 * @code
 * LET debate = ETHICS_INITIALIZE_DEBATE(
 *   "Resource allocation in healthcare crisis",
 *   ["kant", "utilitarianism", "care_ethics"],
 *   "healthcare"
 * )
 * 
 * RETURN debate.debate_id
 * @endcode
 */
class EthicsInitializeDebateFunction : public IFunction {
public:
    ~EthicsInitializeDebateFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_INITIALIZE_DEBATE",
            "Ethics",
            "Initialize a multi-philosophy ethical debate session",
            {
                {"dilemma_description", ArgType::STRING, true, nullptr,
                 "Description of the ethical dilemma"},
                {"philosophy_schools", ArgType::ARRAY, true, nullptr,
                 "List of philosophy schools to participate"},
                {"category", ArgType::STRING, false, "general",
                 "Dilemma category"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "ETHICS_INITIALIZE_DEBATE('Should we allow gene editing?', ['kant', 'utilitarianism'], 'bioethics')"
            },
            {CostComplexity::LINEAR, 50.0, 10.0, true, true, "ethics_debates"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Evaluation Functions
// ============================================================================

/**
 * @brief ETHICS_EVALUATE - Evaluate decision across 5 dimensions
 * 
 * @code
 * LET decision = ETHICS_MAKE_DECISION(...)
 * LET eval = ETHICS_EVALUATE(decision, [])
 * 
 * RETURN {
 *   overall: eval.overall_score,
 *   quality: eval.decision_quality_score,
 *   consistency: eval.consistency_score,
 *   fairness: eval.fairness_score,
 *   alignment: eval.alignment_score,
 *   transparency: eval.transparency_score
 * }
 * @endcode
 */
class EthicsEvaluateFunction : public IFunction {
public:
    ~EthicsEvaluateFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_EVALUATE",
            "Ethics",
            "Evaluate ethical decision quality across 5 dimensions: quality, consistency, fairness, alignment, transparency",
            {
                {"decision", ArgType::OBJECT, true, nullptr,
                 "Decision object to evaluate"},
                {"arguments", ArgType::ARRAY, false, nlohmann::json::array(),
                 "Optional list of arguments used in decision"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "ETHICS_EVALUATE(decision, [])",
                "ETHICS_EVALUATE(decision, arguments)"
            },
            {CostComplexity::LINEAR, 30.0, 5.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief ETHICS_EVALUATE_DIMENSION - Evaluate specific dimension
 * 
 * @code
 * LET fairness = ETHICS_EVALUATE_DIMENSION(decision, "fairness")
 * RETURN fairness
 * @endcode
 */
class EthicsEvaluateDimensionFunction : public IFunction {
public:
    ~EthicsEvaluateDimensionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_EVALUATE_DIMENSION",
            "Ethics",
            "Evaluate a specific dimension of an ethical decision",
            {
                {"decision", ArgType::OBJECT, true, nullptr, "Decision object"},
                {"dimension", ArgType::STRING, true, nullptr,
                 "Dimension: decision_quality, consistency, fairness, alignment, transparency"}
            },
            ArgType::NUMBER,
            true, false,
            {
                "ETHICS_EVALUATE_DIMENSION(decision, 'fairness')",
                "ETHICS_EVALUATE_DIMENSION(decision, 'alignment')"
            },
            {CostComplexity::LINEAR, 10.0, 2.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Argument Management Functions
// ============================================================================

/**
 * @brief ETHICS_GET_ARGUMENTS - Retrieve arguments by philosophy
 * 
 * @code
 * FOR arg IN ETHICS_GET_ARGUMENTS("kant", ["pro", "contra"], 50)
 *   RETURN {
 *     school: arg.philosophy_school,
 *     type: arg.argument_type,
 *     content: arg.content,
 *     strength: arg.strength
 *   }
 * @endcode
 */
class EthicsGetArgumentsFunction : public IFunction {
public:
    ~EthicsGetArgumentsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_GET_ARGUMENTS",
            "Ethics",
            "Retrieve ethical arguments filtered by philosophy school and type",
            {
                {"philosophy_school", ArgType::STRING, true, nullptr,
                 "Philosophy school: kant, utilitarianism, virtue_ethics, etc."},
                {"argument_types", ArgType::ARRAY, false, nlohmann::json::array(),
                 "Filter by types: pro, contra, rebuttal, synthesis (empty = all)"},
                {"limit", ArgType::INTEGER, false, 20,
                 "Maximum number of arguments to return"}
            },
            ArgType::ARRAY,
            true, false,
            {
                "ETHICS_GET_ARGUMENTS('kant', [], 10)",
                "ETHICS_GET_ARGUMENTS('utilitarianism', ['pro'], 50)"
            },
            {CostComplexity::INDEXED, 10.0, 2.0, true, true, "ethics_arguments"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief ETHICS_FIND_SIMILAR_DILEMMAS - Vector similarity search
 * 
 * @code
 * LET similar = ETHICS_FIND_SIMILAR_DILEMMAS(
 *   "Should we allow AI in medical diagnosis?",
 *   0.65,
 *   10
 * )
 * 
 * FOR dilemma IN similar
 *   RETURN {id: dilemma.id, similarity: dilemma.similarity}
 * @endcode
 */
class EthicsFindSimilarDilemmasFunction : public IFunction {
public:
    ~EthicsFindSimilarDilemmasFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_FIND_SIMILAR_DILEMMAS",
            "Ethics",
            "Find similar ethical dilemmas using vector similarity search",
            {
                {"query_text", ArgType::STRING, true, nullptr,
                 "Query text to find similar dilemmas"},
                {"threshold", ArgType::NUMBER, false, 0.65,
                 "Similarity threshold (0-1)"},
                {"limit", ArgType::INTEGER, false, 10,
                 "Maximum number of results"}
            },
            ArgType::ARRAY,
            true, false,
            {
                "ETHICS_FIND_SIMILAR_DILEMMAS('AI privacy vs security', 0.7, 5)"
            },
            {CostComplexity::LINEAR, 50.0, 10.0, true, true, "ethics_dilemmas_vector"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief ETHICS_TRAVERSE_CHAIN - Traverse argument chains
 * 
 * @code
 * LET chain = ETHICS_TRAVERSE_CHAIN("arg_001", 5)
 * 
 * FOR node IN chain
 *   RETURN {
 *     id: node.id,
 *     type: node.relationship_type,
 *     content: node.content
 *   }
 * @endcode
 */
class EthicsTraverseChainFunction : public IFunction {
public:
    ~EthicsTraverseChainFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_TRAVERSE_CHAIN",
            "Ethics",
            "Traverse argument chains using graph relationships (supports/counters/rebuts)",
            {
                {"start_id", ArgType::STRING, true, nullptr,
                 "Starting argument ID"},
                {"max_depth", ArgType::INTEGER, false, 5,
                 "Maximum traversal depth"}
            },
            ArgType::ARRAY,
            true, false,
            {
                "ETHICS_TRAVERSE_CHAIN('arg_001', 3)",
                "ETHICS_TRAVERSE_CHAIN(arg.id, 10)"
            },
            {CostComplexity::LINEAR, 30.0, 5.0, true, true, "ethics_arguments_graph"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Philosophy Functions
// ============================================================================

/**
 * @brief ETHICS_LOAD_PROFILE - Load philosophy profile
 * 
 * @code
 * LET profile = ETHICS_LOAD_PROFILE("kant")
 * RETURN {
 *   name: profile.name,
 *   founder: profile.founder,
 *   main_thesis: profile.main_thesis
 * }
 * @endcode
 */
class EthicsLoadProfileFunction : public IFunction {
public:
    ~EthicsLoadProfileFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_LOAD_PROFILE",
            "Ethics",
            "Load detailed philosophy profile by school name",
            {
                {"school", ArgType::STRING, true, nullptr,
                 "Philosophy school: kant, utilitarianism, virtue_ethics, etc."}
            },
            ArgType::OBJECT,
            true, false,
            {
                "ETHICS_LOAD_PROFILE('kant')",
                "ETHICS_LOAD_PROFILE('utilitarianism')"
            },
            {CostComplexity::INDEXED, 5.0, 1.0, true, false, "ethics_profiles"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief ETHICS_LIST_SCHOOLS - List available philosophy schools
 * 
 * @code
 * LET schools = ETHICS_LIST_SCHOOLS()
 * FOR school IN schools
 *   RETURN school.name
 * @endcode
 */
class EthicsListSchoolsFunction : public IFunction {
public:
    ~EthicsListSchoolsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_LIST_SCHOOLS",
            "Ethics",
            "List all available philosophy schools",
            {},
            ArgType::ARRAY,
            true, false,
            {"ETHICS_LIST_SCHOOLS()"},
            {CostComplexity::CONSTANT, 1.0, 0.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// RAG Context Functions
// ============================================================================

/**
 * @brief ETHICS_BUILD_CONTEXT - Build RAG context for decision
 * 
 * @code
 * LET context = ETHICS_BUILD_CONTEXT(
 *   "Should AI systems be allowed to make hiring decisions?",
 *   ["kant", "utilitarianism"],
 *   "employment"
 * )
 * 
 * RETURN {
 *   similar_dilemmas: context.similar_dilemmas,
 *   best_practices: context.best_practices
 * }
 * @endcode
 */
class EthicsBuildContextFunction : public IFunction {
public:
    ~EthicsBuildContextFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_BUILD_CONTEXT",
            "Ethics",
            "Build RAG context by retrieving similar dilemmas, arguments, and best practices",
            {
                {"dilemma_description", ArgType::STRING, true, nullptr,
                 "Current ethical dilemma"},
                {"philosophy_schools", ArgType::ARRAY, true, nullptr,
                 "Philosophy schools to include"},
                {"category", ArgType::STRING, false, "general",
                 "Dilemma category"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "ETHICS_BUILD_CONTEXT('AI decision-making ethics', ['kant'], 'ai_systems')"
            },
            {CostComplexity::LINEAR, 80.0, 15.0, true, true, "ethics_arguments"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Statistics Functions
// ============================================================================

/**
 * @brief ETHICS_STATS - Get philosophy statistics
 * 
 * @code
 * LET stats = ETHICS_STATS("kant")
 * RETURN {
 *   total_arguments: stats.total_arguments,
 *   total_decisions: stats.total_decisions,
 *   avg_confidence: stats.avg_confidence
 * }
 * @endcode
 */
class EthicsStatsFunction : public IFunction {
public:
    ~EthicsStatsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_STATS",
            "Ethics",
            "Get statistics for a philosophy school (argument count, decision count, etc.)",
            {
                {"philosophy_school", ArgType::STRING, false, "",
                 "Philosophy school (empty = all schools)"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "ETHICS_STATS('kant')",
                "ETHICS_STATS()"
            },
            {CostComplexity::LINEAR, 10.0, 2.0, true, true, "ethics_arguments"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief ETHICS_METRICS - Get system metrics
 * 
 * @code
 * LET metrics = ETHICS_METRICS()
 * RETURN metrics.prometheus_format
 * @endcode
 */
class EthicsMetricsFunction : public IFunction {
public:
    ~EthicsMetricsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ETHICS_METRICS",
            "Ethics",
            "Get Ethics AI system metrics in Prometheus format",
            {},
            ArgType::STRING,
            true, false,
            {"ETHICS_METRICS()"},
            {CostComplexity::CONSTANT, 2.0, 0.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Registration Helper
// ============================================================================

/**
 * @brief Register all ethics AI functions with the registry
 */
inline void registerEthicsFunctions(FunctionRegistry& registry) {
    // Decision making
    registry.registerFunction(std::make_unique<EthicsMakeDecisionFunction>());
    registry.registerFunction(std::make_unique<EthicsInitializeDebateFunction>());
    
    // Evaluation
    registry.registerFunction(std::make_unique<EthicsEvaluateFunction>());
    registry.registerFunction(std::make_unique<EthicsEvaluateDimensionFunction>());
    
    // Argument management
    registry.registerFunction(std::make_unique<EthicsGetArgumentsFunction>());
    registry.registerFunction(std::make_unique<EthicsFindSimilarDilemmasFunction>());
    registry.registerFunction(std::make_unique<EthicsTraverseChainFunction>());
    
    // Philosophy
    registry.registerFunction(std::make_unique<EthicsLoadProfileFunction>());
    registry.registerFunction(std::make_unique<EthicsListSchoolsFunction>());
    
    // RAG context
    registry.registerFunction(std::make_unique<EthicsBuildContextFunction>());
    
    // Statistics
    registry.registerFunction(std::make_unique<EthicsStatsFunction>());
    registry.registerFunction(std::make_unique<EthicsMetricsFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
