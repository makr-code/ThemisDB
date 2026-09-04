// Test: Vulkan Backend Health Check Tests  (Phase 3.3)
// Validates BackendHealthStatus for VulkanVectorBackend and the default
// implementation inherited by CPUVectorBackend.

#include <gtest/gtest.h>
#include "acceleration/vulkan_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/compute_backend.h"

using namespace themis::acceleration;

// ============================================================================
// BackendHealthStatus struct tests
// ============================================================================

TEST(BackendHealthStatus, MakeHealthy) {
    auto s = BackendHealthStatus::makeHealthy("TestGPU");
    EXPECT_EQ(s.status, "healthy");
    EXPECT_TRUE(s.healthy);
    EXPECT_TRUE(s.ready);
    EXPECT_TRUE(s.alive);
    EXPECT_EQ(s.deviceName, "TestGPU");
    EXPECT_TRUE(s.issues.empty());
}

TEST(BackendHealthStatus, MakeDegraded) {
    auto s = BackendHealthStatus::makeDegraded("shader missing");
    EXPECT_EQ(s.status, "degraded");
    EXPECT_FALSE(s.healthy);
    EXPECT_FALSE(s.ready);
    EXPECT_TRUE(s.alive);
    EXPECT_EQ(s.issues.size(), 1u);
    EXPECT_EQ(s.issues[0], "shader missing");
}

TEST(BackendHealthStatus, MakeUnhealthy) {
    auto s = BackendHealthStatus::makeUnhealthy("no ICD");
    EXPECT_EQ(s.status, "unhealthy");
    EXPECT_FALSE(s.healthy);
    EXPECT_FALSE(s.ready);
    EXPECT_FALSE(s.alive);
    EXPECT_EQ(s.issues.size(), 1u);
}

// ============================================================================
// Default IComputeBackend::getHealthStatus() via CPUVectorBackend
// ============================================================================

class HealthCheckDefaultTest : public ::testing::Test {};

TEST_F(HealthCheckDefaultTest, CPU_HealthyAfterInit) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    auto health = backend.getHealthStatus();
    EXPECT_EQ(health.status, "healthy");
    EXPECT_TRUE(health.healthy);
    EXPECT_TRUE(health.ready);
    EXPECT_TRUE(health.alive);
    EXPECT_TRUE(health.issues.empty());
}

TEST_F(HealthCheckDefaultTest, CPU_IsAlwaysAvailable) {
    CPUVectorBackend backend;
    // CPU backend is always available; health before init is degraded but alive
    auto health = backend.getHealthStatus();
    // isAvailable() returns true for CPU, no last error yet → healthy
    EXPECT_EQ(health.status, "healthy");
    EXPECT_TRUE(health.alive);
}

// ============================================================================
// VulkanVectorBackend::getHealthStatus()
// ============================================================================

class VulkanHealthTest : public ::testing::Test {};

TEST_F(VulkanHealthTest, UnhealthyWhenVulkanNotCompiled) {
#ifdef THEMIS_ENABLE_VULKAN
    GTEST_SKIP() << "capability:vulkan_unhealthy_path_applicable=false;reason=vulkan_compiled_in";
#else
    VulkanBackend backend;
    auto health = backend.getHealthStatus();
    EXPECT_EQ(health.status, "unhealthy");
    EXPECT_FALSE(health.healthy);
    EXPECT_FALSE(health.ready);
    EXPECT_FALSE(health.alive);
    EXPECT_FALSE(health.issues.empty());
    EXPECT_NE(health.message.find("THEMIS_ENABLE_VULKAN"), std::string::npos);
#endif
}

TEST_F(VulkanHealthTest, DegradedBeforeInit_WhenVulkanAvailable) {
    VulkanBackend backend = {};
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }

    // Not yet initialised — should be alive but degraded/not-ready
    auto health = backend.getHealthStatus();
    EXPECT_TRUE(health.alive);
    EXPECT_FALSE(health.ready);
    EXPECT_NE(health.status, "healthy");
}

TEST_F(VulkanHealthTest, HealthyAfterSuccessfulInit) {
    VulkanBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available_or_initialization_failed";
    }

    auto health = backend.getHealthStatus();
    EXPECT_TRUE(health.alive);
    // ready iff both compute pipelines are loaded
    if (health.ready) {
        EXPECT_EQ(health.status, "healthy");
        EXPECT_TRUE(health.healthy);
        EXPECT_FALSE(health.deviceName.empty());
    } else {
        // Pipelines missing (shaders not compiled) → degraded
        EXPECT_EQ(health.status, "degraded");
    }

    backend.shutdown();
}

TEST_F(VulkanHealthTest, DegradedAfterShutdown) {
    VulkanBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }

    backend.shutdown();

    auto health = backend.getHealthStatus();
    // After shutdown impl_ is cleaned up; not ready
    EXPECT_FALSE(health.ready);
}

TEST_F(VulkanHealthTest, MemoryFieldsPopulatedWhenInitialized) {
    VulkanBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }

    auto health = backend.getHealthStatus();
    // Device-local heap size should be > 0 on any Vulkan device
    EXPECT_GT(health.memoryAvailableBytes, 0u);

    backend.shutdown();
}

TEST_F(VulkanHealthTest, DriverInfoPresentWhenInitialized) {
    VulkanBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }

    auto health = backend.getHealthStatus();
    EXPECT_FALSE(health.driverInfo.empty());
    EXPECT_NE(health.driverInfo.find("Vulkan"), std::string::npos);

    backend.shutdown();
}
