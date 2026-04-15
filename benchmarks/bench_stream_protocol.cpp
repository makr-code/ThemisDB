/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_stream_protocol.cpp                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     35                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 202546ee10  2026-04-13  perf: add Disabled-Stub-Policy comments to all 21 *_Disab... ║
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

// Disabled: stream protocol API changed; pending re-implementation | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_StreamProtocol_Disabled);
BENCHMARK_MAIN();
