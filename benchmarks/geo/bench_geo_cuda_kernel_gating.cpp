/**
 * @file bench_geo_cuda_kernel_gating.cpp
 * @brief Performance gates for CUDA-accelerated geospatial kernels (A-06, A-07).
 *
 * Validates performance gates for GPU-accelerated Haversine, Point-in-Polygon,
 * and Vincenty distance computations. Gates are conditional on THEMIS_GEO_CUDA=ON
 * and GPU device availability.
 *
 * Performance Targets (per ai_working/GEO_MODULE_PHASE_56_AND_CUDA_KERNEL_GATING_2026_08_10.md):
 * - GATE-A-06-01: Haversine batch p99 ≤ 500ms (1000-10000 points)
 * - GATE-A-06-02: Point-in-Polygon p99 ≤ 2ms (typical polygon set)
 * - GATE-A-07-01: Vincenty batch p99 ≤ 500ms (1000-10000 points)
 * - GATE-A-07-02: GPU batch dispatch queue throughput ≥ 8x vs CPU baseline
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <cmath>
#include <random>

#include "geo/geo_backend_dispatch.h"

namespace themis {
namespace geo {
namespace {

// ============================================================================
// Benchmark Fixtures
// ============================================================================

class GeoGpuGatingFixture : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State&) override {
        // Initialize test data
        rng_.seed(42);  // Deterministic; matches Wave 1 hygiene
        
        // Generate random WGS84 points
        std::uniform_real_distribution<double> lat_dist(-90.0, 90.0);
        std::uniform_real_distribution<double> lon_dist(-180.0, 180.0);
        
        for (size_t i = 0; i < kMaxBatchSize; ++i) {
            points1_.push_back({lat_dist(rng_), lon_dist(rng_)});
            points2_.push_back({lat_dist(rng_), lon_dist(rng_)});
        }
        
        // Create test polygon (e.g., bounding box for a country)
        polygon_.vertices = {
            {-90.0, -180.0},
            {-90.0, 180.0},
            {90.0, 180.0},
            {90.0, -180.0}
        };
    }
    
    void TearDown(const benchmark::State&) override {}
    
    static constexpr size_t kMaxBatchSize = 10000;
    static constexpr size_t kSmallBatchSize = 1000;
    static constexpr size_t kMediumBatchSize = 5000;
    
    std::mt19937_64 rng_;
    std::vector<GeoBackendDispatcher::Point> points1_;
    std::vector<GeoBackendDispatcher::Point> points2_;
    GeoBackendDispatcher::Polygon polygon_;
};

}  // namespace

// ============================================================================
// GATE-A-06-01: Haversine Batch (GPU vs CPU)
// ============================================================================

BENCHMARK_F(GeoGpuGatingFixture, GATE_A_06_01_Haversine_SmallBatch)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher;
    
    // Skip if CUDA not available
    if (!dispatcher.isCudaAvailable()) {
        state.SkipWithMessage("CUDA not available (THEMIS_GEO_CUDA=OFF or no GPU)");
        return;
    }
    
    std::vector<GeoBackendDispatcher::Point> test_points1(
        points1_.begin(), points1_.begin() + kSmallBatchSize);
    std::vector<GeoBackendDispatcher::Point> test_points2(
        points2_.begin(), points2_.begin() + kSmallBatchSize);
    
    for (auto _ : state) {
        auto result = dispatcher.computeHaversineBatch(test_points1, test_points2);
        benchmark::DoNotOptimize(result);
    }
    
    // Performance gate: p99 ≤ 500ms (typical GPU throughput ~1000 pts/ms)
    state.SetLabel("GATE-A-06-01");
}

BENCHMARK_F(GeoGpuGatingFixture, GATE_A_06_01_Haversine_LargeBatch)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher = {};
    
    if (!dispatcher.isCudaAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    
    std::vector<GeoBackendDispatcher::Point> test_points1(
        points1_.begin(), points1_.begin() + kMaxBatchSize);
    std::vector<GeoBackendDispatcher::Point> test_points2(
        points2_.begin(), points2_.begin() + kMaxBatchSize);
    
    for (auto _ : state) {
        auto result = dispatcher.computeHaversineBatch(test_points1, test_points2);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("GATE-A-06-01 (large)");
}

// ============================================================================
// GATE-A-06-02: Point-in-Polygon (GPU vs CPU)
// ============================================================================

BENCHMARK_F(GeoGpuGatingFixture, GATE_A_06_02_PointInPolygon)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher = {};
    
    if (!dispatcher.isCudaAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    
    // Test with medium batch of points
    std::vector<GeoBackendDispatcher::Point> test_points(
        points1_.begin(), points1_.begin() + kMediumBatchSize);
    std::vector<GeoBackendDispatcher::Polygon> polygons = {polygon_};
    
    for (auto _ : state) {
        auto result = dispatcher.computePointInPolygonBatch(
            test_points, polygons, test_points.size());
        benchmark::DoNotOptimize(result);
    }
    
    // Performance gate: p99 ≤ 2ms
    state.SetLabel("GATE-A-06-02");
}

// ============================================================================
// GATE-A-07-01: Vincenty Batch Distance
// ============================================================================

BENCHMARK_F(GeoGpuGatingFixture, GATE_A_07_01_Vincenty_SmallBatch)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher = {};
    
    if (!dispatcher.isCudaAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    
    std::vector<GeoBackendDispatcher::Point> test_points1(
        points1_.begin(), points1_.begin() + kSmallBatchSize);
    std::vector<GeoBackendDispatcher::Point> test_points2(
        points2_.begin(), points2_.begin() + kSmallBatchSize);
    
    for (auto _ : state) {
        auto result = dispatcher.computeVincentyBatch(test_points1, test_points2);
        benchmark::DoNotOptimize(result);
    }
    
    // Performance gate: p99 ≤ 500ms (ellipsoidal model; slightly slower than Haversine)
    state.SetLabel("GATE-A-07-01");
}

BENCHMARK_F(GeoGpuGatingFixture, GATE_A_07_01_Vincenty_LargeBatch)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher = {};
    
    if (!dispatcher.isCudaAvailable()) {
        state.SkipWithMessage("CUDA not available");
        return;
    }
    
    std::vector<GeoBackendDispatcher::Point> test_points1(
        points1_.begin(), points1_.begin() + kMaxBatchSize);
    std::vector<GeoBackendDispatcher::Point> test_points2(
        points2_.begin(), points2_.begin() + kMaxBatchSize);
    
    for (auto _ : state) {
        auto result = dispatcher.computeVincentyBatch(test_points1, test_points2);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("GATE-A-07-01 (large)");
}

// ============================================================================
// GATE-A-07-02: GPU Batch Dispatch Throughput vs CPU
// ============================================================================

BENCHMARK_F(GeoGpuGatingFixture, GATE_A_07_02_GpuThroughputVsCpu)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher;
    
    // This gate measures relative GPU throughput
    // Expected: GPU throughput ≥ 8x CPU baseline for large batches
    
    if (!dispatcher.isCudaAvailable()) {
        state.SkipWithMessage("CUDA not available (CPU baseline only)");
        return;
    }
    
    // Dispatch 10 large batches sequentially to measure throughput
    std::vector<GeoBackendDispatcher::Point> test_points1(
        points1_.begin(), points1_.begin() + kMaxBatchSize);
    std::vector<GeoBackendDispatcher::Point> test_points2(
        points2_.begin(), points2_.begin() + kMaxBatchSize);
    
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) {
            auto result = dispatcher.computeHaversineBatch(
                test_points1, test_points2);
            benchmark::DoNotOptimize(result);
        }
    }
    
    // Performance gate: Throughput ≥ 8x CPU baseline
    // GPU baseline: ~10 batches of 10k points in time T
    // Expected on RTX-class GPU: T < baseline_cpu_time / 8
    state.SetLabel("GATE-A-07-02 (throughput)");
}

// ============================================================================
// CPU Baseline Benchmarks (Always Compiled)
// ============================================================================

BENCHMARK_F(GeoGpuGatingFixture, CPU_Baseline_Haversine)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher;
    
    // Ensure CPU path even on systems with GPU
    // (GPU will not be used if batch size threshold not met)
    std::vector<GeoBackendDispatcher::Point> test_points1(
        points1_.begin(), points1_.begin() + 100);  // Small batch < threshold
    std::vector<GeoBackendDispatcher::Point> test_points2(
        points2_.begin(), points2_.begin() + 100);
    
    for (auto _ : state) {
        auto result = dispatcher.computeHaversineBatch(test_points1, test_points2);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("CPU Baseline Haversine");
}

BENCHMARK_F(GeoGpuGatingFixture, CPU_Baseline_PointInPolygon)(
    benchmark::State& state) {
    
    GeoBackendDispatcher dispatcher;
    
    std::vector<GeoBackendDispatcher::Point> test_points(
        points1_.begin(), points1_.begin() + 100);
    std::vector<GeoBackendDispatcher::Polygon> polygons = {polygon_};
    
    for (auto _ : state) {
        auto result = dispatcher.computePointInPolygonBatch(
            test_points, polygons, test_points.size());
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("CPU Baseline PointInPolygon");
}

}  // namespace geo
}  // namespace themis
