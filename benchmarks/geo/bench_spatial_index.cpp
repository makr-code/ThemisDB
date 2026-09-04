/**
 * @file bench_spatial_index.cpp
 * @brief Benchmarks for spatial operations using the GeoRTree index.
 *
 * Compares the in-memory R-tree (GeoRTree) against a linear scan to
 * demonstrate sub-linear query performance for intersects and contains.
 */

#include <benchmark/benchmark.h>
#include "geo/geo_rtree.h"
#include "utils/geo/ewkb.h"
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>

using namespace themis::geo;

// ─── helpers ─────────────────────────────────────────────────────────────────

static GeometryInfo makePoint(double x, double y) {
    GeometryInfo g(GeometryType::Point);
    g.coords.emplace_back(x, y);
    return g;
}

static GeometryInfo makeBox(double minx, double miny, double maxx, double maxy) {
    GeometryInfo g(GeometryType::Polygon);
    g.rings.push_back({
        {minx, miny}, {maxx, miny}, {maxx, maxy}, {minx, maxy}, {minx, miny}
    });
    return g;
}

// ─── R-tree: bulkLoad then intersects ────────────────────────────────────────

static void BM_RTree_BulkLoad(benchmark::State& state) {
    const int n = state.range(0);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);

    std::vector<std::pair<std::string, GeometryInfo>> entries;
    entries.reserve(n);
    for (int i = 0; i < n; ++i) {
        entries.emplace_back("p" + std::to_string(i),
                             makePoint(lon_dis(gen), lat_dis(gen)));
    }

    for (auto _ : state) {
        GeoRTree tree;
        tree.bulkLoad(entries);
        benchmark::DoNotOptimize(tree.size());
    }
    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_RTree_BulkLoad)
    ->RangeMultiplier(10)
    ->Range(1000, 100000)
    ->Arg(1'000'000)
    ->Complexity();

// ─── R-tree: intersects query ─────────────────────────────────────────────────

static void BM_RTree_Intersects(benchmark::State& state) {
    const int n = state.range(0);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);

    std::vector<std::pair<std::string, GeometryInfo>> entries;
    entries.reserve(n);
    for (int i = 0; i < n; ++i) {
        entries.emplace_back("p" + std::to_string(i),
                             makePoint(lon_dis(gen), lat_dis(gen)));
    }

    GeoRTree tree;
    tree.bulkLoad(entries);

    // Small query bbox (~10° × 10°)
    MBR query(-5.0, -5.0, 5.0, 5.0);

    for (auto _ : state) {
        auto result = tree.intersects(query);
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(n);
}
BENCHMARK(BM_RTree_Intersects)
    ->RangeMultiplier(10)
    ->Range(1000, 100000)
    ->Arg(1'000'000)
    ->Complexity();

// ─── Linear scan: intersects (baseline for comparison) ───────────────────────

static void BM_LinearScan_Intersects(benchmark::State& state) {
    const int n = state.range(0);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);

    struct Entry { MBR mbr; };
    std::vector<Entry> entries;
    entries.reserve(n);
    for (int i = 0; i < n; ++i) {
        double x = lon_dis(gen);
        double y = lat_dis(gen);
        entries.push_back({MBR(x, y, x, y)});
    }

    MBR query(-5.0, -5.0, 5.0, 5.0);

    for (auto _ : state) {
        int count = 0;
        for (const auto& e : entries) {
            if (e.mbr.intersects(query)) {
              ++count;
            }
        }
        benchmark::DoNotOptimize(count);
    }
    state.SetComplexityN(n);
}
BENCHMARK(BM_LinearScan_Intersects)
    ->RangeMultiplier(10)
    ->Range(1000, 100000)
    ->Complexity();

// ─── R-tree: contains (point-in-MBR) query ───────────────────────────────────

static void BM_RTree_Contains(benchmark::State& state) {
    const int n = state.range(0);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);

    std::vector<std::pair<std::string, GeometryInfo>> entries;
    entries.reserve(n);
    for (int i = 0; i < n; ++i) {
        double x = lon_dis(gen);
        double y = lat_dis(gen);
        entries.emplace_back("b" + std::to_string(i),
                             makeBox(x - 1.0, y - 1.0, x + 1.0, y + 1.0));
    }

    GeoRTree tree;
    tree.bulkLoad(entries);

    for (auto _ : state) {
        auto result = tree.contains(0.0, 0.0);
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(n);
}
BENCHMARK(BM_RTree_Contains)
    ->RangeMultiplier(10)
    ->Range(1000, 100000)
    ->Complexity();

// ─── R-tree: incremental inserts ─────────────────────────────────────────────

static void BM_RTree_IncrementalInsert(benchmark::State& state) {
    const int n = state.range(0);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);

    std::vector<std::pair<std::string, GeometryInfo>> entries;
    entries.reserve(n);
    for (int i = 0; i < n; ++i) {
        entries.emplace_back("p" + std::to_string(i),
                             makePoint(lon_dis(gen), lat_dis(gen)));
    }

    for (auto _ : state) {
        GeoRTree tree;
        for (const auto& [key, geom] : entries) {
            tree.insert(key, geom);
        }
        benchmark::DoNotOptimize(tree.size());
    }
    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_RTree_IncrementalInsert)
    ->RangeMultiplier(10)
    ->Range(1000, 10000)
    ->Complexity();

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
