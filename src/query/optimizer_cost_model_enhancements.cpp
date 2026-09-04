/**
 * @file optimizer_cost_model_enhancements.cpp
 * @brief Cost model refinement implementation for Phase 2
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 2 Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 2: Optimizer and Planning Hardening
 */

#include "query/optimizer_cost_model_enhancements.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace themis {
namespace query {

// =============================================================================
// ColumnHistogram
// =============================================================================

double ColumnHistogram::estimateSelectivity(
    const std::string& predicateType,
    const std::vector<double>& values) const {
    
    if (buckets.empty() || totalRows == 0) {
        // Fallback: uniform distribution
        if (predicateType == "=") {
          return 1.0 / std::max(1.0, static_cast<double>(totalRows));
        }
        return 0.1;  // Default selectivity for range predicates
    }
    
    if (predicateType == "=") {
        // Equality: estimate based on distinct value count in relevant bucket
        if (values.empty()) {
          return 0.0;
        }
        double value = values[0];
        
        for (const auto& bucket : buckets) {
            if (value >= bucket.rangeStart && value < bucket.rangeEnd) {
                // Uniform distribution within bucket
                double bucketSelectivity = static_cast<double>(bucket.frequency) / totalRows;
                double inBucketSelectivity = bucketSelectivity / std::max(1.0, static_cast<double>(bucket.distinctValues));
                return inBucketSelectivity;
            }
        }
        return 1.0 / std::max(1.0, static_cast<double>(totalRows));
    }
    
    if (predicateType == "<" && !values.empty()) {
        double threshold = values[0];
        double selectivity = 0.0;
        
        for (const auto& bucket : buckets) {
            if (bucket.rangeEnd <= threshold) {
                selectivity += static_cast<double>(bucket.frequency) / totalRows;
            } else if (bucket.rangeStart < threshold && bucket.rangeEnd > threshold) {
                // Partial overlap with threshold
                double overlap = (threshold - bucket.rangeStart) / (bucket.rangeEnd - bucket.rangeStart);
                selectivity += (static_cast<double>(bucket.frequency) / totalRows) * overlap;
            }
        }
        return std::clamp(selectivity, 0.0, 1.0);
    }
    
    if (predicateType == ">" && !values.empty()) {
        return 1.0 - estimateSelectivity("<", values);
    }
    
    if (predicateType == "<=" && !values.empty()) {
        auto vals = values;
        vals[0] += 1e-10;  // Tiny increment for closed bound
        return estimateSelectivity("<", vals);
    }
    
    if (predicateType == ">=" && !values.empty()) {
        auto vals = values;
        vals[0] -= 1e-10;
        return estimateSelectivity(">", vals);
    }
    
    if (predicateType == "BETWEEN" && values.size() >= 2) {
        double lower = values[0];
        double upper = values[1];
        if (lower > upper) {
          std::swap(lower, upper);
        }
        
        double selectivity = 0.0;
        for (const auto& bucket : buckets) {
            if (bucket.rangeEnd <= lower || bucket.rangeStart >= upper) {
                continue;  // No overlap
            }
            double bucketSelectivity = static_cast<double>(bucket.frequency) / totalRows;
            
            // Calculate overlap fraction
            double overlapStart = std::max(bucket.rangeStart, lower);
            double overlapEnd = std::min(bucket.rangeEnd, upper);
            double bucketWidth = bucket.rangeEnd - bucket.rangeStart;
            if (bucketWidth > 0) {
                double overlapFraction = (overlapEnd - overlapStart) / bucketWidth;
                selectivity += bucketSelectivity * overlapFraction;
            }
        }
        return std::clamp(selectivity, 0.0, 1.0);
    }
    
    // Default fallback for unknown predicate types
    return 0.1;
}

size_t ColumnHistogram::getDistinctValues() const {
    size_t total = 0;
    for (const auto& bucket : buckets) {
        total += bucket.distinctValues;
    }
    return std::max<size_t>(size_t{1}, total);
}

// =============================================================================
// EstimateValidation
// =============================================================================

double EstimateValidation::computeMAPE() const {
    if (samples.empty()) {
      return 0.0;
    }
    
    double sum = 0.0;
    for (const auto& sample : samples) {
        sum += sample.getError();
    }
    return sum / samples.size();
}

double EstimateValidation::computeP95Error() const {
    if (samples.empty()) {
      return 0.0;
    }
    
    std::vector<double> errors;
    for (const auto& sample : samples) {
        errors.push_back(sample.getError());
    }
    std::sort(errors.begin(), errors.end());
    
    size_t idx = static_cast<size_t>(errors.size() * 0.95);
    if (idx >= errors.size()) {
      idx = errors.size() - 1;
    }
    return errors[idx];
}

bool EstimateValidation::hasSystematicUnderestimation() const {
    if (samples.size() < 5) {
      return false;
    }

    std::vector<double> ratios;
    for (const auto& sample : samples) {
        if (sample.actualRows > 0) {
            ratios.push_back(static_cast<double>(sample.actualRows) /
                             std::max<size_t>(size_t{1}, sample.estimatedRows));
        }
    }
    
    if (ratios.size() < 5) {
      return false;
    }
    std::sort(ratios.begin(), ratios.end());
    double median = ratios[ratios.size() / 2];
    return median > 1.5;
}

bool EstimateValidation::hasSystematicOverestimation() const {
    if (samples.size() < 5) {
      return false;
    }

    std::vector<double> ratios;
    for (const auto& sample : samples) {
        if (sample.actualRows > 0) {
            ratios.push_back(static_cast<double>(sample.actualRows) /
                             std::max<size_t>(size_t{1}, sample.estimatedRows));
        }
    }
    
    if (ratios.size() < 5) {
      return false;
    }
    std::sort(ratios.begin(), ratios.end());
    double median = ratios[ratios.size() / 2];
    return median < 0.67;  // 1/1.5
}

// =============================================================================
// CostModelEnhancements - Static implementation
// =============================================================================

static EstimateValidation g_estimate_validation;

double CostModelEnhancements::estimateSelectivityWithHistogram(
    const ColumnHistogram& histogram,
    const std::string& predicateType,
    const std::vector<double>& values) {
    return histogram.estimateSelectivity(predicateType, values);
}

size_t CostModelEnhancements::estimateJoinCardinalityWithCorrelation(
    size_t leftRows,
    size_t rightRows,
    double baseSelectivity,
    const ColumnCorrelation* correlation) {
    
    double adjustedSelectivity = baseSelectivity;
    
    if (correlation != nullptr) {
        if (correlation->isPositive()) {
            // Positive correlation: increase selectivity (more matches)
            adjustedSelectivity *= 1.3;
        } else if (correlation->isNegative()) {
            // Negative correlation: decrease selectivity (fewer matches)
            adjustedSelectivity *= 0.7;
        }
        // Independent columns: no adjustment
    }
    
    adjustedSelectivity = std::clamp(adjustedSelectivity, 0.0, 1.0);
    
    double result = static_cast<double>(leftRows) * static_cast<double>(rightRows) * adjustedSelectivity;
    if (result > static_cast<double>(std::numeric_limits<size_t>::max())) {
        return std::numeric_limits<size_t>::max();
    }
    return static_cast<size_t>(result);
}

double CostModelEnhancements::estimateMultiColumnSelectivity(
    const std::vector<ColumnHistogram>& histograms,
    const std::vector<std::pair<std::string, std::string>>& predicates,
    const std::vector<ColumnCorrelation>& correlations) {
    
    if (predicates.empty()) {
      return 1.0;
    }
    
    // Create a map of column -> histogram for quick lookup
    std::map<std::string, const ColumnHistogram*> histMap;
    for (const auto& hist : histograms) {
        histMap[hist.columnName] = &hist;
    }
    
    double combinedSelectivity = 1.0;
    
    for (const auto& [colName, predType] : predicates) {
        auto it = histMap.find(colName);
        if (it != histMap.end()) {
            double selec = it->second->estimateSelectivity(predType, {});
            combinedSelectivity *= selec;
        } else {
            // Fallback to heuristic
            combinedSelectivity *= 0.1;
        }
    }
    
    // Check for correlations between predicate columns
    if (predicates.size() >= 2) {
        for (const auto& corr : correlations) {
            // See if this correlation involves multiple predicate columns
            bool col1_involved = false, col2_involved = false;
            for (const auto& [colName, _] : predicates) {
                if (colName == corr.column1) {
                  col1_involved = true;
                }
                if (colName == corr.column2) {
                  col2_involved = true;
                }
            }
            
            if (col1_involved && col2_involved) {
                if (corr.isPositive()) {
                    combinedSelectivity *= 1.1;  // Slight adjustment
                } else if (corr.isNegative()) {
                    combinedSelectivity *= 0.9;
                }
            }
        }
    }
    
    return std::clamp(combinedSelectivity, 0.0, 1.0);
}

void CostModelEnhancements::recordEstimate(
    size_t actual,
    size_t estimate,
    const std::string& queryTemplate,
    const std::string& operationType) {
    
    EstimateValidation::Sample sample{
        .estimatedRows = estimate,
        .actualRows = actual,
        .queryTemplate = queryTemplate,
        .operationType = operationType
    };
    
    double error = sample.getError();
    
    // Log if error is significant (> 50%)
    if (error > 0.5) {
        THEMIS_WARN("CostModel: Significant estimate deviation: op={}, template={}, "
                   "estimated={}, actual={}, error={:.1f}%",
                   operationType, queryTemplate, estimate, actual, error * 100.0);
    }
    
    g_estimate_validation.samples.push_back(std::move(sample));
    
    // Periodically check for systematic bias
    if (g_estimate_validation.samples.size() % 50 == 0) {
        if (g_estimate_validation.hasSystematicUnderestimation()) {
            THEMIS_WARN("CostModel: Systematic UNDERESTIMATION detected (median ratio > 1.5)");
        }
        if (g_estimate_validation.hasSystematicOverestimation()) {
            THEMIS_WARN("CostModel: Systematic OVERESTIMATION detected (median ratio < 0.67)");
        }
    }
}

const EstimateValidation& CostModelEnhancements::getEstimateMetrics() {
    return g_estimate_validation;
}

void CostModelEnhancements::clearEstimateMetrics() {
    g_estimate_validation.samples.clear();
}

}  // namespace query
}  // namespace themis
