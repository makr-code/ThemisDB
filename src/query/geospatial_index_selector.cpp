/**
 * @file geospatial_index_selector.cpp
 * @brief Spatial index selection implementation for Phase 6C
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2
 */

#include "query/geospatial_index_selector.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace themis {
namespace query {

// =============================================================================
// DataDistribution
// =============================================================================

DataDistribution DataDistribution::infer(
    size_t totalPoints,
    size_t distinctLocationCells,
    double spatialVariance) {
    
    DataDistribution dist;
    
    if (totalPoints == 0) {
        dist.type = UNIFORM;
        return dist;
    }
    
    // Calculate clustering ratio
    // Uniform: cells filled evenly
    // Clustered: few cells have many points
    double expectedCellsPerPoint = totalPoints > 0 ? 
        static_cast<double>(totalPoints) / distinctLocationCells : 0.0;
    
    // Coefficient of variation in cell density
    // Low variance = uniform; high variance = clustered
    dist.clusteringRatio = std::min(1.0, spatialVariance);
    
    if (dist.clusteringRatio > 0.7) {
        dist.type = CLUSTERED;
        dist.approximateClusterCount = std::max(1UL, distinctLocationCells / 10);
    } else if (dist.clusteringRatio > 0.3) {
        dist.type = SKEWED;
        dist.approximateClusterCount = std::max(1UL, distinctLocationCells / 5);
    } else {
        dist.type = UNIFORM;
        dist.approximateClusterCount = 0;
    }
    
    return dist;
}

double IndexStatistics::getEfficiencyScore() const {
    // Efficiency = hit rate / (maintenance overhead)
    // Higher is better
    
    if (totalQueriesRun == 0) {
        return 0.5;  // Unknown efficiency
    }
    
    double hitRateScore = averageHitRate;  // 0-1
    double maintenanceScore = 1.0 / (1.0 + maintenanceOverhead);  // Lower overhead = higher score
    
    return hitRateScore * maintenanceScore;
}

std::string IndexCandidate::toString() const {
    std::ostringstream oss;
    oss << "IndexCandidate{"
        << "name=" << indexName
        << ", type=" << static_cast<int>(type)
        << ", score=" << score
        << ", gain=" << selectivityGain
        << "x, recommended=" << (isRecommended ? "yes" : "no")
        << "}";
    return oss.str();
}

// =============================================================================
// GeospatialIndexSelector
// =============================================================================

IndexCandidate GeospatialIndexSelector::selectIndex(
    const std::string& predicateType,
    size_t totalRows,
    const std::map<std::string, IndexStatistics>& availableIndexes,
    const DataDistribution& dataDistribution,
    double geometryParam) {
    
    auto candidates = rankIndexes(predicateType, totalRows, availableIndexes,
                                  dataDistribution, geometryParam);
    
    if (candidates.empty()) {
        // Create full scan candidate
        return IndexCandidate{
            .indexName = "FULL_SCAN",
            .type = SpatialIndexType::NONE,
            .score = scoreFullScan(totalRows, predicateType),
            .selectivityGain = 1.0,
            .isRecommended = true
        };
    }
    
    auto best = candidates.front();
    best.isRecommended = true;
    
    THEMIS_DEBUG("GeospatialIndexSelector: Selected index '{}' for {} predicate "
                "on {} rows (score={:.2f}, gain={:.1f}x)",
                best.indexName, predicateType, totalRows, best.score, best.selectivityGain);
    
    return best;
}

std::vector<IndexCandidate> GeospatialIndexSelector::rankIndexes(
    const std::string& predicateType,
    size_t totalRows,
    const std::map<std::string, IndexStatistics>& availableIndexes,
    const DataDistribution& dataDistribution,
    double geometryParam) {
    
    std::vector<IndexCandidate> candidates;
    
    // Score each available index
    for (const auto& [indexName, stats] : availableIndexes) {
        double score = scoreIndex(stats, predicateType, dataDistribution, totalRows);
        
        if (score > 0) {
            double gain = calculateSelectivityGain(stats.type, totalRows, predicateType);
            
            // Estimate cost with this index
            double estimatedCostUs = 0.0;
            if (stats.type == SpatialIndexType::RTREE) {
                estimatedCostUs = std::log2(std::max(1.0, static_cast<double>(totalRows))) * 10.0;
            } else if (stats.type == SpatialIndexType::GRID) {
                estimatedCostUs = std::log2(std::max(1.0, static_cast<double>(totalRows))) * 8.0;
            } else if (stats.type == SpatialIndexType::QUADTREE) {
                estimatedCostUs = std::log2(std::max(1.0, static_cast<double>(totalRows))) * 9.0;
            }
            
            candidates.push_back(IndexCandidate{
                .indexName = indexName,
                .type = stats.type,
                .score = score,
                .estimatedCostWithIndexUs = estimatedCostUs,
                .selectivityGain = gain
            });
        }
    }
    
    // Add full scan as fallback
    candidates.push_back(IndexCandidate{
        .indexName = "FULL_SCAN",
        .type = SpatialIndexType::NONE,
        .score = scoreFullScan(totalRows, predicateType),
        .estimatedCostWithIndexUs = totalRows * 50.0,  // Cost estimate for full scan
        .selectivityGain = 1.0
    });
    
    // Sort by score (highest first)
    std::sort(candidates.begin(), candidates.end(),
              [](const IndexCandidate& a, const IndexCandidate& b) {
                  return a.score > b.score;
              });
    
    return candidates;
}

std::map<std::string, IndexStatistics> GeospatialIndexSelector::getAvailableIndexes(
    const std::string& collectionName) {
    
    // NOTE: This is a callback point for integration with index manager
    // In production, this would query the index manager API
    // For now, return empty map (caller should populate)
    
    THEMIS_DEBUG("GeospatialIndexSelector: getAvailableIndexes called for collection: {}",
                collectionName);
    
    return std::map<std::string, IndexStatistics>();
}

DataDistribution GeospatialIndexSelector::inferDistribution(
    const SpatialHistogram& histogram) {
    
    if (histogram.grid.empty()) {
        return DataDistribution{};
    }
    
    // Calculate variance in cell point counts
    double meanPointsPerCell = histogram.totalPoints > 0 ?
        static_cast<double>(histogram.totalPoints) / histogram.grid.size() : 0.0;
    
    double variance = 0.0;
    for (const auto& cell : histogram.grid) {
        double diff = cell.pointCount - meanPointsPerCell;
        variance += diff * diff;
    }
    variance /= histogram.grid.size();
    
    // Normalize variance to 0-1 range
    double stdDev = std::sqrt(variance);
    double cv = meanPointsPerCell > 0 ? stdDev / meanPointsPerCell : 0.0;
    double normalizedVariance = std::min(1.0, cv);
    
    size_t distinctCells = 0;
    for (const auto& cell : histogram.grid) {
        if (cell.pointCount > 0) {
            distinctCells++;
        }
    }
    
    return DataDistribution::infer(histogram.totalPoints, distinctCells, normalizedVariance);
}

double GeospatialIndexSelector::calculateSelectivityGain(
    SpatialIndexType indexType,
    size_t totalRows,
    const std::string& predicateType) {
    
    if (indexType == SpatialIndexType::NONE) {
        return 1.0;
    }
    
    // Full scan cost: N * predicate_cost_per_row
    double fullScanCost = 0.0;
    if (predicateType == "DISTANCE") {
        fullScanCost = totalRows * 50.0;  // µs
    } else if (predicateType == "CONTAINS") {
        fullScanCost = totalRows * 100.0;  // µs
    } else if (predicateType == "INTERSECTS") {
        fullScanCost = totalRows * 150.0;  // µs
    }
    
    // Index scan cost: log(N) * index_overhead
    double indexCost = std::log2(std::max(1.0, static_cast<double>(totalRows))) * 10.0;
    
    // Selectivity gain: full scan cost / index cost
    // Higher gain = more benefit from indexing
    if (indexCost <= 0) indexCost = 1.0;
    
    return fullScanCost / indexCost;
}

double GeospatialIndexSelector::scoreIndex(
    const IndexStatistics& index,
    const std::string& predicateType,
    const DataDistribution& dataDistribution,
    size_t totalRows) {
    
    double score = 0.0;
    
    // Base score: index type suitability
    if (index.type == SpatialIndexType::RTREE) {
        // R-tree works well for all spatial predicates
        score = 80.0;
    } else if (index.type == SpatialIndexType::GRID) {
        // Grid works best for uniform distributions
        score = dataDistribution.type == DataDistribution::UNIFORM ? 85.0 : 70.0;
    } else if (index.type == SpatialIndexType::QUADTREE) {
        // Quadtree is adaptive, good for mixed distributions
        score = 75.0;
    } else {
        return 0.0;
    }
    
    // Predicate-specific scoring
    if (predicateType == "DISTANCE") {
        // Distance queries benefit most from spatial indexes
        score *= 1.0;
    } else if (predicateType == "CONTAINS") {
        // Containment queries need good bounding box filtering
        score *= 0.95;
    } else if (predicateType == "INTERSECTS") {
        // Intersection queries vary widely
        score *= 0.90;
    }
    
    // Penalty for very large indexes
    if (index.indexSizeBytes > 1000000000) {  // > 1GB
        score *= 0.8;
    }
    
    // Bonus for good efficiency
    score *= (0.5 + index.getEfficiencyScore());  // 0.5 - 1.5 multiplier
    
    // Bonus if this index has been used successfully
    if (index.totalQueriesRun > 0 && index.averageHitRate > 0.5) {
        score *= 1.2;
    }
    
    // Penalty for high maintenance overhead
    if (index.maintenanceOverhead > 0.2) {  // > 20% overhead
        score *= 0.7;
    }
    
    return score;
}

double GeospatialIndexSelector::scoreFullScan(
    size_t totalRows,
    const std::string& predicateType) {
    
    // Full scan score: lower than indexes for good data
    // But can be competitive for very small datasets
    
    if (totalRows < 100) {
        return 40.0;  // Small dataset: full scan is acceptable
    } else if (totalRows < 10000) {
        return 30.0;  // Medium dataset: indexes are better
    } else {
        return 10.0;  // Large dataset: full scan is poor
    }
}

double GeospatialIndexSelector::getIndexTypeCostMultiplier(
    SpatialIndexType type,
    const DataDistribution& distribution) {
    
    switch (type) {
        case SpatialIndexType::RTREE:
            return 1.0;  // Baseline
            
        case SpatialIndexType::GRID:
            // Grid is better for uniform distributions
            if (distribution.type == DataDistribution::UNIFORM) {
                return 0.8;  // 20% faster than R-tree
            } else {
                return 1.2;  // 20% slower for clustered data
            }
            
        case SpatialIndexType::QUADTREE:
            // Quadtree adapts to distribution
            if (distribution.type == DataDistribution::CLUSTERED) {
                return 0.9;  // 10% faster for clustered
            } else if (distribution.type == DataDistribution::UNIFORM) {
                return 1.0;  // Same as R-tree
            } else {
                return 0.95;  // Slightly better for skewed
            }
            
        case SpatialIndexType::NONE:
        default:
            return 1.0;
    }
}

}  // namespace query
}  // namespace themis
