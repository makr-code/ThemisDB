/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cte_subquery.h                                     ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:36:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     284                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 010ee709e6  2026-03-15  docs(query): mark CTESubquery Phase 1 stub replacement co... ║
    • 4730efec66  2026-03-14  feat(query): replace CTESubquery Phase 1 stub with correl... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"
#include "utils/expected.h"

namespace themis {
class QueryEngine; // forward declaration in outer namespace
namespace query {

/**
 * @brief Common Table Expression (CTE) Support für AQL
 * 
 * Unterstützt WITH-Clause für temporary named result sets:
 * 
 * WITH high_earners AS (
 *   FOR u IN users
 *   FILTER u.salary > 100000
 *   RETURN u
 * ),
 * avg_salaries AS (
 *   FOR h IN high_earners
 *   COLLECT city = h.city
 *   AGGREGATE avg_salary = AVG(h.salary)
 *   RETURN {city, avg_salary}
 * )
 * FOR a IN avg_salaries
 *   FILTER a.avg_salary > 120000
 *   RETURN a
 * 
 * Recursive CTEs (Phase 2):
 * WITH RECURSIVE org_tree AS (
 *   FOR e IN employees
 *   FILTER e.manager_id == null
 *   RETURN e
 *   UNION
 *   FOR e IN employees, o IN org_tree
 *   FILTER e.manager_id == o.id
 *   RETURN e
 * )
 * FOR o IN org_tree RETURN o
 */

// Use CTEDefinition from aql_parser.h

/**
 * @brief CTE Evaluator with Recursive CTE Support
 * 
 * v1.3.0: Added recursive CTE support with fixpoint iteration
 */
class CTEEvaluator {
public:
    struct RecursiveCTEConfig {
        size_t max_iterations = 1000;        // Maximum fixpoint iterations
        bool enable_cycle_detection = true;  // Detect cycles in recursive data
        size_t max_result_size = 1000000;    // Maximum total result size
    };
    
    CTEEvaluator() = default;
    explicit CTEEvaluator(const RecursiveCTEConfig& config) : recursiveConfig_(config) {}
    
    /**
     * @brief Evaluiert eine CTE und speichert Resultate
     * @param cte Die CTE-Definition
     * @param queryEngine Query Engine für Sub-Query Execution
     * @param is_recursive true wenn CTE recursive ist
     * @return Result indicating success or error
     */
    Result<void> evaluateCTE(
        const CTEDefinition& cte,
        ::themis::QueryEngine& queryEngine,
        bool is_recursive = false
    );
    
    /**
     * @brief Evaluiert eine recursive CTE mit fixpoint iteration
     * @param cte Die recursive CTE-Definition
     * @param queryEngine Query Engine für Execution
     * @return Result indicating success or error
     */
    Result<void> evaluateRecursiveCTE(
        const CTEDefinition& cte,
        ::themis::QueryEngine& queryEngine
    );
    
    /**
     * @brief Holt gespeicherte CTE-Resultate
     * @param cteName CTE Name
     * @return CTE Results (JSON array) oder empty wenn nicht vorhanden
     */
    std::vector<nlohmann::json> getCTEResults(const std::string& cteName) const;
    
    /**
     * @brief Prüft ob CTE existiert
     * @param cteName CTE Name
     * @return true wenn CTE evaluiert wurde
     */
    bool hasCTE(const std::string& cteName) const;
    
    /**
     * @brief Löscht alle CTE Results (nach Query-Completion)
     */
    void clear();
    
    /**
     * @brief Setzt Recursive CTE Konfiguration
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
     * @brief Prüft ob zwei Result-Sets identisch sind (für fixpoint check)
     */
    bool areResultsEqual(
        const std::vector<nlohmann::json>& a,
        const std::vector<nlohmann::json>& b
    ) const;
    
    /**
     * @brief Erkennt Zyklen in recursive CTEs
     */
    bool detectCycle(
        const std::vector<nlohmann::json>& newResults,
        const std::vector<std::vector<nlohmann::json>>& history
    ) const;
};

/**
 * @brief Subquery Support für AQL
 * 
 * Unterstützt:
 * 1. Scalar Subqueries:
 *    FOR u IN users
 *    FILTER u.salary > (FOR a IN salaries RETURN AVG(a.value))
 *    RETURN u
 * 
 * 2. IN Subqueries:
 *    FOR u IN users
 *    FILTER u.id IN (FOR o IN orders FILTER o.status == "active" RETURN o.user_id)
 *    RETURN u
 * 
 * 3. EXISTS Subqueries:
 *    FOR u IN users
 *    FILTER EXISTS(FOR o IN orders FILTER o.user_id == u.id RETURN 1)
 *    RETURN u
 * 
 * 4. Correlated Subqueries:
 *    FOR u IN users
 *    RETURN {
 *      name: u.name,
 *      order_count: (FOR o IN orders FILTER o.user_id == u.id RETURN COUNT())
 *    }
 */

// Use SubqueryExpr from aql_parser.h

/**
 * @brief Subquery Evaluator
 */
class SubqueryEvaluator {
public:
    SubqueryEvaluator() = default;
    
    /**
     * @brief Evaluiert eine Subquery
     * @param subquery Subquery Definition
     * @param queryEngine Query Engine für Execution
     * @param outerRow Outer Row (für correlated subqueries)
     * @return Subquery Result (scalar value, array, or boolean) or error
     */
    Result<nlohmann::json> evaluateSubquery(
        const query::SubqueryExpr& subquery,
        ::themis::QueryEngine& queryEngine,
        const nlohmann::json& outerRow = nlohmann::json()
    );
    
    /**
     * @brief Evaluiert SCALAR Subquery (returns single value)
     * @param query Subquery
     * @param queryEngine Query Engine
     * @param outerRow Outer Row
     * @return Scalar value or error
     */
    Result<nlohmann::json> evaluateScalarSubquery(
        const std::shared_ptr<query::Query>& query,
        ::themis::QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );
    
    /**
     * @brief Evaluiert IN Subquery (returns set)
     * @param value Value to check
     * @param query Subquery
     * @param queryEngine Query Engine
     * @param outerRow Outer Row
     * @return Result containing boolean (true wenn value in result set)
     */
    Result<bool> evaluateInSubquery(
        const nlohmann::json& value,
        const std::shared_ptr<query::Query>& query,
        ::themis::QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );
    
    /**
     * @brief Evaluiert EXISTS Subquery
     * @param query Subquery
     * @param queryEngine Query Engine
     * @param outerRow Outer Row
     * @return Result containing boolean (true wenn Subquery mindestens ein Result liefert)
     */
    Result<bool> evaluateExistsSubquery(
        const std::shared_ptr<query::Query>& query,
        ::themis::QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );
    
private:
    /**
     * @brief Evaluates a correlated subquery, returning all matching rows as a JSON array.
     *
     * Used when the subquery AST references outer variables. Binds the outer row into
     * the evaluation context so the subquery is executed once with those bindings.
     *
     * @param query     Subquery to execute
     * @param queryEngine  Query engine
     * @param outerRow  Outer row whose fields are injected as parent-context bindings
     * @return JSON array of all result rows, or an error
     */
    Result<nlohmann::json> evaluateArraySubquery(
        const std::shared_ptr<query::Query>& query,
        ::themis::QueryEngine& queryEngine,
        const nlohmann::json& outerRow
    );

    /**
     * @brief Bindet Outer Variables in Subquery Context
     * @param query Subquery
     * @param outerRow Outer Row
     */
    void bindOuterVariables(
        const std::shared_ptr<query::Query>& query,
        const nlohmann::json& outerRow
    );
};

} // namespace query
} // namespace themis
