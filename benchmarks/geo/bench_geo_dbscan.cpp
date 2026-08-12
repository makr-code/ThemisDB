#include <benchmark/benchmark.h>

#include "geo/geo_clustering.h"

#include <random>
#include <vector>

using themis::GeometryInfo;
using themis::GeometryType;
using themis::geo::DbscanConfig;
using themis::geo::GpuClusteringConfig;
using themis::geo::dbscanCluster;

namespace {

GeometryInfo makePoint(double lon, double lat) {
    GeometryInfo point(GeometryType::Point);
    point.coords.emplace_back(lon, lat);
    return point;
}

std::vector<GeometryInfo> makeClusteredPoints(std::size_t count) {
    std::vector<GeometryInfo> points;
    points.reserve(count);

    std::mt19937 rng(1337);
    std::normal_distribution<double> c1_lon(13.404954, 0.01); // Berlin
    std::normal_distribution<double> c1_lat(52.520008, 0.01);
    std::normal_distribution<double> c2_lon(-0.127758, 0.01); // London
    std::normal_distribution<double> c2_lat(51.507351, 0.01);

    for (std::size_t i = 0; i < count; ++i) {
        if ((i & 1U) == 0U) {
            points.emplace_back(makePoint(c1_lon(rng), c1_lat(rng)));
        } else {
            points.emplace_back(makePoint(c2_lon(rng), c2_lat(rng)));
        }
    }
    return points;
}

} // namespace

static void BM_GeoDBSCAN_CPU(benchmark::State& state) {
    const auto point_count = static_cast<std::size_t>(state.range(0));
    const auto points = makeClusteredPoints(point_count);

    DbscanConfig cfg;
    cfg.epsilon_m = 1500.0;
    cfg.min_points = 8;

    GpuClusteringConfig gpu_cfg;
    gpu_cfg.use_gpu = false;

    for (auto _ : state) {
        auto result = dbscanCluster(points, cfg, gpu_cfg);
        benchmark::DoNotOptimize(result.num_clusters);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(point_count));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()) * static_cast<double>(point_count),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoDBSCAN_CPU)
    ->Arg(1000)
    ->Arg(5000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

static void BM_GeoDBSCAN_GPU(benchmark::State& state) {
    const auto point_count = static_cast<std::size_t>(state.range(0));
    const auto points = makeClusteredPoints(point_count);

    DbscanConfig cfg;
    cfg.epsilon_m = 1500.0;
    cfg.min_points = 8;

    GpuClusteringConfig gpu_cfg;
    gpu_cfg.use_gpu = true;
    gpu_cfg.gpu_dbscan_max_n = 100000;

    for (auto _ : state) {
        auto result = dbscanCluster(points, cfg, gpu_cfg);
        benchmark::DoNotOptimize(result.num_clusters);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(point_count));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()) * static_cast<double>(point_count),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoDBSCAN_GPU)
    ->Arg(1000)
    ->Arg(5000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

