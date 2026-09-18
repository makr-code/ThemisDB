/**
 * @file statistical_aggregator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "utils/expected.h"

namespace themis {
namespace query {


class StatisticalAggregator {
public:
    StatisticalAggregator() = default;
    
    /**
     * @brief Calculate Percentile.
     * @param[in] values Input parameter.
     * @param[in] percentile Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculatePercentile(
        std::vector<double> values,
        double percentile
    );
    
    /**
     * @brief Calculate Median.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateMedian(std::vector<double> values);
    
    /**
     * @brief Calculate Std Dev.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateStdDev(const std::vector<double>& values);
    
    /**
     * @brief Calculate Std Dev Pop.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateStdDevPop(const std::vector<double>& values);
    
    /**
     * @brief Calculate Variance.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateVariance(const std::vector<double>& values);
    
    /**
     * @brief Calculate Variance Pop.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateVariancePop(const std::vector<double>& values);
    
    /**
     * @brief Calculate Range.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateRange(const std::vector<double>& values);
    
    /**
     * @brief Calculate IQR.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateIQR(std::vector<double> values);
    
    /**
     * @brief Calculate MAD.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static Result<nlohmann::json> calculateMAD(const std::vector<double>& values);
    
private:
    /**
     * @brief Calculate Mean.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    static double calculateMean(const std::vector<double>& values);
    
    /**
     * @brief Extract Numeric Values.
     * @param[in] jsonValues Input parameter.
     * @return Return value.
     */
    static std::vector<double> extractNumericValues(
        const std::vector<nlohmann::json>& jsonValues
    );
};

} // namespace query
} // namespace themis
