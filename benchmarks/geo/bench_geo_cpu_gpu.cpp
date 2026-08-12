/**
 * @file bench_geo_cpu_gpu.cpp
 * @brief Performance benchmarks: CPU vs GPU throughput for geo spatial operations.
 *
 * Measures and compares throughput of the CPU-exact, CPU-approximate, and GPU
 * (CUDA/HIP with automatic CPU fallback) spatial backends on the core geo
 * operations: batchIntersects, exactIntersects, and geodesicDistance.
 *
 * Production Readiness Checklist item:
 *   [x] Performance benchmarks (CPU vs GPU throughput) — geo/ROADMAP.md
 */

#include <benchmark/benchmark.h>

#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <random>
#include <vector>

using namespace themis::geo;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static const double kPi = 3.14159265358979323846;

static GeometryInfo makePoint(double lon, double lat) {
    GeometryInfo g(GeometryType::Point);
    g.coords.emplace_back(lon, lat);
    return g;
}

// Build a regular convex polygon centred at (cx, cy) with radius r (degrees).
static GeometryInfo makeRegularPolygon(double cx, double cy,
                                       double r, int sides = 8) {
    GeometryInfo g(GeometryType::Polygon);
    std::vector<Coordinate> ring;
    ring.reserve(static_cast<std::size_t>(sides) + 1);
    for (int i = 0; i <= sides; ++i) {
        double angle = 2.0 * kPi * i / sides;
        ring.emplace_back(cx + r * std::cos(angle),
                          cy + r * std::sin(angle));
    }
    g.rings.push_back(ring);
    return g;
}

// Build a SpatialBatchInputs with `n` point-in-polygon pairs.
// Half the points are inside the polygon; half are outside.
static SpatialBatchInputs makeBatchInputs(int n) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<> lon_dis(-170.0, 170.0);
    std::uniform_real_distribution<> lat_dis(-80.0, 80.0);

    // A 5-degree-radius polygon centred at origin
    GeometryInfo poly = makeRegularPolygon(0.0, 0.0, 5.0, 32);

    SpatialBatchInputs in;
    in.count = static_cast<std::size_t>(n);
    in.geoms_a.reserve(in.count);
    in.geoms_b.reserve(in.count);

    for (int i = 0; i < n; ++i) {
        if (i % 2 == 0) {
            // Inside: random point within 4-degree radius
            std::uniform_real_distribution<> r_dis(0.0, 3.0);
            double angle = 2.0 * kPi * (static_cast<double>(i) / n);
            double r = r_dis(gen);
            in.geoms_a.emplace_back(makePoint(r * std::cos(angle),
                                              r * std::sin(angle)));
        } else {
            // Outside: random point far from origin
            in.geoms_a.emplace_back(makePoint(lon_dis(gen), lat_dis(gen)));
        }
        in.geoms_b.push_back(poly);
    }
    return in;
}

// Flat lat/lon arrays for geodesicDistance benchmarks.
struct LatLonPairs {
    std::vector<double> lats1, lons1, lats2, lons2;
};

static LatLonPairs makeLatLonPairs(int n) {
    std::mt19937 gen(99);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);

    LatLonPairs p;
    p.lats1.resize(static_cast<std::size_t>(n));
    p.lons1.resize(static_cast<std::size_t>(n));
    p.lats2.resize(static_cast<std::size_t>(n));
    p.lons2.resize(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        p.lats1[static_cast<std::size_t>(i)] = lat_dis(gen);
        p.lons1[static_cast<std::size_t>(i)] = lon_dis(gen);
        p.lats2[static_cast<std::size_t>(i)] = lat_dis(gen);
        p.lons2[static_cast<std::size_t>(i)] = lon_dis(gen);
    }
    return p;
}

// ─── batchIntersects: CPU-exact backend ──────────────────────────────────────

static void BM_GeoCPUExact_BatchIntersects(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    SpatialBatchInputs in = makeBatchInputs(n);

    ISpatialComputeBackend* backend = getCpuExactBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("CPU-exact geo backend not available");
        return;
    }

    for (auto _ : state) {
        auto result = backend->batchIntersects(in);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["pairs_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoCPUExact_BatchIntersects)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ─── batchIntersects: CPU-approximate backend ─────────────────────────────────

static void BM_GeoCPUApprox_BatchIntersects(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    SpatialBatchInputs in = makeBatchInputs(n);

    ISpatialComputeBackend* backend = getCpuApproximateBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("CPU-approximate geo backend not available");
        return;
    }

    for (auto _ : state) {
        auto result = backend->batchIntersects(in);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["pairs_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoCPUApprox_BatchIntersects)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ─── batchIntersects: GPU backend (with automatic CPU fallback) ───────────────

static void BM_GeoGPU_BatchIntersects(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    SpatialBatchInputs in = makeBatchInputs(n);

    ISpatialComputeBackend* backend = getGpuSpatialBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("GPU geo backend not available");
        return;
    }

    for (auto _ : state) {
        auto result = backend->batchIntersects(in);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["pairs_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoGPU_BatchIntersects)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ─── exactIntersects: CPU-exact backend ──────────────────────────────────────

static void BM_GeoCPUExact_ExactIntersects(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    GeometryInfo poly = makeRegularPolygon(0.0, 0.0, 5.0, 32);

    std::mt19937 gen(77);
    std::uniform_real_distribution<> lon_dis(-10.0, 10.0);
    std::uniform_real_distribution<> lat_dis(-10.0, 10.0);

    std::vector<GeometryInfo> points;
    points.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        points.emplace_back(makePoint(lon_dis(gen), lat_dis(gen)));
    }

    ISpatialComputeBackend* backend = getCpuExactBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("CPU-exact geo backend not available");
        return;
    }

    for (auto _ : state) {
        int hits = 0;
        for (const auto& pt : points) {
            if (backend->exactIntersects(pt, poly)) ++hits;
        }
        benchmark::DoNotOptimize(hits);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoCPUExact_ExactIntersects)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ─── exactIntersects: GPU backend ────────────────────────────────────────────

static void BM_GeoGPU_ExactIntersects(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    GeometryInfo poly = makeRegularPolygon(0.0, 0.0, 5.0, 32);

    std::mt19937 gen(77);
    std::uniform_real_distribution<> lon_dis(-10.0, 10.0);
    std::uniform_real_distribution<> lat_dis(-10.0, 10.0);

    std::vector<GeometryInfo> points;
    points.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        points.emplace_back(makePoint(lon_dis(gen), lat_dis(gen)));
    }

    ISpatialComputeBackend* backend = getGpuSpatialBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("GPU geo backend not available");
        return;
    }

    for (auto _ : state) {
        int hits = 0;
        for (const auto& pt : points) {
            if (backend->exactIntersects(pt, poly)) ++hits;
        }
        benchmark::DoNotOptimize(hits);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoGPU_ExactIntersects)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ─── geodesicDistance: CPU-exact backend ─────────────────────────────────────

static void BM_GeoCPUExact_GeodesicDistance(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    LatLonPairs pairs = makeLatLonPairs(n);

    ISpatialComputeBackend* backend = getCpuExactBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("CPU-exact geo backend not available");
        return;
    }

    for (auto _ : state) {
        double sum = 0.0;
        for (int i = 0; i < n; ++i) {
            sum += backend->geodesicDistance(
                pairs.lats1[static_cast<std::size_t>(i)],
                pairs.lons1[static_cast<std::size_t>(i)],
                pairs.lats2[static_cast<std::size_t>(i)],
                pairs.lons2[static_cast<std::size_t>(i)]);
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoCPUExact_GeodesicDistance)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMillisecond);

// ─── geodesicDistance: GPU backend ───────────────────────────────────────────

static void BM_GeoGPU_GeodesicDistance(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    LatLonPairs pairs = makeLatLonPairs(n);

    ISpatialComputeBackend* backend = getGpuSpatialBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("GPU geo backend not available");
        return;
    }

    for (auto _ : state) {
        double sum = 0.0;
        for (int i = 0; i < n; ++i) {
            sum += backend->geodesicDistance(
                pairs.lats1[static_cast<std::size_t>(i)],
                pairs.lons1[static_cast<std::size_t>(i)],
                pairs.lats2[static_cast<std::size_t>(i)],
                pairs.lons2[static_cast<std::size_t>(i)]);
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoGPU_GeodesicDistance)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMillisecond);

// ─── stBuffer: CPU-exact backend ─────────────────────────────────────────────

static void BM_GeoCPUExact_StBuffer(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    std::mt19937 gen(11);
    std::uniform_real_distribution<> lon_dis(-170.0, 170.0);
    std::uniform_real_distribution<> lat_dis(-80.0, 80.0);

    std::vector<GeometryInfo> points;
    points.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        points.emplace_back(makePoint(lon_dis(gen), lat_dis(gen)));
    }

    ISpatialComputeBackend* backend = getCpuExactBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("CPU-exact geo backend not available");
        return;
    }

    for (auto _ : state) {
        for (const auto& pt : points) {
            auto buf = backend->stBuffer(pt, 1000.0, 36);
            benchmark::DoNotOptimize(buf);
        }
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoCPUExact_StBuffer)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

// ─── stBuffer: GPU backend ───────────────────────────────────────────────────

static void BM_GeoGPU_StBuffer(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    std::mt19937 gen(11);
    std::uniform_real_distribution<> lon_dis(-170.0, 170.0);
    std::uniform_real_distribution<> lat_dis(-80.0, 80.0);

    std::vector<GeometryInfo> points;
    points.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        points.emplace_back(makePoint(lon_dis(gen), lat_dis(gen)));
    }

    ISpatialComputeBackend* backend = getGpuSpatialBackend();
    if (!backend || !backend->isAvailable()) {
        state.SkipWithError("GPU geo backend not available");
        return;
    }

    for (auto _ : state) {
        for (const auto& pt : points) {
            auto buf = backend->stBuffer(pt, 1000.0, 36);
            benchmark::DoNotOptimize(buf);
        }
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * n),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoGPU_StBuffer)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

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
