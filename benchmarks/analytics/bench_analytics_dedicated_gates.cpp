// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_analytics_dedicated_gates.cpp
 * @brief Reproducible benchmarks for production analytics aggregation paths.
 *
 * The benchmark runner records the Google Benchmark JSON output as an artifact.
 * These measurements intentionally report observed latency/throughput without
 * claiming hardware-independent thresholds.
 */

#include <benchmark/benchmark.h>

#include "analytics/cep_engine.h"
#include "analytics/columnar_execution.h"

#include <chrono>
#include <cstdint>
#include <memory>

namespace {

using themisdb::analytics::AggregateSpec;
using themisdb::analytics::AggregationType;
using themisdb::analytics::Aggregator;
using themisdb::analytics::Column;
using themisdb::analytics::ColumnBatch;
using themisdb::analytics::ColumnType;
using themisdb::analytics::ColumnarExecutionEngine;
using themisdb::analytics::Event;
using themisdb::analytics::WindowConfig;
using themisdb::analytics::WindowManager;
using themisdb::analytics::WindowType;

ColumnBatch makeAggregationBatch(std::size_t row_count) {
    ColumnBatch batch;
    auto values = std::make_shared<Column>("value", ColumnType::Double);
    values->reserve(row_count);
    for (std::size_t row = 0; row < row_count; ++row) {
        values->appendDouble(static_cast<double>(row));
    }
    batch.addColumn(std::move(values));
    return batch;
}

Event makeWindowEvent(uint64_t sequence) {
    Event event;
    event.event_name = "analytics-benchmark";
    event.sequence_number = sequence;
    event.timestamp = std::chrono::system_clock::time_point{
        std::chrono::milliseconds{static_cast<int64_t>(sequence)}};
    event.setField("value", static_cast<double>(sequence));
    return event;
}

void BM_AnalyticsColumnarSum(benchmark::State& state) {
    constexpr std::size_t row_count = 1'000;
    const auto batch = makeAggregationBatch(row_count);
    ColumnarExecutionEngine engine;
    for (auto _ : state) {
        const auto result = engine.aggregate(batch, {{
            .result_name = "total",
            .input_column = "value",
            .function = AggregateSpec::Function::Sum
        }});
        const auto total = std::get<double>(result.getColumn("total")->get(0));
        if (total != 499'500.0) {
            state.SkipWithError("columnar SUM returned an unexpected result");
            break;
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(row_count));
}
BENCHMARK(BM_AnalyticsColumnarSum)->Repetitions(5)->ReportAggregatesOnly(true);

void BM_AnalyticsCepSum(benchmark::State& state) {
    constexpr int64_t event_count = 1'000;
    Aggregator aggregator;
    aggregator.addAggregation("total", AggregationType::SUM, "value");
    for (auto _ : state) {
        aggregator.reset();
        for (int64_t value = 0; value < event_count; ++value) {
            Event event;
            event.setField("value", static_cast<double>(value));
            aggregator.processEvent(event);
        }
        const auto result = aggregator.getResult("total");
        if (!result || !std::holds_alternative<double>(result->result)
            || std::get<double>(result->result) != 499'500.0) {
            state.SkipWithError("CEP SUM returned an unexpected result");
            break;
        }
        benchmark::DoNotOptimize(result->result);
    }
    state.SetItemsProcessed(state.iterations() * event_count);
}
BENCHMARK(BM_AnalyticsCepSum)->Repetitions(5)->ReportAggregatesOnly(true);

void BM_AnalyticsCountWindow(benchmark::State& state) {
    constexpr uint64_t window_size = 64;
    WindowConfig config;
    config.type = WindowType::COUNT;
    config.count = window_size;
    WindowManager windows(config);
    uint64_t emitted_events = 0;
    windows.setWindowCallback([&emitted_events](const std::vector<Event>& events, auto, auto) {
        emitted_events += events.size();
    });

    uint64_t sequence = 0;
    for (auto _ : state) {
        for (uint64_t i = 0; i < window_size; ++i) {
            windows.addEvent(makeWindowEvent(sequence++));
        }
        benchmark::DoNotOptimize(windows.getStats().windows_closed);
    }
    if (emitted_events != static_cast<uint64_t>(state.iterations()) * window_size) {
        state.SkipWithError("count window emitted an unexpected number of events");
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(window_size));
}
BENCHMARK(BM_AnalyticsCountWindow)->Repetitions(5)->ReportAggregatesOnly(true);

}  // namespace

BENCHMARK_MAIN();
