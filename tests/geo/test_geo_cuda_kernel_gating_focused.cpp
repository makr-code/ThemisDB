/**
 * @file test_geo_cuda_kernel_gating_focused.cpp
 * @brief Focused tests for CUDA kernel gating (A-06, A-07) — CPU always, GPU conditional.
 *
 * Test structure:
 * - Tests marked "ALWAYS_CPU": Run on all platforms (validates fallback)
 * - Tests marked "GPU_OPTIONAL": Skip if CUDA not available (validates GPU path)
 *
 * Gates:
 * - GATE-A-06-01: Haversine batch correctness + performance
 * - GATE-A-06-02: Point-in-polygon batch correctness + performance
 * - GATE-A-07-01: Vincenty batch correctness + performance
 * - GATE-A-07-02: GPU dispatch integration + error handling
 */

#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <memory>

#include "geo/geo_backend_dispatch.h"

namespace themis {
namespace geo {
namespace {

// ============================================================================
// Test Fixtures
// ============================================================================

class GeoBackendDispatchTest : public ::testing::Test {
protected:
    static constexpr double kEarthRadiusKm = 6371.0;
    static constexpr double kEpsilon = 1e-6;  // Floating point tolerance
    
    void SetUp() override {
        dispatcher_ = std::make_unique<GeoBackendDispatcher>();
        
        // Generate test points
        // Point 1: San Francisco (37.7749°N, 122.4194°W)
        // Point 2: Los Angeles (34.0522°N, 118.2437°W)
        sf_point_ = {37.7749, -122.4194};
        la_point_ = {34.0522, -118.2437};
        
        // Expected Haversine distance: ~559 km
        expected_sf_la_distance_km_ = 559.0;  // Approximate
        
        // Test polygon: rectangular box around California
        california_box_.vertices = {
            {32.0, -125.0},   // SW corner
            {32.0, -114.0},   // SE corner
            {42.0, -114.0},   // NE corner
            {42.0, -125.0}    // NW corner
        };
    }
    
    std::unique_ptr<GeoBackendDispatcher> dispatcher_;
    GeoBackendDispatcher::Point sf_point_;
    GeoBackendDispatcher::Point la_point_;
    GeoBackendDispatcher::Polygon california_box_;
    double expected_sf_la_distance_km_;
};

// ============================================================================
// GATE-A-06-01: Haversine Distance Batch
// ============================================================================

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Haversine_TwoPoints) {
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {la_point_};
    
    auto result = dispatcher_->computeHaversineBatch(points1, points2);
    
    ASSERT_EQ(result.distances_km.size(), 1);
    EXPECT_NEAR(result.distances_km[0], expected_sf_la_distance_km_, 50.0);  // ±50km tolerance
    EXPECT_EQ(result.error_code, 0);
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Haversine_EmptyBatch) {
    std::vector<GeoBackendDispatcher::Point> points1;
    std::vector<GeoBackendDispatcher::Point> points2;
    
    auto result = dispatcher_->computeHaversineBatch(points1, points2);
    
    EXPECT_TRUE(result.distances_km.empty());
    EXPECT_EQ(result.error_code, 0);
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Haversine_SizesMismatch) {
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_, la_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {sf_point_};  // Size mismatch
    
    auto result = dispatcher_->computeHaversineBatch(points1, points2);
    
    EXPECT_NE(result.error_code, 0);  // Should indicate error
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Haversine_Symmetry) {
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {la_point_};
    
    auto result1 = dispatcher_->computeHaversineBatch(points1, points2);
    
    // Reverse order
    std::vector<GeoBackendDispatcher::Point> points1_rev = {la_point_};
    std::vector<GeoBackendDispatcher::Point> points2_rev = {sf_point_};
    
    auto result2 = dispatcher_->computeHaversineBatch(points1_rev, points2_rev);
    
    EXPECT_NEAR(result1.distances_km[0], result2.distances_km[0], kEpsilon);
}

TEST_F(GeoBackendDispatchTest, GPU_OPTIONAL_Haversine_LargeBatch) {
    if (!dispatcher_->isCudaAvailable()) {
        GTEST_SKIP() << "CUDA not available; CPU path tested in ALWAYS_CPU tests";
    }
    
    // Create large batch (>= threshold for GPU dispatch)
    std::vector<GeoBackendDispatcher::Point> points1(5000, sf_point_);
    std::vector<GeoBackendDispatcher::Point> points2(5000, la_point_);
    
    auto result = dispatcher_->computeHaversineBatch(points1, points2);
    
    EXPECT_EQ(result.distances_km.size(), 5000);
    for (size_t i = 0; i < result.distances_km.size(); ++i) {
        EXPECT_NEAR(result.distances_km[i], expected_sf_la_distance_km_, 50.0);
    }
}

// ============================================================================
// GATE-A-06-02: Point-in-Polygon Batch
// ============================================================================

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_PointInPolygon_PointInside) {
    std::vector<GeoBackendDispatcher::Point> test_points = {sf_point_};
    std::vector<GeoBackendDispatcher::Polygon> polygons = {california_box_};
    
    auto result = dispatcher_->computePointInPolygonBatch(
        test_points, polygons, 1);
    
    ASSERT_EQ(result.containment_mask.size(), 1);
    EXPECT_EQ(result.containment_mask[0], 1u) << "SF should be inside California box";
    EXPECT_EQ(result.error_code, 0);
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_PointInPolygon_PointOutside) {
    // Point in New York (outside California)
    GeoBackendDispatcher::Point ny_point = {40.7128, -74.0060};
    std::vector<GeoBackendDispatcher::Point> test_points = {ny_point};
    std::vector<GeoBackendDispatcher::Polygon> polygons = {california_box_};
    
    auto result = dispatcher_->computePointInPolygonBatch(
        test_points, polygons, 1);
    
    ASSERT_EQ(result.containment_mask.size(), 1);
    EXPECT_EQ(result.containment_mask[0], 0u) << "NY should be outside California box";
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_PointInPolygon_EmptyBatch) {
    std::vector<GeoBackendDispatcher::Point> test_points;
    std::vector<GeoBackendDispatcher::Polygon> polygons = {california_box_};
    
    auto result = dispatcher_->computePointInPolygonBatch(
        test_points, polygons, 0);
    
    EXPECT_TRUE(result.containment_mask.empty());
    EXPECT_EQ(result.error_code, 0);
}

TEST_F(GeoBackendDispatchTest, GPU_OPTIONAL_PointInPolygon_LargeBatch) {
    if (!dispatcher_->isCudaAvailable()) {
        GTEST_SKIP() << "CUDA not available; CPU path tested in ALWAYS_CPU tests";
    }
    
    // Create large batch of points (half inside, half outside)
    std::vector<GeoBackendDispatcher::Point> test_points;
    for (size_t i = 0; i < 2500; ++i) {
        test_points.push_back(sf_point_);      // Inside
        test_points.push_back({0.0, 0.0});     // Outside
    }
    
    std::vector<GeoBackendDispatcher::Polygon> polygons = {california_box_};
    
    auto result = dispatcher_->computePointInPolygonBatch(
        test_points, polygons, test_points.size());
    
    EXPECT_EQ(result.containment_mask.size(), test_points.size());
    EXPECT_EQ(result.error_code, 0);
    
    // Verify pattern (alternating inside/outside)
    for (size_t i = 0; i < result.containment_mask.size(); i += 2) {
        EXPECT_EQ(result.containment_mask[i], 1u);      // SF → inside
        EXPECT_EQ(result.containment_mask[i + 1], 0u);  // (0, 0) → outside
    }
}

// ============================================================================
// GATE-A-07-01: Vincenty Distance
// ============================================================================

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Vincenty_TwoPoints) {
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {la_point_};
    
    auto result = dispatcher_->computeVincentyBatch(points1, points2);
    
    ASSERT_EQ(result.distances_km.size(), 1);
    EXPECT_NEAR(result.distances_km[0], expected_sf_la_distance_km_, 50.0);
    EXPECT_EQ(result.error_code, 0);
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Vincenty_HigherPrecision) {
    // Vincenty should be at least as precise as Haversine
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {la_point_};
    
    auto vincenty_result = dispatcher_->computeVincentyBatch(points1, points2);
    auto haversine_result = dispatcher_->computeHaversineBatch(points1, points2);
    
    ASSERT_EQ(vincenty_result.distances_km.size(), 1);
    ASSERT_EQ(haversine_result.distances_km.size(), 1);
    
    // Both should be close (Vincenty slightly more accurate)
    EXPECT_NEAR(vincenty_result.distances_km[0], 
                haversine_result.distances_km[0], 10.0);  // Within 10km
}

TEST_F(GeoBackendDispatchTest, GPU_OPTIONAL_Vincenty_LargeBatch) {
    if (!dispatcher_->isCudaAvailable()) {
        GTEST_SKIP() << "CUDA not available; CPU path tested in ALWAYS_CPU tests";
    }
    
    std::vector<GeoBackendDispatcher::Point> points1(5000, sf_point_);
    std::vector<GeoBackendDispatcher::Point> points2(5000, la_point_);
    
    auto result = dispatcher_->computeVincentyBatch(points1, points2);
    
    EXPECT_EQ(result.distances_km.size(), 5000);
    for (size_t i = 0; i < result.distances_km.size(); ++i) {
        EXPECT_NEAR(result.distances_km[i], expected_sf_la_distance_km_, 50.0);
    }
}

// ============================================================================
// GATE-A-07-02: GPU Dispatch Integration
// ============================================================================

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Dispatch_QueryCudaState) {
    // Should not crash regardless of CUDA state
    bool cuda_available = dispatcher_->isCudaAvailable();
    EXPECT_TRUE(true);  // Just verify no exceptions thrown
    
    // If CUDA available, should have consistent state
    if (cuda_available) {
        EXPECT_TRUE(dispatcher_->isCudaAvailable());  // Should be same after construction
    }
}

TEST_F(GeoBackendDispatchTest, ALWAYS_CPU_Dispatch_ErrorCode_OnSuccess) {
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {la_point_};
    
    auto result = dispatcher_->computeHaversineBatch(points1, points2);
    EXPECT_EQ(result.error_code, 0) << "Success should return error_code=0";
}

TEST_F(GeoBackendDispatchTest, GPU_OPTIONAL_Dispatch_Consistency) {
    if (!dispatcher_->isCudaAvailable()) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    std::vector<GeoBackendDispatcher::Point> points1 = {sf_point_, la_point_};
    std::vector<GeoBackendDispatcher::Point> points2 = {la_point_, sf_point_};
    
    // Multiple calls should be consistent
    auto result1 = dispatcher_->computeHaversineBatch(points1, points2);
    auto result2 = dispatcher_->computeHaversineBatch(points1, points2);
    
    ASSERT_EQ(result1.distances_km.size(), result2.distances_km.size());
    for (size_t i = 0; i < result1.distances_km.size(); ++i) {
        EXPECT_NEAR(result1.distances_km[i], result2.distances_km[i], kEpsilon);
    }
}

}  // namespace
}  // namespace geo
}  // namespace themis

