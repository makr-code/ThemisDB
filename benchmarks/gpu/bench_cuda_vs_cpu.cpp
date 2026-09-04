#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <cmath>
#include <cstdint>

#include "acceleration/cpu_backend.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/kernel_invocation.h"

using namespace themis::acceleration;

// ============================================================================
// Shared test-data helpers
// ============================================================================

namespace {

/// Fills a float vector with uniform random values in [-1, 1].
static std::vector<float> makeFloats(size_t n, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> v(n);
    for (auto& x : v) {
      x = dist(rng);
    }
    return v;
}

/// Fills a double vector with uniform random values in [lo, hi].
static std::vector<double> makeDoubles(size_t n, double lo, double hi,
                                       uint32_t seed = 1337) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(lo, hi);
    std::vector<double> v(n);
    for (auto& x : v) {
      x = dist(rng);
    }
    return v;
}

// ============================================================================
// Lazy-initialised CPU backend singletons (one per benchmark process)
// ============================================================================

CPUVectorBackend& cpuVec() {
    static CPUVectorBackend b;
    static bool init = [&]{ b.initialize(); return true; }();
    (void)init;
    return b;
}

CPUGeoBackend& cpuGeo() {
    static CPUGeoBackend b;
    static bool init = [&]{ b.initialize(); return true; }();
    (void)init;
    return b;
}

#ifdef THEMIS_ENABLE_CUDA
CUDAVectorBackend& cudaVec() {
    static CUDAVectorBackend b;
    static bool init = [&]{ b.initialize(); return true; }();
    (void)init;
    return b;
}

CUDAGeoBackend& cudaGeo() {
    static CUDAGeoBackend b;
    static bool init = [&]{ b.initialize(); return true; }();
    (void)init;
    return b;
}
#endif

} // anonymous namespace

// ============================================================================
// CPU — ANN distance benchmarks
// ============================================================================
// Args: {numVectors, dim}

static void BM_CPU_ANN_L2Distance(benchmark::State& state) {
    const int nVec  = static_cast<int>(state.range(0));
    const int dim   = static_cast<int>(state.range(1));
    const int nQry  = 1;

    auto queries   = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors   = makeFloats(static_cast<size_t>(nVec) * dim, 99);
    std::vector<float> out(static_cast<size_t>(nQry) * nVec);

    auto disp = cpuVec().populateANNDispatch();
    for (auto _ : state) {
        disp.launchL2Distance(queries.data(), vectors.data(), out.data(),
                              nQry, nVec, dim, nullptr);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 0;
}
BENCHMARK(BM_CPU_ANN_L2Distance)
    ->Args({1000,  64})->Args({1000, 128})
    ->Args({1000, 256})->Args({1000, 512})
    ->Unit(benchmark::kMicrosecond);

static void BM_CPU_ANN_CosineDistance(benchmark::State& state) {
    const int nVec  = static_cast<int>(state.range(0));
    const int dim   = static_cast<int>(state.range(1));
    const int nQry  = 1;

    auto queries   = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors   = makeFloats(static_cast<size_t>(nVec) * dim, 99);
    std::vector<float> out(static_cast<size_t>(nQry) * nVec);

    auto disp = cpuVec().populateANNDispatch();
    for (auto _ : state) {
        disp.launchCosine(queries.data(), vectors.data(), out.data(),
                          nQry, nVec, dim, nullptr);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 0;
}
BENCHMARK(BM_CPU_ANN_CosineDistance)
    ->Args({1000,  64})->Args({1000, 128})
    ->Args({1000, 256})->Args({1000, 512})
    ->Unit(benchmark::kMicrosecond);

static void BM_CPU_ANN_InnerProduct(benchmark::State& state) {
    const int nVec  = static_cast<int>(state.range(0));
    const int dim   = static_cast<int>(state.range(1));
    const int nQry  = 1;

    auto queries   = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors   = makeFloats(static_cast<size_t>(nVec) * dim, 99);
    std::vector<float> out(static_cast<size_t>(nQry) * nVec);

    auto disp = cpuVec().populateANNDispatch();
    for (auto _ : state) {
        disp.launchInnerProduct(queries.data(), vectors.data(), out.data(),
                                nQry, nVec, dim, nullptr);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 0;
}
BENCHMARK(BM_CPU_ANN_InnerProduct)
    ->Args({1000,  64})->Args({1000, 128})
    ->Args({1000, 256})->Args({1000, 512})
    ->Unit(benchmark::kMicrosecond);

static void BM_CPU_ANN_TopK(benchmark::State& state) {
    const int nVec  = static_cast<int>(state.range(0));
    const int topK  = static_cast<int>(state.range(1));
    const int nQry  = 1;

    // Pre-compute a distance matrix to isolate TopK from distance cost
    auto dists = makeFloats(static_cast<size_t>(nQry) * nVec, 7);
    std::vector<uint32_t> outIdx(static_cast<size_t>(nQry) * topK);
    std::vector<float>    outDst(static_cast<size_t>(nQry) * topK);

    auto disp = cpuVec().populateANNDispatch();
    for (auto _ : state) {
        disp.launchTopK(dists.data(), outIdx.data(), outDst.data(),
                        nQry, nVec, topK, nullptr);
        benchmark::DoNotOptimize(outIdx.data());
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 0;
    state.counters["topK"]    = topK;
}
BENCHMARK(BM_CPU_ANN_TopK)
    ->Args({1000, 10})->Args({1000,  50})
    ->Args({5000, 10})->Args({5000,  50})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// CPU — Batch KNN end-to-end  (distance + top-K)
// ============================================================================
// Args: {numVectors, dim, topK}

static void BM_CPU_BatchKNN(benchmark::State& state) {
    const int nVec = static_cast<int>(state.range(0));
    const int dim  = static_cast<int>(state.range(1));
    const int topK = static_cast<int>(state.range(2));
    const int nQry = 4;

    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);

    for (auto _ : state) {
        auto results = cpuVec().batchKnnSearch(
            queries.data(), nQry, dim,
            vectors.data(), nVec, topK, /*useL2=*/true);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 0;
    state.counters["topK"]    = topK;
}
BENCHMARK(BM_CPU_BatchKNN)
    ->Args({ 500, 128, 10})
    ->Args({1000, 128, 10})
    ->Args({1000, 256, 10})
    ->Args({1000, 512, 10})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// CPU — Geo benchmarks
// ============================================================================

static void BM_CPU_Geo_HaversineDistance(benchmark::State& state) {
    const int count = static_cast<int>(state.range(0));

    auto lats1 = makeDoubles(count, -90.0,  90.0, 1);
    auto lons1 = makeDoubles(count, -180.0, 180.0, 2);
    auto lats2 = makeDoubles(count, -90.0,  90.0, 3);
    auto lons2 = makeDoubles(count, -180.0, 180.0, 4);
    std::vector<float> out(count);

    auto disp = cpuGeo().populateGeoDispatch();
    for (auto _ : state) {
        disp.launchDistance(lats1.data(), lons1.data(),
                            lats2.data(), lons2.data(),
                            out.data(), count,
                            GeoDistanceFormula::HAVERSINE, nullptr);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
    state.counters["backend"] = 0;
}
BENCHMARK(BM_CPU_Geo_HaversineDistance)
    ->Arg(1000)->Arg(10000)->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

static void BM_CPU_Geo_PointInPolygon(benchmark::State& state) {
    const int numPoints = static_cast<int>(state.range(0));

    auto ptLats = makeDoubles(numPoints, -90.0, 90.0, 5);
    auto ptLons = makeDoubles(numPoints, -180.0, 180.0, 6);

    // Simple rectangular polygon: Berlin bounding box
    const std::vector<double> polygon = {
        52.34, 13.09,  52.34, 13.76,
        52.68, 13.76,  52.68, 13.09
    };
    const int numVerts = static_cast<int>(polygon.size()) / 2;

    std::vector<uint8_t> out(numPoints);
    auto disp = cpuGeo().populateGeoDispatch();

    for (auto _ : state) {
        disp.launchContainment(ptLats.data(), ptLons.data(), numPoints,
                               polygon.data(), numVerts,
                               out.data(), nullptr);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * numPoints);
    state.counters["backend"] = 0;
}
BENCHMARK(BM_CPU_Geo_PointInPolygon)
    ->Arg(1000)->Arg(10000)->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// CUDA — ANN benchmarks
// Compiled only when THEMIS_ENABLE_CUDA is defined; skipped at runtime when
// no CUDA device is available (state.SkipWithMessage).
//
// Coverage note
// -------------
// The CUDA high-level API (CUDAVectorBackend::computeDistances / batchKnnSearch)
// exposes L2 and Cosine via the useL2 flag, and full KNN end-to-end.
// InnerProduct and standalone TopK are only available through the dispatch
// table (populateANNDispatch), which requires caller-managed device memory.
// Adding dispatch-level CUDA benchmarks for those two would need explicit
// cudaMalloc / cudaMemcpy inside the benchmark, introducing copy latency that
// would dominate for the batch sizes used here (1 000–5 000 vectors, dim ≤ 512).  They are therefore
// measured only at the CPU level in this harness.
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

static void BM_CUDA_ANN_L2Distance(benchmark::State& state) {
    if (!cudaVec().isAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    const int nVec = static_cast<int>(state.range(0));
    const int dim  = static_cast<int>(state.range(1));
    const int nQry = 1;

    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);

    for (auto _ : state) {
        auto out = cudaVec().computeDistances(
            queries.data(), nQry, dim,
            vectors.data(), nVec, /*useL2=*/true);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 1;  // 1 = CUDA
}
BENCHMARK(BM_CUDA_ANN_L2Distance)
    ->Args({1000,  64})->Args({1000, 128})
    ->Args({1000, 256})->Args({1000, 512})
    ->Unit(benchmark::kMicrosecond);

static void BM_CUDA_ANN_CosineDistance(benchmark::State& state) {
    if (!cudaVec().isAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    const int nVec = static_cast<int>(state.range(0));
    const int dim  = static_cast<int>(state.range(1));
    const int nQry = 1;

    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);

    for (auto _ : state) {
        auto out = cudaVec().computeDistances(
            queries.data(), nQry, dim,
            vectors.data(), nVec, /*useL2=*/false);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 1;
}
BENCHMARK(BM_CUDA_ANN_CosineDistance)
    ->Args({1000,  64})->Args({1000, 128})
    ->Args({1000, 256})->Args({1000, 512})
    ->Unit(benchmark::kMicrosecond);

static void BM_CUDA_BatchKNN(benchmark::State& state) {
    if (!cudaVec().isAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    const int nVec = static_cast<int>(state.range(0));
    const int dim  = static_cast<int>(state.range(1));
    const int topK = static_cast<int>(state.range(2));
    const int nQry = 4;

    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);

    for (auto _ : state) {
        auto results = cudaVec().batchKnnSearch(
            queries.data(), nQry, dim,
            vectors.data(), nVec, topK, /*useL2=*/true);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(nQry) * nVec);
    state.counters["backend"] = 1;
    state.counters["topK"]    = topK;
}
BENCHMARK(BM_CUDA_BatchKNN)
    ->Args({ 500, 128, 10})
    ->Args({1000, 128, 10})
    ->Args({1000, 256, 10})
    ->Args({1000, 512, 10})
    ->Unit(benchmark::kMicrosecond);

static void BM_CUDA_Geo_HaversineDistance(benchmark::State& state) {
    if (!cudaGeo().isAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    const int count = static_cast<int>(state.range(0));

    auto lats1 = makeDoubles(count, -90.0,  90.0, 1);
    auto lons1 = makeDoubles(count, -180.0, 180.0, 2);
    auto lats2 = makeDoubles(count, -90.0,  90.0, 3);
    auto lons2 = makeDoubles(count, -180.0, 180.0, 4);

    for (auto _ : state) {
        auto out = cudaGeo().batchDistances(
            lats1.data(), lons1.data(),
            lats2.data(), lons2.data(),
            count, /*useHaversine=*/true);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations() * count);
    state.counters["backend"] = 1;
}
BENCHMARK(BM_CUDA_Geo_HaversineDistance)
    ->Arg(1000)->Arg(10000)->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

static void BM_CUDA_Geo_PointInPolygon(benchmark::State& state) {
    if (!cudaGeo().isAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    const int numPoints = static_cast<int>(state.range(0));

    auto ptLats = makeDoubles(numPoints, -90.0, 90.0, 5);
    auto ptLons = makeDoubles(numPoints, -180.0, 180.0, 6);

    // Berlin bounding box polygon
    const std::vector<double> polygon = {
        52.34, 13.09,  52.34, 13.76,
        52.68, 13.76,  52.68, 13.09
    };

    for (auto _ : state) {
        auto out = cudaGeo().batchPointInPolygon(
            ptLats.data(), ptLons.data(), numPoints,
            polygon.data(), static_cast<size_t>(polygon.size()) / 2);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations() * numPoints);
    state.counters["backend"] = 1;
}
BENCHMARK(BM_CUDA_Geo_PointInPolygon)
    ->Arg(1000)->Arg(10000)->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

#endif // THEMIS_ENABLE_CUDA

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
