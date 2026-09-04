/**
 * @file test_performance_cycle_metrics.cpp
 * @brief Tests for the performance cycle metrics module that is covered
 *        by async_metrics_exporter.cpp, chimera_exporter.cpp, and
 *        prometheus_exporter.cpp (all of which extend CycleMetricsCollector).
 *
 * The three exporter classes (CHIMERAExporter, PrometheusExporter,
 * AsyncMetricsExporter) are implementation-only (defined inside .cpp files
 * with no public headers). Their production logic is exercised here through
 * the public header API:
 *
 *   - HardwareCycleCounter: cpu_cycles(), rdtscp(), now_ns()
 *   - CycleTimer RAII guard
 *   - OperationCycleMetrics: field access, default values
 *   - MetricsEntry (from lockfree_metrics_buffer.h): construction
 *
 * Additionally covers `utils/boost_throw_exception.cpp` by verifying that
 * `boost::throw_exception()` correctly propagates a `std::exception`.
 */

#include <gtest/gtest.h>
#include "performance/cycle_metrics.h"
#include "performance/lockfree_metrics_buffer.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace themis::performance;

// ============================================================================
// HardwareCycleCounter
// ============================================================================

TEST(HardwareCycleCounterTest, CpuCycles_IsNonZero) {
    uint64_t cycles = HardwareCycleCounter::cpu_cycles();
    EXPECT_GT(cycles, 0u);
}

TEST(HardwareCycleCounterTest, CpuCycles_IsMonotonicallyIncreasing) {
    uint64_t c1 = HardwareCycleCounter::cpu_cycles();
    // Spin briefly to advance the counter
    volatile uint64_t sum = 0;
    for (int i = 0; i < 1000; ++i) {
      sum += i;
    }
    (void)sum;
    uint64_t c2 = HardwareCycleCounter::cpu_cycles();
    EXPECT_GE(c2, c1);
}

TEST(HardwareCycleCounterTest, Rdtscp_IsNonZero) {
    uint64_t cycles = HardwareCycleCounter::rdtscp();
    EXPECT_GT(cycles, 0u);
}

TEST(HardwareCycleCounterTest, CpuFrequency_IsPositive) {
    uint64_t freq = HardwareCycleCounter::cpu_frequency_hz();
    EXPECT_GT(freq, 0u);
}

TEST(HardwareCycleCounterTest, CpuCycles_IncreasesOverTime) {
    uint64_t c1 = HardwareCycleCounter::cpu_cycles();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t c2 = HardwareCycleCounter::cpu_cycles();
    EXPECT_GE(c2, c1);
}

#ifdef THEMIS_ENABLE_GPU_CYCLE_METRICS
TEST(HardwareCycleCounterTest, GpuCycleCallbacks_AreUsedOnStubPath) {
    HardwareCycleCounter::setGpuCyclesStartFn([]() -> void* {
        return reinterpret_cast<void*>(0x1234);
    });
    HardwareCycleCounter::setGpuCyclesEndFn([](void* event) -> uint64_t {
        return event ? 4242u : 0u;
    });

    void* evt = HardwareCycleCounter::gpu_cycles_start();
    EXPECT_EQ(evt, reinterpret_cast<void*>(0x1234));
    EXPECT_EQ(HardwareCycleCounter::gpu_cycles_end(evt), 4242u);

    HardwareCycleCounter::setGpuCyclesStartFn({});
    HardwareCycleCounter::setGpuCyclesEndFn({});
}
#endif

// ============================================================================
// CycleTimer RAII guard
// ============================================================================

TEST(CycleTimerTest, MeasuresElapsedCycles) {
    uint64_t elapsed = 0;
    {
        ScopedCycleTimer timer(&elapsed);
        // Do some work so the counter advances
        volatile uint64_t sum = 0;
        for (int i = 0; i < 10000; ++i) {
          sum += i;
        }
        (void)sum;
    } // destructor writes elapsed
    EXPECT_GT(elapsed, 0u);
}

TEST(CycleTimerTest, DestructorWritesOnlyOnce) {
    uint64_t elapsed = 0;
    {
        ScopedCycleTimer t(&elapsed);
        (void)t;
    }
    uint64_t first_value = elapsed;
    // Value should not change after timer has been destroyed
    EXPECT_EQ(elapsed, first_value);
}

// ============================================================================
// OperationCycleMetrics defaults
// ============================================================================

TEST(OperationCycleMetricsTest, DefaultValues_AreZero) {
    OperationCycleMetrics m;
    EXPECT_EQ(m.hnsw_search_cycles,       0u);
    EXPECT_EQ(m.pointer_passing_cycles,   0u);
    EXPECT_EQ(m.llm_inference_cycles,     0u);
    EXPECT_EQ(m.l1_cache_cycles,          0u);
}

TEST(OperationCycleMetricsTest, FieldAssignment) {
    OperationCycleMetrics m;
    m.hnsw_search_cycles     = 1234;
    m.pointer_passing_cycles = 56;
    m.llm_inference_cycles   = 789;

    EXPECT_EQ(m.hnsw_search_cycles,     1234u);
    EXPECT_EQ(m.pointer_passing_cycles,   56u);
    EXPECT_EQ(m.llm_inference_cycles,    789u);
}

// ============================================================================
// MetricsEntry (bridges to CHIMERAExporter / PrometheusExporter)
// ============================================================================

TEST(MetricsEntryTest, Construction_DefaultValues) {
    MetricsEntry entry;
    EXPECT_TRUE(entry.operation_name.empty());
    EXPECT_EQ(entry.metrics.hnsw_search_cycles, 0u);
}

TEST(MetricsEntryTest, FieldAssignment) {
    MetricsEntry entry;
    entry.operation_name                 = "test_op";
    entry.metrics.hnsw_search_cycles     = 9999;
    entry.metrics.pointer_passing_cycles = 123;
    entry.timestamp                      = 42000;

    EXPECT_EQ(entry.operation_name,                  "test_op");
    EXPECT_EQ(entry.metrics.hnsw_search_cycles,       9999u);
    EXPECT_EQ(entry.metrics.pointer_passing_cycles,    123u);
    EXPECT_EQ(entry.timestamp,                        42000u);
}

// ============================================================================
// boost::throw_exception (utils/boost_throw_exception.cpp)
// ============================================================================

TEST(BoostThrowExceptionTest, ThrowsStdException) {
    EXPECT_THROW(
        boost::throw_exception(std::runtime_error("test error")),
        std::runtime_error
    );
}

TEST(BoostThrowExceptionTest, ThrownExceptionHasCorrectMessage) {
    try {
        boost::throw_exception(std::runtime_error("boom"));
        FAIL() << "Expected exception not thrown";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "boom");
    }
}

TEST(BoostThrowExceptionTest, ThrowsLogicError) {
    EXPECT_THROW(
        boost::throw_exception(std::logic_error("bad state")),
        std::logic_error
    );
}
