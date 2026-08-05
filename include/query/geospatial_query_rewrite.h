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

/**
 * @brief Result of applying a rewrite rule
 */
struct RewriteResult {
    bool applied = false;           // Was rule applied?
    bool valid = true;              // Is result semantically correct?
    double costReduction = 0.0;     // Cost improvement (0 = no improvement)
    std::string reason;             // Why rule was/wasn't applied
    
    explicit operator bool() const { return applied && valid; }
};

/**
 * @brief Spatial query rewrite rule engine
 * 
 * Implements 5 rewrite rules for optimizing spatial queries.
 */
class GeospatialQueryRewriter {
public:
    /**
     * @brief Apply all applicable rewrite rules to plan
     * 
     * @param plan Query execution plan to optimize
     * @param estimatedCostBefore Cost before rewriting (for validation)
     * @return true if plan was modified
     */
    static bool optimizeQueryPlan(
        ExecutionPlan& plan,
        double estimatedCostBefore);
    
    /**
     * @brief Rule 1: Index Path Reordering
     * 
     * Move indexed ST_CONTAINS before unindexed filters.
     * Rationale: Filter early to reduce downstream processing.
     * 
     * Example:
     *   Before: FILTER a > 5 AND ST_CONTAINS(loc, poly)
     *   After:  FILTER ST_CONTAINS(loc, poly) AND a > 5
     * 
     * @return true if rule applied and plan was modified
     */
    static RewriteResult applyIndexPathReordering(ExecutionPlan& plan);
    
    /**
     * @brief Rule 2: Distance-Based Ordering Optimization
     * 
     * Combine ST_DISTANCE with ORDER BY optimization.
     * Use index to pre-sort results when possible.
     * 
     * Example:
     *   Before: FILTER ST_DISTANCE(doc.loc, center) < 100
     *           SORT BY ST_DISTANCE(doc.loc, center) ASC
     *   After:  Use R-tree nearest-neighbor scan + pre-sorted results
     * 
     * @return true if rule applied
     */
    static RewriteResult applyDistanceOrderingOptimization(ExecutionPlan& plan);
    
    /**
     * @brief Rule 3: Intersection Optimization
     * 
     * Decompose ST_INTERSECTS into bounding box + interior check.
     * Use index for bounding box, refine with geometry.
     * 
     * Example:
     *   Before: ST_INTERSECTS(doc.geom, queryGeom)
     *   After:  ST_BBOX_INTERSECTS(doc.geom, queryBBox)
     *           AND ST_INTERSECTS_REFINED(doc.geom, queryGeom)
     * 
     * @return true if rule applied
     */
    static RewriteResult applyIntersectionOptimization(ExecutionPlan& plan);
    
    /**
     * @brief Rule 4: Redundant Predicate Elimination
     * 
     * Recognize redundant predicates and eliminate.
     * Example: ST_CONTAINS(...) AND ST_DISTANCE(...) < X is redundant
     * if containment already implies distance constraint.
     * 
     * Example:
     *   Before: ST_CONTAINS(doc.loc, bigPoly) AND ST_DISTANCE(doc.loc, center) < 10
     *   After:  ST_CONTAINS(doc.loc, bigPoly)  (if bigPoly contains all points within 10km)
     * 
     * @return true if rule applied
     */
    static RewriteResult applyRedundantPredicateElimination(ExecutionPlan& plan);
    
    /**
     * @brief Rule 5: Predicate Pushdown
     * 
     * Move spatial predicates closer to source.
     * Example: Apply ST_DISTANCE filter before JOIN.
     * 
     * Example:
     *   Before: docs JOIN other_docs ON ...
     *           FILTER ST_DISTANCE(docs.loc, center) < 100
     *   After:  docs.filtered := FILTER ST_DISTANCE(docs.loc, center) < 100
     *           docs.filtered JOIN other_docs ON ...
     * 
     * @return true if rule applied
     */
    static RewriteResult applyPredicatePushdown(ExecutionPlan& plan);
    
    /**
     * @brief Validate transformed plan produces equivalent results
     * 
     * Checks:
     * - Same columns returned
     * - Same predicates enforced
     * - Equivalent join conditions
     * 
     * @return true if plan is semantically equivalent
     */
    static bool validatePlanEquivalence(
        const ExecutionPlan& originalPlan,
        const ExecutionPlan& transformedPlan);

private:
    /**
     * @brief Check if a predicate can be indexed
     */
    static bool canBeIndexed(const std::string& predicateType);
    
    /**
     * @brief Estimate cost reduction for a transformation
     */
    static double estimateCostReduction(
        const std::string& transformation,
        size_t affectedRows,
        double costBefore);
    
    /**
     * @brief Extract spatial predicates from plan
     */
    static std::vector<std::string> extractSpatialPredicates(
        const ExecutionPlan& plan);
    
    /**
     * @brief Reorder predicates in filter
     */
    static bool reorderFilterPredicates(
        ExecutionPlan& plan,
        const std::vector<std::string>& newOrder);
};

}  // namespace query
}  // namespace themis
