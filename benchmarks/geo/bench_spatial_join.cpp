/**
 * @file bench_spatial_join.cpp
 * @brief Performance benchmarks for the spatial JOIN operation.
 *
 * Validates the performance acceptance criterion from the Spatial JOIN Support
 * issue (roadmap:170:geo:v1.6.0:spatial-join-support):
 *
 *   "Spatial JOIN of two 100 000-point collections with a 1 km threshold
 *    returns the first 1 000 results in ≤ 500 ms."
 *
 * Benchmarks:
 *   BM_SpatialJoin_First1000          – time to yield first 1 000 pairs via
 *                                       SpatialJoinIterator (lazy, one at a time).
 *   BM_SpatialJoin_AllPairs           – full batch spatialJoin() for scalability
 *                                       comparison.
 *   BM_SpatialJoin_IndexBuild         – GeoRTree bulkLoad on 100 k points alone.
 */

#include <benchmark/benchmark.h>
#include "geo/spatial_join.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <random>
#include <string>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Uniform WGS84 random point generator.
/// Points are generated within a tight bounding box around Berlin to maximise
/// the chance that many pairs fall within a 1 km radius, giving the benchmark
/// a realistic selectivity.
std::vector<std::pair<std::string, GeometryInfo>>
generatePoints(std::size_t n, double center_lon = 13.405, double center_lat = 52.52,
               double half_deg = 0.1 /* ≈ ±11 km */, uint64_t seed = 42)
{
    std::mt19937_64 gen(seed);
    std::uniform_real_distribution<double> lon_dist(center_lon - half_deg,
                                                    center_lon + half_deg);
    std::uniform_real_distribution<double> lat_dist(center_lat - half_deg,
                                                    center_lat + half_deg);

    std::vector<std::pair<std::string, GeometryInfo>> pts;
    pts.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        GeometryInfo g(GeometryType::Point);
        g.coords.emplace_back(lon_dist(gen), lat_dist(gen));
        pts.emplace_back("p" + std::to_string(i), std::move(g));
    }
    return pts;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// BM_SpatialJoin_First1000
//
// Performance AC:  two 100 000-point collections, 1 km threshold,
//                  first 1 000 results returned in ≤ 500 ms.
// ---------------------------------------------------------------------------

static void BM_SpatialJoin_First1000(benchmark::State& state)
{
    const std::size_t n         = static_cast<std::size_t>(state.range(0));
    const double      threshold = 1000.0; // 1 km
    const std::size_t want      = 1000;

    // Build collections once outside the timed loop to measure query time only.
    auto outer = generatePoints(n, 13.405, 52.52, 0.1, /*seed=*/1);
    auto inner = generatePoints(n, 13.405, 52.52, 0.1, /*seed=*/2);

    SpatialJoinConfig cfg;
    cfg.max_pairs = want; // stop after collecting the first 1 000 results

    for (auto _ : state) {
        SpatialJoinIterator it(outer, inner, threshold, cfg);
        std::size_t found = 0;
        while (it.advance()) {
            benchmark::DoNotOptimize(it.current());
            ++found;
        }
        benchmark::DoNotOptimize(found);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(want));
    state.SetLabel("threshold=1km first_n=1000");
}
// Primary target: 100 000 × 100 000 (n=100000 for each side)
BENCHMARK(BM_SpatialJoin_First1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// BM_SpatialJoin_AllPairs – batch spatialJoin() scalability
// ---------------------------------------------------------------------------

static void BM_SpatialJoin_AllPairs(benchmark::State& state)
{
    const std::size_t n         = static_cast<std::size_t>(state.range(0));
    const double      threshold = 1000.0; // 1 km
    const std::size_t max_pairs = 10000;  // cap to keep the benchmark fast

    auto outer = generatePoints(n, 13.405, 52.52, 0.05, /*seed=*/3);
    auto inner = generatePoints(n, 13.405, 52.52, 0.05, /*seed=*/4);

    SpatialJoinConfig cfg;
    cfg.max_pairs = max_pairs;

    for (auto _ : state) {
        auto result = spatialJoin(outer, inner, threshold, cfg);
        benchmark::DoNotOptimize(result.size());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n));
    state.SetLabel("threshold=1km max_pairs=10k");
}
BENCHMARK(BM_SpatialJoin_AllPairs)
    ->RangeMultiplier(10)
    ->Range(1000, 100000)
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

// ---------------------------------------------------------------------------
// BM_SpatialJoin_IndexBuild – R-tree bulkLoad cost for the inner collection
// ---------------------------------------------------------------------------

static void BM_SpatialJoin_IndexBuild(benchmark::State& state)
{
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    auto pts = generatePoints(n, 13.405, 52.52, 0.1, /*seed=*/5);

    for (auto _ : state) {
        // The SpatialJoinIterator constructor builds the R-tree index.
        // Use threshold=1m so essentially no pairs are found, isolating
        // the construction (R-tree bulkLoad) cost from iteration cost.
        SpatialJoinIterator it(pts, pts, 1.0);
        benchmark::DoNotOptimize(it.done());
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n));
    state.SetLabel("rtree_build_only");
}
BENCHMARK(BM_SpatialJoin_IndexBuild)
    ->RangeMultiplier(10)
    ->Range(1000, 100000)
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
