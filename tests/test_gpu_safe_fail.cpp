#include <gtest/gtest.h>
#include "llm/gpu_safe_fail.h"
#include <thread>
#include <chrono>

using namespace themis::llm;

class GPUSafeFailTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration for tests
        config_ = GPUSafeFailManager::Config();
        config_.failure_threshold = 3;
        config_.success_threshold = 2;
        config_.circuit_reset_timeout = std::chrono::seconds(2);
        config_.enable_cpu_fallback = true;
    }
    
    GPUSafeFailManager::Config config_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(GPUSafeFailTest, InitialState) {
    GPUSafeFailManager manager(config_);
    
    EXPECT_TRUE(manager.isHealthy());
    EXPECT_TRUE(manager.shouldAttemptGPU());
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.state, GPUSafeFailManager::GPUState::HEALTHY);
    EXPECT_EQ(status.consecutive_failures, 0);
    EXPECT_EQ(status.consecutive_successes, 0);
    EXPECT_EQ(status.total_failures, 0);
    EXPECT_EQ(status.total_operations, 0);
    EXPECT_FALSE(status.is_cpu_fallback_active);
}

TEST_F(GPUSafeFailTest, RecordSuccess) {
    GPUSafeFailManager manager(config_);
    
    manager.recordSuccess();
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.consecutive_successes, 1);
    EXPECT_EQ(status.consecutive_failures, 0);
    EXPECT_TRUE(manager.isHealthy());
}

TEST_F(GPUSafeFailTest, RecordFailure) {
    GPUSafeFailManager manager(config_);
    
    manager.recordFailure(
        GPUSafeFailManager::FailureType::DEVICE_ERROR,
        "Test failure"
    );
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.consecutive_failures, 1);
    EXPECT_EQ(status.consecutive_successes, 0);
    EXPECT_EQ(status.total_failures, 1);
    EXPECT_EQ(status.last_error_message, "Test failure");
}

// ============================================================================
// State Transition Tests
// ============================================================================

TEST_F(GPUSafeFailTest, HealthyToDegraded) {
    GPUSafeFailManager manager(config_);
    
    // One failure should move to degraded
    manager.recordFailure(
        GPUSafeFailManager::FailureType::MEMORY_ERROR,
        "OOM"
    );
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.state, GPUSafeFailManager::GPUState::DEGRADED);
    EXPECT_TRUE(manager.isHealthy());  // Degraded is still considered healthy
}

TEST_F(GPUSafeFailTest, DegradedToCircuitOpen) {
    GPUSafeFailManager manager(config_);
    
    // Record failures up to threshold
    for (size_t i = 0; i < config_.failure_threshold; ++i) {
        manager.recordFailure(
            GPUSafeFailManager::FailureType::KERNEL_ERROR,
            "Kernel failed"
        );
    }
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.state, GPUSafeFailManager::GPUState::CIRCUIT_OPEN);
    EXPECT_FALSE(manager.shouldAttemptGPU());
    EXPECT_FALSE(manager.isHealthy());
}

TEST_F(GPUSafeFailTest, DegradedToHealthy) {
    GPUSafeFailManager manager(config_);
    
    // Move to degraded
    manager.recordFailure(
        GPUSafeFailManager::FailureType::TIMEOUT,
        "Timeout"
    );
    EXPECT_EQ(manager.getHealthStatus().state, GPUSafeFailManager::GPUState::DEGRADED);
    
    // Record enough successes to recover
    for (size_t i = 0; i < config_.success_threshold; ++i) {
        manager.recordSuccess();
    }
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.state, GPUSafeFailManager::GPUState::HEALTHY);
}

// ============================================================================
// Circuit Breaker Tests
// ============================================================================

TEST_F(GPUSafeFailTest, CircuitBreakerOpens) {
    GPUSafeFailManager manager(config_);
    
    // Trigger circuit breaker
    for (size_t i = 0; i < config_.failure_threshold; ++i) {
        manager.recordFailure(
            GPUSafeFailManager::FailureType::DEVICE_ERROR,
            "Device error"
        );
    }
    
    EXPECT_FALSE(manager.shouldAttemptGPU());
    EXPECT_FALSE(manager.canResetCircuit());  // Too soon
}

TEST_F(GPUSafeFailTest, CircuitBreakerReset) {
    GPUSafeFailManager manager(config_);
    
    // Open circuit
    for (size_t i = 0; i < config_.failure_threshold; ++i) {
        manager.recordFailure(
            GPUSafeFailManager::FailureType::DEVICE_ERROR,
            "Device error"
        );
    }
    
    EXPECT_EQ(manager.getHealthStatus().state, GPUSafeFailManager::GPUState::CIRCUIT_OPEN);
    
    // Wait for reset timeout
    std::this_thread::sleep_for(config_.circuit_reset_timeout + std::chrono::seconds(1));
    
    EXPECT_TRUE(manager.canResetCircuit());
    
    manager.tryResetCircuit();
    
    // Should transition to degraded for testing
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.state, GPUSafeFailManager::GPUState::DEGRADED);
    EXPECT_TRUE(manager.shouldAttemptGPU());
}

// ============================================================================
// Fallback Mechanism Tests
// ============================================================================

TEST_F(GPUSafeFailTest, SuccessfulGPUOperation) {
    GPUSafeFailManager manager(config_);
    
    bool gpu_executed = false;
    bool cpu_executed = false;
    
    auto gpu_op = [&gpu_executed]() {
        gpu_executed = true;
        return true;
    };
    
    auto cpu_op = [&cpu_executed]() {
        cpu_executed = true;
        return true;
    };
    
    bool result = manager.executeWithFallback(gpu_op, cpu_op, "test_operation");
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(gpu_executed);
    EXPECT_FALSE(cpu_executed);
    EXPECT_FALSE(manager.getHealthStatus().is_cpu_fallback_active);
}

TEST_F(GPUSafeFailTest, GPUFailureFallbackToCPU) {
    GPUSafeFailManager manager(config_);
    
    bool gpu_executed = false;
    bool cpu_executed = false;
    
    auto gpu_op = [&gpu_executed]() {
        gpu_executed = true;
        return false;  // Simulate GPU failure
    };
    
    auto cpu_op = [&cpu_executed]() {
        cpu_executed = true;
        return true;
    };
    
    bool result = manager.executeWithFallback(gpu_op, cpu_op, "test_operation");
    
    EXPECT_TRUE(result);  // Overall operation succeeded via CPU
    EXPECT_TRUE(gpu_executed);
    EXPECT_TRUE(cpu_executed);
    EXPECT_TRUE(manager.getHealthStatus().is_cpu_fallback_active);
}

TEST_F(GPUSafeFailTest, CircuitOpenUsesCPUDirectly) {
    GPUSafeFailManager manager(config_);
    
    // Open circuit
    for (size_t i = 0; i < config_.failure_threshold; ++i) {
        manager.recordFailure(
            GPUSafeFailManager::FailureType::DEVICE_ERROR,
            "Device error"
        );
    }
    
    bool gpu_executed = false;
    bool cpu_executed = false;
    
    auto gpu_op = [&gpu_executed]() {
        gpu_executed = true;
        return true;
    };
    
    auto cpu_op = [&cpu_executed]() {
        cpu_executed = true;
        return true;
    };
    
    bool result = manager.executeWithFallback(gpu_op, cpu_op, "test_operation");
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(gpu_executed);  // GPU not attempted when circuit is open
    EXPECT_TRUE(cpu_executed);
}

TEST_F(GPUSafeFailTest, NoFallbackFails) {
    GPUSafeFailManager manager(config_);
    
    // Open circuit
    for (size_t i = 0; i < config_.failure_threshold; ++i) {
        manager.recordFailure(
            GPUSafeFailManager::FailureType::DEVICE_ERROR,
            "Device error"
        );
    }
    
    bool gpu_executed = false;
    
    auto gpu_op = [&gpu_executed]() {
        gpu_executed = true;
        return true;
    };
    
    // No CPU fallback provided
    bool result = manager.executeWithFallback(gpu_op, nullptr, "test_operation");
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(gpu_executed);
}

// ============================================================================
// Error Rate Tests
// ============================================================================

TEST_F(GPUSafeFailTest, ErrorRateCalculation) {
    GPUSafeFailManager manager(config_);
    
    // Execute 10 operations: 7 success, 3 failures
    for (int i = 0; i < 7; ++i) {
        auto op = []() { return true; };
        manager.executeWithFallback(op, nullptr, "success");
    }
    
    for (int i = 0; i < 3; ++i) {
        auto op = []() { return false; };
        manager.executeWithFallback(op, nullptr, "failure");
    }
    
    float error_rate = manager.getErrorRate();
    EXPECT_NEAR(error_rate, 0.3f, 0.01f);  // 30% error rate
}

// ============================================================================
// Memory Check Tests
// ============================================================================

TEST_F(GPUSafeFailTest, MemoryAvailableCheck) {
    GPUSafeFailManager manager(config_);
    
    size_t total = 1000 * 1024 * 1024;  // 1000 MB
    size_t available = 500 * 1024 * 1024;  // 500 MB available
    size_t request = 100 * 1024 * 1024;  // Request 100 MB
    
    EXPECT_TRUE(manager.checkMemoryAvailable(request, available));
}

TEST_F(GPUSafeFailTest, MemoryInsufficientCheck) {
    GPUSafeFailManager manager(config_);
    
    size_t available = 50 * 1024 * 1024;   // 50 MB available
    size_t request = 100 * 1024 * 1024;    // Request 100 MB
    
    EXPECT_FALSE(manager.checkMemoryAvailable(request, available));
}

// ============================================================================
// Force State Tests
// ============================================================================

TEST_F(GPUSafeFailTest, ForceHealthy) {
    GPUSafeFailManager manager(config_);
    
    // Open circuit
    for (size_t i = 0; i < config_.failure_threshold; ++i) {
        manager.recordFailure(
            GPUSafeFailManager::FailureType::DEVICE_ERROR,
            "Device error"
        );
    }
    
    EXPECT_FALSE(manager.isHealthy());
    
    manager.forceHealthy();
    
    EXPECT_TRUE(manager.isHealthy());
    EXPECT_TRUE(manager.shouldAttemptGPU());
    EXPECT_EQ(manager.getHealthStatus().state, GPUSafeFailManager::GPUState::HEALTHY);
}

TEST_F(GPUSafeFailTest, ForceFailed) {
    GPUSafeFailManager manager(config_);
    
    EXPECT_TRUE(manager.isHealthy());
    
    manager.forceFailed("Maintenance mode");
    
    EXPECT_FALSE(manager.isHealthy());
    EXPECT_FALSE(manager.shouldAttemptGPU());
    EXPECT_EQ(manager.getHealthStatus().state, GPUSafeFailManager::GPUState::FAILED);
    EXPECT_EQ(manager.getHealthStatus().last_error_message, "Maintenance mode");
}

// ============================================================================
// Memory Pressure Monitor Tests
// ============================================================================

TEST_F(GPUSafeFailTest, MemoryPressureNormal) {
    size_t total = 1000 * 1024 * 1024;  // 1 GB
    MemoryPressureMonitor monitor(total);
    
    monitor.updateUsage(500 * 1024 * 1024);  // 50% usage
    
    auto status = monitor.getStatus();
    EXPECT_EQ(status.pressure, MemoryPressureMonitor::PressureLevel::NORMAL);
    EXPECT_FALSE(status.should_trigger_gc);
    EXPECT_FALSE(status.should_block_new);
    EXPECT_NEAR(status.usage_percent, 0.5f, 0.01f);
}

TEST_F(GPUSafeFailTest, MemoryPressureModerate) {
    size_t total = 1000 * 1024 * 1024;
    MemoryPressureMonitor monitor(total);
    
    monitor.updateUsage(750 * 1024 * 1024);  // 75% usage
    
    auto status = monitor.getStatus();
    EXPECT_EQ(status.pressure, MemoryPressureMonitor::PressureLevel::MODERATE);
    EXPECT_FALSE(status.should_trigger_gc);
    EXPECT_FALSE(status.should_block_new);
}

TEST_F(GPUSafeFailTest, MemoryPressureHigh) {
    size_t total = 1000 * 1024 * 1024;
    MemoryPressureMonitor monitor(total);
    
    monitor.updateUsage(900 * 1024 * 1024);  // 90% usage
    
    auto status = monitor.getStatus();
    EXPECT_EQ(status.pressure, MemoryPressureMonitor::PressureLevel::HIGH);
    EXPECT_TRUE(status.should_trigger_gc);
    EXPECT_FALSE(status.should_block_new);
}

TEST_F(GPUSafeFailTest, MemoryPressureCritical) {
    size_t total = 1000 * 1024 * 1024;
    MemoryPressureMonitor monitor(total);
    
    monitor.updateUsage(970 * 1024 * 1024);  // 97% usage
    
    auto status = monitor.getStatus();
    EXPECT_EQ(status.pressure, MemoryPressureMonitor::PressureLevel::CRITICAL);
    EXPECT_TRUE(status.should_trigger_gc);
    EXPECT_TRUE(status.should_block_new);
}

TEST_F(GPUSafeFailTest, MemoryCanAllocate) {
    size_t total = 1000 * 1024 * 1024;
    MemoryPressureMonitor monitor(total);
    
    monitor.updateUsage(500 * 1024 * 1024);  // 50% usage
    
    EXPECT_TRUE(monitor.canAllocate(100 * 1024 * 1024));  // 100 MB - OK
    EXPECT_FALSE(monitor.canAllocate(600 * 1024 * 1024)); // 600 MB - would exceed 95%
}

// ============================================================================
// Timeout Guard Tests
// ============================================================================

TEST_F(GPUSafeFailTest, TimeoutGuardNoTimeout) {
    GPUTimeoutGuard guard(std::chrono::seconds(5), "test_operation");
    
    EXPECT_FALSE(guard.hasTimedOut());
    
    guard.cancel();
    
    // Even after sleeping, cancelled guard shouldn't timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(guard.hasTimedOut());
}

TEST_F(GPUSafeFailTest, TimeoutGuardTimeout) {
    GPUTimeoutGuard guard(std::chrono::seconds(1), "test_operation");
    
    EXPECT_FALSE(guard.hasTimedOut());
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    EXPECT_TRUE(guard.hasTimedOut());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(GPUSafeFailTest, RealWorldScenario) {
    GPUSafeFailManager manager(config_);
    
    // Simulate a series of GPU operations with intermittent failures
    std::vector<bool> operation_results = {
        true, true, false,  // 2 success, 1 failure -> DEGRADED
        true, false, false, // 1 success, 2 failures -> should open circuit
        false               // 3rd failure -> CIRCUIT_OPEN
    };
    
    bool cpu_fallback_used = false;
    
    for (size_t i = 0; i < operation_results.size(); ++i) {
        bool expected_result = operation_results[i];
        
        auto gpu_op = [expected_result]() {
            return expected_result;
        };
        
        auto cpu_op = [&cpu_fallback_used]() {
            cpu_fallback_used = true;
            return true;
        };
        
        manager.executeWithFallback(gpu_op, cpu_op, "operation_" + std::to_string(i));
    }
    
    // Circuit should be open after 3 failures
    EXPECT_FALSE(manager.shouldAttemptGPU());
    EXPECT_TRUE(cpu_fallback_used);
    
    auto status = manager.getHealthStatus();
    EXPECT_EQ(status.state, GPUSafeFailManager::GPUState::CIRCUIT_OPEN);
    EXPECT_EQ(status.consecutive_failures, 3);
}
