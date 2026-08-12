/**
 * @file test_metrics_aggregation.cpp
 * @brief Tests for MetricAggregator — rate calculation, histogram aggregation,
 *        rule-based aggregation, and cardinality management.
 *
 * Acceptance criteria (from issue):
 * - Metrics aggregation is tested.
 * - Cardinality limits and error handling are verified.
 */

#include <gtest/gtest.h>
#include "observability/metric_aggregator.h"
#include "observability/metrics_collector.h"

#include <chrono>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

// ============================================================================
// Fixture
// ============================================================================

class MetricAggregatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        agg_.reset();
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        agg_.reset();
        MetricsCollector::getInstance().reset();
    }
    MetricAggregator agg_;
};

// ============================================================================
// Rate Calculation Tests
// ============================================================================

TEST_F(MetricAggregatorTest, RateReturnsZeroWithOneSample) {
    agg_.recordCounterSample("http_requests_total", 100);
    EXPECT_DOUBLE_EQ(0.0, agg_.calculateRate("http_requests_total"));
}

TEST_F(MetricAggregatorTest, RateReturnsZeroWithNoSamples) {
    EXPECT_DOUBLE_EQ(0.0, agg_.calculateRate("http_requests_total"));
}

TEST_F(MetricAggregatorTest, RateCalculatedCorrectly) {
    // Record two samples ~100 ms apart with a delta of 50.
    agg_.recordCounterSample("req_total", 1000);
    std::this_thread::sleep_for(100ms);
    agg_.recordCounterSample("req_total", 1050);

    double rate = agg_.calculateRate("req_total");
    // Should be approximately 50 / 0.1s = 500 req/s.
    // Allow a wide range for CI timing variance (50ms–500ms actual sleep).
    EXPECT_GT(rate, 50.0);
    EXPECT_LT(rate, 2000.0);
}

TEST_F(MetricAggregatorTest, RateWithLabels) {
    agg_.recordCounterSample("req_total", 200, {{"method", "GET"}});
    std::this_thread::sleep_for(50ms);
    agg_.recordCounterSample("req_total", 210, {{"method", "GET"}});

    // Series with different labels should not interfere.
    double rate_get = agg_.calculateRate("req_total", {{"method", "GET"}});
    double rate_post = agg_.calculateRate("req_total", {{"method", "POST"}});

    EXPECT_GT(rate_get, 0.0);
    EXPECT_DOUBLE_EQ(0.0, rate_post);
}

TEST_F(MetricAggregatorTest, RateHandlesCounterReset) {
    agg_.recordCounterSample("restartable_counter", 500);
    std::this_thread::sleep_for(20ms);
    // Simulate a counter reset (new value < previous value).
    agg_.recordCounterSample("restartable_counter", 10);

    // Should return 0.0 instead of a negative rate.
    EXPECT_DOUBLE_EQ(0.0, agg_.calculateRate("restartable_counter"));
}

TEST_F(MetricAggregatorTest, RateSamplesArePruned) {
    // Use a very short window (1 s).
    agg_.recordCounterSample("pruned_counter", 100, {}, 1s);
    std::this_thread::sleep_for(20ms);
    agg_.recordCounterSample("pruned_counter", 200, {}, 1s);

    // Both samples are within 1 s: rate should be non-zero.
    EXPECT_GT(agg_.calculateRate("pruned_counter"), 0.0);

    // Prune with a tiny window — all samples should be dropped.
    agg_.pruneRateSamples(0s);
    EXPECT_DOUBLE_EQ(0.0, agg_.calculateRate("pruned_counter"));
}

// ============================================================================
// Histogram Aggregation Tests
// ============================================================================

TEST_F(MetricAggregatorTest, HistogramAggregation_Sum) {
    HistogramSnapshot s1{"latency_ms", {{"shard", "1"}}, {10.0, 20.0, 30.0}};
    HistogramSnapshot s2{"latency_ms", {{"shard", "2"}}, {5.0, 15.0, 25.0}};
    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);

    auto result = agg_.aggregateHistograms("latency_ms", AggregationType::SUM);
    EXPECT_DOUBLE_EQ(105.0, result.value);  // 10+20+30+5+15+25
    EXPECT_EQ(AggregationType::SUM, result.type);
    EXPECT_EQ("latency_ms", result.metric_name);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_Avg) {
    HistogramSnapshot s{"latency_ms", {}, {10.0, 20.0, 30.0}};
    agg_.addHistogramSnapshot(s);

    auto result = agg_.aggregateHistograms("latency_ms", AggregationType::AVG);
    EXPECT_DOUBLE_EQ(20.0, result.value);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_Max) {
    HistogramSnapshot s{"latency_ms", {}, {5.0, 99.0, 42.0}};
    agg_.addHistogramSnapshot(s);

    auto result = agg_.aggregateHistograms("latency_ms", AggregationType::MAX);
    EXPECT_DOUBLE_EQ(99.0, result.value);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_Min) {
    HistogramSnapshot s{"latency_ms", {}, {5.0, 99.0, 42.0}};
    agg_.addHistogramSnapshot(s);

    auto result = agg_.aggregateHistograms("latency_ms", AggregationType::MIN);
    EXPECT_DOUBLE_EQ(5.0, result.value);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_P99) {
    // 100 evenly-spaced values [1..100].
    std::vector<double> vals;
    vals.reserve(100);
    for (int i = 1; i <= 100; ++i) vals.push_back(static_cast<double>(i));

    HistogramSnapshot s{"latency_ms", {}, std::move(vals)};
    agg_.addHistogramSnapshot(s);

    auto result = agg_.aggregateHistograms("latency_ms", AggregationType::P99);
    // Nearest-rank p99 of [1..100] → index 98 → value 99
    EXPECT_GE(result.value, 98.0);
    EXPECT_LE(result.value, 100.0);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_P50) {
    std::vector<double> vals = {1.0, 2.0, 3.0, 4.0, 5.0};
    HistogramSnapshot s{"latency_ms", {}, vals};
    agg_.addHistogramSnapshot(s);

    auto result = agg_.aggregateHistograms("latency_ms", AggregationType::P50);
    EXPECT_DOUBLE_EQ(3.0, result.value);  // median of [1,2,3,4,5]
}

TEST_F(MetricAggregatorTest, HistogramAggregation_FilterByLabels) {
    HistogramSnapshot s1{"latency_ms", {{"region", "us-east"}}, {10.0, 20.0}};
    HistogramSnapshot s2{"latency_ms", {{"region", "eu-west"}}, {100.0, 200.0}};
    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);

    // Only aggregate us-east.
    auto result = agg_.aggregateHistograms(
        "latency_ms", AggregationType::SUM, {{"region", "us-east"}});
    EXPECT_DOUBLE_EQ(30.0, result.value);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_ThrowsOnRateType) {
    HistogramSnapshot s{"latency_ms", {}, {1.0, 2.0}};
    agg_.addHistogramSnapshot(s);

    EXPECT_THROW(
        agg_.aggregateHistograms("latency_ms", AggregationType::RATE),
        std::invalid_argument);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_ThrowsWhenNoSnapshots) {
    EXPECT_THROW(
        agg_.aggregateHistograms("nonexistent_metric", AggregationType::AVG),
        std::invalid_argument);
}

TEST_F(MetricAggregatorTest, HistogramAggregation_MultipleSnapshotsSameLabels) {
    // Two snapshots from the same logical series (e.g. two scrapes).
    HistogramSnapshot s1{"req_latency", {{"shard", "1"}}, {10.0, 20.0}};
    HistogramSnapshot s2{"req_latency", {{"shard", "1"}}, {30.0, 40.0}};
    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);

    auto result = agg_.aggregateHistograms("req_latency", AggregationType::SUM);
    EXPECT_DOUBLE_EQ(100.0, result.value);  // 10+20+30+40
}

// ============================================================================
// Rule-Based Aggregation Tests
// ============================================================================

TEST_F(MetricAggregatorTest, AddAndRetrieveRule) {
    AggregationRule rule;
    rule.metric_name = "query_latency_ms";
    rule.type = AggregationType::P95;
    rule.interval = 60s;
    rule.group_by_labels = {"tenant_id"};
    rule.drop_labels = {"shard_id", "instance_id"};

    agg_.addAggregationRule(rule);
    auto rules = agg_.getRules();
    ASSERT_EQ(1u, rules.size());
    EXPECT_EQ("query_latency_ms", rules[0].metric_name);
    EXPECT_EQ(AggregationType::P95, rules[0].type);
}

TEST_F(MetricAggregatorTest, AddRuleReplacesDuplicate) {
    AggregationRule r1;
    r1.metric_name = "latency";
    r1.type = AggregationType::AVG;

    AggregationRule r2;
    r2.metric_name = "latency";
    r2.type = AggregationType::P99;

    agg_.addAggregationRule(r1);
    agg_.addAggregationRule(r2);

    auto rules = agg_.getRules();
    ASSERT_EQ(1u, rules.size());
    EXPECT_EQ(AggregationType::P99, rules[0].type);
}

TEST_F(MetricAggregatorTest, RemoveRule) {
    AggregationRule rule;
    rule.metric_name = "cpu_usage";
    rule.type = AggregationType::MAX;
    agg_.addAggregationRule(rule);

    EXPECT_TRUE(agg_.removeAggregationRule("cpu_usage"));
    EXPECT_FALSE(agg_.removeAggregationRule("cpu_usage"));  // already gone
    EXPECT_TRUE(agg_.getRules().empty());
}

TEST_F(MetricAggregatorTest, ApplyRules_HistogramRule) {
    AggregationRule rule;
    rule.metric_name = "db_query_ms";
    rule.type = AggregationType::P95;
    rule.drop_labels = {"shard_id"};
    agg_.addAggregationRule(rule);

    HistogramSnapshot s1{"db_query_ms", {{"shard_id", "1"}, {"tenant", "acme"}}, {10.0, 20.0, 30.0}};
    HistogramSnapshot s2{"db_query_ms", {{"shard_id", "2"}, {"tenant", "acme"}}, {15.0, 25.0, 35.0}};
    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);

    auto results = agg_.applyRules();
    ASSERT_FALSE(results.empty());
    EXPECT_EQ("db_query_ms", results[0].metric_name);
    EXPECT_EQ(AggregationType::P95, results[0].type);
    EXPECT_GT(results[0].value, 0.0);
}

TEST_F(MetricAggregatorTest, ApplyRules_RateRule) {
    AggregationRule rule;
    rule.metric_name = "counter_total";
    rule.type = AggregationType::RATE;
    agg_.addAggregationRule(rule);

    agg_.recordCounterSample("counter_total", 1000);
    std::this_thread::sleep_for(50ms);
    agg_.recordCounterSample("counter_total", 1100);

    auto results = agg_.applyRules();
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(AggregationType::RATE, results[0].type);
    EXPECT_GT(results[0].value, 0.0);
}

TEST_F(MetricAggregatorTest, ApplyRules_SkipsMetricsWithNoData) {
    AggregationRule rule;
    rule.metric_name = "nonexistent_metric";
    rule.type = AggregationType::SUM;
    agg_.addAggregationRule(rule);

    auto results = agg_.applyRules();
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Cardinality Management Tests
// ============================================================================

TEST_F(MetricAggregatorTest, CardinalityLimit_Enforced) {
    agg_.setMetricCardinalityLimit("high_card_metric", 2);

    HistogramSnapshot s1{"high_card_metric", {{"id", "1"}}, {1.0}};
    HistogramSnapshot s2{"high_card_metric", {{"id", "2"}}, {2.0}};
    HistogramSnapshot s3{"high_card_metric", {{"id", "3"}}, {3.0}};  // should be dropped

    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);
    agg_.addHistogramSnapshot(s3);

    EXPECT_EQ(2u, agg_.getSeriesCount("high_card_metric"));
    EXPECT_EQ(1, agg_.getDroppedSnapshotCount());
}

TEST_F(MetricAggregatorTest, CardinalityLimit_AllowsExistingSeriesUpdate) {
    agg_.setMetricCardinalityLimit("metric", 1);

    HistogramSnapshot s1{"metric", {{"id", "1"}}, {10.0}};
    HistogramSnapshot s1b{"metric", {{"id", "1"}}, {20.0}};  // same labels → allowed

    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s1b);

    EXPECT_EQ(1u, agg_.getSeriesCount("metric"));
    EXPECT_EQ(0, agg_.getDroppedSnapshotCount());
}

TEST_F(MetricAggregatorTest, CardinalityLimit_ZeroMeansUnlimited) {
    agg_.setMetricCardinalityLimit("free_metric", 0);

    for (int i = 0; i < 100; ++i) {
        HistogramSnapshot s{"free_metric", {{"id", std::to_string(i)}}, {static_cast<double>(i)}};
        agg_.addHistogramSnapshot(s);
    }

    EXPECT_EQ(100u, agg_.getSeriesCount("free_metric"));
    EXPECT_EQ(0, agg_.getDroppedSnapshotCount());
}

TEST_F(MetricAggregatorTest, CardinalityLimit_DroppedSnapshotCountAccumulates) {
    agg_.setMetricCardinalityLimit("m", 1);

    HistogramSnapshot s1{"m", {{"a", "1"}}, {1.0}};
    HistogramSnapshot s2{"m", {{"a", "2"}}, {2.0}};
    HistogramSnapshot s3{"m", {{"a", "3"}}, {3.0}};

    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);
    agg_.addHistogramSnapshot(s3);

    EXPECT_EQ(2, agg_.getDroppedSnapshotCount());
}

// ============================================================================
// MetricsCollector Cardinality Integration Tests
// ============================================================================

TEST_F(MetricAggregatorTest, CollectorCardinalityLimitDropsSeries) {
    auto& collector = MetricsCollector::getInstance();
    collector.setCardinalityLimit(2);

    // Record 3 distinct series for the same metric.
    collector.addCounter("requests_total", 1, {{"method", "GET"}});
    collector.addCounter("requests_total", 1, {{"method", "POST"}});
    collector.addCounter("requests_total", 1, {{"method", "DELETE"}});  // dropped

    EXPECT_EQ(1, collector.getDroppedSeriesCount());
}

TEST_F(MetricAggregatorTest, CollectorCardinalityLimitIsDisabledByDefault) {
    auto& collector = MetricsCollector::getInstance();
    EXPECT_EQ(0u, collector.getCardinalityLimit());

    // Many distinct series — none should be dropped.
    for (int i = 0; i < 50; ++i) {
        collector.addCounter("unlimited", 1, {{"id", std::to_string(i)}});
    }
    EXPECT_EQ(0, collector.getDroppedSeriesCount());
}

TEST_F(MetricAggregatorTest, CollectorCardinalityLimitCanBeReset) {
    auto& collector = MetricsCollector::getInstance();
    collector.setCardinalityLimit(1);

    collector.addCounter("m", 1, {{"a", "1"}});
    collector.addCounter("m", 1, {{"a", "2"}});  // dropped

    EXPECT_EQ(1, collector.getDroppedSeriesCount());

    // Resetting clears the counter.
    collector.reset();
    EXPECT_EQ(0, collector.getDroppedSeriesCount());
}

TEST_F(MetricAggregatorTest, CollectorCardinalityLimitAllowsKnownSeries) {
    auto& collector = MetricsCollector::getInstance();
    collector.setCardinalityLimit(1);

    // First insertion creates the series.
    collector.addCounter("m", 1, {{"a", "1"}});
    // Second insertion with same key must be allowed (not a new series).
    collector.addCounter("m", 5, {{"a", "1"}});

    EXPECT_EQ(0, collector.getDroppedSeriesCount());
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(MetricAggregatorTest, ResetClearsSnapshotsAndRateSamples) {
    HistogramSnapshot s{"m", {}, {1.0, 2.0}};
    agg_.addHistogramSnapshot(s);
    agg_.recordCounterSample("c", 100);
    agg_.recordCounterSample("c", 200);

    agg_.reset();

    EXPECT_THROW(agg_.aggregateHistograms("m", AggregationType::SUM),
                 std::invalid_argument);
    EXPECT_DOUBLE_EQ(0.0, agg_.calculateRate("c"));
}

TEST_F(MetricAggregatorTest, ResetPreservesRules) {
    AggregationRule rule;
    rule.metric_name = "persistent_rule";
    rule.type = AggregationType::AVG;
    agg_.addAggregationRule(rule);

    agg_.reset();

    EXPECT_EQ(1u, agg_.getRules().size());
}

TEST_F(MetricAggregatorTest, ResetClearsCardinalityCounters) {
    agg_.setMetricCardinalityLimit("m", 1);
    HistogramSnapshot s1{"m", {{"id", "1"}}, {1.0}};
    HistogramSnapshot s2{"m", {{"id", "2"}}, {2.0}};
    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);

    EXPECT_EQ(1, agg_.getDroppedSnapshotCount());

    agg_.reset();
    EXPECT_EQ(0, agg_.getDroppedSnapshotCount());
    EXPECT_EQ(0u, agg_.getSeriesCount("m"));
}

// ============================================================================
// Thread Safety
// ============================================================================

TEST_F(MetricAggregatorTest, ConcurrentSnapshotAdds_ThreadSafe) {
    constexpr int kThreads = 8;
    constexpr int kSnapshotsPerThread = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kSnapshotsPerThread; ++i) {
                HistogramSnapshot s{
                    "concurrent_metric",
                    {{"thread", std::to_string(t)}, {"i", std::to_string(i)}},
                    {static_cast<double>(t * 100 + i)}};
                agg_.addHistogramSnapshot(s);
            }
        });
    }
    for (auto& th : threads) th.join();

    // Should not throw; all observations from distinct (thread, i) combinations.
    EXPECT_NO_THROW(
        agg_.aggregateHistograms("concurrent_metric", AggregationType::SUM));
}

TEST_F(MetricAggregatorTest, ConcurrentRateSamples_ThreadSafe) {
    constexpr int kThreads = 4;
    constexpr int kSamplesPerThread = 20;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kSamplesPerThread; ++i) {
                agg_.recordCounterSample(
                    "concurrent_counter",
                    static_cast<int64_t>(t * 1000 + i),
                    {{"thread", std::to_string(t)}});
            }
        });
    }
    for (auto& th : threads) th.join();

    // Must not crash; exact rate value is non-deterministic in this test.
    for (int t = 0; t < kThreads; ++t) {
        EXPECT_NO_THROW(
            agg_.calculateRate("concurrent_counter", {{"thread", std::to_string(t)}}));
    }
}

// ============================================================================
// aggregateShardMetrics Tests
// ============================================================================

TEST_F(MetricAggregatorTest, AggregateShardMetrics_ReturnsSnapshotWithTimestamp) {
    AggregationRule rule;
    rule.metric_name = "query_latency_ms";
    rule.type = AggregationType::AVG;
    agg_.addAggregationRule(rule);

    ShardMetrics shard1;
    shard1.shard_id = "shard-0";
    shard1.metrics["query_latency_ms"] = {10.0, 20.0, 30.0};

    auto snap = agg_.aggregateShardMetrics({shard1});
    EXPECT_FALSE(snap.metrics.empty());
    EXPECT_EQ("query_latency_ms", snap.metrics[0].metric_name);
    EXPECT_GT(snap.timestamp.time_since_epoch().count(), 0);
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_SumAcrossTwoShards) {
    AggregationRule rule;
    rule.metric_name = "requests_total";
    rule.type = AggregationType::SUM;
    agg_.addAggregationRule(rule);

    ShardMetrics shard1;
    shard1.shard_id = "shard-0";
    shard1.metrics["requests_total"] = {100.0, 200.0};

    ShardMetrics shard2;
    shard2.shard_id = "shard-1";
    shard2.metrics["requests_total"] = {50.0, 150.0};

    auto snap = agg_.aggregateShardMetrics({shard1, shard2});
    ASSERT_EQ(1u, snap.metrics.size());
    EXPECT_DOUBLE_EQ(500.0, snap.metrics[0].value);  // 100+200+50+150
    EXPECT_EQ(AggregationType::SUM, snap.metrics[0].type);
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_P99AcrossShards) {
    AggregationRule rule;
    rule.metric_name = "latency_ms";
    rule.type = AggregationType::P99;
    agg_.addAggregationRule(rule);

    ShardMetrics shard1;
    shard1.shard_id = "s1";
    std::vector<double> vals;
    vals.reserve(100);
    for (int i = 1; i <= 100; ++i) vals.push_back(static_cast<double>(i));
    shard1.metrics["latency_ms"] = vals;

    auto snap = agg_.aggregateShardMetrics({shard1});
    ASSERT_EQ(1u, snap.metrics.size());
    EXPECT_GE(snap.metrics[0].value, 98.0);
    EXPECT_LE(snap.metrics[0].value, 100.0);
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_DropShardIdLabel) {
    // Shard-id should be droppable via drop_labels so results are merged.
    AggregationRule rule;
    rule.metric_name = "cpu_usage";
    rule.type = AggregationType::AVG;
    rule.drop_labels = {"shard_id"};
    agg_.addAggregationRule(rule);

    ShardMetrics shard1;
    shard1.shard_id = "shard-0";
    shard1.metrics["cpu_usage"] = {80.0};

    ShardMetrics shard2;
    shard2.shard_id = "shard-1";
    shard2.metrics["cpu_usage"] = {60.0};

    auto snap = agg_.aggregateShardMetrics({shard1, shard2});
    ASSERT_EQ(1u, snap.metrics.size());
    EXPECT_DOUBLE_EQ(70.0, snap.metrics[0].value);  // avg(80, 60)
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_GroupByLabel) {
    AggregationRule rule;
    rule.metric_name = "latency_ms";
    rule.type = AggregationType::AVG;
    rule.group_by_labels = {"region"};
    rule.drop_labels = {"shard_id"};
    agg_.addAggregationRule(rule);

    ShardMetrics shard_us;
    shard_us.shard_id = "shard-us-0";
    shard_us.labels = {{"region", "us-east"}};
    shard_us.metrics["latency_ms"] = {10.0, 20.0};

    ShardMetrics shard_eu;
    shard_eu.shard_id = "shard-eu-0";
    shard_eu.labels = {{"region", "eu-west"}};
    shard_eu.metrics["latency_ms"] = {30.0, 40.0};

    auto snap = agg_.aggregateShardMetrics({shard_us, shard_eu});
    // Two groups (us-east, eu-west) → two AggregatedMetric entries.
    ASSERT_EQ(2u, snap.metrics.size());

    // Collect region label values from results.
    std::vector<std::string> regions;
    for (const auto& m : snap.metrics) {
        auto it = m.labels.find("region");
        ASSERT_NE(it, m.labels.end()) << "Expected 'region' label in result";
        regions.push_back(it->second);
    }
    std::sort(regions.begin(), regions.end());
    EXPECT_EQ("eu-west", regions[0]);
    EXPECT_EQ("us-east", regions[1]);
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_LabelsPopulatedInResult) {
    // Verify AggregatedMetric.labels contains the group_by label set.
    AggregationRule rule;
    rule.metric_name = "cpu_ms";
    rule.type = AggregationType::MAX;
    rule.group_by_labels = {"tenant_id"};
    rule.drop_labels = {"shard_id"};
    agg_.addAggregationRule(rule);

    ShardMetrics shard;
    shard.shard_id = "s0";
    shard.labels = {{"tenant_id", "acme"}};
    shard.metrics["cpu_ms"] = {50.0, 80.0};

    auto snap = agg_.aggregateShardMetrics({shard});
    ASSERT_EQ(1u, snap.metrics.size());
    EXPECT_EQ("acme", snap.metrics[0].labels.at("tenant_id"));
}

TEST_F(MetricAggregatorTest, ApplyRules_LabelsPopulatedInResult) {
    // Verify applyRules() populates AggregatedMetric.labels from group_by_labels.
    AggregationRule rule;
    rule.metric_name = "req_ms";
    rule.type = AggregationType::AVG;
    rule.group_by_labels = {"dc"};
    rule.drop_labels = {"host"};
    agg_.addAggregationRule(rule);

    HistogramSnapshot s1{"req_ms", {{"dc", "us-east"}, {"host", "web-01"}}, {10.0, 30.0}};
    HistogramSnapshot s2{"req_ms", {{"dc", "eu-west"}, {"host", "web-02"}}, {20.0, 40.0}};
    agg_.addHistogramSnapshot(s1);
    agg_.addHistogramSnapshot(s2);

    auto results = agg_.applyRules();
    ASSERT_EQ(2u, results.size());

    for (const auto& r : results) {
        EXPECT_FALSE(r.labels.empty()) << "Expected labels to be populated";
        EXPECT_EQ(1u, r.labels.count("dc"));
        EXPECT_EQ(0u, r.labels.count("host")) << "drop_labels should exclude 'host'";
    }
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_NoRules_ReturnsEmpty) {
    ShardMetrics shard1;
    shard1.shard_id = "shard-0";
    shard1.metrics["query_latency_ms"] = {10.0, 20.0};

    auto snap = agg_.aggregateShardMetrics({shard1});
    EXPECT_TRUE(snap.metrics.empty());
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_EmptyInput_ReturnsEmpty) {
    AggregationRule rule;
    rule.metric_name = "latency_ms";
    rule.type = AggregationType::AVG;
    agg_.addAggregationRule(rule);

    auto snap = agg_.aggregateShardMetrics({});
    EXPECT_TRUE(snap.metrics.empty());
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_SkipsRateRules) {
    // RATE rules cannot be evaluated from a batch snapshot; they must be silently skipped.
    AggregationRule rate_rule;
    rate_rule.metric_name = "requests_total";
    rate_rule.type = AggregationType::RATE;
    agg_.addAggregationRule(rate_rule);

    ShardMetrics shard1;
    shard1.shard_id = "shard-0";
    shard1.metrics["requests_total"] = {100.0, 200.0};

    EXPECT_NO_THROW({
        auto snap = agg_.aggregateShardMetrics({shard1});
        EXPECT_TRUE(snap.metrics.empty());
    });
}

TEST_F(MetricAggregatorTest, AggregateShardMetrics_DoesNotMutateInternalBuffer) {
    AggregationRule rule;
    rule.metric_name = "latency_ms";
    rule.type = AggregationType::SUM;
    agg_.addAggregationRule(rule);

    ShardMetrics shard1;
    shard1.shard_id = "shard-0";
    shard1.metrics["latency_ms"] = {5.0, 10.0};

    agg_.aggregateShardMetrics({shard1});

    // Internal buffer should still be empty after the stateless call.
    EXPECT_THROW(agg_.aggregateHistograms("latency_ms", AggregationType::SUM),
                 std::invalid_argument);
}

// ============================================================================
// rollupMetrics Tests
// ============================================================================

TEST_F(MetricAggregatorTest, RollupMetrics_RemovesOldSnapshots) {
    using namespace std::chrono_literals;

    // Add an old snapshot with a manually backdated timestamp.
    HistogramSnapshot old_snap;
    old_snap.metric_name = "latency_ms";
    old_snap.values = {10.0, 20.0};
    old_snap.timestamp = std::chrono::system_clock::now() - std::chrono::hours(2);
    agg_.addHistogramSnapshot(old_snap);

    // Verify data is accessible before rollup.
    EXPECT_NO_THROW(agg_.aggregateHistograms("latency_ms", AggregationType::SUM));

    // Rollup with a 60-minute window removes the 2-hour-old snapshot.
    agg_.rollupMetrics(std::chrono::minutes{60});

    EXPECT_THROW(agg_.aggregateHistograms("latency_ms", AggregationType::SUM),
                 std::invalid_argument);
}

TEST_F(MetricAggregatorTest, RollupMetrics_PreservesRecentSnapshots) {
    // Add a current snapshot.
    HistogramSnapshot snap;
    snap.metric_name = "latency_ms";
    snap.values = {5.0, 15.0};
    snap.timestamp = std::chrono::system_clock::now();
    agg_.addHistogramSnapshot(snap);

    // Rollup with a generous window should not remove the recent snapshot.
    agg_.rollupMetrics(std::chrono::minutes{60});

    EXPECT_NO_THROW({
        auto result = agg_.aggregateHistograms("latency_ms", AggregationType::SUM);
        EXPECT_DOUBLE_EQ(20.0, result.value);
    });
}

TEST_F(MetricAggregatorTest, RollupMetrics_PrunesRateSamples) {
    agg_.recordCounterSample("counter", 100);
    std::this_thread::sleep_for(100ms);
    agg_.recordCounterSample("counter", 200);

    // Before rollup: rate is non-zero.
    EXPECT_GT(agg_.calculateRate("counter"), 0.0);

    // Rollup with 0-minute window prunes all rate samples.
    agg_.rollupMetrics(std::chrono::minutes{0});

    // After pruning: fewer than two samples remain → rate is 0.
    EXPECT_DOUBLE_EQ(0.0, agg_.calculateRate("counter"));
}

TEST_F(MetricAggregatorTest, RollupMetrics_PreservesRulesAndLimits) {
    AggregationRule rule;
    rule.metric_name = "cpu";
    rule.type = AggregationType::MAX;
    agg_.addAggregationRule(rule);
    agg_.setMetricCardinalityLimit("cpu", 5);

    agg_.rollupMetrics(std::chrono::minutes{60});

    EXPECT_EQ(1u, agg_.getRules().size());
}

TEST_F(MetricAggregatorTest, RollupMetrics_MixedAgeSnapshots) {
    // Add one old and one recent snapshot for the same metric.
    HistogramSnapshot old_snap;
    old_snap.metric_name = "cpu";
    old_snap.labels = {{"host", "node-1"}};
    old_snap.values = {90.0};
    old_snap.timestamp = std::chrono::system_clock::now() - std::chrono::hours(3);
    agg_.addHistogramSnapshot(old_snap);

    HistogramSnapshot recent_snap;
    recent_snap.metric_name = "cpu";
    recent_snap.labels = {{"host", "node-2"}};
    recent_snap.values = {50.0};
    recent_snap.timestamp = std::chrono::system_clock::now();
    agg_.addHistogramSnapshot(recent_snap);

    // Roll up with 1-hour window: old snapshot drops, recent stays.
    agg_.rollupMetrics(std::chrono::minutes{60});

    auto result = agg_.aggregateHistograms("cpu", AggregationType::SUM);
    EXPECT_DOUBLE_EQ(50.0, result.value);
}
