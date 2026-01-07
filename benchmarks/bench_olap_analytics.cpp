#include <benchmark/benchmark.h>

// OLAP analytics benchmarks disabled pending engine API alignment.
static void BM_OLAP_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
}

BENCHMARK(BM_OLAP_Disabled);
BENCHMARK_MAIN();
