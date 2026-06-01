/*
 * ThemisDB | File: bench_sanity.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <benchmark/benchmark.h>

static void BM_Sanity(benchmark::State& state) {
    for (auto _ : state) {
        int x = 0;
        for (int i=0;i<1000;i++) x += i;
        benchmark::DoNotOptimize(x);
    }
}

BENCHMARK(BM_Sanity);
