// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include "observability/metric_aggregator.h"
#include "observability/metrics_collector.h"
#include "observability/opentelemetry_tracer.h"
#include "observability/provenance_store.h"
#include "observability/query_profiler.h"
#include "observability/slo_reporter.h"

#include <chrono>

using namespace themis::observability;

namespace {

// Canonical seed for deterministic behavior
constexpr uint64_t kObservabilityBlockBSeed = 42;

// ============================================================================
// OBB-GATE-01: MetricsCollector ingest throughput
// Target: ≥5M metrics/sec
// ============================================================================
static void BM_OBB01_MetricsCollectorIngest(benchmark::State& state) {
    MetricsCollector collector;

    for (auto _ : state) {
        for (int i = 0; i < 100000; ++i) {
            collector.recordMetric({
                .metric_name = "bench_metric",
                .labels = {
                    {"label_1", "value_1"},
                    {"label_2", "value_2"},
                    {"label_3", "value_3"},
                },
                .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
                .value = static_cast<double>(i),
            });
        }
    }

    // Items processed = 100K metrics per iteration
    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_OBB01_MetricsCollectorIngest)->Repetitions(5);

// ============================================================================
// OBB-GATE-02: MetricsAggregator aggregation latency
// Target: ≤100µs for 1000-label aggregation
// ============================================================================
static void BM_OBB02_MetricsAggregatorLatency(benchmark::State& state) {
    MetricsAggregator aggregator;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    auto window_end = window_start + std::chrono::seconds(1);

    // Prepare 1000 samples
    std::vector<MetricSample> samples;
    for (int i = 0; i < 1000; ++i) {
        samples.push_back({
            .metric_name = "bench_metric",
            .labels = {{"id", std::to_string(i)}},
            .timestamp = window_start,
            .value = static_cast<double>(i),
        });
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(aggregator.aggregate(samples, window_start, window_end));
    }
}
BENCHMARK(BM_OBB02_MetricsAggregatorLatency)->Repetitions(5);

// ============================================================================
// OBB-GATE-03: Tracer span lifecycle
// Target: ≤10µs span start + end
// ============================================================================
static void BM_OBB03_TracerSpanLifecycle(benchmark::State& state) {
    OpenTelemetryTracer tracer;

    for (auto _ : state) {
        for (int i = 0; i < 100000; ++i) {
            auto span = tracer.startSpan("bench_op", {});
            if (span.has_value()) {
                benchmark::DoNotOptimize(tracer.endSpan(*span));
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_OBB03_TracerSpanLifecycle)->Repetitions(5);

// ============================================================================
// OBB-GATE-04: SloReporter evaluation latency
// Target: ≤100µs per rule evaluation
// ============================================================================
static void BM_OBB04_SloReporterEvaluation(benchmark::State& state) {
    SloReporter reporter;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    auto window_end = window_start + std::chrono::seconds(1);

    SloRule rule{
        .rule_id = "bench_slo",
        .metric_name = "latency_p99",
        .threshold = 100,
        .operator_type = ComparisonOperator::LESS_THAN_OR_EQUAL,
        .window_start = window_start,
        .window_end = window_end,
    };

    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            benchmark::DoNotOptimize(reporter.evaluateRule(rule, {{"latency_p99", 95.0}}));
        }
    }

    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_OBB04_SloReporterEvaluation)->Repetitions(5);

// ============================================================================
// OBB-GATE-05: QueryProfiler event recording
// Target: ≤5µs per latency event
// ============================================================================
static void BM_OBB05_QueryProfilerLatency(benchmark::State& state) {
    QueryProfiler profiler;

    for (auto _ : state) {
        for (int i = 0; i < 100000; ++i) {
            profiler.recordLatency("bench_query", static_cast<uint64_t>(i % 10000));
        }
    }

    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_OBB05_QueryProfilerLatency)->Repetitions(5);

// ============================================================================
// OBB-GATE-06: ProvisionStore recovery latency
// Target: ≤1ms for 10K state entries
// ============================================================================
static void BM_OBB06_ProvisionStoreRecovery(benchmark::State& state) {
    ProvisionStore store;

    // Pre-populate with 10K provisions
    for (int i = 0; i < 10000; ++i) {
        ProvisionState state_item{
            .provision_id = "prov_" + std::to_string(i),
            .status = ProvisionStatus::ACTIVE,
            .created_at = std::chrono::system_clock::now(),
            .metadata = {{"index", std::to_string(i)}},
        };
        store.persistState(state_item);
    }

    int index = 0;
    for (auto _ : state) {
        // Recover provisions in sequence
        auto result = store.loadState("prov_" + std::to_string(index % 10000));
        benchmark::DoNotOptimize(result);
        index++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OBB06_ProvisionStoreRecovery)->Repetitions(5);

} // namespace

BENCHMARK_MAIN();
