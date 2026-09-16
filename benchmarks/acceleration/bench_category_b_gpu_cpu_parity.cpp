#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

#include "acceleration/cpu_backend.h"
#include "acceleration/cuda_backend.h"

using namespace themis::acceleration;

namespace {

std::vector<uint32_t> sortedVertices(const std::vector<uint32_t>& input) {
    std::vector<uint32_t> out = input;
    std::sort(out.begin(), out.end());
    return out;
}

void BM_CategoryB_BFS_Parity(benchmark::State& state) {
#ifndef THEMIS_ENABLE_CUDA
    state.SkipWithError("THEMIS_ENABLE_CUDA is OFF; Category-B BFS parity gate skipped");
    return;
#else
    CUDAGraphBackend gpu_backend;
    if (!gpu_backend.isAvailable() || !gpu_backend.initialize()) {
        state.SkipWithError("CUDA graph backend unavailable");
        return;
    }

    CPUGraphBackend cpu_backend;
    if (!cpu_backend.initialize()) {
        gpu_backend.shutdown();
        state.SkipWithError("CPU graph backend initialization failed");
        return;
    }

    constexpr size_t kNumVertices = 6;
    std::vector<uint32_t> adjacency(kNumVertices * kNumVertices, 0u);
    adjacency[0 * kNumVertices + 1] = 1u;
    adjacency[0 * kNumVertices + 2] = 1u;
    adjacency[1 * kNumVertices + 3] = 1u;
    adjacency[2 * kNumVertices + 4] = 1u;
    adjacency[4 * kNumVertices + 5] = 1u;

    const std::array<uint32_t, 2> starts = {0u, 2u};
    constexpr uint32_t kMaxDepth = 3u;

    double cpu_total_us = 0.0;
    double gpu_total_us = 0.0;

    for (auto _ : state) {
        const auto cpu_t0 = std::chrono::steady_clock::now();
        const auto cpu_result =
            cpu_backend.batchBFS(adjacency.data(), kNumVertices, starts.data(), starts.size(), kMaxDepth);
        const auto cpu_t1 = std::chrono::steady_clock::now();

        const auto gpu_t0 = std::chrono::steady_clock::now();
        const auto gpu_result =
            gpu_backend.batchBFS(adjacency.data(), kNumVertices, starts.data(), starts.size(), kMaxDepth);
        const auto gpu_t1 = std::chrono::steady_clock::now();

        if (cpu_result.size() != gpu_result.size()) {
            state.SkipWithError("BFS parity mismatch: result batch size differs");
            break;
        }

        for (std::size_t i = 0; i < cpu_result.size(); ++i) {
            if (sortedVertices(cpu_result[i]) != sortedVertices(gpu_result[i])) {
                state.SkipWithError("BFS parity mismatch: visited set differs");
                break;
            }
        }

        cpu_total_us += std::chrono::duration<double, std::micro>(cpu_t1 - cpu_t0).count();
        gpu_total_us += std::chrono::duration<double, std::micro>(gpu_t1 - gpu_t0).count();
    }

    state.counters["cpu_total_us"] = cpu_total_us;
    state.counters["gpu_total_us"] = gpu_total_us;
    state.counters["pairs"] = static_cast<double>(starts.size());

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}

void BM_CategoryB_Dijkstra_Parity(benchmark::State& state) {
#ifndef THEMIS_ENABLE_CUDA
    state.SkipWithError("THEMIS_ENABLE_CUDA is OFF; Category-B Dijkstra parity gate skipped");
    return;
#else
    CUDAGraphBackend gpu_backend;
    if (!gpu_backend.isAvailable() || !gpu_backend.initialize()) {
        state.SkipWithError("CUDA graph backend unavailable");
        return;
    }

    CPUGraphBackend cpu_backend;
    if (!cpu_backend.initialize()) {
        gpu_backend.shutdown();
        state.SkipWithError("CPU graph backend initialization failed");
        return;
    }

    constexpr size_t kNumVertices = 5;
    std::vector<uint32_t> adjacency(kNumVertices * kNumVertices, 0u);
    std::vector<float> weights(kNumVertices * kNumVertices, 0.0f);

    adjacency[0 * kNumVertices + 1] = 1u; weights[0 * kNumVertices + 1] = 1.0f;
    adjacency[1 * kNumVertices + 4] = 1u; weights[1 * kNumVertices + 4] = 2.0f;
    adjacency[0 * kNumVertices + 2] = 1u; weights[0 * kNumVertices + 2] = 2.0f;
    adjacency[2 * kNumVertices + 3] = 1u; weights[2 * kNumVertices + 3] = 2.0f;
    adjacency[3 * kNumVertices + 4] = 1u; weights[3 * kNumVertices + 4] = 2.0f;
    adjacency[1 * kNumVertices + 3] = 1u; weights[1 * kNumVertices + 3] = 1.0f;

    const std::array<uint32_t, 3> starts = {0u, 0u, 1u};
    const std::array<uint32_t, 3> ends = {4u, 3u, 4u};

    double cpu_total_us = 0.0;
    double gpu_total_us = 0.0;

    for (auto _ : state) {
        const auto cpu_t0 = std::chrono::steady_clock::now();
        const auto cpu_result = cpu_backend.batchShortestPath(
            adjacency.data(), weights.data(), kNumVertices, starts.data(), ends.data(), starts.size());
        const auto cpu_t1 = std::chrono::steady_clock::now();

        const auto gpu_t0 = std::chrono::steady_clock::now();
        const auto gpu_result = gpu_backend.batchShortestPath(
            adjacency.data(), weights.data(), kNumVertices, starts.data(), ends.data(), starts.size());
        const auto gpu_t1 = std::chrono::steady_clock::now();

        if (cpu_result != gpu_result) {
            state.SkipWithError("Dijkstra parity mismatch: path outputs differ");
            break;
        }

        cpu_total_us += std::chrono::duration<double, std::micro>(cpu_t1 - cpu_t0).count();
        gpu_total_us += std::chrono::duration<double, std::micro>(gpu_t1 - gpu_t0).count();
    }

    state.counters["cpu_total_us"] = cpu_total_us;
    state.counters["gpu_total_us"] = gpu_total_us;
    state.counters["pairs"] = static_cast<double>(starts.size());

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}

void BM_CategoryB_GeoContains_Parity(benchmark::State& state) {
#ifndef THEMIS_ENABLE_CUDA
    state.SkipWithError("THEMIS_ENABLE_CUDA is OFF; Category-B geo parity gate skipped");
    return;
#else
    CUDAGeoBackend gpu_backend;
    if (!gpu_backend.isAvailable() || !gpu_backend.initialize()) {
        state.SkipWithError("CUDA geo backend unavailable");
        return;
    }

    CPUGeoBackend cpu_backend;
    if (!cpu_backend.initialize()) {
        gpu_backend.shutdown();
        state.SkipWithError("CPU geo backend initialization failed");
        return;
    }

    const std::array<double, 6> point_lats = {0.5, 2.5, 1.0, -1.0, 0.25, 3.0};
    const std::array<double, 6> point_lons = {0.5, 2.5, 0.0, 0.0, 1.75, 1.0};
    const std::array<double, 8> polygon = {
        0.0, 0.0,
        0.0, 2.0,
        2.0, 2.0,
        2.0, 0.0
    };

    double cpu_total_us = 0.0;
    double gpu_total_us = 0.0;

    for (auto _ : state) {
        const auto cpu_t0 = std::chrono::steady_clock::now();
        const auto cpu_result = cpu_backend.batchPointInPolygon(
            point_lats.data(), point_lons.data(), point_lats.size(), polygon.data(), polygon.size() / 2);
        const auto cpu_t1 = std::chrono::steady_clock::now();

        const auto gpu_t0 = std::chrono::steady_clock::now();
        const auto gpu_result = gpu_backend.batchPointInPolygon(
            point_lats.data(), point_lons.data(), point_lats.size(), polygon.data(), polygon.size() / 2);
        const auto gpu_t1 = std::chrono::steady_clock::now();

        if (cpu_result != gpu_result) {
            state.SkipWithError("Geo ST_CONTAINS parity mismatch");
            break;
        }

        cpu_total_us += std::chrono::duration<double, std::micro>(cpu_t1 - cpu_t0).count();
        gpu_total_us += std::chrono::duration<double, std::micro>(gpu_t1 - gpu_t0).count();
    }

    state.counters["cpu_total_us"] = cpu_total_us;
    state.counters["gpu_total_us"] = gpu_total_us;
    state.counters["points"] = static_cast<double>(point_lats.size());

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}

} // namespace

BENCHMARK(BM_CategoryB_BFS_Parity)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_CategoryB_Dijkstra_Parity)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_CategoryB_GeoContains_Parity)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
