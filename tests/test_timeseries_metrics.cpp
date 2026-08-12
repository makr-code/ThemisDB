#include <gtest/gtest.h>
#include "timeseries/timeseries_metrics.h"
#include <thread>
#include <chrono>

using namespace themis;

class TimeSeriesMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        TimeSeriesMetrics::Config config;
        config.enable_histograms = true;
        config.enable_per_metric_stats = true;
        metrics = std::make_shared<TimeSeriesMetrics>(config);
    }

    std::shared_ptr<TimeSeriesMetrics> metrics;
};

TEST_F(TimeSeriesMetricsTest, TestDataPointWrite) {
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 0);
    
    metrics->recordDataPointWrite("cpu", 1.5, true);
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 1);
    
    metrics->recordDataPointWrite("memory", 2.0, true);
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 2);
    
    EXPECT_GT(metrics->getAverageWriteLatency(), 0.0);
}

TEST_F(TimeSeriesMetricsTest, TestBatchWrite) {
    EXPECT_EQ(metrics->getTotalBatchesWritten(), 0);
    
    metrics->recordBatchWrite(100, 5.0, true, true);
    EXPECT_EQ(metrics->getTotalBatchesWritten(), 1);
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 100);
    
    metrics->recordBatchWrite(50, 2.5, false, true);
    EXPECT_EQ(metrics->getTotalBatchesWritten(), 2);
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 150);
}

TEST_F(TimeSeriesMetricsTest, TestCompression) {
    metrics->recordCompression("temp", 10000, 1000);
    
    EXPECT_DOUBLE_EQ(metrics->getAverageCompressionRatio(), 10.0);
    
    metrics->recordCompression("humidity", 5000, 500);
    EXPECT_DOUBLE_EQ(metrics->getAverageCompressionRatio(), 10.0);
}

TEST_F(TimeSeriesMetricsTest, TestQuery) {
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 0);
    
    metrics->recordQuery("cpu", 10.5, 500, 3600000);
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 1);
    
    EXPECT_GT(metrics->getAverageQueryLatency(), 0.0);
}

TEST_F(TimeSeriesMetricsTest, TestAggregation) {
    EXPECT_EQ(metrics->getTotalAggregationsExecuted(), 0);
    
    metrics->recordAggregation("cpu", 5.0, 1000, true);
    EXPECT_EQ(metrics->getTotalAggregationsExecuted(), 1);
    EXPECT_EQ(metrics->getOptimizerHits(), 1);
    EXPECT_EQ(metrics->getOptimizerMisses(), 0);
    
    metrics->recordAggregation("memory", 8.0, 2000, false);
    EXPECT_EQ(metrics->getTotalAggregationsExecuted(), 2);
    EXPECT_EQ(metrics->getOptimizerHits(), 1);
    EXPECT_EQ(metrics->getOptimizerMisses(), 1);
}

TEST_F(TimeSeriesMetricsTest, TestStorageStats) {
    metrics->updateStorageStats(10000, 50, 1048576);
    
    // Currently these are exposed via export methods, not direct getters
    // Just verify no crash
    std::string json = metrics->exportJson();
    EXPECT_FALSE(json.empty());
}

TEST_F(TimeSeriesMetricsTest, TestRetention) {
    metrics->recordRetention("cpu", 100, 15.0);
    metrics->recordRetention("", 500, 50.0);
    
    std::string json = metrics->exportJson();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("retention"), std::string::npos);
}

TEST_F(TimeSeriesMetricsTest, TestPrometheusExport) {
    // Record some metrics
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordBatchWrite(100, 5.0, true, true);
    metrics->recordQuery("cpu", 10.0, 500, 3600000);
    metrics->recordAggregation("cpu", 5.0, 1000, true);
    metrics->updateStorageStats(1000, 10, 1048576);
    
    std::string prom_text = metrics->exportPrometheus();
    
    EXPECT_FALSE(prom_text.empty());
    EXPECT_NE(prom_text.find("themis_timeseries_data_points_written_total"), std::string::npos);
    EXPECT_NE(prom_text.find("themis_timeseries_queries_executed_total"), std::string::npos);
    EXPECT_NE(prom_text.find("themis_timeseries_current_storage_bytes"), std::string::npos);
    EXPECT_NE(prom_text.find("# HELP"), std::string::npos);
    EXPECT_NE(prom_text.find("# TYPE"), std::string::npos);
}

TEST_F(TimeSeriesMetricsTest, TestJsonExport) {
    // Record some metrics
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordBatchWrite(100, 5.0, true, true);
    metrics->recordQuery("cpu", 10.0, 500, 3600000);
    metrics->recordAggregation("cpu", 5.0, 1000, true);
    metrics->updateStorageStats(1000, 10, 1048576);
    metrics->recordCompression("cpu", 10000, 1000);
    
    std::string json_text = metrics->exportJson();
    
    EXPECT_FALSE(json_text.empty());
    EXPECT_NE(json_text.find("\"ingestion\""), std::string::npos);
    EXPECT_NE(json_text.find("\"query\""), std::string::npos);
    EXPECT_NE(json_text.find("\"optimizer\""), std::string::npos);
    EXPECT_NE(json_text.find("\"storage\""), std::string::npos);
    EXPECT_NE(json_text.find("\"retention\""), std::string::npos);
    EXPECT_NE(json_text.find("\"compression_ratio_avg\""), std::string::npos);
}

TEST_F(TimeSeriesMetricsTest, TestReset) {
    // Record some metrics
    metrics->recordDataPointWrite("cpu", 1.0, true);
    metrics->recordBatchWrite(100, 5.0, true, true);
    metrics->recordQuery("cpu", 10.0, 500, 3600000);
    
    EXPECT_GT(metrics->getTotalDataPointsWritten(), 0);
    EXPECT_GT(metrics->getTotalQueriesExecuted(), 0);
    
    metrics->reset();
    
    EXPECT_EQ(metrics->getTotalDataPointsWritten(), 0);
    EXPECT_EQ(metrics->getTotalBatchesWritten(), 0);
    EXPECT_EQ(metrics->getTotalQueriesExecuted(), 0);
    EXPECT_EQ(metrics->getTotalAggregationsExecuted(), 0);
    EXPECT_EQ(metrics->getOptimizerHits(), 0);
    EXPECT_EQ(metrics->getOptimizerMisses(), 0);
}

TEST_F(TimeSeriesMetricsTest, TestContinuousAggregateMetrics) {
    metrics->recordContinuousAggregateRefresh("cpu", 60000, 25.0, 1000);
    
    std::string json = metrics->exportJson();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("continuous_aggregates"), std::string::npos);
}

TEST_F(TimeSeriesMetricsTest, TestPerMetricStats) {
    TimeSeriesMetrics::Config config;
    config.enable_per_metric_stats = true;
    auto metrics_per = std::make_shared<TimeSeriesMetrics>(config);
    
    metrics_per->recordDataPointWrite("cpu", 1.0, true);
    metrics_per->recordDataPointWrite("cpu", 1.5, true);
    metrics_per->recordDataPointWrite("memory", 2.0, true);
    metrics_per->recordQuery("cpu", 10.0, 100, 3600000);
    
    std::string json = metrics_per->exportJson();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("per_metric_stats"), std::string::npos);
    EXPECT_NE(json.find("\"cpu\""), std::string::npos);
    EXPECT_NE(json.find("\"memory\""), std::string::npos);
}

TEST_F(TimeSeriesMetricsTest, TestErrorRecording) {
    metrics->recordDataPointWrite("cpu", 1.0, false);
    
    std::string json = metrics->exportJson();
    EXPECT_NE(json.find("write_errors_total"), std::string::npos);
}

TEST_F(TimeSeriesMetricsTest, TestOutOfOrderWriteRecording) {
    EXPECT_EQ(metrics->getOutOfOrderAccepted(), 0u);
    EXPECT_EQ(metrics->getLateArrivalRejected(), 0u);

    // Record one accepted out-of-order and two rejections
    metrics->recordOutOfOrderWrite("cpu", /*rejected=*/false);
    metrics->recordOutOfOrderWrite("mem", /*rejected=*/true);
    metrics->recordOutOfOrderWrite("mem", /*rejected=*/true);

    EXPECT_EQ(metrics->getOutOfOrderAccepted(), 1u);
    EXPECT_EQ(metrics->getLateArrivalRejected(), 2u);

    // Verify exported JSON contains the new fields
    std::string json = metrics->exportJson();
    EXPECT_NE(json.find("out_of_order_accepted_total"), std::string::npos);
    EXPECT_NE(json.find("late_arrival_rejected_total"), std::string::npos);

    // Verify Prometheus export contains the new metrics
    std::string prom = metrics->exportPrometheus();
    EXPECT_NE(prom.find("themis_timeseries_out_of_order_accepted_total"), std::string::npos);
    EXPECT_NE(prom.find("themis_timeseries_late_arrival_rejected_total"), std::string::npos);

    // Verify reset clears the new counters
    metrics->reset();
    EXPECT_EQ(metrics->getOutOfOrderAccepted(), 0u);
    EXPECT_EQ(metrics->getLateArrivalRejected(), 0u);
}
