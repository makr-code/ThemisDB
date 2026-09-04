/*
 * CUDA acceleration gate benchmarks for B-01.
 *
 * Measures GPU-vs-CPU speedup for vector similarity distance kernels (L2,
 * cosine, inner-product) and enforces the gate `>= 8x` when CUDA is available.
 */

#include <benchmark/benchmark.h>

#include "acceleration/cpu_backend.h"
#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#include "acceleration/kernel_invocation.h"

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

using namespace themis::acceleration;

namespace {

std::vector<float> randomFloatVectors(size_t count, size_t dim, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> data(count * dim);
    for (auto &v : data) {
        v = dist(rng);
    }
    return data;
}

void runSpeedupGate(benchmark::State &state, DistanceMetric metric) {
    constexpr int kNumQueries = 1;
    const int dim             = static_cast<int>(state.range(0));
    const int numVectors      = static_cast<int>(state.range(1));

    auto queries = randomFloatVectors(static_cast<size_t>(kNumQueries), static_cast<size_t>(dim), 17);
    auto vectors = randomFloatVectors(static_cast<size_t>(numVectors), static_cast<size_t>(dim), 23);
    std::vector<float> out_cpu(static_cast<size_t>(kNumQueries) * static_cast<size_t>(numVectors), 0.0f);
    std::vector<float> out_gpu(static_cast<size_t>(kNumQueries) * static_cast<size_t>(numVectors), 0.0f);

    CPUVectorBackend cpu_backend = {};
    if (!cpu_backend.initialize()) {
        state.SkipWithError("CPUVectorBackend::initialize() failed");
        return;
    }
    const ANNKernelDispatch cpu_dispatch = cpu_backend.populateANNDispatch();
    ANNDistanceFn cpu_distance_fn        = cpu_dispatch.distanceLauncherFor(metric);
    if (cpu_distance_fn == nullptr) {
        state.SkipWithError("CPU dispatch table missing requested metric");
        return;
    }

#ifndef THEMIS_ENABLE_CUDA
    state.SkipWithError("THEMIS_ENABLE_CUDA is OFF; CUDA speedup gate skipped");
    return;
#else
    CUDAVectorBackend cuda_backend = {};
    if (!cuda_backend.initialize()) {
        state.SkipWithError("CUDAVectorBackend::initialize() failed");
        return;
    }
    const ANNKernelDispatch gpu_dispatch = cuda_backend.populateANNDispatch();
    ANNDistanceFn gpu_distance_fn        = gpu_dispatch.distanceLauncherFor(metric);
    if (gpu_distance_fn == nullptr) {
        state.SkipWithError("CUDA dispatch table missing requested metric");
        return;
    }

    double cpu_total_us = 0.0;
    double gpu_total_us = 0.0;

    for (auto _ : state) {
        const auto cpu_t0 = std::chrono::steady_clock::now();
        const int cpu_rc = cpu_distance_fn(queries.data(), vectors.data(), out_cpu.data(), kNumQueries, numVectors, dim, nullptr);
        const auto cpu_t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(cpu_rc);
        if (cpu_rc != 0) {
            state.SkipWithError("CPU distance kernel returned non-zero");
            return;
        }
        cpu_total_us += std::chrono::duration<double, std::micro>(cpu_t1 - cpu_t0).count();

        const auto gpu_t0 = std::chrono::steady_clock::now();
        const int gpu_rc = gpu_distance_fn(queries.data(), vectors.data(), out_gpu.data(), kNumQueries, numVectors, dim, nullptr);
        const auto gpu_t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(gpu_rc);
        if (gpu_rc != 0) {
            state.SkipWithError("CUDA distance kernel returned non-zero");
            return;
        }
        gpu_total_us += std::chrono::duration<double, std::micro>(gpu_t1 - gpu_t0).count();
    }

    const double speedup = (gpu_total_us > 0.0) ? (cpu_total_us / gpu_total_us) : 0.0;
    state.counters["cpu_total_us"] = cpu_total_us;
    state.counters["gpu_total_us"] = gpu_total_us;
    state.counters["speedup"] = speedup;
    state.SetLabel("B-01 gate: required speedup >= 8.0x");
    if (speedup < 8.0) {
        state.SkipWithError("ACC-CUDA-B01-01 failed: speedup < 8x");
    }
#endif
}

} // namespace

static void BM_CudaVsCpuL2SpeedupGate(benchmark::State &state) {
    runSpeedupGate(state, DistanceMetric::L2);
}
BENCHMARK(BM_CudaVsCpuL2SpeedupGate)->Args({128, 100000})->Unit(benchmark::kMicrosecond);

static void BM_CudaVsCpuCosineSpeedupGate(benchmark::State &state) {
    runSpeedupGate(state, DistanceMetric::COSINE);
}
BENCHMARK(BM_CudaVsCpuCosineSpeedupGate)->Args({128, 100000})->Unit(benchmark::kMicrosecond);

static void BM_CudaVsCpuInnerProductSpeedupGate(benchmark::State &state) {
    runSpeedupGate(state, DistanceMetric::INNER_PRODUCT);
}
BENCHMARK(BM_CudaVsCpuInnerProductSpeedupGate)->Args({128, 100000})->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

