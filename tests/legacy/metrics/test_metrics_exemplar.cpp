/**
 * @file test_metrics_exemplar.cpp
 * @brief Unit tests for Prometheus exemplar support on histogram metrics.
 *
 * Covers:
 *  - Exemplar struct default construction
 *  - observeHistogramWithExemplar() records value and stores exemplar
 *  - Prometheus export includes exemplar on p99 quantile line
 *  - Last-write-wins: re-recording overwrites previous exemplar
 *  - observeHistogram() without exemplar: no exemplar emitted
 *  - reset() clears exemplar state
 *  - Empty trace_id is not emitted (graceful no-op)
 */

#include <gtest/gtest.h>
#include "observability/metrics_collector.h"

#include <string>

using namespace themis::observability;

class ExemplarTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
};

// ---------------------------------------------------------------------------
// Exemplar struct
// ---------------------------------------------------------------------------

TEST(ExemplarStructTest, DefaultConstruction) {
    Exemplar ex;
    EXPECT_TRUE(ex.trace_id.empty());
    EXPECT_DOUBLE_EQ(0.0, ex.value);
    // timestamp should be initialized to a time after the Unix epoch
    auto epoch = std::chrono::system_clock::time_point{};
    EXPECT_GT(ex.timestamp, epoch)
        << "Expected timestamp to be after epoch";
}

TEST(ExemplarStructTest, ValueConstruction) {
    Exemplar ex("abc123", 42.5);
    EXPECT_EQ("abc123", ex.trace_id);
    EXPECT_DOUBLE_EQ(42.5, ex.value);
}

// ---------------------------------------------------------------------------
// observeHistogramWithExemplar – basic recording
// ---------------------------------------------------------------------------

TEST_F(ExemplarTest, RecordWithExemplar_NoThrow) {
    auto& mc = MetricsCollector::getInstance();
    Exemplar ex("trace-001", 10.0);
    EXPECT_NO_THROW(mc.observeHistogramWithExemplar("test_latency_ms", 10.0, ex));
}

TEST_F(ExemplarTest, RecordWithExemplar_ExistsInPrometheusOutput) {
    auto& mc = MetricsCollector::getInstance();
    Exemplar ex("deadbeef01234567", 37.5);
    mc.observeHistogramWithExemplar("req_latency_ms", 37.5, ex, {{"op", "read"}});

    std::string prom = mc.getPrometheusMetrics();

    // The exemplar trace ID must appear in the output
    EXPECT_NE(std::string::npos, prom.find("deadbeef01234567"))
        << "Expected trace ID in Prometheus output:\n" << prom;
    // The OpenMetrics exemplar marker
    EXPECT_NE(std::string::npos, prom.find("traceID"))
        << "Expected 'traceID' key in Prometheus output:\n" << prom;
    // The metric itself must be present
    EXPECT_NE(std::string::npos, prom.find("req_latency_ms"))
        << "Expected metric name in Prometheus output:\n" << prom;
}

TEST_F(ExemplarTest, ExemplarAttachedToP99Line) {
    auto& mc = MetricsCollector::getInstance();
    // Record several values so all quantiles are non-trivial
    for (double v : {10.0, 20.0, 30.0, 40.0, 50.0}) {
        mc.observeHistogramWithExemplar("latency_ms", v, Exemplar("trace-x", v));
    }

    std::string prom = mc.getPrometheusMetrics();

    // The p99 line contains the exemplar (quantile="0.99" and traceID together)
    auto p99_pos = prom.find("quantile=\"0.99\"");
    ASSERT_NE(std::string::npos, p99_pos)
        << "p99 quantile line not found in:\n" << prom;

    // Find the newline that terminates the p99 line
    auto eol = prom.find('\n', p99_pos);
    std::string p99_line = prom.substr(p99_pos, eol - p99_pos);

    EXPECT_NE(std::string::npos, p99_line.find("traceID"))
        << "Expected exemplar on p99 line: " << p99_line;
}

// ---------------------------------------------------------------------------
// Last-write-wins
// ---------------------------------------------------------------------------

TEST_F(ExemplarTest, LastWriteWins_LatestExemplarInOutput) {
    auto& mc = MetricsCollector::getInstance();
    mc.observeHistogramWithExemplar("svc_latency_ms", 5.0,  Exemplar("first-trace", 5.0));
    mc.observeHistogramWithExemplar("svc_latency_ms", 99.0, Exemplar("second-trace", 99.0));

    std::string prom = mc.getPrometheusMetrics();

    // Only the latest exemplar should appear
    EXPECT_NE(std::string::npos, prom.find("second-trace"))
        << "Expected second-trace in output:\n" << prom;
    EXPECT_EQ(std::string::npos, prom.find("first-trace"))
        << "first-trace must have been overwritten:\n" << prom;
}

// ---------------------------------------------------------------------------
// observeHistogram without exemplar – no traceID emitted
// ---------------------------------------------------------------------------

TEST_F(ExemplarTest, ObserveWithoutExemplar_NoTraceIdInOutput) {
    auto& mc = MetricsCollector::getInstance();
    mc.observeHistogram("plain_latency_ms", 25.0);

    std::string prom = mc.getPrometheusMetrics();

    EXPECT_NE(std::string::npos, prom.find("plain_latency_ms"));
    EXPECT_EQ(std::string::npos, prom.find("traceID"))
        << "Unexpected traceID in output for non-exemplar histogram:\n" << prom;
}

// ---------------------------------------------------------------------------
// Empty trace_id is a no-op (exemplar not stored)
// ---------------------------------------------------------------------------

TEST_F(ExemplarTest, EmptyTraceId_NoExemplarEmitted) {
    auto& mc = MetricsCollector::getInstance();
    Exemplar empty_ex("", 5.0);  // empty trace_id
    mc.observeHistogramWithExemplar("noop_latency_ms", 5.0, empty_ex);

    std::string prom = mc.getPrometheusMetrics();

    EXPECT_NE(std::string::npos, prom.find("noop_latency_ms"));
    EXPECT_EQ(std::string::npos, prom.find("traceID"))
        << "traceID must not appear when trace_id is empty:\n" << prom;
}

// ---------------------------------------------------------------------------
// reset() clears exemplar
// ---------------------------------------------------------------------------

TEST_F(ExemplarTest, Reset_ClearsExemplar) {
    auto& mc = MetricsCollector::getInstance();
    mc.observeHistogramWithExemplar("reset_latency_ms", 7.0, Exemplar("pre-reset-trace", 7.0));

    // Confirm exemplar present before reset
    {
        std::string prom = mc.getPrometheusMetrics();
        EXPECT_NE(std::string::npos, prom.find("pre-reset-trace"));
    }

    mc.reset();

    // After reset, trace ID must not appear
    {
        std::string prom = mc.getPrometheusMetrics();
        EXPECT_EQ(std::string::npos, prom.find("pre-reset-trace"))
            << "Expected exemplar cleared after reset";
    }
}
