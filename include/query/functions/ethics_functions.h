/**
 * @file ethics_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace query {
namespace functions {


// ============================================================================
// Decision Making Functions
// ============================================================================

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
 * @brief Register Ethics Functions.
 * @param[in,out] registry Input/output parameter.
 * @details Calls: registerFunction().
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
