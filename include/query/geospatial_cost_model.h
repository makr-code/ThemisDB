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

struct SpatialHistogram {
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
     * @brief Get Cell For Point.
     * @param[in] lon Input parameter.
     * @param[in] lat Input parameter.
     * @return Pointer to the result.
     */
    const GridCell* getCellForPoint(double lon, double lat) const;
    
    /**
     * @brief Estimate Points In Box.
     * @param[in] minLon Input parameter.
     * @param[in] maxLon Input parameter.
     * @param[in] minLat Input parameter.
     * @param[in] maxLat Input parameter.
     * @return Return value.
     */
    size_t estimatePointsInBox(double minLon, double maxLon, 
                              double minLat, double maxLat) const;
    
    /**
     * @brief Estimate Spatial Selectivity.
     * @param[in] minLon Input parameter.
     * @param[in] maxLon Input parameter.
     * @param[in] minLat Input parameter.
     * @param[in] maxLat Input parameter.
     * @return Return value.
     */
    double estimateSpatialSelectivity(double minLon, double maxLon,
                                     double minLat, double maxLat) const;
};

class GeospatialCostEstimator {
public:
    struct CostEstimate {
        double cpuCostUs = 0.0;      // CPU cost in microseconds
        double ioCostMs = 0.0;       // I/O cost in milliseconds
        size_t estimatedRows = 0;    // Estimated result rows
        double selectivity = 0.0;    // Predicate selectivity (0.0-1.0)
        std::string indexUsed;       // Index name or "FULL_SCAN"
    };
    
    static CostEstimate estimateDistanceCost(
        size_t totalRows,
        double distanceMeters,
        bool hasRtreeIndex = false,
        const SpatialHistogram* histogram = nullptr);
    
    static CostEstimate estimateContainsCost(
        size_t totalRows,
        size_t polygonComplexity,
        bool hasRtreeIndex = false,
        const SpatialHistogram* histogram = nullptr);
    
    static CostEstimate estimateIntersectsCost(
        size_t totalRows,
        size_t queryGeometryComplexity,
        bool hasRtreeIndex = false,
        const SpatialHistogram* histogram = nullptr);
    
    static double estimateSpatialSelectivity(
        const std::string& predicateType,  // "DISTANCE", "CONTAINS", "INTERSECTS"
        double searchRadius,               // For DISTANCE
        size_t geometryComplexity,         // For CONTAINS/INTERSECTS
        const SpatialHistogram* histogram = nullptr);
    
    static SpatialHistogram buildSpatialHistogram(
        const std::vector<std::pair<double, double>>& dataPoints,
        size_t gridDimension = 10);
    
    /**
     * @brief Record Actual Cost.
     * @param[in] estimated Input parameter.
     * @param[in] actualRows Input parameter.
     * @param[in] actualCostUs Input parameter.
     * @param[in] predicateType Input parameter.
     */
    static void recordActualCost(
        const CostEstimate& estimated,
        size_t actualRows,
        double actualCostUs,
        const std::string& predicateType);
    
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    static const EstimateValidation& getMetrics();
    
    /**
     * @brief Clear Metrics.
     */
    static void clearMetrics();
    
private:
    /**
     * @brief Estimate Distance Selectivity.
     * @param[in] distanceMeters Input parameter.
     * @param[in] histogram Input parameter.
     * @return Return value.
     */
    static double estimateDistanceSelectivity(
        double distanceMeters,
        const SpatialHistogram* histogram);
    
    /**
     * @brief Estimate Contains Selectivity.
     * @param[in] polygonComplexity Input parameter.
     * @param[in] histogram Input parameter.
     * @return Return value.
     */
    static double estimateContainsSelectivity(
        size_t polygonComplexity,
        const SpatialHistogram* histogram);
    
    /**
     * @brief Estimate Intersects Selectivity.
     * @param[in] queryGeometryComplexity Input parameter.
     * @param[in] histogram Input parameter.
     * @return Return value.
     */
    static double estimateIntersectsSelectivity(
        size_t queryGeometryComplexity,
        const SpatialHistogram* histogram);
    
    /**
     * @brief Rtree Traversal Cost.
     * @param[in] totalRows Input parameter.
     * @return Return value.
     */
    static double rtreeTraversalCost(size_t totalRows);
    
    /**
     * @brief Geometry Check Cost.
     * @param[in] complexity Input parameter.
     * @return Return value.
     */
    static double geometryCheckCost(size_t complexity);
};

}  // namespace query
}  // namespace themis
