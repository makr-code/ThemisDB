/**
 * @file geospatial_query_rewrite.h
 * @brief Query plan rewrite rules for spatial queries in Phase 6C
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2
 *
 * Implements 5 spatial query rewrite rules:
 * 1. Index path reordering (early filtering)
 * 2. Distance-based ordering optimization
 * 3. Intersection optimization (decomposition)
 * 4. Redundant predicate elimination
 * 5. Predicate pushdown (move toward source)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace themis {
namespace query {

// Forward declarations
class ExecutionPlan;  // Placeholder for actual plan type

struct RewriteResult {
    bool applied = false;           // Was rule applied?
    bool valid = true;              // Is result semantically correct?
    double costReduction = 0.0;     // Cost improvement (0 = no improvement)
    std::string reason;             // Why rule was/wasn't applied
    
    explicit operator bool() const { return applied && valid; }
};

class GeospatialQueryRewriter {
public:
    /**
     * @brief Optimize Query Plan.
     * @param[in,out] plan Input/output parameter.
     * @param[in] estimatedCostBefore Input parameter.
     * @return True when the operation succeeds.
     */
    static bool optimizeQueryPlan(
        ExecutionPlan& plan,
        double estimatedCostBefore);
    
    /**
     * @brief Apply Index Path Reordering.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static RewriteResult applyIndexPathReordering(ExecutionPlan& plan);
    
    /**
     * @brief Apply Distance Ordering Optimization.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static RewriteResult applyDistanceOrderingOptimization(ExecutionPlan& plan);
    
    /**
     * @brief Apply Intersection Optimization.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static RewriteResult applyIntersectionOptimization(ExecutionPlan& plan);
    
    /**
     * @brief Apply Redundant Predicate Elimination.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static RewriteResult applyRedundantPredicateElimination(ExecutionPlan& plan);
    
    /**
     * @brief Apply Predicate Pushdown.
     * @param[in,out] plan Input/output parameter.
     * @return Return value.
     */
    static RewriteResult applyPredicatePushdown(ExecutionPlan& plan);
    
    /**
     * @brief Validate Plan Equivalence.
     * @param[in] originalPlan Input parameter.
     * @param[in] transformedPlan Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validatePlanEquivalence(
        const ExecutionPlan& originalPlan,
        const ExecutionPlan& transformedPlan);

private:
    /**
     * @brief Can Be Indexed.
     * @param[in] predicateType Input parameter.
     * @return True when the operation succeeds.
     */
    static bool canBeIndexed(const std::string& predicateType);
    
    /**
     * @brief Estimate Cost Reduction.
     * @param[in] transformation Input parameter.
     * @param[in] affectedRows Input parameter.
     * @param[in] costBefore Input parameter.
     * @return Return value.
     */
    static double estimateCostReduction(
        const std::string& transformation,
        size_t affectedRows,
        double costBefore);
    
    /**
     * @brief Extract Spatial Predicates.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> extractSpatialPredicates(
        const ExecutionPlan& plan);
    
    /**
     * @brief Reorder Filter Predicates.
     * @param[in,out] plan Input/output parameter.
     * @param[in] newOrder Input parameter.
     * @return True when the operation succeeds.
     */
    static bool reorderFilterPredicates(
        ExecutionPlan& plan,
        const std::vector<std::string>& newOrder);
};

}  // namespace query
}  // namespace themis
