/**
 * @file geospatial_index_selector.h
 * @brief Spatial index selection logic for Phase 6C
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2
 *
 * Implements automatic spatial index selection for query optimization.
 * Selects between R-tree, grid-based indexes, or full scan based on:
 * - Data distribution (clustered vs. uniform)
 * - Predicate type (distance vs. containment)
 * - Index statistics (size, hit rate)
 * - Query characteristics
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "query/geospatial_cost_model.h"

namespace themis {
namespace query {

/**
 * @brief Index type enumeration
 */
enum class SpatialIndexType {
    RTREE,       // R-tree index (balanced, good for range queries)
    GRID,        // Grid-based index (good for uniform distributions)
    QUADTREE,    // Quadtree index (adaptive grid)
    NONE         // No spatial index (full scan)
};

/**
 * @brief Data distribution characteristics
 */
struct DataDistribution {
    enum Type { UNIFORM, CLUSTERED, SKEWED };
    
    Type type = UNIFORM;
    double clusteringRatio = 0.0;    // 0-1: how clustered is data
    bool isGlobalDataset = false;    // True if spans whole globe (WGS84)
    size_t approximateClusterCount = 0;
    
    /**
     * @brief Determine distribution type from statistics
     */
    static DataDistribution infer(
        size_t totalPoints,
        size_t distinctLocationCells,  // From spatial histogram
        double spatialVariance);
};

/**
 * @brief Index statistics from metadata
 */
struct IndexStatistics {
    std::string indexName;
    SpatialIndexType type;
    size_t indexSizeBytes = 0;
    double creationTime = 0.0;       // Seconds to build
    double maintenanceOverhead = 0.0; // Cost as % of insert/update
    size_t estimatedNodeCount = 0;   // For tree structures
    double averageNodeDensity = 0.0; // Points per node
    
    // Historical performance
    size_t totalQueriesRun = 0;
    size_t indexedQueries = 0;
    size_t fullScanFallbacks = 0;
    double averageHitRate = 0.0;     // % of queries using this index
    
    /**
     * @brief Calculate index efficiency score (0-1)
     */
    double getEfficiencyScore() const;
};

/**
 * @brief Ranking of candidate indexes
 */
struct IndexCandidate {
    std::string indexName;
    SpatialIndexType type;
    double score = 0.0;              // Higher is better
    double estimatedCostWithIndexUs = 0.0;
    double selectivityGain = 1.0;    // Cost reduction vs. full scan
    bool isRecommended = false;
    
    /**
     * @brief String representation for debugging
     */
    std::string toString() const;
};

/**
 * @brief Spatial index selector
 * 
 * Analyzes available indexes and query characteristics to select
 * the best index for a spatial predicate.
 */
class GeospatialIndexSelector {
public:
    /**
     * @brief Select best index for a spatial predicate
     * 
     * @param predicateType "DISTANCE", "CONTAINS", or "INTERSECTS"
     * @param totalRows Total documents in collection
     * @param availableIndexes Map of index_name -> IndexStatistics
     * @param dataDistribution Characteristics of data distribution
     * @param geometryParam Optional complexity measure (vertices, radius, etc.)
     * @return Best index candidate with cost estimate
     */
    static IndexCandidate selectIndex(
        const std::string& predicateType,
        size_t totalRows,
        const std::map<std::string, IndexStatistics>& availableIndexes,
        const DataDistribution& dataDistribution,
        double geometryParam = 0.0);
    
    /**
     * @brief Rank all available indexes for a predicate
     * 
     * Returns all candidates sorted by score (best first).
     * Includes full scan as fallback option.
     */
    static std::vector<IndexCandidate> rankIndexes(
        const std::string& predicateType,
        size_t totalRows,
        const std::map<std::string, IndexStatistics>& availableIndexes,
        const DataDistribution& dataDistribution,
        double geometryParam = 0.0);
    
    /**
     * @brief Collect available spatial indexes from collection
     * 
     * This would typically query the index manager to get all
     * spatial indexes on the collection.
     * 
     * @param collectionName Name of collection
     * @return Map of index_name -> IndexStatistics
     * 
     * NOTE: This is a callback point; actual implementation
     * depends on index manager API.
     */
    static std::map<std::string, IndexStatistics> getAvailableIndexes(
        const std::string& collectionName);
    
    /**
     * @brief Infer data distribution from statistics
     * 
     * Analyzes spatial histogram to determine distribution characteristics.
     * 
     * @param histogram Spatial histogram from cost model
     * @return DataDistribution characteristics
     */
    static DataDistribution inferDistribution(
        const SpatialHistogram& histogram);
    
    /**
     * @brief Calculate selectivity gain for an index
     * 
     * Ratio of full scan cost to indexed scan cost.
     * Higher gain = more benefit from using this index.
     * 
     * @param indexType Type of index
     * @param totalRows Total documents
     * @param predicateType Type of spatial predicate
     * @return Selectivity gain (≥1.0, where 1.0 = no benefit)
     */
    static double calculateSelectivityGain(
        SpatialIndexType indexType,
        size_t totalRows,
        const std::string& predicateType);

private:
    /**
     * @brief Score a single index candidate
     * 
     * Considers:
     * - Index type suitability for predicate
     * - Data distribution fit
     * - Index efficiency (maintenance vs. benefit)
     * - Historical hit rate
     */
    static double scoreIndex(
        const IndexStatistics& index,
        const std::string& predicateType,
        const DataDistribution& dataDistribution,
        size_t totalRows);
    
    /**
     * @brief Score full scan fallback option
     */
    static double scoreFullScan(
        size_t totalRows,
        const std::string& predicateType);
    
    /**
     * @brief Get base cost multiplier for index type
     * 
     * R-tree: 1.0x (baseline)
     * Grid: 0.8-1.2x depending on distribution
     * Quadtree: 0.9-1.1x (adaptive)
     */
    static double getIndexTypeCostMultiplier(
        SpatialIndexType type,
        const DataDistribution& distribution);
};

}  // namespace query
}  // namespace themis
