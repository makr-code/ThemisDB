// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include "observability/metric_aggregator.h"
#include "observability/metrics_collector.h"
#include "observability/opentelemetry_tracer.h"
#include "observability/query_profiler.h"
#include "observability/slo_reporter.h"

#include <chrono>
#include <map>
#include <string>
#include <vector>

using namespace themis::observability;

namespace {

struct MockProvisionState {
    std::string provision_id;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point created_at;
};

struct MockProvisionStore {
    std::map<std::string, MockProvisionState> items;

    void persistState(const MockProvisionState& item) {
        items[item.provision_id] = item;
    }

    const MockProvisionState* loadState(const std::string& id) const {
        const auto it = items.find(id);
        if (it == items.end()) {
            return nullptr;
        }
        return &it->second;
    }
};

constexpr uint64_t kObservabilityBlockBSeed = 42;

static void BM_OBB01_MetricsCollectorIngest(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    collector.reset();

    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            collector.addCounter(
                "bench_metric_total",
                1,
                {{"label_1", "value_1"}, {"label_2", "value_2"}, {"label_3", "value_3"}});
            collector.observeHistogram(
                "bench_metric_ms",
                static_cast<double>(i % 1000),
                {{"label_1", "value_1"}, {"label_2", "value_2"}});
        }
    }

    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_OBB01_MetricsCollectorIngest)->Repetitions(5);

static void BM_OBB02_MetricsAggregatorLatency(benchmark::State& state) {
    MetricAggregator aggregator;
    const auto now = std::chrono::system_clock::now();
    const auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    const auto window_end = window_start + std::chrono::seconds(1);

    HistogramSnapshot snapshot;
    snapshot.metric_name = "bench_metric";
    snapshot.labels = {{"service", "bench"}};
    snapshot.values.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        snapshot.values.push_back(static_cast<double>(i));
    }

    for (auto _ : state) {
        aggregator.addHistogramSnapshot(snapshot);
        benchmark::DoNotOptimize(aggregator.aggregateHistograms(
            "bench_metric",
            AggregationType::AVG,
            {{"service", "bench"}}));
    }
}
BENCHMARK(BM_OBB02_MetricsAggregatorLatency)->Repetitions(5);

static void BM_OBB03_TracerSpanLifecycle(benchmark::State& state) {
    OpenTelemetryTracer tracer;

    for (auto _ : state) {
        for (int i = 0; i < 100000; ++i) {
            auto span = tracer.startSpan("bench_op");
            if (span != nullptr) {
                span->setAttribute("iteration", static_cast<int64_t>(i));
                benchmark::DoNotOptimize(span.get());
                span->end();
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_OBB03_TracerSpanLifecycle)->Repetitions(5);

static void BM_OBB04_SloReporterEvaluation(benchmark::State& state) {
    SloReporter reporter;
    SloDefinition slo;
    slo.name = "bench_slo";
    slo.objective = 0.999;
    slo.window = std::chrono::hours(1);
    reporter.registerSlo(slo);

    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            reporter.record("bench_slo", true);
        }
        benchmark::DoNotOptimize(reporter.getStatus("bench_slo"));
    }

    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_OBB04_SloReporterEvaluation)->Repetitions(5);

static void BM_OBB05_QueryProfilerLatency(benchmark::State& state) {
    QueryProfiler profiler;

    for (auto _ : state) {
        for (int i = 0; i < 100000; ++i) {
            const std::string query_id = "bench_query_" + std::to_string(i);
            const std::string query_text = "SELECT 1";
            const std::string tracked_id = profiler.start_query(query_id, query_text);
            profiler.record_phase(tracked_id, QueryPhase::EXECUTE, std::chrono::microseconds{10});
            profiler.end_query(tracked_id);
        }
    }

    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_OBB05_QueryProfilerLatency)->Repetitions(5);

static void BM_OBB06_ProvisionStoreRecovery(benchmark::State& state) {
    MockProvisionStore store;
    for (int i = 0; i < 10000; ++i) {
        MockProvisionState item;
        item.provision_id = "prov_" + std::to_string(i);
        item.created_at = std::chrono::system_clock::now();
        item.metadata = {{"index", std::to_string(i)}};
        store.persistState(item);
    }

    int index = 0;
    for (auto _ : state) {
        const auto* result = store.loadState("prov_" + std::to_string(index % 10000));
        benchmark::DoNotOptimize(result);
        ++index;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OBB06_ProvisionStoreRecovery)->Repetitions(5);

} // namespace

BENCHMARK_MAIN();
