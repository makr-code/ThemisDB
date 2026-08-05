/*
 * ThemisDB | File: test_observability_hardening.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_observability_hardening.cpp
 * @brief Tests for structured logging, trace-context propagation, metric
 *        cardinality limits, and exporter health metrics.
 *
 * Covers the changes introduced for the "Observability Hardening: Structured
 * Logging and Trace Correlation" issue.
 */

#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include "core/concerns/i_logger.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "observability/metrics_collector.h"

#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

using namespace themis::core::concerns;
using namespace themis::observability;

// ---------------------------------------------------------------------------
// Helper: build a SpdlogLoggerAdapter backed by an ostringstream sink
// ---------------------------------------------------------------------------

static std::pair<std::unique_ptr<SpdlogLoggerAdapter>, std::shared_ptr<std::ostringstream>>
makeCapturingAdapter(bool json_mode) {
    auto stream = std::make_shared<std::ostringstream>();
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*stream);
    const std::string name = json_mode ? "test_obs_json" : "test_obs_plain";
    auto spdlog_logger = std::make_shared<spdlog::logger>(name,
                                                           spdlog::sinks_init_list{ostream_sink});
    spdlog_logger->set_level(spdlog::level::trace);
    spdlog_logger->set_pattern("%v");          // emit only the message payload
    auto adapter = std::make_unique<SpdlogLoggerAdapter>(spdlog_logger, json_mode);
    return {std::move(adapter), stream};
}

// ---------------------------------------------------------------------------
// TraceContext tests
// ---------------------------------------------------------------------------

TEST(TraceContextTest, EmptyContext) {
    TraceContext ctx;
    EXPECT_TRUE(ctx.empty());
    EXPECT_TRUE(ctx.trace_id.empty());
    EXPECT_TRUE(ctx.request_id.empty());
}

TEST(TraceContextTest, NonEmptyContext) {
    TraceContext ctx{"abc123", "", "req-456"};
    EXPECT_FALSE(ctx.empty());
    EXPECT_EQ("abc123", ctx.trace_id);
    EXPECT_EQ("req-456", ctx.request_id);
}

TEST(TraceContextTest, PartialContext) {
    TraceContext ctx{"only-trace", "", ""};
    EXPECT_FALSE(ctx.empty());
}

// ---------------------------------------------------------------------------
// Structured logging – NoOpLogger
// ---------------------------------------------------------------------------

TEST(NoOpLoggerStructuredTest, LogStructuredDoesNotThrow) {
    NoOpLogger logger;
    EXPECT_NO_THROW(logger.logStructured(ILogger::Level::INFO, "test",
                                          {{"key", "value"}}));
}

TEST(NoOpLoggerStructuredTest, LogWithContextDoesNotThrow) {
    NoOpLogger logger;
    TraceContext ctx{"trace-001", "", "req-001"};
    EXPECT_NO_THROW(logger.logWithContext(ILogger::Level::WARN, "ctx-msg", ctx,
                                           {{"component", "storage"}}));
}

// ---------------------------------------------------------------------------
// Structured logging – SpdlogLoggerAdapter JSON mode
// ---------------------------------------------------------------------------

class SpdlogAdapterJsonTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto [adapter, stream] = makeCapturingAdapter(/*json_mode=*/true);
        adapter_ = std::move(adapter);
        stream_  = std::move(stream);
    }
    void flush() { adapter_->info(""); } // ensure spdlog flush

    std::unique_ptr<SpdlogLoggerAdapter>    adapter_;
    std::shared_ptr<std::ostringstream>     stream_;
};

TEST_F(SpdlogAdapterJsonTest, JsonModeEnabled) {
    EXPECT_TRUE(adapter_->jsonMode());
}

TEST_F(SpdlogAdapterJsonTest, LogStructuredEmitsJsonObject) {
    adapter_->logStructured(ILogger::Level::INFO, "hello",
                             {{"component", "engine"}, {"op", "select"}});
    const std::string msg = stream_->str();
    EXPECT_NE(std::string::npos, msg.find("\"level\""));
    EXPECT_NE(std::string::npos, msg.find("\"message\""));
    EXPECT_NE(std::string::npos, msg.find("hello"));
    EXPECT_NE(std::string::npos, msg.find("\"component\""));
    EXPECT_NE(std::string::npos, msg.find("engine"));
}

TEST_F(SpdlogAdapterJsonTest, LogStructuredContainsTimestamp) {
    adapter_->logStructured(ILogger::Level::DEBUG, "ts-test", {});
    const std::string msg = stream_->str();
    EXPECT_NE(std::string::npos, msg.find("\"ts\""));
    // ISO-8601 marker: contains 'T' separator and 'Z' UTC suffix
    EXPECT_NE(std::string::npos, msg.find("T"));
    EXPECT_NE(std::string::npos, msg.find("Z"));
}

TEST_F(SpdlogAdapterJsonTest, LogStructuredRedactsSensitiveFields) {
    adapter_->logStructured(ILogger::Level::INFO, "auth",
                             {{"password", "super-secret"},
                              {"username", "alice"}});
    const std::string msg = stream_->str();
    // Password value must be redacted
    EXPECT_EQ(std::string::npos, msg.find("super-secret"));
    EXPECT_NE(std::string::npos, msg.find("[REDACTED]"));
    // Non-sensitive field preserved
    EXPECT_NE(std::string::npos, msg.find("alice"));
}

TEST_F(SpdlogAdapterJsonTest, TokenFieldIsRedacted) {
    adapter_->logStructured(ILogger::Level::INFO, "jwt",
                             {{"token", "eyJhbGciOiJIUzI1NiJ9.payload.sig"}});
    const std::string msg = stream_->str();
    EXPECT_EQ(std::string::npos, msg.find("eyJhbGciOiJIUzI1NiJ9"));
    EXPECT_NE(std::string::npos, msg.find("[REDACTED]"));
}

TEST_F(SpdlogAdapterJsonTest, LogWithContextInjectsTraceAndRequestId) {
    TraceContext ctx{"trace-deadbeef", "", "req-cafebabe"};
    adapter_->logWithContext(ILogger::Level::ERROR, "ctx-error", ctx,
                              {{"shard", "shard-3"}});
    const std::string msg = stream_->str();
    EXPECT_NE(std::string::npos, msg.find("trace-deadbeef"));
    EXPECT_NE(std::string::npos, msg.find("req-cafebabe"));
    EXPECT_NE(std::string::npos, msg.find("shard-3"));
}

TEST_F(SpdlogAdapterJsonTest, LogWithContextHandlesEmptyContext) {
    TraceContext ctx; // empty
    EXPECT_NO_THROW(
        adapter_->logWithContext(ILogger::Level::INFO, "no-ctx", ctx, {}));
    const std::string msg = stream_->str();
    EXPECT_NE(std::string::npos, msg.find("\"message\""));
    // trace_id / request_id keys must NOT appear for empty context
    EXPECT_EQ(std::string::npos, msg.find("trace_id"));
    EXPECT_EQ(std::string::npos, msg.find("request_id"));
}

TEST_F(SpdlogAdapterJsonTest, LogStructuredEscapesSpecialChars) {
    // A backslash in the value must be JSON-escaped to double-backslash
    adapter_->logStructured(ILogger::Level::WARN, "esc",
                             {{"k", "val\\slash"}});
    const std::string msg = stream_->str();
    // The un-escaped one-backslash form must not appear verbatim
    // (because "val\slash" would contain "val" + single_backslash + "slash")
    // In the JSON output the backslash is doubled, so searching for a
    // single-backslash sequence should fail.
    EXPECT_EQ(std::string::npos, msg.find("val\\slash"));
    // The escaped form (two backslashes in raw bytes) MUST be present
    EXPECT_NE(std::string::npos, msg.find("val\\\\slash"));
}

TEST_F(SpdlogAdapterJsonTest, PlainModeDoesNotEmitJson) {
    adapter_->setJsonMode(false);
    adapter_->logStructured(ILogger::Level::INFO, "plain", {{"k", "v"}});
    const std::string msg = stream_->str();
    // In plain mode the output is NOT a JSON object
    EXPECT_TRUE(msg.empty() || msg.front() != '{');
}

// ---------------------------------------------------------------------------
// Metric cardinality limits
// ---------------------------------------------------------------------------

class CardinalityTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
        MetricsCollector::getInstance().setCardinalityLimit(0); // start disabled
    }
    void TearDown() override {
        MetricsCollector::getInstance().setCardinalityLimit(0);
        MetricsCollector::getInstance().reset();
    }
};

TEST_F(CardinalityTest, DefaultLimitIsZeroDisabled) {
    EXPECT_EQ(0u, MetricsCollector::getInstance().getCardinalityLimit());
}

TEST_F(CardinalityTest, SetCardinalityLimitIsRetrievable) {
    MetricsCollector::getInstance().setCardinalityLimit(50);
    EXPECT_EQ(50u, MetricsCollector::getInstance().getCardinalityLimit());
}

TEST_F(CardinalityTest, SeriesWithinLimitAreAccepted) {
    MetricsCollector::getInstance().setCardinalityLimit(3);
    auto& mc = MetricsCollector::getInstance();
    mc.recordCacheHit("L1");
    mc.recordCacheHit("L2");
    mc.recordCacheHit("L3");
    EXPECT_EQ(0, mc.getDroppedSeriesCount());
}

TEST_F(CardinalityTest, SeriesBeyondLimitAreDropped) {
    MetricsCollector::getInstance().setCardinalityLimit(2);
    auto& mc = MetricsCollector::getInstance();
    mc.recordCacheHit("A");
    mc.recordCacheHit("B");
    mc.recordCacheHit("C"); // should be dropped
    mc.recordCacheHit("D"); // should be dropped
    EXPECT_GE(mc.getDroppedSeriesCount(), 2);
}

TEST_F(CardinalityTest, CardinalityOverflowEmitsDiagnosticMetric) {
    MetricsCollector::getInstance().setCardinalityLimit(1);
    auto& mc = MetricsCollector::getInstance();
    mc.recordCacheHit("A");
    mc.recordCacheHit("B"); // dropped and diagnosed

    const std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("metric_cardinality_exceeded_total"));
    EXPECT_NE(std::string::npos, metrics.find("metric=\"cache_hits_total\""));
}

TEST_F(CardinalityTest, ExistingSeriesAlwaysAllowed) {
    MetricsCollector::getInstance().setCardinalityLimit(1);
    auto& mc = MetricsCollector::getInstance();
    mc.recordCacheHit("X");
    int64_t dropped_before = mc.getDroppedSeriesCount();
    // Same label set: must never count as a new series
    mc.recordCacheHit("X");
    mc.recordCacheHit("X");
    EXPECT_EQ(dropped_before, mc.getDroppedSeriesCount());
}

TEST_F(CardinalityTest, ResetClearsDroppedCount) {
    MetricsCollector::getInstance().setCardinalityLimit(1);
    auto& mc = MetricsCollector::getInstance();
    mc.recordCacheHit("P");
    mc.recordCacheHit("Q"); // dropped
    EXPECT_GT(mc.getDroppedSeriesCount(), 0);
    mc.reset();
    EXPECT_EQ(0, mc.getDroppedSeriesCount());
}

TEST_F(CardinalityTest, ZeroLimitDisablesEnforcement) {
    // Cardinality limit 0 means unlimited
    MetricsCollector::getInstance().setCardinalityLimit(0);
    auto& mc = MetricsCollector::getInstance();
    for (int i = 0; i < 20; ++i) {
        mc.recordCacheHit("cache_" + std::to_string(i));
    }
    EXPECT_EQ(0, mc.getDroppedSeriesCount());
}

// ---------------------------------------------------------------------------
// Exporter health metrics
// ---------------------------------------------------------------------------

class ExporterHealthTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
};

TEST_F(ExporterHealthTest, RecordExporterFailureAppearsInMetrics) {
    auto& mc = MetricsCollector::getInstance();
    mc.recordExporterFailure("otlp");
    mc.recordExporterFailure("otlp");
    std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("exporter_failures_total"));
}

TEST_F(ExporterHealthTest, RecordExporterRecoveryAppearsInMetrics) {
    auto& mc = MetricsCollector::getInstance();
    mc.recordExporterRecovery("prometheus");
    std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("exporter_recoveries_total"));
}

TEST_F(ExporterHealthTest, DifferentExporterNamesProduceSeparateSeries) {
    auto& mc = MetricsCollector::getInstance();
    mc.recordExporterFailure("otlp");
    mc.recordExporterFailure("pushgateway");
    std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("otlp"));
    EXPECT_NE(std::string::npos, metrics.find("pushgateway"));
}

TEST_F(ExporterHealthTest, ExporterHealthGaugeTracksFailureAndRecovery) {
    auto& mc = MetricsCollector::getInstance();
    mc.recordExporterFailure("otlp");
    mc.recordExporterRecovery("otlp");

    const std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("exporter_health_status"));
    EXPECT_NE(std::string::npos, metrics.find("exporter=\"otlp\""));
    EXPECT_NE(std::string::npos, metrics.find(" 1.00"));
}

TEST_F(ExporterHealthTest, ExporterIncidentStatsExposeFailureAndRecoveryCounts) {
    auto& mc = MetricsCollector::getInstance();
    mc.recordExporterFailure("prometheus");
    mc.recordExporterFailure("prometheus");
    mc.recordExporterRecovery("prometheus");

    const auto stats = mc.getExporterIncidentStats("prometheus");
    EXPECT_EQ(2, stats.failures);
    EXPECT_EQ(1, stats.recoveries);
    EXPECT_EQ(0, stats.malformed_rejections);
}

TEST_F(ExporterHealthTest, ExporterIncidentStatsDoNotFoldMetricScopedMalformedTelemetry) {
    auto& mc = MetricsCollector::getInstance();
    const std::string oversized_value(kMaxLabelValueBytes + 1, 'x');

    mc.recordExporterFailure("otlp");
    mc.setGauge("invalid_gauge", 42.0, {{"key", oversized_value}});

    const auto stats = mc.getExporterIncidentStats("otlp");
    EXPECT_EQ(1, stats.failures);
    EXPECT_EQ(0, stats.recoveries);
    EXPECT_EQ(0, stats.malformed_rejections);

    const std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("malformed_telemetry_rejections_total"));
    EXPECT_NE(std::string::npos, metrics.find("metric=\"invalid_gauge\""));
}

TEST_F(ExporterHealthTest, InvalidLabelCountIsRejectedWithDiagnosticMetric) {
    auto& mc = MetricsCollector::getInstance();
    std::map<std::string, std::string> labels;
    for (std::size_t i = 0; i < kMaxMetricLabels + 1; ++i) {
        labels.emplace("label_" + std::to_string(i), "value");
    }

    mc.addCounter("invalid_metric_total", 1, labels);

    const std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("malformed_telemetry_rejections_total"));
    EXPECT_NE(std::string::npos, metrics.find("metric=\"invalid_metric_total\""));
    EXPECT_NE(std::string::npos, metrics.find("reason=\"label_count_exceeded\""));
    EXPECT_EQ(std::string::npos, metrics.find("invalid_metric_total{"));
}

TEST_F(ExporterHealthTest, OversizedMetricNameDiagnosticIsTruncatedSafely) {
    auto& mc = MetricsCollector::getInstance();
    const std::string oversized_metric(kMaxLabelValueBytes + 8, 'm');
    const std::string oversized_value(kMaxLabelValueBytes + 1, 'x');

    mc.setGauge(oversized_metric, 42.0, {{"key", oversized_value}});

    const std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("malformed_telemetry_rejections_total"));
    EXPECT_EQ(std::string::npos, metrics.find(oversized_metric + "\""));
    EXPECT_NE(std::string::npos,
              metrics.find(std::string("metric=\"") + oversized_metric.substr(0, kMaxLabelValueBytes) + "\""));
}

TEST_F(ExporterHealthTest, InvalidLabelValueIsRejectedWithDiagnosticMetric) {
    auto& mc = MetricsCollector::getInstance();
    const std::string oversized_value(kMaxLabelValueBytes + 1, 'x');

    mc.setGauge("invalid_gauge", 42.0, {{"key", oversized_value}});

    const std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("malformed_telemetry_rejections_total"));
    EXPECT_NE(std::string::npos, metrics.find("metric=\"invalid_gauge\""));
    EXPECT_NE(std::string::npos, metrics.find("reason=\"label_value_too_long\""));
    EXPECT_EQ(std::string::npos, metrics.find("invalid_gauge{"));
}
