#include <benchmark/benchmark.h>

// Async I/O multiscan benchmarks disabled pending API update.
static void BM_AsyncIO_Multiscan_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
}

// Disabled: async I/O multiscan pending API update | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_AsyncIO_Multiscan_Disabled);
BENCHMARK_MAIN();
