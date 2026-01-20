#include <gtest/gtest.h>
#include "query/statistical_aggregator.h"
#include <nlohmann/json.hpp>
#include <cmath>

using namespace themis::query;
using json = nlohmann::json;

class StatisticalAggregatorTest : public ::testing::Test {
protected:
    // Helper: Vergleicht Doubles mit Epsilon
    bool doubleEquals(double a, double b, double epsilon = 0.0001) {
        return std::abs(a - b) < epsilon;
    }
};

// ============================================================================
// PERCENTILE Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, PercentileBasic) {
    std::vector<double> values = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    
    // 50th percentile (median) = 55
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    ASSERT_TRUE(p50.has_value());
    EXPECT_TRUE(doubleEquals(p50->get<double>(), 55.0));
    
    // 25th percentile (Q1) = 32.5
    auto p25 = StatisticalAggregator::calculatePercentile(values, 25.0);
    ASSERT_TRUE(p25.has_value());
    EXPECT_TRUE(doubleEquals(p25->get<double>(), 32.5));
    
    // 75th percentile (Q3) = 77.5
    auto p75 = StatisticalAggregator::calculatePercentile(values, 75.0);
    ASSERT_TRUE(p75.has_value());
    EXPECT_TRUE(doubleEquals(p75->get<double>(), 77.5));
    
    // 95th percentile = 95.5
    auto p95 = StatisticalAggregator::calculatePercentile(values, 95.0);
    ASSERT_TRUE(p95.has_value());
    EXPECT_TRUE(doubleEquals(p95->get<double>(), 95.5));
    
    // 0th percentile (min) = 10
    auto p0 = StatisticalAggregator::calculatePercentile(values, 0.0);
    ASSERT_TRUE(p0.has_value());
    EXPECT_TRUE(doubleEquals(p0->get<double>(), 10.0));
    
    // 100th percentile (max) = 100
    auto p100 = StatisticalAggregator::calculatePercentile(values, 100.0);
    ASSERT_TRUE(p100.has_value());
    EXPECT_TRUE(doubleEquals(p100->get<double>(), 100.0));
}

TEST_F(StatisticalAggregatorTest, PercentileEmpty) {
    std::vector<double> values = {};
    
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    EXPECT_FALSE(p50.has_value());
    EXPECT_EQ(p50.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
}

TEST_F(StatisticalAggregatorTest, PercentileSingleValue) {
    std::vector<double> values = {42.0};
    
    auto p50 = StatisticalAggregator::calculatePercentile(values, 50.0);
    ASSERT_TRUE(p50.has_value());
    EXPECT_TRUE(doubleEquals(p50->get<double>(), 42.0));
}

TEST_F(StatisticalAggregatorTest, PercentileInvalidRange) {
    std::vector<double> values = {10, 20, 30};
    
    // percentile < 0
    auto pNeg = StatisticalAggregator::calculatePercentile(values, -10.0);
    EXPECT_FALSE(pNeg.has_value());
    EXPECT_EQ(pNeg.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
    
    // percentile > 100
    auto pOver = StatisticalAggregator::calculatePercentile(values, 150.0);
    EXPECT_FALSE(pOver.has_value());
    EXPECT_EQ(pOver.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
}

// ============================================================================
// MEDIAN Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, MedianOddCount) {
    std::vector<double> values = {10, 20, 30, 40, 50};
    
    auto median = StatisticalAggregator::calculateMedian(values);
    ASSERT_TRUE(median.has_value());
    EXPECT_TRUE(doubleEquals(median->get<double>(), 30.0));
}

TEST_F(StatisticalAggregatorTest, MedianEvenCount) {
    std::vector<double> values = {10, 20, 30, 40};
    
    auto median = StatisticalAggregator::calculateMedian(values);
    ASSERT_TRUE(median.has_value());
    EXPECT_TRUE(doubleEquals(median->get<double>(), 25.0));  // (20 + 30) / 2
}

TEST_F(StatisticalAggregatorTest, MedianUnsorted) {
    std::vector<double> values = {50, 10, 30, 20, 40};
    
    auto median = StatisticalAggregator::calculateMedian(values);
    ASSERT_TRUE(median.has_value());
    EXPECT_TRUE(doubleEquals(median->get<double>(), 30.0));
}

// ============================================================================
// VARIANCE Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, VarianceSample) {
    // Values: [2, 4, 4, 4, 5, 5, 7, 9]
    // Mean = 5
    // Sample Variance = sum((x - mean)^2) / (n - 1)
    //                 = (9 + 1 + 1 + 1 + 0 + 0 + 4 + 16) / 7
    //                 = 32 / 7 ≈ 4.571
    std::vector<double> values = {2, 4, 4, 4, 5, 5, 7, 9};
    
    auto variance = StatisticalAggregator::calculateVariance(values);
    ASSERT_TRUE(variance.has_value());
    EXPECT_TRUE(doubleEquals(variance->get<double>(), 32.0 / 7.0, 0.01));
}

TEST_F(StatisticalAggregatorTest, VariancePopulation) {
    // Values: [2, 4, 4, 4, 5, 5, 7, 9]
    // Mean = 5
    // Population Variance = sum((x - mean)^2) / n
    //                     = 32 / 8 = 4.0
    std::vector<double> values = {2, 4, 4, 4, 5, 5, 7, 9};
    
    auto variance = StatisticalAggregator::calculateVariancePop(values);
    ASSERT_TRUE(variance.has_value());
    EXPECT_TRUE(doubleEquals(variance->get<double>(), 4.0));
}

TEST_F(StatisticalAggregatorTest, VarianceInsufficientData) {
    std::vector<double> singleValue = {42.0};
    
    // Sample variance requires n >= 2
    auto varSample = StatisticalAggregator::calculateVariance(singleValue);
    EXPECT_FALSE(varSample.has_value());
    EXPECT_EQ(varSample.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
    
    // Population variance of single value is 0
    auto varPop = StatisticalAggregator::calculateVariancePop(singleValue);
    ASSERT_TRUE(varPop.has_value());
    EXPECT_TRUE(doubleEquals(varPop->get<double>(), 0.0));
}

TEST_F(StatisticalAggregatorTest, VarianceConstantValues) {
    std::vector<double> values = {5.0, 5.0, 5.0, 5.0};
    
    // All values equal → variance = 0
    auto variance = StatisticalAggregator::calculateVariance(values);
    ASSERT_TRUE(variance.has_value());
    EXPECT_TRUE(doubleEquals(variance->get<double>(), 0.0));
}

// ============================================================================
// STANDARD DEVIATION Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, StdDevSample) {
    // Values: [2, 4, 4, 4, 5, 5, 7, 9]
    // Sample Variance ≈ 4.571
    // Sample StdDev = sqrt(4.571) ≈ 2.138
    std::vector<double> values = {2, 4, 4, 4, 5, 5, 7, 9};
    
    auto stddev = StatisticalAggregator::calculateStdDev(values);
    ASSERT_TRUE(stddev.has_value());
    EXPECT_TRUE(doubleEquals(stddev->get<double>(), std::sqrt(32.0 / 7.0), 0.01));
}

TEST_F(StatisticalAggregatorTest, StdDevPopulation) {
    // Values: [2, 4, 4, 4, 5, 5, 7, 9]
    // Population Variance = 4.0
    // Population StdDev = sqrt(4.0) = 2.0
    std::vector<double> values = {2, 4, 4, 4, 5, 5, 7, 9};
    
    auto stddev = StatisticalAggregator::calculateStdDevPop(values);
    ASSERT_TRUE(stddev.has_value());
    EXPECT_TRUE(doubleEquals(stddev->get<double>(), 2.0));
}

TEST_F(StatisticalAggregatorTest, StdDevEmpty) {
    std::vector<double> values = {};
    
    auto stddev = StatisticalAggregator::calculateStdDev(values);
    EXPECT_FALSE(stddev.has_value());
    EXPECT_EQ(stddev.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
}

// ============================================================================
// RANGE Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, RangeBasic) {
    std::vector<double> values = {10, 50, 20, 80, 30};
    
    // Range = MAX - MIN = 80 - 10 = 70
    auto range = StatisticalAggregator::calculateRange(values);
    ASSERT_TRUE(range.has_value());
    EXPECT_TRUE(doubleEquals(range->get<double>(), 70.0));
}

TEST_F(StatisticalAggregatorTest, RangeEmpty) {
    std::vector<double> values = {};
    
    auto range = StatisticalAggregator::calculateRange(values);
    EXPECT_FALSE(range.has_value());
    EXPECT_EQ(range.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
}

TEST_F(StatisticalAggregatorTest, RangeSingleValue) {
    std::vector<double> values = {42.0};
    
    // Range = 42 - 42 = 0
    auto range = StatisticalAggregator::calculateRange(values);
    ASSERT_TRUE(range.has_value());
    EXPECT_TRUE(doubleEquals(range->get<double>(), 0.0));
}

// ============================================================================
// IQR (Interquartile Range) Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, IQRBasic) {
    std::vector<double> values = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    
    // Q1 (25th) ≈ 32.5, Q3 (75th) ≈ 77.5
    // IQR = Q3 - Q1 = 77.5 - 32.5 = 45
    auto iqr = StatisticalAggregator::calculateIQR(values);
    ASSERT_TRUE(iqr.has_value());
    EXPECT_TRUE(doubleEquals(iqr->get<double>(), 45.0));
}

TEST_F(StatisticalAggregatorTest, IQRInsufficientData) {
    std::vector<double> values = {10, 20, 30};  // < 4 values
    
    auto iqr = StatisticalAggregator::calculateIQR(values);
    EXPECT_FALSE(iqr.has_value());
    EXPECT_EQ(iqr.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
}

// ============================================================================
// MAD (Mean Absolute Deviation) Tests
// ============================================================================

TEST_F(StatisticalAggregatorTest, MADBasic) {
    // Values: [2, 4, 6, 8, 10]
    // Mean = 6
    // MAD = (|2-6| + |4-6| + |6-6| + |8-6| + |10-6|) / 5
    //     = (4 + 2 + 0 + 2 + 4) / 5 = 12 / 5 = 2.4
    std::vector<double> values = {2, 4, 6, 8, 10};
    
    auto mad = StatisticalAggregator::calculateMAD(values);
    ASSERT_TRUE(mad.has_value());
    EXPECT_TRUE(doubleEquals(mad->get<double>(), 2.4));
}

TEST_F(StatisticalAggregatorTest, MADConstantValues) {
    std::vector<double> values = {5.0, 5.0, 5.0, 5.0};
    
    // All values equal → MAD = 0
    auto mad = StatisticalAggregator::calculateMAD(values);
    ASSERT_TRUE(mad.has_value());
    EXPECT_TRUE(doubleEquals(mad->get<double>(), 0.0));
}

TEST_F(StatisticalAggregatorTest, MADEmpty) {
    std::vector<double> values = {};
    
    auto mad = StatisticalAggregator::calculateMAD(values);
    EXPECT_FALSE(mad.has_value());
    EXPECT_EQ(mad.error().code(), themis::errors::ErrorCode::ERR_QUERY_AGGREGATION_FAILED);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(StatisticalAggregatorTest, NegativeValues) {
    std::vector<double> values = {-10, -5, 0, 5, 10};
    
    auto median = StatisticalAggregator::calculateMedian(values);
    ASSERT_TRUE(median.has_value());
    EXPECT_TRUE(doubleEquals(median->get<double>(), 0.0));
    
    auto variance = StatisticalAggregator::calculateVariance(values);
    ASSERT_TRUE(variance.has_value());
    EXPECT_GT(variance->get<double>(), 0.0);
}

TEST_F(StatisticalAggregatorTest, LargeDataset) {
    std::vector<double> values;
    for (int i = 1; i <= 1000; ++i) {
        values.push_back(static_cast<double>(i));
    }
    
    // Median of 1..1000 = 500.5
    auto median = StatisticalAggregator::calculateMedian(values);
    ASSERT_TRUE(median.has_value());
    EXPECT_TRUE(doubleEquals(median->get<double>(), 500.5));
    
    // Mean = 500.5
    auto variance = StatisticalAggregator::calculateVariance(values);
    ASSERT_TRUE(variance.has_value());
    EXPECT_GT(variance->get<double>(), 0.0);
}

TEST_F(StatisticalAggregatorTest, FloatingPointPrecision) {
    std::vector<double> values = {0.1, 0.2, 0.3, 0.4, 0.5};
    
    auto median = StatisticalAggregator::calculateMedian(values);
    ASSERT_TRUE(median.has_value());
    EXPECT_TRUE(doubleEquals(median->get<double>(), 0.3, 0.0001));
}
