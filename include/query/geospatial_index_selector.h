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

enum class SpatialIndexType {
    RTREE,       // R-tree index (balanced, good for range queries)
    GRID,        // Grid-based index (good for uniform distributions)
    QUADTREE,    // Quadtree index (adaptive grid)
    NONE         // No spatial index (full scan)
};

struct DataDistribution {
    enum Type { UNIFORM, CLUSTERED, SKEWED };
    
    Type type = UNIFORM;
    double clusteringRatio = 0.0;    // 0-1: how clustered is data
    bool isGlobalDataset = false;    // True if spans whole globe (WGS84)
    size_t approximateClusterCount = 0;
    
    /**
     * @brief Infer.
     * @param[in] totalPoints Input parameter.
     * @param[in] distinctLocationCells Input parameter.
     * @param[in] spatialVariance Input parameter.
     * @return Return value.
     */
    static DataDistribution infer(
        size_t totalPoints,
        size_t distinctLocationCells,  // From spatial histogram
        double spatialVariance);
};

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
     * @brief Get Efficiency Score.
     * @return Return value.
     */
    double getEfficiencyScore() const;
};

struct IndexCandidate {
    std::string indexName;
    SpatialIndexType type;
    double score = 0.0;              // Higher is better
    double estimatedCostWithIndexUs = 0.0;
    double selectivityGain = 1.0;    // Cost reduction vs. full scan
    bool isRecommended = false;
    
    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
};

class GeospatialIndexSelector {
public:
    static IndexCandidate selectIndex(
        const std::string& predicateType,
        size_t totalRows,
        const std::map<std::string, IndexStatistics>& availableIndexes,
        const DataDistribution& dataDistribution,
        double geometryParam = 0.0);
    
    static std::vector<IndexCandidate> rankIndexes(
        const std::string& predicateType,
        size_t totalRows,
        const std::map<std::string, IndexStatistics>& availableIndexes,
        const DataDistribution& dataDistribution,
        double geometryParam = 0.0);
    
    static std::map<std::string, IndexStatistics> getAvailableIndexes(
        const std::string& collectionName);
    
    /**
     * @brief Infer Distribution.
     * @param[in] histogram Input parameter.
     * @return Return value.
     */
    static DataDistribution inferDistribution(
        const SpatialHistogram& histogram);
    
    /**
     * @brief Calculate Selectivity Gain.
     * @param[in] indexType Input parameter.
     * @param[in] totalRows Input parameter.
     * @param[in] predicateType Input parameter.
     * @return Return value.
     */
    static double calculateSelectivityGain(
        SpatialIndexType indexType,
        size_t totalRows,
        const std::string& predicateType);

private:
    /**
     * @brief Score Index.
     * @param[in] index Input parameter.
     * @param[in] predicateType Input parameter.
     * @param[in] dataDistribution Input parameter.
     * @param[in] totalRows Input parameter.
     * @return Return value.
     */
    static double scoreIndex(
        const IndexStatistics& index,
        const std::string& predicateType,
        const DataDistribution& dataDistribution,
        size_t totalRows);
    
    /**
     * @brief Score Full Scan.
     * @param[in] totalRows Input parameter.
     * @param[in] predicateType Input parameter.
     * @return Return value.
     */
    static double scoreFullScan(
        size_t totalRows,
        const std::string& predicateType);
    
    /**
     * @brief Get Index Type Cost Multiplier.
     * @param[in] type Input parameter.
     * @param[in] distribution Input parameter.
     * @return Return value.
     */
    static double getIndexTypeCostMultiplier(
        SpatialIndexType type,
        const DataDistribution& distribution);
};

}  // namespace query
}  // namespace themis
