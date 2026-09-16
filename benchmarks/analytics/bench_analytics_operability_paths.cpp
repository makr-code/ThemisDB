// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_analytics_operability_paths.cpp
 * @brief Direct Wave-D operability benchmarks for analytics export, serving,
 *        distributed retry, and high-cardinality streaming paths.
 *
 * Replaces proxy-only measurement for analytics operability surfaces with
 * direct module-path benchmarks:
 *   - AO-01 JSON export serialization throughput
 *   - AO-02 CSV export serialization throughput
 *   - AO-03 high-cardinality streaming rejection overhead
 *   - AO-04 distributed retry / recovery latency
 *   - AO-05 serving fail-closed input validation latency
 */

#include <benchmark/benchmark.h>

#include "analytics/analytics_export.h"
#include "analytics/distributed_analytics.h"
#include "analytics/ml_serving.h"
#include "analytics/streaming_window.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int64_t kBenchmarkRowCount = 10'000;
constexpr int64_t kStreamingRecordCount = 250'000;
constexpr uint64_t kCanonicalSeed = 42;

::themis::analytics::ArrowRecordBatch makeExportBatch(std::size_t rows) {
    using Batch = ::themis::analytics::ArrowRecordBatch;

    Batch batch;
    batch.addColumn({"id", Batch::DataType::INT64, false});
    batch.addColumn({"category", Batch::DataType::STRING, false});
    batch.addColumn({"score", Batch::DataType::DOUBLE, false});

    for (std::size_t i = 0; i < rows; ++i) {
        batch.appendRow({
            static_cast<int64_t>(i),
            std::string("group_") + std::to_string(i % 64),
            static_cast<double>(i % 10'000) / 100.0,
        });
    }
    return batch;
}

std::vector<::themis::analytics::StreamRecord> makeHighCardinalityRecords(std::size_t count) {
    using ::themis::analytics::StreamRecord;

    std::vector<StreamRecord> records;
    records.reserve(count);

    const auto base = std::chrono::system_clock::time_point{};
    for (std::size_t i = 0; i < count; ++i) {
        StreamRecord record;
        record.record_id = "rec_" + std::to_string(i);
        record.event_time = base + std::chrono::milliseconds(static_cast<int64_t>(i % 1000));
        record.partition_key = "user_" + std::to_string(kCanonicalSeed + i);
        record.ingest_time = base + std::chrono::milliseconds(static_cast<int64_t>(i % 1000));
        record.set("value", static_cast<double>(i % 1024));
        records.push_back(std::move(record));
    }
    return records;
}

class RetryingShardExecutor final : public ::themisdb::analytics::ShardQueryExecutor {
public:
    std::atomic<int> failures_before_success{1};

    ::themis::analytics::OLAPResult execute(const std::string&,
                                            const ::themis::analytics::OLAPQuery&) override {
        const int remaining = failures_before_success.fetch_sub(1, std::memory_order_relaxed);
        if (remaining > 0) {
            throw std::runtime_error("temporary timeout");
        }
        ::themis::analytics::OLAPResult result;
        result.rows.push_back({});
        return result;
    }

    bool isHealthy() const override { return true; }
};

static void BM_AO01_ExportJsonToString(benchmark::State& state) {
    auto exporter = ::themis::analytics::ExporterFactory::createDefaultExporter();
    auto batch = makeExportBatch(static_cast<std::size_t>(state.range(0)));
    ::themis::analytics::ExportOptions options;
    options.format = ::themis::analytics::ExportFormat::JSON;

    for (auto _ : state) {
        auto serialized = exporter->exportToString(batch, options);
        benchmark::DoNotOptimize(serialized);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch.rowCount()));
    state.SetLabel("AO-01 direct JSON export serialization");
}
BENCHMARK(BM_AO01_ExportJsonToString)->Arg(kBenchmarkRowCount)->UseRealTime()->Repetitions(5);

static void BM_AO02_ExportCsvToString(benchmark::State& state) {
    auto exporter = ::themis::analytics::ExporterFactory::createDefaultExporter();
    auto batch = makeExportBatch(static_cast<std::size_t>(state.range(0)));
    ::themis::analytics::ExportOptions options;
    options.format = ::themis::analytics::ExportFormat::CSV;

    for (auto _ : state) {
        auto serialized = exporter->exportToString(batch, options);
        benchmark::DoNotOptimize(serialized);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch.rowCount()));
    state.SetLabel("AO-02 direct CSV export serialization");
}
BENCHMARK(BM_AO02_ExportCsvToString)->Arg(kBenchmarkRowCount)->UseRealTime()->Repetitions(5);

static void BM_AO03_HighCardinalityStreamingBounded(benchmark::State& state) {
    auto records = makeHighCardinalityRecords(static_cast<std::size_t>(state.range(0)));

    for (auto _ : state) {
        ::themis::analytics::TumblingWindowConfig config;
        config.size = std::chrono::seconds(30);
        config.max_distinct_partition_keys = 64;

        auto window = ::themis::analytics::createTumblingWindow(config);
        for (const auto& record : records) {
            benchmark::DoNotOptimize(window->ingest(record));
        }
        benchmark::DoNotOptimize(window->getStats());
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(records.size()));
    state.SetLabel("AO-03 direct high-cardinality bounded streaming");
}
BENCHMARK(BM_AO03_HighCardinalityStreamingBounded)
    ->Arg(kStreamingRecordCount)
    ->UseRealTime()
    ->Repetitions(5);

static void BM_AO04_DistributedRetryRecovery(benchmark::State& state) {
    ::themisdb::analytics::DistributedAnalyticsSharding::Config config;
    config.allow_partial_results = true;
    config.enable_circuit_breaker = false;
    config.retry_config.max_retries = 1;
    config.retry_config.base_delay_ms = 0;
    config.retry_config.max_delay_ms = 0;

    auto coordinator = std::make_unique<::themisdb::analytics::DistributedAnalyticsSharding>(config);
    auto executor = std::make_shared<RetryingShardExecutor>();
    coordinator->addShard("retrying-shard", executor);

    ::themis::analytics::OLAPQuery query;
    query.collection = "analytics_operability";
    query.tenant_id = "bench";

    for (auto _ : state) {
        executor->failures_before_success.store(1, std::memory_order_relaxed);
        auto result = coordinator->executeDistributed(query);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("AO-04 direct distributed retry and recovery");
}
BENCHMARK(BM_AO04_DistributedRetryRecovery)->UseRealTime()->Repetitions(5);

static void BM_AO05_ServingInvalidInputFastFail(benchmark::State& state) {
    ::themisdb::analytics::MLServingClient client;
    ::themisdb::analytics::DataPoint empty_point;

    for (auto _ : state) {
        auto response = client.inferFromDataPoint("operability-model", empty_point);
        benchmark::DoNotOptimize(response);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("AO-05 direct serving fail-closed input validation");
}
BENCHMARK(BM_AO05_ServingInvalidInputFastFail)->UseRealTime()->Repetitions(5);

} // namespace
