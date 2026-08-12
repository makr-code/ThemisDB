// Test: Vulkan Backend Specific Tests
// Validates Vulkan compute backend functionality and edge cases

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#ifdef THEMIS_ENABLE_VULKAN
#include "acceleration/vulkan_backend.h"
#endif
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

using namespace themis::acceleration;

#ifdef THEMIS_ENABLE_VULKAN

class VulkanBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ = std::make_unique<VulkanBackend>();
        
        if (!backend_->initialize() || !backend_->isAvailable()) {
            GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_backend_not_available";
        }
    }
    
    void TearDown() override {
        if (backend_) {
            backend_->shutdown();
        }
    }
    
    std::vector<float> generateRandomVectors(size_t num_vectors, size_t dim, int seed = 42) {
        std::mt19937 rng(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        std::vector<float> vectors(num_vectors * dim);
        for (auto& v : vectors) {
            v = dist(rng);
        }
        
        // Normalize
        for (size_t i = 0; i < num_vectors; i++) {
            float norm = 0.0f;
            for (size_t j = 0; j < dim; j++) {
                float val = vectors[i * dim + j];
                norm += val * val;
            }
            norm = std::sqrt(norm);
            if (norm > 0) {
                for (size_t j = 0; j < dim; j++) {
                    vectors[i * dim + j] /= norm;
                }
            }
        }
        
        return vectors;
    }
    
    std::unique_ptr<VulkanBackend> backend_;
};

// ===== Basic Functionality Tests =====

TEST_F(VulkanBackendTest, BackendIdentification) {
    EXPECT_STREQ(backend_->name(), "Vulkan");
    EXPECT_EQ(backend_->type(), BackendType::VULKAN);
    EXPECT_TRUE(backend_->isAvailable());
}

TEST_F(VulkanBackendTest, GetCapabilities) {
    auto caps = backend_->getCapabilities();
    
    EXPECT_GT(caps.maxMemoryBytes, 0);
    EXPECT_GT(caps.computeUnits, 0);
    EXPECT_FALSE(caps.deviceName.empty());
    
    // Vulkan should support basic operations
    EXPECT_TRUE(caps.supportsVectorOps);
}

// ===== Distance Computation Tests =====

TEST_F(VulkanBackendTest, SimpleDistanceComputation) {
    const size_t num_queries = 10;
    const size_t num_vectors = 100;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim, 43);
    
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true  // L2 distance
    );
    
    ASSERT_EQ(distances.size(), num_queries * num_vectors);
    
    // All distances should be non-negative
    for (float dist : distances) {
        EXPECT_GE(dist, 0.0f);
    }
}

TEST_F(VulkanBackendTest, DistanceComputationL2vsIP) {
    const size_t num_queries = 5;
    const size_t num_vectors = 50;
    const size_t dim = 64;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim, 43);
    
    // L2 distance
    auto distances_l2 = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    // Inner product
    auto distances_ip = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        false
    );
    
    ASSERT_EQ(distances_l2.size(), distances_ip.size());
    
    // Distances should be different (unless vectors are identical)
    bool has_difference = false;
    for (size_t i = 0; i < distances_l2.size(); i++) {
        if (std::abs(distances_l2[i] - distances_ip[i]) > 0.001f) {
            has_difference = true;
            break;
        }
    }
    EXPECT_TRUE(has_difference);
}

// ===== Edge Case Tests =====

TEST_F(VulkanBackendTest, SingleQueryVector) {
    const size_t num_queries = 1;
    const size_t num_vectors = 100;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    EXPECT_EQ(distances.size(), num_vectors);
}

TEST_F(VulkanBackendTest, SingleDatabaseVector) {
    const size_t num_queries = 10;
    const size_t num_vectors = 1;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    EXPECT_EQ(distances.size(), num_queries);
}

TEST_F(VulkanBackendTest, LargeDimension) {
    const size_t num_queries = 10;
    const size_t num_vectors = 100;
    const size_t dim = 2048;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    EXPECT_EQ(distances.size(), num_queries * num_vectors);
}

TEST_F(VulkanBackendTest, SmallDimension) {
    const size_t num_queries = 10;
    const size_t num_vectors = 100;
    const size_t dim = 16;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    EXPECT_EQ(distances.size(), num_queries * num_vectors);
}

// ===== Large Scale Tests =====

TEST_F(VulkanBackendTest, LargeScaleComputation) {
    const size_t num_queries = 1000;
    const size_t num_vectors = 10000;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    EXPECT_EQ(distances.size(), num_queries * num_vectors);
}

// ===== Correctness Tests =====

TEST_F(VulkanBackendTest, IdenticalVectorsHaveZeroDistance) {
    const size_t num_queries = 1;
    const size_t num_vectors = 1;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    
    // Use same vector for both query and database
    auto distances = backend_->computeDistances(
        queries.data(), num_queries, dim,
        queries.data(), num_vectors,
        true
    );
    
    ASSERT_EQ(distances.size(), 1);
    EXPECT_NEAR(distances[0], 0.0f, 0.001f);
}

TEST_F(VulkanBackendTest, OrthogonalVectorsMaximumDistance) {
    const size_t dim = 4;
    
    std::vector<float> query = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> vector = {0.0f, 1.0f, 0.0f, 0.0f};
    
    auto distances = backend_->computeDistances(
        query.data(), 1, dim,
        vector.data(), 1,
        true
    );
    
    ASSERT_EQ(distances.size(), 1);
    // Squared L2 distance between orthogonal unit vectors: (1-0)^2 + (0-1)^2 = 2
    EXPECT_NEAR(distances[0], 2.0f, 0.01f);
}

// ===== Memory Management Tests =====

TEST_F(VulkanBackendTest, MultipleComputations) {
    const size_t num_queries = 10;
    const size_t num_vectors = 100;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    // Perform multiple computations to test memory reuse
    for (int i = 0; i < 5; i++) {
        auto distances = backend_->computeDistances(
            queries.data(), num_queries, dim,
            vectors.data(), num_vectors,
            true
        );
        
        EXPECT_EQ(distances.size(), num_queries * num_vectors);
    }
}

// ===== Initialization and Shutdown Tests =====

TEST_F(VulkanBackendTest, MultipleInitShutdownCycles) {
    // Test that backend can be initialized and shut down multiple times
    for (int i = 0; i < 3; i++) {
        backend_->shutdown();
        EXPECT_TRUE(backend_->initialize());
        EXPECT_TRUE(backend_->isAvailable());
    }
}

// ===== Performance Consistency Tests =====

TEST_F(VulkanBackendTest, ConsistentResults) {
    const size_t num_queries = 10;
    const size_t num_vectors = 100;
    const size_t dim = 128;
    
    auto queries = generateRandomVectors(num_queries, dim);
    auto vectors = generateRandomVectors(num_vectors, dim);
    
    // Compute distances multiple times
    auto distances1 = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    auto distances2 = backend_->computeDistances(
        queries.data(), num_queries, dim,
        vectors.data(), num_vectors,
        true
    );
    
    ASSERT_EQ(distances1.size(), distances2.size());
    
    // Results should be identical (or very close due to floating point)
    for (size_t i = 0; i < distances1.size(); i++) {
        EXPECT_NEAR(distances1[i], distances2[i], 0.0001f);
    }
}

// ===== Non-NVIDIA Fallback Tests =====

// Verify that getCapabilities() populates vendorName after initialization.
// This validates the vendor detection path used for non-NVIDIA hardware.
TEST_F(VulkanBackendTest, CapabilitiesReportVendorName) {
    auto caps = backend_->getCapabilities();
    EXPECT_FALSE(caps.vendorName.empty())
        << "vendorName must be populated after initialization";
    // Device name must also be non-empty
    EXPECT_FALSE(caps.deviceName.empty())
        << "deviceName must be populated after initialization";
}

// When the backend has been successfully initialized, the reported vendor
// must be one of the recognized strings (not the raw "Unknown" fallback
// on any real GPU) or "Unknown" for virtual/software devices.
TEST_F(VulkanBackendTest, CapabilitiesVendorNameIsKnown) {
    auto caps = backend_->getCapabilities();
    static const std::vector<std::string> kKnownVendors = {
        "NVIDIA", "AMD", "Intel", "ARM", "Qualcomm", "ImgTec", "Unknown"
    };
    bool found = false;
    for (const auto& v : kKnownVendors) {
        if (caps.vendorName == v) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Unexpected vendorName: " << caps.vendorName;
}

// Confirm that the backend fallback chain includes VULKAN after CUDA/HIP,
// meaning non-NVIDIA users will transparently receive Vulkan acceleration.
TEST(VulkanFallbackOrder, VulkanInFallbackChain) {
    const auto& order = BackendRegistry::getFallbackOrder();
    auto cudaIt   = std::find(order.begin(), order.end(), BackendType::CUDA);
    auto vulkanIt = std::find(order.begin(), order.end(), BackendType::VULKAN);
    auto cpuIt    = std::find(order.begin(), order.end(), BackendType::CPU);

    ASSERT_NE(vulkanIt, order.end()) << "VULKAN must be present in the fallback chain";
    ASSERT_NE(cpuIt, order.end())    << "CPU must be present in the fallback chain";
    // Vulkan must come before CPU (fallback to CPU only when no GPU available)
    EXPECT_LT(vulkanIt, cpuIt)
        << "VULKAN must appear before CPU in the fallback chain";
    // Vulkan must come after CUDA (CUDA is preferred for NVIDIA users)
    if (cudaIt != order.end()) {
        EXPECT_LT(cudaIt, vulkanIt)
            << "CUDA must appear before VULKAN (NVIDIA users get CUDA first)";
    }
}

#else

TEST(VulkanBackendTest, VulkanNotCompiled) {
    GTEST_SKIP() << "capability:vulkan_compiled=false;reason=vulkan_backend_not_compiled";
}

// Even without Vulkan SDK, the fallback chain must include VULKAN entry.
TEST(VulkanFallbackOrder, VulkanInFallbackChain) {
    const auto& order = BackendRegistry::getFallbackOrder();
    auto vulkanIt = std::find(order.begin(), order.end(), BackendType::VULKAN);
    auto cpuIt    = std::find(order.begin(), order.end(), BackendType::CPU);

    ASSERT_NE(vulkanIt, order.end()) << "VULKAN must be present in the fallback chain";
    ASSERT_NE(cpuIt, order.end())    << "CPU must be present in the fallback chain";
    EXPECT_LT(vulkanIt, cpuIt)
        << "VULKAN must appear before CPU in the fallback chain";
}

#endif // THEMIS_ENABLE_VULKAN
