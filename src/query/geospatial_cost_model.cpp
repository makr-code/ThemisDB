/**
 * @file geospatial_cost_model.cpp
 * @brief Spatial cost estimator implementation for Phase 6C
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2
 */

#include "query/geospatial_cost_model.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {
namespace query {

// =============================================================================
// SpatialHistogram Implementation
// =============================================================================

const SpatialHistogram::GridCell* SpatialHistogram::getCellForPoint(
    double lon, double lat) const {
    
    if (grid.empty()) return nullptr;
    
    for (const auto& cell : grid) {
        if (lon >= cell.minLon && lon < cell.maxLon &&
            lat >= cell.minLat && lat < cell.maxLat) {
            return &cell;
        }
    }
    return nullptr;
}

size_t SpatialHistogram::estimatePointsInBox(
    double minLon, double maxLon,
    double minLat, double maxLat) const {
    
    if (grid.empty() || totalPoints == 0) {
        // Fallback: assume uniform distribution
        double boxArea = (maxLon - minLon) * (maxLat - minLat);
        double globalArea = (globalMaxLon - globalMinLon) * (globalMaxLat - globalMinLat);
        if (globalArea <= 0) return 0;
        return static_cast<size_t>(totalPoints * boxArea / globalArea);
    }
    
    size_t count = 0;
    for (const auto& cell : grid) {
        // Check if cell overlaps with query box
        if (cell.maxLon <= minLon || cell.minLon >= maxLon ||
            cell.maxLat <= minLat || cell.minLat >= maxLat) {
            continue;  // No overlap
        }
        
        // Calculate overlap area
        double overlapMinLon = std::max(cell.minLon, minLon);
        double overlapMaxLon = std::min(cell.maxLon, maxLon);
        double overlapMinLat = std::max(cell.minLat, minLat);
        double overlapMaxLat = std::min(cell.maxLat, maxLat);
        
        double cellArea = (cell.maxLon - cell.minLon) * (cell.maxLat - cell.minLat);
        double overlapArea = (overlapMaxLon - overlapMinLon) * (overlapMaxLat - overlapMinLat);
        
        if (cellArea > 0) {
            double fraction = overlapArea / cellArea;
            count += static_cast<size_t>(cell.pointCount * fraction);
        }
    }
    
    return count;
}

double SpatialHistogram::estimateSpatialSelectivity(
    double minLon, double maxLon,
    double minLat, double maxLat) const {
    
    if (totalPoints == 0) return 0.0;
    
    size_t estimatedPoints = estimatePointsInBox(minLon, maxLon, minLat, maxLat);
    return static_cast<double>(estimatedPoints) / totalPoints;
}

// =============================================================================
// GeospatialCostEstimator Implementation
// =============================================================================

static EstimateValidation g_geospatial_estimates;

GeospatialCostEstimator::CostEstimate GeospatialCostEstimator::estimateDistanceCost(
    size_t totalRows,
    double distanceMeters,
    bool hasRtreeIndex,
    const SpatialHistogram* histogram) {
    
    CostEstimate result;
    result.indexUsed = hasRtreeIndex ? "RTREE" : "FULL_SCAN";
    
    // Estimate selectivity
    result.selectivity = estimateDistanceSelectivity(distanceMeters, histogram);
    result.estimatedRows = static_cast<size_t>(totalRows * result.selectivity);
    
    if (hasRtreeIndex) {
        // R-tree index scan:
        // - Traversal: log(N) * 10µs per level
        // - Result retrieval: k * 5µs per match
        result.cpuCostUs = rtreeTraversalCost(totalRows) + 
                          (result.estimatedRows * 5.0);
        result.ioCostMs = result.estimatedRows * 0.001;  // 1µs per row I/O
    } else {
        // Full scan with distance calculation
        // - Scan: N * 50µs (calculate distance for each point)
        result.cpuCostUs = totalRows * 50.0;
        result.ioCostMs = totalRows * 0.001;
    }
    
    return result;
}

GeospatialCostEstimator::CostEstimate GeospatialCostEstimator::estimateContainsCost(
    size_t totalRows,
    size_t polygonComplexity,
    bool hasRtreeIndex,
    const SpatialHistogram* histogram) {
    
    CostEstimate result;
    result.indexUsed = hasRtreeIndex ? "RTREE" : "FULL_SCAN";
    
    // Estimate selectivity
    result.selectivity = estimateContainsSelectivity(polygonComplexity, histogram);
    result.estimatedRows = static_cast<size_t>(totalRows * result.selectivity);
    
    if (hasRtreeIndex) {
        // R-tree bounding box check + interior check for matches:
        // - Traversal: log(N) * 15µs
        // - Interior check: m * (2µs + polygonComplexity * 0.5µs)
        double checkCost = geometryCheckCost(polygonComplexity);
        result.cpuCostUs = rtreeTraversalCost(totalRows) + 
                          (result.estimatedRows * checkCost);
        result.ioCostMs = result.estimatedRows * 0.001;
    } else {
        // Full scan with point-in-polygon check
        // - Scan: N * (100µs + complexity * 10µs)
        double scanCost = 100.0 + (polygonComplexity * 10.0);
        result.cpuCostUs = totalRows * scanCost;
        result.ioCostMs = totalRows * 0.001;
    }
    
    return result;
}

GeospatialCostEstimator::CostEstimate GeospatialCostEstimator::estimateIntersectsCost(
    size_t totalRows,
    size_t queryGeometryComplexity,
    bool hasRtreeIndex,
    const SpatialHistogram* histogram) {
    
    CostEstimate result;
    result.indexUsed = hasRtreeIndex ? "RTREE" : "FULL_SCAN";
    
    // Estimate selectivity
    result.selectivity = estimateIntersectsSelectivity(queryGeometryComplexity, histogram);
    result.estimatedRows = static_cast<size_t>(totalRows * result.selectivity);
    
    if (hasRtreeIndex) {
        // R-tree overlap detection:
        // - Traversal: log(N) * 20µs
        // - Intersection check: m * (3µs + complexity * 1µs)
        double checkCost = geometryCheckCost(queryGeometryComplexity);
        result.cpuCostUs = rtreeTraversalCost(totalRows) + 
                          (result.estimatedRows * (3.0 + checkCost));
        result.ioCostMs = result.estimatedRows * 0.001;
    } else {
        // Full scan with geometry intersection check
        // - Scan: N * (150µs + complexity * 15µs)
        double scanCost = 150.0 + (queryGeometryComplexity * 15.0);
        result.cpuCostUs = totalRows * scanCost;
        result.ioCostMs = totalRows * 0.001;
    }
    
    return result;
}

double GeospatialCostEstimator::estimateSpatialSelectivity(
    const std::string& predicateType,
    double searchRadius,
    size_t geometryComplexity,
    const SpatialHistogram* histogram) {
    
    if (predicateType == "DISTANCE") {
        return estimateDistanceSelectivity(searchRadius, histogram);
    } else if (predicateType == "CONTAINS") {
        return estimateContainsSelectivity(geometryComplexity, histogram);
    } else if (predicateType == "INTERSECTS") {
        return estimateIntersectsSelectivity(geometryComplexity, histogram);
    }
    
    // Unknown predicate type
    return 0.1;
}

SpatialHistogram GeospatialCostEstimator::buildSpatialHistogram(
    const std::vector<std::pair<double, double>>& dataPoints,
    size_t gridDimension) {
    
    SpatialHistogram hist;
    hist.totalPoints = dataPoints.size();
    
    if (dataPoints.empty()) {
        return hist;
    }
    
    // Calculate global bounds
    hist.globalMinLon = hist.globalMaxLon = dataPoints[0].first;
    hist.globalMinLat = hist.globalMaxLat = dataPoints[0].second;
    
    for (const auto& [lon, lat] : dataPoints) {
        hist.globalMinLon = std::min(hist.globalMinLon, lon);
        hist.globalMaxLon = std::max(hist.globalMaxLon, lon);
        hist.globalMinLat = std::min(hist.globalMinLat, lat);
        hist.globalMaxLat = std::max(hist.globalMaxLat, lat);
    }
    
    // Create grid cells
    double lonWidth = (hist.globalMaxLon - hist.globalMinLon) / gridDimension;
    double latHeight = (hist.globalMaxLat - hist.globalMinLat) / gridDimension;
    
    // Handle edge case: single point or very small range
    if (lonWidth <= 0) lonWidth = 1.0;
    if (latHeight <= 0) latHeight = 1.0;
    
    hist.grid.resize(gridDimension * gridDimension);
    
    for (size_t i = 0; i < gridDimension; ++i) {
        for (size_t j = 0; j < gridDimension; ++j) {
            auto& cell = hist.grid[i * gridDimension + j];
            cell.minLon = hist.globalMinLon + (i * lonWidth);
            cell.maxLon = hist.globalMinLon + ((i + 1) * lonWidth);
            cell.minLat = hist.globalMinLat + (j * latHeight);
            cell.maxLat = hist.globalMinLat + ((j + 1) * latHeight);
            cell.pointCount = 0;
        }
    }
    
    // Distribute points to grid cells
    for (const auto& [lon, lat] : dataPoints) {
        size_t lonIdx = static_cast<size_t>((lon - hist.globalMinLon) / lonWidth);
        size_t latIdx = static_cast<size_t>((lat - hist.globalMinLat) / latHeight);
        
        // Clamp to grid bounds (handle edge case at max boundary)
        lonIdx = std::min(lonIdx, gridDimension - 1);
        latIdx = std::min(latIdx, gridDimension - 1);
        
        auto& cell = hist.grid[lonIdx * gridDimension + latIdx];
        cell.pointCount++;
        cell.density = cell.pointCount / (lonWidth * latHeight);
    }
    
    return hist;
}

void GeospatialCostEstimator::recordActualCost(
    const CostEstimate& estimated,
    size_t actualRows,
    double actualCostUs,
    const std::string& predicateType) {
    
    EstimateValidation::Sample sample{
        .estimatedRows = estimated.estimatedRows,
        .actualRows = actualRows,
        .queryTemplate = "ST_" + predicateType,
        .operationType = "spatial"
    };
    
    double error = sample.getError();
    
    if (error > 0.5) {
        THEMIS_WARN("GeospatialCostModel: Significant estimate deviation: "
                   "predicate={}, estimated={}, actual={}, error={:.1f}%",
                   predicateType, estimated.estimatedRows, actualRows, error * 100.0);
    }
    
    g_geospatial_estimates.samples.push_back(std::move(sample));
}

const EstimateValidation& GeospatialCostEstimator::getMetrics() {
    return g_geospatial_estimates;
}

void GeospatialCostEstimator::clearMetrics() {
    g_geospatial_estimates.samples.clear();
}

// =============================================================================
// Private Helper Methods
// =============================================================================

double GeospatialCostEstimator::estimateDistanceSelectivity(
    double distanceMeters,
    const SpatialHistogram* histogram) {
    
    if (histogram) {
        // Convert distance (meters) to approximate degrees
        // At equator: 1 degree ≈ 111 km
        double distanceDegrees = distanceMeters / 111000.0;
        
        // Search area: circle with radius distanceDegrees
        double searchArea = M_PI * distanceDegrees * distanceDegrees;
        
        // Global area estimate
        double globalLonRange = histogram->globalMaxLon - histogram->globalMinLon;
        double globalLatRange = histogram->globalMaxLat - histogram->globalMinLat;
        double globalArea = globalLonRange * globalLatRange;
        
        if (globalArea <= 0) return 0.001;  // Fallback
        
        double selectivity = searchArea / globalArea;
        return std::clamp(selectivity, 0.0001, 0.5);  // Reasonable bounds
    }
    
    // Heuristic without histogram
    // Typical distance queries: 1km → 0.5%, 10km → 5%, 100km → 20%
    if (distanceMeters < 1000) {
        return 0.005;  // 0.5%
    } else if (distanceMeters < 10000) {
        return 0.05;   // 5%
    } else if (distanceMeters < 100000) {
        return 0.2;    // 20%
    } else {
        return 0.4;    // 40%
    }
}

double GeospatialCostEstimator::estimateContainsSelectivity(
    size_t polygonComplexity,
    const SpatialHistogram* histogram) {
    
    // Selectivity depends on polygon area
    // Typical polygons (city boundaries, etc.): 1-5% containment
    
    if (histogram) {
        // If we knew the polygon area, we could be more precise
        // For now, use complexity as proxy for area
        double selectivity = 0.01 * std::log(std::max(1.0, static_cast<double>(polygonComplexity)));
        return std::clamp(selectivity, 0.001, 0.3);
    }
    
    // Heuristic: complexity 3-10 vertices → 1%, higher complexity → higher selectivity
    return 0.01 * std::log(std::max(1.0, static_cast<double>(polygonComplexity)));
}

double GeospatialCostEstimator::estimateIntersectsSelectivity(
    size_t queryGeometryComplexity,
    const SpatialHistogram* histogram) {
    
    // Intersection selectivity is typically higher than containment
    // because it includes partial overlaps
    
    if (histogram) {
        double selectivity = 0.02 * std::log(std::max(1.0, static_cast<double>(queryGeometryComplexity)));
        return std::clamp(selectivity, 0.005, 0.5);
    }
    
    // Heuristic: similar to containment but ~2x higher
    return 0.02 * std::log(std::max(1.0, static_cast<double>(queryGeometryComplexity)));
}

double GeospatialCostEstimator::rtreeTraversalCost(size_t totalRows) {
    // R-tree: O(log N) traversal
    // Cost per level: approximately 10 microseconds (node access + comparison)
    if (totalRows == 0) return 0.0;
    
    double logN = std::log2(totalRows);
    return logN * 10.0;  // µs
}

double GeospatialCostEstimator::geometryCheckCost(size_t complexity) {
    // Basic geometry check: point-in-polygon using ray casting
    // Cost: O(complexity) with ~0.5µs per vertex
    if (complexity == 0) return 2.0;  // Minimum cost for simple checks
    
    return 2.0 + (complexity * 0.5);  // µs
}

}  // namespace query
}  // namespace themis
