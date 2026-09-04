// Test: Backend Consistency Tests
// Validates that all acceleration backends produce consistent L2 distance results
// This test ensures the fix for squared L2 distance consistency works correctly

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#ifdef THEMIS_ENABLE_HIP
#include "acceleration/hip_backend.h"
#endif

#ifdef THEMIS_ENABLE_OPENCL
#include "acceleration/opencl_backend.h"
#endif

#ifdef THEMIS_ENABLE_METAL
#include "acceleration/metal_backend.h"
#endif

#ifdef THEMIS_ENABLE_VULKAN
#include "acceleration/vulkan_backend.h"
#endif

#include <vector>
#include <cmath>
#include <memory>
#include <iostream>

using namespace themis::acceleration;

class BackendConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test vectors: 4 dimensions for simplicity
        dim = 4;
        numQueries = 2;
        numVectors = 3;
        
        // Query vectors
        queryVectors = {
            1.0f, 0.0f, 0.0f, 0.0f,  // Query 0: unit vector on x-axis
            0.0f, 1.0f, 0.0f, 0.0f   // Query 1: unit vector on y-axis
        };
        
        // Database vectors
        databaseVectors = {
            1.0f, 0.0f, 0.0f, 0.0f,  // Vector 0: same as query 0
            0.0f, 1.0f, 0.0f, 0.0f,  // Vector 1: same as query 1
            0.5f, 0.5f, 0.0f, 0.0f   // Vector 2: intermediate
        };
        
        // Expected squared L2 distances
        // Query 0 vs Vector 0: (1-1)^2 + (0-0)^2 + (0-0)^2 + (0-0)^2 = 0
        // Query 0 vs Vector 1: (1-0)^2 + (0-1)^2 + (0-0)^2 + (0-0)^2 = 1 + 1 = 2
        // Query 0 vs Vector 2: (1-0.5)^2 + (0-0.5)^2 + 0 + 0 = 0.25 + 0.25 = 0.5
        // Query 1 vs Vector 0: (0-1)^2 + (1-0)^2 + 0 + 0 = 1 + 1 = 2
        // Query 1 vs Vector 1: (0-0)^2 + (1-1)^2 + 0 + 0 = 0
        // Query 1 vs Vector 2: (0-0.5)^2 + (1-0.5)^2 + 0 + 0 = 0.25 + 0.25 = 0.5
        expectedDistances = {
            0.0f, 2.0f, 0.5f,  // Query 0
            2.0f, 0.0f, 0.5f   // Query 1
        };
        
        // Initialize CPU backend (always available)
        cpuBackend = std::make_unique<CPUVectorBackend>();
        ASSERT_TRUE(cpuBackend->initialize());
    }
    
    void TearDown() override {
        if (cpuBackend) {
            cpuBackend->shutdown();
        }
    }
    
    // Helper to compare distance arrays with tolerance
    void assertDistancesMatch(const std::vector<float>& actual, 
                             const std::vector<float>& expected,
                             const std::string& backendName,
                             float tolerance = 1e-5f) {
        ASSERT_EQ(actual.size(), expected.size()) 
            << backendName << ": Size mismatch";
        
        for (size_t i = 0; i < expected.size(); ++i) {
            EXPECT_NEAR(actual[i], expected[i], tolerance)
                << backendName << ": Distance mismatch at index " << i
                << " (expected " << expected[i] << ", got " << actual[i] << ")";
        }
    }
    
    size_t dim;
    size_t numQueries;
    size_t numVectors;
    std::vector<float> queryVectors;
    std::vector<float> databaseVectors;
    std::vector<float> expectedDistances;
    std::unique_ptr<CPUVectorBackend> cpuBackend;
};

// ============================================================================
// CPU Backend Test (Baseline)
// ============================================================================

TEST_F(BackendConsistencyTest, CPU_L2_SquaredDistance) {
    std::cout << "Testing CPU backend (baseline)..." << std::endl;
    
    auto distances = cpuBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true  // useL2
    );
    
    assertDistancesMatch(distances, expectedDistances, "CPU Backend");
    
    std::cout << "  ✓ CPU backend returns squared distances as expected" << std::endl;
}

// ============================================================================
// CUDA Backend Test
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA
TEST_F(BackendConsistencyTest, CUDA_L2_SquaredDistance) {
    auto cudaBackend = std::make_unique<CUDAVectorBackend>();
    
    if (!cudaBackend->isAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_backend_not_available";
    }
    
    if (!cudaBackend->initialize()) {
        GTEST_SKIP() << "capability:cuda_backend_initialized=false;reason=cuda_backend_initialization_failed";
    }
    
    std::cout << "Testing CUDA backend..." << std::endl;
    
    auto distances = cudaBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true  // useL2
    );
    
    assertDistancesMatch(distances, expectedDistances, "CUDA Backend");
    
    std::cout << "  ✓ CUDA backend matches CPU (squared distances)" << std::endl;
    
    cudaBackend->shutdown();
}
#endif

// ============================================================================
// HIP Backend Test
// ============================================================================

#ifdef THEMIS_ENABLE_HIP
TEST_F(BackendConsistencyTest, HIP_L2_SquaredDistance) {
    auto hipBackend = std::make_unique<HIPVectorBackend>();
    
    if (!hipBackend->isAvailable()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_backend_not_available";
    }
    
    if (!hipBackend->initialize()) {
        GTEST_SKIP() << "capability:hip_backend_initialized=false;reason=hip_backend_initialization_failed";
    }
    
    std::cout << "Testing HIP backend..." << std::endl;
    
    auto distances = hipBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true  // useL2
    );
    
    assertDistancesMatch(distances, expectedDistances, "HIP Backend");
    
    std::cout << "  ✓ HIP backend matches CPU (squared distances)" << std::endl;
    
    hipBackend->shutdown();
}
#endif

// ============================================================================
// OpenCL Backend Test
// ============================================================================

#ifdef THEMIS_ENABLE_OPENCL
TEST_F(BackendConsistencyTest, OpenCL_L2_SquaredDistance) {
    auto openclBackend = std::make_unique<OpenCLVectorBackend>();
    
    if (!openclBackend->isAvailable()) {
        GTEST_SKIP() << "capability:opencl_runtime_available=false;reason=opencl_backend_not_available";
    }
    
    if (!openclBackend->initialize()) {
        GTEST_SKIP() << "capability:opencl_backend_initialized=false;reason=opencl_backend_initialization_failed";
    }
    
    std::cout << "Testing OpenCL backend..." << std::endl;
    
    auto distances = openclBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true  // useL2
    );
    
    assertDistancesMatch(distances, expectedDistances, "OpenCL Backend");
    
    std::cout << "  ✓ OpenCL backend matches CPU (squared distances)" << std::endl;
    
    openclBackend->shutdown();
}
#endif

// ============================================================================
// Metal Backend Test
// ============================================================================

#ifdef THEMIS_ENABLE_METAL
TEST_F(BackendConsistencyTest, Metal_L2_SquaredDistance) {
    auto metalBackend = std::make_unique<MetalVectorBackend>();
    
    if (!metalBackend->isAvailable()) {
        GTEST_SKIP() << "Metal backend not available";
    }
    
    if (!metalBackend->initialize()) {
        GTEST_SKIP() << "Metal backend initialization failed";
    }
    
    std::cout << "Testing Metal backend..." << std::endl;
    
    auto distances = metalBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true  // useL2
    );
    
    assertDistancesMatch(distances, expectedDistances, "Metal Backend");
    
    std::cout << "  ✓ Metal backend matches CPU (squared distances)" << std::endl;
    
    metalBackend->shutdown();
}
#endif

// ============================================================================
// Vulkan Backend Test
// ============================================================================

#ifdef THEMIS_ENABLE_VULKAN
TEST_F(BackendConsistencyTest, Vulkan_L2_SquaredDistance) {
    auto vulkanBackend = std::make_unique<VulkanBackend>();
    
    if (!vulkanBackend->isAvailable()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_backend_not_available";
    }
    
    if (!vulkanBackend->initialize()) {
        GTEST_SKIP() << "capability:vulkan_backend_initialized=false;reason=vulkan_backend_initialization_failed";
    }
    
    std::cout << "Testing Vulkan backend..." << std::endl;
    
    auto distances = vulkanBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true  // useL2
    );
    
    assertDistancesMatch(distances, expectedDistances, "Vulkan Backend");
    
    std::cout << "  ✓ Vulkan backend matches CPU (squared distances)" << std::endl;
    
    vulkanBackend->shutdown();
}
#endif

// ============================================================================
// Cross-Backend Consistency Test
// ============================================================================

TEST_F(BackendConsistencyTest, CrossBackend_Consistency) {
    std::cout << "\nCross-Backend L2 Distance Consistency Test" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Get CPU baseline
    auto cpuDistances = cpuBackend->computeDistances(
        queryVectors.data(), numQueries, dim,
        databaseVectors.data(), numVectors,
        true
    );
    
    std::cout << "CPU (baseline): " << std::flush;
    for (size_t i = 0; i < std::min(cpuDistances.size(), size_t(6)); ++i) {
        std::cout << cpuDistances[i] << " ";
    }
    std::cout << std::endl;
    
    int backendsTested = 1;
    int backendsMatched = 1;
    
    // Test all available GPU backends
#ifdef THEMIS_ENABLE_CUDA
    {
        auto backend = std::make_unique<CUDAVectorBackend>();
        if (backend->isAvailable() && backend->initialize()) {
            auto distances = backend->computeDistances(
                queryVectors.data(), numQueries, dim,
                databaseVectors.data(), numVectors, true
            );
            backendsTested++;
            std::cout << "CUDA:          " << std::flush;
            for (size_t i = 0; i < std::min(distances.size(), size_t(6)); ++i) {
                std::cout << distances[i] << " ";
            }
            std::cout << std::endl;
            
            bool matches = true;
            for (size_t i = 0; i < distances.size(); ++i) {
                if (std::abs(distances[i] - cpuDistances[i]) > 1e-5f) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
              backendsMatched++;
            }
            backend->shutdown();
        }
    }
#endif
    
#ifdef THEMIS_ENABLE_HIP
    {
        auto backend = std::make_unique<HIPVectorBackend>();
        if (backend->isAvailable() && backend->initialize()) {
            auto distances = backend->computeDistances(
                queryVectors.data(), numQueries, dim,
                databaseVectors.data(), numVectors, true
            );
            backendsTested++;
            std::cout << "HIP:           " << std::flush;
            for (size_t i = 0; i < std::min(distances.size(), size_t(6)); ++i) {
                std::cout << distances[i] << " ";
            }
            std::cout << std::endl;
            
            bool matches = true;
            for (size_t i = 0; i < distances.size(); ++i) {
                if (std::abs(distances[i] - cpuDistances[i]) > 1e-5f) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
              backendsMatched++;
            }
            backend->shutdown();
        }
    }
#endif
    
#ifdef THEMIS_ENABLE_OPENCL
    {
        auto backend = std::make_unique<OpenCLVectorBackend>();
        if (backend->isAvailable() && backend->initialize()) {
            auto distances = backend->computeDistances(
                queryVectors.data(), numQueries, dim,
                databaseVectors.data(), numVectors, true
            );
            backendsTested++;
            std::cout << "OpenCL:        " << std::flush;
            for (size_t i = 0; i < std::min(distances.size(), size_t(6)); ++i) {
                std::cout << distances[i] << " ";
            }
            std::cout << std::endl;
            
            bool matches = true;
            for (size_t i = 0; i < distances.size(); ++i) {
                if (std::abs(distances[i] - cpuDistances[i]) > 1e-5f) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
              backendsMatched++;
            }
            backend->shutdown();
        }
    }
#endif
    
#ifdef THEMIS_ENABLE_METAL
    {
        auto backend = std::make_unique<MetalVectorBackend>();
        if (backend->isAvailable() && backend->initialize()) {
            auto distances = backend->computeDistances(
                queryVectors.data(), numQueries, dim,
                databaseVectors.data(), numVectors, true
            );
            backendsTested++;
            std::cout << "Metal:         " << std::flush;
            for (size_t i = 0; i < std::min(distances.size(), size_t(6)); ++i) {
                std::cout << distances[i] << " ";
            }
            std::cout << std::endl;
            
            bool matches = true;
            for (size_t i = 0; i < distances.size(); ++i) {
                if (std::abs(distances[i] - cpuDistances[i]) > 1e-5f) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
              backendsMatched++;
            }
            backend->shutdown();
        }
    }
#endif
    
#ifdef THEMIS_ENABLE_VULKAN
    {
        auto backend = std::make_unique<VulkanBackend>();
        if (backend->isAvailable() && backend->initialize()) {
            auto distances = backend->computeDistances(
                queryVectors.data(), numQueries, dim,
                databaseVectors.data(), numVectors, true
            );
            backendsTested++;
            std::cout << "Vulkan:        " << std::flush;
            for (size_t i = 0; i < std::min(distances.size(), size_t(6)); ++i) {
                std::cout << distances[i] << " ";
            }
            std::cout << std::endl;
            
            bool matches = true;
            for (size_t i = 0; i < distances.size(); ++i) {
                if (std::abs(distances[i] - cpuDistances[i]) > 1e-5f) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
              backendsMatched++;
            }
            backend->shutdown();
        }
    }
#endif
    
    std::cout << "==========================================" << std::endl;
    std::cout << "Summary: " << backendsMatched << "/" << backendsTested 
              << " backends produce consistent results" << std::endl;
    
    // All tested backends should match
    EXPECT_EQ(backendsMatched, backendsTested) 
        << "Some backends produced inconsistent L2 distances";
}
