// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "observability/metric_aggregator.h"
#include "observability/metrics_collector.h"
#include "observability/opentelemetry_tracer.h"
#include "observability/query_profiler.h"

#include <chrono>
#include <map>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

namespace {

TEST(ObservabilityBlockBMetricsCollector, ExporterIncidentsAreTracked) {
    auto& collector = MetricsCollector::getInstance();
    collector.reset();

    collector.recordExporterFailure("otlp");
    collector.recordExporterFailure("otlp");
    collector.recordExporterRecovery("otlp");
    collector.recordMalformedTelemetry("span_duration_ms", "invalid_label_value");

    const auto stats = collector.getExporterIncidentStats("otlp");
    EXPECT_EQ(stats.failures, 2);
    EXPECT_EQ(stats.recoveries, 1);
    EXPECT_EQ(stats.malformed_rejections, 0);

    collector.reset();
}

TEST(ObservabilityBlockBMetricsAggregator, RateAndHistogramAggregationWork) {
    MetricAggregator aggregator;

    aggregator.recordCounterSample("requests_total", 10);
    std::this_thread::sleep_for(1ms);
    aggregator.recordCounterSample("requests_total", 16);

    const double rate = aggregator.calculateRate("requests_total");
    EXPECT_GT(rate, 0.0);

    HistogramSnapshot snapshot;
    snapshot.metric_name = "latency_ms";
    snapshot.labels = {{"service", "query"}};
    snapshot.values = {10.0, 20.0, 30.0};

    aggregator.addHistogramSnapshot(snapshot);

    const AggregatedMetric aggregated = aggregator.aggregateHistograms("latency_ms", AggregationType::SUM);
    EXPECT_EQ(aggregated.metric_name, "latency_ms");
    EXPECT_EQ(aggregated.type, AggregationType::SUM);
    EXPECT_DOUBLE_EQ(aggregated.value, 60.0);
}

TEST(ObservabilityBlockBTracer, SpanContextInjectionIsDeterministic) {
    OpenTelemetryTracer tracer;

    OpenTelemetryTracer::clearBaggage();
    OpenTelemetryTracer::setBaggageItem("tenant-id", "tenant-42");

    const std::map<std::string, std::string> headers{
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"},
        {"baggage", "tenant-id=tenant-42"},
    };

    const auto extracted = tracer.extractContext(headers);
    EXPECT_TRUE(extracted.isValid());

    auto span = tracer.startSpanFromHeaders("operation-1", headers);
    ASSERT_NE(span, nullptr);
    EXPECT_TRUE(span->isValid());

    std::map<std::string, std::string> outbound;
    tracer.injectContext(*span, outbound);

    EXPECT_NE(outbound.find("traceparent"), outbound.end());
    EXPECT_NE(outbound.find("baggage"), outbound.end());

    span->end();
    OpenTelemetryTracer::clearBaggage();
}

TEST(ObservabilityBlockBQueryProfiler, LifecycleCapturesQueryState) {
    QueryProfiler profiler;

    const std::string query_id = profiler.start_query("q1", "SELECT 1");
    profiler.record_phase(query_id, QueryPhase::EXECUTE, std::chrono::microseconds{125});

    OperatorStats stats{};
    stats.type = OperatorType::SCAN;
    stats.name = "table_scan";
    stats.duration = std::chrono::microseconds{45};
    stats.rows_processed = 1;
    profiler.record_operator(query_id, stats);
    profiler.add_hint(query_id, "use covering index");
    profiler.add_warning(query_id, "planner fallback used");
    profiler.end_query(query_id);

    const auto profile = profiler.get_profile(query_id);
    ASSERT_NE(profile, nullptr);
    EXPECT_EQ(profile->query_id, query_id);
    EXPECT_EQ(profile->query_text, "SELECT 1");
    EXPECT_EQ(profile->phase_timings.at(QueryPhase::EXECUTE), std::chrono::microseconds{125});
    ASSERT_EQ(profile->operator_stats.size(), 1u);
    EXPECT_EQ(profile->operator_stats.front().name, "table_scan");
    EXPECT_EQ(profile->warnings.size(), 1u);
}

TEST(ObservabilityBlockBIntegration, ComponentsRemainUsableTogether) {
    auto& collector = MetricsCollector::getInstance();
    collector.reset();

    MetricAggregator aggregator;
    OpenTelemetryTracer tracer;
    QueryProfiler profiler;

    aggregator.recordCounterSample("batch_total", 100);
    aggregator.recordCounterSample("batch_total", 130);

    HistogramSnapshot snapshot;
    snapshot.metric_name = "batch_latency_ms";
    snapshot.values = {2.0, 4.0, 6.0};
    aggregator.addHistogramSnapshot(snapshot);

    auto span = tracer.startSpan("batch-op");
    ASSERT_NE(span, nullptr);
    auto outbound = std::map<std::string, std::string>{};
    tracer.injectContext(*span, outbound);
    span->end();

    const std::string query_id = profiler.start_query("batch-query", "SELECT count(*) FROM t");
    profiler.record_phase(query_id, QueryPhase::EXECUTE, std::chrono::microseconds{10});
    profiler.end_query(query_id);

    collector.addCounter("observability_block_b_runs", 1);
    collector.recordExporterRecovery("otlp");

    EXPECT_GT(aggregator.calculateRate("batch_total"), 0.0);
    EXPECT_DOUBLE_EQ(aggregator.aggregateHistograms("batch_latency_ms", AggregationType::AVG).value, 4.0);
    EXPECT_NE(profiler.get_profile(query_id), nullptr);
    EXPECT_NE(collector.getPrometheusMetrics().find("observability_block_b_runs"), std::string::npos);

    collector.reset();
}

} // namespace
