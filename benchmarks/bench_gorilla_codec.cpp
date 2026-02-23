/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_gorilla_codec.cpp                            ║
  Version:         0.0.30                                             ║
  Last Modified:   2026-02-23 03:57:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     211                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Benchmark: Gorilla Codec – Compression Ratio & Speed
// Phase 7: Performance Benchmarks for Gorilla codec

#include "timeseries/gorilla.h"
#include <benchmark/benchmark.h>
#include <cmath>
#include <random>
#include <limits>

using namespace themis;

// ===== Helpers =====

static std::vector<std::pair<int64_t, double>> makeConstantSeries(int n) {
    std::vector<std::pair<int64_t, double>> s;
    s.reserve(n);
    for (int i = 0; i < n; ++i) {
        s.push_back({1700000000000LL + i * 1000LL, 42.0});
    }
    return s;
}

static std::vector<std::pair<int64_t, double>> makeSineSeries(int n) {
    std::vector<std::pair<int64_t, double>> s;
    s.reserve(n);
    for (int i = 0; i < n; ++i) {
        s.push_back({1700000000000LL + i * 1000LL, std::sin(i * 0.01) * 100.0});
    }
    return s;
}

static std::vector<std::pair<int64_t, double>> makeRandomSeries(int n, uint64_t seed = 42) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> dist(-1e6, 1e6);
    std::vector<std::pair<int64_t, double>> s;
    s.reserve(n);
    for (int i = 0; i < n; ++i) {
        s.push_back({1700000000000LL + i * 1000LL, dist(rng)});
    }
    return s;
}

static std::vector<uint8_t> encode(const std::vector<std::pair<int64_t, double>>& s) {
    GorillaEncoder enc;
    for (const auto& p : s) enc.add(p.first, p.second);
    return enc.finish();
}

// ===== Encoding Benchmarks =====

static void BM_GorillaEncode_Constant(benchmark::State& state) {
    const int n = state.range(0);
    auto series = makeConstantSeries(n);
    for (auto _ : state) {
        GorillaEncoder enc;
        for (const auto& p : series) enc.add(p.first, p.second);
        auto bytes = enc.finish();
        benchmark::DoNotOptimize(bytes);
    }
    const size_t raw_bytes = n * (sizeof(int64_t) + sizeof(double));
    auto series_encoded = encode(series);
    state.counters["compression_ratio"] =
        static_cast<double>(raw_bytes) / std::max(series_encoded.size(), size_t(1));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaEncode_Constant)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

static void BM_GorillaEncode_Sine(benchmark::State& state) {
    const int n = state.range(0);
    auto series = makeSineSeries(n);
    for (auto _ : state) {
        GorillaEncoder enc;
        for (const auto& p : series) enc.add(p.first, p.second);
        auto bytes = enc.finish();
        benchmark::DoNotOptimize(bytes);
    }
    const size_t raw_bytes = n * (sizeof(int64_t) + sizeof(double));
    auto series_encoded = encode(series);
    state.counters["compression_ratio"] =
        static_cast<double>(raw_bytes) / std::max(series_encoded.size(), size_t(1));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaEncode_Sine)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

static void BM_GorillaEncode_Random(benchmark::State& state) {
    const int n = state.range(0);
    auto series = makeRandomSeries(n);
    for (auto _ : state) {
        GorillaEncoder enc;
        for (const auto& p : series) enc.add(p.first, p.second);
        auto bytes = enc.finish();
        benchmark::DoNotOptimize(bytes);
    }
    const size_t raw_bytes = n * (sizeof(int64_t) + sizeof(double));
    auto series_encoded = encode(series);
    state.counters["compression_ratio"] =
        static_cast<double>(raw_bytes) / std::max(series_encoded.size(), size_t(1));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaEncode_Random)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

// ===== Decoding Benchmarks =====

static void BM_GorillaDecode_Constant(benchmark::State& state) {
    const int n = state.range(0);
    auto compressed = encode(makeConstantSeries(n));
    for (auto _ : state) {
        GorillaDecoder dec(compressed);
        std::vector<std::pair<int64_t, double>> out;
        out.reserve(n);
        while (auto p = dec.next()) out.push_back(*p);
        benchmark::DoNotOptimize(out);
    }
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaDecode_Constant)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

static void BM_GorillaDecode_Sine(benchmark::State& state) {
    const int n = state.range(0);
    auto compressed = encode(makeSineSeries(n));
    for (auto _ : state) {
        GorillaDecoder dec(compressed);
        std::vector<std::pair<int64_t, double>> out;
        out.reserve(n);
        while (auto p = dec.next()) out.push_back(*p);
        benchmark::DoNotOptimize(out);
    }
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaDecode_Sine)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

// ===== Error Recovery Benchmark =====

static void BM_GorillaDecode_Truncated(benchmark::State& state) {
    // Measure overhead of decoding truncated (error) data
    const int n = 1000;
    auto full = encode(makeSineSeries(n));
    // Truncate to 60%
    std::vector<uint8_t> truncated(full.begin(), full.begin() + full.size() * 6 / 10);
    for (auto _ : state) {
        GorillaDecoder dec(truncated);
        int count = 0;
        while (dec.next()) ++count;
        benchmark::DoNotOptimize(count);
    }
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaDecode_Truncated)->Unit(benchmark::kMicrosecond);

// ===== Compression Ratio Summary =====

static void BM_GorillaCompressionRatio(benchmark::State& state) {
    const int n = state.range(0);
    // Use realistic monitoring data: small random walk around a baseline
    std::mt19937_64 rng(0xDEADBEEF);
    std::normal_distribution<double> noise(0.0, 0.5);
    std::vector<std::pair<int64_t, double>> series;
    series.reserve(n);
    double val = 50.0;
    for (int i = 0; i < n; ++i) {
        val += noise(rng);
        series.push_back({1700000000000LL + i * 10000LL, val});
    }
    auto compressed = encode(series);
    const size_t raw_bytes = n * (sizeof(int64_t) + sizeof(double));
    for (auto _ : state) {
        GorillaEncoder enc;
        for (const auto& p : series) enc.add(p.first, p.second);
        auto bytes = enc.finish();
        benchmark::DoNotOptimize(bytes);
    }
    state.counters["raw_bytes"]         = static_cast<double>(raw_bytes);
    state.counters["compressed_bytes"]  = static_cast<double>(compressed.size());
    state.counters["compression_ratio"] =
        static_cast<double>(raw_bytes) / std::max(compressed.size(), size_t(1));
    state.counters["points_per_sec"]    = benchmark::Counter(
        static_cast<double>(state.iterations() * n), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_GorillaCompressionRatio)
    ->Arg(100)->Arg(1000)->Arg(10000)->Arg(100000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
