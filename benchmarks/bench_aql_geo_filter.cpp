/*
 * ThemisDB | File: bench_aql_geo_filter.cpp | Version: 0.0.1
 * Maturity: 🟡 BETA | Score: 84/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Candidate
 */

/**
 * @file bench_aql_geo_filter.cpp
 * @brief Gate-4 benchmark for FILTER ST_Within(...) on a 100K-point corpus.
 *
 * Measures p99 query latency for two paths:
 * - Geo index prefilter (R-tree intersects on query bbox)
 * - Sequential baseline (full scan point-in-bbox test)
 *
 * Gate target:
 * - GATE-GEO-01: p99 <= 50 ms on geo-index path for 100K points.
 */

#include <benchmark/benchmark.h>

#include "geo/geo_rtree.h"
#include "utils/geo/ewkb.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

using themis::geo::GeoRTree;
using themis::geo::GeometryInfo;
using themis::geo::GeometryType;
using themis::geo::MBR;

static constexpr uint64_t kCanonicalSeed = 42;
static constexpr int kDatasetPoints = 100'000;
static constexpr int kQueriesPerIteration = 256;
static constexpr double kGateP99Ms = 50.0;

GeometryInfo makePoint(double lon, double lat) {
    GeometryInfo g(GeometryType::Point);
    g.coords.emplace_back(lon, lat);
    return g;
}

double percentileMs(std::vector<double> samples, double percentile) {
    if (samples.empty()) {
        return 0.0;
    }
    std::sort(samples.begin(), samples.end());
    const double rank = percentile * static_cast<double>(samples.size() - 1);
    const auto idx = static_cast<std::size_t>(rank);
    return samples[idx];
}

struct GeoBenchmarkDataset {
    std::vector<std::pair<std::string, GeometryInfo>> entries;
    std::vector<MBR> linear_points;
    std::vector<MBR> query_bboxes;
    GeoRTree rtree;
};

const GeoBenchmarkDataset& getDataset() {
    static const GeoBenchmarkDataset dataset = [] {
        GeoBenchmarkDataset d;
        d.entries.reserve(kDatasetPoints);
        d.linear_points.reserve(kDatasetPoints);
        d.query_bboxes.reserve(kQueriesPerIteration);

        std::mt19937 rng(kCanonicalSeed);
        std::uniform_real_distribution<double> lon(-180.0, 180.0);
        std::uniform_real_distribution<double> lat(-90.0, 90.0);
        std::uniform_real_distribution<double> qlon(-160.0, 160.0);
        std::uniform_real_distribution<double> qlat(-70.0, 70.0);

        for (int i = 0; i < kDatasetPoints; ++i) {
            const double x = lon(rng);
            const double y = lat(rng);
            d.entries.emplace_back("p" + std::to_string(i), makePoint(x, y));
            d.linear_points.emplace_back(x, y, x, y);
        }

        d.rtree.bulkLoad(d.entries);

        // ~4x4 degree query windows to emulate selective ST_Within regions.
        for (int i = 0; i < kQueriesPerIteration; ++i) {
            const double cx = qlon(rng);
            const double cy = qlat(rng);
            d.query_bboxes.emplace_back(cx - 2.0, cy - 2.0, cx + 2.0, cy + 2.0);
        }

        return d;
    }();

    return dataset;
}

class AqlGeoFilterFixture : public benchmark::Fixture {};

BENCHMARK_F(AqlGeoFilterFixture, Gate4_STWithin_GeoIndex_100K)(benchmark::State& state) {
    const auto& dataset = getDataset();
    std::vector<double> latencies_ms;
    latencies_ms.reserve(kQueriesPerIteration);
    double p99_last_ms = 0.0;

    for (auto _ : state) {
        latencies_ms.clear();
        for (const auto& query_bbox : dataset.query_bboxes) {
            const auto t0 = std::chrono::steady_clock::now();
            const auto matches = dataset.rtree.intersects(query_bbox);
            const auto t1 = std::chrono::steady_clock::now();
            benchmark::DoNotOptimize(matches.size());
            const auto dt = std::chrono::duration<double, std::milli>(t1 - t0).count();
            latencies_ms.push_back(dt);
        }
        p99_last_ms = percentileMs(latencies_ms, 0.99);
    }

    state.SetItemsProcessed(state.iterations() * kQueriesPerIteration);
    state.counters["dataset_points"] = static_cast<double>(kDatasetPoints);
    state.counters["p99_ms"] = p99_last_ms;
    state.counters["gate_target_ms"] = kGateP99Ms;
    state.counters["gate_pass"] = (p99_last_ms <= kGateP99Ms) ? 1.0 : 0.0;
}

BENCHMARK_REGISTER_F(AqlGeoFilterFixture, Gate4_STWithin_GeoIndex_100K)
    ->Iterations(10)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Name("AQL/GATE4/STWithin_GeoIndex_100K");

BENCHMARK_F(AqlGeoFilterFixture, Gate4_STWithin_SequentialBaseline_100K)(benchmark::State& state) {
    const auto& dataset = getDataset();
    std::vector<double> latencies_ms;
    latencies_ms.reserve(kQueriesPerIteration);
    double p99_last_ms = 0.0;

    for (auto _ : state) {
        latencies_ms.clear();
        for (const auto& query_bbox : dataset.query_bboxes) {
            const auto t0 = std::chrono::steady_clock::now();
            std::size_t count = 0;
            for (const auto& point_mbr : dataset.linear_points) {
                if (query_bbox.intersects(point_mbr)) {
                    ++count;
                }
            }
            const auto t1 = std::chrono::steady_clock::now();
            benchmark::DoNotOptimize(count);
            const auto dt = std::chrono::duration<double, std::milli>(t1 - t0).count();
            latencies_ms.push_back(dt);
        }
        p99_last_ms = percentileMs(latencies_ms, 0.99);
    }

    state.SetItemsProcessed(state.iterations() * kQueriesPerIteration);
    state.counters["dataset_points"] = static_cast<double>(kDatasetPoints);
    state.counters["p99_ms"] = p99_last_ms;
}

BENCHMARK_REGISTER_F(AqlGeoFilterFixture, Gate4_STWithin_SequentialBaseline_100K)
    ->Iterations(5)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Name("AQL/GATE4/STWithin_SequentialBaseline_100K");

} // namespace

BENCHMARK_MAIN();
