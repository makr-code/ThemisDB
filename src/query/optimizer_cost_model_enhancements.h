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
     * @brief Estimate Selectivity.
     * @param[in] predicateType Input parameter.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    double estimateSelectivity(const std::string& predicateType,
                              const std::vector<double>& values) const;
    
    /**
     * @brief Get Distinct Values.
     * @return Return value.
     */
    size_t getDistinctValues() const;
};

struct ColumnCorrelation {
    std::string column1;
    std::string column2;
    double correlationCoefficient = 0.0;  // -1.0 to 1.0
    size_t sampleSize = 0;
    bool isPositive() const { return correlationCoefficient > 0.2; }
    bool isNegative() const { return correlationCoefficient < -0.2; }
    bool isIndependent() const { return std::abs(correlationCoefficient) <= 0.2; }
};

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
     * @brief Compute MAPE.
     * @return Return value.
     */
    double computeMAPE() const;
    
    /**
     * @brief Compute P95 Error.
     * @return Return value.
     */
    double computeP95Error() const;
    
    /**
     * @brief Has Systematic Underestimation.
     * @return True when the operation succeeds.
     */
    bool hasSystematicUnderestimation() const;
    
    /**
     * @brief Has Systematic Overestimation.
     * @return True when the operation succeeds.
     */
    bool hasSystematicOverestimation() const;
};

class CostModelEnhancements {
public:
    /**
     * @brief Estimate Selectivity With Histogram.
     * @param[in] histogram Input parameter.
     * @param[in] predicateType Input parameter.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static double estimateSelectivityWithHistogram(
        const ColumnHistogram& histogram,
        const std::string& predicateType,
        const std::vector<double>& values);
    
    static size_t estimateJoinCardinalityWithCorrelation(
        size_t leftRows,
        size_t rightRows,
        double baseSelectivity,
        const ColumnCorrelation* correlation = nullptr);
    
    static double estimateMultiColumnSelectivity(
        const std::vector<ColumnHistogram>& histograms,
        const std::vector<std::pair<std::string, std::string>>& predicates,
        const std::vector<ColumnCorrelation>& correlations);
    
    /**
     * @brief Record Estimate.
     * @param[in] actual Input parameter.
     * @param[in] estimate Input parameter.
     * @param[in] queryTemplate Input parameter.
     * @param[in] operationType Input parameter.
     */
    static void recordEstimate(
        size_t actual,
        size_t estimate,
        const std::string& queryTemplate,
        const std::string& operationType);
    
    /**
     * @brief Get Estimate Metrics.
     * @return Return value.
     */
    static const EstimateValidation& getEstimateMetrics();
    
    /**
     * @brief Clear Estimate Metrics.
     */
    static void clearEstimateMetrics();
};

}  // namespace query
}  // namespace themis
