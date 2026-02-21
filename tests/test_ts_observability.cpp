// Phase 7: Testing, Observability & Prometheus Metrics Export Tests

#include <gtest/gtest.h>
#include "timeseries/timeseries_metrics.h"
#include <thread>
#include <chrono>
#include <string>
#include <regex>
#include <sstream>

using namespace themis;

class TimeSeriesObservabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        TimeSeriesMetrics::Config config;
        config.enable_histograms       = true;
        config.enable_per_metric_stats = true;
        metrics = std::make_shared<TimeSeriesMetrics>(config);
    }

    std::shared_ptr<TimeSeriesMetrics> metrics;

    bool prometheusContains(const std::string& output, const std::string& metric_name) {
        return output.find(metric_name) != std::string::npos;
    }
};

// ===== Write metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordSingleWriteIncrementsCounter) {
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 0u);
    metrics->recordDataPointWrite("cpu", 1.0, true);
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 1u);
}

TEST_F(TimeSeriesObservabilityTest, RecordMultipleWritesAccumulate) {
    for (int i = 0; i < 100; ++i) {
        metrics->recordDataPointWrite("cpu", 0.5, true);
    }
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 100u);
}

TEST_F(TimeSeriesObservabilityTest, RecordWriteFailureTracked) {
    metrics->recordDataPointWrite("cpu", 1.0, false);  // failed write
    // Failed writes still increment total counter but also write_errors
    EXPECT_GE(metrics->getTotalDataPointsWritten(), 1u);
}

TEST_F(TimeSeriesObservabilityTest, RecordBatchWriteIncreasesBatchCounter) {
    metrics->recordBatchWrite(100, 5.0, true, true);
    EXPECT_EQ(metrics->getTotalBatchesWritten(), 1u);
}

TEST_F(TimeSeriesObservabilityTest, RecordMultipleBatchesAccumulate) {
    for (int i = 0; i < 5; ++i) {
        metrics->recordBatchWrite(50, 2.0, true, true);
    }
    EXPECT_EQ(metrics->getTotalBatchesWritten(), 5u);
}

// ===== Query metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordQueryIncrementsCounter) {
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 0u);
    metrics->recordQuery("cpu", 10.0, 100, 60000);
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 1u);
}

TEST_F(TimeSeriesObservabilityTest, RecordMultipleQueriesAccumulate) {
    for (int i = 0; i < 10; ++i) {
        metrics->recordQuery("mem", static_cast<double>(i), 50, 3600000);
    }
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 10u);
}

// ===== Aggregation metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordAggregationIncrementsCounter) {
    EXPECT_EQ(metrics->getTotalAggregationsExecuted(), 0u);
    metrics->recordAggregation("disk", 2.0, 1000, false);
    EXPECT_EQ(metrics->getTotalAggregationsExecuted(), 1u);
}

// ===== Optimizer metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordOptimizerHit) {
    metrics->recordOptimizerResult(true);
    metrics->recordOptimizerResult(true);
    metrics->recordOptimizerResult(false);
    EXPECT_EQ(metrics->getOptimizerHits(), 2u);
    EXPECT_EQ(metrics->getOptimizerMisses(), 1u);
}

TEST_F(TimeSeriesObservabilityTest, OptimizerMissCountCorrect) {
    for (int i = 0; i < 7; ++i) metrics->recordOptimizerResult(false);
    EXPECT_EQ(metrics->getOptimizerMisses(), 7u);
    EXPECT_EQ(metrics->getOptimizerHits(), 0u);
}

// ===== Latency metrics =====

TEST_F(TimeSeriesObservabilityTest, AverageWriteLatencyCalculated) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordDataPointWrite("cpu", 3.0, true);
    double avg = metrics->getAverageWriteLatency();
    EXPECT_GT(avg, 0.0);
    EXPECT_NEAR(avg, 2.0, 0.01);
}

TEST_F(TimeSeriesObservabilityTest, AverageQueryLatencyCalculated) {
    metrics->recordQuery("cpu", 4.0, 100, 60000);
    metrics->recordQuery("cpu", 6.0, 50,  60000);
    double avg = metrics->getAverageQueryLatency();
    EXPECT_NEAR(avg, 5.0, 0.01);
}

// ===== Compression metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordCompressionDoesNotCrash) {
    metrics->recordCompression("cpu", 1000, 100);
    double ratio = metrics->getAverageCompressionRatio();
    EXPECT_GT(ratio, 0.0);
}

TEST_F(TimeSeriesObservabilityTest, CompressionRatioCalculated) {
    // 1000 bytes uncompressed → 100 bytes compressed = 10:1 ratio
    metrics->recordCompression("cpu", 1000, 100);
    double ratio = metrics->getAverageCompressionRatio();
    EXPECT_NEAR(ratio, 10.0, 0.01);
}

// ===== Storage stats =====

TEST_F(TimeSeriesObservabilityTest, UpdateStorageStatsDoesNotCrash) {
    EXPECT_NO_THROW(metrics->updateStorageStats(1000, 5, 4096));
}

// ===== Retention metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordRetentionDoesNotCrash) {
    EXPECT_NO_THROW(metrics->recordRetention("cpu", 100, 50.0));
}

// ===== Continuous aggregate metrics =====

TEST_F(TimeSeriesObservabilityTest, RecordContinuousAggRegRefreshDoesNotCrash) {
    EXPECT_NO_THROW(metrics->recordContinuousAggregateRefresh("cpu", 60000, 5.0, 360));
}

// ===== Prometheus export =====

TEST_F(TimeSeriesObservabilityTest, PrometheusExportNotEmpty) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    std::string output = metrics->exportPrometheus();
    EXPECT_FALSE(output.empty());
}

TEST_F(TimeSeriesObservabilityTest, PrometheusExportContainsWriteCounter) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    std::string output = metrics->exportPrometheus();
    EXPECT_TRUE(prometheusContains(output, "themis_timeseries_data_points_written_total"));
}

TEST_F(TimeSeriesObservabilityTest, PrometheusExportContainsQueryCounter) {
    metrics->recordQuery("cpu", 1.0, 10, 60000);
    std::string output = metrics->exportPrometheus();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(prometheusContains(output, "themis_timeseries_queries_executed_total"));
}

TEST_F(TimeSeriesObservabilityTest, PrometheusExportIsValidText) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordQuery("mem", 2.0, 50, 3600000);
    std::string output = metrics->exportPrometheus();
    // Valid Prometheus format: each line should be either comment (#), metric, or empty
    std::istringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        EXPECT_TRUE(line[0] == '#' || std::isalpha(line[0]) || std::isdigit(line[0]))
            << "Invalid Prometheus line: " << line;
    }
}

// ===== JSON export =====

TEST_F(TimeSeriesObservabilityTest, JsonExportNotEmpty) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    std::string output = metrics->exportJson();
    EXPECT_FALSE(output.empty());
}

TEST_F(TimeSeriesObservabilityTest, JsonExportIsValidJson) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordQuery("mem", 2.0, 10, 60000);
    std::string output = metrics->exportJson();
    // Valid JSON should start with {
    EXPECT_FALSE(output.empty());
    // Strip leading whitespace
    size_t start = output.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        EXPECT_EQ(output[start], '{');
    }
}

// ===== Reset =====

TEST_F(TimeSeriesObservabilityTest, ResetClearsAllCounters) {
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordQuery("cpu", 1.0, 10, 60000);
    metrics->reset();
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 0u);
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 0u);
    EXPECT_EQ(metrics->getOptimizerHits(), 0u);
}

// ===== Thread safety =====

TEST_F(TimeSeriesObservabilityTest, ConcurrentWritesThreadSafe) {
    constexpr int threads = 4;
    constexpr int per_thread = 100;
    std::vector<std::thread> ts;
    for (int i = 0; i < threads; ++i) {
        ts.emplace_back([this]() {
            for (int j = 0; j < per_thread; ++j) {
                metrics->recordDataPointWrite("cpu", 1.0, true);
            }
        });
    }
    for (auto& t : ts) t.join();
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), static_cast<uint64_t>(threads * per_thread));
}
