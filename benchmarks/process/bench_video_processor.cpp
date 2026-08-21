#include <benchmark/benchmark.h>

// Video processor benchmarks disabled; plugin interface unavailable in this build.
static void BM_VideoProcessor_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
}

// Disabled: video processor plugin interface unavailable in this build | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_VideoProcessor_Disabled);
BENCHMARK_MAIN();
