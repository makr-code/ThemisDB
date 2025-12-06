#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/config_loader.h"
#include <vector>
#include <cmath>

using namespace themis::acceleration;

class AccelerationConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register CPU backend (always available)
        auto cpuVector = std::make_unique<CPUVectorBackend>();
        BackendRegistry::instance().registerBackend(std::move(cpuVector));
        
        auto cpuGraph = std::make_unique<CPUGraphBackend>();
        BackendRegistry::instance().registerBackend(std::move(cpuGraph));
        
        auto cpuGeo = std::make_unique<CPUGeoBackend>();
        BackendRegistry::instance().registerBackend(std::move(cpuGeo));
    }
    
    void TearDown() override {
        BackendRegistry::instance().shutdownAll();
    }
};

TEST_F(AccelerationConfigTest, DefaultConfig) {
    auto& registry = BackendRegistry::instance();
    AccelerationConfig config = registry.getConfig();
    
    // Default should be AUTO preference with GPU fallback enabled
    EXPECT_EQ(config.prefer, AccelerationPreference::AUTO);
    EXPECT_TRUE(config.gpuFallback);
    EXPECT_EQ(config.minBatchSize, 1000);
}

TEST_F(AccelerationConfigTest, ConfigurePreferenceCPU) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.prefer = AccelerationPreference::CPU;
    registry.configure(config);
    
    // Should always return CPU backend
    auto* backend = registry.getBestVectorBackend(10000);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->type(), BackendType::CPU);
}

TEST_F(AccelerationConfigTest, ConfigurePreferenceAuto) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.prefer = AccelerationPreference::AUTO;
    registry.configure(config);
    
    // Should return best available (CPU in test environment)
    auto* backend = registry.getBestVectorBackend(10000);
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->isAvailable());
}

TEST_F(AccelerationConfigTest, BatchSizeThreshold) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.prefer = AccelerationPreference::AUTO;
    config.minBatchSize = 1000;
    registry.configure(config);
    
    // Small batch should use CPU
    auto* backendSmall = registry.getBestVectorBackend(100);
    ASSERT_NE(backendSmall, nullptr);
    EXPECT_EQ(backendSmall->type(), BackendType::CPU);
    
    // Large batch should use best available (CPU in test env)
    auto* backendLarge = registry.getBestVectorBackend(5000);
    ASSERT_NE(backendLarge, nullptr);
    EXPECT_TRUE(backendLarge->isAvailable());
}

TEST_F(AccelerationConfigTest, GPUFallbackEnabled) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.prefer = AccelerationPreference::GPU;
    config.gpuFallback = true;
    registry.configure(config);
    
    // Should fall back to CPU when GPU not available
    auto* backend = registry.getBestVectorBackend(10000);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->type(), BackendType::CPU);
}

TEST_F(AccelerationConfigTest, GPUFallbackDisabled) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.prefer = AccelerationPreference::GPU;
    config.gpuFallback = false;
    registry.configure(config);
    
    // Should return nullptr when GPU not available and fallback disabled
    auto* backend = registry.getBestVectorBackend(10000);
    EXPECT_EQ(backend, nullptr);
}

TEST_F(AccelerationConfigTest, HasGPUBackend) {
    auto& registry = BackendRegistry::instance();
    
    // In CPU-only test environment, should return false
    EXPECT_FALSE(registry.hasGPUBackend());
}

TEST_F(AccelerationConfigTest, ConfigLoaderDefault) {
    // Load default configuration
    AccelerationConfig config = ConfigLoader::loadDefault();
    
    // Should return valid defaults
    EXPECT_EQ(config.prefer, AccelerationPreference::AUTO);
    EXPECT_TRUE(config.gpuFallback);
    EXPECT_GE(config.minBatchSize, 0);
}

TEST_F(AccelerationConfigTest, VectorBackendWithBatchSize) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend(500);
    
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->initialize());
    
    // Test data: 3 vectors in 2D space
    std::vector<float> vectors = {
        1.0f, 0.0f,  // Vector 0
        0.0f, 1.0f,  // Vector 1
        1.0f, 1.0f   // Vector 2
    };
    
    // Query vector
    std::vector<float> query = {0.5f, 0.5f};
    
    // Compute distances
    auto distances = backend->computeDistances(
        query.data(), 1, 2,
        vectors.data(), 3,
        true  // Use L2
    );
    
    ASSERT_EQ(distances.size(), 3);
    
    // Verify distances are reasonable
    for (float dist : distances) {
        EXPECT_GE(dist, 0.0f);
        EXPECT_LT(dist, 10.0f);
    }
    
    backend->shutdown();
}

TEST_F(AccelerationConfigTest, GraphBackendWithBatchSize) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.minBatchSize = 100;
    registry.configure(config);
    
    // Small batch
    auto* backendSmall = registry.getBestGraphBackend(50);
    ASSERT_NE(backendSmall, nullptr);
    EXPECT_EQ(backendSmall->type(), BackendType::CPU);
    
    // Large batch
    auto* backendLarge = registry.getBestGraphBackend(500);
    ASSERT_NE(backendLarge, nullptr);
}

TEST_F(AccelerationConfigTest, GeoBackendWithBatchSize) {
    auto& registry = BackendRegistry::instance();
    
    AccelerationConfig config;
    config.minBatchSize = 100;
    registry.configure(config);
    
    // Small batch
    auto* backendSmall = registry.getBestGeoBackend(50);
    ASSERT_NE(backendSmall, nullptr);
    EXPECT_EQ(backendSmall->type(), BackendType::CPU);
    
    // Large batch
    auto* backendLarge = registry.getBestGeoBackend(500);
    ASSERT_NE(backendLarge, nullptr);
}
