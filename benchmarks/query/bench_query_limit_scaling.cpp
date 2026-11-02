#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <nlohmann/json.hpp>

#include "query/vectorized_execution.h"

namespace {

void BM_VectorizedLimit10PullSource(benchmark::State& state) {
    const auto available_rows = static_cast<std::size_t>(state.range(0));
    themis::query::VectorizedExecutionEngine::Config config;
    config.batch_size = 1024;
    themis::query::VectorizedExecutionEngine engine(config);
    themis::query::VectorizedQueryPlan plan;
    plan.setLimit(10);
    std::size_t rows_generated = 0;
    std::size_t peak_working_bytes = 0;

    const themis::query::VectorizedExecutionEngine::RowBatchSource source =
        [&rows_generated](std::size_t offset, std::size_t count) {
            std::vector<nlohmann::json> batch;
            batch.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                batch.push_back({{"id", offset + i}});
            }
            rows_generated += batch.size();
            return batch;
        };

    for (auto _ : state) {
        engine.resetStats();
        auto result = engine.execute(available_rows, source, plan);
        if (!result.has_value()) {
            state.SkipWithError(result.error().message().c_str());
            break;
        }
        if (result->size() != 10) {
            state.SkipWithError("LIMIT 10 did not return ten rows");
            break;
        }
        peak_working_bytes =
            std::max(peak_working_bytes, engine.lastStats().peak_memory_bytes);
        benchmark::DoNotOptimize(result->data());
    }

    state.counters["available_rows"] = static_cast<double>(available_rows);
    state.counters["source_rows_per_iteration"] =
        static_cast<double>(rows_generated) /
        std::max(1.0, static_cast<double>(state.iterations()));
    state.counters["peak_accounted_working_bytes"] =
        static_cast<double>(peak_working_bytes);
    state.SetItemsProcessed(state.iterations() * 10);
}

BENCHMARK(BM_VectorizedLimit10PullSource)
    ->Arg(100000)
    ->Arg(10000000)
    ->MinTime(0.1);

}  // namespace

BENCHMARK_MAIN();
