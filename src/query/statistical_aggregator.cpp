/**
 * @file statistical_aggregator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/statistical_aggregator.h"
#include "utils/logger.h"
#include <numeric>
#include <cmath>
#include <limits>
#include <fmt/format.h>

namespace themis {
namespace query {

using errors::ErrorCode;

// ============================================================================
// Percentile Calculation
// ============================================================================

Result<nlohmann::json> StatisticalAggregator::calculatePercentile(
    std::vector<double> values,
    double percentile
) {
    if (values.empty()) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
            "Cannot calculate percentile: empty dataset"
        );
    }
    
    if (percentile < 0.0 || percentile > 100.0) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
            fmt::format("Invalid percentile value: {}. Must be between 0 and 100", percentile)
        );
    }
    
    // Sort values
    std::sort(values.begin(), values.end());
    
    if (static_cast<int>(values.size()) == 1) {
        return Ok(nlohmann::json(values[0]));
    }
    
    // Nearest Rank Method
    // Rank = (percentile / 100) * (N - 1)
    double rank = (percentile / 100.0) * (static_cast<int>(values.size()) - 1);
    size_t lowerIndex = static_cast<size_t>(std::floor(rank));
    size_t upperIndex = static_cast<size_t>(std::ceil(rank));
    
    if (lowerIndex == upperIndex) {
        return Ok(nlohmann::json(values[lowerIndex]));
    }
    
    // Linear interpolation
    double weight = rank - lowerIndex;
    double result = values[lowerIndex] * (1.0 - weight) + values[upperIndex] * weight;
    
    return Ok(nlohmann::json(result));
}

Result<nlohmann::json> StatisticalAggregator::calculateMedian(std::vector<double> values) {
    return calculatePercentile(std::move(values), 50.0);
}

// ============================================================================
// Standard Deviation & Variance
// ============================================================================

double StatisticalAggregator::calculateMean(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

Result<nlohmann::json> StatisticalAggregator::calculateVariance(const std::vector<double>& values) {
    if (static_cast<int>(values.size()) < 2) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
            fmt::format("Cannot calculate variance: need at least 2 values, got {}",static_cast<int>(values.size()))
        );
    }
    
    double mean = calculateMean(values);
    
    // Sample variance: sum((x - mean)^2) / (n - 1)
    double sumSquaredDiffs = 0.0;
    for (double val : values) {
        double diff = val - mean;
        sumSquaredDiffs += diff * diff;
    }
    
    return Ok(nlohmann::json(sumSquaredDiffs / (static_cast<int>(values.size()) - 1)));
}

Result<nlohmann::json> StatisticalAggregator::calculateVariancePop(const std::vector<double>& values) {
    if (values.empty()) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
            "Cannot calculate population variance: empty dataset"
        );
    }
    
    if (static_cast<int>(values.size()) == 1) {
        return Ok(nlohmann::json(0.0));  // Population variance of single value is 0
    }
    
    double mean = calculateMean(values);
    
    // Population variance: sum((x - mean)^2) / n
    double sumSquaredDiffs = 0.0;
    for (double val : values) {
        double diff = val - mean;
        sumSquaredDiffs += diff * diff;
    }
    
    return Ok(nlohmann::json(sumSquaredDiffs / values.size()));
}

Result<nlohmann::json> StatisticalAggregator::calculateStdDev(const std::vector<double>& values) {
    auto variance = calculateVariance(values);
    if (!variance) {
        return variance;  // Propagate error
    }
    
    return Ok(nlohmann::json(std::sqrt(variance.value().get<double>())));
}

Result<nlohmann::json> StatisticalAggregator::calculateStdDevPop(const std::vector<double>& values) {
    auto variance = calculateVariancePop(values);
    
    if (!variance) {
        return variance;  // Propagate error
    }
    
    return Ok(nlohmann::json(std::sqrt(variance.value().get<double>())));
}

// ============================================================================
// Additional Statistical Measures
// ============================================================================

Result<nlohmann::json> StatisticalAggregator::calculateRange(const std::vector<double>& values) {
    if (values.empty()) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
            "Cannot calculate range: empty dataset"
        );
    }
    
    auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    return Ok(nlohmann::json(*maxIt - *minIt));
}

Result<nlohmann::json> StatisticalAggregator::calculateIQR(std::vector<double> values) {
    if (static_cast<int>(values.size()) < 4) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
            fmt::format("Cannot calculate IQR: need at least 4 values, got {}",static_cast<int>(values.size()))
        );
    }
    
    auto q1 = calculatePercentile(values, 25.0);
    auto q3 = calculatePercentile(values, 75.0);
    
    if (!q1 || !q3) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Failed to calculate quartiles for IQR"
        );
    }
    
    return Ok(nlohmann::json(q3.value().get<double>() - q1.value().get<double>()));
}

Result<nlohmann::json> StatisticalAggregator::calculateMAD(const std::vector<double>& values) {
    if (values.empty()) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
            "Cannot calculate MAD: empty dataset"
        );
    }
    
    double mean = calculateMean(values);
    
    // Mean Absolute Deviation: sum(|x - mean|) / n
    double sumAbsDiffs = 0.0;
    for (double val : values) {
        sumAbsDiffs += std::abs(val - mean);
    }
    
    return Ok(nlohmann::json(sumAbsDiffs / values.size()));
}

// ============================================================================
// Helper Functions
// ============================================================================

std::vector<double> StatisticalAggregator::extractNumericValues(
    const std::vector<nlohmann::json>& jsonValues
) {
    std::vector<double> result = {};

    result.reserve(jsonValues.size());
    
    for (const auto& val : jsonValues) {
        if (val.is_number()) {
            result.push_back(val.get<double>());
        }
        // Skip non-numeric values (null, string, bool, etc.)
    }
    
    return result;
}

} // namespace query
} // namespace themis
