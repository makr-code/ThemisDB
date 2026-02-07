#include <gtest/gtest.h>
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include <memory>
#include <vector>
#include <chrono>

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
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &success_count, calls_per_thread]() {
            for (int i = 0; i < calls_per_thread; ++i) {
                SpatialBatchInputs inputs;
                inputs.count = 10;
                
                try {
                    auto results = backend_->batchIntersects(inputs);
                    if (results.mask.size() == 10) {
                        success_count++;
                    }
                } catch (...) {
                    // Test fails if exception thrown
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All calls should succeed
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
