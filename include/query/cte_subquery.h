/**
 * @file cte_subquery.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"
#include "query/query_engine.h"
#include "utils/expected.h"

namespace themis {
namespace query {

class QueryEngine;


// Use CTEDefinition from aql_parser.h

class CTEEvaluator {
public:
    struct RecursiveCTEConfig {
        size_t max_iterations = 1000;        // Maximum fixpoint iterations
        bool enable_cycle_detection = true;  // Detect cycles in recursive data
        size_t max_result_size = 1000000;    // Maximum total result size
    };
    
    CTEEvaluator() = default;
    explicit CTEEvaluator(const RecursiveCTEConfig& config) : recursiveConfig_(config) {}
    
    Result<void> evaluateCTE(
        const CTEDefinition& cte,
        QueryEngine& queryEngine,
        bool is_recursive = false
    );
    
    /**
     * @brief Evaluate Recursive CTE.
     * @param[in] cte Input parameter.
     * @param[in,out] queryEngine Input/output parameter.
     * @return Return value.
     */
    Result<void> evaluateRecursiveCTE(
        const CTEDefinition& cte,
        QueryEngine& queryEngine
    );
    
    /**
     * @brief Get CTEResults.
     * @param[in] cteName Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> getCTEResults(const std::string& cteName) const;
    
    /**
     * @brief Has CTE.
     * @param[in] cteName Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCTE(const std::string& cteName) const;
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Set Recursive Config.
     * @param[in] config Input parameter.
     * @details Implements setRecursiveConfig without additional internal calls.
     */
    void setRecursiveConfig(const RecursiveCTEConfig& config) {
        recursiveConfig_ = config;
    }
    
private:
    // CTE Name → Results (materialized as JSON array)
    std::unordered_map<std::string, std::vector<nlohmann::json>> cteResults_;
    
    // Recursive CTE configuration
    RecursiveCTEConfig recursiveConfig_;
    
    /**
     * @brief Are Results Equal.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return True when the operation succeeds.
     */
    bool areResultsEqual(
        const std::vector<nlohmann::json>& a,
        const std::vector<nlohmann::json>& b
    ) const;
    
    /**
     * @brief Detect Cycle.
     * @param[in] newResults Input parameter.
     * @param[in] history Input parameter.
     * @return True when the operation succeeds.
     */
    bool detectCycle(
        const std::vector<nlohmann::json>& newResults,
        const std::vector<std::vector<nlohmann::json>>& history
    ) const;
};


// Use SubqueryExpr from aql_parser.h

class SubqueryEvaluator {
public:
    SubqueryEvaluator() = default;
    
    Result<nlohmann::json> evaluateSubquery(
        const query::SubqueryExpr& subquery,
        QueryEngine& queryEngine,
        const nlohmann::json& outerRow = nlohmann::json()
    );
    
    /**
     * @brief Evaluate Scalar Subquery.
     * @param[in] query Input parameter.
     * @param[in,out] queryEngine Input/output parameter.
     * @param[in] outerRow Input parameter.
     * @return Return value.
     */
    Result<nlohmann::json> evaluateScalarSubquery(
        const std::shared_ptr<query::Query>& query,
        QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );
    
    /**
     * @brief Evaluate In Subquery.
     * @param[in] value Input parameter.
     * @param[in] query Input parameter.
     * @param[in,out] queryEngine Input/output parameter.
     * @param[in] outerRow Input parameter.
     * @return Return value.
     */
    Result<bool> evaluateInSubquery(
        const nlohmann::json& value,
        const std::shared_ptr<query::Query>& query,
        QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );
    
    /**
     * @brief Evaluate Exists Subquery.
     * @param[in] query Input parameter.
     * @param[in,out] queryEngine Input/output parameter.
     * @param[in] outerRow Input parameter.
     * @return Return value.
     */
    Result<bool> evaluateExistsSubquery(
        const std::shared_ptr<query::Query>& query,
        QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );
    
private:
    /**
     * @brief Evaluate Array Subquery.
     * @param[in] query Input parameter.
     * @param[in,out] queryEngine Input/output parameter.
     * @param[in] outerRow Input parameter.
     * @return Return value.
     */
    Result<nlohmann::json> evaluateArraySubquery(
        const std::shared_ptr<query::Query>& query,
        QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );

    /**
     * @brief Bind Outer Variables.
     * @param[in] query Input parameter.
     * @param[in] outerRow Input parameter.
     */
    void bindOuterVariables(
        const std::shared_ptr<query::Query>& query,
        const nlohmann::json& outerRow
    );
};

} // namespace query
} // namespace themis
