/**
 * @file geospatial_cost_model.h
 * @brief Spatial cost estimator for Phase 6C: Geospatial Phase 2 optimization
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2 - Optimizer Hints & Performance Hardening
 *
 * Provides cost estimation for spatial predicates (ST_DISTANCE, ST_CONTAINS, ST_INTERSECTS)
 * using histogram-based selectivity and index-aware cost modeling, building on Phase 2
 * optimizer enhancements.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include "query/optimizer_cost_model_enhancements.h"

namespace themis {
namespace query {

/**
 * @brief Histogram for spatial data distributions
 * 
 * Extends ColumnHistogram to support spatial ranges (bounding boxes, grids).
 * Used for estimating selectivity of spatial predicates.
 */
struct SpatialHistogram {
    /**
     * @brief Grid cell for spatial distribution
     */
    struct GridCell {
        double minLon, maxLon;    // Longitude range
        double minLat, maxLat;    // Latitude range
        size_t pointCount = 0;    // Points in this cell
        double density = 0.0;     // Points per square degree
    };
    
    std::string columnName;
    std::vector<GridCell> grid;
    size_t totalPoints = 0;
    
    // Statistics
    double globalMinLon = 180.0, globalMaxLon = -180.0;
    double globalMinLat = 90.0, globalMaxLat = -90.0;
    
    /**
     * @brief Get grid cell containing the point
     */
    const GridCell* getCellForPoint(double lon, double lat) const;
    
    /**
     * @brief Estimate points within a bounding box
     */
    size_t estimatePointsInBox(double minLon, double maxLon, 
                              double minLat, double maxLat) const;
    
    /**
     * @brief Estimate selectivity for spatial predicate
     */
    double estimateSpatialSelectivity(double minLon, double maxLon,
                                     double minLat, double maxLat) const;
};

/**
 * @brief Geospatial cost estimator using Phase 2 framework
 * 
 * Estimates costs for ST_DISTANCE, ST_CONTAINS, ST_INTERSECTS predicates
 * using histogram-based selectivity and index characteristics.
 */
class GeospatialCostEstimator {
public:
    /**
     * @brief Cost estimate result
     */
    struct CostEstimate {
        double cpuCostUs = 0.0;      // CPU cost in microseconds
        double ioCostMs = 0.0;       // I/O cost in milliseconds
        size_t estimatedRows = 0;    // Estimated result rows
        double selectivity = 0.0;    // Predicate selectivity (0.0-1.0)
        std::string indexUsed;       // Index name or "FULL_SCAN"
    };
    
    /**
     * @brief Estimate cost for ST_DISTANCE predicate
     * 
     * @param totalRows Total documents in collection
     * @param distanceMeters Search radius in meters
     * @param hasRtreeIndex Whether R-tree index is available
     * @param histogram Spatial histogram (optional)
     * @return Cost estimate with CPU time, I/O, and selectivity
     * 
     * Example cost model:
     * - With R-tree index: log(N) * 10µs + k * 5µs (k = result count)
     * - Without index: N * 50µs (full scan + distance calc)
     */
    static CostEstimate estimateDistanceCost(
        size_t totalRows,
        double distanceMeters,
        bool hasRtreeIndex = false,
        const SpatialHistogram* histogram = nullptr);
    
    /**
     * @brief Estimate cost for ST_CONTAINS predicate (point-in-polygon)
     * 
     * @param totalRows Total documents in collection
     * @param polygonComplexity Number of vertices in polygon
     * @param hasRtreeIndex Whether R-tree index is available
     * @param histogram Spatial histogram (optional)
     * @return Cost estimate
     * 
     * Example cost model:
     * - With R-tree index: log(N) * 15µs + m * 2µs (m = candidates to check)
     * - Without index: N * (100µs + polygonComplexity * 10µs)
     */
    static CostEstimate estimateContainsCost(
        size_t totalRows,
        size_t polygonComplexity,
        bool hasRtreeIndex = false,
        const SpatialHistogram* histogram = nullptr);
    
    /**
     * @brief Estimate cost for ST_INTERSECTS predicate (geometry overlap)
     * 
     * @param totalRows Total documents in collection
     * @param queryGeometryComplexity Complexity of query geometry
     * @param hasRtreeIndex Whether R-tree index is available
     * @param histogram Spatial histogram (optional)
     * @return Cost estimate
     * 
     * Example cost model:
     * - With R-tree index: log(N) * 20µs + m * 3µs (m = overlap checks)
     * - Without index: N * (150µs + queryGeometryComplexity * 15µs)
     */
    static CostEstimate estimateIntersectsCost(
        size_t totalRows,
        size_t queryGeometryComplexity,
        bool hasRtreeIndex = false,
        const SpatialHistogram* histogram = nullptr);
    
    /**
     * @brief Estimate selectivity using histogram if available
     * 
     * Falls back to heuristic if histogram unavailable.
     */
    static double estimateSpatialSelectivity(
        const std::string& predicateType,  // "DISTANCE", "CONTAINS", "INTERSECTS"
        double searchRadius,               // For DISTANCE
        size_t geometryComplexity,         // For CONTAINS/INTERSECTS
        const SpatialHistogram* histogram = nullptr);
    
    /**
     * @brief Build spatial histogram from sample data
     * 
     * @param dataPoints Vector of {longitude, latitude} pairs
     * @param gridDimension Number of grid cells per dimension (default: 10)
     * @return Spatial histogram for selectivity estimation
     */
    static SpatialHistogram buildSpatialHistogram(
        const std::vector<std::pair<double, double>>& dataPoints,
        size_t gridDimension = 10);
    
    /**
     * @brief Validate cost estimate against actual execution
     * 
     * Records estimate vs. actual for bias detection.
     */
    static void recordActualCost(
        const CostEstimate& estimated,
        size_t actualRows,
        double actualCostUs,
        const std::string& predicateType);
    
    /**
     * @brief Get cost estimation metrics
     */
    static const EstimateValidation& getMetrics();
    
    /**
     * @brief Clear cost metrics
     */
    static void clearMetrics();
    
private:
    /**
     * @brief Estimate selectivity for distance query
     * 
     * Using histogram: interpolate from grid cells within radius.
     * Without histogram: heuristic based on radius size.
     */
    static double estimateDistanceSelectivity(
        double distanceMeters,
        const SpatialHistogram* histogram);
    
    /**
     * @brief Estimate selectivity for containment query
     */
    static double estimateContainsSelectivity(
        size_t polygonComplexity,
        const SpatialHistogram* histogram);
    
    /**
     * @brief Estimate selectivity for intersection query
     */
    static double estimateIntersectsSelectivity(
        size_t queryGeometryComplexity,
        const SpatialHistogram* histogram);
    
    /**
     * @brief R-tree traversal cost in microseconds
     * 
     * log(N) * costPerLevel where costPerLevel ≈ 10µs
     */
    static double rtreeTraversalCost(size_t totalRows);
    
    /**
     * @brief Point geometry check cost
     * 
     * Interior point check cost in microseconds.
     * Varies with geometry complexity (number of vertices).
     */
    static double geometryCheckCost(size_t complexity);
};

}  // namespace query
}  // namespace themis
