// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file bench_gpu_cpu_breakeven_category_b.cpp
/// @brief Phase D CPU baseline benchmarks for GPU break-even Category B kernels.
///
/// Category B covers graph and geospatial kernels that require >= 1.3x GPU
/// speedup before GPU execution is allowed. This benchmark records reproducible
/// CPU-only baselines so later GPU hardware runs can compare against a fixed
/// host reference.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <queue>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kSeed = 42u;
constexpr double kPi     = 3.14159265358979323846;

using Coordinate = std::pair<double, double>;

std::vector<std::vector<std::uint32_t>> makeGraph(std::uint32_t node_count) {
    std::vector<std::vector<std::uint32_t>> graph(node_count);
    for (std::uint32_t node = 0; node < node_count; ++node) {
        auto& edges = graph[node];
        edges.push_back((node + 1) % node_count);
        edges.push_back((node + 7) % node_count);
        edges.push_back((node + 31) % node_count);
        if (node >= 3) {
            edges.push_back(node - 3);
        }
    }
    return graph;
}

std::vector<Coordinate> makeCoordinates(int count) {
    std::mt19937 rng(kSeed);
    std::uniform_real_distribution<double> lat_dist(-80.0, 80.0);
    std::uniform_real_distribution<double> lon_dist(-170.0, 170.0);

    std::vector<Coordinate> points;
    points.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        points.emplace_back(lat_dist(rng), lon_dist(rng));
    }
    return points;
}

double toRadians(double degrees) {
    return degrees * (kPi / 180.0);
}

double haversineKilometers(const Coordinate& lhs, const Coordinate& rhs) {
    constexpr double kEarthRadiusKm = 6371.0;

    const double lat1 = toRadians(lhs.first);
    const double lat2 = toRadians(rhs.first);
    const double dlat = lat2 - lat1;
    const double dlon = toRadians(rhs.second - lhs.second);

    const double sin_lat = std::sin(dlat * 0.5);
    const double sin_lon = std::sin(dlon * 0.5);
    const double a = sin_lat * sin_lat +
                     std::cos(lat1) * std::cos(lat2) * sin_lon * sin_lon;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusKm * c;
}

static void BGPU_CategoryB_BFS_CPUBaseline(benchmark::State& state) {
    const auto node_count = static_cast<std::uint32_t>(state.range(0));
    const auto graph      = makeGraph(node_count);
    std::vector<std::uint8_t> visited(node_count, 0);
    std::queue<std::uint32_t> frontier;

    for (auto _ : state) {
        std::fill(visited.begin(), visited.end(), 0);
        while (!frontier.empty()) {
            frontier.pop();
        }

        frontier.push(0);
        visited[0] = 1;
        std::uint32_t reached = 0;

        while (!frontier.empty()) {
            const auto node = frontier.front();
            frontier.pop();
            ++reached;

            for (const auto neighbor : graph[node]) {
                if (visited[neighbor] == 0) {
                    visited[neighbor] = 1;
                    frontier.push(neighbor);
                }
            }
        }

        benchmark::DoNotOptimize(reached);
    }

    state.SetItemsProcessed(state.iterations() *
                            static_cast<std::int64_t>(node_count));
    state.counters["gate_speedup_x100"] = 130;
    state.counters["avg_out_degree"]    = 4;
    state.SetLabel(
        "Phase D CPU baseline | Category B bfs | GPU review pending 2027");
}
BENCHMARK(BGPU_CategoryB_BFS_CPUBaseline)
    ->Arg(1'024)
    ->Arg(4'096)
    ->Arg(16'384)
    ->Arg(65'536)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

static void BGPU_CategoryB_GeoDistance_CPUBaseline(benchmark::State& state) {
    const int point_count = static_cast<int>(state.range(0));
    const auto lhs_points = makeCoordinates(point_count);
    const auto rhs_points = makeCoordinates(point_count + 17);
    std::vector<double> distances(static_cast<std::size_t>(point_count), 0.0);

    for (auto _ : state) {
        for (int i = 0; i < point_count; ++i) {
            distances[static_cast<std::size_t>(i)] = haversineKilometers(
                lhs_points[static_cast<std::size_t>(i)],
                rhs_points[static_cast<std::size_t>(i)]);
        }
        benchmark::DoNotOptimize(distances);
    }

    state.SetItemsProcessed(state.iterations() * point_count);
    state.counters["gate_speedup_x100"] = 130;
    state.counters["pairwise_points"]   = point_count;
    state.SetLabel(
        "Phase D CPU baseline | Category B geo-distance | GPU review pending 2027");
}
BENCHMARK(BGPU_CategoryB_GeoDistance_CPUBaseline)
    ->Arg(1'024)
    ->Arg(4'096)
    ->Arg(16'384)
    ->Arg(65'536)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

} // namespace
