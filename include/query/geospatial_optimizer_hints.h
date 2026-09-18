/**
 * @file geospatial_optimizer_hints.h
 * @brief Optimizer hints for spatial queries in Phase 6C
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2
 *
 * Provides hint directives for spatial queries:
 * - USE_INDEX(field, "index_name")
 * - FORCE_SCAN(field)
 * - INDEX_PRIORITY(field, priority)
 * - DISTANCE_ORDER(field, "ascending|descending")
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace themis {
namespace query {

/**
 * @brief Spatial optimizer hint types
 */
enum class SpatialHintType {
    USE_INDEX,           // Force use of specific index
    FORCE_SCAN,          // Force full scan (disable index)
    INDEX_PRIORITY,      // Weight factor for index selection
    DISTANCE_ORDER       // Pre-sort by distance
};

/**
 * @brief Single spatial hint directive
 */
struct SpatialHint {
    SpatialHintType type;
    std::string fieldName;           // Field this hint applies to
    std::string indexName;           // For USE_INDEX: specific index name
    double priorityFactor = 1.0;     // For INDEX_PRIORITY: cost adjustment (0.5 = halve cost)
    std::string orderDirection;      // For DISTANCE_ORDER: "ascending" or "descending"
    
    /**
     * @brief Check if hint is valid
     */
    bool isValid() const;
    
    /**
     * @brief String representation for debugging
     */
    std::string toString() const;
};

/**
 * @brief Spatial plan with hint state
 */
struct SpatialPlan {
    std::string predicateId;         // Unique ID for this spatial predicate
    std::vector<SpatialHint> hints;  // Applied hints for this predicate
    
    /**
     * @brief Check if a specific hint type is applied
     */
    bool hasHint(SpatialHintType type) const;
    
    /**
     * @brief Get first hint of given type
     */
    const SpatialHint* getHint(SpatialHintType type) const;
    
    /**
     * @brief Add hint to this plan
     */
    void addHint(const SpatialHint& hint);
    
    /**
     * @brief Get combined cost adjustment from all hints
     */
    double getCostAdjustmentFactor() const;
};

/**
 * @brief Parser and validator for spatial optimizer hints
 */
class SpatialHintParser {
public:
    /**
     * @brief Parse spatial hints from AQL syntax
     * 
     * Supported syntax:
     * - USE_INDEX(doc.location, "geo_idx")
     * - FORCE_SCAN(doc.location)
     * - INDEX_PRIORITY(doc.location, 10.0)
     * - DISTANCE_ORDER(doc.location, "ascending")
     * 
     * @param hintString Raw hint string from query
     * @return Parsed hint, or empty if invalid
     */
    static SpatialHint parseHint(const std::string& hintString);
    
    /**
     * @brief Validate hint against index metadata
     * 
     * Checks:
     * - Field exists in collection
     * - For USE_INDEX: index exists and supports spatial predicates
     * - Priority factor is reasonable (0.1 - 10.0)
     * 
     * @param hint Hint to validate
     * @param availableIndexes Map of index_name -> index_type
     * @return true if hint is valid
     */
    static bool validateHint(
        const SpatialHint& hint,
        const std::map<std::string, std::string>& availableIndexes);
    
    /**
     * @brief Get warning message for hint
     * 
     * May return warning even for valid hints that could be suboptimal.
     * Examples:
     * - Using FORCE_SCAN when good index available
     * - Using USE_INDEX with non-spatial index
     * 
     * @return Warning message, or empty if no warnings
     */
    static std::string getHintWarning(
        const SpatialHint& hint,
        const std::map<std::string, std::string>& availableIndexes);
    
    /**
     * @brief Parse multiple hints from query context
     * 
     * Extracts all hint directives from FILTER/SORT context.
     * Returns vector of valid hints; invalid hints logged and skipped.
     */
    static std::vector<SpatialHint> parseHintsFromQuery(
        const std::string& queryText);
};

/**
 * @brief Hint execution context
 * 
 * Communicates hint information to executor.
 */
struct SpatialHintContext {
    std::vector<SpatialPlan> plans;  // Plan hints for each spatial predicate
    
    /**
     * @brief Get plan for a specific predicate
     */
    const SpatialPlan* getPlanForPredicate(const std::string& predicateId) const;
    
    /**
     * @brief Check if executor should use index for predicate
     */
    bool shouldUseIndex(const std::string& predicateId) const;
    
    /**
     * @brief Get recommended index name (if any)
     */
    std::string getRecommendedIndex(const std::string& predicateId) const;
    
    /**
     * @brief Get cost adjustment for predicate
     */
    double getCostAdjustment(const std::string& predicateId) const;
};

}  // namespace query
}  // namespace themis
