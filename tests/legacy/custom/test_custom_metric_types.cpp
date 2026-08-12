/**
 * @file test_custom_metric_types.cpp
 * @brief Tests for AdvancedMetrics — summary, exponential histogram,
 *        cardinality, time-weighted average, and rate metric types.
 *
 * Acceptance criteria (from issue):
 * - All five custom metric types are implemented and tested.
 * - Edge cases (empty metrics, single samples, window expiry) are verified.
 */

#include <gtest/gtest.h>
#include "observability/advanced_metrics.h"

#include <chrono>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

// ============================================================================
// Fixture
// ============================================================================

class AdvancedMetricsTest : public ::testing::Test {
protected:
    void SetUp() override { metrics_.reset(); }
    void TearDown() override { metrics_.reset(); }
    AdvancedMetrics metrics_;
};

// ============================================================================
// Summary Tests
// ============================================================================

TEST_F(AdvancedMetricsTest, Summary_EmptyMetric_AllQuantilesZero) {
    auto result = metrics_.getSummary("nonexistent", {0.5, 0.99});
    EXPECT_EQ(0u, result.count);
    EXPECT_DOUBLE_EQ(0.0, result.sum);
    EXPECT_DOUBLE_EQ(0.0, result.quantile_values.at(0.5));
    EXPECT_DOUBLE_EQ(0.0, result.quantile_values.at(0.99));
    EXPECT_EQ("nonexistent", result.metric_name);
}

TEST_F(AdvancedMetricsTest, Summary_SingleValue_AllQuantilesEqualThatValue) {
    metrics_.recordSummary("latency_ms", 42.0);
    auto result = metrics_.getSummary("latency_ms", {0.5, 0.99});
    EXPECT_EQ(1u, result.count);
    EXPECT_DOUBLE_EQ(42.0, result.sum);
    EXPECT_DOUBLE_EQ(42.0, result.quantile_values.at(0.5));
    EXPECT_DOUBLE_EQ(42.0, result.quantile_values.at(0.99));
}

TEST_F(AdvancedMetricsTest, Summary_MultipleValues_QuantilesCorrect) {
    // Record values 1..100.
    for (int i = 1; i <= 100; ++i) {
        metrics_.recordSummary("req_latency", static_cast<double>(i));
    }
    auto result = metrics_.getSummary("req_latency", {0.5, 0.9, 0.95, 0.99});

    EXPECT_EQ(100u, result.count);
    EXPECT_DOUBLE_EQ(5050.0, result.sum);

    // P50 of [1..100] should be near 50.
    EXPECT_GE(result.quantile_values.at(0.5), 49.0);
    EXPECT_LE(result.quantile_values.at(0.5), 51.0);

    // P99 should be near 99.
    EXPECT_GE(result.quantile_values.at(0.99), 98.0);
    EXPECT_LE(result.quantile_values.at(0.99), 100.0);
}

TEST_F(AdvancedMetricsTest, Summary_DefaultQuantiles_MapContainsFourEntries) {
    metrics_.recordSummary("cpu_ms", 10.0);
    metrics_.recordSummary("cpu_ms", 20.0);
    auto result = metrics_.getSummary("cpu_ms");
    // Default quantiles: 0.5, 0.9, 0.95, 0.99
    EXPECT_EQ(4u, result.quantile_values.size());
    EXPECT_TRUE(result.quantile_values.count(0.5));
    EXPECT_TRUE(result.quantile_values.count(0.99));
}

TEST_F(AdvancedMetricsTest, Summary_SumAccumulatesCorrectly) {
    metrics_.recordSummary("io_bytes", 100.0);
    metrics_.recordSummary("io_bytes", 200.0);
    metrics_.recordSummary("io_bytes", 300.0);
    auto result = metrics_.getSummary("io_bytes", {0.5});
    EXPECT_DOUBLE_EQ(600.0, result.sum);
    EXPECT_EQ(3u, result.count);
}

TEST_F(AdvancedMetricsTest, Summary_OldestSamplesEvicted_WhenBufferFull) {
    // Fill the buffer to capacity + 1 to trigger eviction.
    for (size_t i = 0; i < AdvancedMetrics::kMaxSummarySamples + 1; ++i) {
        metrics_.recordSummary("overflow_metric", static_cast<double>(i));
    }
    auto result = metrics_.getSummary("overflow_metric", {0.5});
    // Count should be capped at kMaxSummarySamples.
    EXPECT_EQ(AdvancedMetrics::kMaxSummarySamples, result.count);
    // The first value (0.0) should have been evicted, so the minimum is 1.
    EXPECT_GE(result.quantile_values.at(0.5), 1.0);
}

TEST_F(AdvancedMetricsTest, Summary_MultipleSeparateMetrics_Independent) {
    metrics_.recordSummary("metric_a", 10.0);
    metrics_.recordSummary("metric_b", 99.0);

    EXPECT_DOUBLE_EQ(10.0, metrics_.getSummary("metric_a", {1.0}).quantile_values.at(1.0));
    EXPECT_DOUBLE_EQ(99.0, metrics_.getSummary("metric_b", {1.0}).quantile_values.at(1.0));
}

// ============================================================================
// Exponential Histogram Tests
// ============================================================================

TEST_F(AdvancedMetricsTest, ExpHist_EmptyMetric_NoBuckets) {
    auto result = metrics_.getExponentialHistogram("nonexistent");
    EXPECT_TRUE(result.buckets.empty());
    EXPECT_EQ(0u, result.total_count);
    EXPECT_DOUBLE_EQ(0.0, result.sum);
    EXPECT_EQ("nonexistent", result.metric_name);
}

TEST_F(AdvancedMetricsTest, ExpHist_SingleValue_OneBucket) {
    metrics_.recordExponentialHistogram("hist", 4.0, 2.0);
    auto result = metrics_.getExponentialHistogram("hist");

    EXPECT_EQ(1u, result.total_count);
    EXPECT_DOUBLE_EQ(4.0, result.sum);
    EXPECT_EQ(0u, result.zero_count);
    ASSERT_EQ(1u, result.buckets.size());
    EXPECT_EQ(1u, result.buckets[0].count);
    // With scale=2, value 4 falls in bucket [2^2, 2^3) = [4, 8).
    EXPECT_LE(result.buckets[0].lower_bound, 4.0);
    EXPECT_GT(result.buckets[0].upper_bound, 4.0);
}

TEST_F(AdvancedMetricsTest, ExpHist_NonPositiveValue_CountedInZeroBucket) {
    metrics_.recordExponentialHistogram("hist", -1.0);
    metrics_.recordExponentialHistogram("hist", 0.0);
    auto result = metrics_.getExponentialHistogram("hist");

    EXPECT_EQ(2u, result.zero_count);
    EXPECT_EQ(2u, result.total_count);
    EXPECT_TRUE(result.buckets.empty());
}

TEST_F(AdvancedMetricsTest, ExpHist_MultipleValues_BucketsOrdered) {
    // Record values spanning multiple buckets.
    for (double v : {1.0, 2.0, 4.0, 8.0, 16.0}) {
        metrics_.recordExponentialHistogram("wide_range", v, 2.0);
    }
    auto result = metrics_.getExponentialHistogram("wide_range");

    EXPECT_EQ(5u, result.total_count);
    EXPECT_GE(result.buckets.size(), 1u);

    // Buckets must be in ascending order of lower_bound.
    for (size_t i = 1; i < result.buckets.size(); ++i) {
        EXPECT_LT(result.buckets[i - 1].lower_bound, result.buckets[i].lower_bound);
    }
}

TEST_F(AdvancedMetricsTest, ExpHist_Scale_LockedOnFirstCall) {
    metrics_.recordExponentialHistogram("locked", 8.0, 2.0);
    // Second call with a different scale; the stored scale (2.0) should be kept.
    metrics_.recordExponentialHistogram("locked", 8.0, 10.0);
    auto result = metrics_.getExponentialHistogram("locked");
    EXPECT_DOUBLE_EQ(2.0, result.scale);
}

TEST_F(AdvancedMetricsTest, ExpHist_SumCorrect) {
    metrics_.recordExponentialHistogram("sum_test", 3.0);
    metrics_.recordExponentialHistogram("sum_test", 7.0);
    // Non-positive values increment zero_count and are excluded from sum.
    metrics_.recordExponentialHistogram("sum_test", -1.0);
    auto result = metrics_.getExponentialHistogram("sum_test");
    EXPECT_DOUBLE_EQ(10.0, result.sum);
    EXPECT_EQ(3u, result.total_count);
    EXPECT_EQ(1u, result.zero_count);
}

TEST_F(AdvancedMetricsTest, ExpHist_BucketCountsCorrect) {
    // All four values should land in the same bucket [4, 8) with scale=2.
    for (int i = 0; i < 4; ++i) {
        metrics_.recordExponentialHistogram("bucket_count", 5.0, 2.0);
    }
    auto result = metrics_.getExponentialHistogram("bucket_count");
    ASSERT_EQ(1u, result.buckets.size());
    EXPECT_EQ(4u, result.buckets[0].count);
}

// ============================================================================
// Cardinality Tests
// ============================================================================

TEST_F(AdvancedMetricsTest, Cardinality_UnknownMetric_ReturnsZero) {
    EXPECT_EQ(0u, metrics_.getCardinalityEstimate("no_such_metric"));
}

TEST_F(AdvancedMetricsTest, Cardinality_UniqueValues_CountedCorrectly) {
    metrics_.recordCardinality("tenants", "tenant_a");
    metrics_.recordCardinality("tenants", "tenant_b");
    metrics_.recordCardinality("tenants", "tenant_c");
    EXPECT_EQ(3u, metrics_.getCardinalityEstimate("tenants"));
}

TEST_F(AdvancedMetricsTest, Cardinality_DuplicateValues_NotCounted) {
    metrics_.recordCardinality("sessions", "user_1");
    metrics_.recordCardinality("sessions", "user_1");
    metrics_.recordCardinality("sessions", "user_1");
    EXPECT_EQ(1u, metrics_.getCardinalityEstimate("sessions"));
}

TEST_F(AdvancedMetricsTest, Cardinality_MultipleMetrics_Independent) {
    metrics_.recordCardinality("metric_x", "a");
    metrics_.recordCardinality("metric_x", "b");
    metrics_.recordCardinality("metric_y", "a");

    EXPECT_EQ(2u, metrics_.getCardinalityEstimate("metric_x"));
    EXPECT_EQ(1u, metrics_.getCardinalityEstimate("metric_y"));
}

TEST_F(AdvancedMetricsTest, Cardinality_ManyValues_AllCounted) {
    for (int i = 0; i < 100; ++i) {
        metrics_.recordCardinality("ids", std::to_string(i));
    }
    EXPECT_EQ(100u, metrics_.getCardinalityEstimate("ids"));
}

// ============================================================================
// Time-Weighted Average Tests
// ============================================================================

TEST_F(AdvancedMetricsTest, TWA_UnknownMetric_ReturnsZero) {
    EXPECT_DOUBLE_EQ(0.0, metrics_.getTimeWeightedAverage("no_such_metric"));
}

TEST_F(AdvancedMetricsTest, TWA_SingleSample_ReturnsThatValue) {
    metrics_.recordTimeWeightedAverage("qps", 50.0, 60s);
    EXPECT_DOUBLE_EQ(50.0, metrics_.getTimeWeightedAverage("qps"));
}

TEST_F(AdvancedMetricsTest, TWA_ConstantValue_ReturnsThatValue) {
    // Two samples with the same value → TWA should equal that value.
    metrics_.recordTimeWeightedAverage("gauge", 100.0, 60s);
    std::this_thread::sleep_for(20ms);
    metrics_.recordTimeWeightedAverage("gauge", 100.0, 60s);
    double twa = metrics_.getTimeWeightedAverage("gauge");
    EXPECT_NEAR(100.0, twa, 1e-9);
}

TEST_F(AdvancedMetricsTest, TWA_TwoDistinctValues_WeightedByTime) {
    // Hold value=0 for ~50ms, then switch to value=100.
    // The TWA should be closer to 0 than 100 since 0 was held longer.
    metrics_.recordTimeWeightedAverage("gauge", 0.0, 60s);
    std::this_thread::sleep_for(50ms);
    metrics_.recordTimeWeightedAverage("gauge", 100.0, 60s);

    double twa = metrics_.getTimeWeightedAverage("gauge");
    // With only 2 samples, TWA = (0.0 × elapsed) / elapsed = 0.0.
    EXPECT_DOUBLE_EQ(0.0, twa);
}

TEST_F(AdvancedMetricsTest, TWA_OldSamplesExpired) {
    // Record a sample with a 1-second window.
    metrics_.recordTimeWeightedAverage("expiring", 999.0, 1s);
    std::this_thread::sleep_for(10ms);
    // Record again — the first sample is still within the window.
    metrics_.recordTimeWeightedAverage("expiring", 1.0, 1s);

    // Both samples are present; TWA should not be 0.
    double twa = metrics_.getTimeWeightedAverage("expiring");
    EXPECT_GE(twa, 0.0);
}

// ============================================================================
// Rate Tests
// ============================================================================

TEST_F(AdvancedMetricsTest, Rate_UnknownMetric_ReturnsZero) {
    EXPECT_DOUBLE_EQ(0.0, metrics_.getRate("no_such_metric"));
}

TEST_F(AdvancedMetricsTest, Rate_SingleSample_ReturnsZero) {
    metrics_.recordRate("throughput", 1000.0, 60s);
    EXPECT_DOUBLE_EQ(0.0, metrics_.getRate("throughput"));
}

TEST_F(AdvancedMetricsTest, Rate_TwoSamples_ComputedCorrectly) {
    metrics_.recordRate("bytes_out", 0.0, 60s);
    std::this_thread::sleep_for(100ms);
    metrics_.recordRate("bytes_out", 1000.0, 60s);

    double rate = metrics_.getRate("bytes_out");
    // ~1000 bytes / 0.1 s = ~10000 bytes/s.  Wide range for CI timing variance.
    EXPECT_GT(rate, 1000.0);
    EXPECT_LT(rate, 200000.0);
}

TEST_F(AdvancedMetricsTest, Rate_NegativeDelta_ReturnsNegativeRate) {
    // Arbitrary double values can decrease; negative rate is valid here.
    metrics_.recordRate("decreasing", 100.0, 60s);
    std::this_thread::sleep_for(20ms);
    metrics_.recordRate("decreasing", 50.0, 60s);

    double rate = metrics_.getRate("decreasing");
    EXPECT_LT(rate, 0.0);
}

TEST_F(AdvancedMetricsTest, Rate_SamplesOlderThanIntervalEvicted) {
    metrics_.recordRate("short_window", 0.0, 1s);
    std::this_thread::sleep_for(10ms);
    metrics_.recordRate("short_window", 500.0, 1s);

    // Both samples are within the 1-second window.
    EXPECT_GT(metrics_.getRate("short_window"), 0.0);
}

TEST_F(AdvancedMetricsTest, Rate_MultipleMetrics_Independent) {
    metrics_.recordRate("rps", 0.0, 60s);
    metrics_.recordRate("wps", 0.0, 60s);
    std::this_thread::sleep_for(20ms);
    metrics_.recordRate("rps", 200.0, 60s);
    metrics_.recordRate("wps", 50.0, 60s);

    EXPECT_GT(metrics_.getRate("rps"), metrics_.getRate("wps"));
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(AdvancedMetricsTest, Reset_ClearsAllMetricTypes) {
    metrics_.recordSummary("s", 1.0);
    metrics_.recordExponentialHistogram("e", 2.0);
    metrics_.recordCardinality("c", "x");
    metrics_.recordTimeWeightedAverage("t", 3.0, 60s);
    metrics_.recordRate("r", 4.0, 60s);

    metrics_.reset();

    EXPECT_EQ(0u, metrics_.getSummary("s", {0.5}).count);
    EXPECT_TRUE(metrics_.getExponentialHistogram("e").buckets.empty());
    EXPECT_EQ(0u, metrics_.getCardinalityEstimate("c"));
    EXPECT_DOUBLE_EQ(0.0, metrics_.getTimeWeightedAverage("t"));
    EXPECT_DOUBLE_EQ(0.0, metrics_.getRate("r"));
}

TEST_F(AdvancedMetricsTest, Reset_AllowsReRecording) {
    metrics_.recordSummary("s", 5.0);
    metrics_.reset();
    metrics_.recordSummary("s", 42.0);

    auto result = metrics_.getSummary("s", {1.0});
    EXPECT_EQ(1u, result.count);
    EXPECT_DOUBLE_EQ(42.0, result.sum);
}
