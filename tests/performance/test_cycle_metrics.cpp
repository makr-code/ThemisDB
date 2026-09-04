#include "performance/cycle_metrics.h"
#include "performance/cycle_metrics_config.h"
#include "performance/phase4/pmu_counters.h"
#include "performance/expected_cycles.h"
#include "performance/lockfree_metrics_buffer.h"
#include "performance/runtime_config.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace themis::performance;

/**
 * @brief Test hardware cycle counter
 */
TEST(CycleMetricsTest, HardwareCycleCounter) {
    // Test that cycle counter is monotonically increasing
    uint64_t cycles1 = HardwareCycleCounter::cpu_cycles();
    uint64_t cycles2 = HardwareCycleCounter::cpu_cycles();
    uint64_t cycles3 = HardwareCycleCounter::cpu_cycles();
    
    EXPECT_GT(cycles2, cycles1);
    EXPECT_GT(cycles3, cycles2);
    
    // Test RDTSCP (serialized)
    uint64_t rdtscp1 = HardwareCycleCounter::rdtscp();
    uint64_t rdtscp2 = HardwareCycleCounter::rdtscp();
    
    EXPECT_GT(rdtscp2, rdtscp1);
}

/**
 * @brief Test CPU frequency detection
 */
TEST(CycleMetricsTest, CPUFrequency) {
    uint64_t freq = HardwareCycleCounter::cpu_frequency_hz();
    
    // Frequency should be reasonable (1-6 GHz)
    EXPECT_GT(freq, 1'000'000'000ULL);  // > 1 GHz
    EXPECT_LT(freq, 6'000'000'000ULL);  // < 6 GHz
}

/**
 * @brief Test CPU model detection
 */
TEST(CycleMetricsTest, CPUModel) {
    std::string model = HardwareCycleCounter::cpu_model();
    
    EXPECT_FALSE(model.empty());
    std::cout << "CPU Model: " << model << std::endl;
}

/**
 * @brief Test scoped cycle timer
 */
TEST(CycleMetricsTest, ScopedCycleTimer) {
    [[maybe_unused]] uint64_t cycles = 0;
    
    {
        ScopedCycleTimer timer(&cycles);
        // Do some work
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            sum += i;
        }
    }
    
    // Should have measured some cycles
        (void)cycles;
        EXPECT_GT(cycles, 0);
}

/**
 * @brief Test expected cycles calculations
 */
TEST(CycleMetricsTest, ExpectedCycles) {
    // Test deviation calculation
    EXPECT_NEAR(ExpectedCycles::deviation_percent(100, 100), 0.0, 0.01);
    EXPECT_NEAR(ExpectedCycles::deviation_percent(110, 100), 10.0, 0.01);
    EXPECT_NEAR(ExpectedCycles::deviation_percent(90, 100), -10.0, 0.01);
    
    // Test tolerance checks
    EXPECT_TRUE(ExpectedCycles::is_normal(100, 100));
    EXPECT_TRUE(ExpectedCycles::is_normal(110, 100));  // +10% is normal
    EXPECT_FALSE(ExpectedCycles::is_normal(200, 100));  // +100% is not normal
    
    EXPECT_TRUE(ExpectedCycles::is_warning(125, 100));  // +25% is warning
    EXPECT_FALSE(ExpectedCycles::is_warning(110, 100));  // +10% is not warning
    
    EXPECT_TRUE(ExpectedCycles::is_critical(200, 100));  // +100% is critical
    EXPECT_FALSE(ExpectedCycles::is_critical(120, 100));  // +20% is not critical
}

/**
 * @brief Test RAG pipeline expected values
 */
TEST(CycleMetricsTest, RAGPipelineExpectedValues) {
    // Verify pointer overhead is negligible
    EXPECT_LT(ExpectedCycles::RAGPipeline::POINTER_OVERHEAD_PERCENT, 0.001);
    
    // Verify total calculation
    uint64_t expected_total = 
        ExpectedCycles::RAGPipeline::HNSW_10K_SEARCH +
        ExpectedCycles::RAGPipeline::POINTER_PASSING_OVERHEAD +
        ExpectedCycles::RAGPipeline::LLM_10_TOKENS_7B;
    
    EXPECT_EQ(expected_total, ExpectedCycles::RAGPipeline::TOTAL);
}

/**
 * @brief Test lock-free ring buffer
 */
TEST(CycleMetricsTest, LockFreeRingBuffer) {
    LockFreeRingBuffer<int, 16> buffer;
    
    // Test empty
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    
    // Test push
    EXPECT_TRUE(buffer.tryPush(1));
    EXPECT_TRUE(buffer.tryPush(2));
    EXPECT_TRUE(buffer.tryPush(3));
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.size(), 3);
    
    // Test pop
    int value;
    EXPECT_TRUE(buffer.tryPop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(buffer.tryPop(value));
    EXPECT_EQ(value, 2);
    EXPECT_TRUE(buffer.tryPop(value));
    EXPECT_EQ(value, 3);
    
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.tryPop(value));
}

/**
 * @brief Test lock-free ring buffer overflow
 */
TEST(CycleMetricsTest, LockFreeRingBufferOverflow) {
    LockFreeRingBuffer<int, 8> buffer;
    
    // Fill buffer (capacity - 1 because of SPSC design)
    for (int i = 0; i < 7; ++i) {
        EXPECT_TRUE(buffer.tryPush(i));
    }
    
    // Next push should fail (buffer full)
    EXPECT_FALSE(buffer.tryPush(999));
    EXPECT_EQ(buffer.dropped_count(), 1);
    
    // Pop one, then push should succeed
    int value;
    EXPECT_TRUE(buffer.tryPop(value));
    EXPECT_TRUE(buffer.tryPush(777));
}

/**
 * @brief Test lock-free ring buffer with concurrent access
 */
TEST(CycleMetricsTest, LockFreeRingBufferConcurrent) {
    LockFreeRingBuffer<int, 1024> buffer;
    
    constexpr int NUM_ITEMS = 500;
    std::atomic<bool> start_flag{false};
    
    // Producer thread
    std::thread producer([&]() {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        
        for (int i = 0; i < NUM_ITEMS; ++i) {
            while (!buffer.tryPush(i)) {
                std::this_thread::yield();
            }
        }
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        
        int received = 0;
        int value = 0;
        while (received < NUM_ITEMS) {
            if (buffer.tryPop(value)) {
                EXPECT_EQ(value, received);
                ++received;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    // Start both threads
    start_flag.store(true);
    
    producer.join();
    consumer.join();
    
    EXPECT_TRUE(buffer.empty());
}

/**
 * @brief Test runtime configuration
 */
TEST(CycleMetricsTest, RuntimeConfig) {
    auto& config = RuntimeConfig::instance();
    
    // Test sampling rate
    config.setSamplingRate(1);
    EXPECT_EQ(config.getSamplingRate(), 1);
    EXPECT_TRUE(config.shouldMeasure());  // All operations
    
    config.setSamplingRate(2);
    EXPECT_EQ(config.getSamplingRate(), 2);
    
    config.setSamplingRate(0);
    EXPECT_FALSE(config.shouldMeasure());  // No operations
    
    // Test operation filtering
    config.enableOperation("test_op");
    EXPECT_TRUE(config.isOperationEnabled("test_op"));
    EXPECT_FALSE(config.isOperationEnabled("other_op"));
    
    config.disableOperation("test_op");
    EXPECT_TRUE(config.isOperationEnabled("test_op"));  // Empty filter set => all enabled
    
    config.clearOperationFilters();
    EXPECT_TRUE(config.isOperationEnabled("any_op"));  // All enabled when no filters
}

/**
 * @brief Test thread-local metrics buffer
 */
TEST(CycleMetricsTest, ThreadLocalMetricsBuffer) {
    ThreadLocalMetricsBuffer buffer;
    
    EXPECT_TRUE(buffer.empty());
    
    // Record some metrics
    OperationCycleMetrics metrics;
    metrics.hnsw_search_cycles = 1000;
    metrics.pointer_passing_cycles = 150;
    metrics.llm_inference_cycles = 50000;
    
    EXPECT_TRUE(buffer.recordOperation("test_op", metrics));
    EXPECT_FALSE(buffer.empty());
    
    // Drain buffer
    std::vector<MetricsEntry> drained;
    size_t count = buffer.drain(drained);
    
    EXPECT_EQ(count, 1);
    EXPECT_EQ(drained.size(), 1);
    EXPECT_EQ(drained[0].operation_name, "test_op");
    EXPECT_EQ(drained[0].metrics.hnsw_search_cycles, 1000);
    
    EXPECT_TRUE(buffer.empty());
}

/**
 * @brief Test zero-cost abstraction (compile-time test)
 */
TEST(CycleMetricsTest, ZeroCostAbstraction) {
    // This test verifies that macros compile without errors
    // When THEMIS_ENABLE_CYCLE_METRICS is OFF, macros should compile to nothing
    
    [[maybe_unused]] uint64_t cycles = 0;
    
    THEMIS_MEASURE_CYCLES_START(cycles);
    // Do some work
    volatile int sum = 0;
    for (int i = 0; i < 100; ++i) {
        sum += i;
    }
    THEMIS_MEASURE_CYCLES_END(cycles);
    
    THEMIS_SCOPED_CYCLE_TIMER(cycles);
    
    OperationCycleMetrics metrics;
    THEMIS_RECORD_METRICS("test", metrics);
    
    THEMIS_RECORD_FUNCTION_CYCLES("test_func", cycles);

    // PMU cache-miss macros (zero-cost when THEMIS_ENABLE_PMU_COUNTERS is OFF)
    ::themis::performance::phase4::CacheMissAnalyzer pmu_analyzer;
    ::themis::performance::phase4::CacheMissMetrics  pmu_metrics;
    THEMIS_PMU_START(pmu_analyzer);
    THEMIS_PMU_STOP(pmu_analyzer, pmu_metrics);
    THEMIS_SCOPED_CACHE_MISS_TIMER(pmu_analyzer, pmu_metrics);

    // Test should pass regardless of whether metrics are enabled
    (void)cycles;
    SUCCEED();
}

// Main removed - using GTest's main from themis_tests.exe
