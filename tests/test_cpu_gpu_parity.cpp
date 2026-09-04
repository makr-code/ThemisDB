#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "acceleration/cpu_backend.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/graphics_backends.h"
#include "acceleration/kernel_invocation.h"

using namespace themis::acceleration;

// ============================================================================
// Test-data helpers (deterministic, seeded)
// ============================================================================

namespace {

/// Fill a float vector with uniform random values in [-1, 1].
std::vector<float> makeFloats(size_t n, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> v(n);
    for (auto& x : v) {
      x = dist(rng);
    }
    return v;
}

/// Fill a double vector with uniform random values in [lo, hi].
std::vector<double> makeDoubles(size_t n, double lo, double hi,
                                uint32_t seed = 1) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(lo, hi);
    std::vector<double> v(n);
    for (auto& x : v) {
      x = dist(rng);
    }
    return v;
}

/// ANN dispatch parity tolerance — GPU results are expected to be within this
/// relative+absolute bound compared to the CPU reference.
static constexpr float kANNTol = 1e-3f;

/// Geo parity tolerance (km) — a small multiple of FP32 rounding for distances
/// computed in double on CPU and possibly in single-precision on some GPU shaders.
static constexpr float kGeoTol = 1e-2f;

} // anonymous namespace

// ============================================================================
// CUDA ANN parity fixture
//
// SetUp initialises both backends and populates their dispatch tables.
// Every test in this fixture assumes the tables are fully populated.
// Tests are skipped automatically when CUDA is absent (compile or runtime).
// ============================================================================

class CpuVsCudaANNParityTest : public ::testing::Test {
protected:
    CPUVectorBackend  cpuBackend_;
    CUDAVectorBackend cudaBackend_;
    ANNKernelDispatch cpuDisp_;
    ANNKernelDispatch cudaDisp_;

    void SetUp() override {
#ifndef THEMIS_ENABLE_CUDA
        GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled_for_ann_parity";
#else
        if (!cudaBackend_.isAvailable()) {
            GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device_for_ann_parity";
        }
        ASSERT_TRUE(cpuBackend_.initialize());
        ASSERT_TRUE(cudaBackend_.initialize());
        cpuDisp_  = cpuBackend_.populateANNDispatch();
        cudaDisp_ = cudaBackend_.populateANNDispatch();
        ASSERT_NE(cpuDisp_.launchL2Distance,    nullptr) << "CPU L2 slot missing";
        ASSERT_NE(cpuDisp_.launchCosine,        nullptr) << "CPU Cosine slot missing";
        ASSERT_NE(cpuDisp_.launchInnerProduct,  nullptr) << "CPU IP slot missing";
        ASSERT_NE(cpuDisp_.launchTopK,          nullptr) << "CPU TopK slot missing";
        ASSERT_NE(cudaDisp_.launchL2Distance,   nullptr) << "CUDA L2 slot missing";
        ASSERT_NE(cudaDisp_.launchCosine,       nullptr) << "CUDA Cosine slot missing";
        ASSERT_NE(cudaDisp_.launchInnerProduct, nullptr) << "CUDA IP slot missing";
        ASSERT_NE(cudaDisp_.launchTopK,         nullptr) << "CUDA TopK slot missing";
#endif
    }

    void TearDown() override {
        cpuBackend_.shutdown();
        cudaBackend_.shutdown();
    }
};

TEST_F(CpuVsCudaANNParityTest, L2Distance_MatchesCPUReference) {
    const int nQry = 4, nVec = 64, dim = 32;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);
    std::vector<float> cpuOut(nQry * nVec), cudaOut(nQry * nVec);

    ASSERT_EQ(cpuDisp_.launchL2Distance(queries.data(), vectors.data(),
                                        cpuOut.data(), nQry, nVec, dim, nullptr), 0);
    ASSERT_EQ(cudaDisp_.launchL2Distance(queries.data(), vectors.data(),
                                         cudaOut.data(), nQry, nVec, dim, nullptr), 0);

    for (int i = 0; i < nQry * nVec; ++i) {
        EXPECT_NEAR(cpuOut[i], cudaOut[i], kANNTol)
            << "CUDA L2 parity mismatch at index " << i;
    }
}

TEST_F(CpuVsCudaANNParityTest, CosineDistance_MatchesCPUReference) {
    const int nQry = 4, nVec = 64, dim = 32;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 17);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 53);
    std::vector<float> cpuOut(nQry * nVec), cudaOut(nQry * nVec);

    ASSERT_EQ(cpuDisp_.launchCosine(queries.data(), vectors.data(),
                                    cpuOut.data(), nQry, nVec, dim, nullptr), 0);
    ASSERT_EQ(cudaDisp_.launchCosine(queries.data(), vectors.data(),
                                     cudaOut.data(), nQry, nVec, dim, nullptr), 0);

    for (int i = 0; i < nQry * nVec; ++i) {
        EXPECT_NEAR(cpuOut[i], cudaOut[i], kANNTol)
            << "CUDA Cosine parity mismatch at index " << i;
    }
}

TEST_F(CpuVsCudaANNParityTest, InnerProduct_MatchesCPUReference) {
    const int nQry = 4, nVec = 64, dim = 32;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 7);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 13);
    std::vector<float> cpuOut(nQry * nVec), cudaOut(nQry * nVec);

    ASSERT_EQ(cpuDisp_.launchInnerProduct(queries.data(), vectors.data(),
                                          cpuOut.data(), nQry, nVec, dim, nullptr), 0);
    ASSERT_EQ(cudaDisp_.launchInnerProduct(queries.data(), vectors.data(),
                                           cudaOut.data(), nQry, nVec, dim, nullptr), 0);

    for (int i = 0; i < nQry * nVec; ++i) {
        EXPECT_NEAR(cpuOut[i], cudaOut[i], kANNTol)
            << "CUDA InnerProduct parity mismatch at index " << i;
    }
}

TEST_F(CpuVsCudaANNParityTest, TopK_IndicesAndDistancesMatchCPUReference) {
    // Pre-build a shared distance matrix so both backends select from identical input.
    const int nQry = 2, nVec = 100, topK = 10;
    auto dists = makeFloats(static_cast<size_t>(nQry) * nVec, 31);
    std::vector<uint32_t> cpuIdx(nQry * topK), cudaIdx(nQry * topK);
    std::vector<float>    cpuDst(nQry * topK), cudaDst(nQry * topK);

    ASSERT_EQ(cpuDisp_.launchTopK(dists.data(), cpuIdx.data(), cpuDst.data(),
                                  nQry, nVec, topK, nullptr), 0);
    ASSERT_EQ(cudaDisp_.launchTopK(dists.data(), cudaIdx.data(), cudaDst.data(),
                                   nQry, nVec, topK, nullptr), 0);

    for (int i = 0; i < nQry * topK; ++i) {
        EXPECT_FLOAT_EQ(cpuDst[i], cudaDst[i])
            << "CUDA TopK distance parity mismatch at position " << i;
        EXPECT_EQ(cpuIdx[i], cudaIdx[i])
            << "CUDA TopK index parity mismatch at position " << i;
    }
}

// ============================================================================
// CUDA Geo parity fixture
// ============================================================================

class CpuVsCudaGeoParityTest : public ::testing::Test {
protected:
    CPUGeoBackend  cpuBackend_;
    CUDAGeoBackend cudaBackend_;
    GeoKernelDispatch cpuDisp_;
    GeoKernelDispatch cudaDisp_;

    void SetUp() override {
#ifndef THEMIS_ENABLE_CUDA
        GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled_for_geo_parity";
#else
        if (!cudaBackend_.isAvailable()) {
            GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_device_for_geo_parity";
        }
        ASSERT_TRUE(cpuBackend_.initialize());
        ASSERT_TRUE(cudaBackend_.initialize());
        cpuDisp_  = cpuBackend_.populateGeoDispatch();
        cudaDisp_ = cudaBackend_.populateGeoDispatch();
        ASSERT_NE(cpuDisp_.launchDistance,    nullptr) << "CPU Haversine slot missing";
        ASSERT_NE(cpuDisp_.launchContainment, nullptr) << "CPU PiP slot missing";
        ASSERT_NE(cudaDisp_.launchDistance,   nullptr) << "CUDA Haversine slot missing";
        ASSERT_NE(cudaDisp_.launchContainment,nullptr) << "CUDA PiP slot missing";
#endif
    }

    void TearDown() override {
        cpuBackend_.shutdown();
        cudaBackend_.shutdown();
    }
};

TEST_F(CpuVsCudaGeoParityTest, HaversineDistance_MatchesCPUReference) {
    const int count = 200;
    auto lats1 = makeDoubles(count, -90.0,  90.0, 1);
    auto lons1 = makeDoubles(count, -180.0, 180.0, 2);
    auto lats2 = makeDoubles(count, -90.0,  90.0, 3);
    auto lons2 = makeDoubles(count, -180.0, 180.0, 4);
    std::vector<float> cpuOut(count), cudaOut(count);

    ASSERT_EQ(cpuDisp_.launchDistance(lats1.data(), lons1.data(),
                                      lats2.data(), lons2.data(),
                                      cpuOut.data(), count,
                                      GeoDistanceFormula::HAVERSINE, nullptr), 0);
    ASSERT_EQ(cudaDisp_.launchDistance(lats1.data(), lons1.data(),
                                       lats2.data(), lons2.data(),
                                       cudaOut.data(), count,
                                       GeoDistanceFormula::HAVERSINE, nullptr), 0);

    for (int i = 0; i < count; ++i) {
        EXPECT_NEAR(cpuOut[i], cudaOut[i], kGeoTol)
            << "CUDA Haversine parity mismatch at index " << i;
    }
}

TEST_F(CpuVsCudaGeoParityTest, PointInPolygon_MatchesCPUReference) {
    // Berlin bounding box (same polygon used in bench_cuda_vs_cpu.cpp)
    const std::vector<double> polygon = {
        52.34, 13.09,  52.34, 13.76,
        52.68, 13.76,  52.68, 13.09
    };
    const int numVerts = static_cast<int>(polygon.size()) / 2;
    const int numPts   = 100;

    auto ptLats = makeDoubles(numPts, 50.0, 54.0, 5);
    auto ptLons = makeDoubles(numPts, 11.0, 15.0, 6);
    std::vector<uint8_t> cpuOut(numPts, 255u), cudaOut(numPts, 255u);

    ASSERT_EQ(cpuDisp_.launchContainment(ptLats.data(), ptLons.data(), numPts,
                                         polygon.data(), numVerts,
                                         cpuOut.data(), nullptr), 0);
    ASSERT_EQ(cudaDisp_.launchContainment(ptLats.data(), ptLons.data(), numPts,
                                          polygon.data(), numVerts,
                                          cudaOut.data(), nullptr), 0);

    for (int i = 0; i < numPts; ++i) {
        // Both backends must agree on inside/outside (non-zero vs zero).
        bool cpuIn  = (cpuOut[i]  != 0u);
        bool cudaIn = (cudaOut[i] != 0u);
        EXPECT_EQ(cpuIn, cudaIn)
            << "CUDA PiP parity mismatch at point " << i
            << " (lat=" << ptLats[i] << " lon=" << ptLons[i] << ")";
    }
}

// ============================================================================
// Vulkan ANN parity fixture
// ============================================================================

class CpuVsVulkanANNParityTest : public ::testing::Test {
protected:
    CPUVectorBackend    cpuBackend_;
    VulkanVectorBackend vulkanBackend_;
    ANNKernelDispatch   cpuDisp_;
    ANNKernelDispatch   vulkanDisp_;

    void SetUp() override {
#ifndef THEMIS_ENABLE_VULKAN
        GTEST_SKIP() << "capability:vulkan_compiled=false;reason=vulkan_not_compiled_for_ann_parity";
#else
        if (!vulkanBackend_.isAvailable()) {
            GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=no_vulkan_device_for_ann_parity";
        }
        ASSERT_TRUE(cpuBackend_.initialize());
        if (!vulkanBackend_.initialize()) {
            GTEST_SKIP() << "Vulkan backend initialization failed; SPIR-V shaders not compiled or missing";
        }
        cpuDisp_    = cpuBackend_.populateANNDispatch();
        vulkanDisp_ = vulkanBackend_.populateANNDispatch();
        ASSERT_NE(cpuDisp_.launchL2Distance,       nullptr) << "CPU L2 slot missing";
        ASSERT_NE(cpuDisp_.launchCosine,           nullptr) << "CPU Cosine slot missing";
        ASSERT_NE(cpuDisp_.launchInnerProduct,     nullptr) << "CPU IP slot missing";
        ASSERT_NE(cpuDisp_.launchTopK,             nullptr) << "CPU TopK slot missing";
        ASSERT_NE(vulkanDisp_.launchL2Distance,    nullptr) << "Vulkan L2 slot missing";
        ASSERT_NE(vulkanDisp_.launchCosine,        nullptr) << "Vulkan Cosine slot missing";
        ASSERT_NE(vulkanDisp_.launchInnerProduct,  nullptr) << "Vulkan IP slot missing";
        ASSERT_NE(vulkanDisp_.launchTopK,          nullptr) << "Vulkan TopK slot missing";
#endif
    }

    void TearDown() override {
        cpuBackend_.shutdown();
        vulkanBackend_.shutdown();
    }
};

TEST_F(CpuVsVulkanANNParityTest, L2Distance_MatchesCPUReference) {
    const int nQry = 4, nVec = 64, dim = 32;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);
    std::vector<float> cpuOut(nQry * nVec), vkOut(nQry * nVec);

    ASSERT_EQ(cpuDisp_.launchL2Distance(queries.data(), vectors.data(),
                                        cpuOut.data(), nQry, nVec, dim, nullptr), 0);
    ASSERT_EQ(vulkanDisp_.launchL2Distance(queries.data(), vectors.data(),
                                           vkOut.data(), nQry, nVec, dim, nullptr), 0);

    for (int i = 0; i < nQry * nVec; ++i) {
        EXPECT_NEAR(cpuOut[i], vkOut[i], kANNTol)
            << "Vulkan L2 parity mismatch at index " << i;
    }
}

TEST_F(CpuVsVulkanANNParityTest, CosineDistance_MatchesCPUReference) {
    const int nQry = 4, nVec = 64, dim = 32;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 17);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 53);
    std::vector<float> cpuOut(nQry * nVec), vkOut(nQry * nVec);

    ASSERT_EQ(cpuDisp_.launchCosine(queries.data(), vectors.data(),
                                    cpuOut.data(), nQry, nVec, dim, nullptr), 0);
    ASSERT_EQ(vulkanDisp_.launchCosine(queries.data(), vectors.data(),
                                       vkOut.data(), nQry, nVec, dim, nullptr), 0);

    for (int i = 0; i < nQry * nVec; ++i) {
        EXPECT_NEAR(cpuOut[i], vkOut[i], kANNTol)
            << "Vulkan Cosine parity mismatch at index " << i;
    }
}

TEST_F(CpuVsVulkanANNParityTest, InnerProduct_MatchesCPUReference) {
    const int nQry = 4, nVec = 64, dim = 32;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 7);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 13);
    std::vector<float> cpuOut(nQry * nVec), vkOut(nQry * nVec);

    ASSERT_EQ(cpuDisp_.launchInnerProduct(queries.data(), vectors.data(),
                                          cpuOut.data(), nQry, nVec, dim, nullptr), 0);
    ASSERT_EQ(vulkanDisp_.launchInnerProduct(queries.data(), vectors.data(),
                                             vkOut.data(), nQry, nVec, dim, nullptr), 0);

    for (int i = 0; i < nQry * nVec; ++i) {
        EXPECT_NEAR(cpuOut[i], vkOut[i], kANNTol)
            << "Vulkan InnerProduct parity mismatch at index " << i;
    }
}

TEST_F(CpuVsVulkanANNParityTest, TopK_IndicesAndDistancesMatchCPUReference) {
    const int nQry = 2, nVec = 100, topK = 10;
    auto dists = makeFloats(static_cast<size_t>(nQry) * nVec, 31);
    std::vector<uint32_t> cpuIdx(nQry * topK), vkIdx(nQry * topK);
    std::vector<float>    cpuDst(nQry * topK), vkDst(nQry * topK);

    ASSERT_EQ(cpuDisp_.launchTopK(dists.data(), cpuIdx.data(), cpuDst.data(),
                                  nQry, nVec, topK, nullptr), 0);
    ASSERT_EQ(vulkanDisp_.launchTopK(dists.data(), vkIdx.data(), vkDst.data(),
                                     nQry, nVec, topK, nullptr), 0);

    for (int i = 0; i < nQry * topK; ++i) {
        EXPECT_FLOAT_EQ(cpuDst[i], vkDst[i])
            << "Vulkan TopK distance parity mismatch at position " << i;
        EXPECT_EQ(cpuIdx[i], vkIdx[i])
            << "Vulkan TopK index parity mismatch at position " << i;
    }
}

// ============================================================================
// Vulkan Geo parity fixture
// ============================================================================

class CpuVsVulkanGeoParityTest : public ::testing::Test {
protected:
    CPUGeoBackend       cpuBackend_;
    VulkanGeoBackend    vulkanBackend_;
    GeoKernelDispatch   cpuDisp_;
    GeoKernelDispatch   vulkanDisp_;

    void SetUp() override {
#ifndef THEMIS_ENABLE_VULKAN
        GTEST_SKIP() << "capability:vulkan_compiled=false;reason=vulkan_not_compiled_for_geo_parity";
#else
        if (!vulkanBackend_.isAvailable()) {
            GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=no_vulkan_device_for_geo_parity";
        }
        ASSERT_TRUE(cpuBackend_.initialize());
        ASSERT_TRUE(vulkanBackend_.initialize());
        cpuDisp_    = cpuBackend_.populateGeoDispatch();
        vulkanDisp_ = vulkanBackend_.populateGeoDispatch();
        ASSERT_NE(cpuDisp_.launchDistance,     nullptr) << "CPU Haversine slot missing";
        ASSERT_NE(cpuDisp_.launchContainment,  nullptr) << "CPU PiP slot missing";
        ASSERT_NE(vulkanDisp_.launchDistance,  nullptr) << "Vulkan Haversine slot missing";
        ASSERT_NE(vulkanDisp_.launchContainment, nullptr) << "Vulkan PiP slot missing";
#endif
    }

    void TearDown() override {
        cpuBackend_.shutdown();
        vulkanBackend_.shutdown();
    }
};

TEST_F(CpuVsVulkanGeoParityTest, HaversineDistance_MatchesCPUReference) {
    const int count = 200;
    auto lats1 = makeDoubles(count, -90.0,  90.0, 1);
    auto lons1 = makeDoubles(count, -180.0, 180.0, 2);
    auto lats2 = makeDoubles(count, -90.0,  90.0, 3);
    auto lons2 = makeDoubles(count, -180.0, 180.0, 4);
    std::vector<float> cpuOut(count), vkOut(count);

    ASSERT_EQ(cpuDisp_.launchDistance(lats1.data(), lons1.data(),
                                      lats2.data(), lons2.data(),
                                      cpuOut.data(), count,
                                      GeoDistanceFormula::HAVERSINE, nullptr), 0);
    ASSERT_EQ(vulkanDisp_.launchDistance(lats1.data(), lons1.data(),
                                         lats2.data(), lons2.data(),
                                         vkOut.data(), count,
                                         GeoDistanceFormula::HAVERSINE, nullptr), 0);

    for (int i = 0; i < count; ++i) {
        EXPECT_NEAR(cpuOut[i], vkOut[i], kGeoTol)
            << "Vulkan Haversine parity mismatch at index " << i;
    }
}

TEST_F(CpuVsVulkanGeoParityTest, PointInPolygon_MatchesCPUReference) {
    const std::vector<double> polygon = {
        52.34, 13.09,  52.34, 13.76,
        52.68, 13.76,  52.68, 13.09
    };
    const int numVerts = static_cast<int>(polygon.size()) / 2;
    const int numPts   = 100;

    auto ptLats = makeDoubles(numPts, 50.0, 54.0, 5);
    auto ptLons = makeDoubles(numPts, 11.0, 15.0, 6);
    std::vector<uint8_t> cpuOut(numPts, 255u), vkOut(numPts, 255u);

    ASSERT_EQ(cpuDisp_.launchContainment(ptLats.data(), ptLons.data(), numPts,
                                         polygon.data(), numVerts,
                                         cpuOut.data(), nullptr), 0);
    ASSERT_EQ(vulkanDisp_.launchContainment(ptLats.data(), ptLons.data(), numPts,
                                            polygon.data(), numVerts,
                                            vkOut.data(), nullptr), 0);

    for (int i = 0; i < numPts; ++i) {
        bool cpuIn = (cpuOut[i] != 0u);
        bool vkIn  = (vkOut[i]  != 0u);
        EXPECT_EQ(cpuIn, vkIn)
            << "Vulkan PiP parity mismatch at point " << i
            << " (lat=" << ptLats[i] << " lon=" << ptLons[i] << ")";
    }
}

// ============================================================================
// Cross-backend registry parity test
//
// Validates that the BackendRegistry's getBestVectorBackend() and
// getBestGeoBackend() produce results consistent with the CPU reference even
// when a higher-priority backend is selected — i.e. verifies that the registry
// selection and fallback chain do not silently swap in an inconsistent backend.
// ============================================================================

class RegistryParityTest : public ::testing::Test {
protected:
    CPUVectorBackend cpuVec_;
    CPUGeoBackend    cpuGeo_;

    void SetUp() override {
        auto vec = std::make_unique<CPUVectorBackend>();
        auto geo = std::make_unique<CPUGeoBackend>();
        BackendRegistry::instance().registerBackend(std::move(vec));
        BackendRegistry::instance().registerBackend(std::move(geo));

        ASSERT_TRUE(cpuVec_.initialize());
        ASSERT_TRUE(cpuGeo_.initialize());
    }

    void TearDown() override {
        BackendRegistry::instance().shutdownAll();
        cpuVec_.shutdown();
        cpuGeo_.shutdown();
    }
};

TEST_F(RegistryParityTest, BestVectorBackend_ProducesConsistentL2Distances) {
    auto* best = BackendRegistry::instance().getBestVectorBackend();
    ASSERT_NE(best, nullptr);
    if (!best->initialize()) {
        GTEST_SKIP() << "Best vector backend initialization failed; shaders or drivers unavailable";
    }

    const int nQry = 2, nVec = 8, dim = 4;
    auto queries = makeFloats(static_cast<size_t>(nQry) * dim, 42);
    auto vectors = makeFloats(static_cast<size_t>(nVec) * dim, 99);

    auto bestDists = best->computeDistances(queries.data(), nQry, dim,
                                            vectors.data(), nVec, /*useL2=*/true);
    auto cpuDists  = cpuVec_.computeDistances(queries.data(), nQry, dim,
                                              vectors.data(), nVec, /*useL2=*/true);

    ASSERT_EQ(bestDists.size(), cpuDists.size());
    for (size_t i = 0; i < cpuDists.size(); ++i) {
        EXPECT_NEAR(bestDists[i], cpuDists[i], kANNTol)
            << "Registry best-vector-backend L2 parity mismatch at index " << i;
    }

    best->shutdown();
}

TEST_F(RegistryParityTest, BestGeoBackend_ProducesConsistentHaversineDistances) {
    auto* best = BackendRegistry::instance().getBestGeoBackend();
    ASSERT_NE(best, nullptr);
    ASSERT_TRUE(best->initialize());

    // London–Paris pair and a few synthetic coordinate pairs
    const std::vector<double> lats1 = {51.5074, 40.7128, -33.8688};
    const std::vector<double> lons1 = {-0.1278, -74.0060, 151.2093};
    const std::vector<double> lats2 = {48.8566, 34.0522, 35.6762};
    const std::vector<double> lons2 = { 2.3522, -118.2437, 139.6503};
    const int count = static_cast<int>(lats1.size());

    auto bestDists = best->batchDistances(lats1.data(), lons1.data(),
                                          lats2.data(), lons2.data(),
                                          count, /*useHaversine=*/true);
    auto cpuDists  = cpuGeo_.batchDistances(lats1.data(), lons1.data(),
                                             lats2.data(), lons2.data(),
                                             count, /*useHaversine=*/true);

    ASSERT_EQ(bestDists.size(), cpuDists.size());
    for (int i = 0; i < count; ++i) {
        EXPECT_NEAR(bestDists[i], cpuDists[i], kGeoTol)
            << "Registry best-geo-backend Haversine parity mismatch at index " << i;
    }

    best->shutdown();
}
