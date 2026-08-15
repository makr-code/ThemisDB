/**
 * @file test_high_cardinality_metrics.cpp
 * @brief Focused regression tests for high-cardinality metrics (Phase 2, HCM-01..10).
 *
 * Test coverage:
 * - HCM-01: Cardinality limit enforcement
 * - HCM-02: DROP_NEW_SETS policy
 * - HCM-03: AGGREGATE_TO_OTHER policy
 * - HCM-04: WARN_ONLY policy
 * - HCM-05: Cardinality statistics tracking
 * - HCM-06: Custom fallback strategies
 * - HCM-07: Per-metric cardinality configuration
 * - HCM-08: Prefix-based limit configuration
 * - HCM-09: Concurrent label set recording
 * - HCM-10: Edge cases (empty metrics, extreme cardinality)
 */

#include "gtest/gtest.h"
#include "observability/high_cardinality_metrics.h"
#include <thread>
#include <vector>
#include <map>

namespace themis {
namespace observability {

class HighCardinalityMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        tracker = createHighCardinalityMetricsTracker();
    }

    std::unique_ptr<HighCardinalityMetricsTracker> tracker;
};

// HCM-01: Cardinality limit enforcement
TEST_F(HighCardinalityMetricsTest, CardinalityLimitEnforcement) {
    CardinalityLimit limit;
    limit.max_series = 10;
    limit.policy = CardinalityExceededPolicy::DROP_NEW_SETS;
    limit.emit_diagnostics = true;

    tracker->setCardinalityLimit("test_metric", limit);

    // Record 10 unique label sets
    for (int i = 0; i < 10; ++i) {
        std::map<std::string, std::string> labels;
        labels["label"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    auto stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.current_series_count, 10);
    EXPECT_FALSE(stats.at_limit);  // Exactly at limit, not over

    // Try to record 11th unique label set
    std::map<std::string, std::string> labels;
    labels["label"] = "10";
    tracker->recordLabelSet("test_metric", labels);

    stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.current_series_count, 10);  // Should not increase
}

// HCM-02: DROP_NEW_SETS policy
TEST_F(HighCardinalityMetricsTest, DropNewSetPolicy) {
    CardinalityLimit limit;
    limit.max_series = 5;
    limit.policy = CardinalityExceededPolicy::DROP_NEW_SETS;

    tracker->setCardinalityLimit("test_metric", limit);

    // Fill to limit
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    // Try to exceed limit
    std::map<std::string, std::string> excess_labels;
    excess_labels["id"] = "overflow";

    bool can_accept = tracker->canAcceptLabelSet("test_metric", excess_labels);
    EXPECT_FALSE(can_accept);

    tracker->recordLabelSet("test_metric", excess_labels);

    auto stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.rejected_sets_total, 1);
    EXPECT_EQ(stats.current_series_count, 5);
}

// HCM-03: AGGREGATE_TO_OTHER policy
TEST_F(HighCardinalityMetricsTest, AggregateToOtherPolicy) {
    CardinalityLimit limit;
    limit.max_series = 5;
    limit.policy = CardinalityExceededPolicy::AGGREGATE_TO_OTHER;

    tracker->setCardinalityLimit("test_metric", limit);

    // Fill to limit
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    // Try to exceed limit
    std::map<std::string, std::string> excess_labels;
    excess_labels["id"] = "overflow1";

    auto recorded_labels = tracker->recordLabelSet("test_metric", excess_labels);

    // Should have __other label added
    EXPECT_TRUE(recorded_labels.count("__other"));

    auto stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.aggregated_sets_total, 1);
}

// HCM-04: WARN_ONLY policy
TEST_F(HighCardinalityMetricsTest, WarnOnlyPolicy) {
    CardinalityLimit limit;
    limit.max_series = 5;
    limit.policy = CardinalityExceededPolicy::WARN_ONLY;

    tracker->setCardinalityLimit("test_metric", limit);

    // Fill to limit
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    // Try to exceed limit
    std::map<std::string, std::string> excess_labels;
    excess_labels["id"] = "overflow";

    tracker->recordLabelSet("test_metric", excess_labels);

    auto stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.current_series_count, 6);  // Should accept beyond limit
}

// HCM-05: Cardinality statistics tracking
TEST_F(HighCardinalityMetricsTest, CardinalityStatisticsTracking) {
    CardinalityLimit limit;
    limit.max_series = 100;

    tracker->setCardinalityLimit("test_metric", limit);

    // Record some label sets
    for (int i = 0; i < 50; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    auto stats = tracker->getCardinalityStats("test_metric");

    EXPECT_EQ(stats.current_series_count, 50);
    EXPECT_EQ(stats.limit, 100);
    EXPECT_EQ(stats.utilization_percent, 50.0);
    EXPECT_FALSE(stats.at_limit);
}

// HCM-06: Custom fallback strategies
TEST_F(HighCardinalityMetricsTest, CustomFallbackStrategy) {
    class TestFallbackStrategy : public CardinalityFallbackStrategy {
    public:
        std::map<std::string, std::string> apply(
            const std::map<std::string, std::string>& labels) override {
            auto result = labels;
            result["fallback"] = "custom";
            return result;
        }
    };

    CardinalityLimit limit;
    limit.max_series = 3;
    limit.policy = CardinalityExceededPolicy::AGGREGATE_TO_OTHER;

    tracker->setCardinalityLimit("test_metric", limit);
    tracker->setFallbackStrategy("test_metric", std::make_unique<TestFallbackStrategy>());

    // Fill to limit
    for (int i = 0; i < 3; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    // Exceed limit
    std::map<std::string, std::string> excess_labels;
    excess_labels["id"] = "overflow";

    auto recorded = tracker->recordLabelSet("test_metric", excess_labels);

    EXPECT_TRUE(recorded.count("fallback"));
    EXPECT_EQ(recorded["fallback"], "custom");
}

// HCM-07: Per-metric cardinality configuration
TEST_F(HighCardinalityMetricsTest, PerMetricConfiguration) {
    CardinalityLimit limit1;
    limit1.max_series = 10;

    CardinalityLimit limit2;
    limit2.max_series = 100;

    tracker->setCardinalityLimit("metric1", limit1);
    tracker->setCardinalityLimit("metric2", limit2);

    auto retrieved_limit1 = tracker->getCardinalityLimit("metric1");
    auto retrieved_limit2 = tracker->getCardinalityLimit("metric2");

    EXPECT_EQ(retrieved_limit1.max_series, 10);
    EXPECT_EQ(retrieved_limit2.max_series, 100);
}

// HCM-08: Prefix-based limit configuration
TEST_F(HighCardinalityMetricsTest, PrefixBasedLimitConfiguration) {
    CardinalityLimit limit;
    limit.max_series = 50;

    // Create some metrics with common prefix
    tracker->setCardinalityLimit("http_request_duration_ms", limit);
    tracker->setCardinalityLimit("http_request_size_bytes", limit);
    tracker->setCardinalityLimit("db_query_latency_ms", limit);

    // Apply limit to all "http_*" metrics
    std::size_t updated = tracker->setCardinalityLimitByPrefix("http_", limit);

    EXPECT_GE(updated, 2);  // At least 2 http_* metrics
}

// HCM-09: Concurrent label set recording
TEST_F(HighCardinalityMetricsTest, ConcurrentLabelSetRecording) {
    CardinalityLimit limit;
    limit.max_series = 10000;
    limit.policy = CardinalityExceededPolicy::DROP_NEW_SETS;

    tracker->setCardinalityLimit("test_metric", limit);

    std::vector<std::thread> threads;

    // Record label sets concurrently
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 100; ++i) {
                std::map<std::string, std::string> labels;
                labels["thread"] = std::to_string(t);
                labels["iteration"] = std::to_string(i);
                tracker->recordLabelSet("test_metric", labels);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.current_series_count, 1000);  // 10 threads * 100 iterations
}

// HCM-10: Edge cases (empty metrics, extreme cardinality)
TEST_F(HighCardinalityMetricsTest, EmptyMetricHandling) {
    auto stats = tracker->getCardinalityStats("nonexistent_metric");

    EXPECT_EQ(stats.current_series_count, 0);
    EXPECT_EQ(stats.limit, 0);  // No limit configured
}

TEST_F(HighCardinalityMetricsTest, ExtremeCardinalityLimit) {
    CardinalityLimit limit;
    limit.max_series = 1000000;  // Very high limit

    tracker->setCardinalityLimit("extreme_metric", limit);

    // Record many unique label sets
    for (int i = 0; i < 100; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("extreme_metric", labels);
    }

    auto stats = tracker->getCardinalityStats("extreme_metric");
    EXPECT_EQ(stats.current_series_count, 100);
    EXPECT_FALSE(stats.at_limit);
}

TEST_F(HighCardinalityMetricsTest, TrackingEnableDisable) {
    CardinalityLimit limit;
    limit.max_series = 5;

    tracker->setCardinalityLimit("test_metric", limit);

    // Initially enabled
    EXPECT_TRUE(tracker->isTrackingEnabled("test_metric"));

    // Fill to limit
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> labels;
        labels["id"] = std::to_string(i);
        tracker->recordLabelSet("test_metric", labels);
    }

    // Disable tracking
    tracker->setTrackingEnabled("test_metric", false);

    // Should now accept unlimited cardinality
    std::map<std::string, std::string> excess_labels;
    excess_labels["id"] = "overflow";

    bool can_accept = tracker->canAcceptLabelSet("test_metric", excess_labels);
    EXPECT_TRUE(can_accept);

    auto stats = tracker->getCardinalityStats("test_metric");
    EXPECT_EQ(stats.current_series_count, 5);  // Still shows recorded count
}

} // namespace observability
} // namespace themis
