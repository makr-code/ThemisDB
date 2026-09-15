#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "acceleration/cpu_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

using namespace themis::acceleration;

namespace {

constexpr double kDistanceToleranceMeters = 1e-2;

} // namespace

TEST(CategoryBGeoParity, HaversineGpuVsCpuParity) {
#ifndef THEMIS_ENABLE_CUDA
    GTEST_SKIP() << "THEMIS_ENABLE_CUDA is disabled";
#else
    CUDAGeoBackend gpu_backend;
    if (!gpu_backend.isAvailable()) {
        GTEST_SKIP() << "No CUDA-capable GPU available";
    }

    CPUGeoBackend cpu_backend;
    ASSERT_TRUE(cpu_backend.initialize());
    ASSERT_TRUE(gpu_backend.initialize());

    const std::array<double, 6> lats1 = {48.8566, 40.7128, 35.6895, -33.8688, 52.5200, 37.7749};
    const std::array<double, 6> lons1 = {2.3522, -74.0060, 139.6917, 151.2093, 13.4050, -122.4194};
    const std::array<double, 6> lats2 = {51.5074, 34.0522, 1.3521, -23.5505, 41.9028, 47.6062};
    const std::array<double, 6> lons2 = {-0.1278, -118.2437, 103.8198, -46.6333, 12.4964, -122.3321};

    const auto cpu_dist_km =
        cpu_backend.batchDistances(lats1.data(), lons1.data(), lats2.data(), lons2.data(), lats1.size(), true);
    const auto gpu_dist_km =
        gpu_backend.batchDistances(lats1.data(), lons1.data(), lats2.data(), lons2.data(), lats1.size(), true);

    ASSERT_EQ(cpu_dist_km.size(), lats1.size());
    ASSERT_EQ(gpu_dist_km.size(), lats1.size());

    for (std::size_t i = 0; i < lats1.size(); ++i) {
        const double cpu_m = static_cast<double>(cpu_dist_km[i]) * 1000.0;
        const double gpu_m = static_cast<double>(gpu_dist_km[i]) * 1000.0;
        EXPECT_NEAR(gpu_m, cpu_m, kDistanceToleranceMeters);
    }

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}

TEST(CategoryBGeoParity, ContainsGpuVsCpuParity) {
#ifndef THEMIS_ENABLE_CUDA
    GTEST_SKIP() << "THEMIS_ENABLE_CUDA is disabled";
#else
    CUDAGeoBackend gpu_backend;
    if (!gpu_backend.isAvailable()) {
        GTEST_SKIP() << "No CUDA-capable GPU available";
    }

    CPUGeoBackend cpu_backend;
    ASSERT_TRUE(cpu_backend.initialize());
    ASSERT_TRUE(gpu_backend.initialize());

    const std::array<double, 6> point_lats = {0.5, 2.5, 1.0, -1.0, 0.25, 3.0};
    const std::array<double, 6> point_lons = {0.5, 2.5, 0.0,  0.0,  1.75, 1.0};
    const std::array<double, 8> polygon = {
        0.0, 0.0,
        0.0, 2.0,
        2.0, 2.0,
        2.0, 0.0
    };

    const auto cpu_contains = cpu_backend.batchPointInPolygon(
        point_lats.data(), point_lons.data(), point_lats.size(), polygon.data(), polygon.size() / 2);
    const auto gpu_contains = gpu_backend.batchPointInPolygon(
        point_lats.data(), point_lons.data(), point_lats.size(), polygon.data(), polygon.size() / 2);

    ASSERT_EQ(cpu_contains.size(), point_lats.size());
    ASSERT_EQ(gpu_contains.size(), point_lats.size());

    for (std::size_t i = 0; i < point_lats.size(); ++i) {
        EXPECT_EQ(gpu_contains[i], cpu_contains[i])
            << "Containment mismatch at index " << i
            << " point=(" << point_lats[i] << "," << point_lons[i] << ")";
    }

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}
