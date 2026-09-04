/**
 * @file optimizer_cost_model_enhancements.h
 * @brief Cost model refinement helpers for Phase 2: histogram-based estimation,
 *        multi-column correlation, and estimate validation.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 2 Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 2: Optimizer and Planning Hardening
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

namespace themis {
namespace query {

/**
 * @brief Histogram-based cardinality estimation
 * 
 * Represents a histogram for a column with buckets for better selectivity
 * estimation, replacing the simple uniform distribution assumption.
 */
struct ColumnHistogram {
    struct Bucket {
        double rangeStart = 0;
        double rangeEnd;
        size_t frequency;
        size_t distinctValues;
    };
    
    std::string columnName;
    std::vector<Bucket> buckets;
    size_t totalRows = 0;
    bool isNumeric = false;
    
    /**
     * @brief Estimate selectivity for a range predicate using histogram
     * @param predicateType One of "=", "<", ">", "<=", ">=", "BETWEEN"
     * @param value The predicate value(s)
     * @return Selectivity between 0.0 and 1.0
     */
    double estimateSelectivity(const std::string& predicateType,
                              const std::vector<double>& values) const;
    
    /**
     * @brief Get distinct value count for the column
     * @return Total distinct values across all buckets
     */
    size_t getDistinctValues() const;
};

/**
 * @brief Multi-column correlation metadata
 * 
 * Captures correlations between columns to improve join cardinality and
 * filter selectivity estimation.
 */
struct ColumnCorrelation {
    std::string column1;
    std::string column2;
    double correlationCoefficient = 0.0;  // -1.0 to 1.0
    size_t sampleSize = 0;
    bool isPositive() const { return correlationCoefficient > 0.2; }
    bool isNegative() const { return correlationCoefficient < -0.2; }
    bool isIndependent() const { return std::abs(correlationCoefficient) <= 0.2; }
};

/**
 * @brief Estimate validation metrics
 * 
 * Tracks estimate vs. actual cardinality to detect systematic bias.
 */
struct EstimateValidation {
    struct Sample {
        size_t estimatedRows = 0;
        size_t actualRows = 0;
        std::string queryTemplate = {};
        std::string operationType;  // "scan", "filter", "join", "agg"
        
        double getError() const {
            if (actualRows == 0) {
              return 0.0;
            }
            if (estimatedRows == 0) return 1.0;  // 100% error
            return std::abs(static_cast<double>(estimatedRows) - static_cast<double>(actualRows)) /
                   static_cast<double>(actualRows);
        }
    };
    
    std::vector<Sample> samples;
    
    /**
     * @brief Compute mean absolute percentage error (MAPE)
     */
    double computeMAPE() const;
    
    /**
     * @brief Compute 95th percentile error
     */
    double computeP95Error() const;
    
    /**
     * @brief Check for systematic underestimation
     * @return true if median error ratio > 1.5
     */
    bool hasSystematicUnderestimation() const;
    
    /**
     * @brief Check for systematic overestimation
     * @return true if median error ratio < 0.67 (1/1.5)
     */
    bool hasSystematicOverestimation() const;
};

/**
 * @brief Cost model enhancements for Phase 2
 * 
 * Provides improved cardinality estimation with histograms and correlation
 * awareness, plus validation metrics.
 */
class CostModelEnhancements {
public:
    /**
     * @brief Estimate selectivity using histogram if available, else default
     */
    static double estimateSelectivityWithHistogram(
        const ColumnHistogram& histogram,
        const std::string& predicateType,
        const std::vector<double>& values);
    
    /**
     * @brief Estimate join cardinality with correlation awareness
     * 
     * If columns are correlated positively, adjust selectivity upward
     * (more matches expected). If negatively correlated, adjust downward.
     */
    static size_t estimateJoinCardinalityWithCorrelation(
        size_t leftRows,
        size_t rightRows,
        double baseSelectivity,
        const ColumnCorrelation* correlation = nullptr);
    
    /**
     * @brief Estimate filter selectivity for multi-column predicates
     * 
     * Takes correlation into account when predicates are on related columns.
     */
    static double estimateMultiColumnSelectivity(
        const std::vector<ColumnHistogram>& histograms,
        const std::vector<std::pair<std::string, std::string>>& predicates,
        const std::vector<ColumnCorrelation>& correlations);
    
    /**
     * @brief Validate estimate and log deviation if significant
     * @param actual Observed cardinality
     * @param estimate Previously estimated cardinality
     * @param queryTemplate For logging/diagnostics
     * @param operationType One of "scan", "filter", "join", "agg"
     */
    static void recordEstimate(
        size_t actual,
        size_t estimate,
        const std::string& queryTemplate,
        const std::string& operationType);
    
    /**
     * @brief Get estimate validation metrics
     */
    static const EstimateValidation& getEstimateMetrics();
    
    /**
     * @brief Clear estimate validation history
     */
    static void clearEstimateMetrics();
};

}  // namespace query
}  // namespace themis
