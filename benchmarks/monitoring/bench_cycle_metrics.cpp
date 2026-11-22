// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_cycle_metrics.cpp
 * @brief Google Benchmark cases for CPU cycle-counter and memory primitives.
 *
 * These microbenchmarks measure only the operations in their case names. They
 * do not model or measure HNSW, LLM inference, GPU/PCIe activity, or a RAG
 * pipeline.
 */

#include "performance/cycle_metrics.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

void BM_CycleCounterRead(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            themis::performance::HardwareCycleCounter::cpu_cycles());
    }
}
BENCHMARK(BM_CycleCounterRead);

void BM_SerializedCycleCounterRead(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            themis::performance::HardwareCycleCounter::rdtscp());
    }
}
BENCHMARK(BM_SerializedCycleCounterRead);

void BM_PointerAssignment(benchmark::State& state) {
    const std::uint8_t value = 0x42;
    for (auto _ : state) {
        const std::uint8_t* pointer = &value;
        benchmark::DoNotOptimize(pointer);
    }
}
BENCHMARK(BM_PointerAssignment);

void BM_MemoryCopy(benchmark::State& state) {
    const auto size = static_cast<std::size_t>(state.range(0));
    const std::vector<std::uint8_t> source(size, 0x42);
    std::vector<std::uint8_t> destination(size);

    for (auto _ : state) {
        std::memcpy(destination.data(), source.data(), size);
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(size));
}
BENCHMARK(BM_MemoryCopy)->Arg(10 * 1024);

void BM_CacheLineStrideRead(benchmark::State& state) {
    constexpr std::size_t kCacheLineBytes = 64;
    const auto size = static_cast<std::size_t>(state.range(0));
    const std::vector<std::uint64_t> data(size / sizeof(std::uint64_t), 1);
    const auto stride = kCacheLineBytes / sizeof(std::uint64_t);

    for (auto _ : state) {
        std::uint64_t sum = 0;
        for (std::size_t index = 0; index < data.size(); index += stride) {
            sum += data[index];
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(size));
}
BENCHMARK(BM_CacheLineStrideRead)
    ->Arg(32 * 1024)
    ->Arg(256 * 1024)
    ->Arg(8 * 1024 * 1024)
    ->Arg(64 * 1024 * 1024);

} // namespace

BENCHMARK_MAIN();
