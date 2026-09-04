/// @file bench_acceleration_dispatch.cpp
/// @brief Performance benchmarks for the CPU acceleration dispatch layer.
///
/// Uses the kernel dispatch table populated by CPUVectorBackend and
/// CPUGeoBackend, matching the actual production execution path.
///
/// Covers:
///   - ANNKernelDispatch::launchL2Distance   (CPU ANN, L2 metric)
///   - ANNKernelDispatch::launchCosine       (CPU ANN, cosine metric)
///   - ANNKernelDispatch::launchTopK         (top-k selection)
///   - GeoKernelDispatch::launchDistance     (haversine batch)
///
/// Performance targets (src/acceleration/ROADMAP.md):
///   - launchL2Distance 128-dim, 1 query vs 1000 vecs: < 500 µs
///   - launchTopK  10 queries, 1000 vecs, k=10:         < 200 µs
///   - launchDistance 1000 pairs:                        < 100 µs

#include <benchmark/benchmark.h>
#include "acceleration/cpu_backend.h"
#include "acceleration/kernel_invocation.h"
#include <random>
#include <vector>
#include <cstdint>

using namespace themis::acceleration;

// ============================================================================
// Helpers
// ============================================================================

namespace {

std::vector<float> randomFloatVectors(size_t count, size_t dim,
                                       unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> data(count * dim);
    for (auto& v : data) {
      v = dist(rng);
    }
    return data;
}

std::vector<double> randomDoubleVectors(size_t count, unsigned seed = 7) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> lat_dist(-90.0, 90.0);
    std::uniform_real_distribution<double> lon_dist(-180.0, 180.0);
    std::vector<double> data;
    data.reserve(count * 2);
    for (size_t i = 0; i < count; ++i) {
        data.push_back(lat_dist(rng));
        data.push_back(lon_dist(rng));
    }
    return data;
}

} // anonymous namespace

// ============================================================================
// ANN dispatch fixture
// ============================================================================

class AnnDispatchBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        if (!backend_.initialize()) {
            auto& mutable_state = const_cast<benchmark::State&>(state);
            mutable_state.SkipWithError("CPUVectorBackend::initialize() failed");
            return;
        }
        disp_ = backend_.populateANNDispatch();
    }

    CPUVectorBackend backend_;
    ANNKernelDispatch disp_;
};

// ============================================================================
// launchL2Distance – throughput vs. vector-set size
// ============================================================================

BENCHMARK_DEFINE_F(AnnDispatchBenchFixture, L2Distance)(benchmark::State& state) {
    const size_t dim      = static_cast<size_t>(state.range(0));
    const size_t num_vecs = static_cast<size_t>(state.range(1));
    const int    nQry     = 1;

    auto queries = randomFloatVectors(nQry,     dim);
    auto vectors = randomFloatVectors(num_vecs, dim);
    std::vector<float> out(nQry * num_vecs, 0.0f);

    for (auto _ : state) {
        int rc = disp_.launchL2Distance(
            queries.data(), vectors.data(), out.data(),
            nQry, static_cast<int>(num_vecs), static_cast<int>(dim), nullptr);
        benchmark::DoNotOptimize(rc);
        benchmark::DoNotOptimize(out[0]);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_vecs));
    state.SetLabel("L2, dim=" + std::to_string(dim) +
                   " vecs=" + std::to_string(num_vecs));
}

BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, L2Distance)
    ->Args({32,   100})
    ->Args({128,  100})
    ->Args({128, 1000})
    ->Args({512,  100})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// launchCosine – cosine distance dispatch
// ============================================================================

BENCHMARK_DEFINE_F(AnnDispatchBenchFixture, CosineDistance)(benchmark::State& state) {
    const size_t dim      = 128;
    const size_t num_vecs = static_cast<size_t>(state.range(0));
    const int    nQry     = 1;

    auto queries = randomFloatVectors(nQry,     dim);
    auto vectors = randomFloatVectors(num_vecs, dim);
    std::vector<float> out(nQry * num_vecs, 0.0f);

    for (auto _ : state) {
        int rc = disp_.launchCosine(
            queries.data(), vectors.data(), out.data(),
            nQry, static_cast<int>(num_vecs), static_cast<int>(dim), nullptr);
        benchmark::DoNotOptimize(rc);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_vecs));
}

BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, CosineDistance)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// launchTopK – top-k selection from precomputed distance matrix
// ============================================================================

BENCHMARK_DEFINE_F(AnnDispatchBenchFixture, TopK)(benchmark::State& state) {
    const int nQry     = static_cast<int>(state.range(0));
    const int num_vecs = static_cast<int>(state.range(1));
    const int k        = 10;

    // Pre-compute distances
    auto queries = randomFloatVectors(nQry,     128);
    auto vectors = randomFloatVectors(num_vecs, 128);
    std::vector<float> distances(nQry * num_vecs, 0.0f);
    disp_.launchL2Distance(queries.data(), vectors.data(), distances.data(),
                           nQry, num_vecs, 128, nullptr);

    std::vector<uint32_t> topk_idx(nQry * k);
    std::vector<float>    topk_dists(nQry * k);

    for (auto _ : state) {
        int rc = disp_.launchTopK(
            distances.data(), topk_idx.data(), topk_dists.data(),
            nQry, num_vecs, k, nullptr);
        benchmark::DoNotOptimize(rc);
        benchmark::DoNotOptimize(topk_idx[0]);
    }

    state.SetItemsProcessed(state.iterations() * nQry);
    state.SetLabel("k=10, queries=" + std::to_string(nQry) +
                   " vecs=" + std::to_string(num_vecs));
}

BENCHMARK_REGISTER_F(AnnDispatchBenchFixture, TopK)
    ->Args({1,   100})
    ->Args({10,  100})
    ->Args({1,  1000})
    ->Args({10, 1000})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Geo dispatch – launchDistance (haversine batch)
// ============================================================================

class GeoDispatchBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        if (!backend_.initialize()) {
            auto& mutable_state = const_cast<benchmark::State&>(state);
            mutable_state.SkipWithError("CPUGeoBackend::initialize() failed");
            return;
        }
        disp_ = backend_.populateGeoDispatch();
    }

    CPUGeoBackend     backend_;
    GeoKernelDispatch disp_;
};

BENCHMARK_DEFINE_F(GeoDispatchBenchFixture, HaversineBatch)(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));

    auto coords1 = randomDoubleVectors(n, 3);
    auto coords2 = randomDoubleVectors(n, 5);

    std::vector<double> lats1(n), lons1(n), lats2(n), lons2(n);
    for (size_t i = 0; i < n; ++i) {
        lats1[i] = coords1[2 * i];
        lons1[i] = coords1[2 * i + 1];
        lats2[i] = coords2[2 * i];
        lons2[i] = coords2[2 * i + 1];
    }

    std::vector<float> out(n, 0.0f);

    for (auto _ : state) {
        int rc = disp_.launchDistance(
            lats1.data(), lons1.data(),
            lats2.data(), lons2.data(),
            out.data(),
            static_cast<int>(n),
            GeoDistanceFormula::HAVERSINE,
            nullptr);
        benchmark::DoNotOptimize(rc);
        benchmark::DoNotOptimize(out[0]);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
    state.SetLabel("n=" + std::to_string(n));
}

BENCHMARK_REGISTER_F(GeoDispatchBenchFixture, HaversineBatch)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
