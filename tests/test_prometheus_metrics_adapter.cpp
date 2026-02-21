/*
 * Unit tests for PrometheusMetricsAdapter
 *
 * Validates that each IMetrics method is forwarded to the underlying
 * MetricsCollector singleton and that the exported Prometheus text
 * format reflects the recorded observations.
 */

#include "core/concerns/prometheus_metrics_adapter.h"
#include "observability/metrics_collector.h"
#include <gtest/gtest.h>
#include <string>

using namespace themis::core::concerns;
using namespace themis::observability;

class PrometheusMetricsAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset the singleton collector before each test for isolation.
        MetricsCollector::getInstance().reset();
        adapter_ = std::make_unique<PrometheusMetricsAdapter>();
    }

    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }

    std::unique_ptr<PrometheusMetricsAdapter> adapter_;
};

// ===== Counter Tests =====

TEST_F(PrometheusMetricsAdapterTest, IncrementCounterByOne) {
    adapter_->incrementCounter("requests_total");
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("requests_total"));
}

TEST_F(PrometheusMetricsAdapterTest, IncrementCounterByMultiple) {
    adapter_->incrementCounter("requests_total", 5);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("requests_total"));
    EXPECT_NE(std::string::npos, output.find("5"));
}

TEST_F(PrometheusMetricsAdapterTest, IncrementCounterWithLabels) {
    adapter_->incrementCounter("http_requests_total", 1, {{"method", "GET"}, {"status", "200"}});
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("http_requests_total"));
    EXPECT_NE(std::string::npos, output.find("method=\"GET\""));
}

TEST_F(PrometheusMetricsAdapterTest, IncrementCounterAccumulates) {
    adapter_->incrementCounter("ops_total", 3);
    adapter_->incrementCounter("ops_total", 7);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("ops_total"));
    EXPECT_NE(std::string::npos, output.find("10"));
}

// ===== Gauge Tests =====

TEST_F(PrometheusMetricsAdapterTest, SetGauge) {
    adapter_->setGauge("active_connections", 42.0);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("active_connections"));
    EXPECT_NE(std::string::npos, output.find("42"));
}

TEST_F(PrometheusMetricsAdapterTest, SetGaugeOverwrites) {
    adapter_->setGauge("queue_depth", 10.0);
    adapter_->setGauge("queue_depth", 25.0);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("queue_depth"));
    EXPECT_NE(std::string::npos, output.find("25"));
}

TEST_F(PrometheusMetricsAdapterTest, IncrementGauge) {
    adapter_->setGauge("workers", 5.0);
    adapter_->incrementGauge("workers", 3.0);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("workers"));
    EXPECT_NE(std::string::npos, output.find("8"));
}

TEST_F(PrometheusMetricsAdapterTest, DecrementGauge) {
    adapter_->setGauge("workers", 10.0);
    adapter_->decrementGauge("workers", 4.0);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("workers"));
    EXPECT_NE(std::string::npos, output.find("6"));
}

TEST_F(PrometheusMetricsAdapterTest, SetGaugeWithLabels) {
    adapter_->setGauge("pod_count", 3.0, {{"namespace", "default"}});
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("pod_count"));
    EXPECT_NE(std::string::npos, output.find("namespace=\"default\""));
}

// ===== Histogram Tests =====

TEST_F(PrometheusMetricsAdapterTest, ObserveHistogram) {
    adapter_->observeHistogram("request_duration_seconds", 0.05);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("request_duration_seconds"));
}

TEST_F(PrometheusMetricsAdapterTest, ObserveHistogramMultipleSamples) {
    for (int i = 1; i <= 10; ++i) {
        adapter_->observeHistogram("latency_ms", static_cast<double>(i * 10));
    }
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("latency_ms"));
    // Summary format includes count
    EXPECT_NE(std::string::npos, output.find("latency_ms_count"));
}

TEST_F(PrometheusMetricsAdapterTest, ObserveHistogramWithLabels) {
    adapter_->observeHistogram("db_query_duration_ms", 15.5, {{"operation", "SELECT"}});
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("db_query_duration_ms"));
    EXPECT_NE(std::string::npos, output.find("operation=\"SELECT\""));
}

// ===== Convenience Method Tests =====

TEST_F(PrometheusMetricsAdapterTest, RecordLatencyCreatesHistogram) {
    adapter_->recordLatency("db.query", 12.5);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("db.query_latency_ms"));
}

TEST_F(PrometheusMetricsAdapterTest, RecordLatencyWithLabels) {
    adapter_->recordLatency("cache.get", 0.5, {{"result", "hit"}});
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("cache.get_latency_ms"));
    EXPECT_NE(std::string::npos, output.find("result=\"hit\""));
}

TEST_F(PrometheusMetricsAdapterTest, RecordErrorCreatesCounter) {
    adapter_->recordError("db.write");
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("db.write_errors_total"));
}

TEST_F(PrometheusMetricsAdapterTest, RecordSuccessCreatesCounter) {
    adapter_->recordSuccess("db.read");
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("db.read_success_total"));
}

// ===== Export Format Tests =====

TEST_F(PrometheusMetricsAdapterTest, ExportMetricsReturnsNonEmptyAfterRecording) {
    adapter_->incrementCounter("test_counter");
    std::string output = adapter_->exportMetrics();
    EXPECT_FALSE(output.empty());
}

TEST_F(PrometheusMetricsAdapterTest, ExportedOutputContainsBuildInfo) {
    std::string output = adapter_->exportMetrics();
    // MetricsCollector always emits a build-info gauge
    EXPECT_NE(std::string::npos, output.find("themis_build_info"));
}

TEST_F(PrometheusMetricsAdapterTest, ExportedOutputContainsTypeAnnotations) {
    adapter_->incrementCounter("requests_total");
    adapter_->setGauge("connections", 5.0);
    std::string output = adapter_->exportMetrics();
    EXPECT_NE(std::string::npos, output.find("# TYPE"));
}

// ===== Lifecycle Tests =====

TEST_F(PrometheusMetricsAdapterTest, ResetClearsMetrics) {
    adapter_->incrementCounter("some_counter", 99);
    adapter_->reset();
    std::string output = adapter_->exportMetrics();
    EXPECT_EQ(std::string::npos, output.find("some_counter"));
}

TEST_F(PrometheusMetricsAdapterTest, FlushDoesNotThrow) {
    EXPECT_NO_THROW(adapter_->flush());
}

TEST_F(PrometheusMetricsAdapterTest, ShutdownDoesNotThrow) {
    EXPECT_NO_THROW(adapter_->shutdown());
}

TEST_F(PrometheusMetricsAdapterTest, IsHealthyReturnsTrue) {
    auto result = adapter_->isHealthy();
    EXPECT_TRUE(result.ok);
}

// ===== Cardinality Control Tests =====

TEST_F(PrometheusMetricsAdapterTest, CardinalityLimitDropsExcessSeries) {
    MetricsCollector::getInstance().setCardinalityLimit(2);

    adapter_->incrementCounter("evt", 1, {{"label", "a"}});
    adapter_->incrementCounter("evt", 1, {{"label", "b"}});
    adapter_->incrementCounter("evt", 1, {{"label", "c"}}); // should be dropped

    EXPECT_GE(MetricsCollector::getInstance().getDroppedSeriesCount(), 1);

    // Reset cardinality limit for other tests
    MetricsCollector::getInstance().setCardinalityLimit(0);
}
