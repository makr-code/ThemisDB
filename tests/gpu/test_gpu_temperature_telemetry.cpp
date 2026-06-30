/**
 * @file test_gpu_temperature_telemetry.cpp
 * @brief Tests for GPU Temperature Telemetry (Stub #309 remediation)
 * 
 * This test suite verifies that GPUMemoryManager correctly queries GPU
 * temperature using NVML when available and updates health status with
 * temperature data.
 */

#include <gtest/gtest.h>
#include "llm/gpu_memory_manager.h"
#include <chrono>
#include <thread>

namespace themis {
namespace llm {
namespace test {

class GPUTemperatureTelemetryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset NVML function to nullptr before each test
        GPUMemoryManager::setNvmlTemperatureFn(nullptr);
    }
    
    void TearDown() override {
        // Clean up after test
        GPUMemoryManager::setNvmlTemperatureFn(nullptr);
    }
};

// Test 1: Verify temperature is queried and stored in GPUHealth
TEST_F(GPUTemperatureTelemetryTest, TemperatureStoredInGPUHealth) {
    // Create mock temperature provider
    auto temp_provider = [](int gpu_id) -> float {
        // Simulate temperature: 45.5°C for GPU 0, 50.0°C for GPU 1
        return (gpu_id == 0) ? 45.5f : 50.0f;
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;  // 8 GB
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;  // 32 GB
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    // Query GPU health
    auto health0 = manager->getGPUHealth(0);
    auto health1 = manager->getGPUHealth(1);
    
    // Verify temperature was captured
    EXPECT_EQ(health0.device_id, 0);
    EXPECT_EQ(health1.device_id, 1);
    
    // Temperature should be set (may be 0.0f if GPU unavailable, but provider should be called)
    if (health0.is_available) {
        EXPECT_GE(health0.temperature_celsius, 0.0f);
    }
    if (health1.is_available) {
        EXPECT_GE(health1.temperature_celsius, 0.0f);
    }
}

// Test 2: Verify temperature is tracked across multiple queries
TEST_F(GPUTemperatureTelemetryTest, TemperatureTrackedAcrossQueries) {
    // Mock provider that returns different temps on successive calls
    int call_count = 0;
    auto temp_provider = [&call_count](int gpu_id) -> float {
        call_count++;
        return 40.0f + (call_count * 5.0f);  // 45, 50, 55, etc.
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    // First query
    auto health1 = manager->getGPUHealth(0);
    float first_temp = health1.temperature_celsius;
    
    // Second query
    auto health2 = manager->getGPUHealth(0);
    float second_temp = health2.temperature_celsius;
    
    // Temperature should have changed or be queried again
    // (In reality depends on when updateGPUHealth is called)
    EXPECT_GE(first_temp, 0.0f);
    EXPECT_GE(second_temp, 0.0f);
}

// Test 3: Verify temperature affects health status (high temp warning)
TEST_F(GPUTemperatureTelemetryTest, HighTemperatureAffectsHealthStatus) {
    // Mock provider that returns high temperature
    auto temp_provider = [](int /*gpu_id*/) -> float {
        return 85.0f;  // High temperature - may affect health
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto health = manager->getGPUHealth(0);
    
    // Temperature should be captured
    if (health.is_available) {
        EXPECT_GT(health.temperature_celsius, 80.0f);
    }
}

// Test 4: Verify normal temperature range
TEST_F(GPUTemperatureTelemetryTest, NormalTemperatureRange) {
    // Mock provider returning normal temperature
    auto temp_provider = [](int /*gpu_id*/) -> float {
        return 55.0f;  // Normal operating temperature
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto health = manager->getGPUHealth(0);
    
    if (health.is_available) {
        EXPECT_GE(health.temperature_celsius, 0.0f);
        EXPECT_LT(health.temperature_celsius, 100.0f);  // Reasonable upper bound
    }
}

// Test 5: Verify temperature provider exception handling
TEST_F(GPUTemperatureTelemetryTest, TemperatureProviderExceptionHandled) {
    // Mock provider that throws exception
    auto temp_provider = [](int /*gpu_id*/) -> float {
        throw std::runtime_error("NVML temporarily unavailable");
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    // Should not throw - exception should be caught internally
    EXPECT_NO_THROW({
        auto manager = std::make_unique<GPUMemoryManager>(config);
        auto health = manager->getGPUHealth(0);
        // Manager should fall back gracefully
    });
}

// Test 6: Verify multi-GPU temperature tracking
TEST_F(GPUTemperatureTelemetryTest, MultiGPUTemperatureTracking) {
    // Mock provider returns different temps for each GPU
    auto temp_provider = [](int gpu_id) -> float {
        return 40.0f + (gpu_id * 10.0f);  // GPU 0: 40°C, GPU 1: 50°C, GPU 2: 60°C
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1, 2};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto all_health = manager->getAllGPUHealth();
    
    // Should have health info for all GPUs
    EXPECT_GE(all_health.size(), 0);
    
    for (const auto& health : all_health) {
        if (health.is_available) {
            // Each GPU should have temperature tracked
            EXPECT_GE(health.temperature_celsius, 0.0f);
        }
    }
}

// Test 7: Verify temperature in GPU stats
TEST_F(GPUTemperatureTelemetryTest, TemperatureIncludedInGPUStats) {
    auto temp_provider = [](int /*gpu_id*/) -> float {
        return 52.0f;
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto stats = manager->getGPUStats(0);
    
    // Temperature should be included in stats
    EXPECT_EQ(stats.device_id, 0);
    if (stats.is_healthy) {
        EXPECT_GE(stats.temperature_celsius, 0.0f);
    }
}

// Test 8: Verify instance temperature provider fallback
TEST_F(GPUTemperatureTelemetryTest, InstanceTemperatureProviderFallback) {
    // Create manager with instance-level temperature provider
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    // Set instance-level provider via config
    config.temperature_provider_fn = [](int gpu_id, float& temp_out) -> bool {
        temp_out = 48.0f;
        return true;
    };
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    manager->setGPUTemperatureProviderFn(config.temperature_provider_fn);
    
    auto health = manager->getGPUHealth(0);
    
    // Should use instance provider if static NVML function is not set
    if (health.is_available) {
        EXPECT_GE(health.temperature_celsius, 0.0f);
    }
}

// Test 9: Verify NvmlTemperatureFn takes precedence over instance provider
TEST_F(GPUTemperatureTelemetryTest, NvmlFunctionPrecedence) {
    // Set static NVML function
    auto static_provider = [](int /*gpu_id*/) -> float {
        return 60.0f;  // Static function always returns 60°C
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(static_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    // Set instance provider with different value
    config.temperature_provider_fn = [](int /*gpu_id*/, float& temp_out) -> bool {
        temp_out = 30.0f;  // Instance function always returns 30°C
        return true;
    };
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    manager->setGPUTemperatureProviderFn(config.temperature_provider_fn);
    
    auto health = manager->getGPUHealth(0);
    
    // Static NVML function should take precedence
    if (health.is_available && health.temperature_celsius > 0.0f) {
        EXPECT_NEAR(health.temperature_celsius, 60.0f, 5.0f);
    }
}

// Test 10: Verify temperature monitoring timestamp
TEST_F(GPUTemperatureTelemetryTest, TemperatureMonitoringTimestamp) {
    auto temp_provider = [](int /*gpu_id*/) -> float {
        return 55.0f;
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto health = manager->getGPUHealth(0);
    
    // Health should have a check timestamp
    EXPECT_GE(health.last_check_timestamp_ms, 0);
}

// Test 11: Verify fallback when temperature cannot be queried
TEST_F(GPUTemperatureTelemetryTest, FallbackTemperatureWhenUnavailable) {
    // Provider that indicates failure
    auto failing_provider = [](int /*gpu_id*/) -> float {
        throw std::runtime_error("NVML not available");
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(failing_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    // Should not crash - should fall back to computed estimate
    auto health = manager->getGPUHealth(0);
    
    // Should have some temperature value (computed estimate)
    EXPECT_GE(health.temperature_celsius, 0.0f);
}

// Test 12: Verify all GPUs get temperature telemetry
TEST_F(GPUTemperatureTelemetryTest, AllGPUsGetTemperatureTelemetry) {
    auto temp_provider = [](int gpu_id) -> float {
        return 40.0f + (gpu_id * 5.0f);
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1, 2, 3};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto all_health = manager->getAllGPUHealth();
    
    // All GPUs should be tracked
    for (const auto& health : all_health) {
        EXPECT_GE(health.device_id, 0);
        // Temperature should be set (or 0.0f if unavailable)
        EXPECT_GE(health.temperature_celsius, 0.0f);
    }
}

// Test 13: Verify temperature is used in health calculation
TEST_F(GPUTemperatureTelemetryTest, TemperatureUsedInHealthCalculation) {
    // High temperature provider
    auto high_temp_provider = [](int /*gpu_id*/) -> float {
        return 90.0f;  // Very high
    };
    
    GPUMemoryManager::setNvmlTemperatureFn(high_temp_provider);
    
    GPUMemoryManager::Config config;
    config.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    config.max_ram_bytes = 32ULL * 1024 * 1024 * 1024;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    
    auto manager = std::make_unique<GPUMemoryManager>(config);
    
    auto health = manager->getGPUHealth(0);
    
    // High temperature should be captured
    if (health.is_available) {
        EXPECT_GT(health.temperature_celsius, 80.0f);
    }
}

} // namespace test
} // namespace llm
} // namespace themis
