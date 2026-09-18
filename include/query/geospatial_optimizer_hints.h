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

enum class SpatialHintType {
    USE_INDEX,           // Force use of specific index
    FORCE_SCAN,          // Force full scan (disable index)
    INDEX_PRIORITY,      // Weight factor for index selection
    DISTANCE_ORDER       // Pre-sort by distance
};

struct SpatialHint {
    SpatialHintType type;
    std::string fieldName;           // Field this hint applies to
    std::string indexName;           // For USE_INDEX: specific index name
    double priorityFactor = 1.0;     // For INDEX_PRIORITY: cost adjustment (0.5 = halve cost)
    std::string orderDirection;      // For DISTANCE_ORDER: "ascending" or "descending"
    
    /**
     * @brief Is Valid.
     * @return True when the operation succeeds.
     */
    bool isValid() const;
    
    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
};

struct SpatialPlan {
    std::string predicateId;         // Unique ID for this spatial predicate
    std::vector<SpatialHint> hints;  // Applied hints for this predicate
    
    /**
     * @brief Has Hint.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasHint(SpatialHintType type) const;
    
    /**
     * @brief Get Hint.
     * @param[in] type Input parameter.
     * @return Pointer to the result.
     */
    const SpatialHint* getHint(SpatialHintType type) const;
    
    /**
     * @brief Add Hint.
     * @param[in] hint Input parameter.
     */
    void addHint(const SpatialHint& hint);
    
    /**
     * @brief Get Cost Adjustment Factor.
     * @return Return value.
     */
    double getCostAdjustmentFactor() const;
};

class SpatialHintParser {
public:
    /**
     * @brief Parse Hint.
     * @param[in] hintString Input parameter.
     * @return Return value.
     */
    static SpatialHint parseHint(const std::string& hintString);
    
    static bool validateHint(
        const SpatialHint& hint,
        const std::map<std::string, std::string>& availableIndexes);
    
    static std::string getHintWarning(
        const SpatialHint& hint,
        const std::map<std::string, std::string>& availableIndexes);
    
    /**
     * @brief Parse Hints From Query.
     * @param[in] queryText Input parameter.
     * @return Return value.
     */
    static std::vector<SpatialHint> parseHintsFromQuery(
        const std::string& queryText);
};

struct SpatialHintContext {
    std::vector<SpatialPlan> plans;  // Plan hints for each spatial predicate
    
    /**
     * @brief Get Plan For Predicate.
     * @param[in] predicateId Input parameter.
     * @return Pointer to the result.
     */
    const SpatialPlan* getPlanForPredicate(const std::string& predicateId) const;
    
    /**
     * @brief Should Use Index.
     * @param[in] predicateId Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldUseIndex(const std::string& predicateId) const;
    
    /**
     * @brief Get Recommended Index.
     * @param[in] predicateId Input parameter.
     * @return Return value.
     */
    std::string getRecommendedIndex(const std::string& predicateId) const;
    
    /**
     * @brief Get Cost Adjustment.
     * @param[in] predicateId Input parameter.
     * @return Return value.
     */
    double getCostAdjustment(const std::string& predicateId) const;
};

}  // namespace query
}  // namespace themis
