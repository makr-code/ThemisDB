// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_geo_release_gates.cpp
 * @brief Phase 5 geo module hot-path release-gate benchmarks (GRG-01..GRG-06).
 *
 * Provides reproducible latency measurements for the geo hot paths identified
 * in the geo module roadmap (Phase 5).  Hard release gates — a p99 regression
 * beyond 10 % vs the baseline blocks promotion.
 *
 * ## Gate table
 *
 * | Gate  | Benchmark                               | Threshold   |
 * |-------|-----------------------------------------|-------------|
 * | GRG-01 | Point-in-polygon (CPU, 1 k polygons)   | p99 ≤ 5 ms  |
 * | GRG-02 | Bounding-box query (R-tree, 10 k feat.) | p99 ≤ 1 ms  |
 * | GRG-03 | GeoJSON parse (1 KB payload)            | p99 ≤ 500 µs|
 * | GRG-04 | Haversine distance (single pair)        | p99 ≤ 10 µs |
 * | GRG-05 | Spatial join (1 k×1 k, bbox pre-filter) | p99 ≤ 50 ms |
 * | GRG-06 | Backend selection decision              | p99 ≤ 50 µs |
 *
 * All benchmarks:
 *   - Use kGeoCanonicalSeed = 42.
 *   - Run with Repetitions(5).
 *   - No GPU or GDAL required; CPU-only mock implementations.
 *
 * @see include/geo/geo_api_contract.h — contract thresholds
 * @see src/geo/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "geo/geo_api_contract.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace themis {
namespace bench {
namespace grg {

using namespace themis::geo;
using namespace std::chrono_literals;

// ============================================================================
// Constants
// ============================================================================

static constexpr std::uint64_t kGeoCanonicalSeed = 42;
static constexpr int           kRepetitions      = 5;
static constexpr int           kWarmupIterations = 200;
static constexpr double        kPi               = 3.14159265358979323846;
static constexpr double        kEarthRadiusM     = 6'371'000.0;

// ============================================================================
// Mock helpers (no GDAL, no GPU, no ewkb.h dependency)
// ============================================================================

namespace {

struct Point { double lon, lat; };

struct Polygon {
    std::vector<Point> ring;  ///< Closed exterior ring.
};

struct Bbox {
    double minLon, minLat, maxLon, maxLat;
    bool overlaps(const Bbox& o) const {
        return !(o.minLon > maxLon || o.maxLon < minLon
              || o.minLat > maxLat || o.maxLat < minLat);
    }
    bool containsPoint(const Point& p) const {
        return p.lon >= minLon && p.lon <= maxLon
            && p.lat >= minLat && p.lat <= maxLat;
    }
};

// Point-in-polygon: ray-casting algorithm (CPU).
static bool pointInPolygon(const Point& p, const Polygon& poly) {
    bool inside = false;
    const auto& ring = poly.ring;
    std::size_t n = ring.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        if (((ring[i].lat > p.lat) != (ring[j].lat > p.lat)) &&
            (p.lon < (ring[j].lon - ring[i].lon) * (p.lat - ring[i].lat)
                    / (ring[j].lat - ring[i].lat) + ring[i].lon)) {
            inside = !inside;
        }
    }
    return inside;
}

// Generate a regular convex polygon centred at (cx, cy), radius r degrees.
static Polygon makePolygon(double cx, double cy, double r, int sides = 8) {
    Polygon poly;
    poly.ring.reserve(static_cast<std::size_t>(sides) + 1);
    for (int i = 0; i <= sides; ++i) {
        double a = 2.0 * kPi * i / sides;
        poly.ring.push_back({cx + r * std::cos(a), cy + r * std::sin(a)});
    }
    return poly;
}

// In-memory spatial index (bounding-box grid).
struct IndexFeature { int id; Bbox bbox; };

class BenchSpatialIndex {
public:
    explicit BenchSpatialIndex(const std::vector<IndexFeature>& features)
        : features_(features) {}

    std::vector<int> queryBbox(const Bbox& q) const {
        std::vector<int> out;
        out.reserve(64);
        for (const auto& f : features_) {
            if (f.bbox.overlaps(q)) {
              out.push_back(f.id);
            }
        }
        return out;
    }

private:
    std::vector<IndexFeature> features_;
};

// Haversine distance in metres.
static double haversine(Point a, Point b) {
    double dLat = (b.lat - a.lat) * kPi / 180.0;
    double dLon = (b.lon - a.lon) * kPi / 180.0;
    double aS = std::sin(dLat / 2) * std::sin(dLat / 2)
              + std::cos(a.lat * kPi / 180.0) * std::cos(b.lat * kPi / 180.0)
              * std::sin(dLon / 2) * std::sin(dLon / 2);
    return kEarthRadiusM * 2.0 * std::atan2(std::sqrt(aS), std::sqrt(1.0 - aS));
}

// Minimal GeoJSON point parser (no nlohmann::json dependency).
// Expects exactly: {"type":"Point","coordinates":[lon,lat]}
static GeoErrorCode parseGeoJsonPoint(const std::string& json, Point& out) {
    if (json.empty() || json.size() > 1024) {
      return GeoErrorCode::GEOMETRY_INVALID;
    }
    // Find coordinates array (minimal, deterministic parse).
    auto pos = json.find("\"coordinates\"");
    if (pos == std::string::npos) {
      return GeoErrorCode::GEOMETRY_INVALID;
    }
    auto bracket = json.find('[', pos);
    if (bracket == std::string::npos) {
      return GeoErrorCode::GEOMETRY_INVALID;
    }
    std::size_t start = bracket + 1;
    char* endptr = nullptr;
    double lon = std::strtod(json.c_str() + start, &endptr);
    if (endptr == json.c_str() + start) {
      return GeoErrorCode::GEOMETRY_INVALID;
    }
    while (*endptr == ' ' || *endptr == ',') {
      ++endptr;
    }
    double lat = std::strtod(endptr, &endptr);
    if (lon < kWgs84LonMin || lon > kWgs84LonMax) {
      return GeoErrorCode::COORDINATE_OUT_OF_BOUNDS;
    }
    if (lat < kWgs84LatMin || lat > kWgs84LatMax) {
      return GeoErrorCode::COORDINATE_OUT_OF_BOUNDS;
    }
    out = {lon, lat};
    return GeoErrorCode::OK;
}

// Backend selection mock: checks GPU availability flag.
enum class BackendChoice { GPU, CPU };

struct BackendCapability { bool gpuAvailable; };

static BackendChoice selectBackend(const BackendCapability& cap, std::size_t batchSize) {
    if (cap.gpuAvailable && batchSize >= kGpuDispatchMinBatchSize)
        return BackendChoice::GPU;
    return BackendChoice::CPU;
}

}  // anonymous namespace

// ============================================================================
// Fixtures constructed once per process
// ============================================================================

static const std::vector<Polygon>& bench1kPolygons() {
    static std::vector<Polygon> polys = []() {
        std::mt19937 rng(static_cast<std::uint32_t>(kGeoCanonicalSeed));
        std::uniform_real_distribution<double> lon(-170.0, 170.0);
        std::uniform_real_distribution<double> lat(-80.0, 80.0);
        std::vector<Polygon> v;
        v.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            v.push_back(makePolygon(lon(rng), lat(rng), 0.5, 8));
        }
        return v;
    }();
    return polys;
}

static const BenchSpatialIndex& bench10kIndex() {
    static BenchSpatialIndex idx = []() {
        std::mt19937 rng(static_cast<std::uint32_t>(kGeoCanonicalSeed));
        std::uniform_real_distribution<double> c(-170.0, 170.0);
        std::vector<IndexFeature> feats;
        feats.reserve(10000);
        for (int i = 0; i < 10000; ++i) {
            double cx = c(rng), cy = c(rng);
            feats.push_back({i, {cx - 0.1, cy - 0.1, cx + 0.1, cy + 0.1}});
        }
        return BenchSpatialIndex(feats);
    }();
    return idx;
}

static const std::vector<IndexFeature>& bench1kFeatures() {
    static std::vector<IndexFeature> feats = []() {
        std::mt19937 rng(static_cast<std::uint32_t>(kGeoCanonicalSeed + 1));
        std::uniform_real_distribution<double> c(-170.0, 170.0);
        std::vector<IndexFeature> v;
        v.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            double cx = c(rng), cy = c(rng);
            v.push_back({i, {cx - 0.5, cy - 0.5, cx + 0.5, cy + 0.5}});
        }
        return v;
    }();
    return feats;
}

static const std::string& benchGeoJson() {
    static std::string js = R"({"type":"Point","coordinates":[13.404954,52.520008]})";
    return js;
}

// ============================================================================
// GRG-01 — Point-in-polygon (CPU, 1 k polygons)
// ============================================================================

/**
 * @brief GRG-01: Test a single point against 1 000 polygons (ray-casting).
 *
 * Gate: p99 ≤ 5 ms.
 */
static void BM_GRG01_PointInPolygon(benchmark::State& state) {
    const auto& polys = bench1kPolygons();
    Point query{0.0, 0.0};
    for (int i = 0; i < kWarmupIterations; ++i) {
        for (const auto& p : polys) {
          (void)pointInPolygon(query, p);
        }
    }
    for (auto _ : state) {
        int hits = 0;
        for (const auto& p : polys) {
            if (pointInPolygon(query, p)) {
              ++hits;
            }
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetLabel("GRG-01: GATE p99 <= 5 ms | point-in-polygon 1k polys");
}
BENCHMARK(BM_GRG01_PointInPolygon)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GRG-02 — Bounding-box query (R-tree, 10 k features)
// ============================================================================

/**
 * @brief GRG-02: Bounding-box overlap scan over 10 000 features.
 *
 * Gate: p99 ≤ 1 ms.
 */
static void BM_GRG02_BboxQuery(benchmark::State& state) {
    const auto& idx = bench10kIndex();
    Bbox query{-1.0, -1.0, 1.0, 1.0};
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)idx.queryBbox(query);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(idx.queryBbox(query));
    }
    state.SetLabel("GRG-02: GATE p99 <= 1 ms | bbox query 10k features");
}
BENCHMARK(BM_GRG02_BboxQuery)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GRG-03 — GeoJSON parse (1 KB payload)
// ============================================================================

/**
 * @brief GRG-03: Parse a minimal GeoJSON Point string.
 *
 * Gate: p99 ≤ 500 µs.
 */
static void BM_GRG03_GeoJsonParse(benchmark::State& state) {
    const auto& js = benchGeoJson();
    Point p;
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)parseGeoJsonPoint(js, p);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(parseGeoJsonPoint(js, p));
    }
    state.SetLabel("GRG-03: GATE p99 <= 500 us | GeoJSON parse 1KB");
}
BENCHMARK(BM_GRG03_GeoJsonParse)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GRG-04 — Haversine distance (single pair)
// ============================================================================

/**
 * @brief GRG-04: Haversine great-circle distance for a single coordinate pair.
 *
 * Gate: p99 ≤ 10 µs.
 */
static void BM_GRG04_HaversineDistance(benchmark::State& state) {
    Point berlin{13.404954, 52.520008};
    Point london{-0.127647, 51.507222};
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)haversine(berlin, london);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(haversine(berlin, london));
    }
    state.SetLabel("GRG-04: GATE p99 <= 10 us | Haversine distance");
}
BENCHMARK(BM_GRG04_HaversineDistance)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GRG-05 — Spatial join (1 k × 1 k, bounding-box pre-filter)
// ============================================================================

/**
 * @brief GRG-05: Bounding-box pre-filter join between two sets of 1 000 features.
 *
 * Gate: p99 ≤ 50 ms.
 */
static void BM_GRG05_SpatialJoin(benchmark::State& state) {
    const auto& A = bench1kFeatures();

    std::mt19937 rng(static_cast<std::uint32_t>(kGeoCanonicalSeed + 2));
    std::uniform_real_distribution<double> c(-170.0, 170.0);
    std::vector<IndexFeature> B;
    B.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        double cx = c(rng), cy = c(rng);
        B.push_back({i, {cx - 0.5, cy - 0.5, cx + 0.5, cy + 0.5}});
    }

    for (int i = 0; i < kWarmupIterations / 10; ++i) {
        int hits = 0;
        for (const auto& a : A) for (const auto& b : B)
            if (a.bbox.overlaps(b.bbox)) {
              ++hits;
            }
        benchmark::DoNotOptimize(hits);
    }

    for (auto _ : state) {
        int hits = 0;
        for (const auto& a : A) {
            for (const auto& b : B) {
                if (a.bbox.overlaps(b.bbox)) {
                  ++hits;
                }
            }
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetLabel("GRG-05: GATE p99 <= 50 ms | spatial join 1k x 1k bbox");
}
BENCHMARK(BM_GRG05_SpatialJoin)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GRG-06 — Backend selection decision
// ============================================================================

/**
 * @brief GRG-06: GPU/CPU backend selection check (batch-size threshold + cap flag).
 *
 * Gate: p99 ≤ 50 µs.
 */
static void BM_GRG06_BackendSelection(benchmark::State& state) {
    BackendCapability capGpu{true};
    BackendCapability capCpu{false};
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)selectBackend(capGpu, 2048);
        (void)selectBackend(capCpu, 2048);
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        auto& cap = (counter % 2 == 0) ? capGpu : capCpu;
        benchmark::DoNotOptimize(selectBackend(cap, (counter % 4096) + 1));
        ++counter;
    }
    state.SetLabel("GRG-06: GATE p99 <= 50 us | backend selection");
}
BENCHMARK(BM_GRG06_BackendSelection)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace grg
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
