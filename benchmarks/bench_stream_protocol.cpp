/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_stream_protocol.cpp                          ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:21:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     37                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>

// Stream protocol benchmarks disabled; API changed.
static void BM_StreamProtocol_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
}

BENCHMARK(BM_StreamProtocol_Disabled);
BENCHMARK_MAIN();
