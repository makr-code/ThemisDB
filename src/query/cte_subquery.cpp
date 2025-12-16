#include "query/cte_subquery.h"
#include "query/query_engine.h"
#include "query/aql_translator.h"
#include "utils/logger.h"

#ifdef _MSC_VER
#pragma warning(disable: 4100)  // unreferenced formal parameter
#endif

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace query {

// CTEDefinition JSON is provided in aql_parser.h

// ============================================================================
// CTEEvaluator Implementation
// ============================================================================

bool CTEEvaluator::evaluateCTE(
    const CTEDefinition& cte,
    QueryEngine& queryEngine
) {
    if (!cte.subquery) {
        THEMIS_ERROR("CTE '{}' has null subquery", cte.name);
        return false;
    }
    
    try {
        // Create CTESpec for QueryEngine execution
        QueryEngine::CTESpec spec;
        spec.name = cte.name;
        spec.subquery = cte.subquery;
        spec.should_materialize = true;
        
        // Create evaluation context for CTE execution
        QueryEngine::EvaluationContext context;
        
        // Copy previously evaluated CTEs to context so they can be referenced
        context.cte_results = cteResults_;
        
        // Execute CTE via QueryEngine
        auto status = queryEngine.executeCTEs({spec}, context);
        
        if (!status.ok) {
            THEMIS_ERROR("CTE '{}' execution failed: {}", cte.name, status.message);
            return false;
        }
        
        // Extract results from context
        auto it = context.cte_results.find(cte.name);
        if (it != context.cte_results.end()) {
            cteResults_[cte.name] = std::move(it->second);
            THEMIS_DEBUG("CTE '{}' evaluated successfully: {} rows", 
                        cte.name, cteResults_[cte.name].size());
            return true;
        } else {
            THEMIS_ERROR("CTE '{}' results not found in context after execution", cte.name);
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("CTE '{}' evaluation exception: {}", cte.name, e.what());
        return false;
    }
}

std::vector<nlohmann::json> CTEEvaluator::getCTEResults(const std::string& cteName) const {
    auto it = cteResults_.find(cteName);
    if (it != cteResults_.end()) {
        return it->second;
    }
    return {};
}

bool CTEEvaluator::hasCTE(const std::string& cteName) const {
    return cteResults_.find(cteName) != cteResults_.end();
}

void CTEEvaluator::clear() {
    cteResults_.clear();
}

// SubqueryExpr JSON is provided in aql_parser.h

// ============================================================================
// SubqueryEvaluator Implementation
// ============================================================================

nlohmann::json SubqueryEvaluator::evaluateSubquery(
    const query::SubqueryExpr& subquery,
    QueryEngine& queryEngine,
    const nlohmann::json& outerRow
) {
    // Phase 1 stub: treat as scalar subquery; real behavior handled elsewhere
    return evaluateScalarSubquery(subquery.subquery, queryEngine, outerRow);
}

nlohmann::json SubqueryEvaluator::evaluateScalarSubquery(
    const std::shared_ptr<query::Query>& query,
    QueryEngine& queryEngine,
    const nlohmann::json& outerRow
) {
    if (!query) {
        THEMIS_ERROR("Scalar subquery is null");
        return nullptr;
    }
    
    try {
        // Translate subquery to executable form
        auto translation = AQLTranslator::translate(query);
        if (!translation.success) {
            THEMIS_ERROR("Scalar subquery translation failed: {}", translation.error_message);
            return nullptr;
        }
        
        // Create evaluation context
        QueryEngine::EvaluationContext context;
        
        // Bind outer variables if correlated subquery
        if (!outerRow.empty()) {
            // Create parent context with outer row bindings
            QueryEngine::EvaluationContext parentContext;
            if (outerRow.is_object()) {
                for (auto& [key, value] : outerRow.items()) {
                    parentContext.bind(key, value);
                }
            }
            context.parent = &parentContext;
        }
        
        // Execute subquery based on type
        std::vector<nlohmann::json> results;
        
        if (translation.join.has_value()) {
            auto& join = translation.join.value();
            auto [status, joinResults] = queryEngine.executeJoin(
                join.for_nodes,
                join.filters,
                join.let_nodes,
                join.return_node,
                join.sort,
                join.limit,
                &context  // Pass context for parent bindings
            );
            
            if (!status.ok) {
                THEMIS_ERROR("Scalar subquery JOIN execution failed: {}", status.message);
                return nullptr;
            }
            results = std::move(joinResults);
            
        } else if (translation.success) {
            // Conjunctive query
            auto [status, entities] = queryEngine.executeAndEntitiesWithFallback(translation.query);
            if (!status.ok) {
                THEMIS_ERROR("Scalar subquery execution failed: {}", status.message);
                return nullptr;
            }
            
            // Convert entities to JSON
            for (const auto& entity : entities) {
                nlohmann::json j;
                j["_key"] = entity.getPrimaryKey();
                // Extract all fields from entity
                // This is a simplified approach - ideally we'd use entity.toJSON()
                results.push_back(j);
            }
        }
        
        // Validate single-row constraint for scalar subquery
        if (results.empty()) {
            return nullptr;
        }
        
        if (results.size() > 1) {
            THEMIS_ERROR("Scalar subquery returned {} rows (expected 1)", results.size());
            throw std::runtime_error("Scalar subquery returned multiple rows");
        }
        
        // Return the first (and only) result
        return results[0];
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Scalar subquery evaluation exception: {}", e.what());
        return nullptr;
    }
}

bool SubqueryEvaluator::evaluateInSubquery(
    const nlohmann::json& value,
    const std::shared_ptr<query::Query>& query,
    QueryEngine& queryEngine,
    const nlohmann::json& outerRow
) {
    if (!query) {
        THEMIS_ERROR("IN subquery is null");
        return false;
    }
    
    try {
        // Translate subquery
        auto translation = AQLTranslator::translate(query);
        if (!translation.success) {
            THEMIS_ERROR("IN subquery translation failed: {}", translation.error_message);
            return false;
        }
        
        // Create evaluation context
        QueryEngine::EvaluationContext context;
        
        // Bind outer variables if correlated
        if (!outerRow.empty()) {
            QueryEngine::EvaluationContext parentContext;
            if (outerRow.is_object()) {
                for (auto& [key, val] : outerRow.items()) {
                    parentContext.bind(key, val);
                }
            }
            context.parent = &parentContext;
        }
        
        // Execute subquery
        std::vector<nlohmann::json> results;
        
        if (translation.join.has_value()) {
            auto& join = translation.join.value();
            auto [status, joinResults] = queryEngine.executeJoin(
                join.for_nodes,
                join.filters,
                join.let_nodes,
                join.return_node,
                join.sort,
                join.limit,
                &context
            );
            
            if (!status.ok) {
                THEMIS_ERROR("IN subquery JOIN execution failed: {}", status.message);
                return false;
            }
            results = std::move(joinResults);
            
        } else if (translation.success) {
            auto [status, entities] = queryEngine.executeAndEntitiesWithFallback(translation.query);
            if (!status.ok) {
                THEMIS_ERROR("IN subquery execution failed: {}", status.message);
                return false;
            }
            
            for (const auto& entity : entities) {
                nlohmann::json j;
                j["_key"] = entity.getPrimaryKey();
                results.push_back(j);
            }
        }
        
        // Check if value exists in result set
        for (const auto& result : results) {
            if (result == value) {
                return true;
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("IN subquery evaluation exception: {}", e.what());
        return false;
    }
}

bool SubqueryEvaluator::evaluateExistsSubquery(
    const std::shared_ptr<query::Query>& query,
    QueryEngine& queryEngine,
    const nlohmann::json& outerRow
) {
    if (!query) {
        THEMIS_ERROR("EXISTS subquery is null");
        return false;
    }
    
    try {
        // Translate subquery
        auto translation = AQLTranslator::translate(query);
        if (!translation.success) {
            THEMIS_ERROR("EXISTS subquery translation failed: {}", translation.error_message);
            return false;
        }
        
        // Create evaluation context
        QueryEngine::EvaluationContext context;
        
        // Bind outer variables if correlated
        if (!outerRow.empty()) {
            QueryEngine::EvaluationContext parentContext;
            if (outerRow.is_object()) {
                for (auto& [key, val] : outerRow.items()) {
                    parentContext.bind(key, val);
                }
            }
            context.parent = &parentContext;
        }
        
        // Execute subquery - we only need to know if it returns any rows
        // Could optimize with LIMIT 1
        
        if (translation.join.has_value()) {
            auto& join = translation.join.value();
            auto [status, joinResults] = queryEngine.executeJoin(
                join.for_nodes,
                join.filters,
                join.let_nodes,
                join.return_node,
                join.sort,
                join.limit,
                &context
            );
            
            if (!status.ok) {
                THEMIS_ERROR("EXISTS subquery JOIN execution failed: {}", status.message);
                return false;
            }
            
            return !joinResults.empty();
            
        } else if (translation.success) {
            auto [status, entities] = queryEngine.executeAndEntitiesWithFallback(translation.query);
            if (!status.ok) {
                THEMIS_ERROR("EXISTS subquery execution failed: {}", status.message);
                return false;
            }
            
            return !entities.empty();
        }
        
        return false;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("EXISTS subquery evaluation exception: {}", e.what());
        return false;
    }
}

void SubqueryEvaluator::bindOuterVariables(
    const std::shared_ptr<query::Query>& query,
    const nlohmann::json& outerRow
) {
    // Note: Actual variable binding is handled in the evaluation context
    // passed to query execution methods. This method is kept for future
    // use if we need to modify the query AST itself for optimization.
    
    // For now, binding is done via parent context in evaluation methods above
    THEMIS_DEBUG("Binding outer variables for correlated subquery");
}

} // namespace query
} // namespace themis
