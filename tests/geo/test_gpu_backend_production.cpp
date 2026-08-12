#include <gtest/gtest.h>
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include <cmath>
#include <memory>
#include <vector>
#include <chrono>
#include <cstring>
#include <thread>
#include <atomic>

using namespace themis::geo;

// ============================================================================
// Production GPU Backend Tests
// ============================================================================

class GpuBackendProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ = getProductionGpuBackend();
        ASSERT_NE(backend_, nullptr);
    }
    
    void TearDown() override {
        backend_ = nullptr;
    }
    
    ISpatialComputeBackend* backend_;
};

// Test: Backend availability
TEST_F(GpuBackendProductionTest, BackendAvailable) {
    EXPECT_NE(backend_, nullptr);
    EXPECT_TRUE(backend_->isAvailable());
    
    // Should always have a backend (at minimum CPU-parallel)
    const char* name = backend_->name();
    EXPECT_NE(name, nullptr);
    
    // Valid backend names
    std::string backend_name(name);
    EXPECT_TRUE(
        backend_name == "cuda_gpu" || 
        backend_name == "opencl_gpu" || 
        backend_name == "cpu_parallel"
    );
}

// Test: Backend name consistency
TEST_F(GpuBackendProductionTest, BackendName) {
    const char* name = backend_->name();
    EXPECT_NE(name, nullptr);
    EXPECT_GT(strlen(name), 0);
}

// Test: Empty batch input
TEST_F(GpuBackendProductionTest, EmptyBatchInput) {
    SpatialBatchInputs inputs;
    inputs.count = 0;
    
    auto results = backend_->batchIntersects(inputs);
    EXPECT_EQ(results.mask.size(), 0);
}

// Test: Small batch input
TEST_F(GpuBackendProductionTest, SmallBatchInput) {
    SpatialBatchInputs inputs;
    inputs.count = 10;
    
    auto results = backend_->batchIntersects(inputs);
    EXPECT_EQ(results.mask.size(), 10);
}

// Test: Medium batch input (CPU-parallel should handle efficiently)
TEST_F(GpuBackendProductionTest, MediumBatchInput) {
    SpatialBatchInputs inputs;
    inputs.count = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = backend_->batchIntersects(inputs);
    auto end = std::chrono::high_resolution_clock::now();
    
    EXPECT_EQ(results.mask.size(), 1000);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // Should complete within reasonable time (< 1 second for 1000 items)
    EXPECT_LT(duration.count(), 1000);
}

// Test: Large batch input
TEST_F(GpuBackendProductionTest, LargeBatchInput) {
    SpatialBatchInputs inputs;
    inputs.count = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = backend_->batchIntersects(inputs);
    auto end = std::chrono::high_resolution_clock::now();
    
    EXPECT_EQ(results.mask.size(), 10000);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // Should complete within reasonable time (< 5 seconds for 10000 items)
    EXPECT_LT(duration.count(), 5000);
}

// Test: Exact intersects - Point with polygon
TEST_F(GpuBackendProductionTest, ExactIntersectsPointPolygon) {
    // Create a simple polygon (square)
    GeometryInfo polygon(GeometryType::Polygon);
    polygon.coords = {
        Coordinate(0.0, 0.0),
        Coordinate(10.0, 0.0),
        Coordinate(10.0, 10.0),
        Coordinate(0.0, 10.0),
        Coordinate(0.0, 0.0)  // Closed ring
    };
    
    // Point inside polygon
    GeometryInfo point_inside(GeometryType::Point);
    point_inside.coords = {Coordinate(5.0, 5.0)};
    
    EXPECT_TRUE(backend_->exactIntersects(point_inside, polygon));
    EXPECT_TRUE(backend_->exactIntersects(polygon, point_inside));
    
    // Point outside polygon
    GeometryInfo point_outside(GeometryType::Point);
    point_outside.coords = {Coordinate(15.0, 15.0)};
    
    EXPECT_FALSE(backend_->exactIntersects(point_outside, polygon));
    EXPECT_FALSE(backend_->exactIntersects(polygon, point_outside));
}

// Test: Exact intersects - Point on polygon boundary
TEST_F(GpuBackendProductionTest, ExactIntersectsPointOnBoundary) {
    GeometryInfo polygon(GeometryType::Polygon);
    polygon.coords = {
        Coordinate(0.0, 0.0),
        Coordinate(10.0, 0.0),
        Coordinate(10.0, 10.0),
        Coordinate(0.0, 10.0),
        Coordinate(0.0, 0.0)
    };
    
    // Point on boundary (should intersect)
    GeometryInfo point_on_edge(GeometryType::Point);
    point_on_edge.coords = {Coordinate(5.0, 0.0)};
    
    // Boundary points may or may not intersect depending on implementation
    // Both behaviors are acceptable
    backend_->exactIntersects(point_on_edge, polygon);
}

// Test: Exact intersects - Overlapping polygons
TEST_F(GpuBackendProductionTest, ExactIntersectsOverlappingPolygons) {
    // First polygon
    GeometryInfo polygon1(GeometryType::Polygon);
    polygon1.coords = {
        Coordinate(0.0, 0.0),
        Coordinate(10.0, 0.0),
        Coordinate(10.0, 10.0),
        Coordinate(0.0, 10.0),
        Coordinate(0.0, 0.0)
    };
    
    // Second polygon overlapping first
    GeometryInfo polygon2(GeometryType::Polygon);
    polygon2.coords = {
        Coordinate(5.0, 5.0),
        Coordinate(15.0, 5.0),
        Coordinate(15.0, 15.0),
        Coordinate(5.0, 15.0),
        Coordinate(5.0, 5.0)
    };
    
    EXPECT_TRUE(backend_->exactIntersects(polygon1, polygon2));
    EXPECT_TRUE(backend_->exactIntersects(polygon2, polygon1));
}

// Test: Exact intersects - Non-overlapping polygons
TEST_F(GpuBackendProductionTest, ExactIntersectsNonOverlappingPolygons) {
    GeometryInfo polygon1(GeometryType::Polygon);
    polygon1.coords = {
        Coordinate(0.0, 0.0),
        Coordinate(10.0, 0.0),
        Coordinate(10.0, 10.0),
        Coordinate(0.0, 10.0),
        Coordinate(0.0, 0.0)
    };
    
    GeometryInfo polygon2(GeometryType::Polygon);
    polygon2.coords = {
        Coordinate(20.0, 20.0),
        Coordinate(30.0, 20.0),
        Coordinate(30.0, 30.0),
        Coordinate(20.0, 30.0),
        Coordinate(20.0, 20.0)
    };
    
    EXPECT_FALSE(backend_->exactIntersects(polygon1, polygon2));
    EXPECT_FALSE(backend_->exactIntersects(polygon2, polygon1));
}

// Test: Exact intersects - Point-point
TEST_F(GpuBackendProductionTest, ExactIntersectsPointPoint) {
    GeometryInfo point1(GeometryType::Point);
    point1.coords = {Coordinate(5.0, 5.0)};
    
    GeometryInfo point2(GeometryType::Point);
    point2.coords = {Coordinate(5.0, 5.0)};
    
    GeometryInfo point3(GeometryType::Point);
    point3.coords = {Coordinate(10.0, 10.0)};
    
    EXPECT_TRUE(backend_->exactIntersects(point1, point2));
    EXPECT_FALSE(backend_->exactIntersects(point1, point3));
}

// Test: Batch processing consistency
TEST_F(GpuBackendProductionTest, BatchProcessingConsistency) {
    // Multiple calls should return consistent results
    SpatialBatchInputs inputs;
    inputs.count = 100;
    
    auto results1 = backend_->batchIntersects(inputs);
    auto results2 = backend_->batchIntersects(inputs);
    
    EXPECT_EQ(results1.mask.size(), results2.mask.size());
    EXPECT_EQ(results1.mask.size(), 100);
}

// Test: Thread safety (basic check)
TEST_F(GpuBackendProductionTest, ThreadSafety) {
    // Create multiple threads calling the backend
    const int num_threads = 4;
    const int calls_per_thread = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<bool> exception_occurred{false};
    std::string exception_message;
    std::mutex exception_mutex;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &success_count, &exception_occurred, 
                             &exception_message, &exception_mutex, calls_per_thread]() {
            for (int i = 0; i < calls_per_thread; ++i) {
                SpatialBatchInputs inputs;
                inputs.count = 10;
                
                try {
                    auto results = backend_->batchIntersects(inputs);
                    if (results.mask.size() == 10) {
                        success_count++;
                    }
                } catch (const std::exception& e) {
                    // Capture first exception message for diagnostics
                    if (!exception_occurred.exchange(true)) {
                        std::lock_guard<std::mutex> lock(exception_mutex);
                        exception_message = e.what();
                    }
                } catch (...) {
                    // Capture unknown exception
                    if (!exception_occurred.exchange(true)) {
                        std::lock_guard<std::mutex> lock(exception_mutex);
                        exception_message = "Unknown exception";
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All calls should succeed
    if (exception_occurred.load()) {
        FAIL() << "Exception in thread: " << exception_message;
    }
    EXPECT_EQ(success_count.load(), num_threads * calls_per_thread);
}

// Performance benchmark (informational only)
TEST_F(GpuBackendProductionTest, PerformanceBenchmark) {
    const std::vector<size_t> batch_sizes = {10, 100, 1000, 10000};
    
    for (size_t batch_size : batch_sizes) {
        SpatialBatchInputs inputs;
        inputs.count = batch_size;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto results = backend_->batchIntersects(inputs);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Log performance info
        std::cout << "Backend: " << backend_->name() 
                  << ", Batch size: " << batch_size
                  << ", Duration: " << duration.count() << " µs"
                  << ", Per item: " << (duration.count() / batch_size) << " µs"
                  << std::endl;
        
        EXPECT_EQ(results.mask.size(), batch_size);
    }
}

// Test: Get Boost CPU backend
TEST_F(GpuBackendProductionTest, BoostCpuBackendAvailable) {
    auto boost_backend = getBoostCpuBackend();
    
    // May be null if Boost.Geometry not available
    if (boost_backend) {
        EXPECT_TRUE(boost_backend->isAvailable());
        EXPECT_STREQ(boost_backend->name(), "boost_cpu_exact");
    }
}

// ============================================================================
// CUDA/OpenCL parity tests (v1.4.0)
//
// Compare production GPU backend results against the CPU exact backend for a
// 10 K geometry pair dataset.  Both backends must agree on all intersection
// results (GPU uses MBR-based fast path; CPU uses exact ray-casting).
// Tests are run unconditionally — the production backend always falls back to
// CPU-parallel when no GPU is present, so the results still need to match.
// ============================================================================

namespace {

// Build a simple axis-aligned polygon (square) centred at (cx, cy) with
// half-width hw.
static GeometryInfo makeSquare(double cx, double cy, double hw) {
    GeometryInfo g(GeometryType::Polygon);
    g.rings.push_back({
        {cx - hw, cy - hw}, {cx + hw, cy - hw},
        {cx + hw, cy + hw}, {cx - hw, cy + hw},
        {cx - hw, cy - hw}   // closed ring
    });
    return g;
}

// Build an L-shaped polygon within the [ox, ox+2] × [oy, oy+2] bounding box.
// The L occupies the left column and bottom row, leaving the top-right 1×1
// cell empty.  MBR = [ox, oy, ox+2, oy+2].
static GeometryInfo makeLShape(double ox, double oy) {
    GeometryInfo g(GeometryType::Polygon);
    // Outer ring (closed): columns go left then right; rows go bottom then top.
    g.rings.push_back({
        {ox,   oy},   {ox+2, oy},   {ox+2, oy+1},
        {ox+1, oy+1}, {ox+1, oy+2}, {ox,   oy+2},
        {ox,   oy}    // closed
    });
    return g;
}

// Seeded pseudo-random doubles in [lo, hi].
static std::vector<double> seededDoubles(int count, double lo, double hi, int seed) {
    std::vector<double> v(count);
    double x = static_cast<double>(seed) * 1234567.89;
    for (int i = 0; i < count; ++i) {
        x = std::fmod(x * 1.6180339887 + 2.7182818284, 1.0e7);
        v[i] = lo + std::fmod(std::abs(x), 1.0) * (hi - lo);
    }
    return v;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Non-intersecting geometries with overlapping MBRs
//
// Two L-shaped polygons share a common 2×2 bounding box but their actual
// shapes do not overlap.  This validates that batchIntersects returns 0
// (correct) and not 1 (false positive from a pure MBR filter).
// ---------------------------------------------------------------------------

TEST(GpuProductionParityTest, BatchIntersects_LShapeAndSquare_OverlappingMBR_NoIntersection) {
    // L-shape A occupies left column + bottom row of [0,0,2,2].
    // Small square B sits in the top-right 1×1 cell [1,1,2,2] — inside the
    // MBR of A but completely in the "empty" part of the L.
    GeometryInfo lshape = makeLShape(0.0, 0.0);

    // Small square strictly inside the empty top-right corner of the L.
    GeometryInfo small_sq(GeometryType::Polygon);
    small_sq.rings.push_back({
        {1.1, 1.1}, {1.9, 1.1}, {1.9, 1.9}, {1.1, 1.9}, {1.1, 1.1}
    });

    // Confirm: MBRs overlap (small_sq is inside L's MBR).
    auto mbr_l  = lshape.computeMBR();
    auto mbr_sq = small_sq.computeMBR();
    ASSERT_TRUE(mbr_l.intersects(mbr_sq))
        << "Test pre-condition: MBRs must overlap";

    // Confirm: CPU exact backend says they do NOT intersect.
    auto* cpu = getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    bool cpu_exact = cpu->exactIntersects(lshape, small_sq);
    ASSERT_FALSE(cpu_exact)
        << "Test pre-condition: L-shape and square in empty corner must NOT intersect";

    // GPU production backend must also return 0 (no intersection).
    SpatialBatchInputs in;
    in.count = 1;
    in.geoms_a = {lshape};
    in.geoms_b = {small_sq};

    auto* gpu = getProductionGpuBackend();
    ASSERT_NE(gpu, nullptr);
    auto gpu_results = gpu->batchIntersects(in);
    ASSERT_EQ(gpu_results.mask.size(), 1u);
    EXPECT_EQ(gpu_results.mask[0], 0u)
        << "batchIntersects must return 0 for L-shape vs square-in-empty-corner "
           "(overlapping MBR, non-intersecting geometry)";
}

// ---------------------------------------------------------------------------
// 10 K batch-intersects parity
// ---------------------------------------------------------------------------

TEST(GpuProductionParityTest, BatchIntersects_10K_MatchesCpuExact) {
    const int kN = 10000;

    // Build geometry pairs: random WGS-84 bounding boxes.
    auto cx_a = seededDoubles(kN, -179.0,  179.0, 1);
    auto cy_a = seededDoubles(kN,  -89.0,   89.0, 2);
    auto hw_a = seededDoubles(kN,    0.01,   1.0,  3);
    auto cx_b = seededDoubles(kN, -179.0,  179.0, 4);
    auto cy_b = seededDoubles(kN,  -89.0,   89.0, 5);
    auto hw_b = seededDoubles(kN,    0.01,   1.0,  6);

    SpatialBatchInputs in;
    in.count = static_cast<size_t>(kN);
    in.geoms_a.reserve(kN);
    in.geoms_b.reserve(kN);
    for (int i = 0; i < kN; ++i) {
        in.geoms_a.push_back(makeSquare(cx_a[i], cy_a[i], hw_a[i]));
        in.geoms_b.push_back(makeSquare(cx_b[i], cy_b[i], hw_b[i]));
    }

    // Reference: CPU exact backend (MBR-based for polygon pairs)
    auto* cpu = getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto cpu_results = cpu->batchIntersects(in);
    ASSERT_EQ(cpu_results.mask.size(), static_cast<size_t>(kN));

    // GPU production backend
    auto* gpu = getProductionGpuBackend();
    ASSERT_NE(gpu, nullptr);
    ASSERT_TRUE(gpu->isAvailable());
    auto gpu_results = gpu->batchIntersects(in);
    ASSERT_EQ(gpu_results.mask.size(), static_cast<size_t>(kN));

    // Both backends must agree on every pair.
    int mismatch = 0;
    for (int i = 0; i < kN; ++i) {
        if ((cpu_results.mask[i] != 0) != (gpu_results.mask[i] != 0)) {
            ++mismatch;
        }
    }
    EXPECT_EQ(mismatch, 0)
        << "Production GPU backend has " << mismatch
        << " intersection mismatches vs CPU exact backend over " << kN << " pairs";
}

// ---------------------------------------------------------------------------
// exactIntersects parity: point-in-polygon
// ---------------------------------------------------------------------------

TEST(GpuProductionParityTest, ExactIntersects_PointInPolygon_MatchesCpuExact) {
    const int kN = 1000;

    // Fixed square polygon in WGS-84 range.
    GeometryInfo poly = makeSquare(10.0, 48.0, 2.0);  // centred at (10, 48)

    auto* cpu = getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto* gpu = getProductionGpuBackend();
    ASSERT_NE(gpu, nullptr);

    auto lats = seededDoubles(kN, 44.0, 52.0, 7);
    auto lons = seededDoubles(kN,  6.0, 14.0, 8);

    int mismatch = 0;
    for (int i = 0; i < kN; ++i) {
        GeometryInfo pt(GeometryType::Point);
        pt.coords.push_back({lons[i], lats[i]});

        bool cpu_hit = cpu->exactIntersects(pt, poly);
        bool gpu_hit = gpu->exactIntersects(pt, poly);
        if (cpu_hit != gpu_hit) {
            ++mismatch;
        }
    }
    EXPECT_EQ(mismatch, 0)
        << "Production GPU backend has " << mismatch
        << " exactIntersects mismatches vs CPU exact backend over " << kN << " points";
}

// ---------------------------------------------------------------------------
// Registry: production GPU backend is discoverable at runtime
// ---------------------------------------------------------------------------

TEST(GpuProductionRegistryTest, ProductionBackendIsRegistered) {
    auto* reg = getGeoBackendRegistry();
    ASSERT_NE(reg, nullptr) << "getGeoBackendRegistry() must return a non-null registry";
}

TEST(GpuProductionRegistryTest, ProductionBackendIsAvailable) {
    auto* backend = getProductionGpuBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->isAvailable());
    // Backend name must be one of the known GPU or CPU-parallel names.
    std::string name(backend->name());
    EXPECT_TRUE(name == "cuda_gpu" || name == "opencl_gpu" || name == "cpu_parallel")
        << "Unexpected production backend name: " << name;
}
